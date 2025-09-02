// Copyright 2022-2024 Alexander Floden, Alias Alex River. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "VertexPaintDetectionComponent.h"
#include "VertexPaintDetectionInterface.h"
#include "AutoAddColorComponent.generated.h"


USTRUCT(BlueprintType, meta = (DisplayName = "Auto Paint Task Results"))
struct FRVPDPAutoPaintTaskResults {

	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint Component")
	UPrimitiveComponent* MeshComponent = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint Component")
	FRVPDPTaskResults TaskResultInfo;

	UPROPERTY(BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint Component")
	FRVPDPPaintTaskResultInfo PaintTaskResultInfo;

	UPROPERTY(BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint Component")
	FRVPDPAdditionalDataToPassThroughInfo AdditionalData;
};


USTRUCT(BlueprintType, meta = (DisplayName = "Auto Add Color Settings"))
struct FRVPDPAutoAddColorSettings {

	GENERATED_BODY()


	void UpdateExposedProperties(bool NewCanEverGetPaused, float NewDelayBetweenTasks, bool NewStopAutoPaintingMeshIfFullyPainted, bool NewStopAutoPaintingMeshIfCompletelyEmpty, bool NewOnlyStartNewTaskIfChangeWasMadeByOwningPaintComponent) {

		CanEverGetPaused = NewCanEverGetPaused;
		DelayBetweenTasks = NewDelayBetweenTasks;
		RemoveMeshFromAutoPaintingIfFullyPainted = NewStopAutoPaintingMeshIfFullyPainted;
		RemoveMeshFromAutoPaintingIfCompletelyEmpty = NewStopAutoPaintingMeshIfCompletelyEmpty;
		OnlyStartNewTaskIfChangeWasMadeByOwningPaintComponent = NewOnlyStartNewTaskIfChangeWasMadeByOwningPaintComponent;
	}



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint", meta = (ToolTip = "If this Mesh's Auto Painting can ever get Paused. "))
	bool CanEverGetPaused = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint", meta = (ToolTip = "If we should have a delay until we start auto adding colors again. Useful if you're for instance drying a character but don't want it to dry too fast. "))
	float DelayBetweenTasks = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint|Optimization Settings", meta = (ToolTip = "If True then this will Remove the Mesh from being Auto Painted by this component if it's Fully Painted of the Channels / Physics Surfaces we're Trying to Apply. "))
	bool RemoveMeshFromAutoPaintingIfFullyPainted = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint|Optimization Settings", meta = (ToolTip = "If True then this will Remove the Mesh from being Auto Painted by this component if it's Completely Empty of the Channels / Physics Surfaces we're Trying to Remove. \nCan be useful if you have a centralized system, for example a Singleton that listens to Overlaps of Water Bodies and Starts Auto Paint Entire Mesh on them for Drying to make sure they all can Dry in sync. But if a Mesh leaves the water body and has become fully dried, then it can stop auto painting it, i.e. clean it out of it's list so it only has the relevant Meshes in it. "))
	bool RemoveMeshFromAutoPaintingIfCompletelyEmpty = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint|Optimization Settings", meta = (ToolTip = "Optional Optimization so we don't start another task if no change was made from the task that got finished, if the instigator of the task was This auto paint component. \nThis may be useful if you have several auto paint components that is working together since another component can run tasks and trigger another component to check if it should start auto painting again. \nFor Example a Auto Paint Within Area for a Lake, and a Auto Paint Entire Mesh for Drying a Character. If True for the Drying, then it attempts to Dry if there was no change, for instance if the Mesh is completely under the water. If the mesh moves, the within area component can trigger tasks which makes the drying component check if it should start drying again. "))
	bool OnlyStartNewTaskIfChangeWasMadeByOwningPaintComponent = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint|Optimization Settings", meta = (ToolTip = "Even if no Change was Made, or if the Mesh is Competely Empty or Fully Painted, we still Force a new Task to be started for the Mesh. This may be useful if the thing that may cause a change, will be based on other circumstances. For instance if running a paint job with paint condition line of sight, then nearby geometry will be a factor that can create that change, but you don't know when you will be close to it so you want to make sure you always run the tasks (you can in this instance also run a trace if you're close to geometry and then instigate a new paint job then, but then those traces are an added cost). "))
	bool AlwaysForceStartNewTasksForTheMesh = false;


	bool IsPaused = false;

	bool OnlyStartNewRoundOfTasks_HasFinishedCachedResults = false;
	FRVPDPAutoPaintTaskResults OnlyStartNewRoundOfTasks_FinishedCachedResults;
};


USTRUCT()
struct FRVPDPAutoAddColorDelaySettings {

