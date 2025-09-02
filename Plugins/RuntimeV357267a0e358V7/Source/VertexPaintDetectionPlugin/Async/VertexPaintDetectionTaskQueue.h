// Copyright 2022 Alexander Floden, Alias Alex River. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/QueuedThreadPool.h"
#include "CalculateColorsPrerequisites.h"
#include "Engine/EngineTypes.h"
#include "VertexPaintDetectionTaskQueue.generated.h"


DECLARE_DELEGATE_TwoParams(FTaskFinished, const FRVPDPCalculateColorsInfo&, bool TaskSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTaskRemovedFromQueue, const FRVPDPCalculateColorsInfo&, TaskRemoved);



USTRUCT()
struct FRVPDPTaskQueueIDInfo {

	GENERATED_BODY()

	TArray<int> ComponentTaskIDs;
};


/**
 * 
 */
UCLASS()
class VERTEXPAINTDETECTIONPLUGIN_API UVertexPaintDetectionTaskQueue : public UObject
{
	GENERATED_BODY()


public:

	//---------- TASK QUEUE ----------//

	// General

	bool AddCalculateColorsTaskToQueue(FRVPDPCalculateColorsInfo CalculateColorsInfo, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough, int& CreatedTaskID, bool& CanStartTaskImmediately);

	bool StartCalculateColorsTask(int TaskID);

	const TMap<int, FRVPDPAdditionalDataToPassThroughInfo>& GetTasksAdditionalDataToPassThrough() { return TasksAdditionalDataToPassThrough; };

	FRVPDPAdditionalDataToPassThroughInfo GetAdditionalDataToPassThroughForTask(int TaskID) const;

	FTaskFinished TaskFinishedDelegate;


private:

	const TArray<int32>& GetOnGoingAsyncTasksIDs() { return OnGoingPaintTasksIDs; }

	void CalculateColorsTaskFinished(const FRVPDPCalculateColorsInfo& CalculateColorsInfo);


public:

	// Paint

	TMap<UPrimitiveComponent*, int> GetCalculateColorsPaintTasksAmountPerComponent();

	int GetAmountOfPaintTasksComponentHas(const UPrimitiveComponent* MeshComp) const;

	const TMap<int, FRVPDPCalculateColorsInfo>& GetCalculateColorsPaintTasks() { return CalculateColorsPaintQueue; }

	void RemoveMeshComponentFromPaintTaskQueue(const UPrimitiveComponent* MeshComp, bool RemoveCachesTaskSettings = false, bool ShouldRunFailedDelegate = false);

	int GetAmountOfOnGoingPaintTasks() const { return OnGoingPaintTasksIDs.Num(); }


private:

	void RemoveTaskIDFromPaintTaskQueue(int TaskID, bool RemoveCachesTaskSettings = false, bool ShouldRunFailedDelegate = false);

	void ClearCalculateColorsPaintTasks();

	void RefreshPaintTaskQueueWithValidComponents();


	// Detection

public:

	TMap<UPrimitiveComponent*, int> GetCalculateColorsDetectionTasksAmountPerComponent();

	int GetAmountOfDetectionTasksComponentHas(const UPrimitiveComponent* MeshComp) const;

	const TMap<int, FRVPDPCalculateColorsInfo>& GetCalculateColorsDetectionTasks() { return CalculateColorsDetectionQueue; }

	void RemoveMeshComponentFromDetectionTaskQueue(const UPrimitiveComponent* MeshComp, bool RemoveCachesTaskSettings = false, bool ShouldRunFailedDelegate = false);

	int GetAmountOfOnGoingDetectTasks() const { return OnGoingDetectTasksIDs.Num(); }


private:

	void RemoveTaskIDFromDetectionTaskQueue(int TaskID, bool RemoveCachesTaskSettings = false, bool ShouldRunFailedDelegate = false);

	void ClearCalculateColorsDetectionTasks();

	void RefreshDetectionTaskQueueWithValidComponents();

	void ResetThreadPool();

	void ShouldStartResetThreadPoolTimer();


	//---------- PROPERTIES ----------//


	TArray<int32> OnGoingDetectTasksIDs;
	TArray<int32> DetectTasksToStartNext;

	TArray<int32> OnGoingPaintTasksIDs;
	TArray<int32> PaintTasksToStartNext;

	UPROPERTY()
	FTimerHandle TaskQueueThreadPoolResetTimer;

	FQueuedThreadPool* TaskQueueThreadPool = nullptr;

	int LastTaskID = 0;


	UPROPERTY()
	TMap<int, FRVPDPAdditionalDataToPassThroughInfo> TasksAdditionalDataToPassThrough;

	// We use 2 sets of TMaps, one with just the task ID and info of that task, so we don't have any dependency on a pointer to just remove or get things from it, and another we're we use the component ptr to the get the amount of tasks and Task IDs of that mesh. By using it, we could make a solution where if a mesh wasn't valid, we could refresh the Task Queue without having to store memory heavy local variables of the task info struct, just check the invalid ptr, get the Tasks IDs that it had and Remove them from the Task queue.
	// The Paint Component also has a list of the paint tasks it has initiated with its ID, this is so we can get if and what tasks a paint component has initiated very cheaply and safe without dependencies to ptrs, compared to if we had a TMap here with the paint component has a key and then a struct with the ID and calculate colors info. That would also be very expensive since we would have to search through it every time if we wanted to check if a paint comp has any tasks initiated, which you do very often if you want to paint often. 
	UPROPERTY()
	TMap<int, FRVPDPCalculateColorsInfo> CalculateColorsPaintQueue;

	UPROPERTY()
	TMap<UPrimitiveComponent*, FRVPDPTaskQueueIDInfo> ComponentPaintTaskIDs;


	UPROPERTY()
	TMap<int, FRVPDPCalculateColorsInfo> CalculateColorsDetectionQueue;

	UPROPERTY()
	TMap<UPrimitiveComponent*, FRVPDPTaskQueueIDInfo> ComponentDetectTaskIDs;
};
