// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#include "VertexPaintDetectionComponent.h"

// Engine 
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/AssetManager.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"
#include "Engine/SkeletalMesh.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Landscape.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceDynamic.h"

// Plugin
#include "VertexPaintDetectionTaskQueue.h"
#include "VertexPaintFunctionLibrary.h"
#include "VertexPaintDetectionSettings.h"
#include "VertexPaintColorSnippetDataAsset.h"
#include "VertexPaintColorSnippetRefs.h"
#include "VertexPaintOptimizationDataAsset.h"
#include "VertexPaintDetectionLog.h"
#include "VertexPaintDetectionProfiling.h"

// UE5
#if ENGINE_MAJOR_VERSION == 5

#include "Components/DynamicMeshComponent.h"

#if ENGINE_MINOR_VERSION >= 3

#include "GeometryCollection/GeometryCollectionComponent.h"
#include "GeometryCollection/GeometryCollectionObject.h"

#endif
#endif


//-------------------------------------------------------

// Construct

UVertexPaintDetectionComponent::UVertexPaintDetectionComponent() {

	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}


//-------------------------------------------------------

// Get Closest Vertex Data On Mesh

void UVertexPaintDetectionComponent::GetClosestVertexDataOnMesh(FRVPDPGetClosestVertexDataSettings GetClosestVertexDataStruct, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough) {

	if (!UVertexPaintFunctionLibrary::IsWorldValid(GetWorld())) return;

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetClosestVertexDataOnMesh);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - GetClosestVertexDataOnMesh");

	CheckIfRegisteredAndInitialized();

	GetClosestVertexDataStruct.WeakMeshComponent = GetClosestVertexDataStruct.MeshComponent;
	GetClosestVertexDataStruct.TaskWorld = GetWorld();
	GetClosestVertexDataStruct.DebugSettings.WorldTaskWasCreatedIn = GetWorld();

	if (IsValid(GetClosestVertexDataStruct.MeshComponent)) {

		if (IsValid(GetClosestVertexDataStruct.MeshComponent->GetOwner())) {

			RVPDP_LOG(GetClosestVertexDataStruct.DebugSettings, FColor::Cyan, "Trying to Get Closest Vertex Data on Actor: %s and Component: %s", *GetClosestVertexDataStruct.MeshComponent->GetOwner()->GetName(), *GetClosestVertexDataStruct.MeshComponent->GetName());
		}

		GetClosestVertexDataStruct.HitFundementals.HitLocationInComponentSpace = UKismetMathLibrary::InverseTransformLocation(GetClosestVertexDataStruct.MeshComponent->GetComponentTransform(), GetClosestVertexDataStruct.HitFundementals.HitLocation);
	}


	GetCompleteCallbackSettings(GetClosestVertexDataStruct.MeshComponent, GetClosestVertexDataStruct.AutoSetRequiredCallbackSettings, GetClosestVertexDataStruct.CallbackSettings);

	GetClosestVertexDataStruct.GetAverageColorInAreaSettings.VertexNormalToHitNormal_MinimumDotProductToBeAccountedFor = UKismetMathLibrary::FClamp(GetClosestVertexDataStruct.GetAverageColorInAreaSettings.VertexNormalToHitNormal_MinimumDotProductToBeAccountedFor, -1, 1);

	
	FRVPDPCalculateColorsInfo calculateColorsInfo;
	calculateColorsInfo.DetectSettings = GetClosestVertexDataStruct;
	calculateColorsInfo.IsDetectTask = true;
	calculateColorsInfo.VertexPaintDetectionType = EVertexPaintDetectionType::GetClosestVertexDataDetection;
	calculateColorsInfo.GetClosestVertexDataSettings = GetClosestVertexDataStruct;
	calculateColorsInfo.EstimatedColorAtHitLocationSettings = GetClosestVertexDataStruct.GetEstimatedColorAtHitLocationSettings;
	calculateColorsInfo.TaskFundamentalSettings = GetClosestVertexDataStruct;


	if (GetClosestVertexDataOnMeshCheckValid(GetClosestVertexDataStruct)) {

		RVPDP_LOG(GetClosestVertexDataStruct.DebugSettings, FColor::Green, "Get Closest Vertex Data On Mesh CheckValid Successful.");
	}

	else {

		RVPDP_LOG(GetClosestVertexDataStruct.DebugSettings, FColor::Red, "Check Valid Failed at Get Closest Vertex Data On Mesh!");

		GetTaskFundamentalsSettings(GetClosestVertexDataStruct.MeshComponent, calculateColorsInfo);

		GetClosestVertexDataOnMeshTaskFinished(calculateColorsInfo.TaskResult, GetClosestVertexDataStruct, calculateColorsInfo.GetClosestVertexDataResult, calculateColorsInfo.GetClosestVertexData_EstimatedColorAtHitLocationResult, calculateColorsInfo.GetClosestVertexData_AverageColorInArea, calculateColorsInfo.TaskFundamentalSettings, AdditionalDataToPassThrough, true);

		return;
	}


	FillCalculateColorsInfoFundementals(calculateColorsInfo);

	AddTaskToQueue(calculateColorsInfo, AdditionalDataToPassThrough);
}

bool UVertexPaintDetectionComponent::GetClosestVertexDataOnMeshCheckValid(const FRVPDPGetClosestVertexDataSettings& GetClosestVertexDataSettings) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetClosestVertexDataOnMeshCheckValid);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - GetClosestVertexDataOnMeshCheckValid");

	return DetectTaskCheckValid(GetClosestVertexDataSettings, GetClosestVertexDataSettings.MeshComponent);
}

void UVertexPaintDetectionComponent::GetClosestVertexDataOnMeshTaskFinished(const FRVPDPTaskResults& DetectTaskResult, const FRVPDPGetClosestVertexDataSettings& GetClosestVertexDataSettings, const FRVPDPClosestVertexDataResults& ClosestVertexColorResult, const FRVPDPEstimatedColorAtHitLocationInfo& EstimatedColorAtHitLocationResult, const FRVPDPAverageColorInAreaInfo& AverageColor, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough, bool FailedAtCheckValid) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetClosestVertexDataOnMeshTaskFinished);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - GetClosestVertexDataOnMeshTaskFinished");


	if (CurrentGetClosestVertexDataTasks.Contains(DetectTaskResult.TaskID))
		CurrentGetClosestVertexDataTasks.Remove(DetectTaskResult.TaskID);
	
	if (TaskFundamentalSettings.CallbackSettings.RunCallbackDelegate) {

		GetClosestVertexDataDelegate.Broadcast(DetectTaskResult, GetClosestVertexDataSettings, ClosestVertexColorResult, EstimatedColorAtHitLocationResult, AverageColor, AdditionalDataToPassThrough);
	}


	// If hasn't been assigned Task ID, i.e. failed at check valid before the Task Started then the paint component makes sure we run all callbacks. 
	if (DetectTaskResult.TaskID < 0) {

		UVertexPaintFunctionLibrary::RunGetClosestVertexDataCallbackInterfaces(DetectTaskResult, GetClosestVertexDataSettings, ClosestVertexColorResult, EstimatedColorAtHitLocationResult, AverageColor, TaskFundamentalSettings, AdditionalDataToPassThrough);
		UVertexPaintFunctionLibrary::RunFinishedDetectTaskCallbacks(DetectTaskResult, TaskFundamentalSettings, AdditionalDataToPassThrough);
	}
}


//-------------------------------------------------------

// Get Vertex Colors Only

void UVertexPaintDetectionComponent::GetAllVertexColorsOnly(FRVPDPGetColorsOnlySettings GetAllVertexColorsStruct, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough) {

	if (!UVertexPaintFunctionLibrary::IsWorldValid(GetWorld())) return;

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetAllVertexColorsOnly);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - GetAllVertexColorsOnly");


	CheckIfRegisteredAndInitialized();

	GetAllVertexColorsStruct.WeakMeshComponent = GetAllVertexColorsStruct.MeshComponent;
	GetAllVertexColorsStruct.TaskWorld = GetWorld();
	GetAllVertexColorsStruct.DebugSettings.WorldTaskWasCreatedIn = GetWorld();

	if (IsValid(GetAllVertexColorsStruct.MeshComponent)) {

		if (IsValid(GetAllVertexColorsStruct.MeshComponent->GetOwner())) {

			RVPDP_LOG(GetAllVertexColorsStruct.DebugSettings, FColor::Cyan, "Trying to Get Colors Only on Actor: %s and Component: %s", *GetAllVertexColorsStruct.MeshComponent->GetOwner()->GetName(), *GetAllVertexColorsStruct.MeshComponent->GetName());
		}
	}


	GetCompleteCallbackSettings(GetAllVertexColorsStruct.MeshComponent, GetAllVertexColorsStruct.AutoSetRequiredCallbackSettings, GetAllVertexColorsStruct.CallbackSettings);


	FRVPDPCalculateColorsInfo calculateColorsInfo;
	calculateColorsInfo.DetectSettings = GetAllVertexColorsStruct;
	calculateColorsInfo.IsDetectTask = true;
	calculateColorsInfo.VertexPaintDetectionType = EVertexPaintDetectionType::GetAllVertexColorDetection;
	calculateColorsInfo.GetAllVertexColorsSettings = GetAllVertexColorsStruct;
	calculateColorsInfo.TaskFundamentalSettings = GetAllVertexColorsStruct;


	if (GetAllVertexColorsOnlyCheckValid(GetAllVertexColorsStruct)) {

		RVPDP_LOG(GetAllVertexColorsStruct.DebugSettings, FColor::Green, "Get All Vertex Colors Only CheckValid Successful.");
	}

	else {

		RVPDP_LOG(GetAllVertexColorsStruct.DebugSettings, FColor::Red, "Check Valid Failed at Get All Vertex Colors Only!");

		GetTaskFundamentalsSettings(GetAllVertexColorsStruct.MeshComponent, calculateColorsInfo);

		GetAllVertexColorsOnlyTaskFinished(calculateColorsInfo.TaskResult, GetAllVertexColorsStruct, calculateColorsInfo.TaskFundamentalSettings, AdditionalDataToPassThrough, true);
		return;
	}


	FillCalculateColorsInfoFundementals(calculateColorsInfo);

	AddTaskToQueue(calculateColorsInfo, AdditionalDataToPassThrough);
}

bool UVertexPaintDetectionComponent::GetAllVertexColorsOnlyCheckValid(const FRVPDPGetColorsOnlySettings& GetAllVertexColorsSettings) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetAllVertexColorsOnlyCheckValid);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - GetAllVertexColorsOnlyCheckValid");

	return DetectTaskCheckValid(GetAllVertexColorsSettings, GetAllVertexColorsSettings.MeshComponent);
}

void UVertexPaintDetectionComponent::GetAllVertexColorsOnlyTaskFinished(const FRVPDPTaskResults& DetectTaskResult, const FRVPDPGetColorsOnlySettings& GetAllVertexColorsSettings, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough, bool FailedAtCheckValid) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetAllVertexColorsOnlyTaskFinished);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - GetAllVertexColorsOnlyTaskFinished");


	if (CurrentGetAllVertexColorsOnlyTasks.Contains(DetectTaskResult.TaskID))
		CurrentGetAllVertexColorsOnlyTasks.Remove(DetectTaskResult.TaskID);

	if (TaskFundamentalSettings.CallbackSettings.RunCallbackDelegate) {

		GetAllVertexColorsOnlyDelegate.Broadcast(DetectTaskResult, GetAllVertexColorsSettings, AdditionalDataToPassThrough);
	}
	

	// If hasn't been assigned Task ID, i.e. failed at check valid before the Task Started then the paint component makes sure we run all callbacks. 
	if (DetectTaskResult.TaskID < 0) {

		UVertexPaintFunctionLibrary::RunGetAllColorsOnlyCallbackInterfaces(DetectTaskResult, GetAllVertexColorsSettings, TaskFundamentalSettings, AdditionalDataToPassThrough);
		UVertexPaintFunctionLibrary::RunFinishedDetectTaskCallbacks(DetectTaskResult, TaskFundamentalSettings, AdditionalDataToPassThrough);
	}
}


//-------------------------------------------------------

// Get Colors Within Area

void UVertexPaintDetectionComponent::GetColorsWithinArea(FRVPDPGetColorsWithinAreaSettings GetColorsWithinArea, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough) {

	if (!UVertexPaintFunctionLibrary::IsWorldValid(GetWorld())) return;

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetColorsWithinArea);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - GetColorsWithinArea");


	CheckIfRegisteredAndInitialized();

	GetColorsWithinArea.WeakMeshComponent = GetColorsWithinArea.MeshComponent;
	GetColorsWithinArea.TaskWorld = GetWorld();
	GetColorsWithinArea.DebugSettings.WorldTaskWasCreatedIn = GetWorld();

	if (IsValid(GetColorsWithinArea.MeshComponent)) {

		if (IsValid(GetColorsWithinArea.MeshComponent->GetOwner())) {

			RVPDP_LOG(GetColorsWithinArea.DebugSettings, FColor::Cyan, "Trying to Get Colors Within Area on Actor: %s and Component: %s", *GetColorsWithinArea.MeshComponent->GetOwner()->GetName(), *GetColorsWithinArea.MeshComponent->GetName());
		}
	}


	GetCompleteCallbackSettings(GetColorsWithinArea.MeshComponent, GetColorsWithinArea.AutoSetRequiredCallbackSettings, GetColorsWithinArea.CallbackSettings);

	GetColorsWithinArea.WithinAreaSettings.ComponentsToCheckIfIsWithin = GetWithinAreaComponentsToCheckIfIsWithinSettings(GetColorsWithinArea.MeshComponent, GetColorsWithinArea.WithinAreaSettings);

	if (GetColorsWithinArea.WithinAreaSettings.TraceForSpecificBonesWithinArea && Cast<USkeletalMeshComponent>(GetColorsWithinArea.MeshComponent)) {

		if (!UVertexPaintFunctionLibrary::ShouldTaskLoopThroughAllVerticesOnLOD(GetColorsWithinArea.CallbackSettings, GetColorsWithinArea.MeshComponent, FRVPDPOverrideColorsToApplySettings())) {

			// If we're going to loop through all vertices anyway then there is no point in tracing for the bones within area
			GetColorsWithinArea.WithinAreaSettings.SpecificBonesWithinArea = GetMeshBonesWithinComponentsToCheckIfWithinArea(GetColorsWithinArea.MeshComponent, GetColorsWithinArea.WithinAreaSettings.ComponentsToCheckIfIsWithin, GetColorsWithinArea.DebugSettings);
		}
	}

	if (GetColorsWithinArea.DebugSettings.TaskSpecificDebugSymbols.DrawTaskDebugSymbols) {

		DrawWithinAreaDebugSymbols(GetColorsWithinArea.WithinAreaSettings.ComponentsToCheckIfIsWithin, GetColorsWithinArea.DebugSettings.TaskSpecificDebugSymbols.DrawTaskDebugSymbolsDuration);
	}


	FRVPDPCalculateColorsInfo calculateColorsInfo;
	calculateColorsInfo.DetectSettings = GetColorsWithinArea;
	calculateColorsInfo.IsDetectTask = true;
	calculateColorsInfo.VertexPaintDetectionType = EVertexPaintDetectionType::GetColorsWithinArea;
	calculateColorsInfo.GetColorsWithinAreaSettings = GetColorsWithinArea;
	calculateColorsInfo.TaskFundamentalSettings = GetColorsWithinArea;
	calculateColorsInfo.WithinAreaSettings = GetColorsWithinArea.WithinAreaSettings;

	
	if (GetColorsWithinAreaCheckValid(GetColorsWithinArea)) {

		RVPDP_LOG(GetColorsWithinArea.DebugSettings, FColor::Green, "Get Colors Within Area CheckValid Successful");
	}

	else {

		RVPDP_LOG(GetColorsWithinArea.DebugSettings, FColor::Red, "Check Valid Failed at Get Colors Within Area!");

		GetTaskFundamentalsSettings(GetColorsWithinArea.MeshComponent, calculateColorsInfo);

		GetColorsWithinAreaTaskFinished(calculateColorsInfo.TaskResult, GetColorsWithinArea, calculateColorsInfo.WithinArea_Results_BeforeApplyingColors, calculateColorsInfo.TaskFundamentalSettings, AdditionalDataToPassThrough, true);
		return;
	}


	FillCalculateColorsInfoFundementals(calculateColorsInfo);

	AddTaskToQueue(calculateColorsInfo, AdditionalDataToPassThrough);
}

bool UVertexPaintDetectionComponent::GetColorsWithinAreaCheckValid(const FRVPDPGetColorsWithinAreaSettings& GetWithinAreaSettings) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetColorsWithinAreaCheckValid);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - GetColorsWithinAreaCheckValid");


	if (!DetectTaskCheckValid(GetWithinAreaSettings, GetWithinAreaSettings.MeshComponent))
		return false;

	if (!WithinAreaCheckValid(GetWithinAreaSettings.WithinAreaSettings, GetWithinAreaSettings.DebugSettings))
		return false;

	return true;
}

void UVertexPaintDetectionComponent::GetColorsWithinAreaTaskFinished(const FRVPDPTaskResults& DetectTaskResult, const FRVPDPGetColorsWithinAreaSettings& GetColorsWithinAreaSettings, const FRVPDPWithinAreaResults& WithinAreaResults, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough, bool FailedAtCheckValid) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetColorsWithinAreaTaskFinished);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - GetColorsWithinAreaTaskFinished");


	if (CurrentGetColorsWithinAreaTasks.Contains(DetectTaskResult.TaskID))
		CurrentGetColorsWithinAreaTasks.Remove(DetectTaskResult.TaskID);

	if (TaskFundamentalSettings.CallbackSettings.RunCallbackDelegate) {

		GetColorsWithinAreaDelegate.Broadcast(DetectTaskResult, GetColorsWithinAreaSettings, WithinAreaResults, AdditionalDataToPassThrough);
	}


	// If failed at fundamental checks here then makes sure we run callback interfaces from here as well as the game instance won't get a callback that a task has finished. 
	if (FailedAtCheckValid) {

		UVertexPaintFunctionLibrary::RunGetColorsWithinAreaCallbackInterfaces(DetectTaskResult, GetColorsWithinAreaSettings, WithinAreaResults, TaskFundamentalSettings, AdditionalDataToPassThrough);
		UVertexPaintFunctionLibrary::RunFinishedDetectTaskCallbacks(DetectTaskResult, TaskFundamentalSettings, AdditionalDataToPassThrough);
	}
}


//-------------------------------------------------------

// Paint On Mesh At Location

void UVertexPaintDetectionComponent::PaintOnMeshAtLocation(FRVPDPPaintAtLocationSettings PaintAtLocationStruct, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough) {

	if (!UVertexPaintFunctionLibrary::IsWorldValid(GetWorld())) return;

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_PaintOnMeshAtLocation);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - PaintOnMeshAtLocation");


	CheckIfRegisteredAndInitialized();

	PaintAtLocationStruct.WeakMeshComponent = PaintAtLocationStruct.MeshComponent;
	PaintAtLocationStruct.TaskWorld = GetWorld();
	PaintAtLocationStruct.DebugSettings.WorldTaskWasCreatedIn = GetWorld();

	if (IsValid(PaintAtLocationStruct.MeshComponent)) {

		if (IsValid(PaintAtLocationStruct.MeshComponent->GetOwner())) {

			RVPDP_LOG(PaintAtLocationStruct.DebugSettings, FColor::Cyan, "Trying to Paint on Mesh at Location on Actor: %s and Component: %s", *PaintAtLocationStruct.MeshComponent->GetOwner()->GetName(), *PaintAtLocationStruct.MeshComponent->GetName());
		}

		PaintAtLocationStruct.HitFundementals.HitLocationInComponentSpace = UKismetMathLibrary::InverseTransformLocation(PaintAtLocationStruct.MeshComponent->GetComponentTransform(), PaintAtLocationStruct.HitFundementals.HitLocation);
	}


	GetCompleteCallbackSettings(PaintAtLocationStruct.MeshComponent, PaintAtLocationStruct.AutoSetRequiredCallbackSettings, PaintAtLocationStruct.CallbackSettings);

	// Clamps it so if the user sets like 0.000001, we will paint at the lowest amount of 1 when converted to FColor which range from 0-255. Also clamps paint limits and conditions
	ClampPaintSettings(PaintAtLocationStruct.ApplyVertexColorSettings);

	PaintAtLocationStruct.PaintAtAreaSettings.FallOffSettings.ColorLimitAtFallOffEdge = UKismetMathLibrary::FClamp(PaintAtLocationStruct.PaintAtAreaSettings.FallOffSettings.ColorLimitAtFallOffEdge, 0, 1);


	// Fall Off Range
	if (PaintAtLocationStruct.PaintAtAreaSettings.FallOffSettings.PaintAtLocationFallOffType == EVertexPaintAtLocationFallOffType::InwardFallOff)
		PaintAtLocationStruct.PaintAtAreaSettings.PaintAtLocationFallOffMaxRange = PaintAtLocationStruct.PaintAtAreaSettings.AreaOfEffectRangeStart;

	else if (PaintAtLocationStruct.PaintAtAreaSettings.FallOffSettings.PaintAtLocationFallOffType == EVertexPaintAtLocationFallOffType::OutwardFallOff)
		PaintAtLocationStruct.PaintAtAreaSettings.PaintAtLocationFallOffMaxRange = PaintAtLocationStruct.PaintAtAreaSettings.AreaOfEffectRangeEnd;

	else if (PaintAtLocationStruct.PaintAtAreaSettings.FallOffSettings.PaintAtLocationFallOffType == EVertexPaintAtLocationFallOffType::SphericalFallOff)
		PaintAtLocationStruct.PaintAtAreaSettings.PaintAtLocationFallOffMaxRange = PaintAtLocationStruct.PaintAtAreaSettings.AreaOfEffectRangeStart - PaintAtLocationStruct.PaintAtAreaSettings.AreaOfEffectRangeEnd;


	// How we scale the Fall Off, from base outwardly or opposite
	if (PaintAtLocationStruct.PaintAtAreaSettings.FallOffSettings.StartFallOffDistanceFrom == FRVPDPStartFallOffDistanceFromSetting::BaseOfPaintShape)
		PaintAtLocationStruct.PaintAtAreaSettings.PaintAtLocationScaleFallOffFrom = PaintAtLocationStruct.PaintAtAreaSettings.FallOffSettings.DistanceToStartFallOffFrom;

	else if (PaintAtLocationStruct.PaintAtAreaSettings.FallOffSettings.StartFallOffDistanceFrom == FRVPDPStartFallOffDistanceFromSetting::EndOfPaintShape)
		PaintAtLocationStruct.PaintAtAreaSettings.PaintAtLocationScaleFallOffFrom = PaintAtLocationStruct.PaintAtAreaSettings.AreaOfEffectRangeEnd - PaintAtLocationStruct.PaintAtAreaSettings.FallOffSettings.DistanceToStartFallOffFrom;


	FRVPDPGetClosestVertexDataSettings detectSettings;

	// Sets these if gonna detect before painting so can use them in case we need to do any checks in check valid etc. 
	if (PaintAtLocationStruct.GetClosestVertexDataCombo.RunGetClosestVertexDataOnMeshBeforeApplyingPaint) {

		detectSettings.SetPaintDetectionFundementals(PaintAtLocationStruct);

		detectSettings.GetEstimatedColorAtHitLocationSettings = PaintAtLocationStruct.GetEstimatedColorAtHitLocationSettings;
		detectSettings.GetAverageColorInAreaSettings = PaintAtLocationStruct.GetClosestVertexDataCombo.GetAverageColorBeforeApplyingColorSettings;
		detectSettings.GetEstimatedColorAtHitLocationSettings = PaintAtLocationStruct.GetEstimatedColorAtHitLocationSettings;

		if (PaintAtLocationStruct.GetClosestVertexDataCombo.UseCustomHitSettings) {

			detectSettings.HitFundementals.HitLocation = PaintAtLocationStruct.GetClosestVertexDataCombo.CustomHitSettings.HitLocation;
			detectSettings.HitFundementals.HitLocationInComponentSpace = UKismetMathLibrary::InverseTransformLocation(PaintAtLocationStruct.MeshComponent->GetComponentTransform(), PaintAtLocationStruct.GetClosestVertexDataCombo.CustomHitSettings.HitLocation);
			detectSettings.HitFundementals.HitNormal = PaintAtLocationStruct.GetClosestVertexDataCombo.CustomHitSettings.HitNormal;
			detectSettings.HitFundementals.RunTaskFor = PaintAtLocationStruct.GetClosestVertexDataCombo.CustomHitSettings.RunTaskFor;
			detectSettings.HitFundementals.HitBone = PaintAtLocationStruct.GetClosestVertexDataCombo.CustomHitSettings.HitBone;
		}

		else {

			detectSettings.HitFundementals.HitLocation = PaintAtLocationStruct.HitFundementals.HitLocation;
			detectSettings.HitFundementals.HitLocationInComponentSpace = UKismetMathLibrary::InverseTransformLocation(PaintAtLocationStruct.MeshComponent->GetComponentTransform(), PaintAtLocationStruct.HitFundementals.HitLocation);
			detectSettings.HitFundementals.HitNormal = PaintAtLocationStruct.HitFundementals.HitNormal;
			detectSettings.HitFundementals.RunTaskFor = PaintAtLocationStruct.HitFundementals.RunTaskFor;
			detectSettings.HitFundementals.HitBone = PaintAtLocationStruct.HitFundementals.HitBone;
		}
	}


	FRVPDPCalculateColorsInfo calculateColorsInfo;
	calculateColorsInfo.IsDetectCombo = PaintAtLocationStruct.GetClosestVertexDataCombo.RunGetClosestVertexDataOnMeshBeforeApplyingPaint;
	calculateColorsInfo.DetectSettings = detectSettings;
	calculateColorsInfo.IsPaintTask = true;
	calculateColorsInfo.VertexPaintDetectionType = EVertexPaintDetectionType::PaintAtLocation;
	calculateColorsInfo.PaintAtLocationSettings = PaintAtLocationStruct;
	calculateColorsInfo.EstimatedColorAtHitLocationSettings = PaintAtLocationStruct.GetEstimatedColorAtHitLocationSettings;
	calculateColorsInfo.GetClosestVertexDataSettings = detectSettings;
	calculateColorsInfo.ApplyColorSettings = PaintAtLocationStruct;
	calculateColorsInfo.PaintTaskSettings = PaintAtLocationStruct;
	calculateColorsInfo.TaskFundamentalSettings = PaintAtLocationStruct;


	if (PaintOnMeshAtLocationCheckValid(PaintAtLocationStruct)) {

		if (PaintAtLocationStruct.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.ApplyVertexColorUsingPhysicsSurface) {

			RVPDP_LOG(PaintAtLocationStruct.DebugSettings, FColor::Green, "Paint on Mesh at Location CheckValid Successful. Trying to Apply Color with Physics Surfaces: %s  -  Paint Strength on Surfaces Without the Physical Surface: %f", *GetAllPhysicsSurfacesToApplyAsString(PaintAtLocationStruct), PaintAtLocationStruct.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.StrengtOnChannelsWithoutThePhysicsSurfaces);
		}

		else {

			RVPDP_LOG(PaintAtLocationStruct.DebugSettings, FColor::Green, "Paint on Mesh at Location CheckValid Successful. Trying to Apply Color: %s  Area of Effect Min: %f Area of Effect Max: %f  Falloff Strength: %f", *FLinearColor(PaintAtLocationStruct.ApplyVertexColorSettings.ApplyColorsOnRedChannel.AmountToApply, PaintAtLocationStruct.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.AmountToApply, PaintAtLocationStruct.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.AmountToApply, PaintAtLocationStruct.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.AmountToApply).ToString(), PaintAtLocationStruct.PaintAtAreaSettings.AreaOfEffectRangeStart, PaintAtLocationStruct.PaintAtAreaSettings.AreaOfEffectRangeEnd, PaintAtLocationStruct.PaintAtAreaSettings.FallOffSettings.FallOffStrength);
		}
	}

	else {

		RVPDP_LOG(PaintAtLocationStruct.DebugSettings, FColor::Red, "Check Valid Failed at Paint on Mesh at Location!");

		GetTaskFundamentalsSettings(PaintAtLocationStruct.MeshComponent, calculateColorsInfo);


		// If fail and set to run detections then we expect fail callbacks on the get closest vertex data delegates as well
		if (PaintAtLocationStruct.GetClosestVertexDataCombo.RunGetClosestVertexDataOnMeshBeforeApplyingPaint) {

			GetClosestVertexDataOnMeshTaskFinished(calculateColorsInfo.TaskResult, detectSettings, calculateColorsInfo.GetClosestVertexDataResult, calculateColorsInfo.GetClosestVertexData_EstimatedColorAtHitLocationResult, calculateColorsInfo.GetClosestVertexData_AverageColorInArea, detectSettings, AdditionalDataToPassThrough, true);
		}


		PaintOnMeshAtLocationTaskFinished(calculateColorsInfo.TaskResult, calculateColorsInfo.PaintTaskResult, calculateColorsInfo.PaintAtLocationSettings, calculateColorsInfo.GetClosestVertexDataResult, calculateColorsInfo.GetClosestVertexData_EstimatedColorAtHitLocationResult, calculateColorsInfo.PaintAtLocation_AverageColorInArea, calculateColorsInfo.TaskFundamentalSettings, AdditionalDataToPassThrough, true);

		return;
	}


	FillCalculateColorsInfoFundementals(calculateColorsInfo);

	AddTaskToQueue(calculateColorsInfo, AdditionalDataToPassThrough);
}

bool UVertexPaintDetectionComponent::PaintOnMeshAtLocationCheckValid(const FRVPDPPaintAtLocationSettings& PaintAtLocationSettings, bool IgnoreTaskQueueChecks) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_PaintOnMeshAtLocationCheckValid);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - PaintOnMeshAtLocationCheckValid");


	if (!PaintColorsTaskCheckValid(PaintAtLocationSettings, PaintAtLocationSettings.MeshComponent, IgnoreTaskQueueChecks))
		return false;

	if (PaintAtLocationSettings.PaintAtAreaSettings.AreaOfEffectRangeStart > PaintAtLocationSettings.PaintAtAreaSettings.AreaOfEffectRangeEnd) {

		RVPDP_LOG(PaintAtLocationSettings.DebugSettings, FColor::Red, "Check Valid, Paint at Location Fail: Paint Area of Effect is > than areaOfEffectMax");

		return false;
	}


	if (PaintAtLocationSettings.PaintAtAreaSettings.AreaOfEffectRangeStart <= 0 && PaintAtLocationSettings.PaintAtAreaSettings.AreaOfEffectRangeEnd <= 0) {

		RVPDP_LOG(PaintAtLocationSettings.DebugSettings, FColor::Red, "Check Valid, Paint at Location Fail: Paint Area of Effect is <= 0");

		return false;
	}


	return true;
}

void UVertexPaintDetectionComponent::PaintOnMeshAtLocationTaskFinished(const FRVPDPTaskResults& TaskResult, const FRVPDPPaintTaskResultInfo& PaintTaskResult, const FRVPDPPaintAtLocationSettings& PaintAtLocationSettings, const FRVPDPClosestVertexDataResults& ClosestVertexColorResult, const FRVPDPEstimatedColorAtHitLocationInfo& EstimatedColorAtHitLocationResult, const FRVPDPAverageColorInAreaInfo& AverageColor, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough, bool FailedAtCheckValid) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_PaintOnMeshAtLocationTaskFinished);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - PaintOnMeshAtLocationTaskFinished");


	if (CurrentPaintAtLocationTasks.Contains(TaskResult.TaskID))
		CurrentPaintAtLocationTasks.Remove(TaskResult.TaskID);

	if (TaskFundamentalSettings.CallbackSettings.RunCallbackDelegate) {

		VertexColorsPaintedAtLocationDelegate.Broadcast(TaskResult, PaintTaskResult, PaintAtLocationSettings, ClosestVertexColorResult, EstimatedColorAtHitLocationResult, AverageColor, AdditionalDataToPassThrough);
	}


	// If failed at fundamental checks here then makes sure we run callback interfaces from here as well as the game instance won't get a callback that a task has finished. 
	if (FailedAtCheckValid) {

		UVertexPaintFunctionLibrary::RunPaintAtLocationCallbackInterfaces(TaskResult, PaintTaskResult, PaintAtLocationSettings, ClosestVertexColorResult, EstimatedColorAtHitLocationResult, AverageColor, TaskFundamentalSettings, AdditionalDataToPassThrough);
		UVertexPaintFunctionLibrary::RunFinishedPaintTaskCallbacks(TaskResult, PaintTaskResult, TaskFundamentalSettings, AdditionalDataToPassThrough);
	}
}


//-------------------------------------------------------

// Paint Within Area

