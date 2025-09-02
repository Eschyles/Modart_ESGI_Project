// Copyright 2022-2024 Alexander Floden, Alias Alex River. All Rights Reserved.


#include "AutoAddColorWithinAreaComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"

// Plugin
#include "VertexPaintDetectionLog.h"
#include "VertexPaintDetectionProfiling.h"



//-------------------------------------------------------

// Begin Play

void UAutoAddColorWithinAreaComponent::BeginPlay() {

	Super::BeginPlay();


	// Need to make sure we also start paint jobs if the owner is moving, in case we use the optimization options so we don't start new tasks on meshes that are still, or no change has simple not happened so no tasks are currently running for a mesh. So if the owner is moving toward or away from it we need to trigger new tasks to reflect where the owner and the area is now. 
	if (USceneComponent* ownerRootComp = GetOwner()->GetRootComponent()) {

		auto transformUpdated = [this](USceneComponent* UpdatedComponent, EUpdateTransformFlags UpdateTransformFlag, ETeleportType Teleport)->void {


			bool verifyCachedMeshes = false;
			TArray<UPrimitiveComponent*> meshComponentsToCheck;
			AutoPaintingWithinAreaWithSettings.GetKeys(meshComponentsToCheck);


			for (int i = meshComponentsToCheck.Num() - 1; i >= 0; i--) {

				if (!IsValid(meshComponentsToCheck[i])) {

					verifyCachedMeshes = true;
					continue;
				}

				if (!IsMeshBeingPaintedOrIsQueuedUp(meshComponentsToCheck[i])) {

					StartAutoPaintTask(meshComponentsToCheck[i]);
				}
			}

			if (verifyCachedMeshes)
				VerifyAutoPaintingMeshComponents();
		};


		ownerRootComp->TransformUpdated.AddLambda(transformUpdated);
	}
}


//-------------------------------------------------------

// Add Auto Paint Within Area

bool UAutoAddColorWithinAreaComponent::AddAutoPaintWithinArea(UPrimitiveComponent* MeshComponent, FRVPDPPaintWithinAreaSettings PaintWithinAreaSettings, FRVPDPAdditionalDataToPassThroughInfo AdditionalData, FRVPDPAutoAddColorSettings AutoAddColorSettings, bool ResumeIfPaused) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_AddAutoPaintWithinArea);
	RVPDP_CPUPROFILER_STR("Auto Add Within Area Component - AddAutoPaintWithinArea");

	// Note doesn't check if already in TMap since we want to be able to Update the Paint Settings

	PaintWithinAreaSettings.MeshComponent = MeshComponent;
	PaintWithinAreaSettings.WeakMeshComponent = MeshComponent;
	PaintWithinAreaSettings.TaskWorld = GetWorld();
	PaintWithinAreaSettings.DebugSettings.WorldTaskWasCreatedIn = GetWorld();
	DebugSettings.WorldTaskWasCreatedIn = GetWorld();
	PaintWithinAreaSettings.WithinAreaSettings.ComponentsToCheckIfIsWithin = GetWithinAreaComponentsToCheckIfIsWithinSettings(PaintWithinAreaSettings.GetMeshComponent(), PaintWithinAreaSettings.WithinAreaSettings);

	// Since we may run this to Update Paint Settings, and a task may already be going, we don't want the Check Valid to fail if it's already painting in this specific instance or because of a Task Queue limit, we always want to be able to Add/Update it at least. Then it may fail when it actually start because of the task queue limit until it goes down. 
	if (!PaintOnMeshWithinAreaCheckValid(PaintWithinAreaSettings, true)) return false;


	AdjustCallbackSettings(PaintWithinAreaSettings.ApplyVertexColorSettings, PaintWithinAreaSettings.CallbackSettings);


	FRVPDPAutoAddColorSettings autoAddColorSettingsToApply = AutoAddColorSettings;

	// If already added then we don't want cached stuff like IsPaused etc. to get replaced but only exposed changeable properties. 
	if (AutoPaintingMeshes.Contains(PaintWithinAreaSettings.GetMeshComponent())) {

		autoAddColorSettingsToApply = AutoPaintingMeshes.FindRef(PaintWithinAreaSettings.GetMeshComponent());
		autoAddColorSettingsToApply.UpdateExposedProperties(AutoAddColorSettings.CanEverGetPaused, AutoAddColorSettings.DelayBetweenTasks, AutoAddColorSettings.RemoveMeshFromAutoPaintingIfFullyPainted, AutoAddColorSettings.RemoveMeshFromAutoPaintingIfCompletelyEmpty, AutoAddColorSettings.OnlyStartNewTaskIfChangeWasMadeByOwningPaintComponent);
	}


	AutoPaintingWithinAreaWithSettings.Add(PaintWithinAreaSettings.GetMeshComponent(), PaintWithinAreaSettings);
	AutoPaintingWithinAreaAdditionalDataSettings.Add(PaintWithinAreaSettings.GetMeshComponent(), AdditionalData);

	if (ResumeIfPaused)
		autoAddColorSettingsToApply.IsPaused = false;


	MeshAddedToBeAutoPainted(PaintWithinAreaSettings.GetMeshComponent(), autoAddColorSettingsToApply);


	if (autoAddColorSettingsToApply.IsPaused) {

		RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s - Paused Mesh: %s in Actor: %s got it's Within Area Settings Updated!", *GetName(), *MeshComponent->GetName(), *MeshComponent->GetOwner()->GetName());
	}

	else {

		RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s - Mesh: %s in Actor: %s Has been Added/Updated to be Auto Painted Within Area!  ", *GetName(), *MeshComponent->GetName(), *MeshComponent->GetOwner()->GetName());
	}


	StartNewPaintTaskIfAllowed(MeshComponent, autoAddColorSettingsToApply);

	return true;
}


