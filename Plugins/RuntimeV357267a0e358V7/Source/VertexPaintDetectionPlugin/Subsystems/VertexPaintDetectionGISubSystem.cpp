// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 


#include "VertexPaintDetectionGISubSystem.h"

// Engine 
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/AssetData.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Materials/MaterialInstance.h"
#include "Async/Async.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"
#include "Components/SkeletalMeshComponent.h"

// Plugin
#include "VertexPaintDetectionTaskQueue.h"
#include "VertexPaintFunctionLibrary.h"
#include "VertexPaintMaterialDataAsset.h"
#include "VertexPaintDetectionInterface.h"
#include "VertexPaintDetectionComponent.h"
#include "VertexPaintDetectionSettings.h"
#include "VertexPaintColorSnippetRefs.h"
#include "VertexPaintOptimizationDataAsset.h"
#include "VertexPaintDetectionLog.h"
#include "VertexPaintDetectionProfiling.h"

// UE5
#if ENGINE_MAJOR_VERSION == 5

#include "Components/DynamicMeshComponent.h"
#include "GeometryScript/MeshVertexColorFunctions.h"

#if ENGINE_MINOR_VERSION >= 2
#include "StaticMeshComponentLODInfo.h"
#endif

#if ENGINE_MINOR_VERSION >= 3
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "GeometryCollection/GeometryCollection.h"
#include "GeometryCollection/GeometryCollectionObject.h"
#endif

#if ENGINE_MINOR_VERSION >= 4
#include "Rendering/RenderCommandPipes.h"
#endif

#endif



//--------------------------------------------------------

// Initialize

void UVertexPaintDetectionGISubSystem::Initialize(FSubsystemCollectionBase& Collection) {


	RVPDP_LOG(FRVPDPDebugSettings(false, 0, true), FColor::Cyan, "Game Instance Subsystem Initialized");


	// Caches these so they're in memory at all time
	if (auto vertexPaintDetectionSettings = GetDefault<UVertexPaintDetectionSettings>())
		ColorSnippetReferenceDataAsset = vertexPaintDetectionSettings->ColorSnippetReferencesDataAssetToUse.LoadSynchronous();

	if (auto vertexPaintDetectionSettings = GetDefault<UVertexPaintDetectionSettings>())
		OptimizationDataAsset = vertexPaintDetectionSettings->OptimizationDataAssetToUse.LoadSynchronous();

	if (auto vertexPaintDetectionSettings = GetDefault<UVertexPaintDetectionSettings>())
		MaterialDataAsset = vertexPaintDetectionSettings->MaterialsDataAssetToUse.LoadSynchronous();


	TaskQueue = NewObject<UVertexPaintDetectionTaskQueue>(this, UVertexPaintDetectionTaskQueue::StaticClass());

	if (TaskQueue) {

		RVPDP_LOG(FRVPDPDebugSettings(false, 0, true), FColor::Cyan, "Task Queue Successfully Created");

		TaskQueue->TaskFinishedDelegate.BindUObject(this, &UVertexPaintDetectionGISubSystem::WrapUpFinishedTask);
	}


	// Caches All Physical Materials so when we run UVertexPaintFunctionLibrary::GetPhysicalMaterialUsingPhysicsSurface we can just look through these instead of running GetAssetsByClass every time which is very expensive. 

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> AssetData;


#if ENGINE_MAJOR_VERSION == 4

	AssetRegistryModule.Get().GetAssetsByClass(UPhysicalMaterial::StaticClass()->GetFName(), AssetData, true);

#elif ENGINE_MAJOR_VERSION == 5

#if ENGINE_MINOR_VERSION == 0

	AssetRegistryModule.Get().GetAssetsByClass(UPhysicalMaterial::StaticClass()->GetFName(), AssetData, true);


#else

	AssetRegistryModule.Get().GetAssetsByClass(UPhysicalMaterial::StaticClass()->GetClassPathName(), AssetData, true);

#endif
#endif


	PhysicalMaterialAssets.Empty();

	for (int i = 0; i < AssetData.Num(); i++) {

		if (UPhysicalMaterial* physicalMaterial = Cast<UPhysicalMaterial>(AssetData[i].GetAsset())) {

			if (physicalMaterial->SurfaceType != EPhysicalSurface::SurfaceType_Default)
				PhysicalMaterialAssets.Add(physicalMaterial->SurfaceType, physicalMaterial);
		}
	}

	if (PhysicalMaterialAssets.Num() == 0) {

		RVPDP_LOG(FRVPDPDebugSettings(false, 0, true), FColor::Cyan, "NOTE The Plugins Plugin Game Instance Subsystem couldn't find any Physical Material Assets that had any physical surface set, so you will not be able to use GetPhysicalMaterialUsingPhysicsSurface function. \nIf you don't have any Physical Materials with physics surfaces set, then you can disregard this, otherwise you can in the Project Settings->Packaging, add Additional Directories to Cook, you can add the folder that has all the Physical Materials, then they will always be included in a Build and be Accessible.");
	}
}


//-------------------------------------------------------

// Tick

void UVertexPaintDetectionGISubSystem::Tick(float DeltaTime) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GameInstanceSubsystemTick);
	RVPDP_CPUPROFILER_STR("Vertex Paint Game Instance Subsystem - Tick");

	if (CleanUpVertexBufferInfo.Num() == 0) return;


	TArray<int> taskIDs;
	CleanUpVertexBufferInfo.GenerateKeyArray(taskIDs);

	TArray<FRVPDPCleanUpVertexBufferInfo> cleanUpBufferInfos;
	CleanUpVertexBufferInfo.GenerateValueArray(cleanUpBufferInfos);


	for (int i = taskIDs.Num() - 1; i >= 0; i--) {


		FRVPDPCleanUpVertexBufferInfo bufferInfo = cleanUpBufferInfos[i];
		bufferInfo.TimeeSinceVertexBufferReleasedResource += DeltaTime;

		// Cleans up old buffers after a duration when they should no longer be in use by the render thread. Not worth running it right away it may take a few frames before they get un-initialized
		if (bufferInfo.TimeeSinceVertexBufferReleasedResource > DelayUntilCleanUpVertexBuffer) {

			if (FColorVertexBuffer* vertexBuffer = bufferInfo.VertexColorBuffer) {

				if (!vertexBuffer->IsInitialized()) {


					CleanUpVertexBufferInfo.Remove(taskIDs[i]);


					ENQUEUE_RENDER_COMMAND(DeleteVertexBufferCommand)(
						[vertexBuffer](FRHICommandListImmediate& RHICmdList) {

						if (vertexBuffer && !vertexBuffer->IsInitialized()) {

							vertexBuffer->CleanUp();
							delete vertexBuffer;
						}
					});
				}

				else
				{

					// Failsafe in case it's still not un-initialzed. Without this and if we didn't start running the clean up until after a delay, there was rare occasions where if a mesh got destroyed the vertex buffer could still exist and never got un-initialized. 
					ENQUEUE_RENDER_COMMAND(DeleteVertexBuffer)(
						[vertexBuffer](FRHICommandListImmediate& RHICmdList) {

						if (vertexBuffer && vertexBuffer->IsInitialized()) {

							vertexBuffer->ReleaseResource();
						}
					});
				}
			}

			else
			{

				CleanUpVertexBufferInfo.Remove(taskIDs[i]);
			}
		}

		else {

			CleanUpVertexBufferInfo.Add(taskIDs[i], bufferInfo);
		}
	}
}


//-------------------------------------------------------

// Wrap Up Finished Task