void UVertexPaintDetectionComponent::PaintOnMeshWithinArea(FRVPDPPaintWithinAreaSettings PaintWithinAreaStruct, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough) {

	if (!UVertexPaintFunctionLibrary::IsWorldValid(GetWorld())) return;

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_PaintOnMeshWithinArea);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - PaintOnMeshWithinArea");


	CheckIfRegisteredAndInitialized();

	PaintWithinAreaStruct.WeakMeshComponent = PaintWithinAreaStruct.MeshComponent;
	PaintWithinAreaStruct.TaskWorld = GetWorld();
	PaintWithinAreaStruct.DebugSettings.WorldTaskWasCreatedIn = GetWorld();

	if (IsValid(PaintWithinAreaStruct.MeshComponent)) {

		if (IsValid(PaintWithinAreaStruct.MeshComponent->GetOwner())) {

			RVPDP_LOG(PaintWithinAreaStruct.DebugSettings, FColor::Cyan, "Trying to Paint On Mesh Within Area on Actor: %s and Component: %s", *PaintWithinAreaStruct.MeshComponent->GetOwner()->GetName(), *PaintWithinAreaStruct.MeshComponent->GetName());
		}
	}


	GetCompleteCallbackSettings(PaintWithinAreaStruct.MeshComponent, PaintWithinAreaStruct.AutoSetRequiredCallbackSettings, PaintWithinAreaStruct.CallbackSettings);

	// Clamps it so if the user sets like 0.000001, we will paint at the lowest amount of 1 when converted to FColor which range from 0-255. Also clamps paint limits and conditions
	ClampPaintSettings(PaintWithinAreaStruct.ApplyVertexColorSettings);

	PaintWithinAreaStruct.WithinAreaSettings.ComponentsToCheckIfIsWithin = GetWithinAreaComponentsToCheckIfIsWithinSettings(PaintWithinAreaStruct.MeshComponent, PaintWithinAreaStruct.WithinAreaSettings);


	if (!PaintWithinAreaStruct.ColorToApplyToVerticesOutsideOfArea.IsGoingToApplyAnyColorOutsideOfArea() && PaintWithinAreaStruct.WithinAreaSettings.TraceForSpecificBonesWithinArea && Cast<USkeletalMeshComponent>(PaintWithinAreaStruct.MeshComponent)) {

		// If we're going to loop through all vertices anyway then there is no point in tracing for the bones within area
		if (!UVertexPaintFunctionLibrary::ShouldTaskLoopThroughAllVerticesOnLOD(PaintWithinAreaStruct.CallbackSettings, PaintWithinAreaStruct.MeshComponent, PaintWithinAreaStruct.OverrideVertexColorsToApplySettings)) {

			PaintWithinAreaStruct.WithinAreaSettings.SpecificBonesWithinArea = GetMeshBonesWithinComponentsToCheckIfWithinArea(PaintWithinAreaStruct.MeshComponent, PaintWithinAreaStruct.WithinAreaSettings.ComponentsToCheckIfIsWithin, PaintWithinAreaStruct.DebugSettings);
		}
	}


	if (PaintWithinAreaStruct.DebugSettings.TaskSpecificDebugSymbols.DrawTaskDebugSymbols) {

		DrawWithinAreaDebugSymbols(PaintWithinAreaStruct.WithinAreaSettings.ComponentsToCheckIfIsWithin, PaintWithinAreaStruct.DebugSettings.TaskSpecificDebugSymbols.DrawTaskDebugSymbolsDuration);
	}


	FRVPDPGetColorsWithinAreaSettings getColorsWithinAreaSettings;

	if (PaintWithinAreaStruct.GetColorsWithinAreaCombo) {

		getColorsWithinAreaSettings.SetPaintDetectionFundementals(PaintWithinAreaStruct);
		getColorsWithinAreaSettings.WithinAreaSettings = PaintWithinAreaStruct.WithinAreaSettings;
	}

	FRVPDPCalculateColorsInfo calculateColorsInfo;
	calculateColorsInfo.IsDetectCombo = PaintWithinAreaStruct.GetColorsWithinAreaCombo;
	calculateColorsInfo.DetectSettings = getColorsWithinAreaSettings;
	calculateColorsInfo.IsPaintTask = true;
	calculateColorsInfo.VertexPaintDetectionType = EVertexPaintDetectionType::PaintWithinArea;
	calculateColorsInfo.ApplyColorSettings = PaintWithinAreaStruct;
	calculateColorsInfo.PaintTaskSettings = PaintWithinAreaStruct;
	calculateColorsInfo.TaskFundamentalSettings = PaintWithinAreaStruct;
	calculateColorsInfo.WithinAreaSettings = PaintWithinAreaStruct.WithinAreaSettings;
	calculateColorsInfo.GetColorsWithinAreaSettings = getColorsWithinAreaSettings;
	calculateColorsInfo.PaintWithinAreaSettings = PaintWithinAreaStruct;

	if (PaintOnMeshWithinAreaCheckValid(PaintWithinAreaStruct)) {

		if (PaintWithinAreaStruct.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.ApplyVertexColorUsingPhysicsSurface) {

			RVPDP_LOG(PaintWithinAreaStruct.DebugSettings, FColor::Green, "Paint on Mesh Within Area CheckValid Successful. Trying to Apply Color with Physics Surfaces: %s  -  Paint Strength on Surfaces Without the Physical Surface: %f", *GetAllPhysicsSurfacesToApplyAsString(PaintWithinAreaStruct), PaintWithinAreaStruct.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.StrengtOnChannelsWithoutThePhysicsSurfaces);
		}

		else {

			RVPDP_LOG(PaintWithinAreaStruct.DebugSettings, FColor::Green, "Paint on Mesh Within Area CheckValid Successful. Trying to Apply Color: %s", *FLinearColor(PaintWithinAreaStruct.ApplyVertexColorSettings.ApplyColorsOnRedChannel.AmountToApply, PaintWithinAreaStruct.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.AmountToApply, PaintWithinAreaStruct.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.AmountToApply, PaintWithinAreaStruct.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.AmountToApply).ToString());
		}
	}

	else {

		RVPDP_LOG(PaintWithinAreaStruct.DebugSettings, FColor::Red, "Check Valid Failed at Paint on Mesh Within Area!");

		GetTaskFundamentalsSettings(PaintWithinAreaStruct.MeshComponent, calculateColorsInfo);

		PaintOnMeshWithinAreaTaskFinished(calculateColorsInfo.TaskResult, calculateColorsInfo.PaintTaskResult, calculateColorsInfo.PaintWithinAreaSettings, calculateColorsInfo.WithinArea_Results_AfterApplyingColors, calculateColorsInfo.TaskFundamentalSettings, AdditionalDataToPassThrough, true);
		return;
	}


	FillCalculateColorsInfoFundementals(calculateColorsInfo);

	AddTaskToQueue(calculateColorsInfo, AdditionalDataToPassThrough);
}

bool UVertexPaintDetectionComponent::PaintOnMeshWithinAreaCheckValid(const FRVPDPPaintWithinAreaSettings& PaintWithinAreaSettings, bool IgnoreTaskQueueChecks) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_PaintOnMeshWithinAreaCheckValid);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - PaintOnMeshWithinAreaCheckValid");

	if (!PaintColorsTaskCheckValid(PaintWithinAreaSettings, PaintWithinAreaSettings.MeshComponent, IgnoreTaskQueueChecks))
		return false;


	if (!WithinAreaCheckValid(PaintWithinAreaSettings.WithinAreaSettings, PaintWithinAreaSettings.DebugSettings))
		return false;

	return true;
}

bool UVertexPaintDetectionComponent::WithinAreaCheckValid(const FRVPDPWithinAreaSettings& WithinAreaSettings, const FRVPDPDebugSettings& DebugSettings) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_WithinAreaCheckValid);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - WithinAreaCheckValid");


	if (WithinAreaSettings.ComponentsToCheckIfIsWithin.Num() == 0) {

		RVPDP_LOG(DebugSettings, FColor::Red, "Within Area Check Valid Fail! It has no ComponentsToCheckIfIsWithin!");

		return false;
	}


	for (const FRVPDPComponentToCheckIfIsWithinAreaInfo& compsToCheckWithinAreaInfo : WithinAreaSettings.ComponentsToCheckIfIsWithin) {

		if (IsValid(compsToCheckWithinAreaInfo.GetComponentToCheckIfIsWithin())) {

			// Some component types require a specific shape to be set
			if (Cast<ALandscape>(compsToCheckWithinAreaInfo.GetComponentToCheckIfIsWithin()->GetOwner())) {

				if (compsToCheckWithinAreaInfo.PaintWithinAreaShape != EPaintWithinAreaShape::IsComplexShape) {

					RVPDP_LOG(DebugSettings, FColor::Red, "Within Area Check Valid Failed since one of the componentToCheckIfWithin is a Landscape, but the WithinArea Shape is not set to be Complex Shape which is required to check if you're within Landscapes!");

					return false;
				}
			}

			else if (Cast<USphereComponent>(compsToCheckWithinAreaInfo.GetComponentToCheckIfIsWithin())) {

				if (compsToCheckWithinAreaInfo.PaintWithinAreaShape != EPaintWithinAreaShape::IsSphereShape) {

					RVPDP_LOG(DebugSettings, FColor::Red, "Within Area Check Valid Failed since one of the componentToCheckIfWithin is a Sphere Component, but the WithinArea Shape is not set to be sphereShape!");

					return false;
				}
			}

			else if (Cast<UBoxComponent>(compsToCheckWithinAreaInfo.GetComponentToCheckIfIsWithin())) {

				if (compsToCheckWithinAreaInfo.PaintWithinAreaShape != EPaintWithinAreaShape::IsSquareOrRectangleShape) {

					RVPDP_LOG(DebugSettings, FColor::Red, "Within Area Check Valid Failed since one of the componentToCheckIfWithin is a Box Component, but the WithinArea Shape is not set to be Square or Rectangle Shape!");

					return false;
				}
			}

			else if (Cast<UCapsuleComponent>(compsToCheckWithinAreaInfo.GetComponentToCheckIfIsWithin())) {

				if (compsToCheckWithinAreaInfo.PaintWithinAreaShape != EPaintWithinAreaShape::IsCapsuleShape) {

					RVPDP_LOG(DebugSettings, FColor::Red, "Within Area Check Valid Failed since one of the componentToCheckIfWithin is a Capsule Component, but the WithinArea Shape is not set to be Capsule Shape!");

					return false;
				}
			}

			else if (Cast<USkeletalMeshComponent>(compsToCheckWithinAreaInfo.GetComponentToCheckIfIsWithin())) {

				if (compsToCheckWithinAreaInfo.PaintWithinAreaShape != EPaintWithinAreaShape::IsComplexShape) {

					RVPDP_LOG(DebugSettings, FColor::Red, "Within Area Check Valid Failed since one of the componentToCheckIfWithin is a Skeletal Mesh Component, but the WithinArea Shape is not set to be Complex!");

					return false;
				}
			}

			else if (Cast<USplineMeshComponent>(compsToCheckWithinAreaInfo.GetComponentToCheckIfIsWithin())) {

				if (compsToCheckWithinAreaInfo.PaintWithinAreaShape != EPaintWithinAreaShape::IsComplexShape) {

					RVPDP_LOG(DebugSettings, FColor::Red, "Within Area Check Valid Failed since one of the componentToCheckIfWithin is a Spline Mesh Component, but the WithinArea Shape is not set to be Complex!");

					return false;
				}
			}

#if ENGINE_MAJOR_VERSION == 5

			else if (Cast<UDynamicMeshComponent>(compsToCheckWithinAreaInfo.GetComponentToCheckIfIsWithin())) {

				RVPDP_LOG(DebugSettings, FColor::Red, "Within Area Check Valid Failed since one of the componentToCheckIfWithin is a Dynamic Mesh Component, but we don't support those types of components for use with Within Area");

				return false;
			}

#if ENGINE_MINOR_VERSION >= 3
			else if (Cast<UGeometryCollectionComponent>(compsToCheckWithinAreaInfo.ComponentToCheckIfIsWithin)) {

				RVPDP_LOG(DebugSettings, FColor::Red, "Within Area Check Valid Failed since one of the componentToCheckIfWithin is a Geometry Collection Component, but we don't support those types of components for use with Within Area");

				return false;
			}
#endif

#endif

		}

		else {

			RVPDP_LOG(DebugSettings, FColor::Red, "Within Area Check Valid Fail! One of the ComponentsToCheckIfIsWithin is Not Valid!");

			return false;
		}
	}

	return true;
}

void UVertexPaintDetectionComponent::PaintOnMeshWithinAreaTaskFinished(const FRVPDPTaskResults& TaskResult, const FRVPDPPaintTaskResultInfo& PaintTaskResult, const FRVPDPPaintWithinAreaSettings& PaintWithinAreaSettings, const FRVPDPWithinAreaResults& WithinAreaResults, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough, bool FailedAtCheckValid) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_PaintOnMeshWithinAreaTaskFinished);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - PaintOnMeshWithinAreaTaskFinished");


	if (CurrentPaintWithinAreaTasks.Contains(TaskResult.TaskID))
		CurrentPaintWithinAreaTasks.Remove(TaskResult.TaskID);

	if (TaskFundamentalSettings.CallbackSettings.RunCallbackDelegate) {

		VertexColorsPaintedMeshWithinAreaDelegate.Broadcast(TaskResult, PaintTaskResult, PaintWithinAreaSettings, WithinAreaResults, AdditionalDataToPassThrough);
	}


	// If failed at fundamental checks here then makes sure we run callback interfaces from here as well as the game instance won't get a callback that a task has finished. 
	if (FailedAtCheckValid) {

		UVertexPaintFunctionLibrary::RunPaintWithinAreaCallbackInterfaces(TaskResult, PaintTaskResult, PaintWithinAreaSettings, WithinAreaResults, TaskFundamentalSettings, AdditionalDataToPassThrough);
		UVertexPaintFunctionLibrary::RunFinishedPaintTaskCallbacks(TaskResult, PaintTaskResult, TaskFundamentalSettings, AdditionalDataToPassThrough);
	}
}

TArray<FRVPDPComponentToCheckIfIsWithinAreaInfo> UVertexPaintDetectionComponent::GetWithinAreaComponentsToCheckIfIsWithinSettings(UPrimitiveComponent* MeshComponent, FRVPDPWithinAreaSettings WithinAreaSettings) const {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetWithinAreaComponentsToCheckIfIsWithinSettings);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - GetWithinAreaComponentsToCheckIfIsWithinSettings");


	TArray<FRVPDPComponentToCheckIfIsWithinAreaInfo> componentsToCheckIfWithin;

	if (!IsValid(MeshComponent)) return componentsToCheckIfWithin;
	if (WithinAreaSettings.ComponentsToCheckIfIsWithin.Num() == 0) return componentsToCheckIfWithin;


	FTransform origoTransform = FTransform();
	origoTransform.SetIdentity(); 


	// Within Area - We can cache all the things we need such as transform, forward vector etc. so we know where the component was and it's necessary values When the task was created, so in case there is a high task queue and it won't be run until a few seconds later, it will still be calculated from the same place as created. 

	for (FRVPDPComponentToCheckIfIsWithinAreaInfo compsToCheckWithinAreaInfo : WithinAreaSettings.ComponentsToCheckIfIsWithin) {

		if (!IsValid(compsToCheckWithinAreaInfo.ComponentToCheckIfIsWithin)) continue;


		compsToCheckWithinAreaInfo.ComponentToCheckIfIsWithinWeak = compsToCheckWithinAreaInfo.ComponentToCheckIfIsWithin;
		compsToCheckWithinAreaInfo.FallOffSettings.ColorLimitAtFallOffEdge = UKismetMathLibrary::FClamp(compsToCheckWithinAreaInfo.FallOffSettings.ColorLimitAtFallOffEdge, 0, 1);


		UPrimitiveComponent* componentToCheckIfWithin = compsToCheckWithinAreaInfo.ComponentToCheckIfIsWithin;
		compsToCheckWithinAreaInfo.ComponentForwardVector = componentToCheckIfWithin->GetForwardVector();
		compsToCheckWithinAreaInfo.ComponentRightVector = componentToCheckIfWithin->GetRightVector();
		compsToCheckWithinAreaInfo.ComponentUpVector = componentToCheckIfWithin->GetUpVector();

		compsToCheckWithinAreaInfo.ComponentBounds = componentToCheckIfWithin->Bounds;
		componentToCheckIfWithin->GetOwner()->GetActorBounds(false, compsToCheckWithinAreaInfo.ActorBounds_Origin, compsToCheckWithinAreaInfo.ActorBounds_Extent, true);

		// Caches World Transform as well and adjusts it down below so we can save Location, Rotation, Scale in mesh compoent to paint component space. Then when we actually do the Task, we transform those back into World Space, and then re-sets this to be them. So even if the task starts much later then when it was created, and things may have moved since then, we can check things relative to how it was when task was created. 
		compsToCheckWithinAreaInfo.ComponentWorldTransform = componentToCheckIfWithin->GetComponentTransform();

		const float extraExtent = compsToCheckWithinAreaInfo.ExtraExtentOutsideOfArea;


		if (UBoxComponent* boxComp = Cast<UBoxComponent>(componentToCheckIfWithin)) {

			compsToCheckWithinAreaInfo.SquareOrRectangle_XHalfSize = boxComp->GetScaledBoxExtent().X;
			compsToCheckWithinAreaInfo.SquareOrRectangle_YHalfSize = boxComp->GetScaledBoxExtent().Y;
			compsToCheckWithinAreaInfo.SquareOrRectangle_ZHalfSize = boxComp->GetScaledBoxExtent().Z;

			compsToCheckWithinAreaInfo.SquareOrRectangle_XHalfSize += extraExtent;
			compsToCheckWithinAreaInfo.SquareOrRectangle_YHalfSize += extraExtent;
			compsToCheckWithinAreaInfo.SquareOrRectangle_ZHalfSize += extraExtent;
		}

		else if (USphereComponent* sphereComp = Cast<USphereComponent>(componentToCheckIfWithin)) {

			compsToCheckWithinAreaInfo.Sphere_Radius = sphereComp->GetScaledSphereRadius();

			compsToCheckWithinAreaInfo.Sphere_Radius += extraExtent;
		}

		else if (UCapsuleComponent* capsuleComp = Cast<UCapsuleComponent>(componentToCheckIfWithin)) {

			// Unscaled so when we convert to local vertex positions when calculating the tasks it will work even if the capsule is scaled differently
			float radius = capsuleComp->GetUnscaledCapsuleRadius();
			float halfHeight = capsuleComp->GetUnscaledCapsuleHalfHeight();
			const FVector upDirection = capsuleComp->GetComponentQuat().GetAxisZ();
			const FVector capsuleLocation = capsuleComp->GetComponentLocation();

			radius += extraExtent;
			halfHeight += extraExtent;

			compsToCheckWithinAreaInfo.Capsule_Radius = radius;
			compsToCheckWithinAreaInfo.Capsule_HalfHeight = halfHeight;
			compsToCheckWithinAreaInfo.Capsule_Orientation = capsuleComp->GetComponentQuat();

			// - radius so the top and bottom parts are where the spherical shape starts at the end of the capsule. 
			compsToCheckWithinAreaInfo.Capsule_TopPoint = capsuleLocation + upDirection * (halfHeight - radius); 
			compsToCheckWithinAreaInfo.Capsule_BottomPoint = capsuleLocation - upDirection * (halfHeight - radius);
		}

		else if (UStaticMeshComponent* staticMeshComp = Cast<UStaticMeshComponent>(componentToCheckIfWithin)) {

			// Since we can get Get Location on Spline Mesh Components we have to use their Box Get Center so we could still use cached World_Transform for somethings
			if (Cast<USplineMeshComponent>(componentToCheckIfWithin)) {

				compsToCheckWithinAreaInfo.ComponentWorldTransform.SetLocation(compsToCheckWithinAreaInfo.ComponentBounds.GetBox().GetCenter());

				// Gets the AggGeom Bounds so we can get the thickness with any added box collisions the mesh may have. 
				FBoxSphereBounds bounds;
				componentToCheckIfWithin->GetBodySetup()->AggGeom.CalcBoxSphereBounds(bounds, compsToCheckWithinAreaInfo.ComponentWorldTransform);

				// Updated the extent thickness so we have the proper one with any added collisions
				compsToCheckWithinAreaInfo.ComponentBounds.BoxExtent.Z = bounds.BoxExtent.Z;

				// Spline Mesh should be complex shape and doesn't support Extra Extent, so don't bother doing anything with it here. Leaves this here though as a reminder in case we can figure out a is vertex within area thing on spline meshes using just math without any traces, i.e. it doesn't have to require complex shape 
				// if (calculateColorsInfo.PaintWithinAreaSettings.Actor)
					// compsToCheckIfWithin.ComponentBounds.BoxExtent.Z += calculateColorsInfo.PaintWithinAreaSettings.ExtraExtentToApplyPaintOn;

				FVector adjustedLocationWithMeshBoxOffsetTakenIntoAccount = componentToCheckIfWithin->Bounds.GetBox().GetCenter();

				// To get the extent from the correct center Location we take added collisions into account as well so the bounds match
				if (staticMeshComp->GetStaticMesh()->GetBodySetup()) {

					if (staticMeshComp->GetStaticMesh()->GetBodySetup()->AggGeom.GetElementCount() > 0) {

						float differenceFromMeshLocationAndBoxCollisionZ = 0;


						for (int j = 0; j < staticMeshComp->GetStaticMesh()->GetBodySetup()->AggGeom.BoxElems.Num(); j++)
							differenceFromMeshLocationAndBoxCollisionZ += staticMeshComp->GetStaticMesh()->GetBodySetup()->AggGeom.BoxElems[j].Center.Z;

						for (int j = 0; j < componentToCheckIfWithin->GetBodySetup()->AggGeom.ConvexElems.Num(); j++)
							differenceFromMeshLocationAndBoxCollisionZ += componentToCheckIfWithin->GetBodySetup()->AggGeom.ConvexElems[j].GetTransform().GetLocation().Z;

						for (int j = 0; j < componentToCheckIfWithin->GetBodySetup()->AggGeom.SphereElems.Num(); j++)
							differenceFromMeshLocationAndBoxCollisionZ += componentToCheckIfWithin->GetBodySetup()->AggGeom.SphereElems[j].Center.Z;

						for (int j = 0; j < componentToCheckIfWithin->GetBodySetup()->AggGeom.SphylElems.Num(); j++)
							differenceFromMeshLocationAndBoxCollisionZ += componentToCheckIfWithin->GetBodySetup()->AggGeom.SphylElems[j].Center.Z;

						for (int j = 0; j < componentToCheckIfWithin->GetBodySetup()->AggGeom.TaperedCapsuleElems.Num(); j++)
							differenceFromMeshLocationAndBoxCollisionZ += componentToCheckIfWithin->GetBodySetup()->AggGeom.TaperedCapsuleElems[j].Center.Z;


						differenceFromMeshLocationAndBoxCollisionZ /= staticMeshComp->GetStaticMesh()->GetBodySetup()->AggGeom.GetElementCount();
						adjustedLocationWithMeshBoxOffsetTakenIntoAccount.Z += differenceFromMeshLocationAndBoxCollisionZ;

						// Sets World Transform Location to be the adjusted Location so it's at the Real Center
						compsToCheckWithinAreaInfo.ComponentWorldTransform.SetLocation(adjustedLocationWithMeshBoxOffsetTakenIntoAccount);
					}
				}
			}

			else {

				// Calc Box Sphere Bounds Without taking the Rotation into account, this is because if for instance a rectangle is rotated, then that affects the bounds and the result we would get here would be the correct extents of the mesh and it's collision, and it is the Extent that is what we're after
				FTransform transformWithoutRotation;
				transformWithoutRotation.SetLocation(compsToCheckWithinAreaInfo.ComponentWorldTransform.GetLocation());
				transformWithoutRotation.SetScale3D(FVector(1, 1, 1));
				const FVector componentScale = compsToCheckWithinAreaInfo.ComponentWorldTransform.GetScale3D();

				// Always runs this so we have proper Bounds no matter what paint within area type, so even if complex shape on static mesh component etc. 
				componentToCheckIfWithin->GetBodySetup()->AggGeom.CalcBoxSphereBounds(compsToCheckWithinAreaInfo.CleanAggGeomBounds, transformWithoutRotation);
				const auto cleanBoundsBox = compsToCheckWithinAreaInfo.CleanAggGeomBounds.GetBox();

				// Also gets the Bounds but with Actual Rotation and Scale taken into account
				transformWithoutRotation.SetScale3D(componentScale);
				transformWithoutRotation.SetRotation(compsToCheckWithinAreaInfo.ComponentWorldTransform.GetRotation());
				componentToCheckIfWithin->GetBodySetup()->AggGeom.CalcBoxSphereBounds(compsToCheckWithinAreaInfo.ComponentBounds, transformWithoutRotation);



				// If a Cube / Rectangle then checks if it has box collisions in it's static mesh, if so then we have to adjust the world transform so that it's Location is in the center between the Actor and the collision. For instance if you have a floor, with added box collision underneath it that stretches 1M, without this we would get the bounds offset since the Location of the Actor is at the floor height. But with this we can get the actual center Location of the Bounds since it takes into account the box collision
				if (compsToCheckWithinAreaInfo.PaintWithinAreaShape == EPaintWithinAreaShape::IsSquareOrRectangleShape) {

					// To get the extent from the correct center Location that takes added box collision into account we had to do this. 
					if (componentToCheckIfWithin->GetBodySetup()) {


						FVector adjustedLocationWithMeshBoxOffsetTakenIntoAccount = componentToCheckIfWithin->Bounds.GetBox().GetCenter();
						float differenceFromMeshLocationAndBoxCollisionZ = 0;


						if (componentToCheckIfWithin->GetBodySetup()->AggGeom.BoxElems.Num() > 0) {

							// Sets them to a very high values so we will always set the min and max values down below. If they where 0 we would've had an issue where if for instance there where two boxes at -50 and -100, then none would be higher then maxValue 0 and -50 wouldn't get set to be the maxValue as it should. 
							float maxZValue = -10000000;
							float minZValue = 10000000;

							for (int j = 0; j < componentToCheckIfWithin->GetBodySetup()->AggGeom.BoxElems.Num(); j++) {

								const auto aggGeomBoxElem = componentToCheckIfWithin->GetBodySetup()->AggGeom.BoxElems[j];

								// Gets the current min and max values based on their position and their extent
								float currentBoxMaxZ = aggGeomBoxElem.Center.Z + (aggGeomBoxElem.Z / 2);
								float currentBoxMinZ = aggGeomBoxElem.Center.Z - (aggGeomBoxElem.Z / 2);


								// Caches the the Highest and Lowest Z, so we can get the correct center point of them for the bounds
								if (currentBoxMaxZ > maxZValue)
									maxZValue = currentBoxMaxZ;

								if (currentBoxMinZ < minZValue)
									minZValue = currentBoxMinZ;
							}

							// The Middle point between the max and min values
							differenceFromMeshLocationAndBoxCollisionZ = (maxZValue + minZValue) / 2;
						}

						adjustedLocationWithMeshBoxOffsetTakenIntoAccount.Z += differenceFromMeshLocationAndBoxCollisionZ;


						// Sets World Transform Location to be the adjusted Location so it's at the Real Center
						compsToCheckWithinAreaInfo.ComponentWorldTransform.SetLocation(adjustedLocationWithMeshBoxOffsetTakenIntoAccount);

						// If there's more than 1 Box, and they're not centered on top of eachother then it seizes to be a Square or Rectangle, so throws a Error warning about that if there's more than 1. Should be a pretty rare case though. 
						/*
						if (compsToCheckIfWithin.ComponentToCheckIfIsWithin->GetBodySetup()->AggGeom.BoxElems.Num() > 1) {

							RVPDP_LOG(colorSettings.DebugSettings, FColor::Orange, "Paint Within Area with Square or Rectangle for Static Mesh Component. NOTE That the Static Mesh has more than 1 Box Collision under it. If they're not centered then then it's not a Square or Rectangle anymore and the Result may not be the expected one. ");
						}
						*/
					}

					compsToCheckWithinAreaInfo.SquareOrRectangle_XHalfSize = cleanBoundsBox.GetExtent().X * componentScale.X;
					compsToCheckWithinAreaInfo.SquareOrRectangle_YHalfSize = cleanBoundsBox.GetExtent().Y * componentScale.Y;
					compsToCheckWithinAreaInfo.SquareOrRectangle_ZHalfSize = cleanBoundsBox.GetExtent().Z * componentScale.Z;

					compsToCheckWithinAreaInfo.SquareOrRectangle_XHalfSize += extraExtent;
					compsToCheckWithinAreaInfo.SquareOrRectangle_YHalfSize += extraExtent;
					compsToCheckWithinAreaInfo.SquareOrRectangle_ZHalfSize += extraExtent;
				}

				// Uses AggGeom Bounds so we get the correct bounds even if the mesh is 1, 1, 1 in scale but much larger than 50cm radius
				else if (compsToCheckWithinAreaInfo.PaintWithinAreaShape == EPaintWithinAreaShape::IsSphereShape) {

					compsToCheckWithinAreaInfo.Sphere_Radius = (compsToCheckWithinAreaInfo.CleanAggGeomBounds.SphereRadius * componentScale.X);

					compsToCheckWithinAreaInfo.Sphere_Radius += extraExtent;
				}

				else if (compsToCheckWithinAreaInfo.PaintWithinAreaShape == EPaintWithinAreaShape::IsCapsuleShape) {

					float radius = cleanBoundsBox.GetExtent().X;
					float halfHeight = cleanBoundsBox.GetExtent().Z;
					const FVector upDirection = staticMeshComp->GetComponentQuat().GetAxisZ();
					const FVector capsuleLocation = staticMeshComp->GetComponentLocation();

					radius += extraExtent;
					halfHeight += extraExtent;

					compsToCheckWithinAreaInfo.Capsule_Radius = radius;
					compsToCheckWithinAreaInfo.Capsule_HalfHeight = halfHeight;
					compsToCheckWithinAreaInfo.Capsule_Orientation = staticMeshComp->GetComponentQuat();
					compsToCheckWithinAreaInfo.Capsule_TopPoint = capsuleLocation + upDirection * (halfHeight - radius);
					compsToCheckWithinAreaInfo.Capsule_BottomPoint = capsuleLocation - upDirection * (halfHeight - radius);
				}

				else if (compsToCheckWithinAreaInfo.PaintWithinAreaShape == EPaintWithinAreaShape::IsConeShape) {


					const FVector coneOrigin = componentToCheckIfWithin->GetComponentLocation();
					const FVector coneDirection = componentToCheckIfWithin->GetUpVector() * -1;
					float coneRadius = FMath::Max(cleanBoundsBox.GetExtent().Y, cleanBoundsBox.GetExtent().X) * componentScale.X;
					float coneHeight = (cleanBoundsBox.GetExtent().Z * 2.0f) * componentScale.Z;

					coneRadius += extraExtent;
					coneHeight += extraExtent;


					// Sets these after applying Extra Extent to what they use
					const FVector bottomPosition = coneOrigin + (coneHeight * coneDirection);
					const float coneSlantHeight = FMath::Sqrt(FMath::Square(coneRadius) + FMath::Square(coneHeight)); // Height to the bottom edges of the cone
					const float angleInDegrees = FMath::RadiansToDegrees(FMath::Atan(coneRadius / coneHeight));


					compsToCheckWithinAreaInfo.Cone_Origin = coneOrigin;
					compsToCheckWithinAreaInfo.Cone_Direction = coneDirection;
					compsToCheckWithinAreaInfo.Cone_Radius = coneRadius;
					compsToCheckWithinAreaInfo.Cone_AngleInDegrees = angleInDegrees;
					compsToCheckWithinAreaInfo.Cone_Height = coneHeight;
					compsToCheckWithinAreaInfo.Cone_SlantHeight = coneSlantHeight;

					compsToCheckWithinAreaInfo.Cone_CenterBottomTransform = compsToCheckWithinAreaInfo.ComponentWorldTransform;
					compsToCheckWithinAreaInfo.Cone_CenterBottomTransform.SetLocation(bottomPosition);

					compsToCheckWithinAreaInfo.Cone_CenterTransform = compsToCheckWithinAreaInfo.ComponentWorldTransform;
					compsToCheckWithinAreaInfo.Cone_CenterTransform.SetLocation(componentToCheckIfWithin->Bounds.Origin);
				}
			}
		}

		else if (USkeletalMeshComponent* skeletalMeshComp = Cast<USkeletalMeshComponent>(componentToCheckIfWithin)) {


			USkeletalMesh* paintWithinAreaSkelMesh = nullptr;

#if ENGINE_MAJOR_VERSION == 4

			paintWithinAreaSkelMesh = skeletalMeshComp->SkeletalMesh;

#elif ENGINE_MAJOR_VERSION == 5

#if ENGINE_MINOR_VERSION == 0

			paintWithinAreaSkelMesh = skeletalMeshComp->SkeletalMesh.Get();

#else

			paintWithinAreaSkelMesh = skeletalMeshComp->GetSkeletalMeshAsset();

#endif

#endif

			// The Engines own CalcAABB has a requirement that the scale has to be uniform for some reason, so it doesn't work if for instance the characters scale is 1, 1, 0.5. So we only run it if uniform, otherwise we fall back to a custom one where i've copied over the exact same logic but commented out the uniform check, which works as intended despite the scale not being uniform..
			if (skeletalMeshComp->GetComponentTransform().GetScale3D().IsUniform()) {

				compsToCheckWithinAreaInfo.ComponentBounds = paintWithinAreaSkelMesh->GetPhysicsAsset()->CalcAABB(skeletalMeshComp, skeletalMeshComp->GetComponentTransform());
			}

			else {

				compsToCheckWithinAreaInfo.ComponentBounds = UVertexPaintFunctionLibrary::CalcAABBWithoutUniformCheck(skeletalMeshComp, skeletalMeshComp->GetComponentTransform());
			}

			// Note Skeletal Mesh should be complex shape and doesn't support Extra Extent, so doesn't bother doing anything with it here. 

			FVector location = compsToCheckWithinAreaInfo.ComponentWorldTransform.GetLocation();

			// Sets the Z to be the center of the bounds, since its common for the skel mesh to be at the bottom
			location.Z = compsToCheckWithinAreaInfo.ComponentBounds.GetBox().GetCenter().Z;
			compsToCheckWithinAreaInfo.ComponentWorldTransform.SetLocation(location);
		}

		else if (Cast<ALandscape>(componentToCheckIfWithin->GetOwner())) {

			// Landscape require complex shape which doesn't support extra extent so doesn't bother setting anything of it here
		}

		else {

			continue;
		}



		// Stores the Relative Location, Rotation and Scale so we can transform it back when the task actually starts, so we can do calculations based on where the component to check if within actually was when task got created. 
		const FVector compsToCheckIfWithinRelativeLocation = UKismetMathLibrary::InverseTransformLocation(origoTransform, componentToCheckIfWithin->GetComponentLocation());
		compsToCheckWithinAreaInfo.ComponentRelativeLocationToMeshBeingPainted = compsToCheckIfWithinRelativeLocation;

		const FRotator compsToCheckIfWithinRelativeRotation = UKismetMathLibrary::InverseTransformRotation(origoTransform, componentToCheckIfWithin->GetComponentRotation());
		compsToCheckWithinAreaInfo.ComponentRelativeRotationToMeshBeingPainted = compsToCheckIfWithinRelativeRotation;

		const FVector compsToCheckIfWithinRelativeScale = UKismetMathLibrary::InverseTransformLocation(origoTransform, componentToCheckIfWithin->GetComponentScale());
		compsToCheckWithinAreaInfo.ComponentRelativeScaleToMeshBeingPainted = compsToCheckIfWithinRelativeScale;


		FRVPDPPaintWithinAreaFallOffSettings componentToCheckIfWithinFalloffSettings = compsToCheckWithinAreaInfo.FallOffSettings;

		switch (compsToCheckWithinAreaInfo.PaintWithinAreaShape) {


			case EPaintWithinAreaShape::IsSquareOrRectangleShape: {


				// Caching the local positions we use later when calculating if a vertex is within area

				// Forward
				const FVector forwardPosWorld = compsToCheckWithinAreaInfo.ComponentWorldTransform.GetLocation() + compsToCheckWithinAreaInfo.ComponentForwardVector * compsToCheckWithinAreaInfo.SquareOrRectangle_XHalfSize;
				compsToCheckWithinAreaInfo.SquareOrRectangle_ForwardPosLocal = UKismetMathLibrary::InverseTransformLocation(compsToCheckWithinAreaInfo.ComponentWorldTransform, forwardPosWorld);

				// Back
				const FVector backPosWorld = compsToCheckWithinAreaInfo.ComponentWorldTransform.GetLocation() + (compsToCheckWithinAreaInfo.ComponentForwardVector * -1) * compsToCheckWithinAreaInfo.SquareOrRectangle_XHalfSize;
				compsToCheckWithinAreaInfo.SquareOrRectangle_BackPosLocal = UKismetMathLibrary::InverseTransformLocation(compsToCheckWithinAreaInfo.ComponentWorldTransform, backPosWorld);

				// Left
				const FVector leftPosWorld = compsToCheckWithinAreaInfo.ComponentWorldTransform.GetLocation() + (compsToCheckWithinAreaInfo.ComponentRightVector * -1) * compsToCheckWithinAreaInfo.SquareOrRectangle_YHalfSize;
				compsToCheckWithinAreaInfo.SquareOrRectangle_LeftPosLocal = UKismetMathLibrary::InverseTransformLocation(compsToCheckWithinAreaInfo.ComponentWorldTransform, leftPosWorld);

				// Right
				const FVector rightPosWorld = compsToCheckWithinAreaInfo.ComponentWorldTransform.GetLocation() + compsToCheckWithinAreaInfo.ComponentRightVector * compsToCheckWithinAreaInfo.SquareOrRectangle_YHalfSize;
				compsToCheckWithinAreaInfo.SquareOrRectangle_RightPosLocal = UKismetMathLibrary::InverseTransformLocation(compsToCheckWithinAreaInfo.ComponentWorldTransform, rightPosWorld);


				// Down
				const FVector downPosWorld = compsToCheckWithinAreaInfo.ComponentWorldTransform.GetLocation() + (compsToCheckWithinAreaInfo.ComponentUpVector * -1) * compsToCheckWithinAreaInfo.SquareOrRectangle_ZHalfSize;
				compsToCheckWithinAreaInfo.SquareOrRectangle_DownPosLocal = UKismetMathLibrary::InverseTransformLocation(compsToCheckWithinAreaInfo.ComponentWorldTransform, downPosWorld);

				// Up
				const FVector upPosWorld = compsToCheckWithinAreaInfo.ComponentWorldTransform.GetLocation() + compsToCheckWithinAreaInfo.ComponentUpVector * compsToCheckWithinAreaInfo.SquareOrRectangle_ZHalfSize;
				compsToCheckWithinAreaInfo.SquareOrRectangle_UpPosLocal = UKismetMathLibrary::InverseTransformLocation(compsToCheckWithinAreaInfo.ComponentWorldTransform, upPosWorld);


				if (componentToCheckIfWithinFalloffSettings.FallOffStrength > 0) {


					if (componentToCheckIfWithinFalloffSettings.StartFallOffDistanceFrom == FRVPDPStartFallOffDistanceFromSetting::BaseOfPaintShape) {

						compsToCheckWithinAreaInfo.FallOff_ScaleFrom = componentToCheckIfWithinFalloffSettings.DistanceToStartFallOffFrom;
					}

					// Substracts distance from the very Edge of the area, to where the user wants to base the fall off from. So you can have it start to fade always for instance 20cm from the edge.  
					else if (componentToCheckIfWithinFalloffSettings.StartFallOffDistanceFrom == FRVPDPStartFallOffDistanceFromSetting::EndOfPaintShape) {

						compsToCheckWithinAreaInfo.FallOff_ScaleFrom = compsToCheckWithinAreaInfo.SquareOrRectangle_ZHalfSize * 2;
						compsToCheckWithinAreaInfo.FallOff_ScaleFrom -= componentToCheckIfWithinFalloffSettings.DistanceToStartFallOffFrom;
					}


					// AoE is used for falloff calculation when paint within area
					if (componentToCheckIfWithinFalloffSettings.PaintWithinAreaFallOffType == EVertexPaintWithinAreaFallOffType::SphericalFromCenter) {

						compsToCheckWithinAreaInfo.FallOff_WithinAreaOfEffect = FVector(compsToCheckWithinAreaInfo.SquareOrRectangle_XHalfSize, compsToCheckWithinAreaInfo.SquareOrRectangle_YHalfSize, compsToCheckWithinAreaInfo.SquareOrRectangle_ZHalfSize).Size();
					}

					// For Gradiants that go across the entire area (unlike falloff type spherical that go from center outwards) we *2 so it covers it all. 
					else if (componentToCheckIfWithinFalloffSettings.PaintWithinAreaFallOffType == EVertexPaintWithinAreaFallOffType::GradiantUpward || componentToCheckIfWithinFalloffSettings.PaintWithinAreaFallOffType == EVertexPaintWithinAreaFallOffType::GradiantDownward) {

						compsToCheckWithinAreaInfo.FallOff_WithinAreaOfEffect = compsToCheckWithinAreaInfo.SquareOrRectangle_ZHalfSize * 2;
					}

					// For inverse gradiant that go from middle outward it should only be half size. 
					else if (componentToCheckIfWithinFalloffSettings.PaintWithinAreaFallOffType == EVertexPaintWithinAreaFallOffType::InverseGradiant) {

						compsToCheckWithinAreaInfo.FallOff_WithinAreaOfEffect = compsToCheckWithinAreaInfo.SquareOrRectangle_ZHalfSize;
					}


					// Want the fall off scale be from the pure range not whatever is added to extra extent, otherwise the user has to remember to counter that extra extent with DistanceToStartFallOffFrom for it to make a difference
					compsToCheckWithinAreaInfo.FallOff_ScaleFrom -= extraExtent;
				}


				break;
			}


			case EPaintWithinAreaShape::IsSphereShape: {


				if (componentToCheckIfWithinFalloffSettings.FallOffStrength > 0) {


					if (componentToCheckIfWithinFalloffSettings.StartFallOffDistanceFrom == FRVPDPStartFallOffDistanceFromSetting::BaseOfPaintShape) {

						compsToCheckWithinAreaInfo.FallOff_ScaleFrom = componentToCheckIfWithinFalloffSettings.DistanceToStartFallOffFrom;
					}

					else if (componentToCheckIfWithinFalloffSettings.StartFallOffDistanceFrom == FRVPDPStartFallOffDistanceFromSetting::EndOfPaintShape) {

						compsToCheckWithinAreaInfo.FallOff_ScaleFrom = compsToCheckWithinAreaInfo.Sphere_Radius * 2;
						compsToCheckWithinAreaInfo.FallOff_ScaleFrom -= componentToCheckIfWithinFalloffSettings.DistanceToStartFallOffFrom;
					}


					if (componentToCheckIfWithinFalloffSettings.PaintWithinAreaFallOffType == EVertexPaintWithinAreaFallOffType::SphericalFromCenter) {

						compsToCheckWithinAreaInfo.FallOff_WithinAreaOfEffect = compsToCheckWithinAreaInfo.Sphere_Radius;
					}

					// *2 so it covers from bottom all the way to the top, not just the radie
					else if (componentToCheckIfWithinFalloffSettings.PaintWithinAreaFallOffType == EVertexPaintWithinAreaFallOffType::GradiantUpward || componentToCheckIfWithinFalloffSettings.PaintWithinAreaFallOffType == EVertexPaintWithinAreaFallOffType::GradiantDownward) {

						compsToCheckWithinAreaInfo.FallOff_WithinAreaOfEffect = compsToCheckWithinAreaInfo.Sphere_Radius * 2;
					}

					// For inverse gradiant that go from middle outward it should only be radie
					else if (componentToCheckIfWithinFalloffSettings.PaintWithinAreaFallOffType == EVertexPaintWithinAreaFallOffType::InverseGradiant) {

						compsToCheckWithinAreaInfo.FallOff_WithinAreaOfEffect = compsToCheckWithinAreaInfo.Sphere_Radius;
					}


					compsToCheckWithinAreaInfo.FallOff_ScaleFrom -= extraExtent;
				}


				break;
			}


			case EPaintWithinAreaShape::IsComplexShape: {


				if (componentToCheckIfWithinFalloffSettings.FallOffStrength > 0) {

					if (componentToCheckIfWithinFalloffSettings.StartFallOffDistanceFrom == FRVPDPStartFallOffDistanceFromSetting::BaseOfPaintShape) {

						compsToCheckWithinAreaInfo.FallOff_ScaleFrom = componentToCheckIfWithinFalloffSettings.DistanceToStartFallOffFrom;
					}

					else if (componentToCheckIfWithinFalloffSettings.StartFallOffDistanceFrom == FRVPDPStartFallOffDistanceFromSetting::EndOfPaintShape) {

						compsToCheckWithinAreaInfo.FallOff_ScaleFrom = compsToCheckWithinAreaInfo.ActorBounds_Extent.Size();
						compsToCheckWithinAreaInfo.FallOff_ScaleFrom -= componentToCheckIfWithinFalloffSettings.DistanceToStartFallOffFrom;
					}


					if (Cast<USkeletalMeshComponent>(componentToCheckIfWithin)) {

						if (componentToCheckIfWithinFalloffSettings.PaintWithinAreaFallOffType == EVertexPaintWithinAreaFallOffType::SphericalFromCenter) {

							compsToCheckWithinAreaInfo.FallOff_WithinAreaOfEffect = (compsToCheckWithinAreaInfo.ComponentBounds.GetBox().GetExtent() * componentToCheckIfWithin->GetComponentScale()).Size();
						}

						else if (componentToCheckIfWithinFalloffSettings.PaintWithinAreaFallOffType == EVertexPaintWithinAreaFallOffType::GradiantUpward || componentToCheckIfWithinFalloffSettings.PaintWithinAreaFallOffType == EVertexPaintWithinAreaFallOffType::GradiantDownward) {

							compsToCheckWithinAreaInfo.FallOff_WithinAreaOfEffect = (compsToCheckWithinAreaInfo.ComponentBounds.GetBox().GetExtent().Z * 2) * componentToCheckIfWithin->GetComponentScale().Z;
						}

						else if (componentToCheckIfWithinFalloffSettings.PaintWithinAreaFallOffType == EVertexPaintWithinAreaFallOffType::InverseGradiant) {

							compsToCheckWithinAreaInfo.FallOff_WithinAreaOfEffect = (compsToCheckWithinAreaInfo.ComponentBounds.GetBox().GetExtent().Z) * componentToCheckIfWithin->GetComponentScale().Z;
						}
					}

					else if (Cast<USplineMeshComponent>(componentToCheckIfWithin)) {

						// Can't calculate AoE based off the componentToCheckIfIsWithin_Bounds since they only cover each indvidual part. ActorBounds_Extent covers all of them 
						if (componentToCheckIfWithinFalloffSettings.PaintWithinAreaFallOffType == EVertexPaintWithinAreaFallOffType::SphericalFromCenter) {

							compsToCheckWithinAreaInfo.FallOff_WithinAreaOfEffect = compsToCheckWithinAreaInfo.ActorBounds_Extent.Size();
						}

						else if (componentToCheckIfWithinFalloffSettings.PaintWithinAreaFallOffType == EVertexPaintWithinAreaFallOffType::GradiantUpward || componentToCheckIfWithinFalloffSettings.PaintWithinAreaFallOffType == EVertexPaintWithinAreaFallOffType::GradiantDownward) {

							compsToCheckWithinAreaInfo.FallOff_WithinAreaOfEffect = compsToCheckWithinAreaInfo.ActorBounds_Extent.Size() * 2;
						}

						else if (componentToCheckIfWithinFalloffSettings.PaintWithinAreaFallOffType == EVertexPaintWithinAreaFallOffType::InverseGradiant) {

							compsToCheckWithinAreaInfo.FallOff_WithinAreaOfEffect = compsToCheckWithinAreaInfo.ActorBounds_Extent.Size() * 2;
						}
					}

					else if (Cast<UStaticMeshComponent>(componentToCheckIfWithin)) {

						// Uses CleanAggGeomBounds when static mesh and complex since the shape can be Any shape
						if (componentToCheckIfWithinFalloffSettings.PaintWithinAreaFallOffType == EVertexPaintWithinAreaFallOffType::SphericalFromCenter) {

							compsToCheckWithinAreaInfo.FallOff_WithinAreaOfEffect = (compsToCheckWithinAreaInfo.CleanAggGeomBounds.GetBox().GetExtent() * compsToCheckWithinAreaInfo.ComponentWorldTransform.GetScale3D()).Size();
						}

						else if (componentToCheckIfWithinFalloffSettings.PaintWithinAreaFallOffType == EVertexPaintWithinAreaFallOffType::GradiantUpward || componentToCheckIfWithinFalloffSettings.PaintWithinAreaFallOffType == EVertexPaintWithinAreaFallOffType::GradiantDownward) {

							compsToCheckWithinAreaInfo.FallOff_WithinAreaOfEffect = (compsToCheckWithinAreaInfo.CleanAggGeomBounds.GetBox().GetExtent().Z * 2) * compsToCheckWithinAreaInfo.ComponentWorldTransform.GetScale3D().Z;
						}

						else if (componentToCheckIfWithinFalloffSettings.PaintWithinAreaFallOffType == EVertexPaintWithinAreaFallOffType::InverseGradiant) {

							compsToCheckWithinAreaInfo.FallOff_WithinAreaOfEffect = (compsToCheckWithinAreaInfo.CleanAggGeomBounds.GetBox().GetExtent().Z) * compsToCheckWithinAreaInfo.ComponentWorldTransform.GetScale3D().Z;
						}
					}


					compsToCheckWithinAreaInfo.FallOff_ScaleFrom -= extraExtent;
				}


				break;
			}


			case EPaintWithinAreaShape::IsCapsuleShape: {


				compsToCheckWithinAreaInfo.Capsule_TopPoint_Local = UKismetMathLibrary::InverseTransformLocation(compsToCheckWithinAreaInfo.ComponentWorldTransform, compsToCheckWithinAreaInfo.Capsule_TopPoint);
				compsToCheckWithinAreaInfo.Capsule_BottomPoint_Local = UKismetMathLibrary::InverseTransformLocation(compsToCheckWithinAreaInfo.ComponentWorldTransform, compsToCheckWithinAreaInfo.Capsule_BottomPoint);


				if (componentToCheckIfWithinFalloffSettings.FallOffStrength > 0) {


					if (componentToCheckIfWithinFalloffSettings.StartFallOffDistanceFrom == FRVPDPStartFallOffDistanceFromSetting::BaseOfPaintShape) {

						compsToCheckWithinAreaInfo.FallOff_ScaleFrom = componentToCheckIfWithinFalloffSettings.DistanceToStartFallOffFrom;
					}

					else if (componentToCheckIfWithinFalloffSettings.StartFallOffDistanceFrom == FRVPDPStartFallOffDistanceFromSetting::EndOfPaintShape) {

						compsToCheckWithinAreaInfo.FallOff_ScaleFrom = compsToCheckWithinAreaInfo.Capsule_HalfHeight * 2;
						compsToCheckWithinAreaInfo.FallOff_ScaleFrom -= componentToCheckIfWithinFalloffSettings.DistanceToStartFallOffFrom;
					}


					if (componentToCheckIfWithinFalloffSettings.PaintWithinAreaFallOffType == EVertexPaintWithinAreaFallOffType::SphericalFromCenter) {

						compsToCheckWithinAreaInfo.FallOff_WithinAreaOfEffect = compsToCheckWithinAreaInfo.Capsule_HalfHeight;
					}

					else if (componentToCheckIfWithinFalloffSettings.PaintWithinAreaFallOffType == EVertexPaintWithinAreaFallOffType::GradiantUpward || componentToCheckIfWithinFalloffSettings.PaintWithinAreaFallOffType == EVertexPaintWithinAreaFallOffType::GradiantDownward) {

						compsToCheckWithinAreaInfo.FallOff_WithinAreaOfEffect = compsToCheckWithinAreaInfo.Capsule_HalfHeight * 2;
					}

					else if (componentToCheckIfWithinFalloffSettings.PaintWithinAreaFallOffType == EVertexPaintWithinAreaFallOffType::InverseGradiant) {

						compsToCheckWithinAreaInfo.FallOff_WithinAreaOfEffect = compsToCheckWithinAreaInfo.Capsule_HalfHeight;
					}


					compsToCheckWithinAreaInfo.FallOff_ScaleFrom -= extraExtent;
				}


				break;
			}


			case EPaintWithinAreaShape::IsConeShape: {


				if (componentToCheckIfWithinFalloffSettings.FallOffStrength > 0) {


					if (componentToCheckIfWithinFalloffSettings.StartFallOffDistanceFrom == FRVPDPStartFallOffDistanceFromSetting::BaseOfPaintShape) {

						compsToCheckWithinAreaInfo.FallOff_ScaleFrom = componentToCheckIfWithinFalloffSettings.DistanceToStartFallOffFrom;
					}

					else if (componentToCheckIfWithinFalloffSettings.StartFallOffDistanceFrom == FRVPDPStartFallOffDistanceFromSetting::EndOfPaintShape) {

						compsToCheckWithinAreaInfo.FallOff_ScaleFrom = compsToCheckWithinAreaInfo.Cone_SlantHeight;
						compsToCheckWithinAreaInfo.FallOff_ScaleFrom -= componentToCheckIfWithinFalloffSettings.DistanceToStartFallOffFrom;
					}


					if (componentToCheckIfWithinFalloffSettings.PaintWithinAreaFallOffType == EVertexPaintWithinAreaFallOffType::SphericalFromCenter) {

						compsToCheckWithinAreaInfo.FallOff_WithinAreaOfEffect = compsToCheckWithinAreaInfo.Cone_SlantHeight / 2;
					}

					else if (componentToCheckIfWithinFalloffSettings.PaintWithinAreaFallOffType == EVertexPaintWithinAreaFallOffType::GradiantUpward || componentToCheckIfWithinFalloffSettings.PaintWithinAreaFallOffType == EVertexPaintWithinAreaFallOffType::GradiantDownward) {

						compsToCheckWithinAreaInfo.FallOff_WithinAreaOfEffect = compsToCheckWithinAreaInfo.Cone_SlantHeight;
					}

					else if (componentToCheckIfWithinFalloffSettings.PaintWithinAreaFallOffType == EVertexPaintWithinAreaFallOffType::InverseGradiant) {

						compsToCheckWithinAreaInfo.FallOff_WithinAreaOfEffect = compsToCheckWithinAreaInfo.Cone_SlantHeight / 2;
					}


					compsToCheckWithinAreaInfo.FallOff_ScaleFrom -= extraExtent;
				}


				break;
			}

			default:
				break;
		}


		componentsToCheckIfWithin.Add(compsToCheckWithinAreaInfo);
	}


	return componentsToCheckIfWithin;
}