	GENERATED_BODY()

	float DelayBetweenTasks = 0;
	float DelayBetweenTasksCounter = 0;
};


USTRUCT(BlueprintType, meta = (DisplayName = "Start New Round Of Tasks Info"))
struct FRVPDPStartNewRoundOfTasksInfo {
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint Component")
	TMap<UPrimitiveComponent*, FRVPDPAutoPaintTaskResults> NewRoundOfTasks;
};




DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FStartedNewRoundOfTasks, FRVPDPStartNewRoundOfTasksInfo, StartedNewRoundOfTasksOnMeshes);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FStartedAutoPaintingMesh, UPrimitiveComponent*, MeshComponent);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FVerifiedAutoPaintingMeshComponents);


UCLASS(Abstract)
class VERTEXPAINTDETECTIONPLUGIN_API UAutoAddColorComponent : public UVertexPaintDetectionComponent, public IVertexPaintDetectionInterface {

	GENERATED_BODY()

protected:

	UAutoAddColorComponent();
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


public:


	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint")
	virtual void SetIfAutoPaintedMeshShouldOnlyStartNewTaskIfChangeWasMade(UPrimitiveComponent* MeshComponent, bool OnlyStartNewTaskIfChangeWasMade);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint")
	virtual void StopAllAutoPainting();

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint")
	virtual void StopAutoPaintingMesh(UPrimitiveComponent* MeshComponent);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint")
	virtual void PauseAllAutoPainting();

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint")
	virtual void PauseAutoPaintingMesh(UPrimitiveComponent* MeshComponent);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint")
	virtual void ResumeAllAutoPainting();

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint")
	virtual void ResumeAutoPaintingMesh(UPrimitiveComponent* MeshComponent);

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint")
	const TMap<UPrimitiveComponent*, FRVPDPAutoAddColorSettings>& GetMeshesBeingAutoPainted() const { return AutoPaintingMeshes; }

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint")
	bool IsMeshBeingAutoPainted(const UPrimitiveComponent* MeshComponent) const { return AutoPaintingMeshes.Contains(MeshComponent); }

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint")
	bool IsAutoPaintedMeshPaused(const UPrimitiveComponent* MeshComponent) const;

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint")
	bool GetIsAutoPainting() const { return IsAutoPaintingEnabled; }

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint")
	bool IsNewRoundOfTasksGoingToStartAfterDelay() const;

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint")
	bool GetIfMeshShouldOnlyStartNewTaskIfChangeWasMade(const UPrimitiveComponent* MeshComponent) const;

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint")
	bool CanAutoPaintedMeshGetPaused(const UPrimitiveComponent* MeshComponent) const;



