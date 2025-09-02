// Copyright 2022-2024 Alexander Floden, Alias Alex River. All Rights Reserved.


#include "AutoAddColorEntireMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"

// Plugin
#include "VertexPaintDetectionLog.h"
#include "VertexPaintDetectionProfiling.h"


//-------------------------------------------------------

// Begin Play

void UAutoAddColorEntireMeshComponent::BeginPlay() {

	Super::BeginPlay();

	if (AutoPaintRootMeshAtBeginPlay) {

		if (auto mesh = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent())) {

			AddAutoPaintEntireMesh(mesh, AutoPaintRootMeshAtBeginPlay_PaintEntireMeshSettings, FRVPDPAdditionalDataToPassThroughInfo(), AutoPaintRootMeshAtBeginPlay_DelaySettings);
		}
	}
}


//-------------------------------------------------------

// Add Auto Paint Entire Mesh

bool UAutoAddColorEntireMeshComponent::AddAutoPaintEntireMesh(UPrimitiveComponent* MeshComponent, FRVPDPPaintOnEntireMeshSettings PaintEntireMeshSettings, FRVPDPAdditionalDataToPassThroughInfo AdditionalData, FRVPDPAutoAddColorSettings AutoAddColorSettings, bool ResumeIfPaused) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_AddAutoPaintEntireMesh);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - AddAutoPaintEntireMesh");

	// Note doesn't check if already in TMap since we want to be able to Update the Paint Settings

	PaintEntireMeshSettings.MeshComponent = MeshComponent;
	PaintEntireMeshSettings.WeakMeshComponent = MeshComponent;
	PaintEntireMeshSettings.TaskWorld = GetWorld();
	PaintEntireMeshSettings.DebugSettings.WorldTaskWasCreatedIn = GetWorld();
	DebugSettings.WorldTaskWasCreatedIn = GetWorld();

	// Since we may run this to Update Paint Settings, and a task may already be going, we don't want the Check Valid to fail if it's already painting in this specific instance or because of a Task Queue limit, we always want to be able to Add/Update it at least. Then it may fail when it actually start because of the task queue limit until it goes down. 
	if (!PaintOnEntireMeshCheckValid(PaintEntireMeshSettings, true)) return false;

	AdjustCallbackSettings(PaintEntireMeshSettings.ApplyVertexColorSettings, PaintEntireMeshSettings.CallbackSettings);


	FRVPDPAutoAddColorSettings autoAddColorSettingsToApply = AutoAddColorSettings;

	// If already added then we don't want cached stuff like IsPaused etc. to get replaced but only exposed changeable properties. 
	if (AutoPaintingMeshes.Contains(PaintEntireMeshSettings.GetMeshComponent())) {

		autoAddColorSettingsToApply = AutoPaintingMeshes.FindRef(PaintEntireMeshSettings.GetMeshComponent());
		autoAddColorSettingsToApply.UpdateExposedProperties(AutoAddColorSettings.CanEverGetPaused, AutoAddColorSettings.DelayBetweenTasks, AutoAddColorSettings.RemoveMeshFromAutoPaintingIfFullyPainted, AutoAddColorSettings.RemoveMeshFromAutoPaintingIfCompletelyEmpty, AutoAddColorSettings.OnlyStartNewTaskIfChangeWasMadeByOwningPaintComponent);
	}


	AutoPaintingEntireMeshesWithSettings.Add(PaintEntireMeshSettings.GetMeshComponent(), PaintEntireMeshSettings);
	AutoPaintingEntireMeshesAdditionalDataSettings.Add(PaintEntireMeshSettings.GetMeshComponent(), AdditionalData);

	if (ResumeIfPaused)
		autoAddColorSettingsToApply.IsPaused = false;

	MeshAddedToBeAutoPainted(PaintEntireMeshSettings.GetMeshComponent(), autoAddColorSettingsToApply);


	if (autoAddColorSettingsToApply.IsPaused) {

		RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s - Paused Mesh: %s in Actor: %s got it's Auto Paint over the Entire Mesh Settings Updated!", *GetName(), *MeshComponent->GetName(), *MeshComponent->GetOwner()->GetName());
	}

	else {

		RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s - Mesh: %s in Actor: %s Has been Added/Updated to be Auto Painted over the Entire Mesh!", *GetName(), *MeshComponent->GetName(), *MeshComponent->GetOwner()->GetName());
	}


	StartNewPaintTaskIfAllowed(MeshComponent, autoAddColorSettingsToApply);

	return true;
}