void UVertexPaintDetectionComponent::DrawWithinAreaDebugSymbols(TArray<FRVPDPComponentToCheckIfIsWithinAreaInfo> ComponentsToCheckIfIsWithin, float Duration) const {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_DrawWithinAreaDebugSymbols);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - DrawWithinAreaDebugSymbols");

	if (ComponentsToCheckIfIsWithin.Num() == 0) return;


	for (const FRVPDPComponentToCheckIfIsWithinAreaInfo& compsToCheckWithinAreaInfo : ComponentsToCheckIfIsWithin) {


		if (Cast<UBoxComponent>(compsToCheckWithinAreaInfo.GetComponentToCheckIfIsWithin()) || (Cast<UStaticMeshComponent>(compsToCheckWithinAreaInfo.GetComponentToCheckIfIsWithin()) && compsToCheckWithinAreaInfo.PaintWithinAreaShape == EPaintWithinAreaShape::IsSquareOrRectangleShape)) {

			FVector debugExtent = FVector(ForceInitToZero);
			debugExtent.X = compsToCheckWithinAreaInfo.SquareOrRectangle_XHalfSize;
			debugExtent.Y = compsToCheckWithinAreaInfo.SquareOrRectangle_YHalfSize;
			debugExtent.Z = compsToCheckWithinAreaInfo.SquareOrRectangle_ZHalfSize;


			// Scales the thickness depending on the size, since it was very difficult to see the debug box if the size was big and it was too thin. Min of 5 though so if the size is very small it's still easy to see up close. 
			const float maxDebugThickness = debugExtent.Size() / 1000;
			float debugThickness = UKismetMathLibrary::MapRangeClamped(debugExtent.Size(), 0, debugExtent.Size(), 5, maxDebugThickness);
			debugThickness = UKismetMathLibrary::FClamp(debugThickness, 5, maxDebugThickness);

			DrawDebugBox(GetWorld(), compsToCheckWithinAreaInfo.ComponentBounds.GetBox().GetCenter(), debugExtent, compsToCheckWithinAreaInfo.GetComponentToCheckIfIsWithin()->GetComponentRotation().Quaternion(), FColor::Red, false, Duration, 0, debugThickness);
		}

		else if (Cast<USphereComponent>(compsToCheckWithinAreaInfo.GetComponentToCheckIfIsWithin()) || (Cast<UStaticMeshComponent>(compsToCheckWithinAreaInfo.GetComponentToCheckIfIsWithin()) && compsToCheckWithinAreaInfo.PaintWithinAreaShape == EPaintWithinAreaShape::IsSphereShape)) {

			const float sphereShapeRadius = compsToCheckWithinAreaInfo.Sphere_Radius;
			const float maxDebugThickness = sphereShapeRadius / 1000;
			float debugThickness = UKismetMathLibrary::MapRangeClamped(sphereShapeRadius, 0, sphereShapeRadius, 5, maxDebugThickness);
			debugThickness = UKismetMathLibrary::FClamp(debugThickness, 5, maxDebugThickness);

			DrawDebugSphere(GetWorld(), compsToCheckWithinAreaInfo.ComponentBounds.GetBox().GetCenter(), sphereShapeRadius, 10, FColor::Red, false, Duration, 0, debugThickness);
		}

		else if (Cast<UCapsuleComponent>(compsToCheckWithinAreaInfo.GetComponentToCheckIfIsWithin()) || (Cast<UStaticMeshComponent>(compsToCheckWithinAreaInfo.GetComponentToCheckIfIsWithin()) && compsToCheckWithinAreaInfo.PaintWithinAreaShape == EPaintWithinAreaShape::IsCapsuleShape)) {

			const FVector capsuleTopPoint = compsToCheckWithinAreaInfo.Capsule_TopPoint;
			const FVector capsuleBottomPoint = compsToCheckWithinAreaInfo.Capsule_BottomPoint;
			const float capsuleShapeRadius = compsToCheckWithinAreaInfo.Capsule_Radius;
			const float capsuleShapeHeight = compsToCheckWithinAreaInfo.Capsule_HalfHeight;
			const FQuat capsuleShapeOrientation = compsToCheckWithinAreaInfo.Capsule_Orientation;
			const float maxDebugThickness = capsuleShapeRadius / 1000;
			float debugThickness = UKismetMathLibrary::MapRangeClamped(capsuleShapeRadius, 0, capsuleShapeRadius, 5, maxDebugThickness);
			debugThickness = UKismetMathLibrary::FClamp(debugThickness, 5, maxDebugThickness);

			DrawDebugBox(GetWorld(), capsuleTopPoint, FVector(5, 5, 5), FQuat::Identity, FColor::Red, false, Duration, 0, debugThickness);
			DrawDebugBox(GetWorld(), capsuleBottomPoint, FVector(5, 5, 5), FQuat::Identity, FColor::Red, false, Duration, 0, debugThickness);
			DrawDebugCapsule(GetWorld(), compsToCheckWithinAreaInfo.ComponentBounds.GetBox().GetCenter(), capsuleShapeHeight, capsuleShapeRadius, capsuleShapeOrientation, FColor::Red, false, Duration, 0, debugThickness);
		}

		else if (Cast<UStaticMeshComponent>(compsToCheckWithinAreaInfo.GetComponentToCheckIfIsWithin()) && compsToCheckWithinAreaInfo.PaintWithinAreaShape == EPaintWithinAreaShape::IsConeShape) {

			const FVector coneOrigin = compsToCheckWithinAreaInfo.Cone_Origin;
			const FVector coneDirection = compsToCheckWithinAreaInfo.Cone_Direction;
			const float coneAngleRad = FMath::DegreesToRadians(compsToCheckWithinAreaInfo.Cone_AngleInDegrees);
			const float coneSlantHeight = compsToCheckWithinAreaInfo.Cone_SlantHeight;
			const float maxDebugThickness = coneAngleRad / 1000;
			float debugThickness = UKismetMathLibrary::MapRangeClamped(coneAngleRad, 0, coneAngleRad, 5, maxDebugThickness);
			debugThickness = UKismetMathLibrary::FClamp(debugThickness, 5, maxDebugThickness);

			DrawDebugCone(GetWorld(), coneOrigin, coneDirection, coneSlantHeight, coneAngleRad, coneAngleRad, 12, FColor::Red, false, Duration, 0, debugThickness);
		}

		else if (Cast<USplineMeshComponent>(compsToCheckWithinAreaInfo.GetComponentToCheckIfIsWithin())) {

			const auto debugExtent = compsToCheckWithinAreaInfo.ComponentBounds.BoxExtent;
			const float maxDebugThickness = debugExtent.Size() / 1000;
			float debugThickness = UKismetMathLibrary::MapRangeClamped(debugExtent.Size(), 0, debugExtent.Size(), 5, maxDebugThickness);
			debugThickness = UKismetMathLibrary::FClamp(debugThickness, 5, maxDebugThickness);

			DrawDebugBox(GetWorld(), compsToCheckWithinAreaInfo.ComponentWorldTransform.GetLocation(), debugExtent * compsToCheckWithinAreaInfo.ComponentWorldTransform.GetScale3D(), compsToCheckWithinAreaInfo.GetComponentToCheckIfIsWithin()->GetComponentRotation().Quaternion(), FColor::Red, false, Duration, 0, debugThickness);
		}

		else if (Cast<ALandscape>(compsToCheckWithinAreaInfo.GetComponentToCheckIfIsWithin()->GetOwner())) {

			const auto debugExtent = compsToCheckWithinAreaInfo.ComponentBounds.BoxExtent;
			const float maxDebugThickness = debugExtent.Size() / 1000;
			float debugThickness = UKismetMathLibrary::MapRangeClamped(debugExtent.Size(), 0, debugExtent.Size(), 5, maxDebugThickness);
			debugThickness = UKismetMathLibrary::FClamp(debugThickness, 5, maxDebugThickness);

			DrawDebugBox(GetWorld(), compsToCheckWithinAreaInfo.ComponentBounds.GetBox().GetCenter(), debugExtent, compsToCheckWithinAreaInfo.GetComponentToCheckIfIsWithin()->GetComponentRotation().Quaternion(), FColor::Red, false, Duration, 0, debugThickness);
		}

		else if (compsToCheckWithinAreaInfo.PaintWithinAreaShape == EPaintWithinAreaShape::IsComplexShape) {

			const auto debugExtent = compsToCheckWithinAreaInfo.ComponentBounds.BoxExtent;
			const float maxDebugThickness = debugExtent.Size() / 1000;
			float debugThickness = UKismetMathLibrary::MapRangeClamped(debugExtent.Size(), 0, debugExtent.Size(), 5, maxDebugThickness);
			debugThickness = UKismetMathLibrary::FClamp(debugThickness, 5, maxDebugThickness);

			DrawDebugBox(GetWorld(), compsToCheckWithinAreaInfo.ComponentBounds.Origin, debugExtent, FQuat::Identity, FColor::Red, false, Duration, 0, debugThickness);
		}
	}
}

TArray<FName> UVertexPaintDetectionComponent::GetMeshBonesWithinComponentsToCheckIfWithinArea(UPrimitiveComponent* MeshComponent, const TArray<FRVPDPComponentToCheckIfIsWithinAreaInfo>& ComponentsToCheckIfWithinArea, const FRVPDPDebugSettings& DebugSettings) const {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetMeshBonesWithinComponentsToCheckIfWithinArea);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - GetMeshBonesWithinComponentsToCheckIfWithinArea");

	if (!IsValid(MeshComponent)) return TArray<FName>();
	if (!Cast<USkeletalMeshComponent>(MeshComponent)) return TArray<FName>();
	if (ComponentsToCheckIfWithinArea.Num() == 0) return TArray<FName>();


	RVPDP_LOG(DebugSettings, FColor::Cyan, "Tracing for Bones on MeshComponent to Paint/Detect Within Area");


	TArray<FName> bonesWithinArea;
	TArray<AActor*> actorsToIgnore;
	EDrawDebugTrace::Type drawDebugType = EDrawDebugTrace::None;
	const UObject* worldContext = MeshComponent;


	const bool debugTrace = DebugSettings.TaskSpecificDebugSymbols.DrawTaskDebugSymbols;
	const float debugDuration = DebugSettings.TaskSpecificDebugSymbols.DrawTaskDebugSymbolsDuration;

	if (debugTrace)
		drawDebugType = EDrawDebugTrace::ForDuration;


	TArray<TEnumAsByte<EObjectTypeQuery>> objectTypesToTraceFor;

	for (const FRVPDPComponentToCheckIfIsWithinAreaInfo& checkWithinAreaInfo : ComponentsToCheckIfWithinArea) {

		if (!checkWithinAreaInfo.GetComponentToCheckIfIsWithin()) continue;


		// Since we're only interested of the bones of the MeshComponent we're only tracing for it's object type
		const FVector traceLocation = checkWithinAreaInfo.ComponentWorldTransform.GetLocation();
		const ECollisionChannel overlappingComponentCollisionChannel = MeshComponent->GetCollisionObjectType();
		const EObjectTypeQuery overlappingComponentObjectType = UEngineTypes::ConvertToObjectType(overlappingComponentCollisionChannel);
		objectTypesToTraceFor.Add(overlappingComponentObjectType);


		TArray<FHitResult> hitResults;

		switch (checkWithinAreaInfo.PaintWithinAreaShape) {

			case EPaintWithinAreaShape::IsSquareOrRectangleShape: {

				const FVector traceHalfSize = FVector(
					checkWithinAreaInfo.SquareOrRectangle_XHalfSize,
					checkWithinAreaInfo.SquareOrRectangle_YHalfSize,
					checkWithinAreaInfo.SquareOrRectangle_ZHalfSize
				);

				// If some extent was set
				if (traceHalfSize.Size() > 0) {

					UKismetSystemLibrary::BoxTraceMultiForObjects(worldContext, traceLocation, traceLocation, traceHalfSize, checkWithinAreaInfo.GetComponentToCheckIfIsWithin()->GetComponentRotation(), objectTypesToTraceFor, false, actorsToIgnore, drawDebugType, hitResults, false, FLinearColor::Red, FLinearColor::Green, debugDuration);
				}

				break;
			}

			case EPaintWithinAreaShape::IsSphereShape: {

				const float traceSphereShapeRadius = checkWithinAreaInfo.Sphere_Radius;

				if (traceSphereShapeRadius > 0) {

					UKismetSystemLibrary::SphereTraceMultiForObjects(worldContext, traceLocation, traceLocation * 1.00001f, traceSphereShapeRadius, objectTypesToTraceFor, false, actorsToIgnore, drawDebugType, hitResults, false, FLinearColor::Red, FLinearColor::Green, debugDuration);
				}

				break;
			}

			case EPaintWithinAreaShape::IsComplexShape: {

				const FVector traceHalfSize = checkWithinAreaInfo.ComponentBounds.GetBox().GetExtent();

				UKismetSystemLibrary::BoxTraceMultiForObjects(worldContext, traceLocation, traceLocation, traceHalfSize, FRotator(0, 0, 0), objectTypesToTraceFor, false, actorsToIgnore, drawDebugType, hitResults, false, FLinearColor::Red, FLinearColor::Green, debugDuration);

				break;
			}

			case EPaintWithinAreaShape::IsCapsuleShape: {

				const float traceCapsuleShapeRadius = checkWithinAreaInfo.Capsule_Radius;
				const float traceCapsuleShapeHeight = checkWithinAreaInfo.Capsule_HalfHeight;
				const FQuat traceCapsuleShapeOrientation = checkWithinAreaInfo.Capsule_Orientation;


				if (traceCapsuleShapeRadius > 0 && traceCapsuleShapeHeight > 0) {
					
					const FCollisionShape traceShape = FCollisionShape::MakeCapsule(traceCapsuleShapeRadius, traceCapsuleShapeHeight);
					GetWorld()->SweepMultiByObjectType(hitResults, traceLocation, traceLocation, traceCapsuleShapeOrientation, objectTypesToTraceFor, traceShape, FCollisionQueryParams::DefaultQueryParam);

					
					if (debugTrace) {

					 	DrawDebugCapsule(GetWorld(), traceLocation, traceCapsuleShapeHeight, traceCapsuleShapeRadius, traceCapsuleShapeOrientation, FColor::Red, false, debugDuration, 0, 5);
					}
					
				}

				break;
			}
			
			case EPaintWithinAreaShape::IsConeShape: {

				UVertexPaintFunctionLibrary::MultiConeTraceForObjects(hitResults, checkWithinAreaInfo.GetComponentToCheckIfIsWithin(), checkWithinAreaInfo.Cone_Origin, checkWithinAreaInfo.Cone_Direction, checkWithinAreaInfo.Cone_Height, checkWithinAreaInfo.Cone_Radius, objectTypesToTraceFor, actorsToIgnore, debugTrace, debugDuration);

				break;
			}

			default:
				break;
		}


		for (const FHitResult& hitResult : hitResults) {

			if (hitResult.Component.Get() == MeshComponent) {

				const FName hitBoneName = hitResult.BoneName;

				if (hitBoneName != NAME_None && !bonesWithinArea.Contains(hitBoneName)) {

					bonesWithinArea.Add(hitBoneName);
				}
			}
		}
	}

	return bonesWithinArea;
}