//-------------------------------------------------------

// Update Auto Painted Within Area

void UAutoAddColorWithinAreaComponent::UpdateAutoPaintedWithinArea(UPrimitiveComponent* MeshComponent, FRVPDPPaintWithinAreaSettings PaintWithinAreaSettings, FRVPDPAdditionalDataToPassThroughInfo AdditionalData, FRVPDPAutoAddColorSettings AutoAddColorSettings, bool ResumeIfPaused) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_UpdateAutoPaintedWithinArea);
	RVPDP_CPUPROFILER_STR("Auto Add Within Area Component - UpdateAutoPaintedWithinArea");

	if (!MeshComponent) return;
	if (MeshComponent->IsBeingDestroyed()) return;
	if (!AutoPaintingWithinAreaWithSettings.Contains(MeshComponent)) return;

	PaintWithinAreaSettings.MeshComponent = MeshComponent;
	PaintWithinAreaSettings.WeakMeshComponent = MeshComponent;
	PaintWithinAreaSettings.TaskWorld = GetWorld();
	PaintWithinAreaSettings.DebugSettings.WorldTaskWasCreatedIn = GetWorld();
	DebugSettings.WorldTaskWasCreatedIn = GetWorld();
	PaintWithinAreaSettings.WithinAreaSettings.ComponentsToCheckIfIsWithin = GetWithinAreaComponentsToCheckIfIsWithinSettings(PaintWithinAreaSettings.GetMeshComponent(), PaintWithinAreaSettings.WithinAreaSettings);


	// Since we may run this to Update Paint Settings, and a task may already be going, we don't want the Check Valid to fail if it's already painting in this specific instance or because of a Task Queue limit, we always want to be able to Add/Update it at least. Then it may fail when it actually start because of the task queue limit until it goes down. 
	if (!PaintOnMeshWithinAreaCheckValid(PaintWithinAreaSettings, true)) {

		// So if the user was trying to update it to invalid paint settings, then rather than have the task continue to be auto painted with something the user was trying to change away from we instead stop auto painting it. 
		StopAutoPaintingMesh(MeshComponent);
		return;
	}


	AdjustCallbackSettings(PaintWithinAreaSettings.ApplyVertexColorSettings, PaintWithinAreaSettings.CallbackSettings);


	FRVPDPAutoAddColorSettings autoAddColorSettingsToApply = AutoAddColorSettings;

	// If already added then we don't want cached stuff like IsPaused etc. to get replaced but only exposed changeable properties. 
	if (AutoPaintingMeshes.Contains(PaintWithinAreaSettings.GetMeshComponent())) {

		autoAddColorSettingsToApply = AutoPaintingMeshes.FindRef(PaintWithinAreaSettings.GetMeshComponent());
		autoAddColorSettingsToApply.UpdateExposedProperties(AutoAddColorSettings.CanEverGetPaused, AutoAddColorSettings.DelayBetweenTasks, AutoAddColorSettings.RemoveMeshFromAutoPaintingIfFullyPainted, AutoAddColorSettings.RemoveMeshFromAutoPaintingIfCompletelyEmpty, AutoAddColorSettings.OnlyStartNewTaskIfChangeWasMadeByOwningPaintComponent);
	}


	AutoPaintingWithinAreaWithSettings.Add(PaintWithinAreaSettings.GetMeshComponent(), PaintWithinAreaSettings);
	AutoPaintingWithinAreaAdditionalDataSettings.Add(PaintWithinAreaSettings.GetMeshComponent(), AdditionalData);

	if (ResumeIfPaused)
		autoAddColorSettingsToApply.IsPaused = false;


	MeshAddedToBeAutoPainted(PaintWithinAreaSettings.GetMeshComponent(), autoAddColorSettingsToApply);


	if (autoAddColorSettingsToApply.IsPaused) {

		RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s - Paused Mesh: %s in Actor: %s got it's Within Area Settings Updated!", *GetName(), *MeshComponent->GetName(), *MeshComponent->GetOwner()->GetName());
	}

	else {

		RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s - Mesh: %s in Actor: %s got it's Auto Paint Within Area Settings Updated! ", *GetName(), *MeshComponent->GetName(), *MeshComponent->GetOwner()->GetName());
	}
}