void UVertexPaintDetectionGISubSystem::WrapUpFinishedTask(const FRVPDPCalculateColorsInfo& CalculateColorsInfo, bool TaskSuccessful) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_WrapUpFinishedTask);
	RVPDP_CPUPROFILER_STR("Vertex Paint Game Instance Subsystem - WrapUpFinishedTask");

	
	if (TaskSuccessful && CalculateColorsInfo.PaintTaskResult.AnyVertexColorGotChanged && CalculateColorsInfo.PaintTaskSettings.ApplyPaintJobToVertexColors) {

		ApplyVertexColorsToMeshComponent(CalculateColorsInfo.GetVertexPaintComponent(), CalculateColorsInfo.TaskID, CalculateColorsInfo.NewColorVertexBuffersPerLOD, CalculateColorsInfo.DynamicMeshComponentVertexColors, CalculateColorsInfo.GeometryCollectionComponentVertexColors);

		RVPDP_LOG(CalculateColorsInfo.TaskFundamentalSettings.DebugSettings, FColor::Green, "Paint Task Success and Change was Made - Applied Vertex Colors to Mesh! ");
	}

	else {

		// Cleans up the created color buffers that isn't going to be used because no change was made so we don't get memory leak
		if (CalculateColorsInfo.NewColorVertexBuffersPerLOD.Num() > 0) {

			for (int i = 0; i < CalculateColorsInfo.NewColorVertexBuffersPerLOD.Num(); i++) {

				if (CalculateColorsInfo.NewColorVertexBuffersPerLOD[i])
					AddColorVertexBufferToBufferCleanup(CalculateColorsInfo.TaskID, CalculateColorsInfo.NewColorVertexBuffersPerLOD[i]);
			}
		}
	}


	switch (CalculateColorsInfo.VertexPaintDetectionType) {

		case EVertexPaintDetectionType::GetClosestVertexDataDetection:


			GetClosestVertexDataOnMesh_WrapUp(TaskSuccessful, CalculateColorsInfo.GetClosestVertexDataSettings, CalculateColorsInfo.TaskResult, CalculateColorsInfo.GetClosestVertexDataResult, CalculateColorsInfo.GetClosestVertexData_AverageColorInArea, CalculateColorsInfo.GetClosestVertexData_EstimatedColorAtHitLocationResult, CalculateColorsInfo.TaskFundamentalSettings);
			break;

		case EVertexPaintDetectionType::GetAllVertexColorDetection:


			GetAllVertexColorsOnly_WrapUp(TaskSuccessful, CalculateColorsInfo.GetAllVertexColorsSettings, CalculateColorsInfo.TaskResult, CalculateColorsInfo.TaskFundamentalSettings);
			break;

		case EVertexPaintDetectionType::GetColorsWithinArea:


			GetColorsWithinArea_WrapUp(TaskSuccessful, CalculateColorsInfo.GetColorsWithinAreaSettings, CalculateColorsInfo.WithinArea_Results_BeforeApplyingColors, CalculateColorsInfo.TaskResult, CalculateColorsInfo.TaskFundamentalSettings);
			break;

		case EVertexPaintDetectionType::PaintAtLocation:


			// Detection Before painting is applied, so you can for instance run proper SFX if a rain drop falls on a dry jacket before it gets wet
			if (CalculateColorsInfo.PaintAtLocationSettings.GetClosestVertexDataCombo.RunGetClosestVertexDataOnMeshBeforeApplyingPaint) {

				GetClosestVertexDataOnMesh_WrapUp(TaskSuccessful, CalculateColorsInfo.GetClosestVertexDataSettings, CalculateColorsInfo.DetectComboTaskResults, CalculateColorsInfo.GetClosestVertexDataResult, CalculateColorsInfo.GetClosestVertexData_AverageColorInArea, CalculateColorsInfo.GetClosestVertexData_EstimatedColorAtHitLocationResult, CalculateColorsInfo.TaskFundamentalSettings);
			}

			PaintOnMeshAtLocation_WrapUp(TaskSuccessful, CalculateColorsInfo.PaintAtLocationSettings, CalculateColorsInfo.TaskResult, CalculateColorsInfo.PaintTaskResult, CalculateColorsInfo.GetClosestVertexDataResult, CalculateColorsInfo.PaintAtLocation_AverageColorInArea, CalculateColorsInfo.GetClosestVertexData_EstimatedColorAtHitLocationResult, CalculateColorsInfo.TaskFundamentalSettings);


			break;

		case EVertexPaintDetectionType::PaintWithinArea:


			if (CalculateColorsInfo.PaintWithinAreaSettings.GetColorsWithinAreaCombo) {

				GetColorsWithinArea_WrapUp(TaskSuccessful, CalculateColorsInfo.GetColorsWithinAreaSettings, CalculateColorsInfo.WithinArea_Results_BeforeApplyingColors, CalculateColorsInfo.DetectComboTaskResults, CalculateColorsInfo.TaskFundamentalSettings);
			}

			PaintWithinArea_WrapUp(TaskSuccessful, CalculateColorsInfo.PaintWithinAreaSettings, CalculateColorsInfo.WithinArea_Results_AfterApplyingColors, CalculateColorsInfo.TaskResult, CalculateColorsInfo.PaintTaskResult, CalculateColorsInfo.TaskFundamentalSettings);


			break;

		case EVertexPaintDetectionType::PaintEntireMesh:

				
			PaintOnEntireMesh_WrapUp(TaskSuccessful, CalculateColorsInfo.PaintOnEntireMeshSettings, CalculateColorsInfo.TaskResult, CalculateColorsInfo.PaintTaskResult, CalculateColorsInfo.TaskFundamentalSettings);

			break;

		case EVertexPaintDetectionType::PaintColorSnippet:


			PaintColorSnippet_WrapUp(TaskSuccessful, CalculateColorsInfo.PaintColorSnippetSettings, CalculateColorsInfo.TaskResult, CalculateColorsInfo.PaintTaskResult, CalculateColorsInfo.TaskFundamentalSettings);

			break;

		case EVertexPaintDetectionType::SetMeshVertexColorsDirectly:


			SetMeshComponentVertexColors_WrapUp(TaskSuccessful, CalculateColorsInfo.SetVertexColorsSettings, CalculateColorsInfo.TaskResult, CalculateColorsInfo.PaintTaskResult, CalculateColorsInfo.TaskFundamentalSettings);

			break;

		case EVertexPaintDetectionType::SetMeshVertexColorsDirectlyUsingSerializedString:


			SetMeshComponentVertexColorsUsingSerializedString_WrapUp(TaskSuccessful, CalculateColorsInfo.SetVertexColorsUsingSerializedStringSettings, CalculateColorsInfo.TaskResult, CalculateColorsInfo.PaintTaskResult, CalculateColorsInfo.TaskFundamentalSettings);

			break;

		default:
			break;
	}


	// Update Chaos Cloth Physics 
	if (TaskSuccessful && IsValid(CalculateColorsInfo.ApplyColorSettings.GetMeshComponent())) {

		// Affected Cloth Physics is exclusive to UE5 and ChaosCloth
#if ENGINE_MAJOR_VERSION == 5

		if (IsValid(CalculateColorsInfo.GetVertexPaintSkelComponent()) && CalculateColorsInfo.PaintTaskSettings.AffectClothPhysics) {

			if (CalculateColorsInfo.ClothPhysicsSettings.Num() > 0) {

				for (auto& clothPhysSettings : CalculateColorsInfo.ClothPhysicsSettings) {

					// Sets Chaos Cloth Physics on the Game Thread as it seems it could rarely cause a crash on the Async Thread
					UVertexPaintFunctionLibrary::SetChaosClothPhysics(CalculateColorsInfo.GetVertexPaintSkelComponent(), clothPhysSettings.Key, clothPhysSettings.Value);
				}
			}

			// If the task run didn't loop through all verts, i.e. didn't set CalculateColorsInfo.ClothPhysicsSettings but are set to affect cloth physics, then we manually have to run Update Chaos Cloth. This can occur if it's a Paint Entire Mesh, like something that dries a character, where we would still want to update the cloth, if they have any. 
			else {

				// Runs this after we've run Wrap Ups Above, i.e. after colors should've been updated. 
				UVertexPaintFunctionLibrary::UpdateChaosClothPhysicsWithExistingColors(CalculateColorsInfo.GetVertexPaintSkelComponent());
			}
		}

#endif
	}
}


//-------------------------------------------------------

// Get Closest Vertex Data On Mesh Wrap Up