//-------------------------------------------------------

// Paint On Entire Mesh

void UVertexPaintDetectionComponent::PaintOnEntireMesh(FRVPDPPaintOnEntireMeshSettings PaintOnEntireMeshStruct, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough) {

	if (!UVertexPaintFunctionLibrary::IsWorldValid(GetWorld())) return;

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_PaintOnEntireMesh);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - PaintOnEntireMesh");


	CheckIfRegisteredAndInitialized();

	PaintOnEntireMeshStruct.WeakMeshComponent = PaintOnEntireMeshStruct.MeshComponent;
	PaintOnEntireMeshStruct.TaskWorld = GetWorld();
	PaintOnEntireMeshStruct.DebugSettings.WorldTaskWasCreatedIn = GetWorld();

	if (IsValid(PaintOnEntireMeshStruct.MeshComponent)) {

		if (IsValid(PaintOnEntireMeshStruct.MeshComponent->GetOwner())) {

			RVPDP_LOG(PaintOnEntireMeshStruct.DebugSettings, FColor::Cyan, "Trying to Paint Entire Mesh on Actor: %s and Component: %s", *PaintOnEntireMeshStruct.MeshComponent->GetOwner()->GetName(), *PaintOnEntireMeshStruct.MeshComponent->GetName());
		}
	}


	GetCompleteCallbackSettings(PaintOnEntireMeshStruct.MeshComponent, PaintOnEntireMeshStruct.AutoSetRequiredCallbackSettings, PaintOnEntireMeshStruct.CallbackSettings);

	// Clamps it so if the user sets like 0.000001, we will paint at the lowest amount of 1 when converted to FColor which range from 0-255. Also clamps paint limits and conditions
	ClampPaintSettings(PaintOnEntireMeshStruct.ApplyVertexColorSettings);


	PaintOnEntireMeshStruct.CanClearOutPreviousMeshTasksInQueue = true;

	// If any is set to Add instead of Set, meaning we want to Add on top of vertex colors that the previous tasks will change then that means that previous tasks are still relevant and shouldn't be cleared out. 
	if (PaintOnEntireMeshStruct.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.ApplyVertexColorUsingPhysicsSurface) {


		for (int i = 0; i < PaintOnEntireMeshStruct.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.PhysicalSurfacesToApply.Num(); i++) {

			if (PaintOnEntireMeshStruct.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.PhysicalSurfacesToApply[i].SettingToApplyWith == EApplyVertexColorSetting::EAddVertexColor) {

				PaintOnEntireMeshStruct.CanClearOutPreviousMeshTasksInQueue = false;
				break;
			}
		}
	}

	else {

		if (PaintOnEntireMeshStruct.ApplyVertexColorSettings.ApplyColorsOnRedChannel.ApplyWithSetting == EApplyVertexColorSetting::EAddVertexColor)
			PaintOnEntireMeshStruct.CanClearOutPreviousMeshTasksInQueue = false;

		if (PaintOnEntireMeshStruct.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.ApplyWithSetting == EApplyVertexColorSetting::EAddVertexColor)
			PaintOnEntireMeshStruct.CanClearOutPreviousMeshTasksInQueue = false;

		if (PaintOnEntireMeshStruct.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.ApplyWithSetting == EApplyVertexColorSetting::EAddVertexColor)
			PaintOnEntireMeshStruct.CanClearOutPreviousMeshTasksInQueue = false;

		if (PaintOnEntireMeshStruct.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.ApplyWithSetting == EApplyVertexColorSetting::EAddVertexColor)
			PaintOnEntireMeshStruct.CanClearOutPreviousMeshTasksInQueue = false;
	}


	FRVPDPCalculateColorsInfo calculateColorsInfo;
	calculateColorsInfo.IsPaintTask = true;
	calculateColorsInfo.VertexPaintDetectionType = EVertexPaintDetectionType::PaintEntireMesh;
	calculateColorsInfo.PaintOnEntireMeshSettings = PaintOnEntireMeshStruct;
	calculateColorsInfo.ApplyColorSettings = PaintOnEntireMeshStruct;
	calculateColorsInfo.PaintTaskSettings = PaintOnEntireMeshStruct;
	calculateColorsInfo.TaskFundamentalSettings = PaintOnEntireMeshStruct;


	if (PaintOnEntireMeshCheckValid(PaintOnEntireMeshStruct)) {

		if (PaintOnEntireMeshStruct.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.ApplyVertexColorUsingPhysicsSurface) {

			RVPDP_LOG(PaintOnEntireMeshStruct.DebugSettings, FColor::Green, "Paint on Entire Mesh CheckValid Successful. Trying to Apply Color with Physics Surfaces: %s  -  Paint Strength on Surfaces Without the Physical Surface: %f", *GetAllPhysicsSurfacesToApplyAsString(PaintOnEntireMeshStruct), PaintOnEntireMeshStruct.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.StrengtOnChannelsWithoutThePhysicsSurfaces);
		}

		else {

			RVPDP_LOG(PaintOnEntireMeshStruct.DebugSettings, FColor::Green, "Paint on Entire Mesh CheckValid Successful. Trying to Apply Color: %s", *FLinearColor(PaintOnEntireMeshStruct.ApplyVertexColorSettings.ApplyColorsOnRedChannel.AmountToApply, PaintOnEntireMeshStruct.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.AmountToApply, PaintOnEntireMeshStruct.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.AmountToApply, PaintOnEntireMeshStruct.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.AmountToApply).ToString());
		}
	}

	else {

		RVPDP_LOG(PaintOnEntireMeshStruct.DebugSettings, FColor::Red, "Check Valid Failed at Paint on Entire Mesh!");

		GetTaskFundamentalsSettings(PaintOnEntireMeshStruct.MeshComponent, calculateColorsInfo);

		PaintOnEntireMeshTaskFinished(calculateColorsInfo.TaskResult, calculateColorsInfo.PaintTaskResult, calculateColorsInfo.PaintOnEntireMeshSettings, calculateColorsInfo.TaskFundamentalSettings, AdditionalDataToPassThrough, true);
		return;
	}


	FillCalculateColorsInfoFundementals(calculateColorsInfo);

	AddTaskToQueue(calculateColorsInfo, AdditionalDataToPassThrough);
}

bool UVertexPaintDetectionComponent::PaintOnEntireMeshCheckValid(const FRVPDPPaintOnEntireMeshSettings& PaintOnEntireMeshSettings, bool IgnoreTaskQueueChecks) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_PaintOnEntireMeshCheckValid);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - PaintOnEntireMeshCheckValid");

	if (!PaintColorsTaskCheckValid(PaintOnEntireMeshSettings, PaintOnEntireMeshSettings.MeshComponent, IgnoreTaskQueueChecks))
		return false;
	
	if (PaintOnEntireMeshSettings.PaintOnRandomVerticesSettings.PaintAtRandomVerticesSpreadOutOverTheEntireMesh) {

		if (PaintOnEntireMeshSettings.PaintOnRandomVerticesSettings.PaintAtRandomVerticesSpreadOutOverTheEntireMesh_PercentToPaint <= 0) {

			RVPDP_LOG(PaintOnEntireMeshSettings.DebugSettings, FColor::Red, "Check Valid, Paint at Entire Mesh Fail: Set to Paint Random Vertices Spread Out Over the Entire Mesh but the Percent to Spread out is 0 when it should be between 0-100. ");

			return false;
		}
		else if (PaintOnEntireMeshSettings.PaintOnRandomVerticesSettings.PaintAtRandomVerticesSpreadOutOverTheEntireMesh_PercentToPaint >= 100) {

			RVPDP_LOG(PaintOnEntireMeshSettings.DebugSettings, FColor::Red, "Check Valid, Paint at Entire Mesh Fail: Set to Paint Random Vertices Spread Out Over the Entire Mesh to 100 Percent, meaning the Entire Mesh should be painted, which makes it unnecessary and un-optimized to use Paint at Random Vertices when you can just turn that off and just use Paint Entire Mesh the regular way. ");

			return false;
		}



		int numberOfVertices = 0;

		if (const USkeletalMeshComponent* skelMeshComp = Cast<USkeletalMeshComponent>(PaintOnEntireMeshSettings.MeshComponent)) {

			if (skelMeshComp->GetSkeletalMeshRenderData()) {

				if (skelMeshComp->GetSkeletalMeshRenderData()->LODRenderData.IsValidIndex(0))
					numberOfVertices = skelMeshComp->GetSkeletalMeshRenderData()->LODRenderData[0].GetNumVertices();
			}
		}

		else if (const UStaticMeshComponent* staticMeshComp = Cast<UStaticMeshComponent>(PaintOnEntireMeshSettings.MeshComponent)) {

			if (staticMeshComp->GetStaticMesh()) {

				if (staticMeshComp->GetStaticMesh()->GetRenderData()) {

					if (staticMeshComp->GetStaticMesh()->GetRenderData()->LODResources.IsValidIndex(0))
						numberOfVertices = staticMeshComp->GetStaticMesh()->GetRenderData()->LODResources[0].GetNumVertices();
				}
			}
		}

		if (numberOfVertices > 0) {

			if (UKismetMathLibrary::MapRangeClamped(PaintOnEntireMeshSettings.PaintOnRandomVerticesSettings.PaintAtRandomVerticesSpreadOutOverTheEntireMesh_PercentToPaint, 0, 100, 0, numberOfVertices) < 1) {

				RVPDP_LOG(PaintOnEntireMeshSettings.DebugSettings, FColor::Red, "Check Valid, Paint at Entire Mesh Fail: Set to Paint Random Vertices Spread Out Over the Entire Mesh but the Percent set to Spread out is so low that not even a single vertex at LOD0 would get painted.");

				return false;
			}
		}
	}

	else if (PaintOnEntireMeshSettings.PaintOnRandomVerticesSettings.OnlyRandomizeWithinAreaOfEffectAtLocation) {

		RVPDP_LOG(PaintOnEntireMeshSettings.DebugSettings, FColor::Cyan, "Paint Entire Mesh Set to Randomize Vertices to Paint Within Area of Effect: %f", PaintOnEntireMeshSettings.PaintOnRandomVerticesSettings.OnlyRandomizeWithinAreaOfEffectAtLocation_AreaOfEffect);
	}


	return true;
}

void UVertexPaintDetectionComponent::PaintOnEntireMeshTaskFinished(const FRVPDPTaskResults& TaskResult, const FRVPDPPaintTaskResultInfo& PaintTaskResult, const FRVPDPPaintOnEntireMeshSettings& PaintOnEntireMeshSettings, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough, bool FailedAtCheckValid) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_PaintOnEntireMeshTaskFinished);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - PaintOnEntireMeshTaskFinished");


	if (CurrentPaintEntireMeshTasks.Contains(TaskResult.TaskID))
		CurrentPaintEntireMeshTasks.Remove(TaskResult.TaskID);

	if (TaskFundamentalSettings.CallbackSettings.RunCallbackDelegate) {

		VertexColorsPaintedEntireMeshDelegate.Broadcast(TaskResult, PaintTaskResult, PaintOnEntireMeshSettings, AdditionalDataToPassThrough);
	}


	// If failed at fundamental checks here then makes sure we run callback interfaces from here as well as the game instance won't get a callback that a task has finished. 
	if (FailedAtCheckValid) {

		UVertexPaintFunctionLibrary::RunPaintEntireMeshCallbackInterfaces(TaskResult, PaintTaskResult, PaintOnEntireMeshSettings, TaskFundamentalSettings, AdditionalDataToPassThrough);
		UVertexPaintFunctionLibrary::RunFinishedPaintTaskCallbacks(TaskResult, PaintTaskResult, TaskFundamentalSettings, AdditionalDataToPassThrough);
	}
}


//--------------------------------------------------------

// Paint Color Snippet on Mesh

void UVertexPaintDetectionComponent::PaintColorSnippetOnMesh(FRVPDPPaintColorSnippetSettings PaintColorSnippetStruct, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough) {

	if (!UVertexPaintFunctionLibrary::IsWorldValid(GetWorld())) return;

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_PaintColorSnippetOnMesh);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - PaintColorSnippetOnMesh");


	CheckIfRegisteredAndInitialized();

	PaintColorSnippetStruct.WeakMeshComponent = PaintColorSnippetStruct.MeshComponent;
	PaintColorSnippetStruct.TaskWorld = GetWorld();
	PaintColorSnippetStruct.DebugSettings.WorldTaskWasCreatedIn = GetWorld();

	// If was run with invalid MeshComponent, but as Group Snippet and vid valid group snippet meshes, which can easily happen if you call this from C++, then we manually set the MeshComponent to be 0 of the group snippet meshes since the check valids etc. requires a valid MeshComponent. 
	if (!IsValid(PaintColorSnippetStruct.MeshComponent) && PaintColorSnippetStruct.PaintColorSnippetSetting == FRVPDPPaintColorSnippetSetting::PaintGroupSnippet) {

		if (PaintColorSnippetStruct.PaintGroupSnippetMeshes.Num() > 0) {

			PaintColorSnippetStruct.MeshComponent = PaintColorSnippetStruct.PaintGroupSnippetMeshes[0];
		}
	}


	if (IsValid(PaintColorSnippetStruct.MeshComponent)) {

		if (IsValid(PaintColorSnippetStruct.MeshComponent->GetOwner())) {

			RVPDP_LOG(PaintColorSnippetStruct.DebugSettings, FColor::Cyan, "Trying to Paint Color Snippet on Actor: %s and Component: %s", *PaintColorSnippetStruct.MeshComponent->GetOwner()->GetName(), *PaintColorSnippetStruct.MeshComponent->GetName());
		}
	}


	GetCompleteCallbackSettings(PaintColorSnippetStruct.MeshComponent, PaintColorSnippetStruct.AutoSetRequiredCallbackSettings, PaintColorSnippetStruct.CallbackSettings);

	ClampPaintLimits(PaintColorSnippetStruct.VertexColorRedChannelsLimit, PaintColorSnippetStruct.VertexColorGreenChannelsLimit, PaintColorSnippetStruct.VertexColorBlueChannelsLimit, PaintColorSnippetStruct.VertexColorAlphaChannelsLimit);


	// Paint Jobs that set colors directly instead of adding ontop of old ones can clear out previous tasks in the queue since they're irrelevant. 
	PaintColorSnippetStruct.CanClearOutPreviousMeshTasksInQueue = true;


	// Fills this up here as well eventhough we're gonna have to wait for a load callback and do it again later, so even if check valid fails we can return correct things using this
	FRVPDPCalculateColorsInfo calculateColorsInfo;
	calculateColorsInfo.IsPaintTask = true;
	calculateColorsInfo.VertexPaintDetectionType = EVertexPaintDetectionType::PaintColorSnippet;
	calculateColorsInfo.PaintTaskSettings = PaintColorSnippetStruct;
	calculateColorsInfo.PaintColorSnippetSettings = PaintColorSnippetStruct;
	calculateColorsInfo.PaintDirectlyTaskSettings = PaintColorSnippetStruct;
	calculateColorsInfo.IsPaintDirectlyTask = true;
	calculateColorsInfo.TaskFundamentalSettings = PaintColorSnippetStruct;


	if (PaintColorSnippetCheckValid(PaintColorSnippetStruct)) {

		FString paintColorSnippetType = "";

		if (PaintColorSnippetStruct.PaintColorSnippetSetting == FRVPDPPaintColorSnippetSetting::PaintSingleSnippet)
			paintColorSnippetType = "Single Snippet";

		else if (PaintColorSnippetStruct.PaintColorSnippetSetting == FRVPDPPaintColorSnippetSetting::PaintGroupSnippet)
			paintColorSnippetType = "Group Snippet";


		RVPDP_LOG(PaintColorSnippetStruct.DebugSettings, FColor::Green, "Paint Color Snippet CheckValid Successful. Trying to Apply Color Snippet Type: %s", *paintColorSnippetType);
	}

	else {

		RVPDP_LOG(PaintColorSnippetStruct.DebugSettings, FColor::Red, "Check Valid Failed at Paint Color Snippet!");

		GetTaskFundamentalsSettings(PaintColorSnippetStruct.MeshComponent, calculateColorsInfo);

		PaintColorSnippetOnMeshTaskFinished(calculateColorsInfo.TaskResult, calculateColorsInfo.PaintTaskResult, calculateColorsInfo.PaintColorSnippetSettings, calculateColorsInfo.TaskFundamentalSettings, AdditionalDataToPassThrough, true);
		return;
	}


	FillCalculateColorsInfoFundementals(calculateColorsInfo);


	TMap<FGameplayTag, TSoftObjectPtr<UVertexPaintColorSnippetDataAsset>> availableColorSnippetTags;
	UVertexPaintFunctionLibrary::GetAllMeshColorSnippetsAsTags_Wrapper(PaintColorSnippetStruct.MeshComponent, availableColorSnippetTags);


	// If ColorSnippetTag isn't set, or it's set but the Mesh doesn't have the Color Snippet Tag, and the randomizeAnyColorSnippetTagUnderThisCategory is set, then we can Randomize any available color snippet tag under the category, if there are any. 
	if (PaintColorSnippetStruct.ColorSnippetTag.IsValid()) {


		switch (PaintColorSnippetStruct.PaintColorSnippetSetting) {

		case FRVPDPPaintColorSnippetSetting::PaintSingleSnippet:

			if (PaintColorSnippetStruct.RandomizeSnippetUnderChosenTagCategory) {

				TMap<FGameplayTag, TSoftObjectPtr<UVertexPaintColorSnippetDataAsset>> availableColorSnippetTagsAndDataAssets;
				TArray<FGameplayTag> meshTagsUnderCategory;

				// Get All Snippets for the Mesh under this Category, so if we get any at all we know we got something this mesh can use. 
				UVertexPaintFunctionLibrary::GetAllMeshColorSnippetsTagsUnderTagCategory_Wrapper(PaintColorSnippetStruct.MeshComponent, PaintColorSnippetStruct.ColorSnippetTag, availableColorSnippetTagsAndDataAssets);
				availableColorSnippetTagsAndDataAssets.GetKeys(meshTagsUnderCategory);

				if (availableColorSnippetTagsAndDataAssets.Num() > 0) {

					PaintColorSnippetStruct.ColorSnippetDataAssetInfo.ColorSnippetID = meshTagsUnderCategory[UKismetMathLibrary::RandomIntegerInRange(0, availableColorSnippetTagsAndDataAssets.Num() - 1)].GetTagName().ToString();

					// So even if randomizing, the colorSnippetToPaint Tag will match the randomized snippet ID. 
					PaintColorSnippetStruct.ColorSnippetTag = FGameplayTag::RequestGameplayTag(FName(*PaintColorSnippetStruct.ColorSnippetDataAssetInfo.ColorSnippetID));
				}

				else {

					RVPDP_LOG(PaintColorSnippetStruct.DebugSettings, FColor::Red, "Paint Color Snippet Task Failed as it was set to Randomize a Snippet under Category, but we couldn't find any Snippet Tag under the Category for the Mesh: %s", *PaintColorSnippetStruct.MeshComponent->GetName());

					PaintColorSnippetOnMeshTaskFinished(calculateColorsInfo.TaskResult, calculateColorsInfo.PaintTaskResult, calculateColorsInfo.PaintColorSnippetSettings, calculateColorsInfo.TaskFundamentalSettings, AdditionalDataToPassThrough, true);
					return;
				}
			}

			if (availableColorSnippetTags.Contains(PaintColorSnippetStruct.ColorSnippetTag)) {

				PaintColorSnippetStruct.ColorSnippetDataAssetInfo.ColorSnippetID = PaintColorSnippetStruct.ColorSnippetTag.GetTagName().ToString();
			}

			else {

				RVPDP_LOG(PaintColorSnippetStruct.DebugSettings, FColor::Red, "Check Valid Failed at Paint Color Snippet, trying to paint regular snippet but couldn't find a snippet associated with the Mesh that matches the choosen tag. Makes sure you choose a snippet tag that you know belongs the Mesh you're trying to paint. You can always append the Source Mesh name to the snippet ID. ");

				PaintColorSnippetOnMeshTaskFinished(calculateColorsInfo.TaskResult, calculateColorsInfo.PaintTaskResult, calculateColorsInfo.PaintColorSnippetSettings, calculateColorsInfo.TaskFundamentalSettings, AdditionalDataToPassThrough, true);
				return;
			}

			break;

		case FRVPDPPaintColorSnippetSetting::PaintGroupSnippet:


			if (PaintColorSnippetStruct.PaintGroupSnippetMeshes.Num() > 1) {


				if (PaintColorSnippetStruct.RandomizeSnippetUnderChosenTagCategory) {

					TArray<FString> groupSnippetsInCategory;

					for (auto& groupSnippetsAndMeshes : UVertexPaintFunctionLibrary::GetColorSnippetReferenceDataAsset(this)->GroupSnippetsAndAssociatedMeshes) {

						if (groupSnippetsAndMeshes.Key.StartsWith(PaintColorSnippetStruct.ColorSnippetTag.ToString()))
							groupSnippetsInCategory.Add(groupSnippetsAndMeshes.Key);
					}


					if (groupSnippetsInCategory.Num() > 0) {

						PaintColorSnippetStruct.ColorSnippetDataAssetInfo.ColorSnippetID = groupSnippetsInCategory[UKismetMathLibrary::RandomIntegerInRange(0, groupSnippetsInCategory.Num() - 1)];

						// So even if randomizing, the colorSnippetToPaint Tag will match the randomized snippet ID. 
						PaintColorSnippetStruct.ColorSnippetTag = FGameplayTag::RequestGameplayTag(FName(*PaintColorSnippetStruct.ColorSnippetDataAssetInfo.ColorSnippetID));
					}

					else {

						RVPDP_LOG(PaintColorSnippetStruct.DebugSettings, FColor::Red, "Paint Group Snippet Task Failed as it was set to Randomize a Snippet under Category, but we couldn't find any Snippet Tag under the Category for the Mesh: %s", *PaintColorSnippetStruct.MeshComponent->GetName());

						PaintColorSnippetOnMeshTaskFinished(calculateColorsInfo.TaskResult, calculateColorsInfo.PaintTaskResult, calculateColorsInfo.PaintColorSnippetSettings, calculateColorsInfo.TaskFundamentalSettings, AdditionalDataToPassThrough, true);
						return;
					}
				}


				TMap<FString, UPrimitiveComponent*> childSnippetsAndMatchingMeshes;
				if (UVertexPaintFunctionLibrary::GetColorSnippetReferenceDataAsset(this)->CheckAndGetTheComponentsThatMatchGroupChildSnippets(this, PaintColorSnippetStruct.ColorSnippetTag.ToString(), PaintColorSnippetStruct.PaintGroupSnippetMeshes, childSnippetsAndMatchingMeshes)) {

					TArray<UPrimitiveComponent*> meshComponentsToPaintGroupSnippetsOn;
					childSnippetsAndMatchingMeshes.GenerateValueArray(meshComponentsToPaintGroupSnippetsOn);

					// Updates the Paint Group Snippet Meshes array with the ones actually associated with the snippet, in case the user sends other meshes as well. 
					PaintColorSnippetStruct.PaintGroupSnippetMeshes = meshComponentsToPaintGroupSnippetsOn;

					// Since Paint Group Snippet Queues up other regular Paint Snippet tasks, we remove the initial one added 
					CurrentPaintColorSnippetTasks.Remove(calculateColorsInfo.TaskID);

					RVPDP_LOG(PaintColorSnippetStruct.DebugSettings, FColor::Cyan, "Paint Color Snippet, Trying to Paint Group Snippet and Successfully associated the right amount of Tags to Meshes, so loops through and starts regular Paint Color Snippet Single for those Meshes with the right Tag.");

					for (auto snippetAndMatchingMesh : childSnippetsAndMatchingMeshes) {

						FName tagName = FName(*snippetAndMatchingMesh.Key);
						FGameplayTag colorSnippetTag;
						colorSnippetTag = FGameplayTag::RequestGameplayTag(tagName, true);

						FRVPDPPaintColorSnippetSettings childSnippetSettings = PaintColorSnippetStruct;
						childSnippetSettings.ColorSnippetTag = colorSnippetTag;
						childSnippetSettings.MeshComponent = snippetAndMatchingMesh.Value;
						childSnippetSettings.PaintColorSnippetSetting = FRVPDPPaintColorSnippetSetting::PaintSingleSnippet;
						childSnippetSettings.RandomizeSnippetUnderChosenTagCategory = false;

						PaintColorSnippetOnMesh(childSnippetSettings, AdditionalDataToPassThrough);
					}

					return;
				}

				RVPDP_LOG(PaintColorSnippetStruct.DebugSettings, FColor::Red, "Paint Color Snippet Task Failed, trying to paint group snippet but didn't associate correct Meshes to the tags under the group. Most likely incorrect Meshes where connected or their relative Location/rotation isn't the same as when the group snippet was created.");

				PaintColorSnippetOnMeshTaskFinished(calculateColorsInfo.TaskResult, calculateColorsInfo.PaintTaskResult, calculateColorsInfo.PaintColorSnippetSettings, calculateColorsInfo.TaskFundamentalSettings, AdditionalDataToPassThrough, true);
				return;
			}

			else {

				RVPDP_LOG(PaintColorSnippetStruct.DebugSettings, FColor::Red, "Check Valid Failed at Paint Color Snippet, trying to Paint Group Snippet but no meshes has been connected. ");

				PaintColorSnippetOnMeshTaskFinished(calculateColorsInfo.TaskResult, calculateColorsInfo.PaintTaskResult, calculateColorsInfo.PaintColorSnippetSettings, calculateColorsInfo.TaskFundamentalSettings, AdditionalDataToPassThrough, true);
				return;
			}

			break;

		default:
			break;
		
		}
	}


	if (PaintColorSnippetStruct.ColorSnippetDataAssetInfo.ColorSnippetID.IsEmpty()) {

		RVPDP_LOG(PaintColorSnippetStruct.DebugSettings, FColor::Red, "Check Valid Failed at Paint Color Snippet, couldn't get Valid Snippet ID to Apply!");

		PaintColorSnippetOnMeshTaskFinished(calculateColorsInfo.TaskResult, calculateColorsInfo.PaintTaskResult, calculateColorsInfo.PaintColorSnippetSettings, calculateColorsInfo.TaskFundamentalSettings, AdditionalDataToPassThrough, true);
		return;
	}


	// Updates this after setting ColorSnippetID
	calculateColorsInfo.PaintColorSnippetSettings = PaintColorSnippetStruct;


	// If Color Snippet ID has been set and matches the tag
	if (PaintColorSnippetStruct.ColorSnippetDataAssetInfo.ColorSnippetID == PaintColorSnippetStruct.ColorSnippetTag.GetTagName().ToString()) {

		if (availableColorSnippetTags.FindRef(PaintColorSnippetStruct.ColorSnippetTag).ToSoftObjectPath().ToString().Len() > 0) {

			if (availableColorSnippetTags.FindRef(PaintColorSnippetStruct.ColorSnippetTag).Get()) {


				RVPDP_LOG(PaintColorSnippetStruct.DebugSettings, FColor::Cyan, "Successfull in getting the Color Snippet Data Asset as it was already loaded!");

				PaintColorSnippetStruct.ColorSnippetDataAssetInfo.ColorSnippetDataAssetSnippetIsStoredIn = availableColorSnippetTags.FindRef(PaintColorSnippetStruct.ColorSnippetTag).Get();

				// Updates after setting data asset ref
				calculateColorsInfo.PaintColorSnippetSettings = PaintColorSnippetStruct;

				WrapUpLoadedColorSnippetDataAsset(calculateColorsInfo, AdditionalDataToPassThrough, ELoadColorSnippetDataAssetOptions::LoadPaintColorSnippetDataAsset);
			}

			else {

				TMap<FString, FSoftObjectPath> colorSnippetIDsAndDataAssetPath;
				colorSnippetIDsAndDataAssetPath.Add(PaintColorSnippetStruct.ColorSnippetDataAssetInfo.ColorSnippetID, availableColorSnippetTags.FindRef(PaintColorSnippetStruct.ColorSnippetTag).ToSoftObjectPath());

				if (LoadColorSnippetDataAsset(calculateColorsInfo, AdditionalDataToPassThrough, colorSnippetIDsAndDataAssetPath, ELoadColorSnippetDataAssetOptions::LoadPaintColorSnippetDataAsset)) {

					//
				}

				else {

					RVPDP_LOG(PaintColorSnippetStruct.DebugSettings, FColor::Red, "Failed to Start Color Snippet Async Load Request and thus the Task!");

					calculateColorsInfo.PaintColorSnippetSettings = PaintColorSnippetStruct; // Since we set ID etc. we update this before returning

					PaintColorSnippetOnMeshTaskFinished(calculateColorsInfo.TaskResult, calculateColorsInfo.PaintTaskResult, calculateColorsInfo.PaintColorSnippetSettings, calculateColorsInfo.TaskFundamentalSettings, AdditionalDataToPassThrough, true);
					return;
				}
			}
		}

		else {

			RVPDP_LOG(PaintColorSnippetStruct.DebugSettings, FColor::Red, "Paint Color Snippet Fail as we the Snippet Found didn't point to a valid color snippet data asset soft pointer.");
		}
	}

	else {

		RVPDP_LOG(PaintColorSnippetStruct.DebugSettings, FColor::Red, "Paint Color Snippet Fail as the Color Snippet ID didn't get set correctly. The Tag you tried to Paint is probably not Registered to this Mesh. If you know it is, try saving and restarting the editor.");
	}

}

bool UVertexPaintDetectionComponent::PaintColorSnippetCheckValid(const FRVPDPPaintColorSnippetSettings& PaintColorSnippetSettings) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_PaintColorSnippetCheckValid);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - PaintColorSnippetCheckValid");


	if (!PaintTaskCheckValid(PaintColorSnippetSettings, PaintColorSnippetSettings.MeshComponent))
		return false;

	if (!PaintColorSnippetSettings.ColorSnippetTag.IsValid()) {


		RVPDP_LOG(PaintColorSnippetSettings.DebugSettings, FColor::Red, "PaintColorSnippet Failed because no Snippet Tag was provided and it's not set to Randomize a Snippet Tag either!");

		return false;
	}

	if (!UVertexPaintFunctionLibrary::GetColorSnippetReferenceDataAsset(this)) {

		RVPDP_LOG(PaintColorSnippetSettings.DebugSettings, FColor::Red, "PaintColorSnippet Failed because the Color Snippet Reference Data Asset isn't set!");
		return false;
	}

	return true;
}

void UVertexPaintDetectionComponent::PaintColorSnippetOnMeshTaskFinished(const FRVPDPTaskResults& TaskResult, const FRVPDPPaintTaskResultInfo& PaintTaskResult, const FRVPDPPaintColorSnippetSettings& PaintColorSnippetSettings, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough, bool FailedAtCheckValid) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_PaintColorSnippetOnMeshTaskFinished);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - PaintColorSnippetOnMeshTaskFinished");


	if (CurrentPaintColorSnippetTasks.Contains(TaskResult.TaskID))
		CurrentPaintColorSnippetTasks.Remove(TaskResult.TaskID);

	if (TaskFundamentalSettings.CallbackSettings.RunCallbackDelegate) {

		VertexColorsPaintColorSnippetDelegate.Broadcast(TaskResult, PaintTaskResult, PaintColorSnippetSettings, AdditionalDataToPassThrough);
	}


	// If failed at fundamental checks here then makes sure we run callback interfaces from here as well as the game instance won't get a callback that a task has finished. 
	if (FailedAtCheckValid) {

		UVertexPaintFunctionLibrary::RunPaintColorSnippetCallbackInterfaces(TaskResult, PaintTaskResult, PaintColorSnippetSettings, TaskFundamentalSettings, AdditionalDataToPassThrough);
		UVertexPaintFunctionLibrary::RunFinishedPaintTaskCallbacks(TaskResult, PaintTaskResult, TaskFundamentalSettings, AdditionalDataToPassThrough);
	}
}


//--------------------------------------------------------

// Set Mesh Component Vertex Colors