//-------------------------------------------------------

// Start Auto Paint Task

void UAutoAddColorWithinAreaComponent::StartAutoPaintTask(UPrimitiveComponent* meshComponent) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_StartAutoPaintTask);
	RVPDP_CPUPROFILER_STR("Auto Add Within Area Component - StartAutoPaintTask");

	if (!meshComponent || meshComponent->IsBeingDestroyed()) return;
	if (!GetIsAutoPainting()) return;
	if (IsMeshBeingPaintedOrIsQueuedUp(meshComponent)) return;
	if (!AutoPaintingWithinAreaWithSettings.Contains(meshComponent)) return;
	if (!AutoPaintingWithinAreaAdditionalDataSettings.Contains(meshComponent)) return;


	Super::StartAutoPaintTask(meshComponent);


	RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s - StartAutoPaintTask Within Area for Actor %s and Mesh: %s", *GetName(), *meshComponent->GetOwner()->GetName(), *meshComponent->GetName());

	PaintOnMeshWithinArea(AutoPaintingWithinAreaWithSettings.FindRef(meshComponent), AutoPaintingWithinAreaAdditionalDataSettings.FindRef(meshComponent));
}


//-------------------------------------------------------

// Should Always Tick

bool UAutoAddColorWithinAreaComponent::ShouldAlwaysTick() const {

	if (Super::ShouldAlwaysTick()) return true;

	return (MeshComponentsToCheckIfMoved.Num() > 0 && (OnlyPaintOnMovingMeshZ || OnlyPaintOnMovingMeshXY || OnlyPaintOnRotatedMesh || OnlyPaintOnReScaledMesh));
}


//-------------------------------------------------------

// Update

void UAutoAddColorWithinAreaComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	
	RVPDP_SCOPE_CYCLE_COUNTER(STAT_AutoAddWithinAreaTickComponent);
	RVPDP_CPUPROFILER_STR("Auto Add Within Area Component - TickComponent");

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// If these are false then will determine in ShouldStartNewTaskOnComponent if another task should be started. 
	if (!OnlyPaintOnMovingMeshZ && !OnlyPaintOnMovingMeshXY && !OnlyPaintOnRotatedMesh && !OnlyPaintOnReScaledMesh) return;
	if (AutoPaintingMeshes.Num() == 0) return;
	if (MeshComponentsToCheckIfMoved.Num() == 0) return;


	TArray<UPrimitiveComponent*> meshComponentsToCheck;
	MeshComponentsToCheckIfMoved.GetKeys(meshComponentsToCheck);

	TArray<FTransform> lastMeshTransforms;
	MeshComponentsToCheckIfMoved.GenerateValueArray(lastMeshTransforms);

	
	bool verifyCachedMeshes = false;

	for (int i = meshComponentsToCheck.Num() - 1; i >= 0; i--) {

		if (!IsValid(meshComponentsToCheck[i])) {

			verifyCachedMeshes = true;
			continue;
		}


		UPrimitiveComponent* meshComponent = meshComponentsToCheck[i];
		const FVector lastMeshLocation = lastMeshTransforms[i].GetLocation();
		const FRotator lastMeshRotation = lastMeshTransforms[i].GetRotation().Rotator();
		const FVector lastMeshScale = lastMeshTransforms[i].GetScale3D();


		// If something has already queued it up then clear it out from this check. 
		if (IsMeshBeingPaintedOrIsQueuedUp(meshComponent)) {

			MeshComponentsToCheckIfMoved.Remove(meshComponent);
		}

		else if (HasMeshMovedSinceLastPaintJob(meshComponent, lastMeshLocation) || HasMeshRotatedSinceLastPaintJob(meshComponent, lastMeshRotation) || HasMeshReScaledSinceLastPaintJob(meshComponent, lastMeshScale)) {

			RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s - Mesh: %s in Actor: %s has Moved or Rotated so starts a new Paint Task!", *GetName(), *meshComponent->GetName(), *meshComponent->GetOwner()->GetName());

			MeshComponentsToCheckIfMoved.Remove(meshComponent);
			StartAutoPaintTask(meshComponent);
		}
	}


	if (verifyCachedMeshes)
		VerifyAutoPaintingMeshComponents();


	if (MeshComponentsToCheckIfMoved.Num() == 0)
		SetComponentTickEnabled(ShouldAlwaysTick());
}


