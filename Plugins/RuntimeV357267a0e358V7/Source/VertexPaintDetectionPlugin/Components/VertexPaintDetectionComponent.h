// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

// Prerequisites
#include "CalculateColorsPrerequisites.h"

// Paint Prerequisites
#include "SetColorsWithStringPrerequisites.h"
#include "SetMeshColorsPrerequisites.h"
#include "PaintSnippetsPrerequisites.h"
#include "PaintAtLocationPrerequisites.h"
#include "PaintEntireMeshPrerequisites.h"
#include "PaintWithinAreaPrerequisites.h"

//  Detect Prerequisites
#include "GetClosestVertexDataPrerequisites.h"
#include "GetColorsOnlyPrerequisites.h"
#include "GetColorsWithinAreaPrerequisites.h"

#include "Engine/StreamableManager.h"
#include "VertexPaintDetectionComponent.generated.h"


//--------------------------------------------------------

// Detect Delegates

DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(FVertexColorGetClosestVertexData, const FRVPDPTaskResults&, TaskResultInfo, const FRVPDPGetClosestVertexDataSettings&, DetectedMeshWithSettings, const FRVPDPClosestVertexDataResults&, ClosestVertexInfo, const FRVPDPEstimatedColorAtHitLocationInfo&, EstimatedColorAtHitLocationInfo, const FRVPDPAverageColorInAreaInfo&, AvarageColorInAreaInfo, const FRVPDPAdditionalDataToPassThroughInfo&, AdditionalData);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FVertexColorGetAllVertexColorsOnly, const FRVPDPTaskResults&, TaskResultInfo, const FRVPDPGetColorsOnlySettings&, GotAllVertexColorsWithSettings, const FRVPDPAdditionalDataToPassThroughInfo&, AdditionalData);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FGetColorsWithinArea, const FRVPDPTaskResults&, TaskResultInfo, const FRVPDPGetColorsWithinAreaSettings&, GetColorsWithinAreaSettings, const FRVPDPWithinAreaResults&, WithinAreaResults, const FRVPDPAdditionalDataToPassThroughInfo&, AdditionalData);


//--------------------------------------------------------

// Paint Delegates

DECLARE_DYNAMIC_MULTICAST_DELEGATE_SevenParams(FVertexColorPaintedAtLocation, const FRVPDPTaskResults&, TaskResultInfo, const FRVPDPPaintTaskResultInfo&, PaintTaskResultInfo, const FRVPDPPaintAtLocationSettings&, PaintedAtLocationWithSettings, const FRVPDPClosestVertexDataResults&, ClosestVertexInfoAfterApplyingColor, const FRVPDPEstimatedColorAtHitLocationInfo&, EstimatedColorAtHitLocationInfo, const FRVPDPAverageColorInAreaInfo&, AvarageColorInAreaInfo, const FRVPDPAdditionalDataToPassThroughInfo&, AdditionalData);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FVertexColorPaintedWithinArea, const FRVPDPTaskResults&, TaskResultInfo, const FRVPDPPaintTaskResultInfo&, PaintTaskResultInfo, const FRVPDPPaintWithinAreaSettings&, PaintedWithinAreaWithSettings, const FRVPDPWithinAreaResults&, WithinAreaResults, const FRVPDPAdditionalDataToPassThroughInfo&, AdditionalData);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FVertexColorPaintedEntireMesh, const FRVPDPTaskResults&, TaskResultInfo, const FRVPDPPaintTaskResultInfo&, PaintTaskResultInfo, const FRVPDPPaintOnEntireMeshSettings&, EntireMeshPaintedWithSettings, const FRVPDPAdditionalDataToPassThroughInfo&, AdditionalData);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FVertexColorPaintColorSnippet, const FRVPDPTaskResults&, TaskResultInfo, const FRVPDPPaintTaskResultInfo&, PaintTaskResultInfo, const FRVPDPPaintColorSnippetSettings&, PaintColorSnippetWithSettings, const FRVPDPAdditionalDataToPassThroughInfo&, AdditionalData);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FSetMeshComponentVertexColors, const FRVPDPTaskResults&, TaskResultInfo, const FRVPDPPaintTaskResultInfo&, PaintTaskResultInfo, const FRVPDPSetVertexColorsSettings&, SetMeshComponentVertexColorWithSettings, const FRVPDPAdditionalDataToPassThroughInfo&, AdditionalData);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FSetMeshComponentVertexColorsUsingSerializedString, const FRVPDPTaskResults&, TaskResultInfo, const FRVPDPPaintTaskResultInfo&, PaintTaskResultInfo, const FRVPDPSetVertexColorsUsingSerializedStringSettings&, SetMeshComponentVertexColorUsingSerializedStringWithSettings, const FRVPDPAdditionalDataToPassThroughInfo&, AdditionalData);