	UPROPERTY(BlueprintAssignable, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint")
	FStartedNewRoundOfTasks StartedNewRoundOfTasksDelegate;

	UPROPERTY(BlueprintAssignable, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint")
	FStartedAutoPaintingMesh StartedAutoPaintingMeshDelegate;

	// Runs whenever VerifyAutoPaintingMeshComponents() runs and the meshes has been verified, which is essentially when a mesh component that's been auto painted isn't valid anymore. Useful if anything else that uses this component wants to do clean up as well
	UPROPERTY(BlueprintAssignable, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint")
	FVerifiedAutoPaintingMeshComponents VerifiedAutoPaintingMeshesDelegate;
	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint|Fundemental Settings", meta = (ToolTip = "You can specify the min amount you want to consider the Meshes Painted Full. This is useful if you have something that may paint randomly all over the Mesh, like a Weather system that stops painting after fully painted or empty, where you may get to 0.995 but the last couple of digits may take a longer time than expected before it gets fully painted. Then it may be useful to consider it full even if not at 1 but just slightly under, which in most cases aren't noticable anyway. "))
	float ConsiderMeshFullyPaintedIfOverOrEqual = 0.999f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint|Fundemental Settings", meta = (ToolTip = "If Painting by Lerping to a Target Value, then we don't check if Fully Painted or Empty, but rather how close the Average color to the Target Value is. This is the error tolerance how close we will accept the average color and not stop running tasks. "))
	float LerpToTargetErrorToleranceIfEqual = 0.005f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint|Fundemental Settings", meta = (ToolTip = "When the Task is Finished, if the instigating auto paint component Shouldn't immediately queue up another task but instead do it after 0.01 seconds even if delay between tasks is set to 0. \nThis is very useful since if several auto paint components is working together, for example a Auto Within Area Component for a Lake, and a Auto Entire Mesh Component for Drying, then it works much better since they let each other jump in and queue up a new task since they're both listening for paint finish task event for that mesh component. "))
	bool ApplyOneFrameDelayBetweenTasksForInstigatingComponent = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint|Fundemental Settings", meta = (ToolTip = "If the Auto Paint isn't currently auto painting anything or waiting for a delay to finish to queue up something else, then we start a looping timer to verify the cached meshes so in case any gets destroyed they get cleaned up. Default to 60 which the default GC interval. "))
	float VerifyMeshComponentsInterval = 60;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint|Rounds of Tasks Settings", meta = (ToolTip = "If all Tasks has to be Finished before we can Add new ones. This may be useful if this is something like Auto Paint Entire Mesh for Drying of Wet Characters and other Meshes that may have different amount of vertcies. Because then their tasks will not finish exactly the same, meaning they could get dried unevenly fast. \nBut with this, all of the Meshes will run the same amount of Tasks so they would Dry equally fast. \nIf this is true then the Delay Between Tasks set for the Mesh will be ignored since it has to wait for them all to finish, so the one set below will be used instead. \n\nIf This is True, then it will reserve bool3 in the Additional Data Settings to make sure we don't start a new task prematurely, so avoid using that if also using this! "))
	bool OnlyStartNewRoundOfTasksIfAllHasBeenFinished = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint|Rounds of Tasks Settings", meta = (ToolTip = "If the Round of Tasks finishes before this duration, we can invoke a delay with the time left to this duration, so if the round has finished in 0.1 second, but you've set this to 0.3, then we will have a 0.2 second delay before the next round starts. \nThis is useful if for example if you have a centralized system that handles all Drying of meshes, but there is only 1 mesh being dried so the task and round finishes very quickly compared to if you're drying 20 meshes, meaning the mesh dries too fast. \nBut with this, you can have a minimum duration of the round, so things can't dry too fast. \nThe issue of uneven drying can still persist where if drying many and heavy meshes the round may take too long and the drying go to slow instead. This is something we can't do anything about, except simply recommending having lighter meshes, and atleast it's better with too slow and realistic drying instead of too fast. "))
	float MinimumDurationOfRoundOfTasks = 0;


	UPROPERTY(VisibleAnywhere, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint|Auto Paint Root Mesh at Begin Play", meta = (ToolTip = "Some children may not support auto painting at begin play, like Paint At Location since it requires a location. Had to be VisibleAnywhere for EditConditions to work. "))
	bool IsAllowedToAutoPaintAtBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint|Auto Paint Root Mesh at Begin Play", meta = (ToolTip = "If the we should start Auto Painting the Root Component of the Actor this component is on on begin play, if the root comp is a Primitive Component, like static or skeletal mesh. \nThis is useful if the Actor may not be a Blueprint, but just a Static/Skeletal Mesh Actor in the level that you may want to Dry if they become Wet or something. ", EditCondition = "IsAllowedToAutoPaintAtBeginPlay"))
	bool AutoPaintRootMeshAtBeginPlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint|Auto Paint Root Mesh at Begin Play", meta = (ToolTip = "The Delay Settings when starting auto paint at begin play on the root mesh comp. ", EditCondition = "IsAllowedToAutoPaintAtBeginPlay"))
	FRVPDPAutoAddColorSettings AutoPaintRootMeshAtBeginPlay_DelaySettings;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint|Debug Settings")
	bool PrintDebugLogsToScreen = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint|Debug Settings")
	float PrintDebugLogsToScreen_Duration = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint|Debug Settings")
	bool PrintDebugLogsToOutputLog = false;


protected:

	virtual bool ShouldAlwaysTick() const { return AutoPaintingMeshesQueuedUpWithDelay.Num() > 0; }

	virtual void PaintTaskFinishedOnRegisteredMeshComponent_Implementation(const FRVPDPTaskResults& TaskResultInfo, const FRVPDPPaintTaskResultInfo& PaintTaskResultInfo, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalData) override;

	virtual void StartAutoPaintTask(UPrimitiveComponent* MeshComponent);

	virtual void StartNewPaintTaskIfAllowed(UPrimitiveComponent* MeshComponent, const FRVPDPAutoAddColorSettings& AutoAddColorSettings);

	virtual bool CanStartNewTaskBasedOnTaskResult(const FRVPDPTaskResults& TaskResultInfo, const FRVPDPPaintTaskResultInfo& PaintTaskResultInfo, bool& IsFullyPainted, bool& IsCompletelyEmpty);

	virtual bool GetApplyVertexColorSettings(UPrimitiveComponent* MeshComponent, FRVPDPApplyColorSettings& ApplyVertexColorSettings);

	virtual void VerifyAutoPaintingMeshComponents();

	bool IsMeshBeingPaintedOrIsQueuedUp(UPrimitiveComponent* MeshComponent);

	void MeshAddedToBeAutoPainted(UPrimitiveComponent* meshComponent, const FRVPDPAutoAddColorSettings& AutoAddColorSettings);

	void AdjustCallbackSettings(const FRVPDPApplyColorSettings& ApplyVertexColorSettings, FRVPDPTaskCallbackSettings& CallbackSettings);


	UPROPERTY()
	TMap<UPrimitiveComponent*, FRVPDPAutoAddColorSettings> AutoPaintingMeshes;

	bool IsAutoPaintingEnabled = false;

	FRVPDPDebugSettings DebugSettings;


private:

	UFUNCTION()
	void AutoPaintedActorDestroyed(AActor* DestroyedActor);

	UFUNCTION()
	void AutoPaintDelayFinished(UPrimitiveComponent* MeshComponent);

	UFUNCTION()
	void StartNewRoundOfTasks();

	bool DoesAutoPaintOnMeshPaintUsingPhysicsSurface(TArray<FRVPDPPhysicsSurfaceToApplySettings>& PhysicsSurfacesToApply, const FRVPDPApplyColorSettings& ApplyVertexColorsSettings);

	bool DoesAutoPaintOnMeshPaintUsingVertexColorChannels(TArray<EVertexColorChannel>& PaintsOnChannels, const FRVPDPApplyColorSettings& ApplyVertexColorsSettings);

	bool IsAnyTaskGoingToStartAfterDelay();

	void ShouldStartNewRoundOfTasks();

	bool ShouldStartNewTaskOnComponent(UPrimitiveComponent* MeshComponent, const FRVPDPTaskResults& TaskResultInfo, const FRVPDPPaintTaskResultInfo& PaintTaskResultInfo, bool WasTriggeredFromNewRound = false);


	FTimerHandle OnlyStartNewRoundOfTasksDelayTimer;

	float TotalDurationRoundOfTasksTook = 0;

	UPROPERTY()
	TMap<UPrimitiveComponent*, FRVPDPAutoAddColorDelaySettings> AutoPaintingMeshesQueuedUpWithDelay;
};
