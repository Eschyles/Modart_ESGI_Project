// Copyright 2022-2024 Alexander Floden, Alias Alex River. All Rights Reserved.


#include "AutoPaintWithinAreaUpToMaxSize.h"

// Engine
#include "Kismet/KismetMathLibrary.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"

// Plugin
#include "VertexPaintDetectionInterface.h"
#include "VertexPaintDetectionProfiling.h"
#include "VertexPaintDetectionLog.h"


//----------------------------------------

// Constructor

UAutoPaintWithinAreaUpToMaxSize::UAutoPaintWithinAreaUpToMaxSize() {

	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}


//----------------------------------------

// Verify Component

void UAutoPaintWithinAreaUpToMaxSize::VerifyComponent() {

	// Necessary in case you Add the Component in Runtime in BP, then immediately call to auto paint.
	if (!IsRegistered() || !HasBeenInitialized() || !IsActive()) {

		if (!IsRegistered())
			RegisterComponent();

		if (!HasBeenInitialized())
			InitializeComponent();

		if (!IsActive())
			SetActive(true);
	}
}


//----------------------------------------

// Add Auto Paint Within Sphere

void UAutoPaintWithinAreaUpToMaxSize::AddAutoPaintWithinSphere(UPrimitiveComponent* MeshComponent, FVector Location, FName Bone, FRVPDPAutoPaintUpToSphereSettings AutoPaintUpToSphereSettings) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_AddAutoPaintWithinSphere);
	RVPDP_CPUPROFILER_STR("Auto Paint Within Area Up To Max Size Component - AddAutoPaintWithinSphere");

	if (!IsValid(MeshComponent)) return;


	VerifyComponent();

	TArray<UPrimitiveComponent*> ownerComponents;
	GetOwner()->GetComponents<UPrimitiveComponent>(ownerComponents);
	if (!ownerComponents.Contains(MeshComponent)) return;



	FRVPDPAutoPaintUpToMaxSizeSettings autoPaintSettingsToAdd;
	if (AutoPaintingUpToMaxSizeOnMeshComponents.Contains(MeshComponent)) {

		autoPaintSettingsToAdd = AutoPaintingUpToMaxSizeOnMeshComponents.FindRef(MeshComponent);
	}


	FTransform sphereSpawnTransform;
	sphereSpawnTransform.SetLocation(Location);

	if (USphereComponent* spawnedSphereComponent = Cast<USphereComponent>(GetOwner()->AddComponentByClass(USphereComponent::StaticClass(), false, sphereSpawnTransform, true))) {


		spawnedSphereComponent->SetSphereRadius(AutoPaintUpToSphereSettings.StartRadius, false);
		spawnedSphereComponent->SetGenerateOverlapEvents(false);
		spawnedSphereComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
		spawnedSphereComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		spawnedSphereComponent->SetCollisionObjectType(ECC_WorldDynamic);


		FAttachmentTransformRules attachmentRules = FAttachmentTransformRules(EAttachmentRule::KeepWorld, false);

		if (spawnedSphereComponent->AttachToComponent(MeshComponent, attachmentRules, Bone)) {

			spawnedSphereComponent->SetWorldLocation(Location);
			AutoPaintUpToSphereSettings.CurrentSpreadRadius = AutoPaintUpToSphereSettings.StartRadius;
			AutoPaintUpToSphereSettings.AutoPaintSphereComponent = spawnedSphereComponent;

			autoPaintSettingsToAdd.AutoPaintSphereSettings.Add(AutoPaintUpToSphereSettings);
			AutoPaintingUpToMaxSizeOnMeshComponents.Add(MeshComponent, autoPaintSettingsToAdd);

			RVPDP_LOG(FRVPDPDebugSettings(GetWorld(), DebugAutoPaintAreaUpToLogsToScreen, DebugAutoPaintAreaUpToScreen_Duration, DebugAutoPaintAreaUpToOutputLog), FColor::Cyan, "Auto Painting in Sphere Shape on Actor %s and Mesh: %s", *MeshComponent->GetOwner()->GetName(), *MeshComponent->GetName());
		}

		else {

			RVPDP_LOG(FRVPDPDebugSettings(GetWorld(), DebugAutoPaintAreaUpToLogsToScreen, DebugAutoPaintAreaUpToScreen_Duration, DebugAutoPaintAreaUpToOutputLog), FColor::Cyan, "Failed to Attach Sphere Component to Auto Paint in Sphere Shape on Actor %s and Mesh: %s", *MeshComponent->GetOwner()->GetName(), *MeshComponent->GetName());
		}
	}

	else {

		RVPDP_LOG(FRVPDPDebugSettings(GetWorld(), DebugAutoPaintAreaUpToLogsToScreen, DebugAutoPaintAreaUpToScreen_Duration, DebugAutoPaintAreaUpToOutputLog), FColor::Cyan, "Failed to Spawn Sphere Component to Auto Paint in Sphere Shape on Actor %s and Mesh: %s", *MeshComponent->GetOwner()->GetName(), *MeshComponent->GetName());
	}
}