void UVertexPaintDetectionGISubSystem::GetClosestVertexDataOnMesh_WrapUp(bool TaskSuccessful, const FRVPDPGetClosestVertexDataSettings& GetClosestVertexDataSettings, const FRVPDPTaskResults& DetectTaskResult, const FRVPDPClosestVertexDataResults& GetClosestVertexDataResult, const FRVPDPAverageColorInAreaInfo& AverageColorInArea, const FRVPDPEstimatedColorAtHitLocationInfo& EstimatedColorAtHitLocationResult, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetClosestVertexDataOnMeshWrapUp);
	RVPDP_CPUPROFILER_STR("Vertex Paint Game Instance Subsystem - GetClosestVertexDataOnMeshWrapUp");


	if (TaskSuccessful && GetClosestVertexDataResult.ClosestVertexDataSuccessfullyAcquired) {


		RVPDP_LOG(TaskFundamentalSettings.DebugSettings, FColor::Green, "Get Closest Vertex Data Success");


		DrawClosestVertexDebugSymbols(GetClosestVertexDataResult, GetClosestVertexDataSettings.GetAverageColorInAreaSettings.AreaRangeToGetAvarageColorFrom, TaskFundamentalSettings.DebugSettings);


		if (EstimatedColorAtHitLocationResult.EstimatedColorAtHitLocationDataSuccessfullyAcquired) {

			DrawEstimatedColorAtHitLocationDebugSymbols(EstimatedColorAtHitLocationResult, GetClosestVertexDataResult.ClosestVertexGeneralInfo.ClosestVertexPositionInfo.ClosestVertexPositionWorld, GetClosestVertexDataSettings.HitFundementals.HitLocation, EstimatedColorAtHitLocationResult.VertexWeEstimatedTowardWorldLocation, EstimatedColorAtHitLocationResult.ClosestVerticesWorldLocations, TaskFundamentalSettings.DebugSettings);
		}
	}

	else {

		RVPDP_LOG(TaskFundamentalSettings.DebugSettings, FColor::Red, "Get Closest Vertex Data Failed");
	}



	if (IsValid(TaskQueue)) {

		if (UVertexPaintDetectionComponent* vertexPaintComponent = DetectTaskResult.GetAssociatedPaintComponent()) {

			vertexPaintComponent->GetClosestVertexDataOnMeshTaskFinished(DetectTaskResult, GetClosestVertexDataSettings, GetClosestVertexDataResult, EstimatedColorAtHitLocationResult, AverageColorInArea, TaskFundamentalSettings, TaskQueue->GetAdditionalDataToPassThroughForTask(DetectTaskResult.TaskID), false);
		}

		UVertexPaintFunctionLibrary::RunGetClosestVertexDataCallbackInterfaces(DetectTaskResult, GetClosestVertexDataSettings, GetClosestVertexDataResult, EstimatedColorAtHitLocationResult, AverageColorInArea, TaskFundamentalSettings, TaskQueue->GetAdditionalDataToPassThroughForTask(DetectTaskResult.TaskID));

		UVertexPaintFunctionLibrary::RunFinishedDetectTaskCallbacks(DetectTaskResult, TaskFundamentalSettings, TaskQueue->GetAdditionalDataToPassThroughForTask(DetectTaskResult.TaskID));
	}
}


//-------------------------------------------------------

// Get All Vertex Colors Only Wrap Up

void UVertexPaintDetectionGISubSystem::GetAllVertexColorsOnly_WrapUp(bool TaskSuccessful, const FRVPDPGetColorsOnlySettings& GetAllVertexColorsSettings, const FRVPDPTaskResults& DetectTaskResult, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings) const {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetAllVertexColorsOnly);
	RVPDP_CPUPROFILER_STR("Vertex Paint Game Instance Subsystem - GetAllVertexColorsOnly");


	if (TaskSuccessful)
		RVPDP_LOG(TaskFundamentalSettings.DebugSettings, FColor::Green, "Get All Vertex Colors Only Success");
	else 
		RVPDP_LOG(TaskFundamentalSettings.DebugSettings, FColor::Red, "Get All Vertex Colors Only Failed");


	if (IsValid(TaskQueue)) {

		if (UVertexPaintDetectionComponent* vertexPaintComponent = DetectTaskResult.GetAssociatedPaintComponent()) {

			vertexPaintComponent->GetAllVertexColorsOnlyTaskFinished(DetectTaskResult, GetAllVertexColorsSettings, TaskFundamentalSettings, TaskQueue->GetAdditionalDataToPassThroughForTask(DetectTaskResult.TaskID), false);
		}

		UVertexPaintFunctionLibrary::RunGetAllColorsOnlyCallbackInterfaces(DetectTaskResult, GetAllVertexColorsSettings, TaskFundamentalSettings, TaskQueue->GetAdditionalDataToPassThroughForTask(DetectTaskResult.TaskID));

		UVertexPaintFunctionLibrary::RunFinishedDetectTaskCallbacks(DetectTaskResult, TaskFundamentalSettings, TaskQueue->GetAdditionalDataToPassThroughForTask(DetectTaskResult.TaskID));
	}
}


//-------------------------------------------------------

// Get Colors Within Area Wrap Up

void UVertexPaintDetectionGISubSystem::GetColorsWithinArea_WrapUp(bool TaskSuccessful, const FRVPDPGetColorsWithinAreaSettings& GetColorsWithinAreaSettings, const FRVPDPWithinAreaResults& WithinAreaResults, const FRVPDPTaskResults& DetectTaskResult, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings) const {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetColorsWithinArea);
	RVPDP_CPUPROFILER_STR("Vertex Paint Game Instance Subsystem - GetColorsWithinArea");


	if (TaskSuccessful)
		RVPDP_LOG(TaskFundamentalSettings.DebugSettings, FColor::Green, "Get Colors Within Area Success");
	else 
		RVPDP_LOG(TaskFundamentalSettings.DebugSettings, FColor::Red, "Get Colors Within Area Failed");


	if (IsValid(TaskQueue)) {

		if (UVertexPaintDetectionComponent* vertexPaintComponent = DetectTaskResult.GetAssociatedPaintComponent()) {

			vertexPaintComponent->GetColorsWithinAreaTaskFinished(DetectTaskResult, GetColorsWithinAreaSettings, WithinAreaResults, TaskFundamentalSettings, TaskQueue->GetAdditionalDataToPassThroughForTask(DetectTaskResult.TaskID), false);
		}


		UVertexPaintFunctionLibrary::RunGetColorsWithinAreaCallbackInterfaces(DetectTaskResult, GetColorsWithinAreaSettings, WithinAreaResults, TaskFundamentalSettings, TaskQueue->GetAdditionalDataToPassThroughForTask(DetectTaskResult.TaskID));

		UVertexPaintFunctionLibrary::RunFinishedDetectTaskCallbacks(DetectTaskResult, TaskFundamentalSettings, TaskQueue->GetAdditionalDataToPassThroughForTask(DetectTaskResult.TaskID));
	}
}


//-------------------------------------------------------

// Paint On Mesh At Location Wrap Up

void UVertexPaintDetectionGISubSystem::PaintOnMeshAtLocation_WrapUp(bool TaskSuccessful, const FRVPDPPaintAtLocationSettings& PaintAtLocationSettings, const FRVPDPTaskResults& TaskResult, const FRVPDPPaintTaskResultInfo& PaintTaskResult, const FRVPDPClosestVertexDataResults& GetClosestVertexDataResult, const FRVPDPAverageColorInAreaInfo& AverageColorInArea, const FRVPDPEstimatedColorAtHitLocationInfo& EstimatedColorAtHitLocationResult, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_PaintOnMeshAtLocation);
	RVPDP_CPUPROFILER_STR("Vertex Paint Game Instance Subsystem - PaintOnMeshAtLocation");


	if (TaskSuccessful) {

		RVPDP_LOG(TaskFundamentalSettings.DebugSettings, FColor::Green, "Paint on Mesh at Location Success");

		if (GetClosestVertexDataResult.ClosestVertexDataSuccessfullyAcquired) {

			DrawClosestVertexDebugSymbols(GetClosestVertexDataResult, PaintAtLocationSettings.PaintAtAreaSettings.AreaOfEffectRangeEnd, TaskFundamentalSettings.DebugSettings);
		}


		if (EstimatedColorAtHitLocationResult.EstimatedColorAtHitLocationDataSuccessfullyAcquired) {

			DrawEstimatedColorAtHitLocationDebugSymbols(EstimatedColorAtHitLocationResult, GetClosestVertexDataResult.ClosestVertexGeneralInfo.ClosestVertexPositionInfo.ClosestVertexPositionWorld, PaintAtLocationSettings.HitFundementals.HitLocation, EstimatedColorAtHitLocationResult.VertexWeEstimatedTowardWorldLocation, EstimatedColorAtHitLocationResult.ClosestVerticesWorldLocations, TaskFundamentalSettings.DebugSettings);
		}
	}

	else {

		RVPDP_LOG(TaskFundamentalSettings.DebugSettings, FColor::Red, "Paint on Mesh at Location Failed");
	}


	if (IsValid(TaskQueue)) {

		if (UVertexPaintDetectionComponent* vertexPaintComponent = TaskResult.GetAssociatedPaintComponent()) {

			vertexPaintComponent->PaintOnMeshAtLocationTaskFinished(TaskResult, PaintTaskResult, PaintAtLocationSettings, GetClosestVertexDataResult, EstimatedColorAtHitLocationResult, AverageColorInArea, TaskFundamentalSettings, TaskQueue->GetAdditionalDataToPassThroughForTask(TaskResult.TaskID), false);
		}

		UVertexPaintFunctionLibrary::RunPaintAtLocationCallbackInterfaces(TaskResult, PaintTaskResult, PaintAtLocationSettings, GetClosestVertexDataResult, EstimatedColorAtHitLocationResult, AverageColorInArea, TaskFundamentalSettings, TaskQueue->GetAdditionalDataToPassThroughForTask(TaskResult.TaskID));

		UVertexPaintFunctionLibrary::RunFinishedPaintTaskCallbacks(TaskResult, PaintTaskResult, TaskFundamentalSettings, TaskQueue->GetAdditionalDataToPassThroughForTask(TaskResult.TaskID));
	}
}