void UVertexPaintDetectionComponent::SetMeshComponentVertexColors(FRVPDPSetVertexColorsSettings SetMeshComponentVertexColorsSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough) {

	if (!UVertexPaintFunctionLibrary::IsWorldValid(GetWorld())) return;

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_SetMeshComponentVertexColors);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - SetMeshComponentVertexColors");


	CheckIfRegisteredAndInitialized();

	SetMeshComponentVertexColorsSettings.WeakMeshComponent = SetMeshComponentVertexColorsSettings.MeshComponent;
	SetMeshComponentVertexColorsSettings.TaskWorld = GetWorld();
	SetMeshComponentVertexColorsSettings.DebugSettings.WorldTaskWasCreatedIn = GetWorld();

	if (IsValid(SetMeshComponentVertexColorsSettings.MeshComponent)) {

		if (IsValid(SetMeshComponentVertexColorsSettings.MeshComponent->GetOwner())) {

			RVPDP_LOG(SetMeshComponentVertexColorsSettings.DebugSettings, FColor::Cyan, "Trying to Set Mesh Component Vertex Colors on Actor: %s and Component: %s", *SetMeshComponentVertexColorsSettings.MeshComponent->GetOwner()->GetName(), *SetMeshComponentVertexColorsSettings.MeshComponent->GetName());
		}
	}


	GetCompleteCallbackSettings(SetMeshComponentVertexColorsSettings.MeshComponent, SetMeshComponentVertexColorsSettings.AutoSetRequiredCallbackSettings, SetMeshComponentVertexColorsSettings.CallbackSettings);

	ClampPaintLimits(SetMeshComponentVertexColorsSettings.VertexColorRedChannelsLimit, SetMeshComponentVertexColorsSettings.VertexColorGreenChannelsLimit, SetMeshComponentVertexColorsSettings.VertexColorBlueChannelsLimit, SetMeshComponentVertexColorsSettings.VertexColorAlphaChannelsLimit);


	// Paint Jobs that set colors directly instead of adding ontop of old ones can clear out previous tasks in the queue since they're irrelevant. 
	SetMeshComponentVertexColorsSettings.CanClearOutPreviousMeshTasksInQueue = true;


	FRVPDPCalculateColorsInfo calculateColorsInfo;
	calculateColorsInfo.IsPaintTask = true;
	calculateColorsInfo.VertexPaintDetectionType = EVertexPaintDetectionType::SetMeshVertexColorsDirectly;
	calculateColorsInfo.PaintTaskSettings = SetMeshComponentVertexColorsSettings;
	calculateColorsInfo.SetVertexColorsSettings = SetMeshComponentVertexColorsSettings;
	calculateColorsInfo.PaintDirectlyTaskSettings = SetMeshComponentVertexColorsSettings;
	calculateColorsInfo.IsPaintDirectlyTask = true;
	calculateColorsInfo.TaskFundamentalSettings = SetMeshComponentVertexColorsSettings;


	if (SetMeshComponentVertexColorsCheckValid(SetMeshComponentVertexColorsSettings)) {

		RVPDP_LOG(SetMeshComponentVertexColorsSettings.DebugSettings, FColor::Green, "Check Valid Successful for Set Mesh Component Vertex Colors");
	}

	else {

		RVPDP_LOG(SetMeshComponentVertexColorsSettings.DebugSettings, FColor::Red, "Check Valid Failed for Set Mesh Component Vertex Colors");

		GetTaskFundamentalsSettings(SetMeshComponentVertexColorsSettings.MeshComponent, calculateColorsInfo);

		SetMeshComponentVertexColorsTaskFinished(calculateColorsInfo.TaskResult, calculateColorsInfo.PaintTaskResult, calculateColorsInfo.SetVertexColorsSettings, calculateColorsInfo.TaskFundamentalSettings, AdditionalDataToPassThrough, true);
		return;
	}


	FillCalculateColorsInfoFundementals(calculateColorsInfo);

	AddTaskToQueue(calculateColorsInfo, AdditionalDataToPassThrough);
}

bool UVertexPaintDetectionComponent::SetMeshComponentVertexColorsCheckValid(const FRVPDPSetVertexColorsSettings& SetMeshComponentVertexColorsSettings) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_SetMeshComponentVertexColorsCheckValid);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - SetMeshComponentVertexColorsCheckValid");

	if (!PaintTaskCheckValid(SetMeshComponentVertexColorsSettings, SetMeshComponentVertexColorsSettings.MeshComponent))
		return false;

	// If passed fundamental checks, then checks set mesh component vertex colors specific ones
	if (SetMeshComponentVertexColorsSettings.VertexColorsAtLOD0ToSet.Num() == 0) {

		RVPDP_LOG(SetMeshComponentVertexColorsSettings.DebugSettings, FColor::Red, "Trying to Set Mesh Component Vertex Colors but the color array passed in are 0 in length!");

		return false;
	}

	return true;
}

void UVertexPaintDetectionComponent::SetMeshComponentVertexColorsTaskFinished(const FRVPDPTaskResults& TaskResult, const FRVPDPPaintTaskResultInfo& PaintTaskResult, const FRVPDPSetVertexColorsSettings& SetVertexColorsSettings, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough, bool FailedAtCheckValid) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_SetMeshComponentVertexColorsTaskFinished);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - SetMeshComponentVertexColorsTaskFinished");


	if (CurrentSetMeshComponentVertexColorsTasks.Contains(TaskResult.TaskID))
		CurrentSetMeshComponentVertexColorsTasks.Remove(TaskResult.TaskID);

	if (TaskFundamentalSettings.CallbackSettings.RunCallbackDelegate) {

		VertexColorsSetMeshColorsDelegate.Broadcast(TaskResult, PaintTaskResult, SetVertexColorsSettings, AdditionalDataToPassThrough);
	}


	// If failed at fundamental checks here then makes sure we run callback interfaces from here as well as the game instance won't get a callback that a task has finished. 
	if (FailedAtCheckValid) {

		UVertexPaintFunctionLibrary::RunPaintSetMeshColorsCallbackInterfaces(TaskResult, PaintTaskResult, SetVertexColorsSettings, TaskFundamentalSettings, AdditionalDataToPassThrough);
		UVertexPaintFunctionLibrary::RunFinishedPaintTaskCallbacks(TaskResult, PaintTaskResult, TaskFundamentalSettings, AdditionalDataToPassThrough);
	}
}


//--------------------------------------------------------

// Set Mesh Component Vertex Colors Using SerializedString

void UVertexPaintDetectionComponent::SetMeshComponentVertexColorsUsingSerializedString(FRVPDPSetVertexColorsUsingSerializedStringSettings SetMeshComponentVertexColorsUsingSerializedStringSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough) {

	if (!UVertexPaintFunctionLibrary::IsWorldValid(GetWorld())) return;

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_SetMeshComponentVertexColorsUsingSerializedString);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - SetMeshComponentVertexColorsUsingSerializedString");


	CheckIfRegisteredAndInitialized();

	SetMeshComponentVertexColorsUsingSerializedStringSettings.WeakMeshComponent = SetMeshComponentVertexColorsUsingSerializedStringSettings.MeshComponent;
	SetMeshComponentVertexColorsUsingSerializedStringSettings.TaskWorld = GetWorld();
	SetMeshComponentVertexColorsUsingSerializedStringSettings.DebugSettings.WorldTaskWasCreatedIn = GetWorld();

	if (IsValid(SetMeshComponentVertexColorsUsingSerializedStringSettings.MeshComponent)) {

		if (IsValid(SetMeshComponentVertexColorsUsingSerializedStringSettings.MeshComponent->GetOwner())) {

			RVPDP_LOG(SetMeshComponentVertexColorsUsingSerializedStringSettings.DebugSettings, FColor::Cyan, "Trying to Set Mesh Component Vertex Colors Using Serialized String on Actor: %s and Component: %s", *SetMeshComponentVertexColorsUsingSerializedStringSettings.MeshComponent->GetOwner()->GetName(), *SetMeshComponentVertexColorsUsingSerializedStringSettings.MeshComponent->GetName());
		}
	}


	GetCompleteCallbackSettings(SetMeshComponentVertexColorsUsingSerializedStringSettings.MeshComponent, SetMeshComponentVertexColorsUsingSerializedStringSettings.AutoSetRequiredCallbackSettings, SetMeshComponentVertexColorsUsingSerializedStringSettings.CallbackSettings);

	ClampPaintLimits(SetMeshComponentVertexColorsUsingSerializedStringSettings.VertexColorRedChannelsLimit, SetMeshComponentVertexColorsUsingSerializedStringSettings.VertexColorGreenChannelsLimit, SetMeshComponentVertexColorsUsingSerializedStringSettings.VertexColorBlueChannelsLimit, SetMeshComponentVertexColorsUsingSerializedStringSettings.VertexColorAlphaChannelsLimit);


	// Paint Jobs that set colors directly instead of adding ontop of old ones can clear out previous tasks in the queue since they're irrelevant. 
	SetMeshComponentVertexColorsUsingSerializedStringSettings.CanClearOutPreviousMeshTasksInQueue = true;


	FRVPDPCalculateColorsInfo calculateColorsInfo;
	calculateColorsInfo.IsPaintTask = true;
	calculateColorsInfo.VertexPaintDetectionType = EVertexPaintDetectionType::SetMeshVertexColorsDirectlyUsingSerializedString;
	calculateColorsInfo.PaintTaskSettings = SetMeshComponentVertexColorsUsingSerializedStringSettings;
	calculateColorsInfo.SetVertexColorsUsingSerializedStringSettings = SetMeshComponentVertexColorsUsingSerializedStringSettings;
	calculateColorsInfo.PaintDirectlyTaskSettings = SetMeshComponentVertexColorsUsingSerializedStringSettings;
	calculateColorsInfo.IsPaintDirectlyTask = true;
	calculateColorsInfo.TaskFundamentalSettings = SetMeshComponentVertexColorsUsingSerializedStringSettings;

	
	if (SetMeshComponentVertexColorsUsingSerializedStringCheckValid(SetMeshComponentVertexColorsUsingSerializedStringSettings)) {

		RVPDP_LOG(SetMeshComponentVertexColorsUsingSerializedStringSettings.DebugSettings, FColor::Green, "Check Valid Successful for Set Mesh Component Vertex Colors Using Serialized String");
	}

	else {

		RVPDP_LOG(SetMeshComponentVertexColorsUsingSerializedStringSettings.DebugSettings, FColor::Red, "Check Valid Failed for Set Mesh Component Vertex Colors Using Serialized String");

		GetTaskFundamentalsSettings(SetMeshComponentVertexColorsUsingSerializedStringSettings.MeshComponent, calculateColorsInfo);

		SetMeshComponentVertexColorsUsingSerializedStringTaskFinished(calculateColorsInfo.TaskResult, calculateColorsInfo.PaintTaskResult, calculateColorsInfo.SetVertexColorsUsingSerializedStringSettings, calculateColorsInfo.TaskFundamentalSettings, AdditionalDataToPassThrough, true);
		return;
	}


	FillCalculateColorsInfoFundementals(calculateColorsInfo);

	AddTaskToQueue(calculateColorsInfo, AdditionalDataToPassThrough);
}

bool UVertexPaintDetectionComponent::SetMeshComponentVertexColorsUsingSerializedStringCheckValid(const FRVPDPSetVertexColorsUsingSerializedStringSettings& SetMeshComponentVertexColorsUsingSerializedStringSettings) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_SetMeshComponentVertexColorsUsingSerializedStringCheckValid);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - SetMeshComponentVertexColorsUsingSerializedStringCheckValid");


	if (!PaintTaskCheckValid(SetMeshComponentVertexColorsUsingSerializedStringSettings, SetMeshComponentVertexColorsUsingSerializedStringSettings.MeshComponent))
		return false;

	if (SetMeshComponentVertexColorsUsingSerializedStringSettings.SerializedColorDataAtLOD0.Len() <= 0) {

		RVPDP_LOG(SetMeshComponentVertexColorsUsingSerializedStringSettings.DebugSettings, FColor::Red, "Trying to Set Mesh Component Vertex Colors Using Serialized String but the serializedColorData String passed in are 0 in length!");

		return false;
	}


	return true;
}

void UVertexPaintDetectionComponent::SetMeshComponentVertexColorsUsingSerializedStringTaskFinished(const FRVPDPTaskResults& TaskResult, const FRVPDPPaintTaskResultInfo& PaintTaskResult, const FRVPDPSetVertexColorsUsingSerializedStringSettings& SetVertexColorsUsingSerializedStringSettings, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough, bool FailedAtCheckValid) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_SetMeshComponentVertexColorsUsingSerializedStringTaskFinished);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - SetMeshComponentVertexColorsUsingSerializedStringTaskFinished");


	if (CurrentSetMeshComponentVertexColorsUsingSerializedStringTasks.Contains(TaskResult.TaskID))
		CurrentSetMeshComponentVertexColorsUsingSerializedStringTasks.Remove(TaskResult.TaskID);

	if (TaskFundamentalSettings.CallbackSettings.RunCallbackDelegate) {

		VertexColorsSetMeshColorsUsingSerializedStringDelegate.Broadcast(TaskResult, PaintTaskResult, SetVertexColorsUsingSerializedStringSettings, AdditionalDataToPassThrough);
	}


	// If failed at fundamental checks here then makes sure we run callback interfaces from here as well as the game instance won't get a callback that a task has finished. 
	if (FailedAtCheckValid) {

		UVertexPaintFunctionLibrary::RunPaintSetMeshColorsUsingSerializedStringCallbackInterfaces(TaskResult, PaintTaskResult, SetVertexColorsUsingSerializedStringSettings, TaskFundamentalSettings, AdditionalDataToPassThrough);
		UVertexPaintFunctionLibrary::RunFinishedPaintTaskCallbacks(TaskResult, PaintTaskResult, TaskFundamentalSettings, AdditionalDataToPassThrough);
	}
}


//--------------------------------------------------------

// Does Compare Color Snippet Info Need To BeSet

bool UVertexPaintDetectionComponent::DoesCompareColorSnippetInfoNeedToBeSet(FRVPDPCalculateColorsInfo CalculateColorsInfo, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_DoesCompareColorSnippetInfoNeedToBeSet);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - DoesCompareColorSnippetInfoNeedToBeSet");

	if (!CalculateColorsInfo.TaskFundamentalSettings.MeshComponent) return false;
	if (CalculateColorsInfo.TaskFundamentalSettings.CallbackSettings.CompareMeshVertexColorsToColorSnippets.CompareWithColorsSnippetTag.Num() == 0) return false;

	// If already been set
	if (CalculateColorsInfo.TaskFundamentalSettings.CallbackSettings.CompareMeshVertexColorsToColorSnippets.CompareWithColorsSnippetDataAssetInfo.Num() > 0) return false;


	TMap<FGameplayTag, TSoftObjectPtr<UVertexPaintColorSnippetDataAsset>> availableColorSnippetTags;
	UVertexPaintFunctionLibrary::GetAllMeshColorSnippetsAsTags_Wrapper(CalculateColorsInfo.TaskFundamentalSettings.MeshComponent, availableColorSnippetTags);


	bool loadCompareColorSnippetDataAssets = false;
	int amountOfLoadedDataAssets = 0;


	for (const FGameplayTag& colorSnippetTag : CalculateColorsInfo.TaskFundamentalSettings.CallbackSettings.CompareMeshVertexColorsToColorSnippets.CompareWithColorsSnippetTag) {

		if (!availableColorSnippetTags.Contains(colorSnippetTag)) {

			RVPDP_LOG(CalculateColorsInfo.TaskFundamentalSettings.DebugSettings, FColor::Orange, "Set to Compare with Color Snippet but the Color Snippet Tag %s isn't available, but is valid. Perhaps you've selected a 'Parent' Tag, for instance if you have Floor.Oily registered and choose Floor.  ", *colorSnippetTag.ToString());

			continue;
		}


		// If already loaded
		if (availableColorSnippetTags.FindRef(colorSnippetTag).Get()) {

			RVPDP_LOG(CalculateColorsInfo.TaskFundamentalSettings.DebugSettings, FColor::Cyan, "Set to Compare with Color Snippet: %s which is in a Data Asset that's already loaded.", *colorSnippetTag.ToString());

			amountOfLoadedDataAssets++;
		}


		// If not loaded and has a valid path
		else if (availableColorSnippetTags.FindRef(colorSnippetTag).ToSoftObjectPath().IsValid()) {


			RVPDP_LOG(CalculateColorsInfo.TaskFundamentalSettings.DebugSettings, FColor::Cyan, "Set to Compare with Color Snippet: %s which is in a Data Asset that needs to be loaded. ", *colorSnippetTag.ToString());

			loadCompareColorSnippetDataAssets = true;
		}
	}


	// If at least one data asset isn't loaded then we run the load logic for all so it's easier to update calculate colors info when all callbacks from it has arrived
	if (loadCompareColorSnippetDataAssets) {

		for (const FGameplayTag& colorSnippetTag : CalculateColorsInfo.TaskFundamentalSettings.CallbackSettings.CompareMeshVertexColorsToColorSnippets.CompareWithColorsSnippetTag) {

			if (!availableColorSnippetTags.Contains(colorSnippetTag)) continue;


			TMap<FString, FSoftObjectPath> colorSnippetIDsAndDataAssetPath;
			colorSnippetIDsAndDataAssetPath.Add(colorSnippetTag.ToString(), availableColorSnippetTags.FindRef(colorSnippetTag).ToSoftObjectPath());

			LoadColorSnippetDataAsset(CalculateColorsInfo, AdditionalDataToPassThrough, colorSnippetIDsAndDataAssetPath, ELoadColorSnippetDataAssetOptions::LoadCompareWithColorSnippetDataAsset);
		}

		return true;
	}

	else if (amountOfLoadedDataAssets > 0){


		CalculateColorsInfo.TaskFundamentalSettings.CallbackSettings.CompareMeshVertexColorsToColorSnippets.CompareWithColorsSnippetDataAssetInfo.Empty();

		// If already loaded then just sets the compareWithColorsSnippetDataAssetInfos and wraps ups
		for (const FGameplayTag& colorSnippetTag : CalculateColorsInfo.TaskFundamentalSettings.CallbackSettings.CompareMeshVertexColorsToColorSnippets.CompareWithColorsSnippetTag) {

			if (!availableColorSnippetTags.Contains(colorSnippetTag)) continue;


			FRVPDPColorSnippetDataAssetInfo colorSnippetDataAssetInfo;
			colorSnippetDataAssetInfo.ColorSnippetID = colorSnippetTag.ToString();
			colorSnippetDataAssetInfo.ColorSnippetDataAssetSnippetIsStoredIn = availableColorSnippetTags.FindRef(colorSnippetTag).Get();

			CalculateColorsInfo.TaskFundamentalSettings.CallbackSettings.CompareMeshVertexColorsToColorSnippets.CompareWithColorsSnippetDataAssetInfo.Add(colorSnippetDataAssetInfo);
		}


		WrapUpLoadedColorSnippetDataAsset(CalculateColorsInfo, AdditionalDataToPassThrough, ELoadColorSnippetDataAssetOptions::LoadCompareWithColorSnippetDataAsset);

		return true;
	}


	return false;
}

bool UVertexPaintDetectionComponent::LoadColorSnippetDataAsset(const FRVPDPCalculateColorsInfo& CalculateColorsInfo, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough, const TMap<FString, FSoftObjectPath>& ColorSnippetIDsAndDataAssetPath, ELoadColorSnippetDataAssetOptions LoadColorSnippetDataAssetSetting) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_LoadColorSnippetDataAsset);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - LoadColorSnippetDataAsset");

	if (ColorSnippetIDsAndDataAssetPath.Num() == 0) return false;


	TArray<FRVPDPLoadColorSnippetInfo> loadColorSnippetInfos;
	TArray<int> currentColorSnippetRequestAsyncTaskIDs;
	ColorSnippetAsyncLoadHandleMap.GenerateKeyArray(currentColorSnippetRequestAsyncTaskIDs);


	for (auto& colorSnippetIDAndPath : ColorSnippetIDsAndDataAssetPath) {

		if (colorSnippetIDAndPath.Key.IsEmpty()) continue;
		if (!colorSnippetIDAndPath.Value.IsValid()) continue;


		const int taskID = currentColorSnippetRequestAsyncTaskIDs.Num() + 1;
		currentColorSnippetRequestAsyncTaskIDs.Add(taskID);

		FRVPDPLoadColorSnippetInfo loadColorSnippetInfo;
		loadColorSnippetInfo.ColorSnippetID = colorSnippetIDAndPath.Key;
		loadColorSnippetInfo.LoadPath = colorSnippetIDAndPath.Value;
		loadColorSnippetInfo.LoadTaskID = taskID;

		loadColorSnippetInfos.Add(loadColorSnippetInfo);
	}


	// Resets globals before loading which used when all has been finished loading
	LoadColorSnippetDataAssetsCallbacks = 0;
	LoadedCompareColorSnippetDataAssets.Empty();


	for (int i = 0; i < loadColorSnippetInfos.Num(); i++) {

		
		TAsyncLoadPriority asyncLoadPriority = FStreamableManager::AsyncLoadHighPriority;
		const FString colorSnippedID = loadColorSnippetInfos[i].ColorSnippetID;
		const int taskID = loadColorSnippetInfos[i].LoadTaskID;
		const int expectedAmountOfCallbacks = loadColorSnippetInfos.Num();

		auto callbackLambda = [this, CalculateColorsInfo, AdditionalDataToPassThrough, colorSnippedID, LoadColorSnippetDataAssetSetting, taskID, expectedAmountOfCallbacks]() {

			this->FinishedLoadingColorSnippetDataAsset(taskID, CalculateColorsInfo, AdditionalDataToPassThrough, colorSnippedID, LoadColorSnippetDataAssetSetting, expectedAmountOfCallbacks);

			return;
		};


		FStreamableDelegate callbackDelegate;
		callbackDelegate.BindLambda(callbackLambda);

		ColorSnippetAsyncLoadHandleMap.Add(loadColorSnippetInfos[i].LoadTaskID, UAssetManager::GetStreamableManager().RequestAsyncLoad(loadColorSnippetInfos[i].LoadPath, callbackDelegate, asyncLoadPriority, false, false, "Async Loading Color Snippet Data Asset"));


		switch (LoadColorSnippetDataAssetSetting) {

		case ELoadColorSnippetDataAssetOptions::LoadCompareWithColorSnippetDataAsset:

			RVPDP_LOG(CalculateColorsInfo.TaskFundamentalSettings.DebugSettings, FColor::Cyan, "Couldn't Resolve Soft Ptr so started Loading Color Snippet Data Asset that stores Snippet ID: %s to Compare Colors with Task ID: %i. ", *colorSnippedID, taskID);

			break;

		case ELoadColorSnippetDataAssetOptions::LoadPaintColorSnippetDataAsset:

			RVPDP_LOG(CalculateColorsInfo.TaskFundamentalSettings.DebugSettings, FColor::Cyan, "Couldn't Resolve Soft Ptr so started Loading Color Snippet Data Asset that stores Snippet ID: %s for Paint Color Snippet Paint Job with Task ID: %i ", *colorSnippedID, taskID);

			break;

		default:
			break;
		}
	}

	return true;
}

void UVertexPaintDetectionComponent::FinishedLoadingColorSnippetDataAsset(int FinishedTaskID, FRVPDPCalculateColorsInfo CalculateColorsInfo, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough, const FString& ColorSnippetID, ELoadColorSnippetDataAssetOptions LoadedColorSnippetDataAssetSettings, int AmountOfExpectedFinishedLoadingCallbacks) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_FinishedLoadingColorSnippetDataAsset);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - FinishedLoadingColorSnippetDataAsset");

	if (!IsInGameThread()) return;
	if (!CalculateColorsInfo.TaskFundamentalSettings.TaskWorld.IsValid() || !UVertexPaintFunctionLibrary::IsWorldValid(CalculateColorsInfo.TaskFundamentalSettings.TaskWorld.Get())) return;


	LoadColorSnippetDataAssetsCallbacks++;


	FString loadColorSnippetSetting = "";

	if (LoadedColorSnippetDataAssetSettings == ELoadColorSnippetDataAssetOptions::LoadPaintColorSnippetDataAsset)
		loadColorSnippetSetting = "'Paint Color Snippet'";

	else if (LoadedColorSnippetDataAssetSettings == ELoadColorSnippetDataAssetOptions::LoadCompareWithColorSnippetDataAsset)
		loadColorSnippetSetting = "'Compare Color Snippet'";


	RVPDP_LOG(CalculateColorsInfo.TaskFundamentalSettings.DebugSettings, FColor::Cyan, "Loading Color Snippet Data Asset Callback: %s, for Task: %i  -  Was trying to Load Data Asset for Snippet: %s", *loadColorSnippetSetting, FinishedTaskID, *ColorSnippetID);


	TSharedPtr<FStreamableHandle> finishedStreamableHandle;

	TArray<int> taskIDs;
	ColorSnippetAsyncLoadHandleMap.GenerateKeyArray(taskIDs);

	TArray<TSharedPtr<FStreamableHandle>> streamableHandles;
	ColorSnippetAsyncLoadHandleMap.GenerateValueArray(streamableHandles);

	// Finds the streamable handle associated with the finished handle ID and then removes the ID from the TMap
	for (int i = 0; i < taskIDs.Num(); i++) {

		if (taskIDs[i] == FinishedTaskID) {

			finishedStreamableHandle = streamableHandles[i];
			ColorSnippetAsyncLoadHandleMap.Remove(taskIDs[i]);

			break;
		}
	}


	if (finishedStreamableHandle.Get()) {

		if (finishedStreamableHandle.Get()->GetLoadedAsset()) {

			if (auto colorSnippetDataAsset = Cast<UVertexPaintColorSnippetDataAsset>(finishedStreamableHandle.Get()->GetLoadedAsset())) {

				if (colorSnippetDataAsset->SnippetColorData.Contains(ColorSnippetID)) {

					if (colorSnippetDataAsset->SnippetColorData.FindRef(ColorSnippetID).ColorSnippetDataPerLOD.IsValidIndex(0)) {


						RVPDP_LOG(CalculateColorsInfo.TaskFundamentalSettings.DebugSettings, FColor::Cyan, "Successfully Loaded Color Snippet Data Asset: %s for Color Snippet: %s", *colorSnippetDataAsset->GetName(), *ColorSnippetID);


						if (LoadedColorSnippetDataAssetSettings == ELoadColorSnippetDataAssetOptions::LoadPaintColorSnippetDataAsset) {

							CalculateColorsInfo.PaintColorSnippetSettings.ColorSnippetDataAssetInfo.ColorSnippetDataAssetSnippetIsStoredIn = colorSnippetDataAsset;
						}

						else if (LoadedColorSnippetDataAssetSettings == ELoadColorSnippetDataAssetOptions::LoadCompareWithColorSnippetDataAsset) {

							LoadedCompareColorSnippetDataAssets.Add(ColorSnippetID, colorSnippetDataAsset);
						}
					}

					else {

						RVPDP_LOG(CalculateColorsInfo.TaskFundamentalSettings.DebugSettings, FColor::Red, "PaintColorSnippet Fail! - The Color Snippet Data Per LOD doesn't have a valid LOD0 index. ");
					}
				}

				else {

					RVPDP_LOG(CalculateColorsInfo.TaskFundamentalSettings.DebugSettings, FColor::Red, "PaintColorSnippet Fail! - The Color Snippet Data Asset: %s the Mesh have referenced to store their color snippet doesn't have the Snippet ID: %s Registered! Try removing the snippet from the Editor Widget and adding it again.", *colorSnippetDataAsset->GetName(), *ColorSnippetID);
				}
			}

			else {

				RVPDP_LOG(CalculateColorsInfo.TaskFundamentalSettings.DebugSettings, FColor::Red, "PaintColorSnippet Fail! - The Color Snippet Data Asset could not be loaded. If you have deleted it after storing snippets on it then recommend opening up the Editor Utility widget which will refresh and clear up if any mesh has references to old data assets ");
			}
		}

		else {

			RVPDP_LOG(CalculateColorsInfo.TaskFundamentalSettings.DebugSettings, FColor::Red, "PaintColorSnippet Fail! - The Color Snippet Data Asset could not be loaded. If you have deleted it after storing snippets on it then recommend opening up the Editor Utility widget which will refresh and clear up if any mesh has references to old data assets ");
		}
	}

	else {

		RVPDP_LOG(CalculateColorsInfo.TaskFundamentalSettings.DebugSettings, FColor::Red, "PaintColorSnippet Fail! - After Atting to Load the Color Snippet Data Asset the FStreamableHandle wasn't Valid! ");
	}


	// Continues with setting CompareWithColorsSnippetDataAssetInfo etc. when all callbacks have arrived
	if (LoadColorSnippetDataAssetsCallbacks < AmountOfExpectedFinishedLoadingCallbacks) return;


	LoadColorSnippetDataAssetsCallbacks = 0;


	if (LoadedColorSnippetDataAssetSettings == ELoadColorSnippetDataAssetOptions::LoadCompareWithColorSnippetDataAsset) {


		CalculateColorsInfo.TaskFundamentalSettings.CallbackSettings.CompareMeshVertexColorsToColorSnippets.CompareWithColorsSnippetDataAssetInfo.Empty();

		for (auto& loadedCompareSnippets : LoadedCompareColorSnippetDataAssets) {

			FRVPDPColorSnippetDataAssetInfo colorSnippetDataAssetInfo;
			colorSnippetDataAssetInfo.ColorSnippetID = loadedCompareSnippets.Key;
			colorSnippetDataAssetInfo.ColorSnippetDataAssetSnippetIsStoredIn = loadedCompareSnippets.Value;

			CalculateColorsInfo.TaskFundamentalSettings.CallbackSettings.CompareMeshVertexColorsToColorSnippets.CompareWithColorsSnippetDataAssetInfo.Add(colorSnippetDataAssetInfo);
		}

		LoadedCompareColorSnippetDataAssets.Empty();
	}


	// Checks if things are still valid after loading data asset
	switch (CalculateColorsInfo.VertexPaintDetectionType) {

	case EVertexPaintDetectionType::GetClosestVertexDataDetection:

		if (!GetClosestVertexDataOnMeshCheckValid(CalculateColorsInfo.GetClosestVertexDataSettings)) {

			if (LoadedColorSnippetDataAssetSettings == ELoadColorSnippetDataAssetOptions::LoadCompareWithColorSnippetDataAsset) {

				RVPDP_LOG(CalculateColorsInfo.TaskFundamentalSettings.DebugSettings, FColor::Red, "Check Valid Failed for GetClosestVertexData after Loading Compare Color Snippet Data Asset.");
			}

			GetClosestVertexDataOnMeshTaskFinished(CalculateColorsInfo.TaskResult, CalculateColorsInfo.GetClosestVertexDataSettings, FRVPDPClosestVertexDataResults(), FRVPDPEstimatedColorAtHitLocationInfo(), FRVPDPAverageColorInAreaInfo(), CalculateColorsInfo.GetClosestVertexDataSettings, AdditionalDataToPassThrough, true);

			return;
		}

		break;

	case EVertexPaintDetectionType::GetAllVertexColorDetection:

		if (!GetAllVertexColorsOnlyCheckValid(CalculateColorsInfo.GetAllVertexColorsSettings)) {

			if (LoadedColorSnippetDataAssetSettings == ELoadColorSnippetDataAssetOptions::LoadCompareWithColorSnippetDataAsset) {

				RVPDP_LOG(CalculateColorsInfo.TaskFundamentalSettings.DebugSettings, FColor::Red, "Check Valid Failed for GetAllVertexColorsOnly after Loading Compare Color Snippet Data Asset.");
			}

			GetAllVertexColorsOnlyTaskFinished(CalculateColorsInfo.TaskResult, CalculateColorsInfo.GetAllVertexColorsSettings, CalculateColorsInfo.TaskFundamentalSettings, AdditionalDataToPassThrough, true);
			return;
		}

		break;


	case EVertexPaintDetectionType::GetColorsWithinArea:

		if (!GetColorsWithinAreaCheckValid(CalculateColorsInfo.GetColorsWithinAreaSettings)) {

			if (LoadedColorSnippetDataAssetSettings == ELoadColorSnippetDataAssetOptions::LoadCompareWithColorSnippetDataAsset) {

				RVPDP_LOG(CalculateColorsInfo.TaskFundamentalSettings.DebugSettings, FColor::Red, "Check Valid Failed for GetColorsWithinArea after Loading Compare Color Snippet Data Asset.");
			}

			GetColorsWithinAreaTaskFinished(CalculateColorsInfo.TaskResult, CalculateColorsInfo.GetColorsWithinAreaSettings, CalculateColorsInfo.WithinArea_Results_BeforeApplyingColors, CalculateColorsInfo.TaskFundamentalSettings, AdditionalDataToPassThrough, true);
			return;
		}

		break;

	case EVertexPaintDetectionType::PaintAtLocation:

		if (!PaintOnMeshAtLocationCheckValid(CalculateColorsInfo.PaintAtLocationSettings)) {

			if (LoadedColorSnippetDataAssetSettings == ELoadColorSnippetDataAssetOptions::LoadCompareWithColorSnippetDataAsset) {

				RVPDP_LOG(CalculateColorsInfo.TaskFundamentalSettings.DebugSettings, FColor::Red, "Check Valid Failed for Paint at Location after Loading Compare Color Snippet Data Asset.");
			}

			// If fail and set to run detections then we expect fail callbacks on the get closest vertex data delegates as well
			if (CalculateColorsInfo.PaintAtLocationSettings.GetClosestVertexDataCombo.RunGetClosestVertexDataOnMeshBeforeApplyingPaint) {

				GetClosestVertexDataOnMeshTaskFinished(CalculateColorsInfo.TaskResult, CalculateColorsInfo.GetClosestVertexDataSettings, FRVPDPClosestVertexDataResults(), FRVPDPEstimatedColorAtHitLocationInfo(), FRVPDPAverageColorInAreaInfo(), CalculateColorsInfo.GetClosestVertexDataSettings, AdditionalDataToPassThrough, true);
			}

			PaintOnMeshAtLocationTaskFinished(CalculateColorsInfo.TaskResult, CalculateColorsInfo.PaintTaskResult, CalculateColorsInfo.PaintAtLocationSettings, CalculateColorsInfo.GetClosestVertexDataResult, CalculateColorsInfo.GetClosestVertexData_EstimatedColorAtHitLocationResult, CalculateColorsInfo.PaintAtLocation_AverageColorInArea, CalculateColorsInfo.TaskFundamentalSettings, AdditionalDataToPassThrough, true);


			return;
		}

		break;

	case EVertexPaintDetectionType::PaintWithinArea:

		if (!PaintOnMeshWithinAreaCheckValid(CalculateColorsInfo.PaintWithinAreaSettings)) {

			if (LoadedColorSnippetDataAssetSettings == ELoadColorSnippetDataAssetOptions::LoadCompareWithColorSnippetDataAsset) {

				RVPDP_LOG(CalculateColorsInfo.TaskFundamentalSettings.DebugSettings, FColor::Red, "Check Valid Failed for Paint Within Area after Loading Compare Color Snippet Data Asset.");
			}

			if (CalculateColorsInfo.PaintWithinAreaSettings.GetColorsWithinAreaCombo)
				GetColorsWithinAreaTaskFinished(CalculateColorsInfo.TaskResult, CalculateColorsInfo.GetColorsWithinAreaSettings, CalculateColorsInfo.WithinArea_Results_BeforeApplyingColors, CalculateColorsInfo.TaskFundamentalSettings, AdditionalDataToPassThrough, true);

			PaintOnMeshWithinAreaTaskFinished(CalculateColorsInfo.TaskResult, CalculateColorsInfo.PaintTaskResult, CalculateColorsInfo.PaintWithinAreaSettings, CalculateColorsInfo.WithinArea_Results_AfterApplyingColors, CalculateColorsInfo.TaskFundamentalSettings, AdditionalDataToPassThrough, true);
			return;
		}

		break;

	case EVertexPaintDetectionType::PaintEntireMesh:

		if (!PaintOnEntireMeshCheckValid(CalculateColorsInfo.PaintOnEntireMeshSettings)) {

			if (LoadedColorSnippetDataAssetSettings == ELoadColorSnippetDataAssetOptions::LoadCompareWithColorSnippetDataAsset) {

				RVPDP_LOG(CalculateColorsInfo.TaskFundamentalSettings.DebugSettings, FColor::Red, "Check Valid Failed for Paint Entire Mesh after Loading Compare Color Snippet Data Asset.");
			}

			PaintOnEntireMeshTaskFinished(CalculateColorsInfo.TaskResult, CalculateColorsInfo.PaintTaskResult, CalculateColorsInfo.PaintOnEntireMeshSettings, CalculateColorsInfo.TaskFundamentalSettings, AdditionalDataToPassThrough, true);
			return;
		}

		break;

	case EVertexPaintDetectionType::PaintColorSnippet:

		if (!PaintColorSnippetCheckValid(CalculateColorsInfo.PaintColorSnippetSettings)) {

			if (LoadedColorSnippetDataAssetSettings == ELoadColorSnippetDataAssetOptions::LoadPaintColorSnippetDataAsset) {

				RVPDP_LOG(CalculateColorsInfo.TaskFundamentalSettings.DebugSettings, FColor::Red, "Check Valid Failed for Paint Color Snippet after Loading Paint Color Snippet Data Asset!");
			}

			else if (LoadedColorSnippetDataAssetSettings == ELoadColorSnippetDataAssetOptions::LoadCompareWithColorSnippetDataAsset) {

				RVPDP_LOG(CalculateColorsInfo.TaskFundamentalSettings.DebugSettings, FColor::Red, "Check Valid Failed for Paint Color Snippet after Loading Compare Color Snippet Data Asset.");
			}

			PaintColorSnippetOnMeshTaskFinished(CalculateColorsInfo.TaskResult, CalculateColorsInfo.PaintTaskResult, CalculateColorsInfo.PaintColorSnippetSettings, CalculateColorsInfo.TaskFundamentalSettings, AdditionalDataToPassThrough, true);
			return;
		}

		break;

	case EVertexPaintDetectionType::SetMeshVertexColorsDirectly:

		if (!SetMeshComponentVertexColorsCheckValid(CalculateColorsInfo.SetVertexColorsSettings)) {

			if (LoadedColorSnippetDataAssetSettings == ELoadColorSnippetDataAssetOptions::LoadPaintColorSnippetDataAsset) {

				RVPDP_LOG(CalculateColorsInfo.TaskFundamentalSettings.DebugSettings, FColor::Red, "Check Valid Failed for Set Mesh Component Vertex Colors after Loading Paint Color Snippet Data Asset!");
			}

			else if (LoadedColorSnippetDataAssetSettings == ELoadColorSnippetDataAssetOptions::LoadCompareWithColorSnippetDataAsset) {

				RVPDP_LOG(CalculateColorsInfo.TaskFundamentalSettings.DebugSettings, FColor::Red, "Check Valid Failed for Set Mesh Component Vertex Colors after Loading Compare Color Snippet Data Asset.");
			}

			SetMeshComponentVertexColorsTaskFinished(CalculateColorsInfo.TaskResult, CalculateColorsInfo.PaintTaskResult, CalculateColorsInfo.SetVertexColorsSettings, CalculateColorsInfo.TaskFundamentalSettings, AdditionalDataToPassThrough, true);
			return;
		}

		break;

	case EVertexPaintDetectionType::SetMeshVertexColorsDirectlyUsingSerializedString:

		if (!SetMeshComponentVertexColorsUsingSerializedStringCheckValid(CalculateColorsInfo.SetVertexColorsUsingSerializedStringSettings)) {

			if (LoadedColorSnippetDataAssetSettings == ELoadColorSnippetDataAssetOptions::LoadPaintColorSnippetDataAsset) {

				RVPDP_LOG(CalculateColorsInfo.TaskFundamentalSettings.DebugSettings, FColor::Red, "Check Valid Failed for Set Mesh Component Vertex Colors Using Serialized String after Loading Paint Color Snippet Data Asset!");
			}

			else if (LoadedColorSnippetDataAssetSettings == ELoadColorSnippetDataAssetOptions::LoadCompareWithColorSnippetDataAsset) {

				RVPDP_LOG(CalculateColorsInfo.TaskFundamentalSettings.DebugSettings, FColor::Red, "Check Valid Failed for Set Mesh Component Vertex Colors Using Serialized String after Loading Compare Color Snippet Data Asset.");
			}

			SetMeshComponentVertexColorsUsingSerializedStringTaskFinished(CalculateColorsInfo.TaskResult, CalculateColorsInfo.PaintTaskResult, CalculateColorsInfo.SetVertexColorsUsingSerializedStringSettings, CalculateColorsInfo.TaskFundamentalSettings, AdditionalDataToPassThrough, true);
			return;
		}

		break;

	default:
		break;
	}


	WrapUpLoadedColorSnippetDataAsset(CalculateColorsInfo, AdditionalDataToPassThrough, LoadedColorSnippetDataAssetSettings);
}

