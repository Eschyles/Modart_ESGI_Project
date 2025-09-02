// Copyright 2023 Alexander Floden, Alias Alex River. All Rights Reserved.


#include "AutoAddColorPaintAtLocComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"

// Plugin
#include "VertexPaintDetectionLog.h"
#include "VertexPaintDetectionProfiling.h"


//-------------------------------------------------------

// Construct

UAutoAddColorPaintAtLocComponent::UAutoAddColorPaintAtLocComponent() {

	IsAllowedToAutoPaintAtBeginPlay = false;
}


//-------------------------------------------------------

// Begin Play

void UAutoAddColorPaintAtLocComponent::BeginPlay() {

	Super::BeginPlay();


	// Need to make sure we also start paint jobs if the owner is moving, so if we're running traces from the actor to a point, but we're not starting any new tasks because no difference has been made, but if the owning actor starts moving then we want to make sure it does so the new point the traces gets hit on gets painted. 
	if (USceneComponent* ownerRootComp = GetOwner()->GetRootComponent()) {

		auto transformUpdated = [this](USceneComponent* UpdatedComponent, EUpdateTransformFlags UpdateTransformFlag, ETeleportType Teleport)->void {


			bool verifyCachedMeshes = false;
			TArray<UPrimitiveComponent*> meshComponentsToCheck;
			AutoPaintingAtLocationWithSettings.GetKeys(meshComponentsToCheck);


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

// Add Auto Paint At Location

bool UAutoAddColorPaintAtLocComponent::AddAutoPaintAtLocation(UPrimitiveComponent* MeshComponent, FRVPDPPaintAtLocationSettings PaintAtLocationSettings, FRVPDPAdditionalDataToPassThroughInfo AdditionalData, FRVPDPAutoAddColorSettings AutoAddColorSettings, bool ResumeIfPaused) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_AddAutoPaintAtLocation);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - AddAutoPaintAtLocation");

	// Note doesn't check if already in TMap since we want to be able to Update the Paint Settings

	PaintAtLocationSettings.MeshComponent = MeshComponent;
	PaintAtLocationSettings.WeakMeshComponent = MeshComponent;
	PaintAtLocationSettings.TaskWorld = GetWorld();
	PaintAtLocationSettings.DebugSettings.WorldTaskWasCreatedIn = GetWorld();
	DebugSettings.WorldTaskWasCreatedIn = GetWorld();


	// Since we may run this to Update Paint Settings, and a task may already be going, we don't want the Check Valid to fail if it's already painting in this specific instance or because of a Task Queue limit, we always want to be able to Add/Update it at least. Then it may fail when it actually start because of the task queue limit until it goes down. 
	if (!PaintOnMeshAtLocationCheckValid(PaintAtLocationSettings, true)) return false;


	AdjustCallbackSettings(PaintAtLocationSettings.ApplyVertexColorSettings, PaintAtLocationSettings.CallbackSettings);


	FRVPDPAutoAddColorSettings autoAddColorSettingsToApply = AutoAddColorSettings;

	// If already added then we don't want cached stuff like IsPaused etc. to get replaced but only exposed changeable properties. 
	if (AutoPaintingMeshes.Contains(PaintAtLocationSettings.GetMeshComponent())) {

		autoAddColorSettingsToApply = AutoPaintingMeshes.FindRef(PaintAtLocationSettings.GetMeshComponent());
		autoAddColorSettingsToApply.UpdateExposedProperties(AutoAddColorSettings.CanEverGetPaused, AutoAddColorSettings.DelayBetweenTasks, AutoAddColorSettings.RemoveMeshFromAutoPaintingIfFullyPainted, AutoAddColorSettings.RemoveMeshFromAutoPaintingIfCompletelyEmpty, AutoAddColorSettings.OnlyStartNewTaskIfChangeWasMadeByOwningPaintComponent);
	}


	AutoPaintingAtLocationWithSettings.Add(PaintAtLocationSettings.GetMeshComponent(), PaintAtLocationSettings);
	AutoPaintingAtLocationAdditionalDataSettings.Add(PaintAtLocationSettings.GetMeshComponent(), AdditionalData);

	if (ResumeIfPaused)
		autoAddColorSettingsToApply.IsPaused = false;


	MeshAddedToBeAutoPainted(PaintAtLocationSettings.GetMeshComponent(), autoAddColorSettingsToApply);


	if (autoAddColorSettingsToApply.IsPaused) {

		RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s - Paused Mesh: %s in Actor: %s got it's Auto Paint at Location Settings Updated!", *GetName(), *MeshComponent->GetName(), *MeshComponent->GetOwner()->GetName());
	}

	else {

		RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s - Mesh: %s in Actor: %s Has been Added/Updated to be Auto Painted at Location!", *GetName(), *MeshComponent->GetName(), *MeshComponent->GetOwner()->GetName());
	}


	StartNewPaintTaskIfAllowed(MeshComponent, autoAddColorSettingsToApply);

	return true;
}


//-------------------------------------------------------

// Add Auto Paint At Location

void UAutoAddColorPaintAtLocComponent::UpdateAutoPaintedAtLocation(UPrimitiveComponent* MeshComponent, FRVPDPPaintAtLocationSettings PaintAtLocationSettings, FRVPDPAdditionalDataToPassThroughInfo AdditionalData, FRVPDPAutoAddColorSettings AutoAddColorSettings, bool ResumeIfPaused) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_UpdateAutoPaintedAtLocation);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - UpdateAutoPaintedAtLocation");

	if (!MeshComponent) return;
	if (MeshComponent->IsBeingDestroyed()) return;
	if (!AutoPaintingAtLocationWithSettings.Contains(MeshComponent)) return;

	PaintAtLocationSettings.MeshComponent = MeshComponent;
	PaintAtLocationSettings.WeakMeshComponent = MeshComponent;
	PaintAtLocationSettings.TaskWorld = GetWorld();
	PaintAtLocationSettings.DebugSettings.WorldTaskWasCreatedIn = GetWorld();
	DebugSettings.WorldTaskWasCreatedIn = GetWorld();


	// Since we may run this to Update Paint Settings, and a task may already be going, we don't want the Check Valid to fail if it's already painting in this specific instance or because of a Task Queue limit, we always want to be able to Add/Update it at least. Then it may fail when it actually start because of the task queue limit until it goes down. 
	if (!PaintOnMeshAtLocationCheckValid(PaintAtLocationSettings, true)) {

		// So if the user was trying to update it to invalid paint settings, then rather than have the task continue to be auto painted with something the user was trying to change away from we instead stop auto painting it. 
		StopAutoPaintingMesh(MeshComponent);
		return;
	}


	AdjustCallbackSettings(PaintAtLocationSettings.ApplyVertexColorSettings, PaintAtLocationSettings.CallbackSettings);


	FRVPDPAutoAddColorSettings autoAddColorSettingsToApply = AutoAddColorSettings;

	// If already added then we don't want cached stuff like IsPaused etc. to get replaced but only exposed changeable properties. 
	if (AutoPaintingMeshes.Contains(PaintAtLocationSettings.GetMeshComponent())) {

		autoAddColorSettingsToApply = AutoPaintingMeshes.FindRef(PaintAtLocationSettings.GetMeshComponent());
		autoAddColorSettingsToApply.UpdateExposedProperties(AutoAddColorSettings.CanEverGetPaused, AutoAddColorSettings.DelayBetweenTasks, AutoAddColorSettings.RemoveMeshFromAutoPaintingIfFullyPainted, AutoAddColorSettings.RemoveMeshFromAutoPaintingIfCompletelyEmpty, AutoAddColorSettings.OnlyStartNewTaskIfChangeWasMadeByOwningPaintComponent);
	}


	AutoPaintingAtLocationWithSettings.Add(PaintAtLocationSettings.GetMeshComponent(), PaintAtLocationSettings);
	AutoPaintingAtLocationAdditionalDataSettings.Add(PaintAtLocationSettings.GetMeshComponent(), AdditionalData);

	if (ResumeIfPaused)
		autoAddColorSettingsToApply.IsPaused = false;


	MeshAddedToBeAutoPainted(PaintAtLocationSettings.GetMeshComponent(), autoAddColorSettingsToApply);


	if (autoAddColorSettingsToApply.IsPaused) {

		RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s - Paused Mesh: %s in Actor: %s got it's Auto Paint at Location Settings Updated!", *GetName(), *MeshComponent->GetName(), *MeshComponent->GetOwner()->GetName());
	}

	else {

		RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s - Mesh: %s in Actor: %s got it's Auto Paint At Location Settings Updated! ", *GetName(), *MeshComponent->GetName(), *MeshComponent->GetOwner()->GetName());
	}
}