//----------------------------------------

// Add Auto Paint Within Rectangle

void UAutoPaintWithinAreaUpToMaxSize::AddAutoPaintWithinRectangle(UPrimitiveComponent* MeshComponent, FVector Location, FRotator Rotation, FName Bone, FRVPDPAutoPaintUpToRectangleSettings AutoPaintUpToRectangleSettings) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_AddAutoPaintWithinRectangle);
	RVPDP_CPUPROFILER_STR("Auto Paint Within Area Up To Max Size Component - AddAutoPaintWithinRectangle");

	if (!IsValid(MeshComponent)) return;


	VerifyComponent();

	TArray<UPrimitiveComponent*> ownerComponents;
	GetOwner()->GetComponents<UPrimitiveComponent>(ownerComponents);
	if (!ownerComponents.Contains(MeshComponent)) return;


	FRVPDPAutoPaintUpToMaxSizeSettings autoPaintSettingsToAdd;
	if (AutoPaintingUpToMaxSizeOnMeshComponents.Contains(MeshComponent)) {

		autoPaintSettingsToAdd = AutoPaintingUpToMaxSizeOnMeshComponents.FindRef(MeshComponent);
	}


	FTransform boxSpawnTransform;
	boxSpawnTransform.SetLocation(Location);

	if (UBoxComponent* spawnedBoxComponent = Cast<UBoxComponent>(GetOwner()->AddComponentByClass(UBoxComponent::StaticClass(), false, boxSpawnTransform, true))) {

		spawnedBoxComponent->SetBoxExtent(AutoPaintUpToRectangleSettings.StartExtent);
		spawnedBoxComponent->SetGenerateOverlapEvents(false);
		spawnedBoxComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
		spawnedBoxComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		spawnedBoxComponent->SetCollisionObjectType(ECC_WorldDynamic);


		FAttachmentTransformRules attachmentRules = FAttachmentTransformRules(EAttachmentRule::KeepWorld, false);

		if (spawnedBoxComponent->AttachToComponent(MeshComponent, attachmentRules, Bone)) {

			spawnedBoxComponent->SetWorldLocation(Location);
			spawnedBoxComponent->SetWorldRotation(Rotation);
			AutoPaintUpToRectangleSettings.AutoPaintBoxComponent = spawnedBoxComponent;

			autoPaintSettingsToAdd.AutoPaintRectangleSettings.Add(AutoPaintUpToRectangleSettings);
			AutoPaintingUpToMaxSizeOnMeshComponents.Add(MeshComponent, autoPaintSettingsToAdd);

			RVPDP_LOG(FRVPDPDebugSettings(GetWorld(), DebugAutoPaintAreaUpToLogsToScreen, DebugAutoPaintAreaUpToScreen_Duration, DebugAutoPaintAreaUpToOutputLog), FColor::Cyan, "Auto Painting in Rectangle Shape on Actor %s and Mesh: %s", *MeshComponent->GetOwner()->GetName(), *MeshComponent->GetName());
		}

		else {

			RVPDP_LOG(FRVPDPDebugSettings(GetWorld(), DebugAutoPaintAreaUpToLogsToScreen, DebugAutoPaintAreaUpToScreen_Duration, DebugAutoPaintAreaUpToOutputLog), FColor::Cyan, "Failed to Attach Box Component to Auto Paint in Rectangle Shape on Actor %s and Mesh: %s", *MeshComponent->GetOwner()->GetName(), *MeshComponent->GetName());
		}
	}

	else {

		RVPDP_LOG(FRVPDPDebugSettings(GetWorld(), DebugAutoPaintAreaUpToLogsToScreen, DebugAutoPaintAreaUpToScreen_Duration, DebugAutoPaintAreaUpToOutputLog), FColor::Cyan, "Failed to Spawn Box Component to Auto Paint in Rectangle Shape on Actor %s and Mesh: %s", *MeshComponent->GetOwner()->GetName(), *MeshComponent->GetName());
	}
}