//-------------------------------------------------------

// Paint Within Area Wrap Up

void UVertexPaintDetectionGISubSystem::PaintWithinArea_WrapUp(bool TaskSuccessful, const FRVPDPPaintWithinAreaSettings& PaintWithinAreaSettings, const FRVPDPWithinAreaResults& WithinAreaResult, const FRVPDPTaskResults& TaskResult, const FRVPDPPaintTaskResultInfo& PaintTaskResult, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings) const {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_PaintWithinArea);
	RVPDP_CPUPROFILER_STR("Vertex Paint Game Instance Subsystem - PaintWithinArea");


	if (TaskSuccessful)
		RVPDP_LOG(TaskFundamentalSettings.DebugSettings, FColor::Green, "Paint on Mesh Within Area Success");
	else
		RVPDP_LOG(TaskFundamentalSettings.DebugSettings, FColor::Red, "Paint on Mesh Within Area Failed");


	if (IsValid(TaskQueue)) {


		if (UVertexPaintDetectionComponent* vertexPaintComponent = TaskResult.GetAssociatedPaintComponent()) {

			vertexPaintComponent->PaintOnMeshWithinAreaTaskFinished(TaskResult, PaintTaskResult, PaintWithinAreaSettings, WithinAreaResult, TaskFundamentalSettings, TaskQueue->GetAdditionalDataToPassThroughForTask(TaskResult.TaskID), false);
		}


		UVertexPaintFunctionLibrary::RunPaintWithinAreaCallbackInterfaces(TaskResult, PaintTaskResult, PaintWithinAreaSettings, WithinAreaResult, TaskFundamentalSettings, TaskQueue->GetAdditionalDataToPassThroughForTask(TaskResult.TaskID));

		UVertexPaintFunctionLibrary::RunFinishedPaintTaskCallbacks(TaskResult, PaintTaskResult, TaskFundamentalSettings, TaskQueue->GetAdditionalDataToPassThroughForTask(TaskResult.TaskID));
	}
}


//-------------------------------------------------------

// Paint Within Area Wrap Up

void UVertexPaintDetectionGISubSystem::PaintOnEntireMesh_WrapUp(bool TaskSuccessful, const FRVPDPPaintOnEntireMeshSettings& PaintOnEntireMeshSettings, const FRVPDPTaskResults& TaskResult, const FRVPDPPaintTaskResultInfo& PaintTaskResult, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings) const {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_PaintOnEntireMesh);
	RVPDP_CPUPROFILER_STR("Vertex Paint Game Instance Subsystem - PaintOnEntireMesh");


	if (TaskSuccessful)
		RVPDP_LOG(TaskFundamentalSettings.DebugSettings, FColor::Green, "Paint on Entire Mesh Success");
	else
		RVPDP_LOG(TaskFundamentalSettings.DebugSettings, FColor::Red, "Paint on Entire Mesh Failed");


	if (IsValid(TaskQueue)) {

		if (UVertexPaintDetectionComponent* vertexPaintComponent = TaskResult.GetAssociatedPaintComponent()) {

			vertexPaintComponent->PaintOnEntireMeshTaskFinished(TaskResult, PaintTaskResult, PaintOnEntireMeshSettings, TaskFundamentalSettings, TaskQueue->GetAdditionalDataToPassThroughForTask(TaskResult.TaskID), false);
		}

		UVertexPaintFunctionLibrary::RunPaintEntireMeshCallbackInterfaces(TaskResult, PaintTaskResult, PaintOnEntireMeshSettings, TaskFundamentalSettings, TaskQueue->GetAdditionalDataToPassThroughForTask(TaskResult.TaskID));

		UVertexPaintFunctionLibrary::RunFinishedPaintTaskCallbacks(TaskResult, PaintTaskResult, TaskFundamentalSettings, TaskQueue->GetAdditionalDataToPassThroughForTask(TaskResult.TaskID));
	}
}


//-------------------------------------------------------

// Paint Color Snippet Wrap Up

void UVertexPaintDetectionGISubSystem::PaintColorSnippet_WrapUp(bool TaskSuccessful, const FRVPDPPaintColorSnippetSettings& PaintColorSnippetSettings, const FRVPDPTaskResults& TaskResult, const FRVPDPPaintTaskResultInfo& PaintTaskResult, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings) const {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_PaintColorSnippet);
	RVPDP_CPUPROFILER_STR("Vertex Paint Game Instance Subsystem - PaintColorSnippet");


	if (TaskSuccessful)
		RVPDP_LOG(TaskFundamentalSettings.DebugSettings, FColor::Green, "Paint Color Snippet Success");
	else
		RVPDP_LOG(TaskFundamentalSettings.DebugSettings, FColor::Red, "Paint Color Snippet Failed");


	if (IsValid(TaskQueue)) {

		if (UVertexPaintDetectionComponent* vertexPaintComponent = TaskResult.GetAssociatedPaintComponent()) {

			vertexPaintComponent->PaintColorSnippetOnMeshTaskFinished(TaskResult, PaintTaskResult, PaintColorSnippetSettings, TaskFundamentalSettings, TaskQueue->GetAdditionalDataToPassThroughForTask(TaskResult.TaskID), false);
		}

		UVertexPaintFunctionLibrary::RunPaintColorSnippetCallbackInterfaces(TaskResult, PaintTaskResult, PaintColorSnippetSettings, TaskFundamentalSettings, TaskQueue->GetAdditionalDataToPassThroughForTask(TaskResult.TaskID));

		UVertexPaintFunctionLibrary::RunFinishedPaintTaskCallbacks(TaskResult, PaintTaskResult, TaskFundamentalSettings, TaskQueue->GetAdditionalDataToPassThroughForTask(TaskResult.TaskID));
	}
}


//-------------------------------------------------------

// Set Mesh Component Vertex Colors Wrap Up

void UVertexPaintDetectionGISubSystem::SetMeshComponentVertexColors_WrapUp(bool TaskSuccessful, const FRVPDPSetVertexColorsSettings& SetVertexColorsSettings, const FRVPDPTaskResults& TaskResult, const FRVPDPPaintTaskResultInfo& PaintTaskResult, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings) const {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_SetMeshComponentVertexColors);
	RVPDP_CPUPROFILER_STR("Vertex Paint Game Instance Subsystem - SetMeshComponentVertexColors");


	if (TaskSuccessful)
		RVPDP_LOG(TaskFundamentalSettings.DebugSettings, FColor::Green, "Set Mesh Component Vertex Colors Success");
	else
		RVPDP_LOG(TaskFundamentalSettings.DebugSettings, FColor::Red, "Set Mesh Component Vertex Colors Failed");


	if (IsValid(TaskQueue)) {

		if (UVertexPaintDetectionComponent* vertexPaintComponent = TaskResult.GetAssociatedPaintComponent()) {

			vertexPaintComponent->SetMeshComponentVertexColorsTaskFinished(TaskResult, PaintTaskResult, SetVertexColorsSettings, TaskFundamentalSettings, TaskQueue->GetAdditionalDataToPassThroughForTask(TaskResult.TaskID), false);
		}

		UVertexPaintFunctionLibrary::RunPaintSetMeshColorsCallbackInterfaces(TaskResult, PaintTaskResult, SetVertexColorsSettings, TaskFundamentalSettings, TaskQueue->GetAdditionalDataToPassThroughForTask(TaskResult.TaskID));

		UVertexPaintFunctionLibrary::RunFinishedPaintTaskCallbacks(TaskResult, PaintTaskResult, TaskFundamentalSettings, TaskQueue->GetAdditionalDataToPassThroughForTask(TaskResult.TaskID));
	}
}


//-------------------------------------------------------

// Set Mesh Component Vertex Colors Using Serialized String Wrap Up