//--------------------------------------------------------

// Load Color Snippet Info

struct FRVPDPLoadColorSnippetInfo {

	FString ColorSnippetID = "";
	int LoadTaskID = -1;
	FSoftObjectPath LoadPath;
};



UCLASS(Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent), DisplayName = "Runtime Vertex Paint Component", ClassGroup = (Custom))
class VERTEXPAINTDETECTIONPLUGIN_API UVertexPaintDetectionComponent : public UActorComponent {

	GENERATED_BODY()


protected:

	UVertexPaintDetectionComponent();


public:

	//---------- TASKS ----------//

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin")
	TMap<int, UPrimitiveComponent*> GetCurrentGetClosestVertexDataTasks() { return CurrentGetClosestVertexDataTasks; }

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin")
	TMap<int, UPrimitiveComponent*> GetCurrentGetAllVertexColorsOnlyTasks() { return CurrentGetAllVertexColorsOnlyTasks; }

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin")
	TMap<int, UPrimitiveComponent*> GetCurrentGetColorsWithinAreaTasks() { return CurrentGetColorsWithinAreaTasks; }

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin")
	TMap<int, UPrimitiveComponent*> GetCurrentPaintAtLocationTasks() { return CurrentPaintAtLocationTasks; }

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin")
	TMap<int, UPrimitiveComponent*> GetCurrentPaintWithinAreaTasks() { return CurrentPaintWithinAreaTasks; }

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin")
	TMap<int, UPrimitiveComponent*> GetCurrentPaintEntireMeshTasks() { return CurrentPaintEntireMeshTasks; }

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin")
	TMap<int, UPrimitiveComponent*> GetCurrentPaintColorSnippetTasks() { return CurrentPaintColorSnippetTasks; }

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin")
	TMap<int, UPrimitiveComponent*> GetCurrentSetMeshComponentVertexColorsTasks() { return CurrentSetMeshComponentVertexColorsTasks; }

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin")
	TMap<int, UPrimitiveComponent*> GetCurrentSetMeshComponentVertexColorsUsingSerializedStringTasks() { return CurrentSetMeshComponentVertexColorsUsingSerializedStringTasks; }

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "This returns the amount of Tasks that this component has queued up but have not finished yet. Can be useful if you for instance want to make sure another task of a certain type doesn't start if there's already one going. "))
	void GetCurrentTasksInitiatedByComponent(int& TotalAmountOfTasks, int& AmountOfGetClosestVertexDataTasks, int& AmountOfGetAllVertexColorsOnlyTasks, int& AmountOfGetColorsWithinAreaTasks, int& AmountOfPaintAtLocationTasks, int& AmountOfPaintWithinAreaTasks, int& AmountOfPaintEntireMeshTasks, int& AmountOfPaintColorSnippetTasks, int& AmountOfSetMeshComponentVertexColorsTasks, int& AmountOfSetMeshComponentVertexColorsUsingSerializedStringTasks);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Gets all Primitive Components actively being painted or detected. "))
	TArray<UPrimitiveComponent*> GetCurrentTasksMeshComponents();

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The Total Tasks this Component has Queued up but haven't finished yet. "))
	int GetTotalTasksInitiatedByComponent();

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If this Component have any Tasks Queued up. "))
	bool HasAnyTasksQueuedUp() { return (GetTotalTasksInitiatedByComponent() > 0); }

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin")
	bool HasPaintTaskQueuedUpOnMeshComponent(UPrimitiveComponent* meshComponent) const;

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin")
	bool HasDetectTaskQueuedUpOnMeshComponent(UPrimitiveComponent* meshComponent) const;

