// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 


#include "VertexPaintClothOverlapComponent.h"

// Engine 
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/SkeletalMesh.h"
#include "Components/SkeletalMeshComponent.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "ClothingAsset.h"
#include <Net/UnrealNetwork.h>
#include "DrawDebugHelpers.h"

// Plugin
#include "VertexPaintDetectionInterface.h"
#include "VertexPaintDetectionProfiling.h"


//-------------------------------------------------------

// Construct

UVertexPaintClothOverlapComponent::UVertexPaintClothOverlapComponent() {

	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}


//-------------------------------------------------------

// GetLifetimeReplicatedProps

void UVertexPaintClothOverlapComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UVertexPaintClothOverlapComponent, ClothOverlapTracingEnabled);
}


//-------------------------------------------------------

// Start

void UVertexPaintClothOverlapComponent::BeginPlay() {

	Super::BeginPlay();


	AttachedSkeletalMeshComponent = Cast<USkeletalMeshComponent>(GetAttachParent());

	if (IsValid(AttachedSkeletalMeshComponent) && ClothOverlapTracingEnabled) {

		ActivateClothOverlapTracing();
	}

	else {

		DeActivateClothOverlapTracing();
	}
}


//-------------------------------------------------------

// Update

void UVertexPaintClothOverlapComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_ClothOverlapComponentTick);
	RVPDP_CPUPROFILER_STR("Cloth Overlap Component - Tick");


	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!IsValid(AttachedSkeletalMeshComponent)) return;
	if (ClothObjectsToSphereTrace.Num() == 0) return;
	if (!ClothOverlapTracingEnabled) return;


	if (GetOwner()->HasAuthority()) {

		if (AttachedSkeletalMeshComponent) {

			const FSkeletalMeshLODRenderData& skelMeshRenderData = AttachedSkeletalMeshComponent->GetSkeletalMeshRenderData()->LODRenderData[0];

			// If cloth simulation is de-activated then disables cloth overlapping
			if (!skelMeshRenderData.HasClothData()) {

				SetClothOverlapTracingEnabled(false);
				return;
			}
		}

		// If Dedicated Server then doesn't do any actual traces since the cloth isn't actually moving for them
		if (UGameplayStatics::GetPlayerController(GetWorld(), 0)) {

			if (!UGameplayStatics::GetPlayerController(GetWorld(), 0)->IsLocalPlayerController())
				return;
		}
	}


	TMap<int32, FClothSimulData> clothingData;

	if (IsInGameThread())
		clothingData = AttachedSkeletalMeshComponent->GetCurrentClothingData_GameThread();
	else
		clothingData = AttachedSkeletalMeshComponent->GetCurrentClothingData_AnyThread();

	if (clothingData.Num() == 0) return;



	ClothTraceCurrentCallbacks = 0;
	ClothTraceExpectedCallbacks = 0;

	FCollisionQueryParams queryParams;
	queryParams.AddIgnoredActor(GetOwner());

	FCollisionObjectQueryParams collisionQueryParams;

	for (auto objectType : ClothObjectsToSphereTrace)
		collisionQueryParams.AddObjectTypesToQuery(UEngineTypes::ConvertToCollisionChannel(objectType));


	for (auto& currentClothData : clothingData) {

		for (int i = 0; i < currentClothData.Value.Positions.Num(); i += FMath::Clamp(ClothVertexTraceInterval, 1, ClothVertexTraceInterval)) {


#if ENGINE_MAJOR_VERSION == 4

			FVector clothVertexPos = currentClothData.Value.Positions[i];


			// UE5 with Chaos Cloth required a conversion of the cloth positions and rotation to get them aligned to the bone the cloth is attached to
#elif ENGINE_MAJOR_VERSION == 5

			FVector clothVertexPos = static_cast<FVector>(currentClothData.Value.Positions[i]);

			

			// Converts Cloth Component Position so it aligns Location and Rotation Wise correctly with bone it's attached to.
			if (ClothBoneTransformsInComponentSpace.Contains(AttachedSkeletalMeshComponentClothingAssets[currentClothData.Key])) {

				FQuat clothQuat = ClothBoneQuaternionsInComponentSpace.FindRef(AttachedSkeletalMeshComponentClothingAssets[currentClothData.Key]);


				// Rotates the Vector according to the bone it's attached to
				clothVertexPos = clothQuat.RotateVector(clothVertexPos);


				// Since the Vertex Positions we get from Cloth is component 0, 0, 0 we had to add the bone component local here to get it at the right Location, so we now have the correct rotation and Location for each cloth vertex
				clothVertexPos = clothVertexPos + ClothBoneTransformsInComponentSpace.FindRef(AttachedSkeletalMeshComponentClothingAssets[currentClothData.Key]).GetLocation();
			}
#endif


			clothVertexPos = UKismetMathLibrary::TransformLocation(AttachedSkeletalMeshComponent->GetComponentTransform(), clothVertexPos);

			FTraceDelegate sphereDelegate;
			sphereDelegate.BindUObject(this, &UVertexPaintClothOverlapComponent::OnSphereTraceComplete);

			GetWorld()->AsyncSweepByObjectType(EAsyncTraceType::Single, clothVertexPos, clothVertexPos, FQuat::Identity, collisionQueryParams, FCollisionShape::MakeSphere(ClothVertexTraceRadius), queryParams, &sphereDelegate);

			ClothTraceExpectedCallbacks++;
		}
	}
}