void UVertexPaintDetectionComponent::WrapUpLoadedColorSnippetDataAsset(FRVPDPCalculateColorsInfo CalculateColorsInfo, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough, ELoadColorSnippetDataAssetOptions LoadColorSnippetDataAssetSetting) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_WrapUpLoadedColorSnippetDataAsset);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - WrapUpLoadedColorSnippetDataAsset");


	switch (LoadColorSnippetDataAssetSetting) {

	case ELoadColorSnippetDataAssetOptions::LoadCompareWithColorSnippetDataAsset:

		CalculateColorsInfo.PaintTaskSettings.CallbackSettings = CalculateColorsInfo.TaskFundamentalSettings.CallbackSettings;

		break;

	case ELoadColorSnippetDataAssetOptions::LoadPaintColorSnippetDataAsset:

		// Updates this as we've updated the struct with results
		CalculateColorsInfo.PaintColorSnippetSettings = CalculateColorsInfo.PaintColorSnippetSettings;
		CalculateColorsInfo.PaintTaskSettings = CalculateColorsInfo.PaintColorSnippetSettings;

		// If failed at getting the data asset which is crucial for the paint job then bails out
		if (!CalculateColorsInfo.PaintColorSnippetSettings.ColorSnippetDataAssetInfo.ColorSnippetDataAssetSnippetIsStoredIn) {
			
			PaintColorSnippetOnMeshTaskFinished(CalculateColorsInfo.TaskResult, CalculateColorsInfo.PaintTaskResult, CalculateColorsInfo.PaintColorSnippetSettings, CalculateColorsInfo.TaskFundamentalSettings, AdditionalDataToPassThrough, true);
			return;
		}

		if (DoesCompareColorSnippetInfoNeedToBeSet(CalculateColorsInfo, AdditionalDataToPassThrough)) {

			// If should Load Color Snippet for Compare Colors then we don't want to Add to Task Queue now but rather when this function runs again after the compare colors data asset has finished loading. 
			return;
		}

		break;

	default:
		break;
	}


	AddTaskToQueue(CalculateColorsInfo, AdditionalDataToPassThrough);
}


//-------------------------------------------------------

// Add Task To Queue

void UVertexPaintDetectionComponent::AddTaskToQueue(const FRVPDPCalculateColorsInfo& CalculateColorsInfo, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_AddTaskToQueue);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - AddTaskToQueue");


	if (DoesCompareColorSnippetInfoNeedToBeSet(CalculateColorsInfo, AdditionalDataToPassThrough)) {


		// If should Load Color Snippet for Compare Colors, then Loads the color snippet data assets in first, then Add the task to the queue when we can get the colors from the data asset when task is run.
		return;
	}

	else {

		if (UVertexPaintDetectionTaskQueue* taskQueue = UVertexPaintFunctionLibrary::GetVertexPaintTaskQueue(GetWorld())) {

			int createdTaskID = 0;
			bool canStartTaskImmediately = false;

			if (taskQueue->AddCalculateColorsTaskToQueue(CalculateColorsInfo, AdditionalDataToPassThrough, createdTaskID, canStartTaskImmediately)) {


				// If successfully added then caches the task on the component as well with the ID created. 
				switch (CalculateColorsInfo.VertexPaintDetectionType) {

				case EVertexPaintDetectionType::GetClosestVertexDataDetection:
					
					CurrentGetClosestVertexDataTasks.Add(createdTaskID, CalculateColorsInfo.GetVertexPaintComponent());
					break;

				case EVertexPaintDetectionType::GetAllVertexColorDetection:

					CurrentGetAllVertexColorsOnlyTasks.Add(createdTaskID, CalculateColorsInfo.GetVertexPaintComponent());
					break;

				case EVertexPaintDetectionType::GetColorsWithinArea:

					CurrentGetColorsWithinAreaTasks.Add(createdTaskID, CalculateColorsInfo.GetVertexPaintComponent());
					break;

				case EVertexPaintDetectionType::PaintAtLocation:

					CurrentPaintAtLocationTasks.Add(createdTaskID, CalculateColorsInfo.GetVertexPaintComponent());
					break;

				case EVertexPaintDetectionType::PaintWithinArea:

					CurrentPaintWithinAreaTasks.Add(createdTaskID, CalculateColorsInfo.GetVertexPaintComponent());
					break;

				case EVertexPaintDetectionType::PaintEntireMesh:

					CurrentPaintEntireMeshTasks.Add(createdTaskID, CalculateColorsInfo.GetVertexPaintComponent());
					break;

				case EVertexPaintDetectionType::PaintColorSnippet:

					CurrentPaintColorSnippetTasks.Add(createdTaskID, CalculateColorsInfo.GetVertexPaintComponent());
					break;

				case EVertexPaintDetectionType::SetMeshVertexColorsDirectly:

					CurrentSetMeshComponentVertexColorsTasks.Add(createdTaskID, CalculateColorsInfo.GetVertexPaintComponent());
					break;

				case EVertexPaintDetectionType::SetMeshVertexColorsDirectlyUsingSerializedString:

					CurrentSetMeshComponentVertexColorsUsingSerializedStringTasks.Add(createdTaskID, CalculateColorsInfo.GetVertexPaintComponent());
					break;

				default:
					break;
				}


				// Starts the actual tasks after caching it on the component if allowed, i.e. no other tasks for the mesh component is queued up. So if the task would fail immediately, we can still run callbacks etc. as intended.
				if (canStartTaskImmediately)
					taskQueue->StartCalculateColorsTask(createdTaskID);
			}

			else {

				RVPDP_LOG(CalculateColorsInfo.TaskFundamentalSettings.DebugSettings, FColor::Red, "Failed to Add Task for Mesh: %s", *CalculateColorsInfo.VertexPaintComponentName);
			}
		}
	}
}


//-------------------------------------------------------

// Get Current Tasks Initiated By Component

void UVertexPaintDetectionComponent::GetCurrentTasksInitiatedByComponent(int& TotalAmountOfTasks, int& AmountOfGetClosestVertexDataTasks, int& AmountOfGetAllVertexColorsOnlyTasks, int& AmountOfGetColorsWithinAreaTasks, int& AmountOfPaintAtLocationTasks, int& AmountOfPaintWithinAreaTasks, int& AmountOfPaintEntireMeshTasks, int& AmountOfPaintColorSnippetTasks, int& AmountOfSetMeshComponentVertexColorsTasks, int& AmountOfSetMeshComponentVertexColorsUsingSerializedStringTasks) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetCurrentTasksInitiatedByComponent);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - GetCurrentTasksInitiatedByComponent");

	TotalAmountOfTasks = CurrentGetClosestVertexDataTasks.Num() + CurrentGetAllVertexColorsOnlyTasks.Num() + CurrentGetColorsWithinAreaTasks.Num() + CurrentPaintAtLocationTasks.Num() + CurrentPaintWithinAreaTasks.Num() + CurrentPaintEntireMeshTasks.Num() + CurrentPaintColorSnippetTasks.Num() + CurrentSetMeshComponentVertexColorsTasks.Num() + CurrentSetMeshComponentVertexColorsUsingSerializedStringTasks.Num();

	AmountOfGetClosestVertexDataTasks = CurrentGetClosestVertexDataTasks.Num();
	AmountOfGetAllVertexColorsOnlyTasks = CurrentGetAllVertexColorsOnlyTasks.Num();
	AmountOfGetColorsWithinAreaTasks = CurrentGetColorsWithinAreaTasks.Num();
	AmountOfPaintAtLocationTasks = CurrentPaintAtLocationTasks.Num();
	AmountOfPaintWithinAreaTasks = CurrentPaintWithinAreaTasks.Num();
	AmountOfPaintEntireMeshTasks = CurrentPaintEntireMeshTasks.Num();
	AmountOfPaintColorSnippetTasks = CurrentPaintColorSnippetTasks.Num();
	AmountOfSetMeshComponentVertexColorsTasks = CurrentSetMeshComponentVertexColorsTasks.Num();
	AmountOfSetMeshComponentVertexColorsUsingSerializedStringTasks = CurrentSetMeshComponentVertexColorsUsingSerializedStringTasks.Num();
}


//-------------------------------------------------------

// Get Total Tasks Initiated By Component

int UVertexPaintDetectionComponent::GetTotalTasksInitiatedByComponent() {

	int totalAmountOfTasks;
	int amountOfGetClosestVertexDataTasks;
	int amountOfGetAllVertexColorsOnlyTasks;
	int amountOfGetColorsWithinAreaTasks;
	int amountOfPaintAtLocationTasks;
	int amountOfPaintWithinAreaTasks;
	int amountOfPaintEntireMeshTasks;
	int amountOfPaintColorSnippetTasks;
	int amountOfSetMeshComponentColorsTasks;
	int amountOfSetMeshComponentColorsUsingSerializedStringTasks;

	GetCurrentTasksInitiatedByComponent(totalAmountOfTasks, amountOfGetClosestVertexDataTasks, amountOfGetAllVertexColorsOnlyTasks, amountOfGetColorsWithinAreaTasks, amountOfPaintAtLocationTasks, amountOfPaintWithinAreaTasks, amountOfPaintEntireMeshTasks, amountOfPaintColorSnippetTasks, amountOfSetMeshComponentColorsTasks, amountOfSetMeshComponentColorsUsingSerializedStringTasks);

	return totalAmountOfTasks;
}


//-------------------------------------------------------

// Has Paint Task Queued Up On Mesh Component

bool UVertexPaintDetectionComponent::HasPaintTaskQueuedUpOnMeshComponent(UPrimitiveComponent* meshComponent) const {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_HasPaintTaskQueuedUpOnMeshComponent);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - HasPaintTaskQueuedUpOnMeshComponent");


	TArray<UPrimitiveComponent*> meshComponents;

	CurrentPaintAtLocationTasks.GenerateValueArray(meshComponents);
	if (meshComponents.Contains(meshComponent))
		return true;

	CurrentPaintWithinAreaTasks.GenerateValueArray(meshComponents);
	if (meshComponents.Contains(meshComponent))
		return true;

	CurrentPaintEntireMeshTasks.GenerateValueArray(meshComponents);
	if (meshComponents.Contains(meshComponent))
		return true;

	CurrentPaintColorSnippetTasks.GenerateValueArray(meshComponents);
	if (meshComponents.Contains(meshComponent))
		return true;

	CurrentSetMeshComponentVertexColorsTasks.GenerateValueArray(meshComponents);
	if (meshComponents.Contains(meshComponent))
		return true;

	CurrentSetMeshComponentVertexColorsUsingSerializedStringTasks.GenerateValueArray(meshComponents);
	if (meshComponents.Contains(meshComponent))
		return true;

	return false;
}


//-------------------------------------------------------

// Has Detect Task Queued Up On Mesh Component

bool UVertexPaintDetectionComponent::HasDetectTaskQueuedUpOnMeshComponent(UPrimitiveComponent* meshComponent) const {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_HasDetectTaskQueuedUpOnMeshComponent);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - HasDetectTaskQueuedUpOnMeshComponent");


	TArray<UPrimitiveComponent*> meshComponents;

	CurrentGetClosestVertexDataTasks.GenerateValueArray(meshComponents);
	if (meshComponents.Contains(meshComponent))
		return true;

	CurrentGetAllVertexColorsOnlyTasks.GenerateValueArray(meshComponents);
	if (meshComponents.Contains(meshComponent))
		return true;

	CurrentGetColorsWithinAreaTasks.GenerateValueArray(meshComponents);
	if (meshComponents.Contains(meshComponent))
		return true;

	return false;
}


//-------------------------------------------------------

// Get Current Tasks Mesh Components

TArray<UPrimitiveComponent*> UVertexPaintDetectionComponent::GetCurrentTasksMeshComponents() {

	TArray<UPrimitiveComponent*> totalMeshComponents;
	TArray<UPrimitiveComponent*> tasksMeshComponents;


	if (CurrentGetClosestVertexDataTasks.Num() > 0) {

		CurrentGetClosestVertexDataTasks.GenerateValueArray(tasksMeshComponents);
		totalMeshComponents.Append(tasksMeshComponents);
	}


	if (CurrentGetAllVertexColorsOnlyTasks.Num() > 0) {

		CurrentGetAllVertexColorsOnlyTasks.GenerateValueArray(tasksMeshComponents);
		totalMeshComponents.Append(tasksMeshComponents);
	}


	if (CurrentGetColorsWithinAreaTasks.Num() > 0) {

		CurrentGetColorsWithinAreaTasks.GenerateValueArray(tasksMeshComponents);
		totalMeshComponents.Append(tasksMeshComponents);
	}


	if (CurrentGetColorsWithinAreaTasks.Num() > 0) {

		CurrentGetColorsWithinAreaTasks.GenerateValueArray(tasksMeshComponents);
		totalMeshComponents.Append(tasksMeshComponents);
	}


	if (CurrentPaintAtLocationTasks.Num() > 0) {

		CurrentPaintAtLocationTasks.GenerateValueArray(tasksMeshComponents);
		totalMeshComponents.Append(tasksMeshComponents);
	}


	if (CurrentPaintWithinAreaTasks.Num() > 0) {

		CurrentPaintWithinAreaTasks.GenerateValueArray(tasksMeshComponents);
		totalMeshComponents.Append(tasksMeshComponents);
	}


	if (CurrentPaintEntireMeshTasks.Num() > 0) {

		CurrentPaintEntireMeshTasks.GenerateValueArray(tasksMeshComponents);
		totalMeshComponents.Append(tasksMeshComponents);
	}


	if (CurrentPaintColorSnippetTasks.Num() > 0) {

		CurrentPaintColorSnippetTasks.GenerateValueArray(tasksMeshComponents);
		totalMeshComponents.Append(tasksMeshComponents);
	}


	if (CurrentSetMeshComponentVertexColorsTasks.Num() > 0) {

		CurrentSetMeshComponentVertexColorsTasks.GenerateValueArray(tasksMeshComponents);
		totalMeshComponents.Append(tasksMeshComponents);
	}


	if (CurrentSetMeshComponentVertexColorsUsingSerializedStringTasks.Num() > 0) {

		CurrentSetMeshComponentVertexColorsUsingSerializedStringTasks.GenerateValueArray(tasksMeshComponents);
		totalMeshComponents.Append(tasksMeshComponents);
	}


	return totalMeshComponents;
}


//-------------------------------------------------------

// Get All Physics Surfaces To Apply As String

FString UVertexPaintDetectionComponent::GetAllPhysicsSurfacesToApplyAsString(const FRVPDPApplyColorSetting& ColorSettings) const {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetAllPhysicsSurfacesToApplyAsString);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - GetAllPhysicsSurfacesToApplyAsString");


	FString physicsSurfaceToApplyTotal = "";
	TArray<FRVPDPPhysicsSurfaceToApplySettings> physicsSurfacesToApply = ColorSettings.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.PhysicalSurfacesToApply;

	for (int i = 0; i < physicsSurfacesToApply.Num(); i++) {

		physicsSurfaceToApplyTotal += *StaticEnum<EPhysicalSurface>()->GetDisplayNameTextByValue(physicsSurfacesToApply[i].SurfaceToApply.GetValue()).ToString();
		physicsSurfaceToApplyTotal += " - Strength: ";
		physicsSurfaceToApplyTotal += FString::SanitizeFloat(physicsSurfacesToApply[i].StrengthToApply);

		// If not last index
		if (i + 1 != physicsSurfacesToApply.Num())
			physicsSurfaceToApplyTotal += ", ";
	}

	return physicsSurfaceToApplyTotal;
}


//-------------------------------------------------------

// Check If Registered And Initialized

void UVertexPaintDetectionComponent::CheckIfRegisteredAndInitialized() {

	if (!UVertexPaintFunctionLibrary::IsWorldValid(GetWorld())) return;

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_CheckIfRegisteredAndInitialized);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - CheckIfRegisteredAndInitialized");


	// Necessary in case you Add the Component in Runtime in BP, then immediately call Paint Jobs. Otherwise you would have to use a Delay which is undesirable. 
	if (!IsRegistered() || !HasBeenInitialized() || !IsActive() || !IsValid(this)) {

		if (!IsRegistered())
			RegisterComponent();

		if (!HasBeenInitialized())
			InitializeComponent();

		if (!IsActive())
			SetActive(true);
	}
}


//-------------------------------------------------------

// Clamp Paint Settings

void UVertexPaintDetectionComponent::ClampPaintSettings(FRVPDPApplyColorSettings& ApplyVertexColorSettings) const {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_ClampPaintSettings);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - ClampPaintSettings");


	// Clamps it so if the user sets like 0.000001, we will paint at the lowest amount of 1 when converted to FColor which range from 0-255
	ApplyVertexColorSettings.ApplyColorsOnRedChannel.AmountToApply = GetClampedPaintStrength(ApplyVertexColorSettings.ApplyColorsOnRedChannel.AmountToApply);
	ApplyVertexColorSettings.ApplyColorsOnGreenChannel.AmountToApply = GetClampedPaintStrength(ApplyVertexColorSettings.ApplyColorsOnGreenChannel.AmountToApply);
	ApplyVertexColorSettings.ApplyColorsOnBlueChannel.AmountToApply = GetClampedPaintStrength(ApplyVertexColorSettings.ApplyColorsOnBlueChannel.AmountToApply);
	ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.AmountToApply = GetClampedPaintStrength(ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.AmountToApply);

	if (ApplyVertexColorSettings.AdjustPaintStrengthToDeltaTimeSettings.AdjustPaintStrengthsToDeltaTime) {

		ApplyVertexColorSettings.ApplyColorsOnRedChannel.AmountToApply = ApplyVertexColorSettings.ApplyColorsOnRedChannel.AmountToApply * ApplyVertexColorSettings.AdjustPaintStrengthToDeltaTimeSettings.DeltaTimeToAdjustTo * ApplyVertexColorSettings.AdjustPaintStrengthToDeltaTimeSettings.TargetFPS;

		ApplyVertexColorSettings.ApplyColorsOnGreenChannel.AmountToApply = ApplyVertexColorSettings.ApplyColorsOnGreenChannel.AmountToApply * ApplyVertexColorSettings.AdjustPaintStrengthToDeltaTimeSettings.DeltaTimeToAdjustTo * ApplyVertexColorSettings.AdjustPaintStrengthToDeltaTimeSettings.TargetFPS;

		ApplyVertexColorSettings.ApplyColorsOnBlueChannel.AmountToApply = ApplyVertexColorSettings.ApplyColorsOnBlueChannel.AmountToApply * ApplyVertexColorSettings.AdjustPaintStrengthToDeltaTimeSettings.DeltaTimeToAdjustTo * ApplyVertexColorSettings.AdjustPaintStrengthToDeltaTimeSettings.TargetFPS;

		ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.AmountToApply = ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.AmountToApply * ApplyVertexColorSettings.AdjustPaintStrengthToDeltaTimeSettings.DeltaTimeToAdjustTo * ApplyVertexColorSettings.AdjustPaintStrengthToDeltaTimeSettings.TargetFPS;
	}


	for (int i = 0; i < ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.PhysicalSurfacesToApply.Num(); i++) {

		ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.PhysicalSurfacesToApply[i].StrengthToApply = GetClampedPaintStrength(ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.PhysicalSurfacesToApply[i].StrengthToApply);


		if (ApplyVertexColorSettings.AdjustPaintStrengthToDeltaTimeSettings.AdjustPaintStrengthsToDeltaTime)
			ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.PhysicalSurfacesToApply[i].StrengthToApply = ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.PhysicalSurfacesToApply[i].StrengthToApply * ApplyVertexColorSettings.AdjustPaintStrengthToDeltaTimeSettings.DeltaTimeToAdjustTo * ApplyVertexColorSettings.AdjustPaintStrengthToDeltaTimeSettings.TargetFPS;

		ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.PhysicalSurfacesToApply[i].SurfacePaintLimit.PaintLimit = UKismetMathLibrary::FClamp(ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.PhysicalSurfacesToApply[i].SurfacePaintLimit.PaintLimit, 0, 1);
	}

	ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.StrengtOnChannelsWithoutThePhysicsSurfaces = GetClampedPaintStrength(ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.StrengtOnChannelsWithoutThePhysicsSurfaces);

	ClampPaintLimits(ApplyVertexColorSettings.ApplyColorsOnRedChannel.PaintLimit, ApplyVertexColorSettings.ApplyColorsOnGreenChannel.PaintLimit, ApplyVertexColorSettings.ApplyColorsOnBlueChannel.PaintLimit, ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.PaintLimit);
}


//-------------------------------------------------------

// Clamp Paint Limits

void UVertexPaintDetectionComponent::ClampPaintLimits(FRVPDPPaintLimitSettings& RedChannelLimit, FRVPDPPaintLimitSettings& BlueChannelLimit, FRVPDPPaintLimitSettings& GreenChannelLimit, FRVPDPPaintLimitSettings& AlphaChannelLimit) const {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_ClampPaintLimits);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - ClampPaintLimits");

	RedChannelLimit.PaintLimit = UKismetMathLibrary::FClamp(RedChannelLimit.PaintLimit, 0, 1);
	BlueChannelLimit.PaintLimit = UKismetMathLibrary::FClamp(BlueChannelLimit.PaintLimit, 0, 1);
	GreenChannelLimit.PaintLimit = UKismetMathLibrary::FClamp(GreenChannelLimit.PaintLimit, 0, 1);
	AlphaChannelLimit.PaintLimit = UKismetMathLibrary::FClamp(AlphaChannelLimit.PaintLimit, 0, 1);
}


//-------------------------------------------------------

// Get Clamped Paint Strength

float UVertexPaintDetectionComponent::GetClampedPaintStrength(float PaintStrength) const {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetClampedPaintStrength);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - GetClampedPaintStrength");


	if (PaintStrength > 0)
		PaintStrength = UKismetMathLibrary::FClamp(PaintStrength, 0.005, 1);

	else if (PaintStrength < 0)
		PaintStrength = UKismetMathLibrary::FClamp(PaintStrength, -1, -0.005);

	return PaintStrength;
}


//-------------------------------------------------------

// Resolve Paint Conditions