public:


	//---------- PAINT & DETECT ----------//

	virtual void GetClosestVertexDataOnMesh(FRVPDPGetClosestVertexDataSettings GetClosestVertexDataStruct, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough);

	virtual void GetAllVertexColorsOnly(FRVPDPGetColorsOnlySettings GetAllVertexColorsStruct, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough);

	virtual void GetColorsWithinArea(FRVPDPGetColorsWithinAreaSettings GetColorsWithinArea, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough);

	virtual void PaintOnMeshAtLocation(FRVPDPPaintAtLocationSettings PaintAtLocationStruct, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough);

	virtual void PaintOnMeshWithinArea(FRVPDPPaintWithinAreaSettings PaintWithinAreaStruct, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough);

	virtual void PaintOnEntireMesh(FRVPDPPaintOnEntireMeshSettings PaintOnEntireMeshStruct, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough);

	virtual void PaintColorSnippetOnMesh(FRVPDPPaintColorSnippetSettings PaintColorSnippetStruct, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough);

	virtual void SetMeshComponentVertexColors(FRVPDPSetVertexColorsSettings SetMeshComponentVertexColorsSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough);

	virtual void SetMeshComponentVertexColorsUsingSerializedString(FRVPDPSetVertexColorsUsingSerializedStringSettings SetMeshComponentVertexColorsUsingSerializedStringSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough);


protected:

	virtual bool CheckValidFundementals(const FRVPDPTaskFundamentalSettings& TaskFundementals, UPrimitiveComponent* Component);
	virtual bool CheckValidVertexPaintSettings(const FRVPDPPaintTaskSettings& VertexPaintSettings, bool IgnoreTaskQueueChecks = false);
	virtual bool CheckValidVertexPaintColorSettings(const FRVPDPApplyColorSetting& ColorSettings);

	virtual bool DetectTaskCheckValid(const FRVPDPTaskFundamentalSettings& TaskFundementals, UPrimitiveComponent* Component);
	virtual bool PaintColorsTaskCheckValid(const FRVPDPApplyColorSetting& ColorSettings, UPrimitiveComponent* Component, bool IgnoreTaskQueueChecks = false);
	virtual bool PaintTaskCheckValid(const FRVPDPPaintTaskSettings& PaintSettings, UPrimitiveComponent* Component);
	virtual bool WithinAreaCheckValid(const FRVPDPWithinAreaSettings& WithinAreaSettings, const FRVPDPDebugSettings& DebugSettings);

	// Paint Tasks can Ignore the Task Queue Checks since there are certain instances where we don't want that to make the check valid fail, such as when the Auto Paint Component wants to Add/Update a Mesh Component
	virtual bool GetClosestVertexDataOnMeshCheckValid(const FRVPDPGetClosestVertexDataSettings& GetClosestVertexDataSettings);
	virtual bool GetAllVertexColorsOnlyCheckValid(const FRVPDPGetColorsOnlySettings& GetAllVertexColorsSettings);
	virtual bool GetColorsWithinAreaCheckValid(const FRVPDPGetColorsWithinAreaSettings& GetWithinAreaSettings);
	virtual bool PaintOnMeshAtLocationCheckValid(const FRVPDPPaintAtLocationSettings& PaintAtLocationSettings, bool IgnoreTaskQueueChecks = false);
	virtual bool PaintOnMeshWithinAreaCheckValid(const FRVPDPPaintWithinAreaSettings& PaintWithinAreaSettings, bool IgnoreTaskQueueChecks = false);
	virtual bool PaintOnEntireMeshCheckValid(const FRVPDPPaintOnEntireMeshSettings& PaintOnEntireMeshSettings, bool IgnoreTaskQueueChecks = false);
	virtual bool PaintColorSnippetCheckValid(const FRVPDPPaintColorSnippetSettings& PaintColorSnippetSettings);
	virtual bool SetMeshComponentVertexColorsCheckValid(const FRVPDPSetVertexColorsSettings& SetMeshComponentVertexColorsSettings);
	virtual bool SetMeshComponentVertexColorsUsingSerializedStringCheckValid(const FRVPDPSetVertexColorsUsingSerializedStringSettings& SetMeshComponentVertexColorsUsingSerializedStringSettings);

	TArray<FRVPDPComponentToCheckIfIsWithinAreaInfo> GetWithinAreaComponentsToCheckIfIsWithinSettings(UPrimitiveComponent* MeshComponent, FRVPDPWithinAreaSettings WithinAreaSettings) const;