//-------------------------------------------------------

// Update Auto Painted Entire Mesh

void UAutoAddColorEntireMeshComponent::UpdateAutoPaintedEntireMesh(UPrimitiveComponent* MeshComponent, FRVPDPPaintOnEntireMeshSettings PaintEntireMeshSettings, FRVPDPAdditionalDataToPassThroughInfo AdditionalData, FRVPDPAutoAddColorSettings AutoAddColorSettings, bool ResumeIfPaused) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_UpdateAutoPaintedEntireMesh);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - UpdateAutoPaintedEntireMesh");

	if (!MeshComponent) return;
	if (MeshComponent->IsBeingDestroyed()) return;
	if (!AutoPaintingEntireMeshesWithSettings.Contains(MeshComponent)) return;


	PaintEntireMeshSettings.MeshComponent = MeshComponent;
	PaintEntireMeshSettings.WeakMeshComponent = MeshComponent;
	PaintEntireMeshSettings.TaskWorld = GetWorld();
	PaintEntireMeshSettings.DebugSettings.WorldTaskWasCreatedIn = GetWorld();
	DebugSettings.WorldTaskWasCreatedIn = GetWorld();


	// Since we may run this to Update Paint Settings, and a task may already be going, we don't want the Check Valid to fail if it's already painting in this specific instance or because of a Task Queue limit, we always want to be able to Add/Update it at least. Then it may fail when it actually start because of the task queue limit until it goes down. 
	if (!PaintOnEntireMeshCheckValid(PaintEntireMeshSettings, true)) {

		// So if the user was trying to update it to invalid paint settings, then rather than have the task continue to be auto painted with something the user was trying to change away from we instead stop auto painting it. 
		StopAutoPaintingMesh(MeshComponent);
		return;
	}


	AdjustCallbackSettings(PaintEntireMeshSettings.ApplyVertexColorSettings, PaintEntireMeshSettings.CallbackSettings);


	FRVPDPAutoAddColorSettings autoAddColorSettingsToApply = AutoAddColorSettings;

	// If already added then we don't want cached stuff like IsPaused etc. to get replaced but only exposed changeable properties. 
	if (AutoPaintingMeshes.Contains(PaintEntireMeshSettings.GetMeshComponent())) {

		autoAddColorSettingsToApply = AutoPaintingMeshes.FindRef(PaintEntireMeshSettings.GetMeshComponent());
		autoAddColorSettingsToApply.UpdateExposedProperties(AutoAddColorSettings.CanEverGetPaused, AutoAddColorSettings.DelayBetweenTasks, AutoAddColorSettings.RemoveMeshFromAutoPaintingIfFullyPainted, AutoAddColorSettings.RemoveMeshFromAutoPaintingIfCompletelyEmpty, AutoAddColorSettings.OnlyStartNewTaskIfChangeWasMadeByOwningPaintComponent);
	}


	AutoPaintingEntireMeshesWithSettings.Add(PaintEntireMeshSettings.GetMeshComponent(), PaintEntireMeshSettings);
	AutoPaintingEntireMeshesAdditionalDataSettings.Add(PaintEntireMeshSettings.GetMeshComponent(), AdditionalData);

	if (ResumeIfPaused)
		autoAddColorSettingsToApply.IsPaused = false;


	MeshAddedToBeAutoPainted(PaintEntireMeshSettings.GetMeshComponent(), autoAddColorSettingsToApply);


	if (autoAddColorSettingsToApply.IsPaused) {

		RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s - Paused Mesh: %s in Actor: %s got it's Auto Paint Entire Mesh Settings Updated!", *GetName(), *MeshComponent->GetName(), *MeshComponent->GetOwner()->GetName());
	}

	else {

		RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s - Mesh: %s in Actor: %s got it's Auto Paint Entire Mesh Settings Updated!", *GetName(), *MeshComponent->GetName(), *MeshComponent->GetOwner()->GetName());
	}
}


//-------------------------------------------------------

// Start Auto Paint Task