void UVertexPaintDetectionComponent::ResolvePaintConditions(FRVPDPApplyColorSetting& PaintColorSettings) const {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_ResolvePaintConditions);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - ResolvePaintConditions");

	if (!IsValid(PaintColorSettings.MeshComponent)) return;


	TArray<FRVPDPIsVertexNormalWithinDotToDirectionPaintConditionSettings> isVertexNormalWithinDotToDirection;
	TArray<FRVPDPIsVertexDirectionTowardLocationPaintConditionSettings> onlyAffectWithDirToLoc;
	TArray<FRVPDPIsVertexWithinDirectionFromLocationPaintConditionSettings> onlyAffectWithinDirection;
	TArray<FRVPDPIsVertexAboveOrBelowWorldZPaintConditionSettings> ifAboveOrBelowWorldZ;
	TArray<FRVPDPIsVertexColorWithinColorRangePaintConditionSettings> isWithinColorRange;
	TArray<FRVPDPIsVertexOnBonePaintConditionSettings> isBone;
	TArray<FRVPDPIsVertexOnMaterialPaintConditionSettings> isOnMaterial;
	TArray<FRVPDPDoesVertexHasLineOfSightPaintConditionSettings> lineOfSight;


	TMap<int, TArray<FRVPDPIsVertexNormalWithinDotToDirectionPaintConditionSettings>> physicalSurfaceToApply_isVertexNormalWithinDotToNormal;
	TMap<int, TArray<FRVPDPIsVertexDirectionTowardLocationPaintConditionSettings>> physicalSurfaceToApply_onlyAffectWithDirToLoc;
	TMap<int, TArray<FRVPDPIsVertexWithinDirectionFromLocationPaintConditionSettings>> physicalSurfaceToApply_onlyAffectWithinDirection;
	TMap<int, TArray<FRVPDPIsVertexAboveOrBelowWorldZPaintConditionSettings>> physicalSurfaceToApply_ifAboveOrBelowWorldZ;
	TMap<int, TArray<FRVPDPIsVertexColorWithinColorRangePaintConditionSettings>> physicalSurfaceToApply_isWithinColorRange;
	TMap<int, TArray<FRVPDPIsVertexOnBonePaintConditionSettings>> physicalSurfaceToApply_isBone;
	TMap<int, TArray<FRVPDPIsVertexOnMaterialPaintConditionSettings>> physicalSurfaceToApply_IsOnMaterial;
	TMap<int, TArray<FRVPDPDoesVertexHasLineOfSightPaintConditionSettings>> physicalSurfaceToApply_lineOfSight;

	
	for (int i = 0; i < 5; i++) {

		switch (i) {

		case 0:


			if (PaintColorSettings.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.ApplyVertexColorUsingPhysicsSurface && PaintColorSettings.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.PhysicalSurfacesToApply.Num() > 0) {

				for (int j = 0; j < PaintColorSettings.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.PhysicalSurfacesToApply.Num(); j++) {


					FRVPDPPaintConditionSettings paintConditions = PaintColorSettings.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.PhysicalSurfacesToApply[j].PaintConditions;

					if (paintConditions.IsVertexNormalWithinDotToDirection.Num() > 0)
						physicalSurfaceToApply_isVertexNormalWithinDotToNormal.Add(j, paintConditions.IsVertexNormalWithinDotToDirection);

					if (paintConditions.OnlyAffectVerticesWithDirectionToActorOrLocation.Num() > 0)
						physicalSurfaceToApply_onlyAffectWithDirToLoc.Add(j, paintConditions.OnlyAffectVerticesWithDirectionToActorOrLocation);

					if (paintConditions.OnlyAffectVerticesWithinDirectionFromActorOrLocation.Num() > 0)
						physicalSurfaceToApply_onlyAffectWithinDirection.Add(j, paintConditions.OnlyAffectVerticesWithinDirectionFromActorOrLocation);

					if (paintConditions.IfVertexIsAboveOrBelowWorldZ.Num() > 0)
						physicalSurfaceToApply_ifAboveOrBelowWorldZ.Add(j, paintConditions.IfVertexIsAboveOrBelowWorldZ);

					if (paintConditions.IfVertexColorIsWithinRange.Num() > 0)
						physicalSurfaceToApply_isWithinColorRange.Add(j, paintConditions.IfVertexColorIsWithinRange);

					if (paintConditions.IfVertexIsOnBone.Num() > 0)
						physicalSurfaceToApply_isBone.Add(j, paintConditions.IfVertexIsOnBone);

					if (paintConditions.IfVertexIsOnMaterial.Num() > 0)
						physicalSurfaceToApply_IsOnMaterial.Add(j, paintConditions.IfVertexIsOnMaterial);

					if (paintConditions.IfVertexHasLineOfSightTo.Num() > 0)
						physicalSurfaceToApply_lineOfSight.Add(j, paintConditions.IfVertexHasLineOfSightTo);
				}

				if (physicalSurfaceToApply_isVertexNormalWithinDotToNormal.Num() == 0 && physicalSurfaceToApply_onlyAffectWithDirToLoc.Num() == 0 && physicalSurfaceToApply_onlyAffectWithinDirection.Num() == 0 && physicalSurfaceToApply_ifAboveOrBelowWorldZ.Num() == 0 && physicalSurfaceToApply_isWithinColorRange.Num() == 0 && physicalSurfaceToApply_isBone.Num() == 0 && physicalSurfaceToApply_IsOnMaterial.Num() == 0 && physicalSurfaceToApply_lineOfSight.Num() == 0)
					continue;
			}

			else {

				continue;
			}
			break;

		case 1:

			isVertexNormalWithinDotToDirection = PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnRedChannel.PaintConditions.IsVertexNormalWithinDotToDirection;
			onlyAffectWithDirToLoc = PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnRedChannel.PaintConditions.OnlyAffectVerticesWithDirectionToActorOrLocation;
			onlyAffectWithinDirection = PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnRedChannel.PaintConditions.OnlyAffectVerticesWithinDirectionFromActorOrLocation;
			ifAboveOrBelowWorldZ = PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnRedChannel.PaintConditions.IfVertexIsAboveOrBelowWorldZ;
			isWithinColorRange = PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnRedChannel.PaintConditions.IfVertexColorIsWithinRange;
			isBone = PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnRedChannel.PaintConditions.IfVertexIsOnBone;
			isOnMaterial = PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnRedChannel.PaintConditions.IfVertexIsOnMaterial;
			lineOfSight = PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnRedChannel.PaintConditions.IfVertexHasLineOfSightTo;
			break;

		case 2:

			isVertexNormalWithinDotToDirection = PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.PaintConditions.IsVertexNormalWithinDotToDirection;
			onlyAffectWithDirToLoc = PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.PaintConditions.OnlyAffectVerticesWithDirectionToActorOrLocation;
			onlyAffectWithinDirection = PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.PaintConditions.OnlyAffectVerticesWithinDirectionFromActorOrLocation;
			ifAboveOrBelowWorldZ = PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.PaintConditions.IfVertexIsAboveOrBelowWorldZ;
			isWithinColorRange = PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.PaintConditions.IfVertexColorIsWithinRange;
			isBone = PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.PaintConditions.IfVertexIsOnBone;
			isOnMaterial = PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.PaintConditions.IfVertexIsOnMaterial;
			lineOfSight = PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.PaintConditions.IfVertexHasLineOfSightTo;
			break;

		case 3:

			isVertexNormalWithinDotToDirection = PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.PaintConditions.IsVertexNormalWithinDotToDirection;
			onlyAffectWithDirToLoc = PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.PaintConditions.OnlyAffectVerticesWithDirectionToActorOrLocation;
			onlyAffectWithinDirection = PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.PaintConditions.OnlyAffectVerticesWithinDirectionFromActorOrLocation;
			ifAboveOrBelowWorldZ = PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.PaintConditions.IfVertexIsAboveOrBelowWorldZ;
			isWithinColorRange = PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.PaintConditions.IfVertexColorIsWithinRange;
			isBone = PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.PaintConditions.IfVertexIsOnBone;
			isOnMaterial = PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.PaintConditions.IfVertexIsOnMaterial;
			lineOfSight = PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.PaintConditions.IfVertexHasLineOfSightTo;
			break;

		case 4:

			isVertexNormalWithinDotToDirection = PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.PaintConditions.IsVertexNormalWithinDotToDirection;
			onlyAffectWithDirToLoc = PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.PaintConditions.OnlyAffectVerticesWithDirectionToActorOrLocation;
			onlyAffectWithinDirection = PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.PaintConditions.OnlyAffectVerticesWithinDirectionFromActorOrLocation;
			ifAboveOrBelowWorldZ = PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.PaintConditions.IfVertexIsAboveOrBelowWorldZ;
			isWithinColorRange = PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.PaintConditions.IfVertexColorIsWithinRange;
			isBone = PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.PaintConditions.IfVertexIsOnBone;
			isOnMaterial = PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.PaintConditions.IfVertexIsOnMaterial;
			lineOfSight = PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.PaintConditions.IfVertexHasLineOfSightTo;
			break;

		default:
			break;
		}



		int amountOfSetsOfPaintConditions = 1;

		// Physics Surface can apply several with different paint conditions for each
		if (PaintColorSettings.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.ApplyVertexColorUsingPhysicsSurface)
			amountOfSetsOfPaintConditions = PaintColorSettings.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.PhysicalSurfacesToApply.Num();


		for (int j = 0; j < amountOfSetsOfPaintConditions; j++) {


			if (PaintColorSettings.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.ApplyVertexColorUsingPhysicsSurface) {

				isVertexNormalWithinDotToDirection = physicalSurfaceToApply_isVertexNormalWithinDotToNormal.FindRef(j);
				onlyAffectWithDirToLoc = physicalSurfaceToApply_onlyAffectWithDirToLoc.FindRef(j);
				onlyAffectWithinDirection = physicalSurfaceToApply_onlyAffectWithinDirection.FindRef(j);
				ifAboveOrBelowWorldZ = physicalSurfaceToApply_ifAboveOrBelowWorldZ.FindRef(j);
				isWithinColorRange = physicalSurfaceToApply_isWithinColorRange.FindRef(j);
				isBone = physicalSurfaceToApply_isBone.FindRef(j);
				isOnMaterial = physicalSurfaceToApply_IsOnMaterial.FindRef(j);
				lineOfSight = physicalSurfaceToApply_lineOfSight.FindRef(j);
			}


			for (int k = 0; k < isVertexNormalWithinDotToDirection.Num(); k++) {

				isVertexNormalWithinDotToDirection[k].ColorStrengthIfThisConditionIsNotMet = GetClampedPaintStrength(isVertexNormalWithinDotToDirection[k].ColorStrengthIfThisConditionIsNotMet);

				if (IsValid(isVertexNormalWithinDotToDirection[k].GetDirectionFromActorRotation))
					isVertexNormalWithinDotToDirection[k].DirectionToCheckAgainst = isVertexNormalWithinDotToDirection[k].GetDirectionFromActorRotation->GetActorRotation().Vector();

				isVertexNormalWithinDotToDirection[k].MinDotProductToDirectionRequired = UKismetMathLibrary::FClamp(isVertexNormalWithinDotToDirection[k].MinDotProductToDirectionRequired, -1, 1);
			}

			for (int k = 0; k < onlyAffectWithDirToLoc.Num(); k++) {

				onlyAffectWithDirToLoc[k].ColorStrengthIfThisConditionIsNotMet = GetClampedPaintStrength(onlyAffectWithDirToLoc[k].ColorStrengthIfThisConditionIsNotMet);

				if (IsValid(onlyAffectWithDirToLoc[k].Actor))
					onlyAffectWithDirToLoc[k].Location = onlyAffectWithDirToLoc[k].Actor->GetActorLocation();

				onlyAffectWithDirToLoc[k].MinDotProductToActorOrLocation = UKismetMathLibrary::FClamp(onlyAffectWithDirToLoc[k].MinDotProductToActorOrLocation, -1, 1);
			}

			for (int k = 0; k < onlyAffectWithinDirection.Num(); k++) {

				onlyAffectWithinDirection[k].ColorStrengthIfThisConditionIsNotMet = GetClampedPaintStrength(onlyAffectWithinDirection[k].ColorStrengthIfThisConditionIsNotMet);

				if (IsValid(onlyAffectWithinDirection[k].Actor))
					onlyAffectWithinDirection[k].Location = onlyAffectWithinDirection[k].Actor->GetActorLocation();

				onlyAffectWithinDirection[k].MinDotProductToDirection = UKismetMathLibrary::FClamp(onlyAffectWithinDirection[k].MinDotProductToDirection, -1, 1);
			}

			for (int k = 0; k < ifAboveOrBelowWorldZ.Num(); k++) {

				ifAboveOrBelowWorldZ[k].ColorStrengthIfThisConditionIsNotMet = GetClampedPaintStrength(ifAboveOrBelowWorldZ[k].ColorStrengthIfThisConditionIsNotMet);
			}

			for (int k = 0; k < isWithinColorRange.Num(); k++) {

				isWithinColorRange[k].ColorStrengthIfThisConditionIsNotMet = GetClampedPaintStrength(isWithinColorRange[k].ColorStrengthIfThisConditionIsNotMet);
				isWithinColorRange[k].IfCurrentVertexColorValueIsHigherOrEqualThan = GetClampedPaintStrength(isWithinColorRange[k].IfCurrentVertexColorValueIsHigherOrEqualThan);
				isWithinColorRange[k].IfCurrentVertexColorValueIsLessOrEqualThan = GetClampedPaintStrength(isWithinColorRange[k].IfCurrentVertexColorValueIsLessOrEqualThan);

				isWithinColorRange[k].IfHigherOrEqualThanInt = static_cast<uint8>(UKismetMathLibrary::MapRangeClamped(isWithinColorRange[k].IfCurrentVertexColorValueIsHigherOrEqualThan, 0, 1, 0, 255));

				isWithinColorRange[k].IfLessOrEqualThanInt = static_cast<uint8>(UKismetMathLibrary::MapRangeClamped(isWithinColorRange[k].IfCurrentVertexColorValueIsLessOrEqualThan, 0, 1, 0, 255));
			}

			for (int k = 0; k < isBone.Num(); k++) {

				isBone[k].ColorStrengthIfThisConditionIsNotMet = GetClampedPaintStrength(isBone[k].ColorStrengthIfThisConditionIsNotMet);

				isBone[k].MinBoneWeight = UKismetMathLibrary::FClamp(isBone[k].MinBoneWeight, 0, 1);
			}

			for (int k = isOnMaterial.Num()-1; k >= 0; k--) {

				// If added an null element then we clear them out. 
				if (!isOnMaterial[k].IfVertexIsOnMaterial) {

					isOnMaterial.RemoveAt(k);
					continue;
				}

				// Caches the Material Parent as well, if it's an instance, so if we don't match the material the vertex has outright, we can check if they're parents match. By caching we don't have to do this every vertex but just once for this one at least. 
				UMaterialInterface* ifVertexIsOnMaterialParent = isOnMaterial[k].IfVertexIsOnMaterial;
				if (UMaterialInstanceDynamic* materialInstanceDynamic = Cast<UMaterialInstanceDynamic>(isOnMaterial[k].IfVertexIsOnMaterial)) {

					if (UMaterialInstance* materialInstance = Cast<UMaterialInstance>(materialInstanceDynamic->Parent))
						ifVertexIsOnMaterialParent = materialInstance->Parent;
					else
						ifVertexIsOnMaterialParent = materialInstanceDynamic->Parent;
				}

				else if (UMaterialInstance* materialInstance = Cast<UMaterialInstance>(isOnMaterial[k].IfVertexIsOnMaterial)) {

					ifVertexIsOnMaterialParent = materialInstance->Parent;
				}

				isOnMaterial[k].IfVertexIsOnMaterialParent = ifVertexIsOnMaterialParent;
			}

			for (int k = 0; k < lineOfSight.Num(); k++) {

				lineOfSight[k].ColorStrengthIfThisConditionIsNotMet = GetClampedPaintStrength(lineOfSight[k].ColorStrengthIfThisConditionIsNotMet);

				if (IsValid(lineOfSight[k].IfVertexHasLineOfSightToActor))
					lineOfSight[k].IfVertexHasLineOfSightToPosition = lineOfSight[k].IfVertexHasLineOfSightToActor->GetActorLocation();
			}


			if (PaintColorSettings.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.ApplyVertexColorUsingPhysicsSurface) {

				physicalSurfaceToApply_isVertexNormalWithinDotToNormal.Add(j, isVertexNormalWithinDotToDirection);
				physicalSurfaceToApply_onlyAffectWithDirToLoc.Add(j, onlyAffectWithDirToLoc);
				physicalSurfaceToApply_onlyAffectWithinDirection.Add(j, onlyAffectWithinDirection);
				physicalSurfaceToApply_ifAboveOrBelowWorldZ.Add(j, ifAboveOrBelowWorldZ);
				physicalSurfaceToApply_isWithinColorRange.Add(j, isWithinColorRange);
				physicalSurfaceToApply_isBone.Add(j, isBone);
				physicalSurfaceToApply_IsOnMaterial.Add(j, isOnMaterial);
				physicalSurfaceToApply_lineOfSight.Add(j, lineOfSight);
			}
		}



		// Updates PaintColorSettings with the clamped settings and resolved Actor locations

		switch (i) {

		case 0:


			if (PaintColorSettings.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.ApplyVertexColorUsingPhysicsSurface) {

				for (int j = 0; j < PaintColorSettings.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.PhysicalSurfacesToApply.Num(); j++) {


					PaintColorSettings.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.PhysicalSurfacesToApply[j].PaintConditions.IsVertexNormalWithinDotToDirection = physicalSurfaceToApply_isVertexNormalWithinDotToNormal.FindRef(j);

					PaintColorSettings.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.PhysicalSurfacesToApply[j].PaintConditions.OnlyAffectVerticesWithDirectionToActorOrLocation = physicalSurfaceToApply_onlyAffectWithDirToLoc.FindRef(j);

					PaintColorSettings.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.PhysicalSurfacesToApply[j].PaintConditions.OnlyAffectVerticesWithinDirectionFromActorOrLocation = physicalSurfaceToApply_onlyAffectWithinDirection.FindRef(j);

					PaintColorSettings.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.PhysicalSurfacesToApply[j].PaintConditions.IfVertexIsAboveOrBelowWorldZ = physicalSurfaceToApply_ifAboveOrBelowWorldZ.FindRef(j);

					PaintColorSettings.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.PhysicalSurfacesToApply[j].PaintConditions.IfVertexColorIsWithinRange = physicalSurfaceToApply_isWithinColorRange.FindRef(j);

					PaintColorSettings.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.PhysicalSurfacesToApply[j].PaintConditions.IfVertexIsOnBone = physicalSurfaceToApply_isBone.FindRef(j);

					PaintColorSettings.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.PhysicalSurfacesToApply[j].PaintConditions.IfVertexIsOnMaterial = physicalSurfaceToApply_IsOnMaterial.FindRef(j);

					PaintColorSettings.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.PhysicalSurfacesToApply[j].PaintConditions.IfVertexHasLineOfSightTo = physicalSurfaceToApply_lineOfSight.FindRef(j);
				}
			}
			break;

		case 1:

			PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnRedChannel.PaintConditions.IsVertexNormalWithinDotToDirection = isVertexNormalWithinDotToDirection;
			PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnRedChannel.PaintConditions.OnlyAffectVerticesWithDirectionToActorOrLocation = onlyAffectWithDirToLoc;
			PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnRedChannel.PaintConditions.OnlyAffectVerticesWithinDirectionFromActorOrLocation = onlyAffectWithinDirection;
			PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnRedChannel.PaintConditions.IfVertexIsAboveOrBelowWorldZ = ifAboveOrBelowWorldZ;
			PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnRedChannel.PaintConditions.IfVertexColorIsWithinRange = isWithinColorRange;
			PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnRedChannel.PaintConditions.IfVertexIsOnBone = isBone;
			PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnRedChannel.PaintConditions.IfVertexIsOnMaterial = isOnMaterial;
			PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnRedChannel.PaintConditions.IfVertexHasLineOfSightTo = lineOfSight;
			break;

		case 2:

			PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.PaintConditions.IsVertexNormalWithinDotToDirection = isVertexNormalWithinDotToDirection;
			PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.PaintConditions.OnlyAffectVerticesWithDirectionToActorOrLocation = onlyAffectWithDirToLoc;
			PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.PaintConditions.OnlyAffectVerticesWithinDirectionFromActorOrLocation = onlyAffectWithinDirection;
			PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.PaintConditions.IfVertexIsAboveOrBelowWorldZ = ifAboveOrBelowWorldZ;
			PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.PaintConditions.IfVertexColorIsWithinRange = isWithinColorRange;
			PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.PaintConditions.IfVertexIsOnBone = isBone;
			PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.PaintConditions.IfVertexIsOnMaterial = isOnMaterial;
			PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.PaintConditions.IfVertexHasLineOfSightTo = lineOfSight;
			break;

		case 3:

			PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.PaintConditions.IsVertexNormalWithinDotToDirection = isVertexNormalWithinDotToDirection;
			PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.PaintConditions.OnlyAffectVerticesWithDirectionToActorOrLocation = onlyAffectWithDirToLoc;
			PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.PaintConditions.OnlyAffectVerticesWithinDirectionFromActorOrLocation = onlyAffectWithinDirection;
			PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.PaintConditions.IfVertexIsAboveOrBelowWorldZ = ifAboveOrBelowWorldZ;
			PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.PaintConditions.IfVertexColorIsWithinRange = isWithinColorRange;
			PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.PaintConditions.IfVertexIsOnBone = isBone;
			PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.PaintConditions.IfVertexIsOnMaterial = isOnMaterial;
			PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.PaintConditions.IfVertexHasLineOfSightTo = lineOfSight;
			break;

		case 4:

			PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.PaintConditions.IsVertexNormalWithinDotToDirection = isVertexNormalWithinDotToDirection;
			PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.PaintConditions.OnlyAffectVerticesWithDirectionToActorOrLocation = onlyAffectWithDirToLoc;
			PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.PaintConditions.OnlyAffectVerticesWithinDirectionFromActorOrLocation = onlyAffectWithinDirection;
			PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.PaintConditions.IfVertexIsAboveOrBelowWorldZ = ifAboveOrBelowWorldZ;
			PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.PaintConditions.IfVertexColorIsWithinRange = isWithinColorRange;
			PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.PaintConditions.IfVertexIsOnBone = isBone;
			PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.PaintConditions.IfVertexIsOnMaterial = isOnMaterial;
			PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.PaintConditions.IfVertexHasLineOfSightTo = lineOfSight;
			break;

		default:
			break;
		}
	}
}


//-------------------------------------------------------

// Get Complete Callback Settings

void UVertexPaintDetectionComponent::GetCompleteCallbackSettings(UPrimitiveComponent* MeshComponent, bool AutoAppendRequiresCallbackSettings, FRVPDPTaskCallbackSettings& callbackSettings) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetCompleteCallbackSettings);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - GetCompleteCallbackSettings");


	if (AutoAppendRequiresCallbackSettings && MeshComponent) {

		bool meshPaintedByAutoPaintComponent = false;
		FRVPDPTaskCallbackSettings meshRequiredCallbackSettings;

		// Returns True if it got some callback settings that isn't default
		if (UVertexPaintFunctionLibrary::GetMeshRequiredCallbackSettings(MeshComponent, meshPaintedByAutoPaintComponent, meshRequiredCallbackSettings)) {

			// Appends only on if a setting was default, so we don't override some of the user setting that they set.
			callbackSettings.AppendCallbackSettingsToDefaults(meshRequiredCallbackSettings);

			// If Mesh specifically being painted by an auto paint component, then we Have to override it to make sure the auto paint component can use the callback data for it's behaviour. 
			if (meshPaintedByAutoPaintComponent) {

				callbackSettings.IncludeAmountOfPaintedColorsOfEachChannel.IncludeIfMinColorAmountIs = FRVPDPIncludeAmountOfPaintedColorsOfEachChannelSettings().IncludeIfMinColorAmountIs;
				callbackSettings.IncludeAmountOfPaintedColorsOfEachChannel.IncludeVertexColorChannelResultOfEachChannel = true;
				callbackSettings.IncludeAmountOfPaintedColorsOfEachChannel.IncludePhysicsSurfaceResultOfEachChannel = true;
			}
		}
	}


	// Clamps so we don't get any unexpected behaviours down the line
	callbackSettings.CompareMeshVertexColorsToColorArray.TransformedErrorTolerance = static_cast<int32>(UKismetMathLibrary::MapRangeClamped(callbackSettings.CompareMeshVertexColorsToColorArray.ComparisonErrorTolerance, 0, 1, 0, 255));
	callbackSettings.CompareMeshVertexColorsToColorArray.TransformedEmptyVertexColor = UVertexPaintFunctionLibrary::ReliableFLinearToFColor(callbackSettings.CompareMeshVertexColorsToColorArray.EmptyVertexColor);

	callbackSettings.CompareMeshVertexColorsToColorSnippets.TransformedErrorTolerance = static_cast<int32>(UKismetMathLibrary::MapRangeClamped(callbackSettings.CompareMeshVertexColorsToColorSnippets.ComparisonErrorTolerance, 0, 1, 0, 255));
	callbackSettings.CompareMeshVertexColorsToColorSnippets.TransformedEmptyVertexColor = UVertexPaintFunctionLibrary::ReliableFLinearToFColor(callbackSettings.CompareMeshVertexColorsToColorSnippets.EmptyVertexColor);

	callbackSettings.IncludeAmountOfPaintedColorsOfEachChannel.IncludeIfMinColorAmountIs = UKismetMathLibrary::FClamp(callbackSettings.IncludeAmountOfPaintedColorsOfEachChannel.IncludeIfMinColorAmountIs, 0, 1);
}


//-------------------------------------------------------

// Fill Calculate Colors Info Fundementals

void UVertexPaintDetectionComponent::FillCalculateColorsInfoFundementals(FRVPDPCalculateColorsInfo& CalculateColorsInfo) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_FillCalculateColorsInfoFundementals);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - FillCalculateColorsInfoFundementals");

	if (!IsValid(CalculateColorsInfo.TaskFundamentalSettings.MeshComponent)) return;
	if (!CalculateColorsInfo.TaskFundamentalSettings.TaskWorld.IsValid() || !UVertexPaintFunctionLibrary::IsWorldValid(CalculateColorsInfo.TaskFundamentalSettings.TaskWorld.Get())) return;


	int lodsToLoopThrough = 1;

	if (UStaticMeshComponent* staticMeshComponent = Cast<UStaticMeshComponent>(CalculateColorsInfo.TaskFundamentalSettings.MeshComponent)) {

		if (staticMeshComponent->GetStaticMesh()) {

			if (FStaticMeshRenderData* staticMeshRenderData = staticMeshComponent->GetStaticMesh()->GetRenderData()) {

				CalculateColorsInfo.IsStaticMeshTask = true;
				CalculateColorsInfo.VertexPaintStaticMeshComponent = staticMeshComponent;
				lodsToLoopThrough = staticMeshRenderData->LODResources.Num();

				if (USplineMeshComponent* splineMeshComponent = Cast<USplineMeshComponent>(CalculateColorsInfo.TaskFundamentalSettings.MeshComponent)) {

					CalculateColorsInfo.IsSplineMeshTask = true;
					CalculateColorsInfo.VertexPaintSplineMeshComponent = splineMeshComponent;
				}

				else if (UInstancedStaticMeshComponent* instancedStaticMeshComponent = Cast<UInstancedStaticMeshComponent>(CalculateColorsInfo.TaskFundamentalSettings.MeshComponent)) {

					CalculateColorsInfo.IsInstancedMeshTask = true;
					CalculateColorsInfo.VertexPaintInstancedStaticMeshComponent = instancedStaticMeshComponent;

					const int componentItem = CalculateColorsInfo.TaskFundamentalSettings.ComponentItem;
					instancedStaticMeshComponent->GetInstanceTransform(componentItem, CalculateColorsInfo.InstancedMeshTransform, false);
				}
			}
		}
	}

	else if (USkeletalMeshComponent* skeletalMeshComponent = Cast<USkeletalMeshComponent>(CalculateColorsInfo.TaskFundamentalSettings.MeshComponent)) {

		if (FSkeletalMeshRenderData* skelMeshRenderData = skeletalMeshComponent->GetSkeletalMeshRenderData()) {


			CalculateColorsInfo.IsSkeletalMeshTask = true;
			CalculateColorsInfo.VertexPaintSkelComponent = skeletalMeshComponent;

			USkinnedMeshComponent* skinnedMasterComp = nullptr;

			lodsToLoopThrough = skelMeshRenderData->LODRenderData.Num();
			skeletalMeshComponent->GetBoneNames(CalculateColorsInfo.SkeletalMeshBonesNames);


			USkeletalMesh* skeletalMesh = nullptr;

#if ENGINE_MAJOR_VERSION == 4

			skeletalMesh = skeletalMeshComponent->SkeletalMesh;

			// If it has a Master Component then we had to use that when calling CacheRefToLocalMatrices, otherwise we got a crash
			if (skeletalMeshComponent->MasterPoseComponent.Get())
				skinnedMasterComp = skeletalMeshComponent->MasterPoseComponent.Get();


#elif ENGINE_MAJOR_VERSION == 5


#if ENGINE_MINOR_VERSION == 0

			skeletalMesh = skeletalMeshComponent->SkeletalMesh.Get();

			// If it has a Master Component then we had to use that when calling CacheRefToLocalMatrices, otherwise we got a crash
			if (skeletalMeshComponent->MasterPoseComponent.Get())
				skinnedMasterComp = skeletalMeshComponent->MasterPoseComponent.Get();

#else

			skeletalMesh = skeletalMeshComponent->GetSkeletalMeshAsset();

			// If it has a Master Component then we had to use that when calling CacheRefToLocalMatrices, otherwise we got a crash
			if (skeletalMeshComponent->LeaderPoseComponent.Get())
				skinnedMasterComp = skeletalMeshComponent->LeaderPoseComponent.Get();

#endif
#endif


			CalculateColorsInfo.VertexPaintSkeletalMesh = skeletalMesh;


			USkinnedMeshComponent* masterSkinnedComponent = nullptr;

			// Only if we actually had a Master component we set the MasterSkinned to that, otherwise the skeletal mesh comp is considered the Master
			if (skinnedMasterComp)
				masterSkinnedComponent = skinnedMasterComp;
			else 
				masterSkinnedComponent = skeletalMeshComponent;


			if (masterSkinnedComponent) {

				masterSkinnedComponent->CacheRefToLocalMatrices(CalculateColorsInfo.SkeletalMeshRefToLocals);
			}


			// Specific Bones to Loop Through Optimization
			switch (CalculateColorsInfo.VertexPaintDetectionType) {

				case EVertexPaintDetectionType::GetClosestVertexDataDetection:

					if (CalculateColorsInfo.GetClosestVertexDataSettings.HitFundementals.HitBone != NAME_None)
						CalculateColorsInfo.SpecificSkeletalMeshBonesToLoopThrough.Add(CalculateColorsInfo.GetClosestVertexDataSettings.HitFundementals.HitBone);

					break;

				case EVertexPaintDetectionType::GetColorsWithinArea:

					if (CalculateColorsInfo.WithinAreaSettings.SpecificBonesWithinArea.Num() > 0)
						CalculateColorsInfo.SpecificSkeletalMeshBonesToLoopThrough = CalculateColorsInfo.WithinAreaSettings.SpecificBonesWithinArea;

					break;

				case EVertexPaintDetectionType::PaintAtLocation:

					if (CalculateColorsInfo.PaintAtLocationSettings.PaintAtAreaSettings.SpecificBonesToPaint.Num() > 0)
						CalculateColorsInfo.SpecificSkeletalMeshBonesToLoopThrough = CalculateColorsInfo.PaintAtLocationSettings.PaintAtAreaSettings.SpecificBonesToPaint;

					break;

				case EVertexPaintDetectionType::PaintWithinArea:

					if (CalculateColorsInfo.WithinAreaSettings.SpecificBonesWithinArea.Num() > 0)
						CalculateColorsInfo.SpecificSkeletalMeshBonesToLoopThrough = CalculateColorsInfo.WithinAreaSettings.SpecificBonesWithinArea;
					break;

				default:
					break;
			}

			if (CalculateColorsInfo.SpecificSkeletalMeshBonesToLoopThrough.Num() > 0) {


				if (UVertexPaintOptimizationDataAsset* optimizationDataAsset = UVertexPaintFunctionLibrary::GetOptimizationDataAsset(skeletalMeshComponent)) {


					const TMap<USkeletalMesh*, FRVPDPRegisteredSkeletalMeshInfo> allSkeletalMeshBoneInfos = optimizationDataAsset->GetRegisteredSkeletalMeshInfo();

					if (allSkeletalMeshBoneInfos.Contains(skeletalMesh)) {


						const FRVPDPRegisteredSkeletalMeshInfo skeletalMeshBoneInfo = allSkeletalMeshBoneInfos.FindRef(skeletalMesh);

						// If have all Paintable bones, i.e. not IK bones, then going to loop through them all anyway so no point in running any optimization logic
						if (CalculateColorsInfo.SpecificSkeletalMeshBonesToLoopThrough.Num() == skeletalMeshBoneInfo.TotalAmountsOfPaintableBonesWithCollision) {

							CalculateColorsInfo.SpecificSkeletalMeshBonesToLoopThrough.Empty();

							RVPDP_LOG(CalculateColorsInfo.TaskFundamentalSettings.DebugSettings, FColor::Cyan, "The Specific Skeletal Mesh Bones to Loop Through matches the total amount of bones paintable bones with collision, meaning all of the bones of the skeletal mesh will get looped through anyway so resets specific bones to loop through.  ");
						}

						// Otherwise we get the Additional Bones we want to loop through, the ones that are childs of the one we send in that doesn't have collision of their own that we can't hit with a trace. 
						else {

							TArray<FName> additionalBonesToInclude;

							for (const FName& boneName : CalculateColorsInfo.SpecificSkeletalMeshBonesToLoopThrough) {


								FRVPDPSkeletalMeshBonesToIncludeInfo boneChildsToInclude = skeletalMeshBoneInfo.SkeletalMeshBonesToInclude.FindRef(boneName);

								if (boneChildsToInclude.BoneChildsToInclude.Num() > 0)
									additionalBonesToInclude.Append(boneChildsToInclude.BoneChildsToInclude);

								if (boneChildsToInclude.BoneParentsToInclude.Num() > 0)
									additionalBonesToInclude.Append(boneChildsToInclude.BoneParentsToInclude);
							}


							for (const FName& boneName : additionalBonesToInclude) {

								if (!CalculateColorsInfo.SpecificSkeletalMeshBonesToLoopThrough.Contains(boneName))
									CalculateColorsInfo.SpecificSkeletalMeshBonesToLoopThrough.Add(boneName);
							}
						}
					}
				}
			}
		}
	}


	// UE5 Specific Meshes
#if ENGINE_MAJOR_VERSION == 5


	else if (UDynamicMeshComponent* dynamicMeshComponent = Cast<UDynamicMeshComponent>(CalculateColorsInfo.TaskFundamentalSettings.MeshComponent)) {
		CalculateColorsInfo.IsDynamicMeshTask = true;
		CalculateColorsInfo.VertexPaintDynamicMeshComponent = dynamicMeshComponent;
	}

#if ENGINE_MINOR_VERSION >= 3

	else if (UGeometryCollectionComponent* geometryCollectionComponent = Cast<UGeometryCollectionComponent>(CalculateColorsInfo.TaskFundamentalSettings.MeshComponent)) {

		if (const UGeometryCollection* geometryCollection = geometryCollectionComponent->GetRestCollection()) {

			CalculateColorsInfo.IsGeometryCollectionTask = true;
			CalculateColorsInfo.VertexPaintGeometryCollectionComponent = geometryCollectionComponent;
			CalculateColorsInfo.VertexPaintGeometryCollection = const_cast<UGeometryCollection*>(geometryCollection);
			CalculateColorsInfo.VertexPaintGeometryCollectionData = geometryCollection->GetGeometryCollection().Get();
		}
	}
		
#endif

#endif
	

	// If Painting
	if (CalculateColorsInfo.PaintTaskSettings.MeshComponent) {

		lodsToLoopThrough = UVertexPaintFunctionLibrary::GetAmountOfLODsToPaintOn(CalculateColorsInfo.TaskFundamentalSettings.MeshComponent, CalculateColorsInfo.PaintTaskSettings.OverrideLOD.OverrideLODToPaintUpTo, CalculateColorsInfo.PaintTaskSettings.OverrideLOD.AmountOfLODsToPaint);

		CalculateColorsInfo.TaskFundamentalSettings.CallbackSettings.CallbacksOnObjectsForMeshComponent = UVertexPaintFunctionLibrary::GetRegisteredPaintTaskCallbacksObjectsForSpecificMeshComponent(this, CalculateColorsInfo.PaintTaskSettings.MeshComponent);
	}

	// If Detecting
	else if (CalculateColorsInfo.DetectSettings.MeshComponent) {

		// If set to only include vertex data for Lod 0 in the callback, there's no reason to loop through more LODs, unlike painting, where you still need to paint on all LODs even though you only want LOD0 in the callback. 
		if (CalculateColorsInfo.TaskFundamentalSettings.CallbackSettings.IncludeVertexDataOnlyForLOD0)
			lodsToLoopThrough = 1;

		CalculateColorsInfo.TaskFundamentalSettings.CallbackSettings.CallbacksOnObjectsForMeshComponent = UVertexPaintFunctionLibrary::GetRegisteredDetectTaskCallbacksObjectsForSpecificMeshComponent(this, CalculateColorsInfo.PaintTaskSettings.MeshComponent);
	}


	GetTaskFundamentalsSettings(CalculateColorsInfo.TaskFundamentalSettings.MeshComponent, CalculateColorsInfo, lodsToLoopThrough);

	CalculateColorsInfo.VertexPaintMaterialDataAsset = UVertexPaintFunctionLibrary::GetVertexPaintMaterialDataAsset(CalculateColorsInfo.TaskFundamentalSettings.MeshComponent);
	ResolvePaintConditions(CalculateColorsInfo.ApplyColorSettings);


	const bool colorSnippetTask = (CalculateColorsInfo.VertexPaintDetectionType == EVertexPaintDetectionType::PaintColorSnippet);

	const bool setMeshColorsTask = (CalculateColorsInfo.VertexPaintDetectionType == EVertexPaintDetectionType::SetMeshVertexColorsDirectly || CalculateColorsInfo.VertexPaintDetectionType == EVertexPaintDetectionType::SetMeshVertexColorsDirectlyUsingSerializedString);

	const bool paintEntireMeshWithPropogateToLODs = (CalculateColorsInfo.VertexPaintDetectionType == EVertexPaintDetectionType::PaintEntireMesh && CalculateColorsInfo.PaintOnEntireMeshSettings.PaintOnRandomVerticesSettings.PaintAtRandomVerticesSpreadOutOverTheEntireMesh && CalculateColorsInfo.PaintOnEntireMeshSettings.PaintOnRandomVerticesSettings.PaintAtRandomVerticesSpreadOutOverTheEntireMesh_PropogateLOD0ToAllLODsMethod == EPaintEntireMeshPropogateToLODsMethod::ModifiedEngineMethod);


	// Paint Color Snippets, Set Mesh Component Vertex Colors regularly or with Serialized String only use LOD0 data, so for them we always propogate to other LODs. Paint Randomly over the Entire Mesh however has the option to do this since you might want to randomize on each LOD to save performance
	if (colorSnippetTask || setMeshColorsTask || paintEntireMeshWithPropogateToLODs) {

		if (lodsToLoopThrough > 1) {

			if (UVertexPaintOptimizationDataAsset* optimizationDataAsset = UVertexPaintFunctionLibrary::GetOptimizationDataAsset(this)) {

				if (optimizationDataAsset->HasMeshPropogateToLODInfoRegistered(CalculateColorsInfo.VertexPaintSourceMesh))
					CalculateColorsInfo.PropogateLOD0ToAllLODsFromStoredData = true;
				else
					CalculateColorsInfo.PropogateLOD0ToAllLODsUsingOctree = true;
			}

			else {

				CalculateColorsInfo.PropogateLOD0ToAllLODsUsingOctree = true;
			}
		}
	}

	
	if (CalculateColorsInfo.TaskFundamentalSettings.MultiThreadSettings.UseMultithreadingForCalculations) {

		if (CalculateColorsInfo.TaskFundamentalSettings.DebugSettings.GameThreadSpecificDebugSymbols.DrawVertexPositionDebugPoint || CalculateColorsInfo.TaskFundamentalSettings.DebugSettings.GameThreadSpecificDebugSymbols.DrawVertexPositionDebugPointIfGotPaintApplied || CalculateColorsInfo.TaskFundamentalSettings.DebugSettings.GameThreadSpecificDebugSymbols.DrawClothVertexPositionDebugPoint || CalculateColorsInfo.TaskFundamentalSettings.DebugSettings.GameThreadSpecificDebugSymbols.DrawVertexNormalDebugArrow) {


			RVPDP_LOG(CalculateColorsInfo.TaskFundamentalSettings.DebugSettings, FColor::Orange, "Paint/Detect Task is set to Draw Debug Symbols at Vertices but it only works if not Multithreading, i.e. on the Game Thread.");
		}
	}
}