//-------------------------------------------------------

// Should Start New Task On Component

bool UAutoAddColorWithinAreaComponent::CanStartNewTaskBasedOnTaskResult(const FRVPDPTaskResults& TaskResultInfo, const FRVPDPPaintTaskResultInfo& PaintTaskResultInfo, bool& IsFullyPainted, bool& IsCompletelyEmpty) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_CanStartNewTaskBasedOnTaskResult);
	RVPDP_CPUPROFILER_STR("Auto Add Within Area Component - CanStartNewTaskBasedOnTaskResult");

	// If not gonna check on tick if any change in Location, rotation or scale then we can let the parent decide if we should start new task based on the amount of colors of each channel etc. 
	if (!OnlyPaintOnMovingMeshZ && !OnlyPaintOnMovingMeshXY && !OnlyPaintOnRotatedMesh && !OnlyPaintOnReScaledMesh)
		return Super::CanStartNewTaskBasedOnTaskResult(TaskResultInfo, PaintTaskResultInfo, IsFullyPainted, IsCompletelyEmpty);


	if (UPrimitiveComponent* meshComponent = TaskResultInfo.MeshVertexData.MeshComp.Get()) {

		if (!IsAutoPaintingEnabled) return false;
		if (!TaskResultInfo.TaskSuccessfull) return false;
		if (!AutoPaintingMeshes.Contains(meshComponent)) return false;
		if (IsMeshBeingPaintedOrIsQueuedUp(meshComponent)) return false;


		// If going to check if moved or rotated etc. then we add it to the list and make sure tick is on. 
		if (!MeshComponentsToCheckIfMoved.Contains(meshComponent)) {

			MeshComponentsToCheckIfMoved.Add(meshComponent, meshComponent->GetComponentTransform());

			SetComponentTickEnabled(true);

			RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Within Area Component: %s - CanStartNewTaskBasedOnTaskResult() Turned on Tick since the paint component is set to check if change has occured in location, rotation or scale! ", *GetName());
		}
	}


	if (PaintTaskResultInfo.AnyVertexColorGotChanged) {

		RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Within Area Component: %s - CanStartNewTaskBasedOnTaskResult() Since change was made from last Paint Task then we still run CanStartNewTaskBasedOnTaskResult Super on Parent that checks if it should start new task depending on Task Results. So for example if not fully painted and if trying to add, it should start another task. ", *GetName());

		return Super::CanStartNewTaskBasedOnTaskResult(TaskResultInfo, PaintTaskResultInfo, IsFullyPainted, IsCompletelyEmpty);
	}


	return false;
}


//-------------------------------------------------------

// Has Mesh Moved Since Last Paint Job