void UAutoAddColorEntireMeshComponent::StartAutoPaintTask(UPrimitiveComponent* MeshComponent) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_StartAutoPaintTask);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - StartAutoPaintTask");

	if (!MeshComponent || MeshComponent->IsBeingDestroyed()) return;
	if (!GetIsAutoPainting()) return;
	if (IsMeshBeingPaintedOrIsQueuedUp(MeshComponent)) return;
	if (!AutoPaintingEntireMeshesWithSettings.Contains(MeshComponent)) return;
	if (!AutoPaintingEntireMeshesAdditionalDataSettings.Contains(MeshComponent)) return;


	Super::StartAutoPaintTask(MeshComponent);


	RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s - StartAutoPaintTask Entire Mesh for Actor: %s and Mesh: %s", *GetName(), *MeshComponent->GetOwner()->GetName(), *MeshComponent->GetName());

	PaintOnEntireMesh(AutoPaintingEntireMeshesWithSettings.FindRef(MeshComponent), AutoPaintingEntireMeshesAdditionalDataSettings.FindRef(MeshComponent));
}


//-------------------------------------------------------

// Get Apply Vertex Color Settings

bool UAutoAddColorEntireMeshComponent::GetApplyVertexColorSettings(UPrimitiveComponent* MeshComponent, FRVPDPApplyColorSettings& ApplyVertexColorSettings) {

	ApplyVertexColorSettings = FRVPDPApplyColorSettings();

	if (!AutoPaintingEntireMeshesWithSettings.Contains(MeshComponent)) return false;

	ApplyVertexColorSettings = AutoPaintingEntireMeshesWithSettings.FindRef(MeshComponent).ApplyVertexColorSettings;

	return true;
}


//-------------------------------------------------------

// Stop All Auto Painting

void UAutoAddColorEntireMeshComponent::StopAllAutoPainting() {

	Super::StopAllAutoPainting();

	AutoPaintingEntireMeshesWithSettings.Empty();
	AutoPaintingEntireMeshesAdditionalDataSettings.Empty();
}


//-------------------------------------------------------

// Verify Mesh Components

void UAutoAddColorEntireMeshComponent::VerifyAutoPaintingMeshComponents() {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_VerifyAutoPaintingMeshComponents);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - VerifyAutoPaintingMeshComponents");

	Super::VerifyAutoPaintingMeshComponents();


	if (AutoPaintingEntireMeshesWithSettings.Num() > 0) {

		TArray<UPrimitiveComponent*> meshComponents;
		AutoPaintingEntireMeshesWithSettings.GetKeys(meshComponents);

		TArray<FRVPDPPaintOnEntireMeshSettings> paintSettings;
		AutoPaintingEntireMeshesWithSettings.GenerateValueArray(paintSettings);

		AutoPaintingEntireMeshesWithSettings.Empty();

		for (int i = 0; i < meshComponents.Num(); i++) {

			if (IsValid(meshComponents[i]))
				AutoPaintingEntireMeshesWithSettings.Add(meshComponents[i], paintSettings[i]);
		}
	}


	if (AutoPaintingEntireMeshesAdditionalDataSettings.Num() > 0) {

		TArray<UPrimitiveComponent*> meshComponents;
		AutoPaintingEntireMeshesAdditionalDataSettings.GetKeys(meshComponents);

		TArray<FRVPDPAdditionalDataToPassThroughInfo> additionalDataSettings;
		AutoPaintingEntireMeshesAdditionalDataSettings.GenerateValueArray(additionalDataSettings);

		AutoPaintingEntireMeshesAdditionalDataSettings.Empty();

		for (int i = 0; i < meshComponents.Num(); i++) {

			if (IsValid(meshComponents[i]))
				AutoPaintingEntireMeshesAdditionalDataSettings.Add(meshComponents[i], additionalDataSettings[i]);
		}
	}
}


//-------------------------------------------------------

// Stop Auto Painting Mesh

void UAutoAddColorEntireMeshComponent::StopAutoPaintingMesh(UPrimitiveComponent* MeshComponent) {

	Super::StopAutoPaintingMesh(MeshComponent);

	if (!MeshComponent) return;


	if (AutoPaintingEntireMeshesWithSettings.Contains(MeshComponent))
		AutoPaintingEntireMeshesWithSettings.Remove(MeshComponent);

	if (AutoPaintingEntireMeshesAdditionalDataSettings.Contains(MeshComponent))
		AutoPaintingEntireMeshesAdditionalDataSettings.Remove(MeshComponent);
}