public:

	//---------- CALLBACKS ----------//

	virtual void GetClosestVertexDataOnMeshTaskFinished(const FRVPDPTaskResults& DetectTaskResult, const FRVPDPGetClosestVertexDataSettings& GetClosestVertexDataSettings, const FRVPDPClosestVertexDataResults& ClosestVertexColorResult, const FRVPDPEstimatedColorAtHitLocationInfo& EstimatedColorAtHitLocationResult, const FRVPDPAverageColorInAreaInfo& AverageColor, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough, bool FailedAtCheckValid = false);

	virtual void GetAllVertexColorsOnlyTaskFinished(const FRVPDPTaskResults& DetectTaskResult, const FRVPDPGetColorsOnlySettings& GetAllVertexColorsSettings, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough, bool FailedAtCheckValid = false);

	virtual void GetColorsWithinAreaTaskFinished(const FRVPDPTaskResults& DetectTaskResult, const FRVPDPGetColorsWithinAreaSettings& GetColorsWithinAreaSettings, const FRVPDPWithinAreaResults& WithinAreaResults, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough, bool FailedAtCheckValid = false);

	virtual void PaintOnMeshAtLocationTaskFinished(const FRVPDPTaskResults& TaskResult, const FRVPDPPaintTaskResultInfo& PaintTaskResult, const FRVPDPPaintAtLocationSettings& PaintAtLocationSettings, const FRVPDPClosestVertexDataResults& ClosestVertexColorResult, const FRVPDPEstimatedColorAtHitLocationInfo& EstimatedColorAtHitLocationResult, const FRVPDPAverageColorInAreaInfo& AverageColor, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough, bool FailedAtCheckValid = false);

	virtual void PaintOnMeshWithinAreaTaskFinished(const FRVPDPTaskResults& TaskResult, const FRVPDPPaintTaskResultInfo& PaintTaskResult, const FRVPDPPaintWithinAreaSettings& PaintWithinAreaSettings, const FRVPDPWithinAreaResults& WithinAreaResults, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough, bool FailedAtCheckValid = false);

	virtual void PaintOnEntireMeshTaskFinished(const FRVPDPTaskResults& TaskResult, const FRVPDPPaintTaskResultInfo& PaintTaskResult, const FRVPDPPaintOnEntireMeshSettings& PaintOnEntireMeshSettings, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough, bool FailedAtCheckValid = false);

	virtual void PaintColorSnippetOnMeshTaskFinished(const FRVPDPTaskResults& TaskResult, const FRVPDPPaintTaskResultInfo& PaintTaskResult, const FRVPDPPaintColorSnippetSettings& PaintColorSnippetSettings, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough, bool FailedAtCheckValid = false);

	virtual void SetMeshComponentVertexColorsTaskFinished(const FRVPDPTaskResults& TaskResult, const FRVPDPPaintTaskResultInfo& PaintTaskResult, const FRVPDPSetVertexColorsSettings& SetVertexColorsSettings, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough, bool FailedAtCheckValid = false);

	virtual void SetMeshComponentVertexColorsUsingSerializedStringTaskFinished(const FRVPDPTaskResults& TaskResult, const FRVPDPPaintTaskResultInfo& PaintTaskResult, const FRVPDPSetVertexColorsUsingSerializedStringSettings& SetVertexColorsUsingSerializedStringSettings, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough, bool FailedAtCheckValid = false);


	UPROPERTY(BlueprintAssignable, Category = "Runtime Vertex Paint and Detection Plugin|Detection")
		FVertexColorGetClosestVertexData GetClosestVertexDataDelegate; // Broadcasts when Finished Getting Vertex Data, either successfully or unsuccessfully. 

	UPROPERTY(BlueprintAssignable, Category = "Runtime Vertex Paint and Detection Plugin|Detection", meta = (ToolTip = "Broadcasts when Finished Getting All Vertex Colors Only, either successfully or unsuccessfully."))
		FVertexColorGetAllVertexColorsOnly GetAllVertexColorsOnlyDelegate; // Broadcasts when Finished Getting Vertex Data, either successfully or unsuccessfully.

	UPROPERTY(BlueprintAssignable, Category = "Runtime Vertex Paint and Detection Plugin|Detection", meta = (ToolTip = "Broadcasts when Finished Getting Colors Within Area, either successfully or unsuccessfully."))
	FGetColorsWithinArea GetColorsWithinAreaDelegate; // Broadcasts when Finished Getting Vertex Data, either successfully or unsuccessfully. 


	UPROPERTY(BlueprintAssignable, Category = "Runtime Vertex Paint and Detection Plugin|Painting", meta = (ToolTip = "Broadcasts when Finished Painting Vertex Colors, either successfully or unsuccessfully."))
		FVertexColorPaintedAtLocation VertexColorsPaintedAtLocationDelegate; // Broadcasts when Finished Painting Vertex Colors, either successfully or unsuccessfully. 

	UPROPERTY(BlueprintAssignable, Category = "Runtime Vertex Paint and Detection Plugin|Painting", meta = (ToolTip = "Broadcasts when Finished Painting Vertex Colors on Mesh within Area, either successfully or unsuccessfully."))
		FVertexColorPaintedWithinArea VertexColorsPaintedMeshWithinAreaDelegate; // Broadcasts when Finished Painting Vertex Colors on Mesh within Area, either successfully or unsuccessfully. 

	UPROPERTY(BlueprintAssignable, Category = "Runtime Vertex Paint and Detection Plugin|Painting", meta = (ToolTip = "Broadcasts when Finished Painting Vertex Colors on Entire Mesh, either successfully or unsuccessfully."))
		FVertexColorPaintedEntireMesh VertexColorsPaintedEntireMeshDelegate; // Broadcasts when Finished Painting Vertex Colors on Entire Mesh, either successfully or unsuccessfully.

	UPROPERTY(BlueprintAssignable, Category = "Runtime Vertex Paint and Detection Plugin|Painting", meta = (ToolTip = "Broadcasts when Finished Paint Color Snippet, either successfully or unsuccessfully. "))
		FVertexColorPaintColorSnippet VertexColorsPaintColorSnippetDelegate; // Broadcasts when Finished Paint Color Snippet, either successfully or unsuccessfully. 

	UPROPERTY(BlueprintAssignable, Category = "Runtime Vertex Paint and Detection Plugin|Painting", meta = (ToolTip = "Broadcasts when VertexPaintDetectionGISubSystem has run it's SetMeshComponentVertexColors IF you provided a component. Useful if you're loading vertex data and want to start each task when the previous is finished. "))
		FSetMeshComponentVertexColors VertexColorsSetMeshColorsDelegate; // Broadcasts when VertexPaintDetectionGISubSystem has run it's SetMeshComponentVertexColors IF you provided a component. Useful if you're loading vertex data and want to start each task when the previous is finished.

	UPROPERTY(BlueprintAssignable, Category = "Runtime Vertex Paint and Detection Plugin|Painting", meta = (ToolTip = "Broadcasts when VertexPaintDetectionGISubSystem has run it's SetMeshComponentVertexColorsUsingSerializedString IF you provided a component. Useful if you're loading vertex data and want to start each task when the previous is finished. "))
		FSetMeshComponentVertexColorsUsingSerializedString VertexColorsSetMeshColorsUsingSerializedStringDelegate; // Broadcasts when VertexPaintDetectionGISubSystem has run it's SetMeshComponentVertexColorsUsingSerializedString IF you provided a component. Useful if you're loading vertex data and want to start each task when the previous is finished. 