void UVertexPaintDetectionGISubSystem::SetMeshComponentVertexColorsUsingSerializedString_WrapUp(bool TaskSuccessful, const FRVPDPSetVertexColorsUsingSerializedStringSettings& SetVertexColorsUsingSerializedStringSettings, const FRVPDPTaskResults& TaskResult, const FRVPDPPaintTaskResultInfo& PaintTaskResult, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings) const {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_SetMeshComponentVertexColorsUsingSerializedString);
	RVPDP_CPUPROFILER_STR("Vertex Paint Game Instance Subsystem - SetMeshComponentVertexColorsUsingSerializedString");


	if (TaskSuccessful)
		RVPDP_LOG(TaskFundamentalSettings.DebugSettings, FColor::Green, "Set Mesh Component Vertex Colors Using Serialized String Success");
	else
		RVPDP_LOG(TaskFundamentalSettings.DebugSettings, FColor::Red, "Set Mesh Component Vertex Colors Using Serialized String Failed");


	if (IsValid(TaskQueue)) {

		if (UVertexPaintDetectionComponent* vertexPaintComponent = TaskResult.GetAssociatedPaintComponent()) {

			vertexPaintComponent->SetMeshComponentVertexColorsUsingSerializedStringTaskFinished(TaskResult, PaintTaskResult, SetVertexColorsUsingSerializedStringSettings, TaskFundamentalSettings, TaskQueue->GetAdditionalDataToPassThroughForTask(TaskResult.TaskID), false);
		}

		UVertexPaintFunctionLibrary::RunPaintSetMeshColorsUsingSerializedStringCallbackInterfaces(TaskResult, PaintTaskResult, SetVertexColorsUsingSerializedStringSettings, TaskFundamentalSettings, TaskQueue->GetAdditionalDataToPassThroughForTask(TaskResult.TaskID));

		UVertexPaintFunctionLibrary::RunFinishedPaintTaskCallbacks(TaskResult, PaintTaskResult, TaskFundamentalSettings, TaskQueue->GetAdditionalDataToPassThroughForTask(TaskResult.TaskID));
	}
}


//--------------------------------------------------------

// Apply Vertex Colors To Mesh Component

void UVertexPaintDetectionGISubSystem::ApplyVertexColorsToMeshComponent(UPrimitiveComponent* MeshComponent, int TaskID, TArray<FColorVertexBuffer*> NewColorVertexBuffers, const TArray<FColor>& DynamicMeshComponentVertexColors, const TArray<FLinearColor>& GeometryCollectionComponentVertexColors) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_ApplyVertexColorsToMeshComponent);
	RVPDP_CPUPROFILER_STR("Vertex Paint Game Instance Subsystem - ApplyVertexColorsToMeshComponent");


	if (UStaticMeshComponent* staticMeshComponent = Cast<UStaticMeshComponent>(MeshComponent)) {


		int lodMax = NewColorVertexBuffers.Num();

		// Had to run this as it seemed spawned meshes with 1 LOD didn't get painted properly otherwise. 
		if (staticMeshComponent->LODData.Num() < lodMax)
			staticMeshComponent->SetLODDataCount(lodMax, lodMax);


		for (int i = 0; i < NewColorVertexBuffers.Num(); i++) {
			
			// Makes sure to clean up any buffer that are empty
			if (NewColorVertexBuffers[i] && NewColorVertexBuffers[i]->GetNumVertices() <= 0) {

				AddColorVertexBufferToBufferCleanup(TaskID, NewColorVertexBuffers[i]);
				continue;
			}


			// If have been overriden before
			if (staticMeshComponent->LODData[i].OverrideVertexColors) {

				// NOTE Causes big FPS drops which is especially noticable for builds in 5.1 and up!
				// LODInfo->ReleaseOverrideVertexColorsAndBlock();

				// Adds it to a array of old color buffers that we .CleanUp after a short delay when it is safe. RenderThread is 1-2 frames behind GameThread
				AddColorVertexBufferToBufferCleanup(TaskID, staticMeshComponent->LODData[i].OverrideVertexColors);
				staticMeshComponent->LODData[i].OverrideVertexColors = nullptr;
			}


			staticMeshComponent->LODData[i].OverrideVertexColors = NewColorVertexBuffers[i];

			// Initialize resource and mark render state of object as dirty in order for the engine to re-render it
			BeginInitResource(staticMeshComponent->LODData[i].OverrideVertexColors);
		}

		staticMeshComponent->MarkRenderStateDirty();
	}


	else if (USkeletalMeshComponent* skeletalMeshComponent = Cast<USkeletalMeshComponent>(MeshComponent)) {


		for (int i = 0; i < NewColorVertexBuffers.Num(); i++) {

			// Cleans up any that are empty
			if (NewColorVertexBuffers[i] && NewColorVertexBuffers[i]->GetNumVertices() <= 0) {

				AddColorVertexBufferToBufferCleanup(TaskID, NewColorVertexBuffers[i]);
				continue;
			}


			if (skeletalMeshComponent->LODInfo[i].OverrideVertexColors) {

				// NOTE Causes big FPS drops which is especially noticable for builds in 5.1 and up!
				// LODInfo->ReleaseOverrideVertexColorsAndBlock();

				// Adds it to a array of old color buffers that we .CleanUp after a short delay when it is safe. RenderThread is 1-2 frames behind GameThread
				AddColorVertexBufferToBufferCleanup(TaskID, skeletalMeshComponent->LODInfo[i].OverrideVertexColors);

				skeletalMeshComponent->LODInfo[i].OverrideVertexColors = nullptr;
			}

			skeletalMeshComponent->LODInfo[i].OverrideVertexColors = NewColorVertexBuffers[i];

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4

			BeginInitResource(skeletalMeshComponent->LODInfo[i].OverrideVertexColors, &UE::RenderCommandPipe::SkeletalMesh);
#else
			BeginInitResource(skeletalMeshComponent->LODInfo[i].OverrideVertexColors);
#endif
		}

		skeletalMeshComponent->MarkRenderStateDirty();
	}


#if ENGINE_MAJOR_VERSION == 5

	else if (UDynamicMeshComponent* dynamicMeshComponent = Cast<UDynamicMeshComponent>(MeshComponent)) {


		if (DynamicMeshComponentVertexColors.Num() > 0) {


			UE::Geometry::FDynamicMesh3* dynamicMesh3 = nullptr;
			dynamicMesh3 = &dynamicMeshComponent->GetDynamicMesh()->GetMeshRef();


			if (dynamicMesh3) {


				// In order to get the Current Color from Vertex Info we couldn't just run SetMeshPerVertexColors(), we had to actually run ESetVertexColor with the value as well!
				for (int i = 0; i < dynamicMesh3->MaxVertexID(); i++) {

					if (!dynamicMesh3->IsVertex(i)) continue;
					if (!DynamicMeshComponentVertexColors.IsValidIndex(i)) continue;


					const FColor vertexColor = DynamicMeshComponentVertexColors[i];

					dynamicMesh3->SetVertexColor(i, FVector3f(vertexColor.R, vertexColor.G, vertexColor.B));
				}


				FGeometryScriptColorList colorList;
				colorList.List = MakeShared<TArray<FLinearColor>>(DynamicMeshComponentVertexColors);

				// NOTE Very heavy since we have to run it here on the game thread and it has 2 deep loops. So we can drop like 80 FPS by this if it's a detailed dynamic mesh
				UGeometryScriptLibrary_MeshVertexColorFunctions::SetMeshPerVertexColors(dynamicMeshComponent->GetDynamicMesh(), colorList, nullptr);
			}
		}
	}

		
#if WITH_EDITOR

#if ENGINE_MINOR_VERSION >= 3

	// Geometry Collection Components can currently only be painted in Editor time because geometryCollection->RebuildRenderData() is Editor Only

	else if (UGeometryCollectionComponent* geometryCollectionComponent = Cast<UGeometryCollectionComponent>(MeshComponent)) {

		if (GeometryCollectionComponentVertexColors.Num() > 0) {

			if (const UGeometryCollection* geometryCollection = geometryCollectionComponent->GetRestCollection()) {


				geometryCollection->GetGeometryCollection().Get()->Color = GeometryCollectionComponentVertexColors;

				auto geoCollection = const_cast<UGeometryCollection*>(geometryCollection);

				// This fixed so RebuildRenderData actually updates the Render Data everytime we run it instead of just once. 
				geoCollection->InvalidateCollection();

				// Unfortunately this is Editor Only
				geoCollection->RebuildRenderData();


				/*
					Unfortunately the proxy mesh (not the actual collision) gets rendered at the default Location of the geo collection component after RebuildRenderData.
					I couldn't find a reliable way of resetting it to the correct place which is geometryCollectionComponent->GetRootCurrentTransform() Location.

					Waking it up worked somewhat, but if painting every single frame, it got reset so it looked like it was a different spot. Marking Render State dirty didn't work, if run 1 frame after it sometimes worked, if 2 frames after it never worked.

					But since this feature is very experimental, and we can't even run RebuildRenderData in packaged build but only in Editor, we can return to this later on.
				*/


				// Wonder if a better way is to Reset it from an Updated TManagedArray, with the new colors etc., instead of RebuildRenderData. May get heavy if it has to loop through vertices on the game thread though, and feels like we're just replacing one ugly solution with another ugly one.  
				// geometryCollection->ResetFrom
			}
		}
	}