//-------------------------------------------------------

// On Sphere Trace Complete

void UVertexPaintClothOverlapComponent::OnSphereTraceComplete(const FTraceHandle& TraceHandle, FTraceDatum& TraceData) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_OnSphereTraceComplete);
	RVPDP_CPUPROFILER_STR("Cloth Overlap Component - OnSphereTraceComplete");

	ClothTraceCurrentCallbacks++;


	if (TraceData.OutHits.Num() > 0) {

		for (int i = 0; i < TraceData.OutHits.Num(); i++) {

			if (TraceData.OutHits[i].bBlockingHit) {


				if (DebugClothSphereTraces)
					DrawDebugSphere(GetWorld(), TraceData.OutHits[i].TraceStart, ClothVertexTraceRadius, 6, FColor::Green, false, DebugClothSphereTracesDuration, 0, 1);


				if (TraceData.OutHits[i].Component.Get()) {

					// If the Mesh the Cloth got trace hit on, is set to Overlap the Skeletal Mesh the Cloth is under 
					if (TraceData.OutHits[i].Component.Get()->GetCollisionResponseToChannel(AttachedSkeletalMeshComponent->GetCollisionObjectType()) == ECollisionResponse::ECR_Overlap) {

						CurrentTraceOverlappingComponentAndItemsCache.Add(TraceData.OutHits[i].Component.Get(), TraceData.OutHits[i].Item);
					}
				}
			}
		}
	}

	else {

		if (DebugClothSphereTraces)
			DrawDebugSphere(GetWorld(), TraceData.Start, ClothVertexTraceRadius, 6, FColor::Red, false, DebugClothSphereTracesDuration, 0, 1);
	}



	// When gotten all callbacks from all cloths we can safely check what we've started and stopped overlapping. 
	if (ClothTraceCurrentCallbacks < ClothTraceExpectedCallbacks) return;



	// If current overlapped components doesn't contain the cached one, then runs End Overlap on the cached one
	for (auto& cachedOverlappingComponentAndItem : ClothOverlappingComponentAndItemsCache) {


		if (cachedOverlappingComponentAndItem.Key && IsValid(cachedOverlappingComponentAndItem.Key->GetOwner())) {

			if (!CurrentTraceOverlappingComponentAndItemsCache.Contains(cachedOverlappingComponentAndItem.Key)) {

				if (UKismetSystemLibrary::DoesImplementInterface(cachedOverlappingComponentAndItem.Key->GetOwner(), UVertexPaintDetectionInterface::StaticClass())) {

					IVertexPaintDetectionInterface::Execute_ClothEndOverlappingMesh(cachedOverlappingComponentAndItem.Key->GetOwner(), cachedOverlappingComponentAndItem.Key, GetOwner(), AttachedSkeletalMeshComponent, cachedOverlappingComponentAndItem.Value);
				}
			}
		}
	}


	// If the cached ones doesn't contain a current overlapping, then runs begin overlap
	for (auto& currentOverlappingComps : CurrentTraceOverlappingComponentAndItemsCache) {


		if (!ClothOverlappingComponentAndItemsCache.Contains(currentOverlappingComps.Key)) {

			if (UKismetSystemLibrary::DoesImplementInterface(currentOverlappingComps.Key->GetOwner(), UVertexPaintDetectionInterface::StaticClass()))
				IVertexPaintDetectionInterface::Execute_ClothBeginOverlappingMesh(currentOverlappingComps.Key->GetOwner(), currentOverlappingComps.Key, GetOwner(), AttachedSkeletalMeshComponent, currentOverlappingComps.Value);
		}
	}


	// Updates the cache with the latest overlapping components and items
	ClothOverlappingComponentAndItemsCache = CurrentTraceOverlappingComponentAndItemsCache;

	// Resets global properties used for the tracing
	CurrentTraceOverlappingComponentAndItemsCache.Empty();
	ClothTraceCurrentCallbacks = 0;
	ClothTraceExpectedCallbacks = 0;
}


//-------------------------------------------------------

// Set Enable Cloth Overlap Tracing