private:


	//---------- UTILITIES ----------//

	void FillCalculateColorsInfoFundementals(FRVPDPCalculateColorsInfo& CalculateColorsInfo);

	void GetTaskFundamentalsSettings(UPrimitiveComponent* MeshComponent, FRVPDPCalculateColorsInfo& CalculateColorsInfo, int LODsToLoopThrough = 1);

	void AddTaskToQueue(const FRVPDPCalculateColorsInfo& CalculateColorsInfo, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough);

	bool DoesCompareColorSnippetInfoNeedToBeSet(FRVPDPCalculateColorsInfo CalculateColorsInfo, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough);

	bool LoadColorSnippetDataAsset(const FRVPDPCalculateColorsInfo& CalculateColorsInfo, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough, const TMap<FString, FSoftObjectPath>& ColorSnippetIDsAndDataAssetPath, ELoadColorSnippetDataAssetOptions LoadColorSnippetDataAssetSetting);

	void FinishedLoadingColorSnippetDataAsset(int FinishedTaskID, FRVPDPCalculateColorsInfo CalculateColorsInfo, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough, const FString& ColorSnippetID, ELoadColorSnippetDataAssetOptions LoadedColorSnippetDataAssetSettings, int AmountOfExpectedFinishedLoadingCallbacks);

	void WrapUpLoadedColorSnippetDataAsset(FRVPDPCalculateColorsInfo CalculateColorsInfo, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough, ELoadColorSnippetDataAssetOptions LoadColorSnippetDataAssetSetting);

	FString GetAllPhysicsSurfacesToApplyAsString(const FRVPDPApplyColorSetting& ColorSettings) const;

	void CheckIfRegisteredAndInitialized();

	void ClampPaintSettings(FRVPDPApplyColorSettings& ApplyVertexColorSettings) const;

	float GetClampedPaintStrength(float PaintStrength) const;

	void ClampPaintLimits(FRVPDPPaintLimitSettings& RedChannelLimit, FRVPDPPaintLimitSettings& BlueChannelLimit, FRVPDPPaintLimitSettings& GreenChannelLimit, FRVPDPPaintLimitSettings& AlphaChannelLimit) const;

	TArray<FName> GetMeshBonesWithinComponentsToCheckIfWithinArea(UPrimitiveComponent* MeshComponent, const TArray<FRVPDPComponentToCheckIfIsWithinAreaInfo>& ComponentsToCheckIfWithinArea, const FRVPDPDebugSettings& DebugSettings) const;

	void DrawWithinAreaDebugSymbols(TArray<FRVPDPComponentToCheckIfIsWithinAreaInfo> ComponentsToCheckIfIsWithin, float Duration) const;

	void ResolvePaintConditions(FRVPDPApplyColorSetting& PaintColorSettings) const;

	void GetCompleteCallbackSettings(UPrimitiveComponent* MeshComponent, bool AutoAppendRequiresCallbackSettings, FRVPDPTaskCallbackSettings& FundamentalSettings);