#endif // UE5.3 or up
#endif // WITH EDITOR

#endif // UE5

}


//--------------------------------------------------------

// Add Color Vertex Buffer To Buffer Cleanup

void UVertexPaintDetectionGISubSystem::AddColorVertexBufferToBufferCleanup(int TaskID, FColorVertexBuffer* VertexColorBuffer) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_AddColorVertexBufferToBufferCleanup);
	RVPDP_CPUPROFILER_STR("Vertex Paint Game Instance Subsystem - AddColorVertexBufferToBufferCleanup");

	if (!VertexColorBuffer) return;
	if (CleanUpVertexBufferInfo.Contains(TaskID)) return;


	FRVPDPCleanUpVertexBufferInfo cleanUpVertexBufferInfo;
	cleanUpVertexBufferInfo.TaskID = TaskID;
	cleanUpVertexBufferInfo.VertexColorBuffer = VertexColorBuffer;
	CleanUpVertexBufferInfo.Add(TaskID, cleanUpVertexBufferInfo);


	// Releases Resource on the Render Thread so we can Delete the vertex buffer ptr after a short delay when it isn't used by the render thread.
	if (VertexColorBuffer->IsInitialized()) {

		ENQUEUE_RENDER_COMMAND(DeleteVertexBuffer)(
			[VertexColorBuffer](FRHICommandListImmediate& RHICmdList) {

			if (VertexColorBuffer && VertexColorBuffer->IsInitialized()) {

				VertexColorBuffer->ReleaseResource();
			}
		});
	}
}


//--------------------------------------------------------

// Register Callback From Specific Mesh Component

void UVertexPaintDetectionGISubSystem::RegisterPaintTaskCallbacksToObjectFromSpecifiedMeshComponent(UPrimitiveComponent* MeshComponent, UObject* ObjectToRegisterForCallbacks) {

	if (!IsValid(MeshComponent)) return;
	if (!IsValid(MeshComponent->GetOwner())) return;
	if (!IsValid(ObjectToRegisterForCallbacks)) return;


	FRVPDPCallbackFromSpecifiedMeshComponentsInfo callbackOnObjectsInfo;

	if (PaintTaskCallbacksFromSpecificMeshComponents.Contains(MeshComponent)) {

		callbackOnObjectsInfo = PaintTaskCallbacksFromSpecificMeshComponents.FindRef(MeshComponent);

		if (callbackOnObjectsInfo.RunCallbacksOnObjects.Contains(ObjectToRegisterForCallbacks)) return;
	}

	else if (!MeshComponent->GetOwner()->OnDestroyed.IsAlreadyBound(this, &UVertexPaintDetectionGISubSystem::RegisteredPaintTaskCallbackOwnerDestroyed)) {

		MeshComponent->GetOwner()->OnDestroyed.AddDynamic(this, &UVertexPaintDetectionGISubSystem::RegisteredPaintTaskCallbackOwnerDestroyed);
	}

	callbackOnObjectsInfo.RunCallbacksOnObjects.Add(ObjectToRegisterForCallbacks);
	PaintTaskCallbacksFromSpecificMeshComponents.Add(MeshComponent, callbackOnObjectsInfo);


	// Activates a looping timer that verifies that the meshes are still valid, since they can get destroyed eventhough the Actor doesn't. 
	if (!GetWorld()->GetTimerManager().IsTimerActive(VerifyCallbackSpecificMeshComponentsTimer)) {

		GetWorld()->GetTimerManager().SetTimer(VerifyCallbackSpecificMeshComponentsTimer, this, &UVertexPaintDetectionGISubSystem::VerifyCallbackSpecificMeshComponents, VerifyCallbackSpecificMeshComponentsIntervall, true);
	}
}


//--------------------------------------------------------

// Un-Register Callback From Specific Mesh Component

void UVertexPaintDetectionGISubSystem::UnRegisterPaintTaskCallbacksToObjectFromSpecifiedMeshComponent(UPrimitiveComponent* MeshComponent, UObject* ObjectToUnregisterForCallbacks) {

	if (!IsValid(MeshComponent)) return;
	if (!IsValid(MeshComponent->GetOwner())) return;
	if (!IsValid(ObjectToUnregisterForCallbacks)) return;
	if (!PaintTaskCallbacksFromSpecificMeshComponents.Contains(MeshComponent)) return;


	FRVPDPCallbackFromSpecifiedMeshComponentsInfo callbackOnObjectsInfo = PaintTaskCallbacksFromSpecificMeshComponents.FindRef(MeshComponent);

	if (!callbackOnObjectsInfo.RunCallbacksOnObjects.Contains(ObjectToUnregisterForCallbacks)) return;


	callbackOnObjectsInfo.RunCallbacksOnObjects.Remove(ObjectToUnregisterForCallbacks);


	if (callbackOnObjectsInfo.RunCallbacksOnObjects.Num() > 0) {

		PaintTaskCallbacksFromSpecificMeshComponents.Add(MeshComponent, callbackOnObjectsInfo);
	}

	else {

		PaintTaskCallbacksFromSpecificMeshComponents.Remove(MeshComponent);


		// Checks if any of the meshes left has the same owner as the one we just removed, if not then we can remove the Destroyed binding. 
		TArray<UPrimitiveComponent*> meshComponentsToReceiveCallbacksFrom;
		PaintTaskCallbacksFromSpecificMeshComponents.GenerateKeyArray(meshComponentsToReceiveCallbacksFrom);

		bool invalidMeshComponent = false;

		for (int i = 0; i < meshComponentsToReceiveCallbacksFrom.Num(); i++) {

			if (IsValid(meshComponentsToReceiveCallbacksFrom[i])) {

				if (meshComponentsToReceiveCallbacksFrom[i]->GetOwner() == MeshComponent->GetOwner())
					return;
			}

			else {

				invalidMeshComponent = true;
			}
		}

		if (MeshComponent->GetOwner()->OnDestroyed.IsAlreadyBound(this, &UVertexPaintDetectionGISubSystem::RegisteredPaintTaskCallbackOwnerDestroyed)) {

			MeshComponent->GetOwner()->OnDestroyed.RemoveDynamic(this, &UVertexPaintDetectionGISubSystem::RegisteredPaintTaskCallbackOwnerDestroyed);
		}


		// If nothing left to verify
		if (PaintTaskCallbacksFromSpecificMeshComponents.Num() == 0 && DetectTaskCallbacksFromSpecificMeshComponents.Num() == 0)
			GetWorld()->GetTimerManager().ClearTimer(VerifyCallbackSpecificMeshComponentsTimer);

		// Otherwise if stuff is left, and one of them was invalid then makes sure we verify it right away 
		else if (invalidMeshComponent)
			VerifyCallbackSpecificMeshComponents();
	}
}


//--------------------------------------------------------

// Registered Paint Task Callback Owner Destroyed

void UVertexPaintDetectionGISubSystem::RegisteredPaintTaskCallbackOwnerDestroyed(AActor* DestroyedActor) {


	TArray<UPrimitiveComponent*> meshComponents;
	PaintTaskCallbacksFromSpecificMeshComponents.GetKeys(meshComponents);

	for (int i = 0; i < meshComponents.Num(); i++) {

		// Clears out all Mesh Components belonging to the destroyed Actor. 
		if (meshComponents[i] && meshComponents[i]->GetOwner() == DestroyedActor)
			PaintTaskCallbacksFromSpecificMeshComponents.Remove(meshComponents[i]);
	}


	// If nothing left to verify
	if (PaintTaskCallbacksFromSpecificMeshComponents.Num() == 0 && DetectTaskCallbacksFromSpecificMeshComponents.Num() == 0)
		GetWorld()->GetTimerManager().ClearTimer(VerifyCallbackSpecificMeshComponentsTimer);
}


//--------------------------------------------------------

// Register Callback From Specific Mesh Component