void UVertexPaintClothOverlapComponent::SetClothOverlapTracingEnabled(bool EnableClothTracing) {


	ClothOverlapTracingEnabled = EnableClothTracing;

	// If server then the on rep won't run so we call the functions right away. 
	if (GetOwner()->HasAuthority()) {

		if (ClothOverlapTracingEnabled)
			ActivateClothOverlapTracing();
		else
			DeActivateClothOverlapTracing();
	}
}


//-------------------------------------------------------

// OnRep_Enable Cloth Overlap Tracing Rep

void UVertexPaintClothOverlapComponent::OnRep_ClothOverlapTracingEnabled() {

	if (ClothOverlapTracingEnabled)
		ActivateClothOverlapTracing();
	else
		DeActivateClothOverlapTracing();
}


//-------------------------------------------------------

// Activate Cloth Overlap Tracing

void UVertexPaintClothOverlapComponent::ActivateClothOverlapTracing() {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_ActivateClothOverlapTracing);
	RVPDP_CPUPROFILER_STR("Cloth Overlap Component - ActivateClothOverlapTracing");

	if (!AttachedSkeletalMeshComponent && ClothOverlapTracingEnabled) return;


	const FSkeletalMeshLODRenderData& skelMeshRenderData = AttachedSkeletalMeshComponent->GetSkeletalMeshRenderData()->LODRenderData[0];

	if (skelMeshRenderData.HasClothData()) {

		USkeletalMesh* skelMesh = nullptr;

#if ENGINE_MAJOR_VERSION == 4

		skelMesh = AttachedSkeletalMeshComponent->SkeletalMesh;

#elif ENGINE_MAJOR_VERSION == 5

	#if ENGINE_MINOR_VERSION == 0

		skelMesh = AttachedSkeletalMeshComponent->SkeletalMesh.Get();

	#else

		skelMesh = AttachedSkeletalMeshComponent->GetSkeletalMeshAsset();

	#endif
#endif



		ClothBoneTransformsInComponentSpace.Empty();
		ClothBoneQuaternionsInComponentSpace.Empty();

		AttachedSkeletalMeshComponentClothingAssets = skelMesh->GetMeshClothingAssets();

		for (UClothingAssetBase* clothAsset : AttachedSkeletalMeshComponentClothingAssets) {

			if (UClothingAssetCommon* clothingAssetCommon = Cast<UClothingAssetCommon>(clothAsset)) {

				if (clothingAssetCommon->UsedBoneNames.IsValidIndex(0)) {

					// Get Bone Transform in World Space
					FTransform boneTransform = AttachedSkeletalMeshComponent->GetBoneTransform(AttachedSkeletalMeshComponent->GetBoneIndex(clothingAssetCommon->UsedBoneNames[0]));

					// Transforms from World to Component Local
					boneTransform.SetLocation(UKismetMathLibrary::InverseTransformLocation(AttachedSkeletalMeshComponent->GetComponentTransform(), boneTransform.GetLocation()));
					boneTransform.SetRotation(static_cast<FQuat>(UKismetMathLibrary::InverseTransformRotation(AttachedSkeletalMeshComponent->GetComponentTransform(), boneTransform.GetRotation().Rotator())));


					ClothBoneTransformsInComponentSpace.Add(clothAsset, boneTransform);

					ClothBoneQuaternionsInComponentSpace.Add(clothAsset, AttachedSkeletalMeshComponent->GetBoneQuaternion(clothingAssetCommon->UsedBoneNames[0], EBoneSpaces::ComponentSpace));
				}
			}
		}


		SetComponentTickEnabled(true);
	}
}


//-------------------------------------------------------

// DeActivate Cloth Overlap Tracing

void UVertexPaintClothOverlapComponent::DeActivateClothOverlapTracing() {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_DeActivateClothOverlapTracing);
	RVPDP_CPUPROFILER_STR("Cloth Overlap Component - DeActivateClothOverlapTracing");

	if (!AttachedSkeletalMeshComponent) return;


	ClothBoneTransformsInComponentSpace.Empty();
	ClothBoneQuaternionsInComponentSpace.Empty();
	AttachedSkeletalMeshComponentClothingAssets.Empty();
	ClothTraceExpectedCallbacks = 0;
	ClothTraceCurrentCallbacks = 0;


	for (auto& currentOverlappingComps : ClothOverlappingComponentAndItemsCache) {


		if (UKismetSystemLibrary::DoesImplementInterface(currentOverlappingComps.Key->GetOwner(), UVertexPaintDetectionInterface::StaticClass()))
			IVertexPaintDetectionInterface::Execute_ClothEndOverlappingMesh(currentOverlappingComps.Key->GetOwner(), currentOverlappingComps.Key, GetOwner(), AttachedSkeletalMeshComponent, currentOverlappingComps.Value);
	}

	ClothOverlappingComponentAndItemsCache.Empty();

	SetComponentTickEnabled(false);
}