bool UAutoAddColorWithinAreaComponent::HasMeshMovedSinceLastPaintJob(const UPrimitiveComponent* MeshComponent, const FVector& LastMeshLocation) const {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_HasMeshMovedSinceLastPaintJob);
	RVPDP_CPUPROFILER_STR("Auto Add Within Area Component - HasMeshMovedSinceLastPaintJob");

	if (!OnlyPaintOnMovingMeshZ && !OnlyPaintOnMovingMeshXY) return false;


	// Can optimize further by requiring that the Mesh is Moving on either XYZ for another auto paint job to start, otherwise we don't, so if a mesh is still in the area and no difference would occur there is no need to start another. 

	if (OnlyPaintOnMovingMeshZ) {

		// If has moved more than 1 up or down in Z
		if (MeshComponent->GetComponentLocation().Z < LastMeshLocation.Z - OnlyPaintOnMovingMeshZDifferenceRequired || MeshComponent->GetComponentLocation().Z > LastMeshLocation.Z + OnlyPaintOnMovingMeshZDifferenceRequired)
			return true;
	}

	if (OnlyPaintOnMovingMeshXY) {

		// If has moved on XY
		if (MeshComponent->GetComponentLocation().X < LastMeshLocation.X - OnlyPaintOnMovingMeshXYDifferenceRequired || MeshComponent->GetComponentLocation().X > LastMeshLocation.X + OnlyPaintOnMovingMeshXYDifferenceRequired)
			return true;

		else if (MeshComponent->GetComponentLocation().Y < LastMeshLocation.Y - OnlyPaintOnMovingMeshXYDifferenceRequired || MeshComponent->GetComponentLocation().Y > LastMeshLocation.Y + OnlyPaintOnMovingMeshXYDifferenceRequired)
			return true;
	}

	return false;
}


//-------------------------------------------------------

// Has Mesh Rotated Since Last Paint Job

bool UAutoAddColorWithinAreaComponent::HasMeshRotatedSinceLastPaintJob(const UPrimitiveComponent* MeshComponent, const FRotator& LastMeshRotation) const {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_HasMeshRotatedSinceLastPaintJob);
	RVPDP_CPUPROFILER_STR("Auto Add Within Area Component - HasMeshRotatedSinceLastPaintJob");

	if (!OnlyPaintOnRotatedMesh) return false;


	if (MeshComponent->GetComponentRotation().Pitch < LastMeshRotation.Pitch - OnlyPaintOnRotatedMeshDifferenceRequired || MeshComponent->GetComponentRotation().Pitch > LastMeshRotation.Pitch + OnlyPaintOnRotatedMeshDifferenceRequired)
		return true;

	if (MeshComponent->GetComponentRotation().Roll < LastMeshRotation.Roll - OnlyPaintOnRotatedMeshDifferenceRequired || MeshComponent->GetComponentRotation().Roll > LastMeshRotation.Roll + OnlyPaintOnRotatedMeshDifferenceRequired)
		return true;

	if (MeshComponent->GetComponentRotation().Yaw < LastMeshRotation.Yaw - OnlyPaintOnRotatedMeshDifferenceRequired || MeshComponent->GetComponentRotation().Yaw > LastMeshRotation.Yaw + OnlyPaintOnRotatedMeshDifferenceRequired)
		return true;


	return false;
}


//-------------------------------------------------------

// Has Mesh ReScaled Since Last Paint Job

bool UAutoAddColorWithinAreaComponent::HasMeshReScaledSinceLastPaintJob(const UPrimitiveComponent* MeshComponent, const FVector& LastMeshScale) const {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_HasMeshReScaledSinceLastPaintJob);
	RVPDP_CPUPROFILER_STR("Auto Add Within Area Component - HasMeshReScaledSinceLastPaintJob");

	if (!OnlyPaintOnReScaledMesh) return false;


	if (MeshComponent->GetComponentScale().X < LastMeshScale.X - OnlyPaintOnReScaledMeshDifferenceRequired || MeshComponent->GetComponentScale().X > LastMeshScale.X + OnlyPaintOnReScaledMeshDifferenceRequired)
		return true;

	if (MeshComponent->GetComponentScale().Y < LastMeshScale.Y - OnlyPaintOnReScaledMeshDifferenceRequired || MeshComponent->GetComponentScale().Y > LastMeshScale.Y + OnlyPaintOnReScaledMeshDifferenceRequired)
		return true;

	if (MeshComponent->GetComponentScale().Z < LastMeshScale.Z - OnlyPaintOnReScaledMeshDifferenceRequired || MeshComponent->GetComponentScale().Z > LastMeshScale.Z + OnlyPaintOnReScaledMeshDifferenceRequired)
		return true;


	return false;
}


//-------------------------------------------------------

// Get Apply Vertex Color Settings