void UVertexPaintDetectionGISubSystem::RegisterDetectTaskCallbacksToObjectFromSpecifiedMeshComponent(UPrimitiveComponent* MeshComponent, UObject* ObjectToRegisterForCallbacks) {

	if (!IsValid(MeshComponent)) return;
	if (!IsValid(MeshComponent->GetOwner())) return;
	if (!IsValid(ObjectToRegisterForCallbacks)) return;


	FRVPDPCallbackFromSpecifiedMeshComponentsInfo callbackOnObjectsInfo;

	if (DetectTaskCallbacksFromSpecificMeshComponents.Contains(MeshComponent)) {

		callbackOnObjectsInfo = DetectTaskCallbacksFromSpecificMeshComponents.FindRef(MeshComponent);

		if (callbackOnObjectsInfo.RunCallbacksOnObjects.Contains(ObjectToRegisterForCallbacks)) return;
	}

	else if (!MeshComponent->GetOwner()->OnDestroyed.IsAlreadyBound(this, &UVertexPaintDetectionGISubSystem::RegisteredDetectTaskCallbackOwnerDestroyed)) {

		MeshComponent->GetOwner()->OnDestroyed.AddDynamic(this, &UVertexPaintDetectionGISubSystem::RegisteredDetectTaskCallbackOwnerDestroyed);
	}

	callbackOnObjectsInfo.RunCallbacksOnObjects.Add(ObjectToRegisterForCallbacks);
	DetectTaskCallbacksFromSpecificMeshComponents.Add(MeshComponent, callbackOnObjectsInfo);


	// Activates a looping timer that verifies that the meshes are still valid, since they can get destroyed eventhough the Actor doesn't. 
	if (!GetWorld()->GetTimerManager().IsTimerActive(VerifyCallbackSpecificMeshComponentsTimer))
		GetWorld()->GetTimerManager().SetTimer(VerifyCallbackSpecificMeshComponentsTimer, this, &UVertexPaintDetectionGISubSystem::VerifyCallbackSpecificMeshComponents, VerifyCallbackSpecificMeshComponentsIntervall, true);
}


//--------------------------------------------------------

// Un-Register Callback From Specific Mesh Component

void UVertexPaintDetectionGISubSystem::UnRegisterDetectTaskCallbacksToObjectFromSpecifiedMeshComponent(UPrimitiveComponent* MeshComponent, UObject* ObjectToUnregisterForCallbacks) {

	if (!IsValid(MeshComponent)) return;
	if (!IsValid(MeshComponent->GetOwner())) return;
	if (!IsValid(ObjectToUnregisterForCallbacks)) return;
	if (!DetectTaskCallbacksFromSpecificMeshComponents.Contains(MeshComponent)) return;


	FRVPDPCallbackFromSpecifiedMeshComponentsInfo callbackOnObjectsInfo = DetectTaskCallbacksFromSpecificMeshComponents.FindRef(MeshComponent);

	if (!callbackOnObjectsInfo.RunCallbacksOnObjects.Contains(ObjectToUnregisterForCallbacks)) return;


	callbackOnObjectsInfo.RunCallbacksOnObjects.Remove(ObjectToUnregisterForCallbacks);


	if (callbackOnObjectsInfo.RunCallbacksOnObjects.Num() > 0) {

		DetectTaskCallbacksFromSpecificMeshComponents.Add(MeshComponent, callbackOnObjectsInfo);
	}

	else {

		DetectTaskCallbacksFromSpecificMeshComponents.Remove(MeshComponent);


		// Checks if any of the meshes left has the same owner as the one we just removed, if not then we can remove the Destroyed binding. 
		TArray<UPrimitiveComponent*> meshComponentsToReceiveCallbacksFrom;
		DetectTaskCallbacksFromSpecificMeshComponents.GenerateKeyArray(meshComponentsToReceiveCallbacksFrom);

		bool invalidMeshComponent = false;

		for (int i = 0; i < meshComponentsToReceiveCallbacksFrom.Num(); i++) {

			if (IsValid(meshComponentsToReceiveCallbacksFrom[i])) {

				if (meshComponentsToReceiveCallbacksFrom[i]->GetOwner() == MeshComponent->GetOwner())
					return;
			}

			else {

				invalidMeshComponent = true;
			}
		}

		if (MeshComponent->GetOwner()->OnDestroyed.IsAlreadyBound(this, &UVertexPaintDetectionGISubSystem::RegisteredDetectTaskCallbackOwnerDestroyed)) {

			MeshComponent->GetOwner()->OnDestroyed.RemoveDynamic(this, &UVertexPaintDetectionGISubSystem::RegisteredDetectTaskCallbackOwnerDestroyed);
		}


		// If nothing left to verify
		if (PaintTaskCallbacksFromSpecificMeshComponents.Num() == 0 && DetectTaskCallbacksFromSpecificMeshComponents.Num() == 0)
			GetWorld()->GetTimerManager().ClearTimer(VerifyCallbackSpecificMeshComponentsTimer);

		// Otherwise if stuff is left, and one of them was invalid then makes sure we verify it right away 
		else if (invalidMeshComponent)
			VerifyCallbackSpecificMeshComponents();
	}
}


//--------------------------------------------------------

// Registered Detect Task Callback Owner Destroyed

void UVertexPaintDetectionGISubSystem::RegisteredDetectTaskCallbackOwnerDestroyed(AActor* DestroyedActor) {


	TArray<UPrimitiveComponent*> meshComponents;
	DetectTaskCallbacksFromSpecificMeshComponents.GetKeys(meshComponents);

	for (int i = 0; i < meshComponents.Num(); i++) {

		// Clears out all Mesh Components belonging to the destroyed Actor. 
		if (meshComponents[i] && meshComponents[i]->GetOwner() == DestroyedActor)
			DetectTaskCallbacksFromSpecificMeshComponents.Remove(meshComponents[i]);
	}


	// If nothing left to verify
	if (PaintTaskCallbacksFromSpecificMeshComponents.Num() == 0 && DetectTaskCallbacksFromSpecificMeshComponents.Num() == 0)
		GetWorld()->GetTimerManager().ClearTimer(VerifyCallbackSpecificMeshComponentsTimer);
}


//--------------------------------------------------------

// Verify Callback Specific Mesh Components

void UVertexPaintDetectionGISubSystem::VerifyCallbackSpecificMeshComponents() {


	// Since Mesh Components can be destroyed and we can't get a callback to when it happens as with actors, we have to verify if they're still valid so we don't have dangling pointers if mesh components get destroyed but their Actor still exists.


	if (PaintTaskCallbacksFromSpecificMeshComponents.Num() > 0) {

		TArray<UPrimitiveComponent*> meshComponents;
		PaintTaskCallbacksFromSpecificMeshComponents.GetKeys(meshComponents);

		TArray<FRVPDPCallbackFromSpecifiedMeshComponentsInfo> callbackFromSpecifiedMeshComponents;
		PaintTaskCallbacksFromSpecificMeshComponents.GenerateValueArray(callbackFromSpecifiedMeshComponents);

		PaintTaskCallbacksFromSpecificMeshComponents.Empty();

		for (int i = 0; i < meshComponents.Num(); i++) {

			if (IsValid(meshComponents[i]) && IsValid(meshComponents[i]->GetOwner())) {

				PaintTaskCallbacksFromSpecificMeshComponents.Add(meshComponents[i], callbackFromSpecifiedMeshComponents[i]);
			}
		}
	}


	if (DetectTaskCallbacksFromSpecificMeshComponents.Num() > 0) {

		TArray<UPrimitiveComponent*> meshComponents;
		DetectTaskCallbacksFromSpecificMeshComponents.GetKeys(meshComponents);

		TArray<FRVPDPCallbackFromSpecifiedMeshComponentsInfo> callbackFromSpecifiedMeshComponents;
		DetectTaskCallbacksFromSpecificMeshComponents.GenerateValueArray(callbackFromSpecifiedMeshComponents);

		DetectTaskCallbacksFromSpecificMeshComponents.Empty();

		for (int i = 0; i < meshComponents.Num(); i++) {

			if (IsValid(meshComponents[i]) && IsValid(meshComponents[i]->GetOwner())) {

				DetectTaskCallbacksFromSpecificMeshComponents.Add(meshComponents[i], callbackFromSpecifiedMeshComponents[i]);
			}
		}
	}
}


//-------------------------------------------------------

// Draw Closest Vertex Debug Symbols