protected:

	//---------- PROPERTIES ----------//

	TMap<int, TSharedPtr<FStreamableHandle>> ColorSnippetAsyncLoadHandleMap;

	UPROPERTY()
	TMap<int, UPrimitiveComponent*> CurrentGetClosestVertexDataTasks;

	UPROPERTY()
	TMap<int, UPrimitiveComponent*> CurrentGetAllVertexColorsOnlyTasks;

	UPROPERTY()
	TMap<int, UPrimitiveComponent*> CurrentGetColorsWithinAreaTasks;

	UPROPERTY()
	TMap<int, UPrimitiveComponent*> CurrentPaintAtLocationTasks;

	UPROPERTY()
	TMap<int, UPrimitiveComponent*> CurrentPaintWithinAreaTasks;

	UPROPERTY()
	TMap<int, UPrimitiveComponent*> CurrentPaintEntireMeshTasks;

	UPROPERTY()
	TMap<int, UPrimitiveComponent*> CurrentPaintColorSnippetTasks;

	UPROPERTY()
	TMap<int, UPrimitiveComponent*> CurrentSetMeshComponentVertexColorsTasks;

	UPROPERTY()
	TMap<int, UPrimitiveComponent*> CurrentSetMeshComponentVertexColorsUsingSerializedStringTasks;

	UPROPERTY()
	TMap<FString, UVertexPaintColorSnippetDataAsset*> LoadedCompareColorSnippetDataAssets;

	int LoadColorSnippetDataAssetsCallbacks;
};