//-------------------------------------------------------

// Start Auto Paint Task

void UAutoAddColorPaintAtLocComponent::StartAutoPaintTask(UPrimitiveComponent* MeshComponent) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_StartAutoPaintTask);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - StartAutoPaintTask");

	if (!MeshComponent || MeshComponent->IsBeingDestroyed()) return;
	if (!GetIsAutoPainting()) return;
	if (IsMeshBeingPaintedOrIsQueuedUp(MeshComponent)) return;
	if (!AutoPaintingAtLocationWithSettings.Contains(MeshComponent)) return;
	if (!AutoPaintingAtLocationAdditionalDataSettings.Contains(MeshComponent)) return;

	Super::StartAutoPaintTask(MeshComponent);


	RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s - StartAutoPaintTask At Location for Actor %s and Mesh: %s", *GetName(), *MeshComponent->GetOwner()->GetName(), *MeshComponent->GetName());

	PaintOnMeshAtLocation(AutoPaintingAtLocationWithSettings.FindRef(MeshComponent), AutoPaintingAtLocationAdditionalDataSettings.FindRef(MeshComponent));
}


//-------------------------------------------------------

// Get Apply Vertex Color Settings

bool UAutoAddColorPaintAtLocComponent::GetApplyVertexColorSettings(UPrimitiveComponent* MeshComponent, FRVPDPApplyColorSettings& ApplyVertexColorSettings) {

	ApplyVertexColorSettings = FRVPDPApplyColorSettings();

	if (!AutoPaintingAtLocationWithSettings.Contains(MeshComponent)) return false;

	ApplyVertexColorSettings = AutoPaintingAtLocationWithSettings.FindRef(MeshComponent).ApplyVertexColorSettings;

	return true;
}