//-------------------------------------------------------

// Get Task Fundamentals Settings

void UVertexPaintDetectionComponent::GetTaskFundamentalsSettings(UPrimitiveComponent* MeshComponent, FRVPDPCalculateColorsInfo& CalculateColorsInfo, int LODsToLoopThrough) {


	CalculateColorsInfo.TaskResult.AssociatedPaintComponent = this;

	if (!IsValid(MeshComponent)) return;
	if (!IsValid(MeshComponent->GetOwner())) return;

	CalculateColorsInfo.TaskFundamentalSettings.MeshComponent = MeshComponent;
	CalculateColorsInfo.TaskResult.MeshComponent = MeshComponent;

	CalculateColorsInfo.VertexPaintComponent = MeshComponent;
	CalculateColorsInfo.VertexPaintComponentName = MeshComponent->GetName();
	CalculateColorsInfo.VertexPaintComponentTransform = MeshComponent->GetComponentTransform();
	CalculateColorsInfo.VertexPaintSourceMesh = UVertexPaintFunctionLibrary::GetMeshComponentSourceMesh(MeshComponent);
	CalculateColorsInfo.VertexPaintActor = MeshComponent->GetOwner();
	CalculateColorsInfo.VertexPaintActorName = MeshComponent->GetOwner()->GetName();
	CalculateColorsInfo.VertexPaintActorTransform = MeshComponent->GetOwner()->GetTransform();

	if (CalculateColorsInfo.VertexPaintSourceMesh) {

		CalculateColorsInfo.InitialMeshVertexData.MeshSource = TSoftObjectPtr<UObject>(FSoftObjectPath(CalculateColorsInfo.VertexPaintSourceMesh->GetPathName()));
	}


	CalculateColorsInfo.LodsToLoopThrough = LODsToLoopThrough;

	if (LODsToLoopThrough > 0) {

		CalculateColorsInfo.InitialMeshVertexData.MeshDataPerLOD.SetNum(LODsToLoopThrough);

		for (int i = 0; i < CalculateColorsInfo.InitialMeshVertexData.MeshDataPerLOD.Num(); i++)
			CalculateColorsInfo.InitialMeshVertexData.MeshDataPerLOD[i].Lod = i;
	}

	CalculateColorsInfo.InitialMeshVertexData.MeshComp = MeshComponent;
	CalculateColorsInfo.TaskResult.MeshVertexData = CalculateColorsInfo.InitialMeshVertexData;


	// If haven't specified an Actor to run interfaces on then we default to the Actor we're painting/detecting on
	if (!CalculateColorsInfo.TaskFundamentalSettings.CallbackSettings.ObjectToRunInterfacesOn.IsValid()) {

		CalculateColorsInfo.TaskFundamentalSettings.CallbackSettings.ObjectToRunInterfacesOn = MeshComponent->GetOwner();
	}
}


//-------------------------------------------------------

// Check Valids

bool UVertexPaintDetectionComponent::CheckValidFundementals(const FRVPDPTaskFundamentalSettings& TaskFundementals, UPrimitiveComponent* Component) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_CheckValidFundementals);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - CheckValidFundementals");


	if (!IsInGameThread()) {

		RVPDP_LOG(TaskFundementals.DebugSettings, FColor::Red, "Trying to Start a Task but not from Game Thread!");
		return false;
	}

	if (IsBeingDestroyed()) {

		RVPDP_LOG(TaskFundementals.DebugSettings, FColor::Red, "Trying to Start a Task but Paint Component is being Destroyed");
		return false;
	}

	if (!IsValid(Component)) {

		RVPDP_LOG(TaskFundementals.DebugSettings, FColor::Cyan, "Trying to Paint/Detect but Component provided is not Valid! Have you connected anything to the Component pin on the Node?");
		return false;
	}

	if (!IsValid(Component->GetOwner())) {

		RVPDP_LOG(TaskFundementals.DebugSettings, FColor::Cyan, "Trying to Paint/Detect but Component Owning actor is not Valid! ");
		return false;
	}

	if (!UVertexPaintFunctionLibrary::IsWorldValid(Component->GetWorld())) {

		RVPDP_LOG(TaskFundementals.DebugSettings, FColor::Cyan, "Trying to Paint/Detect but World not Valid");
		return false;
	}

	if (!UVertexPaintFunctionLibrary::GetVertexPaintTaskQueue(GetWorld())) {

		RVPDP_LOG(TaskFundementals.DebugSettings, FColor::Red, "Trying to Start a Task but Vertex Paint Task Queue is not Valid! Possibly the the Game Instance Subsystem has not been created. Are you using a Custom Game Instance Subsystem perhaps and overriding any virtual functions?");
		return false;
	}


	if (UStaticMeshComponent* staticMeshComp = Cast<UStaticMeshComponent>(Component)) {

		if (!IsValid(staticMeshComp->GetStaticMesh())) {


			RVPDP_LOG(TaskFundementals.DebugSettings, FColor::Red, "Trying to Paint/Detect on Static Mesh Component %s but it has no Mesh is Set", *staticMeshComp->GetName());
			return false;
		}

		if (!staticMeshComp->GetStaticMesh()->bAllowCPUAccess) {

			RVPDP_LOG(TaskFundementals.DebugSettings, FColor::Red, "Trying to Paint/Detect on Static Mesh but it's CPU Access is not set to True");
			return false;
		}


#if ENGINE_MAJOR_VERSION == 5

		// Note ->NaniteSettings was only available in Editor, so had to use this to check if nanite is enabled. 
		if (staticMeshComp->GetStaticMesh().Get()->HasValidNaniteData()) {

			RVPDP_LOG(TaskFundementals.DebugSettings, FColor::Red, "Trying to Paint/Detect on Static Mesh that has Nanite Enabled! Vertex Painting on Nanite Meshes is currently not supported.");
			return false;
		}

#endif


		if (!staticMeshComp->GetBodySetup()) {

			RVPDP_LOG(TaskFundementals.DebugSettings, FColor::Red, "Trying to Paint/Detect on Static Mesh but it doesn't have a Body Setup");
			return false;
		}


		RVPDP_LOG(TaskFundementals.DebugSettings, FColor::Cyan, "Trying to Paint/Detect on Static Mesh Component, with Source Mesh: %s", *staticMeshComp->GetStaticMesh()->GetName());

		return true;
	}

	else if (USkeletalMeshComponent* skelMeshComp = Cast<USkeletalMeshComponent>(Component)) {

		USkeletalMesh* skelMesh = nullptr;

#if ENGINE_MAJOR_VERSION == 4

		skelMesh = skelMeshComp->SkeletalMesh;

#elif ENGINE_MAJOR_VERSION == 5

#if ENGINE_MINOR_VERSION == 0

		skelMesh = skelMeshComp->SkeletalMesh.Get();

#else

		skelMesh = skelMeshComp->GetSkeletalMeshAsset();

#endif
#endif


		if (!IsValid(skelMesh)) {

			RVPDP_LOG(TaskFundementals.DebugSettings, FColor::Red, "Trying to Paint/Detect on Skeletal Mesh Component but source mesh is null");
			return false;
		}

		else {

			RVPDP_LOG(TaskFundementals.DebugSettings, FColor::Cyan, "Trying to Paint/Detect on Skeletal Mesh Component, with Source Mesh: %s", *skelMesh->GetName());
		}


		if (!skelMeshComp->GetSkeletalMeshRenderData()) {

			RVPDP_LOG(TaskFundementals.DebugSettings, FColor::Red, "Trying to Paint/Detect on Skeletal Mesh but it hasn't properly been initialized yet because it's Skeletal Mesh Render Data isn't valid. ");
			return false;
		}

		if (!skelMeshComp->GetSkeletalMeshRenderData()->IsInitialized()) {

			RVPDP_LOG(TaskFundementals.DebugSettings, FColor::Red, "Trying to Paint/Detect on Skeletal Mesh %s but it's SkeletalMeshRenderData isn't Initialized yet. ", *Component->GetOwner()->GetName());
			return false;
		}


		if (!skelMesh->GetResourceForRendering()) {

			RVPDP_LOG(TaskFundementals.DebugSettings, FColor::Red, "Trying to Paint/Detect on Skeletal Mesh %s but it has invalid ResourceForRendering!", *Component->GetOwner()->GetName());
			return false;
		}


		// Could get a crash very rarely if switching skeletal meshes and painting every frame, so added these for extra checks so we hopefully can't create a task if these aren't valid. We also have these in the task itself in case they become invalid after this. 
		for (int currentLOD = 0; currentLOD < skelMeshComp->GetSkeletalMeshRenderData()->LODRenderData.Num(); currentLOD++) {


			const FSkeletalMeshLODRenderData& skelMeshRenderData = skelMeshComp->GetSkeletalMeshRenderData()->LODRenderData[currentLOD];

			if (skelMeshRenderData.HasClothData()) {

				if (!skelMeshRenderData.ClothVertexBuffer.IsInitialized()) {

					RVPDP_LOG(TaskFundementals.DebugSettings, FColor::Red, "Trying to Paint/Detect on Skeletal Mesh with Cloth but cloth vertex buffer hasn't properly been initialized yet.");
					return false;
				}
			}

			if (!skelMeshRenderData.StaticVertexBuffers.StaticMeshVertexBuffer.IsInitialized() || !skelMeshRenderData.StaticVertexBuffers.StaticMeshVertexBuffer.TangentsVertexBuffer.IsInitialized() || !skelMeshRenderData.StaticVertexBuffers.StaticMeshVertexBuffer.GetTangentData()) {

				RVPDP_LOG(TaskFundementals.DebugSettings, FColor::Red, "Trying to Paint/Detect on Skeletal Mesh but LOD %i StaticMeshVertexBuffer, TangentVertexBuffer or TangentData isn't initialized yet. ", currentLOD);
				return false;
			}
		}

		return true;
	}

	else if (UInstancedStaticMeshComponent* instancedMeshComp = Cast<UInstancedStaticMeshComponent>(Component)) {

		if (TaskFundementals.ComponentItem < 0) {

			RVPDP_LOG(TaskFundementals.DebugSettings, FColor::Red, "Trying to Paint/Detect on Instanced Mesh %s but the Item provided is less than 0, so we can't get the specific instance and can calculate Location properly. ", *instancedMeshComp->GetName());
			return false;
		}

		return true;
	}


	// UE5 Does some more checks on UE5 specific meshes
#if ENGINE_MAJOR_VERSION == 5

	if (UDynamicMeshComponent* dynamicMeshComp = Cast<UDynamicMeshComponent>(Component)) {

		if (dynamicMeshComp->GetDynamicMesh()) {


			UE::Geometry::FDynamicMesh3* dynamicMesh3 = nullptr;
			dynamicMesh3 = &dynamicMeshComp->GetDynamicMesh()->GetMeshRef();

			if (dynamicMesh3) {

				// Enable Vertex Colors so we can get them when running paint / Detect jobs
				if (!dynamicMesh3->HasVertexColors()) {

					dynamicMesh3->EnableVertexColors(FVector3f(0, 0, 0));
				}

				/*
				* If we Enable Normals and UVs, we apparently Set them to be whatever the initial value we give here as well, which may mess up something that the user want to have depending on the shape they've made. Think this is meant to be set somewhere where the dynamic mesh is being created, and if set correct there, the HasUVs and HasVertexNormals should be true and we should be able to get them later .
				*
				if (!dynamicMeshComp->GetDynamicMesh()->GetMeshPtr()->HasVertexUVs())
					dynamicMeshComp->GetDynamicMesh()->GetMeshPtr()->EnableVertexUVs(FVector2f(0, 1));

				if (!dynamicMeshComp->GetDynamicMesh()->GetMeshPtr()->HasVertexNormals())
					dynamicMeshComp->GetDynamicMesh()->GetMeshPtr()->EnableVertexNormals(FVector3f(0, 0, 1));
				*/

				RVPDP_LOG(TaskFundementals.DebugSettings, FColor::Cyan, "Trying to Paint/Detect on Dynamic Mesh Component: %s", *dynamicMeshComp->GetName());

				return true;
			}

			else {

				RVPDP_LOG(TaskFundementals.DebugSettings, FColor::Red, "Trying to Paint/Detect on Dynamic Mesh Component: %s but DynamicMesh Object GetMeshPtr is null. ", *dynamicMeshComp->GetName());

				return false;
			}
		}

		else {

			RVPDP_LOG(TaskFundementals.DebugSettings, FColor::Red, "Trying to Paint/Detect on Dynamic Mesh Component: %s but couldn't GetDynamicMesh Object. ", *dynamicMeshComp->GetName());

			return false;
		}
	}

#if ENGINE_MINOR_VERSION >= 3

	if (UGeometryCollectionComponent* geometryCollectionComp = Cast<UGeometryCollectionComponent>(Component)) {

		// Geometry Collection RebuildRenderData is only available from 5.3 so any version before that unfortunately can't paint on Geometry Collection Components. 

#if WITH_EDITOR

		if (geometryCollectionComp->GetRestCollection()) {

			TSharedPtr<FGeometryCollection, ESPMode::ThreadSafe> geometryCollectionData = geometryCollectionComp->GetRestCollection()->GetGeometryCollection();


			if (geometryCollectionData.Get()) {

				RVPDP_LOG(TaskFundementals.DebugSettings, FColor::Cyan, "Trying to Paint/Detect on Geometry Collection Component with Rest Collection: %s", *geometryCollectionComp->GetRestCollection()->GetName());

				return true;
			}

			else {

				RVPDP_LOG(TaskFundementals.DebugSettings, FColor::Red, "Trying to Paint/Detect on Geometry Collection Component Geometry Collection Data isn't valid!");
			}
		}

		else {

			RVPDP_LOG(TaskFundementals.DebugSettings, FColor::Red, "Trying to Paint/Detect on Geometry Collection Component but couldn't get Rest Collection!");
		}

#else

		RVPDP_LOG(TaskFundementals.DebugSettings, FColor::Red, "Trying to Paint/Detect on Geometry Collection Component but Not in Editor Time. You can currently only paint on them in Editor since the GeometryCollection->RebuildRenderData is Editor Only!");

		return false;
#endif

	}

#endif
#endif


	RVPDP_LOG(TaskFundementals.DebugSettings, FColor::Red, "Trying to Paint/Detect but Component %s is not a Static, Spline, Skeletal, Dynamic, Instanced or Geometry Collection Mesh!", *Component->GetName());

	return false;
}

bool UVertexPaintDetectionComponent::CheckValidVertexPaintSettings(const FRVPDPPaintTaskSettings& VertexPaintSettings, bool IgnoreTaskQueueChecks) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_CheckValidVertexPaintSettings);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - CheckValidVertexPaintSettings");

	if (!IsValid(VertexPaintSettings.MeshComponent)) return false;
	if (!VertexPaintSettings.TaskWorld.IsValid() || !UVertexPaintFunctionLibrary::IsWorldValid(VertexPaintSettings.TaskWorld.Get())) return false;


	if (!IgnoreTaskQueueChecks && VertexPaintSettings.OnlyAllowOneTaskForMeshPerPaintComponent && !VertexPaintSettings.CanClearOutPreviousMeshTasksInQueue) {

		if (HasPaintTaskQueuedUpOnMeshComponent(VertexPaintSettings.MeshComponent)) {

			RVPDP_LOG(VertexPaintSettings.DebugSettings, FColor::Red, "Won't Queue up another Paint Task because the Vertex Paint Component: %s already has one queued up for the Actor: %s and Mesh Component %s and OnlyAllowOneTaskForMeshPerPaintComponent in the Task Settings is Set to True. ", *GetName(), *VertexPaintSettings.MeshComponent->GetOwner()->GetName(), *VertexPaintSettings.MeshComponent->GetName());

			return false;
		}
	}

	// Can ignore the task queue limit, in case you need to load a game and queue a bunch of paint task but just once. If a task that can clear out the task queue then it doesn't bother with this check either since it will clear it out anyway. 
	if (!VertexPaintSettings.IgnoreTaskQueueLimit && !VertexPaintSettings.CanClearOutPreviousMeshTasksInQueue) {

		if (UVertexPaintDetectionTaskQueue* taskQueue = UVertexPaintFunctionLibrary::GetVertexPaintTaskQueue(VertexPaintSettings.MeshComponent)) {

			const int maxAllowedTasks = GetDefault<UVertexPaintDetectionSettings>()->MaxAmountOfAllowedTasksPerMesh;
			const int taskAmount = taskQueue->GetAmountOfPaintTasksComponentHas(VertexPaintSettings.MeshComponent);

			if (taskAmount >= maxAllowedTasks) {

				// Always prints this to log so users can see in the output that something may not be fully optimized
				UKismetSystemLibrary::PrintString(VertexPaintSettings.MeshComponent, FString::Printf(TEXT("Actor: %s and Mesh: %s Has reached %i Allowed Paint Tasks Queue Limit and we don't Allow any more per Mesh as the Performance can get affected if the queue per mesh becomes too big. You can change this in the Project Settings! \nBut if the queue grows to big you may get unwanted result as well since it may take a while for a paint job to show it's effect. \nRecommend Reviewing how often you Add new Tasks in case you're doing it every frame, you can for instance check if a paint component has any active tasks and only add it if it doesn't have any, or add a new task when the Old one is Finished. "), *VertexPaintSettings.MeshComponent->GetOwner()->GetName(), *VertexPaintSettings.MeshComponent->GetName(), maxAllowedTasks), VertexPaintSettings.DebugSettings.PrintLogsToScreen, true, FColor::Red, VertexPaintSettings.DebugSettings.PrintLogsToScreen_Duration);

				return false;
			}
		}
	}


	if (VertexPaintSettings.OverrideLOD.OverrideLODToPaintUpTo) {

		if (VertexPaintSettings.OverrideLOD.AmountOfLODsToPaint <= 0) {

			RVPDP_LOG(VertexPaintSettings.DebugSettings, FColor::Red, "Trying to Paint and Override LOD to paint on but LOD given is <= 0!");

			return false;
		}
	}


	// If anything is added onto the array
	if (VertexPaintSettings.CanOnlyApplyPaintOnTheseActors.Num() > 0) {

		// If the Actor we're trying to paint on isn't in the array. 
		if (!VertexPaintSettings.CanOnlyApplyPaintOnTheseActors.Contains(VertexPaintSettings.MeshComponent->GetOwner())) {

			RVPDP_LOG(VertexPaintSettings.DebugSettings, FColor::Red, "Actor: %s  isn't in the CanOnlyApplyPaintOnTheseActors array that has been set. Either add the Actor to it, or don't fill the array.", *VertexPaintSettings.MeshComponent->GetOwner()->GetName());

			return false;
		}
	}


	if (auto skelMeshComp = Cast<USkeletalMeshComponent>(VertexPaintSettings.MeshComponent)) {

		if (skelMeshComp->GetSkeletalMeshRenderData()) {

			if (skelMeshComp->GetSkeletalMeshRenderData()->LODRenderData.IsValidIndex(skelMeshComp->GetPredictedLODLevel())) {

				// If current viewable LOD has cloth then requires bWaitForParallelClothTask to be true
				if (skelMeshComp->GetSkeletalMeshRenderData()->LODRenderData[skelMeshComp->GetPredictedLODLevel()].HasClothData()) {

					if (!skelMeshComp->bWaitForParallelClothTask && VertexPaintSettings.AffectClothPhysics) {

						RVPDP_LOG(VertexPaintSettings.DebugSettings, FColor::Red, "Trying to Detect/Paint on Skeletal Mesh with Cloth, with AffectClothPhysics to True but the Skeletal Mesh Component bWaitForParallelClothTask is false! You can set it to True by selecting the skeletal mesh component and set bWaitForParallelClothTask to True in the Details. ");

						return false;
					}
				}
			}
		}
	}


	if (VertexPaintSettings.OverrideVertexColorsToApplySettings.OverrideVertexColorsToApply && !IsValid(VertexPaintSettings.OverrideVertexColorsToApplySettings.ObjectToRunOverrideVertexColorsInterface)) {

		RVPDP_LOG(VertexPaintSettings.DebugSettings, FColor::Orange, "Trying to Paint and Override Vertex Colors To Apply, but the neither the Actor or Component set to run the Override Vertex Colors Interface is valid! Task will still run but the interface won't be called. ");
	}

	return true;
}

bool UVertexPaintDetectionComponent::CheckValidVertexPaintColorSettings(const FRVPDPApplyColorSetting& ColorSettings) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_CheckValidVertexPaintColorSettings);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - CheckValidVertexPaintColorSettings");

	if (!IsValid(ColorSettings.MeshComponent)) return false;
	if (!ColorSettings.TaskWorld.IsValid() || !UVertexPaintFunctionLibrary::IsWorldValid(ColorSettings.TaskWorld.Get())) return false;


	if (ColorSettings.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.ApplyVertexColorUsingPhysicsSurface) {

		if (!UVertexPaintFunctionLibrary::GetVertexPaintMaterialDataAsset(ColorSettings.MeshComponent)) {

			RVPDP_LOG(ColorSettings.DebugSettings, FColor::Red, "Set to Apply Colors Using Physics Surface, but no Material Data Asset is set in the Project Settings. This means that we can't get what Physics Surface is on each Vertex Color Channel. ");

			return false;
		}


		if (ColorSettings.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.PhysicalSurfacesToApply.Num() == 0) {

			RVPDP_LOG(ColorSettings.DebugSettings, FColor::Red, "Set to Apply Colors Using Physics Surface, but no Physics Surfaces has been added to the TMap. ");

			return false;
		}


		bool isAllPaintStrengthsZero = true;

		for (const FRVPDPPhysicsSurfaceToApplySettings& physicalSurfaceToApply : ColorSettings.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.PhysicalSurfacesToApply) {

			if (physicalSurfaceToApply.SurfaceToApply == EPhysicalSurface::SurfaceType_Default) {

				RVPDP_LOG(ColorSettings.DebugSettings, FColor::Red, "Set to Apply Colors Using Physics Surfaces, but one of them set to apply Default Physics Surface which isn't possible. ");
				return false;
			}

			if (physicalSurfaceToApply.SettingToApplyWith == EApplyVertexColorSetting::EAddVertexColor) {

				if (physicalSurfaceToApply.StrengthToApply != 0) {

					isAllPaintStrengthsZero = false;
					break;
				}
			}

			else if (physicalSurfaceToApply.SettingToApplyWith == EApplyVertexColorSetting::ESetVertexColor) {

				isAllPaintStrengthsZero = false;
				break;
			}
		}


		// If set to paint with Physics Surface with Add, but the Paint Strength is 0, and the strength on channels without the surface is also 0 with Add, so no difference will be made
		if (isAllPaintStrengthsZero && ColorSettings.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.StrengtOnChannelsWithoutThePhysicsSurfaces == 0 && ColorSettings.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.ApplyWithSettingOnChannelsWithoutThePhysicsSurface == EApplyVertexColorSetting::EAddVertexColor) {

			
			if (ColorSettings.OverrideVertexColorsToApplySettings.OverrideVertexColorsToApply && IsValid(ColorSettings.OverrideVertexColorsToApplySettings.ObjectToRunOverrideVertexColorsInterface)) {

				RVPDP_LOG(ColorSettings.DebugSettings, FColor::Orange, "Set to Apply Colors Using Physics Surface, but with Paint Strength 0. But since the task is also set to Override Vertex Colors, the task will be allowed to run in case the override interface returns to apply any colors.");
			}

			else {

				bool anyPhysicsSurfaceTryingToLerpToTarget = false;
				for (const FRVPDPPhysicsSurfaceToApplySettings& physicalSurfaceToApply : ColorSettings.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.PhysicalSurfacesToApply) {

					if (physicalSurfaceToApply.LerpPhysicsSurfaceToTarget.LerpToTarget && physicalSurfaceToApply.LerpPhysicsSurfaceToTarget.LerpStrength != 0) {

						anyPhysicsSurfaceTryingToLerpToTarget = true;
						break;
					}
				}

				if (!anyPhysicsSurfaceTryingToLerpToTarget) {

					RVPDP_LOG(ColorSettings.DebugSettings, FColor::Red, "Trying to Apply Colors using Physics Surface but Paint Strength is 0, which wouldn't make any difference, and is not trying to Lerp to a Target either. ");

					return false;
				}
			}
		}


		/* This check if the Material has any of the physics surfaces registered was quite costly, about 1 ms when you have a bunch of tasks running so skipping it for now. 
		bool gotChannelsPhysicsSurfaceIsRegisteredTo = false;

		for (int i = 0; i < ColorSettings.MeshComponent->GetNumMaterials(); i++) {

			for (const FRVPDPPhysicsSurfaceToApplySettings& physicalSurfaceToApply : ColorSettings.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.PhysicalSurfacesToApply) {

				bool physicsSurfaceAtRed = false;
				bool physicsSurfaceAtGreen = false;
				bool physicsSurfaceAtBlue = false;
				bool physicsSurfaceAtAlpha = false;

				UVertexPaintFunctionLibrary::GetChannelsPhysicsSurfaceIsRegisteredTo(ColorSettings.MeshComponent, ColorSettings.MeshComponent->GetMaterial(i), physicalSurfaceToApply.SurfaceToApply, physicsSurfaceAtRed, physicsSurfaceAtGreen, physicsSurfaceAtBlue, physicsSurfaceAtAlpha);


				if (physicsSurfaceAtRed || physicsSurfaceAtGreen || physicsSurfaceAtBlue || physicsSurfaceAtAlpha) {

					gotChannelsPhysicsSurfaceIsRegisteredTo = true;
					break;
				}
			}

			if (gotChannelsPhysicsSurfaceIsRegisteredTo)
				break;
		}

		if (!gotChannelsPhysicsSurfaceIsRegisteredTo) {

			RVPDP_LOG(ColorSettings.DebugSettings, FColor::Red, "Set to Apply Colors Using Physics Surface, but was unable to get Colors To Apply on any of the Mesh's Materials. Double check if the physics surfaces is registered to it, as well as physics surface families is setup correctly.");

			return false;
		}
		*/
	}


	// Checks if any difference is going to be made by any of the RGBA channels if intend to use them and not applying with physics surface
	else {


		// If all set to apply with Add but with no amount, and not overriding or lerping colors then there won't be a difference
		if (ColorSettings.ApplyVertexColorSettings.ApplyColorsOnRedChannel.ApplyWithSetting == EApplyVertexColorSetting::EAddVertexColor && ColorSettings.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.ApplyWithSetting == EApplyVertexColorSetting::EAddVertexColor && ColorSettings.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.ApplyWithSetting == EApplyVertexColorSetting::EAddVertexColor && ColorSettings.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.ApplyWithSetting == EApplyVertexColorSetting::EAddVertexColor) {

			if (ColorSettings.ApplyVertexColorSettings.ApplyColorsOnRedChannel.AmountToApply == 0 && ColorSettings.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.AmountToApply == 0 && ColorSettings.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.AmountToApply == 0 && ColorSettings.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.AmountToApply == 0) {

				if (ColorSettings.OverrideVertexColorsToApplySettings.OverrideVertexColorsToApply && IsValid(ColorSettings.OverrideVertexColorsToApplySettings.ObjectToRunOverrideVertexColorsInterface)) {

					RVPDP_LOG(ColorSettings.DebugSettings, FColor::Orange, "Trying to Apply Colors with RGBA with 0 Strength in Colors. But since the Task is also set to Override Vertex Colors, it will be allowed to run.");
				}

				else {

					if (!ColorSettings.ApplyVertexColorSettings.IsAnyColorChannelSetToSetToLerpToTarget()) {

						RVPDP_LOG(ColorSettings.DebugSettings, FColor::Red, "Trying to Apply Colors with RGBA with 0 Strength in Colors which wouldn't make any difference. ");

						return false;
					}
				}
			}
		}
	}


	return true;
}

bool UVertexPaintDetectionComponent::DetectTaskCheckValid(const FRVPDPTaskFundamentalSettings& TaskFundementals, UPrimitiveComponent* Component) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_DetectTaskCheckValid);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - DetectTaskCheckValid");


	if (!CheckValidFundementals(TaskFundementals, Component))
		return false;


	if (TaskFundementals.OnlyAllowOneTaskForMeshPerPaintComponent) {

		if (HasDetectTaskQueuedUpOnMeshComponent(Component)) {

			RVPDP_LOG(TaskFundementals.DebugSettings, FColor::Red, "Won't Queue up another Detect Task because the Vertex Paint Component: %s already has one queued up for the Actor: %s and Mesh Component %s and OnlyAllowOneTaskForMeshPerPaintComponent in the Task Settings is Set to True. ", *GetName(), *TaskFundementals.MeshComponent->GetOwner()->GetName(), *TaskFundementals.MeshComponent->GetName());

			return false;
		}
	}

	// Can ignore the Task queue limit, which may be useful for Save / Load Systems, in case you need to detect on a bunch of meshes but just once
	if (!TaskFundementals.IgnoreTaskQueueLimit) {

		if (UVertexPaintDetectionTaskQueue* taskQueue = UVertexPaintFunctionLibrary::GetVertexPaintTaskQueue(TaskFundementals.MeshComponent)) {

			const int maxAllowedTasks = GetDefault<UVertexPaintDetectionSettings>()->MaxAmountOfAllowedTasksPerMesh;
			const int taskAmount = taskQueue->GetAmountOfDetectionTasksComponentHas(TaskFundementals.MeshComponent);

			if (taskAmount >= maxAllowedTasks) {


				// Always prints this to log so users can see in the output that something may not be fully optimized
				UKismetSystemLibrary::PrintString(TaskFundementals.GetTaskWorld(), FString::Printf(TEXT("Actor: %s and Mesh: %s Has over %i Allowed Detect Tasks Queue Limit and we don't Allow any more per Mesh as the Performance gets affected if the queue per mesh becomes to big since the TMaps become more expensive to use! You can change this in the Project Settings, but if the queue grows too big you will get unwanted result as well since it may take a while for a detect job run and the callback will run. \nRecommend Reviewing how often you Add new Tasks. You can for instance Add a New Task when the Old one is Finished instead of adding them every frame. "), *TaskFundementals.MeshComponent->GetOwner()->GetName(), *TaskFundementals.MeshComponent->GetName(), maxAllowedTasks), TaskFundementals.DebugSettings.PrintLogsToScreen, true, FColor::Red, TaskFundementals.DebugSettings.PrintLogsToScreen_Duration);

				return false;
			}
		}
	}

	return true;
}

bool UVertexPaintDetectionComponent::PaintColorsTaskCheckValid(const FRVPDPApplyColorSetting& ColorSettings, UPrimitiveComponent* Component, bool IgnoreTaskQueueChecks) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_PaintColorsTaskCheckValid);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - PaintColorsTaskCheckValid");


	if (!CheckValidFundementals(ColorSettings, Component))
		return false;

	if (!CheckValidVertexPaintSettings(ColorSettings, IgnoreTaskQueueChecks))
		return false;

	if (!CheckValidVertexPaintColorSettings(ColorSettings))
		return false;

	return true;
}

bool UVertexPaintDetectionComponent::PaintTaskCheckValid(const FRVPDPPaintTaskSettings& PaintSettings, UPrimitiveComponent* Component) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_PaintTaskCheckValid);
	RVPDP_CPUPROFILER_STR("Vertex Paint Component - PaintTaskCheckValid");


	if (!CheckValidFundementals(PaintSettings, Component))
		return false;

	if (!CheckValidVertexPaintSettings(PaintSettings))
		return false;

	return true;
}