//----------------------------------------

// Add Auto Paint Within Capsule

void UAutoPaintWithinAreaUpToMaxSize::AddAutoPaintWithinCapsule(UPrimitiveComponent* MeshComponent, FVector Location, FRotator Rotation, FName Bone, FRVPDPAutoPaintUpToCapsuleSettings AutoPaintUpToCapsuleSettings) {


	RVPDP_SCOPE_CYCLE_COUNTER(STAT_AddAutoPaintWithinCapsule);
	RVPDP_CPUPROFILER_STR("Auto Paint Within Area Up To Max Size Component - AddAutoPaintWithinCapsule");

	if (!IsValid(MeshComponent)) return;


	VerifyComponent();

	TArray<UPrimitiveComponent*> ownerComponents;
	GetOwner()->GetComponents<UPrimitiveComponent>(ownerComponents);
	if (!ownerComponents.Contains(MeshComponent)) return;


	FRVPDPAutoPaintUpToMaxSizeSettings autoPaintSettingsToAdd;
	if (AutoPaintingUpToMaxSizeOnMeshComponents.Contains(MeshComponent)) {

		autoPaintSettingsToAdd = AutoPaintingUpToMaxSizeOnMeshComponents.FindRef(MeshComponent);
	}


	FTransform capsuleSpawnTransform;
	capsuleSpawnTransform.SetLocation(Location);

	if (UCapsuleComponent* spawnedCapsuleComponent = Cast<UCapsuleComponent>(GetOwner()->AddComponentByClass(UCapsuleComponent::StaticClass(), false, capsuleSpawnTransform, true))) {

		spawnedCapsuleComponent->SetCapsuleHalfHeight(AutoPaintUpToCapsuleSettings.StartHalfHeight);
		spawnedCapsuleComponent->SetCapsuleRadius(AutoPaintUpToCapsuleSettings.StartRadius);
		spawnedCapsuleComponent->SetGenerateOverlapEvents(false);
		spawnedCapsuleComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
		spawnedCapsuleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		spawnedCapsuleComponent->SetCollisionObjectType(ECC_WorldDynamic);


		FAttachmentTransformRules attachmentRules = FAttachmentTransformRules(EAttachmentRule::KeepWorld, false);

		if (spawnedCapsuleComponent->AttachToComponent(MeshComponent, attachmentRules, Bone)) {

			spawnedCapsuleComponent->SetWorldLocation(Location);
			spawnedCapsuleComponent->SetWorldRotation(Rotation);
			AutoPaintUpToCapsuleSettings.CurrentHalfHeight = AutoPaintUpToCapsuleSettings.StartHalfHeight;
			AutoPaintUpToCapsuleSettings.CurrentRadius = AutoPaintUpToCapsuleSettings.StartRadius;
			AutoPaintUpToCapsuleSettings.AutoPaintCapsuleComponent = spawnedCapsuleComponent;

			autoPaintSettingsToAdd.AutoPaintCapsuleSettings.Add(AutoPaintUpToCapsuleSettings);
			AutoPaintingUpToMaxSizeOnMeshComponents.Add(MeshComponent, autoPaintSettingsToAdd);

			RVPDP_LOG(FRVPDPDebugSettings(GetWorld(), DebugAutoPaintAreaUpToLogsToScreen, DebugAutoPaintAreaUpToScreen_Duration, DebugAutoPaintAreaUpToOutputLog), FColor::Cyan, "Auto Painting in Capsule Shape on Actor %s and Mesh: %s", *MeshComponent->GetOwner()->GetName(), *MeshComponent->GetName());
		}

		else {

			RVPDP_LOG(FRVPDPDebugSettings(GetWorld(), DebugAutoPaintAreaUpToLogsToScreen, DebugAutoPaintAreaUpToScreen_Duration, DebugAutoPaintAreaUpToOutputLog), FColor::Cyan, "Failed to Attach Capsule Component to Auto Paint in Capsule Shape on Actor %s and Mesh: %s", *MeshComponent->GetOwner()->GetName(), *MeshComponent->GetName());
		}
	}

	else {

		RVPDP_LOG(FRVPDPDebugSettings(GetWorld(), DebugAutoPaintAreaUpToLogsToScreen, DebugAutoPaintAreaUpToScreen_Duration, DebugAutoPaintAreaUpToOutputLog), FColor::Cyan, "Failed to Spawn Capsule Component to Auto Paint in Capsule Shape on Actor %s and Mesh: %s", *MeshComponent->GetOwner()->GetName(), *MeshComponent->GetName());
	}
}