//-------------------------------------------------------

// Stop All Auto Painting

void UAutoAddColorPaintAtLocComponent::StopAllAutoPainting() {

	Super::StopAllAutoPainting();

	AutoPaintingAtLocationWithSettings.Empty();
	AutoPaintingAtLocationAdditionalDataSettings.Empty();
}


//-------------------------------------------------------

// Verify Mesh Components

void UAutoAddColorPaintAtLocComponent::VerifyAutoPaintingMeshComponents() {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_VerifyAutoPaintingMeshComponents);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - VerifyAutoPaintingMeshComponents");

	Super::VerifyAutoPaintingMeshComponents();


	if (AutoPaintingAtLocationWithSettings.Num() > 0) {

		TArray<UPrimitiveComponent*> meshComponents;
		AutoPaintingAtLocationWithSettings.GetKeys(meshComponents);

		TArray<FRVPDPPaintAtLocationSettings> paintSettings;
		AutoPaintingAtLocationWithSettings.GenerateValueArray(paintSettings);

		AutoPaintingAtLocationWithSettings.Empty();

		for (int i = 0; i < meshComponents.Num(); i++) {

			if (IsValid(meshComponents[i]))
				AutoPaintingAtLocationWithSettings.Add(meshComponents[i], paintSettings[i]);
		}
	}


	if (AutoPaintingAtLocationAdditionalDataSettings.Num() > 0) {

		TArray<UPrimitiveComponent*> meshComponents;
		AutoPaintingAtLocationAdditionalDataSettings.GetKeys(meshComponents);

		TArray<FRVPDPAdditionalDataToPassThroughInfo> additionalDataSettings;
		AutoPaintingAtLocationAdditionalDataSettings.GenerateValueArray(additionalDataSettings);

		AutoPaintingAtLocationAdditionalDataSettings.Empty();

		for (int i = 0; i < meshComponents.Num(); i++) {

			if (IsValid(meshComponents[i]))
				AutoPaintingAtLocationAdditionalDataSettings.Add(meshComponents[i], additionalDataSettings[i]);
		}
	}
}


//-------------------------------------------------------

// Stop Auto Painting Mesh

void UAutoAddColorPaintAtLocComponent::StopAutoPaintingMesh(UPrimitiveComponent* MeshComponent) {

	Super::StopAutoPaintingMesh(MeshComponent);

	if (!MeshComponent) return;


	if (AutoPaintingAtLocationWithSettings.Contains(MeshComponent))
		AutoPaintingAtLocationWithSettings.Remove(MeshComponent);

	if (AutoPaintingAtLocationAdditionalDataSettings.Contains(MeshComponent))
		AutoPaintingAtLocationAdditionalDataSettings.Remove(MeshComponent);
}