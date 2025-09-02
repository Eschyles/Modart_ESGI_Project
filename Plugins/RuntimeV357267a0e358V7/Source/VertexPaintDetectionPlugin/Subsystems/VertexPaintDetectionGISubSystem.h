// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CalculateColorsPrerequisites.h"
#include "FundementalsPrerequisites.h"
#include "Tickable.h"
#include "VertexPaintDetectionGISubSystem.generated.h"


class UVertexPaintDetectionTaskQueue;
class UVertexPaintMaterialDataAsset;
class UVertexPaintOptimizationDataAsset;
class UVertexPaintColorSnippetRefs;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FVertexPaintTaskFinished, const FRVPDPTaskResults&, TaskResultInfo, const FRVPDPPaintTaskResultInfo&, PaintTaskResultInfo, const FRVPDPAdditionalDataToPassThroughInfo&, AdditionalData);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FVertexDetectTaskFinished, const FRVPDPTaskResults&, TaskResultInfo, const FRVPDPAdditionalDataToPassThroughInfo&, AdditionalData);


struct FRVPDPCleanUpVertexBufferInfo {

	int TaskID = 0;
	float TimeeSinceVertexBufferReleasedResource = 0;
	FColorVertexBuffer* VertexColorBuffer;
};


/**
 *
 */