//----------------------------------------

// Update

void UAutoPaintWithinAreaUpToMaxSize::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_AutoPaintWithinAreaUpToMaxSizeTick);
	RVPDP_CPUPROFILER_STR("Auto Paint Within Area Up To Max Size Component - Tick");

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);


	TArray<UPrimitiveComponent*> meshComponents;
	AutoPaintingUpToMaxSizeOnMeshComponents.GenerateKeyArray(meshComponents);

	TArray<FRVPDPAutoPaintUpToMaxSizeSettings> meshAutoPaintUpToMaxSettings;
	AutoPaintingUpToMaxSizeOnMeshComponents.GenerateValueArray(meshAutoPaintUpToMaxSettings);

	bool invalidMesh = false;

	for (int i = meshComponents.Num() - 1; i >= 0; i--) {

		if (!IsValid(meshComponents[i])) {

			RVPDP_LOG(FRVPDPDebugSettings(GetWorld(), DebugAutoPaintAreaUpToLogsToScreen, DebugAutoPaintAreaUpToScreen_Duration, DebugAutoPaintAreaUpToOutputLog), FColor::Cyan, "Auto Painting on Mesh Component that isn't valid anymore, so will reconstruct TMap. ");

			invalidMesh = true;
			continue;
		}


		TArray<FRVPDPComponentToCheckIfIsWithinAreaInfo> componentsToCheckIfIsWithinAreaInfos;


		// Auto Paint Sphere
		for (int j = meshAutoPaintUpToMaxSettings[i].AutoPaintSphereSettings.Num() - 1; j >= 0; j--) {


			// Increases and updates the sphere component radius. If over the limit then cleans it up. 
			FRVPDPAutoPaintUpToSphereSettings autoPaintSphereSettings = meshAutoPaintUpToMaxSettings[i].AutoPaintSphereSettings[j];


			if (!autoPaintSphereSettings.OnlyIncreaseShapeSizeWhenTaskIsRun)
				autoPaintSphereSettings.CurrentSpreadRadius += autoPaintSphereSettings.ShapeIncreaseSpeed * DeltaTime;


			if (autoPaintSphereSettings.CurrentSpreadRadius >= autoPaintSphereSettings.MaxRadius) {

				RVPDP_LOG(FRVPDPDebugSettings(GetWorld(), DebugAutoPaintAreaUpToLogsToScreen, DebugAutoPaintAreaUpToScreen_Duration, DebugAutoPaintAreaUpToOutputLog), FColor::Cyan, "Auto Paint Sphere reached Max Radius so Removes it from Actor %s and Mesh %s", *meshComponents[i]->GetOwner()->GetName(), *meshComponents[i]->GetName());

				autoPaintSphereSettings.AutoPaintSphereComponent->DestroyComponent();
				meshAutoPaintUpToMaxSettings[i].AutoPaintSphereSettings.RemoveAt(j);
			}

			else if (GetTotalTasksInitiatedByComponent() <= 0) {


				if (autoPaintSphereSettings.DelayBetweenPaintTasks > 0) {


					autoPaintSphereSettings.DelayBetweenPaintTaskCurrentCounter += DeltaTime;


					if (autoPaintSphereSettings.DelayBetweenPaintTaskCurrentCounter >= autoPaintSphereSettings.DelayBetweenPaintTasks) {

						autoPaintSphereSettings.DelayBetweenPaintTaskCurrentCounter = 0;
					}

					// If haven't passed the delay between tasks then just updates the settings and skips the rest. 
					else {

						meshAutoPaintUpToMaxSettings[i].AutoPaintSphereSettings[j] = autoPaintSphereSettings;
						continue;
					}
				}


				if (autoPaintSphereSettings.OnlyIncreaseShapeSizeWhenTaskIsRun)
					autoPaintSphereSettings.CurrentSpreadRadius += autoPaintSphereSettings.ShapeIncreaseSpeed * DeltaTime;


				FRVPDPComponentToCheckIfIsWithinAreaInfo componentToCheckIfIsWithinInfo;
				componentToCheckIfIsWithinInfo.PaintWithinAreaShape = EPaintWithinAreaShape::IsSphereShape;
				componentToCheckIfIsWithinInfo.ComponentToCheckIfIsWithin = meshAutoPaintUpToMaxSettings[i].AutoPaintSphereSettings[j].AutoPaintSphereComponent;
				componentToCheckIfIsWithinInfo.FallOffSettings = meshAutoPaintUpToMaxSettings[i].AutoPaintSphereSettings[j].FallOffSettings;
				componentsToCheckIfIsWithinAreaInfos.Add(componentToCheckIfIsWithinInfo);

				autoPaintSphereSettings.AutoPaintSphereComponent->SetSphereRadius(autoPaintSphereSettings.CurrentSpreadRadius, false);
				meshAutoPaintUpToMaxSettings[i].AutoPaintSphereSettings[j] = autoPaintSphereSettings;
			}
		}


		// Auto Paint Rectangle
		for (int j = meshAutoPaintUpToMaxSettings[i].AutoPaintRectangleSettings.Num() - 1; j >= 0; j--) {


			// Increases and updates the box component extents. If over the limit then cleans it up. 
			FRVPDPAutoPaintUpToRectangleSettings autoPaintRectangleSettings = meshAutoPaintUpToMaxSettings[i].AutoPaintRectangleSettings[j];


			if (!autoPaintRectangleSettings.OnlyIncreaseShapeSizeWhenTaskIsRun)
				autoPaintRectangleSettings.CurrentExtent += autoPaintRectangleSettings.ShapeIncreaseSpeed * DeltaTime;


			if (autoPaintRectangleSettings.CurrentExtent >= autoPaintRectangleSettings.MaxExtentFromStart) {

				RVPDP_LOG(FRVPDPDebugSettings(GetWorld(), DebugAutoPaintAreaUpToLogsToScreen, DebugAutoPaintAreaUpToScreen_Duration, DebugAutoPaintAreaUpToOutputLog), FColor::Cyan, "Auto Paint Rectangle reached Max Extent so Removes it from Actor %s and Mesh %s", *meshComponents[i]->GetOwner()->GetName(), *meshComponents[i]->GetName());

				autoPaintRectangleSettings.AutoPaintBoxComponent->DestroyComponent();
				meshAutoPaintUpToMaxSettings[i].AutoPaintRectangleSettings.RemoveAt(j);
			}

			else if (GetTotalTasksInitiatedByComponent() <= 0) {


				if (autoPaintRectangleSettings.DelayBetweenPaintTasks > 0) {


					autoPaintRectangleSettings.DelayBetweenPaintTaskCurrentCounter += DeltaTime;


					if (autoPaintRectangleSettings.DelayBetweenPaintTaskCurrentCounter >= autoPaintRectangleSettings.DelayBetweenPaintTasks) {

						autoPaintRectangleSettings.DelayBetweenPaintTaskCurrentCounter = 0;
					}

					// If haven't passed the delay between tasks then just updates the settings and skips the rest. 
					else {

						meshAutoPaintUpToMaxSettings[i].AutoPaintRectangleSettings[j] = autoPaintRectangleSettings;
						continue;
					}
				}


				if (autoPaintRectangleSettings.OnlyIncreaseShapeSizeWhenTaskIsRun)
					autoPaintRectangleSettings.CurrentExtent += autoPaintRectangleSettings.ShapeIncreaseSpeed * DeltaTime;


				FRVPDPComponentToCheckIfIsWithinAreaInfo componentToCheckIfIsWithin;
				componentToCheckIfIsWithin.PaintWithinAreaShape = EPaintWithinAreaShape::IsSquareOrRectangleShape;
				componentToCheckIfIsWithin.ComponentToCheckIfIsWithin = meshAutoPaintUpToMaxSettings[i].AutoPaintRectangleSettings[j].AutoPaintBoxComponent;
				componentToCheckIfIsWithin.FallOffSettings = meshAutoPaintUpToMaxSettings[i].AutoPaintRectangleSettings[j].FallOffSettings;
				componentsToCheckIfIsWithinAreaInfos.Add(componentToCheckIfIsWithin);

				const FVector newExtent = autoPaintRectangleSettings.StartExtent + FVector(autoPaintRectangleSettings.CurrentExtent);
				autoPaintRectangleSettings.AutoPaintBoxComponent->SetBoxExtent(newExtent, false);
				meshAutoPaintUpToMaxSettings[i].AutoPaintRectangleSettings[j] = autoPaintRectangleSettings;
			}
		}


		// Auto Paint Capsule
		for (int j = meshAutoPaintUpToMaxSettings[i].AutoPaintCapsuleSettings.Num() - 1; j >= 0; j--) {


			// Increases and updates the capsule component radius & Height. If over the limit on both then cleans it up. 
			FRVPDPAutoPaintUpToCapsuleSettings autoPaintCapsuleSettings = meshAutoPaintUpToMaxSettings[i].AutoPaintCapsuleSettings[j];


			if (!autoPaintCapsuleSettings.OnlyIncreaseShapeSizeWhenTaskIsRun) {

				if (autoPaintCapsuleSettings.CurrentRadius < autoPaintCapsuleSettings.MaxRadius) {

					autoPaintCapsuleSettings.CurrentRadius += autoPaintCapsuleSettings.ShapeIncreaseSpeed * DeltaTime;
					autoPaintCapsuleSettings.CurrentRadius = UKismetMathLibrary::FClamp(autoPaintCapsuleSettings.CurrentRadius, 0, autoPaintCapsuleSettings.MaxRadius);
				}

				if (autoPaintCapsuleSettings.CurrentHalfHeight < autoPaintCapsuleSettings.MaxHalfHeight) {

					autoPaintCapsuleSettings.CurrentHalfHeight += autoPaintCapsuleSettings.ShapeIncreaseSpeed * DeltaTime;
					autoPaintCapsuleSettings.CurrentHalfHeight = UKismetMathLibrary::FClamp(autoPaintCapsuleSettings.CurrentHalfHeight, 0, autoPaintCapsuleSettings.MaxHalfHeight);
				}
			}


			if (autoPaintCapsuleSettings.CurrentRadius >= autoPaintCapsuleSettings.MaxRadius && autoPaintCapsuleSettings.CurrentHalfHeight >= autoPaintCapsuleSettings.MaxHalfHeight) {

				RVPDP_LOG(FRVPDPDebugSettings(GetWorld(), DebugAutoPaintAreaUpToLogsToScreen, DebugAutoPaintAreaUpToScreen_Duration, DebugAutoPaintAreaUpToOutputLog), FColor::Cyan, "Auto Paint Capsule reached Max Radius and Half Height so Removes it from Actor %s and Mesh %s", *meshComponents[i]->GetOwner()->GetName(), *meshComponents[i]->GetName());

				autoPaintCapsuleSettings.AutoPaintCapsuleComponent->DestroyComponent();
				meshAutoPaintUpToMaxSettings[i].AutoPaintCapsuleSettings.RemoveAt(j);
			}

			else if (GetTotalTasksInitiatedByComponent() <= 0) {

				
				if (autoPaintCapsuleSettings.DelayBetweenPaintTasks > 0) {


					autoPaintCapsuleSettings.DelayBetweenPaintTaskCurrentCounter += DeltaTime;


					if (autoPaintCapsuleSettings.DelayBetweenPaintTaskCurrentCounter >= autoPaintCapsuleSettings.DelayBetweenPaintTasks) {

						autoPaintCapsuleSettings.DelayBetweenPaintTaskCurrentCounter = 0;
					}

					// If haven't passed the delay between tasks then just updates the settings and skips the rest. 
					else {

						meshAutoPaintUpToMaxSettings[i].AutoPaintCapsuleSettings[j] = autoPaintCapsuleSettings;
						continue;
					}
				}


				if (autoPaintCapsuleSettings.OnlyIncreaseShapeSizeWhenTaskIsRun) {

					if (autoPaintCapsuleSettings.CurrentRadius < autoPaintCapsuleSettings.MaxRadius) {

						autoPaintCapsuleSettings.CurrentRadius += autoPaintCapsuleSettings.ShapeIncreaseSpeed * DeltaTime;
						autoPaintCapsuleSettings.CurrentRadius = UKismetMathLibrary::FClamp(autoPaintCapsuleSettings.CurrentRadius, 0, autoPaintCapsuleSettings.MaxRadius);
					}

					if (autoPaintCapsuleSettings.CurrentHalfHeight < autoPaintCapsuleSettings.MaxHalfHeight) {

						autoPaintCapsuleSettings.CurrentHalfHeight += autoPaintCapsuleSettings.ShapeIncreaseSpeed * DeltaTime;
						autoPaintCapsuleSettings.CurrentHalfHeight = UKismetMathLibrary::FClamp(autoPaintCapsuleSettings.CurrentHalfHeight, 0, autoPaintCapsuleSettings.MaxHalfHeight);
					}
				}


				FRVPDPComponentToCheckIfIsWithinAreaInfo componentToCheckIfIsWithin;
				componentToCheckIfIsWithin.PaintWithinAreaShape = EPaintWithinAreaShape::IsCapsuleShape;
				componentToCheckIfIsWithin.ComponentToCheckIfIsWithin = meshAutoPaintUpToMaxSettings[i].AutoPaintCapsuleSettings[j].AutoPaintCapsuleComponent;
				componentToCheckIfIsWithin.FallOffSettings = meshAutoPaintUpToMaxSettings[i].AutoPaintCapsuleSettings[j].FallOffSettings;
				componentsToCheckIfIsWithinAreaInfos.Add(componentToCheckIfIsWithin);

				autoPaintCapsuleSettings.AutoPaintCapsuleComponent->SetCapsuleHalfHeight(autoPaintCapsuleSettings.CurrentHalfHeight, false);
				autoPaintCapsuleSettings.AutoPaintCapsuleComponent->SetCapsuleRadius(autoPaintCapsuleSettings.CurrentRadius, false);
				meshAutoPaintUpToMaxSettings[i].AutoPaintCapsuleSettings[j] = autoPaintCapsuleSettings;
			}
		}


		// If no Paint Within Area Shapes left
		if (meshAutoPaintUpToMaxSettings[i].AutoPaintSphereSettings.Num() == 0 && meshAutoPaintUpToMaxSettings[i].AutoPaintRectangleSettings.Num() == 0 && meshAutoPaintUpToMaxSettings[i].AutoPaintCapsuleSettings.Num() == 0) {

			AutoPaintingUpToMaxSizeOnMeshComponents.Remove(meshComponents[i]);
		}

		else {

			AutoPaintingUpToMaxSizeOnMeshComponents.Add(meshComponents[i], meshAutoPaintUpToMaxSettings[i]);

			if (componentsToCheckIfIsWithinAreaInfos.Num() > 0) {

				FRVPDPPaintWithinAreaSettings paintWithinAreaSettings;
				paintWithinAreaSettings.MeshComponent = meshComponents[i];
				paintWithinAreaSettings.ApplyVertexColorSettings = ApplyVertexColorSettings;
				paintWithinAreaSettings.DebugSettings = TaskDebugSettings;
				paintWithinAreaSettings.WithinAreaSettings.ComponentsToCheckIfIsWithin = componentsToCheckIfIsWithinAreaInfos;
				paintWithinAreaSettings.WithinAreaSettings.TraceForSpecificBonesWithinArea = true;

				PaintOnMeshWithinArea(paintWithinAreaSettings, FRVPDPAdditionalDataToPassThroughInfo());
			}
		}
	}

	
	if (AutoPaintingUpToMaxSizeOnMeshComponents.Num() == 0) {

		RVPDP_LOG(FRVPDPDebugSettings(GetWorld(), DebugAutoPaintAreaUpToLogsToScreen, DebugAutoPaintAreaUpToScreen_Duration, DebugAutoPaintAreaUpToOutputLog), FColor::Cyan, "No more Auto Paintings on Actor %s so Destroys Auto Paint Within Area Up To Max Size Component! ", *GetOwner()->GetName());

		DestroyComponent();
		return;
	}
	

	if (invalidMesh) {


		RVPDP_LOG(FRVPDPDebugSettings(GetWorld(), DebugAutoPaintAreaUpToLogsToScreen, DebugAutoPaintAreaUpToScreen_Duration, DebugAutoPaintAreaUpToOutputLog), FColor::Cyan, "One Mesh we're Auto Painting Within Area on got Invalid, so we reconstruct the TMap. ");


		TArray<UPrimitiveComponent*> meshes;
		AutoPaintingUpToMaxSizeOnMeshComponents.GenerateKeyArray(meshComponents);

		TArray<FRVPDPAutoPaintUpToMaxSizeSettings> autoPaintUpToMaxSizeSettings;
		AutoPaintingUpToMaxSizeOnMeshComponents.GenerateValueArray(meshAutoPaintUpToMaxSettings);

		AutoPaintingUpToMaxSizeOnMeshComponents.Empty();


		for (int i = meshes.Num() - 1; i >= 0; i--) {

			if (IsValid(meshes[i])) {

				AutoPaintingUpToMaxSizeOnMeshComponents.Add(meshes[i], autoPaintUpToMaxSizeSettings[i]);
			}
		}
	}
}