void UVertexPaintDetectionGISubSystem::DrawClosestVertexDebugSymbols(const FRVPDPClosestVertexDataResults& ClosestVertexDataResult, const float& AverageColorAreaOfEffect, const FRVPDPDebugSettings& DebugSettings) {

	if (!DebugSettings.WorldTaskWasCreatedIn.IsValid()) return;
	if (!IsValid(DebugSettings.WorldTaskWasCreatedIn.Get())) return;
	if (!DebugSettings.TaskSpecificDebugSymbols.DrawTaskDebugSymbols) return;


	DrawDebugBox(DebugSettings.WorldTaskWasCreatedIn.Get(), ClosestVertexDataResult.ClosestVertexGeneralInfo.ClosestVertexPositionInfo.ClosestVertexPositionWorld, FVector(5, 5, 5), FColor::Red, false, DebugSettings.TaskSpecificDebugSymbols.DrawTaskDebugSymbolsDuration, 0, 3);

	DrawDebugDirectionalArrow(DebugSettings.WorldTaskWasCreatedIn.Get(), ClosestVertexDataResult.ClosestVertexGeneralInfo.ClosestVertexPositionInfo.ClosestVertexPositionWorld, (ClosestVertexDataResult.ClosestVertexGeneralInfo.ClosestVertexPositionInfo.ClosestVertexPositionWorld + (ClosestVertexDataResult.ClosestVertexGeneralInfo.ClosestVertexNormalInfo.ClosestVertexNormal * 100)), 50, FColor::Red, false, DebugSettings.TaskSpecificDebugSymbols.DrawTaskDebugSymbolsDuration, 0, 3);

	if (AverageColorAreaOfEffect > 0) {

		DrawDebugSphere(DebugSettings.WorldTaskWasCreatedIn.Get(), ClosestVertexDataResult.ClosestVertexGeneralInfo.ClosestVertexPositionInfo.ClosestVertexPositionWorld, AverageColorAreaOfEffect, 16, FColor::Green, false, DebugSettings.TaskSpecificDebugSymbols.DrawTaskDebugSymbolsDuration, 0, 3);
	}
}


//-------------------------------------------------------

// Draw Estimated Color At Hit Location Debug Symbols

void UVertexPaintDetectionGISubSystem::DrawEstimatedColorAtHitLocationDebugSymbols(const FRVPDPEstimatedColorAtHitLocationInfo& EstimatedColorAtHitLocationResult, const FVector& ClosestVertexWorldPosition, const FVector& TaskHitLocation, const FVector& VertexPositionInWorldSpaceToLerpToward, const TArray<FVector>& ClosestVertexWorldLocationsUsedToEstimateColorAtHitLocation, const FRVPDPDebugSettings& DebugSettings) {

	if (!DebugSettings.WorldTaskWasCreatedIn.IsValid()) return;
	if (!IsValid(DebugSettings.WorldTaskWasCreatedIn.Get())) return;


	// Estimated Color at Hit Location
	if (EstimatedColorAtHitLocationResult.EstimatedColorAtHitLocationDataSuccessfullyAcquired) {

		RVPDP_LOG(DebugSettings, FColor::Green, "Estimated Hit Location Color: %s", *EstimatedColorAtHitLocationResult.EstimatedColorAtHitLocation.ToString());
	}


	// Draw Estimated Color At Hit Location related sebug symbols
	if (EstimatedColorAtHitLocationResult.EstimatedColorAtHitLocationDataSuccessfullyAcquired) {


		if (DebugSettings.TaskSpecificDebugSymbols.DrawTaskDebugSymbols) {


			DrawDebugBox(DebugSettings.WorldTaskWasCreatedIn.Get(), EstimatedColorAtHitLocationResult.EstimatedColorAtWorldLocation, FVector(5, 5, 5), FQuat::Identity, FColor::Emerald, false, DebugSettings.TaskSpecificDebugSymbols.DrawTaskDebugSymbolsDuration, 0, 3);


			DrawDebugBox(DebugSettings.WorldTaskWasCreatedIn.Get(), ClosestVertexWorldPosition, FVector(5, 5, 5), FQuat::Identity, FColor::Silver, false, DebugSettings.TaskSpecificDebugSymbols.DrawTaskDebugSymbolsDuration, 0, 3);


			// Hit Location
			DrawDebugBox(DebugSettings.WorldTaskWasCreatedIn.Get(), TaskHitLocation, FVector(5, 5, 5), FQuat::Identity, FColor::Purple, false, DebugSettings.TaskSpecificDebugSymbols.DrawTaskDebugSymbolsDuration, 0, 3);

			// Vertex we Lerp Toward
			DrawDebugBox(DebugSettings.WorldTaskWasCreatedIn.Get(), VertexPositionInWorldSpaceToLerpToward, FVector(5, 5, 5), FQuat::Identity, FColor::Yellow, false, DebugSettings.TaskSpecificDebugSymbols.DrawTaskDebugSymbolsDuration, 0, 3);

			// Direction from closest vertex to hit Location
			DrawDebugDirectionalArrow(DebugSettings.WorldTaskWasCreatedIn.Get(), ClosestVertexWorldPosition, TaskHitLocation, 10, FColor::Purple, false, DebugSettings.TaskSpecificDebugSymbols.DrawTaskDebugSymbolsDuration, 0, 5);


			// All of the 9 closest vertices Paint
			for (const FVector& vertexWorldLocation : ClosestVertexWorldLocationsUsedToEstimateColorAtHitLocation) {

				DrawDebugBox(DebugSettings.WorldTaskWasCreatedIn.Get(), vertexWorldLocation, FVector(2.5f, 2.5f, 2.5f), FQuat::Identity, FColor::Blue, false, DebugSettings.TaskSpecificDebugSymbols.DrawTaskDebugSymbolsDuration, 0, 3);
			}
		}


		// Estimated Physics Surface Value
		if (EstimatedColorAtHitLocationResult.EstimatedPhysicalSurfaceInfoAtHitLocation.PhysicsSurfaceSuccessfullyAcquired) {


			RVPDP_LOG(DebugSettings, FColor::Green, "Physics Surfaces and Color Values at Estimated Color at Hit Location: Default: %s: %f  -  Red: %s: %f  -  Green: %s: %f  -  Blue: %s: %f  -  Alpha: %s: %f", *EstimatedColorAtHitLocationResult.EstimatedPhysicalSurfaceInfoAtHitLocation.PhysicsSurface_AtDefault.PhysicalSurfaceAsStringAtChannel, EstimatedColorAtHitLocationResult.EstimatedPhysicalSurfaceInfoAtHitLocation.PhysicsSurface_AtDefault.PhysicalSurfaceValueAtChannel, *EstimatedColorAtHitLocationResult.EstimatedPhysicalSurfaceInfoAtHitLocation.PhysicsSurface_AtRed.PhysicalSurfaceAsStringAtChannel, EstimatedColorAtHitLocationResult.EstimatedPhysicalSurfaceInfoAtHitLocation.PhysicsSurface_AtRed.PhysicalSurfaceValueAtChannel, *EstimatedColorAtHitLocationResult.EstimatedPhysicalSurfaceInfoAtHitLocation.PhysicsSurface_AtGreen.PhysicalSurfaceAsStringAtChannel, EstimatedColorAtHitLocationResult.EstimatedPhysicalSurfaceInfoAtHitLocation.PhysicsSurface_AtGreen.PhysicalSurfaceValueAtChannel, *EstimatedColorAtHitLocationResult.EstimatedPhysicalSurfaceInfoAtHitLocation.PhysicsSurface_AtBlue.PhysicalSurfaceAsStringAtChannel, EstimatedColorAtHitLocationResult.EstimatedPhysicalSurfaceInfoAtHitLocation.PhysicsSurface_AtBlue.PhysicalSurfaceValueAtChannel, *EstimatedColorAtHitLocationResult.EstimatedPhysicalSurfaceInfoAtHitLocation.PhysicsSurface_AtAlpha.PhysicalSurfaceAsStringAtChannel, EstimatedColorAtHitLocationResult.EstimatedPhysicalSurfaceInfoAtHitLocation.PhysicsSurface_AtAlpha.PhysicalSurfaceValueAtChannel);
		}

		// If couldn't get physical surface but just colors
		else {

			RVPDP_LOG(DebugSettings, FColor::Orange, "Couldn't get Physical Surfaces at Estimated Color at Hit Location, perhaps the Hit Material isn't registered in the Material Data Asset?");
		}
	}
}


//-------------------------------------------------------

// Deinitialize

void UVertexPaintDetectionGISubSystem::Deinitialize() {

	RVPDP_LOG(FRVPDPDebugSettings(false, 0, true), FColor::Cyan, "Game Instance Subsystem De-Initialized");
}