UCLASS()
class VERTEXPAINTDETECTIONPLUGIN_API UVertexPaintDetectionGISubSystem : public UGameInstanceSubsystem, public FTickableGameObject {

	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Tick
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return CleanUpVertexBufferInfo.Num() > 0; }
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UVertexPaintDetectionGISubSystem, STATGROUP_Tickables); }
	virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Always; }
	virtual bool IsTickableWhenPaused() const override { return true; }

	// Callbacks from Specific Mesh Components
	void RegisterPaintTaskCallbacksToObjectFromSpecifiedMeshComponent(UPrimitiveComponent* MeshComponent, UObject* ObjectToRegisterForCallbacks);
	void UnRegisterPaintTaskCallbacksToObjectFromSpecifiedMeshComponent(UPrimitiveComponent* MeshComponent, UObject* ObjectToUnregisterForCallbacks);
	void RegisterDetectTaskCallbacksToObjectFromSpecifiedMeshComponent(UPrimitiveComponent* MeshComponent, UObject* ObjectToRegisterForCallbacks);
	void UnRegisterDetectTaskCallbacksToObjectFromSpecifiedMeshComponent(UPrimitiveComponent* MeshComponent, UObject* ObjectToUnregisterForCallbacks);
	const TMap<UPrimitiveComponent*, FRVPDPCallbackFromSpecifiedMeshComponentsInfo>& GetPaintTaskCallbacksFromSpecificMeshComponents() { return PaintTaskCallbacksFromSpecificMeshComponents; }
	const TMap<UPrimitiveComponent*, FRVPDPCallbackFromSpecifiedMeshComponentsInfo>& GetDetectTaskCallbacksFromSpecificMeshComponents() { return DetectTaskCallbacksFromSpecificMeshComponents; }

	// Cached Core Ptrs
	TMap<TEnumAsByte<EPhysicalSurface>, UPhysicalMaterial*> GetAllCachedPhysicsMaterialAssets() { return PhysicalMaterialAssets; }
	UVertexPaintDetectionTaskQueue* GetVertexPaintTaskQueue() const { return TaskQueue; }
	UVertexPaintMaterialDataAsset* GetCachedMaterialDataAsset() const { return MaterialDataAsset; }
	UVertexPaintOptimizationDataAsset* GetCachedOptimizationDataAsset() const { return OptimizationDataAsset; }
	UVertexPaintColorSnippetRefs* GetCachedColorSnippetReferenceDataAsset() const { return ColorSnippetReferenceDataAsset; }


	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Amount of old cached vertex buffers pointed that we're waiting to get uninitialized so we can safely delete them. Can't do it right away since they may still be used by the render thread for a while. "))
	int GetAmountOfVertexBuffersCleaningUp() const { return CleanUpVertexBufferInfo.Num(); }

	UPROPERTY(BlueprintAssignable, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Broadcasts when Finished a Paint Task."))
	FVertexPaintTaskFinished VertexPaintTaskFinished; // Broadcasts when Finished a Paint Task.

	UPROPERTY(BlueprintAssignable, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Broadcasts when Finished a Detect Task."))
	FVertexDetectTaskFinished VertexDetectTaskFinished; // Broadcasts when Finished a Detect Task.


protected:

	//---------- PAINT / DETECTION ----------//

	void WrapUpFinishedTask(const FRVPDPCalculateColorsInfo& CalculateColorsInfo, bool TaskSuccessful);

	void GetClosestVertexDataOnMesh_WrapUp(bool TaskSuccessful, const FRVPDPGetClosestVertexDataSettings& GetClosestVertexDataSettings, const FRVPDPTaskResults& DetectTaskResult, const FRVPDPClosestVertexDataResults& GetClosestVertexDataResult, const FRVPDPAverageColorInAreaInfo& AverageColorInArea, const FRVPDPEstimatedColorAtHitLocationInfo& EstimatedColorAtHitLocationResult, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings);

	void GetAllVertexColorsOnly_WrapUp(bool TaskSuccessful, const FRVPDPGetColorsOnlySettings& GetAllVertexColorsSettings, const FRVPDPTaskResults& DetectTaskResult, const FRVPDPTaskFundamentalSettings& TaskFundamentalSetting) const;

	void GetColorsWithinArea_WrapUp(bool TaskSuccessful, const FRVPDPGetColorsWithinAreaSettings& GetColorsWithinAreaSettings, const FRVPDPWithinAreaResults& WithinAreaResults, const FRVPDPTaskResults& DetectTaskResult, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings) const;

	void PaintOnMeshAtLocation_WrapUp(bool TaskSuccessful, const FRVPDPPaintAtLocationSettings& PaintAtLocationSettings, const FRVPDPTaskResults& TaskResult, const FRVPDPPaintTaskResultInfo& PaintTaskResult, const FRVPDPClosestVertexDataResults& GetClosestVertexDataResult, const FRVPDPAverageColorInAreaInfo& AverageColorInArea, const FRVPDPEstimatedColorAtHitLocationInfo& EstimatedColorAtHitLocationResult, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings);

	void PaintWithinArea_WrapUp(bool TaskSuccessful, const FRVPDPPaintWithinAreaSettings& PaintWithinAreaSettings, const FRVPDPWithinAreaResults& WithinAreaResult, const FRVPDPTaskResults& TaskResult, const FRVPDPPaintTaskResultInfo& PaintTaskResult, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings) const;

	void PaintOnEntireMesh_WrapUp(bool TaskSuccessful, const FRVPDPPaintOnEntireMeshSettings& PaintOnEntireMeshSettings, const FRVPDPTaskResults& TaskResult, const FRVPDPPaintTaskResultInfo& PaintTaskResult, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings) const;

	void PaintColorSnippet_WrapUp(bool TaskSuccessful, const FRVPDPPaintColorSnippetSettings& PaintColorSnippetSettings, const FRVPDPTaskResults& TaskResult, const FRVPDPPaintTaskResultInfo& PaintTaskResult, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings) const;

	void SetMeshComponentVertexColors_WrapUp(bool TaskSuccessful, const FRVPDPSetVertexColorsSettings& SetVertexColorsSettings, const FRVPDPTaskResults& TaskResult, const FRVPDPPaintTaskResultInfo& PaintTaskResult, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings) const;

	void SetMeshComponentVertexColorsUsingSerializedString_WrapUp(bool TaskSuccessful, const FRVPDPSetVertexColorsUsingSerializedStringSettings& SetVertexColorsUsingSerializedStringSettings, const FRVPDPTaskResults& TaskResult, const FRVPDPPaintTaskResultInfo& PaintTaskResult, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings) const;

	void DrawClosestVertexDebugSymbols(const FRVPDPClosestVertexDataResults& ClosestVertexDataResult, const float& AverageColorAreaOfEffect, const FRVPDPDebugSettings& DebugSettings);

	void DrawEstimatedColorAtHitLocationDebugSymbols(const FRVPDPEstimatedColorAtHitLocationInfo& EstimatedColorAtHitLocationResult, const FVector& ClosestVertexWorldPosition, const FVector& TaskHitLocation, const FVector& VertexPositionInWorldSpaceToLerpToward, const TArray<FVector>& ClosestVertexWorldLocationsUsedToEstimateColorAtHitLocation, const FRVPDPDebugSettings& DebugSettings);

	void AddColorVertexBufferToBufferCleanup(int TaskID, FColorVertexBuffer* VertexColorBuffer);

	void ApplyVertexColorsToMeshComponent(UPrimitiveComponent* MeshComponent, int TaskID, TArray<FColorVertexBuffer*> NewColorVertexBuffers, const TArray<FColor>& DynamicMeshComponentVertexColors, const TArray<FLinearColor>& GeometryCollectionComponentVertexColors);


protected:

	//---------- CALLBACKS ----------//

	UFUNCTION()
	void RegisteredPaintTaskCallbackOwnerDestroyed(AActor* DestroyedActor);

	UFUNCTION()
	void RegisteredDetectTaskCallbackOwnerDestroyed(AActor* DestroyedActor);

	void VerifyCallbackSpecificMeshComponents();


	//---------- PROPERTIES ----------//

	UPROPERTY()
	UVertexPaintDetectionTaskQueue* TaskQueue;


	UPROPERTY()
	TMap<UPrimitiveComponent*, FRVPDPCallbackFromSpecifiedMeshComponentsInfo> PaintTaskCallbacksFromSpecificMeshComponents;

	UPROPERTY()
	TMap<UPrimitiveComponent*, FRVPDPCallbackFromSpecifiedMeshComponentsInfo> DetectTaskCallbacksFromSpecificMeshComponents;

	FTimerHandle VerifyCallbackSpecificMeshComponentsTimer;

	float VerifyCallbackSpecificMeshComponentsIntervall = 10;


	UPROPERTY()
		TMap<TEnumAsByte<EPhysicalSurface>, UPhysicalMaterial*> PhysicalMaterialAssets;

	// Caches these so they're in memory at all time, so we don't have to worry about a use case where they may not be in memory when a asynctask has started, and that we will get a crash because we can't load them on the async thread
	UPROPERTY()
		UVertexPaintColorSnippetRefs* ColorSnippetReferenceDataAsset; 

	UPROPERTY()
		UVertexPaintOptimizationDataAsset* OptimizationDataAsset;

	UPROPERTY()
		UVertexPaintMaterialDataAsset* MaterialDataAsset;

	TMap<int32, FRVPDPCleanUpVertexBufferInfo> CleanUpVertexBufferInfo;

	float DelayUntilCleanUpVertexBuffer = 2;
};