//----------------------------------------

// Stop All Auto Painting

void UAutoPaintWithinAreaUpToMaxSize::StopAllAutoPainting() {


	RVPDP_LOG(FRVPDPDebugSettings(GetWorld(), DebugAutoPaintAreaUpToLogsToScreen, DebugAutoPaintAreaUpToScreen_Duration, DebugAutoPaintAreaUpToOutputLog), FColor::Cyan, "Stop Auto Painting Within Area Up to Max Size so Component in Actor %s Destroys itself!", *GetOwner()->GetName());

	DestroyComponent();
}


//----------------------------------------

// On Component Destroyed

void UAutoPaintWithinAreaUpToMaxSize::OnComponentDestroyed(bool bDestroyingHierarchy) {

	Super::OnComponentDestroyed(bDestroyingHierarchy);


	TArray<UPrimitiveComponent*> meshComponents;
	AutoPaintingUpToMaxSizeOnMeshComponents.GenerateKeyArray(meshComponents);

	TArray<FRVPDPAutoPaintUpToMaxSizeSettings> meshAutoPaintUpToMaxSettings;
	AutoPaintingUpToMaxSizeOnMeshComponents.GenerateValueArray(meshAutoPaintUpToMaxSettings);


	for (auto& autoPaintUpToMaxSize : AutoPaintingUpToMaxSizeOnMeshComponents) {


		if (autoPaintUpToMaxSize.Value.AutoPaintSphereSettings.Num() > 0) {

			for (FRVPDPAutoPaintUpToSphereSettings& autoPaintSphereSettings : autoPaintUpToMaxSize.Value.AutoPaintSphereSettings) {

				if (IsValid(autoPaintSphereSettings.AutoPaintSphereComponent))
					autoPaintSphereSettings.AutoPaintSphereComponent->DestroyComponent();
			}
		}

		if (autoPaintUpToMaxSize.Value.AutoPaintCapsuleSettings.Num() > 0) {

			for (FRVPDPAutoPaintUpToCapsuleSettings& autoPaintCapsuleSettings : autoPaintUpToMaxSize.Value.AutoPaintCapsuleSettings) {

				if (IsValid(autoPaintCapsuleSettings.AutoPaintCapsuleComponent))
					autoPaintCapsuleSettings.AutoPaintCapsuleComponent->DestroyComponent();
			}
		}

		if (autoPaintUpToMaxSize.Value.AutoPaintRectangleSettings.Num() > 0) {

			for (FRVPDPAutoPaintUpToRectangleSettings& autoPaintRectangleSettings : autoPaintUpToMaxSize.Value.AutoPaintRectangleSettings) {

				if (IsValid(autoPaintRectangleSettings.AutoPaintBoxComponent))
					autoPaintRectangleSettings.AutoPaintBoxComponent->DestroyComponent();
			}
		}
	}

	AutoPaintingUpToMaxSizeOnMeshComponents.Empty();
}