bool UAutoAddColorWithinAreaComponent::GetApplyVertexColorSettings(UPrimitiveComponent* MeshComponent, FRVPDPApplyColorSettings& ApplyVertexColorSettings) {

	ApplyVertexColorSettings = FRVPDPApplyColorSettings();

	if (!AutoPaintingWithinAreaWithSettings.Contains(MeshComponent)) return false;

	ApplyVertexColorSettings = AutoPaintingWithinAreaWithSettings.FindRef(MeshComponent).ApplyVertexColorSettings;

	return true;
}


//-------------------------------------------------------

// Stop All Auto Painting

void UAutoAddColorWithinAreaComponent::StopAllAutoPainting() {

	Super::StopAllAutoPainting();

	AutoPaintingWithinAreaWithSettings.Empty();
	AutoPaintingWithinAreaAdditionalDataSettings.Empty();
	MeshComponentsToCheckIfMoved.Empty();

	PrimaryComponentTick.bStartWithTickEnabled = false;
}


//-------------------------------------------------------

// Verify Mesh Components

void UAutoAddColorWithinAreaComponent::VerifyAutoPaintingMeshComponents() {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_VerifyAutoPaintingMeshComponents);
	RVPDP_CPUPROFILER_STR("Auto Add Within Area Component - VerifyAutoPaintingMeshComponents");

	Super::VerifyAutoPaintingMeshComponents();


	if (AutoPaintingWithinAreaWithSettings.Num() > 0) {

		TArray<UPrimitiveComponent*> meshComponents;
		AutoPaintingWithinAreaWithSettings.GetKeys(meshComponents);

		TArray<FRVPDPPaintWithinAreaSettings> paintSettings;
		AutoPaintingWithinAreaWithSettings.GenerateValueArray(paintSettings);

		AutoPaintingWithinAreaWithSettings.Empty();

		for (int i = 0; i < meshComponents.Num(); i++) {

			if (IsValid(meshComponents[i]))
				AutoPaintingWithinAreaWithSettings.Add(meshComponents[i], paintSettings[i]);
		}
	}

	if (AutoPaintingWithinAreaAdditionalDataSettings.Num() > 0) {

		TArray<UPrimitiveComponent*> meshComponents;
		AutoPaintingWithinAreaAdditionalDataSettings.GetKeys(meshComponents);

		TArray<FRVPDPAdditionalDataToPassThroughInfo> additionalDataSettings;
		AutoPaintingWithinAreaAdditionalDataSettings.GenerateValueArray(additionalDataSettings);

		AutoPaintingWithinAreaAdditionalDataSettings.Empty();

		for (int i = 0; i < meshComponents.Num(); i++) {

			if (IsValid(meshComponents[i]))
				AutoPaintingWithinAreaAdditionalDataSettings.Add(meshComponents[i], additionalDataSettings[i]);
		}
	}

	if (MeshComponentsToCheckIfMoved.Num() > 0) {

		TArray<UPrimitiveComponent*> lastAutoPaintWithinAreaMeshes;
		MeshComponentsToCheckIfMoved.GetKeys(lastAutoPaintWithinAreaMeshes);

		TArray<FTransform> lastMeshTransforms;
		MeshComponentsToCheckIfMoved.GenerateValueArray(lastMeshTransforms);

		MeshComponentsToCheckIfMoved.Empty();

		for (int i = 0; i < lastAutoPaintWithinAreaMeshes.Num(); i++) {

			if (IsValid(lastAutoPaintWithinAreaMeshes[i])) {

				MeshComponentsToCheckIfMoved.Add(lastAutoPaintWithinAreaMeshes[i], lastMeshTransforms[i]);
			}
		}
	}
}


//-------------------------------------------------------

// Stop Auto Painting Mesh

void UAutoAddColorWithinAreaComponent::StopAutoPaintingMesh(UPrimitiveComponent* MeshComponent) {

	Super::StopAutoPaintingMesh(MeshComponent);

	if (!MeshComponent) return;


	if (AutoPaintingWithinAreaWithSettings.Contains(MeshComponent))
		AutoPaintingWithinAreaWithSettings.Remove(MeshComponent);

	if (AutoPaintingWithinAreaAdditionalDataSettings.Contains(MeshComponent))
		AutoPaintingWithinAreaAdditionalDataSettings.Remove(MeshComponent);

	if (MeshComponentsToCheckIfMoved.Contains(MeshComponent))
		MeshComponentsToCheckIfMoved.Remove(MeshComponent);

	if (MeshComponentsToCheckIfMoved.Num() == 0)
		PrimaryComponentTick.bStartWithTickEnabled = false;
}