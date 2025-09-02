// Copyright 2022-2024 Alexander Floden, Alias Alex River. All Rights Reserved.


#include "VertexPaintDetectionTaskQueue.h"

// Engine
#include "TimerManager.h"
#include "Async/Async.h"
#include "Async/AsyncWork.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Components/PrimitiveComponent.h"

// Plugin
#include "VertexPaintCalculateColorsTask.h"
#include "VertexPaintFunctionLibrary.h"
#include "VertexPaintDetectionSettings.h"
#include "VertexPaintDetectionLog.h"
#include "VertexPaintDetectionProfiling.h"



//-------------------------------------------------------

// Add Calculate Colors Task To Job Queue

bool UVertexPaintDetectionTaskQueue::AddCalculateColorsTaskToQueue(FRVPDPCalculateColorsInfo CalculateColorsInfo, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough, int& CreatedTaskID, bool& CanStartTaskImmediately) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_AddCalculateColorsTaskToQueue);
	RVPDP_CPUPROFILER_STR("Task Queue - AddCalculateColorsTaskToQueue");

	CreatedTaskID = -1;
	CanStartTaskImmediately = false;

	if (!CalculateColorsInfo.TaskFundamentalSettings.TaskWorld.IsValid() || !UVertexPaintFunctionLibrary::IsWorldValid(CalculateColorsInfo.TaskFundamentalSettings.TaskWorld.Get())) {

		RVPDP_LOG(CalculateColorsInfo.TaskFundamentalSettings.DebugSettings, FColor::Red, "Failed to Add Task for Mesh: %s because Task World isn't Valid!", *CalculateColorsInfo.VertexPaintComponentName);

		return false;
	} 


	LastTaskID++;

	if (LastTaskID >= 100000)
		LastTaskID = 1;

	CreatedTaskID = LastTaskID;
	CalculateColorsInfo.TaskID = CreatedTaskID;
	CalculateColorsInfo.TaskResult.TaskID = CreatedTaskID;


	if (CalculateColorsInfo.PaintTaskSettings.GetMeshComponent() && CalculateColorsInfo.PaintTaskSettings.CanClearOutPreviousMeshTasksInQueue) {
		
		RVPDP_LOG(CalculateColorsInfo.TaskFundamentalSettings.DebugSettings, FColor::Cyan, "Actor %s and Mesh %s got Painted with Paint Entire Mesh with Set, Color Snippet or SetMeshComponentWithVertexColors, or any other paint job that applies colors directly. So we Clears the Paint Task Queue for that Mesh since the previous tasks is irrelevant since this will Set the colors to be something, not Adds onto old ones. ", *CalculateColorsInfo.VertexPaintActorName, *CalculateColorsInfo.VertexPaintComponentName);

		RemoveMeshComponentFromDetectionTaskQueue(CalculateColorsInfo.PaintTaskSettings.GetMeshComponent(), true, true);
		RemoveMeshComponentFromPaintTaskQueue(CalculateColorsInfo.PaintTaskSettings.GetMeshComponent(), true, true);
	}


	FRVPDPTaskQueueIDInfo taskQueueIDs;

	if (CalculateColorsInfo.PaintTaskSettings.GetMeshComponent()) {

		taskQueueIDs = ComponentPaintTaskIDs.FindRef(CalculateColorsInfo.GetVertexPaintComponent());
		taskQueueIDs.ComponentTaskIDs.Add(CalculateColorsInfo.TaskID);

		// If doesn't have any more tasks than the one we added we can start it immediately
		if (taskQueueIDs.ComponentTaskIDs.Num() == 1)
			CanStartTaskImmediately = true;


		ComponentPaintTaskIDs.Add(CalculateColorsInfo.GetVertexPaintComponent(), taskQueueIDs);
		CalculateColorsPaintQueue.Add(CalculateColorsInfo.TaskID, CalculateColorsInfo);

		// If Paint Within Area with Complex Shape, then throws a warning if the task queue is over a limit about how the result may not be 100% what you expect
		if (IsValid(CalculateColorsInfo.PaintWithinAreaSettings.GetMeshComponent()) && taskQueueIDs.ComponentTaskIDs.Num() > 5) {

			for (int i = 0; i < CalculateColorsInfo.PaintWithinAreaSettings.WithinAreaSettings.ComponentsToCheckIfIsWithin.Num(); i++) {

				if (CalculateColorsInfo.PaintWithinAreaSettings.WithinAreaSettings.ComponentsToCheckIfIsWithin[i].PaintWithinAreaShape == EPaintWithinAreaShape::IsComplexShape) {

					RVPDP_LOG(CalculateColorsInfo.TaskFundamentalSettings.DebugSettings, FColor::Orange, "Warning that task Paint Within Area with Complex may not yield expected result since the Task Queue for the Mesh is over 5, meaning it may take a few frames until the task runs, and if the Mesh you want to paint paint, and componentToCheckIfWithin has moved away from eachother you may not get the expected result. ");
				}
			}
		}
	}

	else if (CalculateColorsInfo.DetectSettings.GetMeshComponent()) {


		taskQueueIDs = ComponentDetectTaskIDs.FindRef(CalculateColorsInfo.GetVertexPaintComponent());
		taskQueueIDs.ComponentTaskIDs.Add(CalculateColorsInfo.TaskID);

		if (taskQueueIDs.ComponentTaskIDs.Num() == 1)
			CanStartTaskImmediately = true;


		ComponentDetectTaskIDs.Add(CalculateColorsInfo.GetVertexPaintComponent(), taskQueueIDs);
		CalculateColorsDetectionQueue.Add(CalculateColorsInfo.TaskID, CalculateColorsInfo);
	}


	// If the Additional Data is not the default settings, something has changed that we want to pass through then we make sure to cache it
	if (AdditionalDataToPassThrough != FRVPDPAdditionalDataToPassThroughInfo()) {

		TasksAdditionalDataToPassThrough.Add(CalculateColorsInfo.TaskID, AdditionalDataToPassThrough);

		RVPDP_LOG(CalculateColorsInfo.TaskFundamentalSettings.DebugSettings, FColor::Cyan, "Has Any Additional Data that needs to be passed through so caches it for Task: %i", CalculateColorsInfo.TaskID);
	}


	RVPDP_LOG(CalculateColorsInfo.TaskFundamentalSettings.DebugSettings, FColor::Cyan, "Added Task for Mesh: %s  -  with ID:  %i  -  Current Amount of Tasks for Mesh: %i", *CalculateColorsInfo.VertexPaintComponentName, CalculateColorsInfo.TaskID, taskQueueIDs.ComponentTaskIDs.Num());

	return true;
}


//-------------------------------------------------------

// Start Calculate Colors Task

bool UVertexPaintDetectionTaskQueue::StartCalculateColorsTask(int TaskID) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_StartCalculateColorsTask);
	RVPDP_CPUPROFILER_STR("Task Queue - StartCalculateColorsTask");

	const UVertexPaintDetectionSettings* vertexPaintDetectionSettings = GetDefault<UVertexPaintDetectionSettings>();
	if (!vertexPaintDetectionSettings) return false;


	int maxAmountOfActiveTasks = 0;

#if WITH_EDITOR
	maxAmountOfActiveTasks = vertexPaintDetectionSettings->MaxAmountOfActiveTasksInEditor;
#elif UE_BUILD_DEVELOPMENT
	maxAmountOfActiveTasks = vertexPaintDetectionSettings->MaxAmountOfActiveTasksInDevelopmentBuild;
#elif UE_BUILD_SHIPPING
	maxAmountOfActiveTasks = vertexPaintDetectionSettings->MaxAmountOfActiveTasksInShippingBuild;
#endif


	FRVPDPCalculateColorsInfo* calculateColorsInfo = nullptr;

	if (auto* paintTaskCalculateColorsInfo = CalculateColorsPaintQueue.Find(TaskID)) {

		if (OnGoingPaintTasksIDs.Contains(TaskID)) return false;


		// If has max amount of active tasks set to be a limit, and if we're going to go past the limit, then we don't start the next one but does so when a Task has been finished
		if (maxAmountOfActiveTasks > 0 && OnGoingPaintTasksIDs.Num() >= maxAmountOfActiveTasks) {

			if (!PaintTasksToStartNext.Contains(TaskID)) {

				PaintTasksToStartNext.Add(TaskID);

				RVPDP_LOG(paintTaskCalculateColorsInfo->TaskFundamentalSettings.DebugSettings, FColor::Orange, "Trying to start Paint Task: %i but we've reached the Max Amount of Active Tasks Limit of %i, so Adds the Task to a seperate queue so it will get started when it's its turn. ", TaskID, maxAmountOfActiveTasks);
			}

			return false;
		} 


		paintTaskCalculateColorsInfo->TaskStartedTimeStamp = paintTaskCalculateColorsInfo->TaskFundamentalSettings.TaskWorld->RealTimeSeconds;
		calculateColorsInfo = paintTaskCalculateColorsInfo;
	}

	else if (auto* detectTaskCalculateColorsInfo = CalculateColorsDetectionQueue.Find(TaskID)) {

		if (OnGoingDetectTasksIDs.Contains(TaskID)) return false;
		if (maxAmountOfActiveTasks > 0 && OnGoingDetectTasksIDs.Num() >= maxAmountOfActiveTasks) {

			if (!DetectTasksToStartNext.Contains(TaskID)) {

				DetectTasksToStartNext.Add(TaskID);

				RVPDP_LOG(detectTaskCalculateColorsInfo->TaskFundamentalSettings.DebugSettings, FColor::Orange, "Trying to start Detect Task: %i but we've reached the Max Amount of Active Tasks Limit of %i, so Adds the Task to a seperate queue so it will get started when it's its turn. ", TaskID, maxAmountOfActiveTasks);
			}

			return false;
		}


		detectTaskCalculateColorsInfo->TaskStartedTimeStamp = detectTaskCalculateColorsInfo->TaskFundamentalSettings.TaskWorld->RealTimeSeconds;
		calculateColorsInfo = detectTaskCalculateColorsInfo;
	}

	else {

		return false;
	}


	FVertexPaintCalculateColorsTask calculateColorsTask;
	calculateColorsTask.SetCalculateColorsInfo(*calculateColorsInfo);
	calculateColorsTask.AsyncTaskFinishedDelegate.BindUObject(this, &UVertexPaintDetectionTaskQueue::CalculateColorsTaskFinished);


	if (FPlatformProcess::SupportsMultithreading() && calculateColorsInfo->TaskFundamentalSettings.MultiThreadSettings.UseMultithreadingForCalculations) {


		// Clears reset timer if new Tasks is going to be started
		if (TaskQueueThreadPoolResetTimer.IsValid() && GetWorld()) {

			if (GetWorld()->GetTimerManager().TimerExists(TaskQueueThreadPoolResetTimer))
				GetWorld()->GetTimerManager().ClearTimer(TaskQueueThreadPoolResetTimer);
		}


		bool successfulAtCreatingThreadPool = true;

		// If threadpool hasn't already been created. It gets destroyed when a task is finished and there are no more left
		if (!TaskQueueThreadPool) {


			int32 numberOfThreads = FGenericPlatformMisc::NumberOfWorkerThreadsToSpawn();
			EThreadPriority threadPriority = EThreadPriority::TPri_Highest;
			FString threadPrioString = "Highest"; // EThreadPriority isn't UENUM so couldn't comfortably print it as string below so using this instead


			if (vertexPaintDetectionSettings->UseMaximumAmountOfCoresForMultithreading)
				numberOfThreads = FGenericPlatformMisc::NumberOfCoresIncludingHyperthreads();


			switch (vertexPaintDetectionSettings->MultithreadPriority) {

			case ETVertexPaintThreadPriority::TimeCritical:
				threadPriority = EThreadPriority::TPri_TimeCritical;
				threadPrioString = "Time Critical";
				break;

			case ETVertexPaintThreadPriority::Highest:
				threadPriority = EThreadPriority::TPri_Highest;
				threadPrioString = "Highest";
				break;

			case ETVertexPaintThreadPriority::Normal:
				threadPriority = EThreadPriority::TPri_Normal;
				threadPrioString = "Normal";
				break;

			case ETVertexPaintThreadPriority::Slowest:
				threadPriority = EThreadPriority::TPri_Lowest;
				threadPrioString = "Slowest";
				break;

			default:
				break;
			}


			TaskQueueThreadPool = FQueuedThreadPool::Allocate();

			// Default Stack Size of 2MB since we're doing such heavy stuff. 
			successfulAtCreatingThreadPool = TaskQueueThreadPool->Create(numberOfThreads, 2 * (1024 * 1024), threadPriority, TEXT("Runtime Vertex Paint and Detection Plugin Thread"));
			

			if (successfulAtCreatingThreadPool) {

				RVPDP_LOG(calculateColorsInfo->TaskFundamentalSettings.DebugSettings, FColor::Cyan, "Creating Thread Pool with Priority: %s  -  Num of Threads: %i", *threadPrioString, numberOfThreads);
			}

			else {

				if (TaskQueueThreadPool)
					delete TaskQueueThreadPool;

				TaskQueueThreadPool = nullptr;

				RVPDP_LOG(calculateColorsInfo->TaskFundamentalSettings.DebugSettings, FColor::Cyan, "Failed at Creating Thread Pool. ");
			}
		}


		if (successfulAtCreatingThreadPool && TaskQueueThreadPool) {


			if (calculateColorsInfo->IsDetectTask)
				OnGoingDetectTasksIDs.Add(calculateColorsInfo->TaskID);

			else if (calculateColorsInfo->IsPaintTask)
				OnGoingPaintTasksIDs.Add(calculateColorsInfo->TaskID);

			if (calculateColorsInfo->GetVertexPaintActor() && calculateColorsInfo->GetVertexPaintComponent()) {

				RVPDP_LOG(calculateColorsInfo->TaskFundamentalSettings.DebugSettings, FColor::Cyan, "Starting Task %i on Async Thread for Actor: %s and Mesh: %s. ", calculateColorsInfo->TaskID, *calculateColorsInfo->VertexPaintActorName, *calculateColorsInfo->VertexPaintComponentName);
			}

			(new FAutoDeleteAsyncTask<FVertexPaintCalculateColorsTask>(calculateColorsTask))->StartBackgroundTask(TaskQueueThreadPool);

			return true;
		}
	}

	else {

		if (calculateColorsInfo->TaskFundamentalSettings.MultiThreadSettings.UseMultithreadingForCalculations) {

			RVPDP_LOG(calculateColorsInfo->TaskFundamentalSettings.DebugSettings, FColor::Orange, "Trying to run Calculations on Multithread but platform does not support it. Falling back to Game Thread instead!");
		}


		if (calculateColorsInfo->IsDetectTask)
			OnGoingDetectTasksIDs.Add(calculateColorsInfo->TaskID);

		else if (calculateColorsInfo->IsPaintTask)
			OnGoingPaintTasksIDs.Add(calculateColorsInfo->TaskID);

		RVPDP_LOG(calculateColorsInfo->TaskFundamentalSettings.DebugSettings, FColor::Cyan, "Starting Task %i on Game Thread for Actor: %s and Mesh: %s", calculateColorsInfo->TaskID, * calculateColorsInfo->VertexPaintActorName, *calculateColorsInfo->VertexPaintComponentName);

		(new FAutoDeleteAsyncTask<FVertexPaintCalculateColorsTask>(calculateColorsTask))->StartSynchronousTask();

		return true;
	}

	return false;
}


//-------------------------------------------------------

// Calculate Colors Task Finished

void UVertexPaintDetectionTaskQueue::CalculateColorsTaskFinished(const FRVPDPCalculateColorsInfo& CalculateColorsInfo) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_CalculateColorsTaskFinished);
	RVPDP_CPUPROFILER_STR("Task Queue - CalculateColorsTaskFinished");


	if (CalculateColorsInfo.IsDetectTask) {

		if (OnGoingDetectTasksIDs.Contains(CalculateColorsInfo.TaskID))
			OnGoingDetectTasksIDs.Remove(CalculateColorsInfo.TaskID);
	}

	else if (CalculateColorsInfo.IsPaintTask) {

		if (OnGoingPaintTasksIDs.Contains(CalculateColorsInfo.TaskID))
			OnGoingPaintTasksIDs.Remove(CalculateColorsInfo.TaskID);
	}


	if (IsInGameThread()) {

		RVPDP_LOG(CalculateColorsInfo.TaskFundamentalSettings.DebugSettings, FColor::Cyan, "Calculate Colors Task Finished for Task ID: %i", CalculateColorsInfo.TaskID);
	}

	else {

		RVPDP_LOG(CalculateColorsInfo.TaskFundamentalSettings.DebugSettings, FColor::Red, "Calculate Colors Task Finished for Task: %i but NOT in Game Thread as it should!", CalculateColorsInfo.TaskID);

		return;
	}


	const bool isMeshComponentStillValid = IsValid(CalculateColorsInfo.GetVertexPaintComponent());


	if (isMeshComponentStillValid) {


		FRVPDPTaskQueueIDInfo taskQueueIDs;

		if (CalculateColorsInfo.PaintTaskSettings.GetMeshComponent() && ComponentPaintTaskIDs.Contains(CalculateColorsInfo.GetVertexPaintComponent())) {


			taskQueueIDs = ComponentPaintTaskIDs.FindRef(CalculateColorsInfo.GetVertexPaintComponent());

			if (taskQueueIDs.ComponentTaskIDs.IsValidIndex(0))
				taskQueueIDs.ComponentTaskIDs.RemoveAt(0);


			if (taskQueueIDs.ComponentTaskIDs.Num() == 0)
				ComponentPaintTaskIDs.Remove(CalculateColorsInfo.GetVertexPaintComponent());

			else
				ComponentPaintTaskIDs.Add(CalculateColorsInfo.GetVertexPaintComponent(), taskQueueIDs);
		}

		if (CalculateColorsInfo.DetectSettings.GetMeshComponent() && ComponentDetectTaskIDs.Contains(CalculateColorsInfo.GetVertexPaintComponent())) {


			taskQueueIDs = ComponentDetectTaskIDs.FindRef(CalculateColorsInfo.GetVertexPaintComponent());

			if (taskQueueIDs.ComponentTaskIDs.IsValidIndex(0))
				taskQueueIDs.ComponentTaskIDs.RemoveAt(0);


			if (taskQueueIDs.ComponentTaskIDs.Num() == 0)
				ComponentDetectTaskIDs.Remove(CalculateColorsInfo.GetVertexPaintComponent());

			else
				ComponentDetectTaskIDs.Add(CalculateColorsInfo.GetVertexPaintComponent(), taskQueueIDs);
		}


		RVPDP_LOG(CalculateColorsInfo.TaskFundamentalSettings.DebugSettings, FColor::Cyan, "Amount of Tasks left for Component: %s  -  %i", *CalculateColorsInfo.VertexPaintComponentName, taskQueueIDs.ComponentTaskIDs.Num());
	}


	bool taskWasStillPartOfQueue = false;


	if (CalculateColorsDetectionQueue.Contains(CalculateColorsInfo.TaskID)) {

		RemoveTaskIDFromDetectionTaskQueue(CalculateColorsInfo.TaskID);
		taskWasStillPartOfQueue = true;
	}

	if (CalculateColorsPaintQueue.Contains(CalculateColorsInfo.TaskID)) {

		RemoveTaskIDFromPaintTaskQueue(CalculateColorsInfo.TaskID);
		taskWasStillPartOfQueue = true;
	}


	RVPDP_LOG(CalculateColorsInfo.TaskFundamentalSettings.DebugSettings, FColor::Cyan, "Total Amount of Paint Tasks Left: %i  -  Total Amount of Detect Tasks Left: %i  -  Task still Valid: %i", CalculateColorsPaintQueue.Num(), CalculateColorsDetectionQueue.Num(), taskWasStillPartOfQueue);


	bool sourceMeshHasChanged = false;

#if ENGINE_MAJOR_VERSION == 5

	sourceMeshHasChanged = (CalculateColorsInfo.VertexPaintSourceMesh != UVertexPaintFunctionLibrary::GetMeshComponentSourceMesh(CalculateColorsInfo.GetVertexPaintComponent()) && !CalculateColorsInfo.GetVertexPaintDynamicMeshComponent());

#elif ENGINE_MAJOR_VERSION == 4

	sourceMeshHasChanged = (CalculateColorsInfo.VertexPaintSourceMesh != UVertexPaintFunctionLibrary::GetMeshComponentSourceMesh(CalculateColorsInfo.GetVertexPaintComponent()));

#endif

	
	// If Failed Task because of these various reasons
	if (!CalculateColorsInfo.TaskResult.TaskSuccessfull && (sourceMeshHasChanged || !isMeshComponentStillValid || !taskWasStillPartOfQueue)) {


		// If was still a part of the detect or paint task queues, then need to clear out the queue from anymore related to the mesh
		if (taskWasStillPartOfQueue) {


			RVPDP_LOG(CalculateColorsInfo.TaskFundamentalSettings.DebugSettings, FColor::Red, "Task: %i Failed. Source Mesh, Paint Component or the Task isn't valid anymore. ", CalculateColorsInfo.TaskID);


			if (CalculateColorsInfo.IsDetectTask) {

				// If still valid mesh comp but just the source mesh has changed
				if (isMeshComponentStillValid) {

					if (sourceMeshHasChanged)
						RemoveMeshComponentFromDetectionTaskQueue(CalculateColorsInfo.GetVertexPaintComponent());
					else
						RefreshDetectionTaskQueueWithValidComponents();
				}

				// Otherwise need to completely refresh the queue since can't do it faster with the ptr
				else {

					RefreshDetectionTaskQueueWithValidComponents();
				}
			}

			else {


				if (isMeshComponentStillValid) {

					if (sourceMeshHasChanged)
						RemoveMeshComponentFromPaintTaskQueue(CalculateColorsInfo.GetVertexPaintComponent());
					else
						RefreshPaintTaskQueueWithValidComponents();
				}

				else {

					RefreshPaintTaskQueueWithValidComponents();
				}
			}
		}

		else {

			RVPDP_LOG(CalculateColorsInfo.TaskFundamentalSettings.DebugSettings, FColor::Red, "Finished Task: %i is not Valid anymore. Possibly a paint job that Sets the colors was created After this task, which makes this task invalid since there's no use for it's result. ", CalculateColorsInfo.TaskID);
		}

		TaskFinishedDelegate.Execute(CalculateColorsInfo, false);
	}

	else if (CalculateColorsInfo.TaskResult.TaskSuccessfull) {

		RVPDP_LOG(CalculateColorsInfo.TaskFundamentalSettings.DebugSettings, FColor::Green, "Task Successfully Finished for Actor: %s on Mesh Component: %s  -  Runs Wrap Up!", *CalculateColorsInfo.VertexPaintActorName, *CalculateColorsInfo.VertexPaintComponentName);

		TaskFinishedDelegate.Execute(CalculateColorsInfo, true);
	}

	else {

		RVPDP_LOG(CalculateColorsInfo.TaskFundamentalSettings.DebugSettings, FColor::Red, "Task Failed with ID: %i!", CalculateColorsInfo.TaskID);

		TaskFinishedDelegate.Execute(CalculateColorsInfo, false);
	}


	// Removes Cached Settings for the Task as we're by this point completely done with it and want to check if we should start another task for the mesh, if it's valid. 
	if (TasksAdditionalDataToPassThrough.Contains(CalculateColorsInfo.TaskID))
		TasksAdditionalDataToPassThrough.Remove(CalculateColorsInfo.TaskID);


	// If there is any paint tasks that is set to start next then they get higher prio
	if (CalculateColorsInfo.IsPaintTask) {


		int paintTaskToStartNextID = -1;

		if (PaintTasksToStartNext.Num() > 0) {

			paintTaskToStartNextID = PaintTasksToStartNext[0];
			PaintTasksToStartNext.RemoveAt(0);

			RVPDP_LOG(CalculateColorsInfo.TaskFundamentalSettings.DebugSettings, FColor::Cyan, "Starting Paint Task %i that had been placed in a seperate Task Queue because we reached the Max Allowed Active Task Limit. ", paintTaskToStartNextID);

			StartCalculateColorsTask(paintTaskToStartNextID);
		}

		if (isMeshComponentStillValid) {

			if (ComponentPaintTaskIDs.Contains(CalculateColorsInfo.GetVertexPaintComponent())) {


				const int paintMeshNextTaskID = ComponentPaintTaskIDs.FindRef(CalculateColorsInfo.GetVertexPaintComponent()).ComponentTaskIDs[0];

				// If the next in this mesh queue wasnt the one that just got started above
				if (paintTaskToStartNextID != paintMeshNextTaskID) {

					if (CalculateColorsPaintQueue.Contains(paintMeshNextTaskID)) {

						RVPDP_LOG(CalculateColorsInfo.TaskFundamentalSettings.DebugSettings, FColor::Cyan, "Starts the next Paint Task in Queue for Actor: %s and Mesh: %s with Task ID: %i", *CalculateColorsInfo.GetVertexPaintComponent()->GetOwner()->GetName(), *CalculateColorsInfo.VertexPaintComponentName, paintMeshNextTaskID);

						StartCalculateColorsTask(paintMeshNextTaskID);
					}
				}
			}
		}
	}

	if (CalculateColorsInfo.IsDetectTask) {


		int detectTaskToStartNextID = -1;

		if (DetectTasksToStartNext.Num() > 0) {

			detectTaskToStartNextID = DetectTasksToStartNext[0];
			DetectTasksToStartNext.RemoveAt(0);

			RVPDP_LOG(CalculateColorsInfo.TaskFundamentalSettings.DebugSettings, FColor::Cyan, "Starting Detect Task %i that had been placed in a seperate Task Queue because we reached the Max Allowed Active Task Limit. ", detectTaskToStartNextID);

			StartCalculateColorsTask(detectTaskToStartNextID);
		}

		if (isMeshComponentStillValid) {

			// If it exists at all it means that it has more tasks for the mesh, we grabs the first one
			if (ComponentDetectTaskIDs.Contains(CalculateColorsInfo.GetVertexPaintComponent())) {
				

				const int detectMeshNextTaskID = ComponentDetectTaskIDs.FindRef(CalculateColorsInfo.GetVertexPaintComponent()).ComponentTaskIDs[0];

				if (detectTaskToStartNextID != detectMeshNextTaskID) {

					if (CalculateColorsDetectionQueue.Contains(detectMeshNextTaskID)) {

						RVPDP_LOG(CalculateColorsInfo.TaskFundamentalSettings.DebugSettings, FColor::Cyan, "Starts the next Detect Task in Queue for Actor: %s and Mesh: %s with Task ID: %i", *CalculateColorsInfo.GetVertexPaintComponent()->GetOwner()->GetName(), *CalculateColorsInfo.VertexPaintComponentName, detectMeshNextTaskID);

						StartCalculateColorsTask(detectMeshNextTaskID);
					}
				}
			}
		}
	}


	ShouldStartResetThreadPoolTimer();
}


//-------------------------------------------------------

// Reset Thread Pool

void UVertexPaintDetectionTaskQueue::ResetThreadPool() {

	if (TaskQueueThreadPool && CalculateColorsPaintQueue.Num() == 0 && CalculateColorsDetectionQueue.Num() == 0) {

		RVPDP_LOG(FRVPDPDebugSettings(false, 0, true), FColor::Cyan, "Reset Thread Pool!");

		TaskQueueThreadPool->Destroy();
		delete TaskQueueThreadPool;
		TaskQueueThreadPool = nullptr;
	}
}


//-------------------------------------------------------

// Should Start Reset Thread Pool Timer

void UVertexPaintDetectionTaskQueue::ShouldStartResetThreadPoolTimer() {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_ShouldStartResetThreadPoolTimer);
	RVPDP_CPUPROFILER_STR("Task Queue - ShouldStartResetThreadPoolTimer");


	// If no more tasks left then starts a timer to reset the thread pool
	if (CalculateColorsPaintQueue.Num() == 0 && CalculateColorsDetectionQueue.Num() == 0) {

		if (IsValid(GetWorld())) {

			GetWorld()->GetTimerManager().SetTimer(TaskQueueThreadPoolResetTimer, this, &UVertexPaintDetectionTaskQueue::ResetThreadPool, 10, false, 10);
		}
		else {

			ResetThreadPool();
		}
	}

	else {

		if (GetWorld()) {

			if (GetWorld()->GetTimerManager().TimerExists(TaskQueueThreadPoolResetTimer))
				GetWorld()->GetTimerManager().ClearTimer(TaskQueueThreadPoolResetTimer);
		}
	}
}


//-------------------------------------------------------

// Get Calculate Colors Paint / Detection Task Amount

TMap<UPrimitiveComponent*, int> UVertexPaintDetectionTaskQueue::GetCalculateColorsPaintTasksAmountPerComponent() {


	TMap<UPrimitiveComponent*, int> meshesAndTaskAmounts;

	for (auto& taskIDs : ComponentPaintTaskIDs)
		meshesAndTaskAmounts.Add(taskIDs.Key, taskIDs.Value.ComponentTaskIDs.Num());

	return meshesAndTaskAmounts;
}

TMap<UPrimitiveComponent*, int> UVertexPaintDetectionTaskQueue::GetCalculateColorsDetectionTasksAmountPerComponent() {


	TMap<UPrimitiveComponent*, int> meshesAndTaskAmounts;

	for (auto& taskIDs : ComponentDetectTaskIDs)
		meshesAndTaskAmounts.Add(taskIDs.Key, taskIDs.Value.ComponentTaskIDs.Num());

	return meshesAndTaskAmounts;
}


//-------------------------------------------------------

// Remove Mesh Component From Paint / Detection Task Queue

void UVertexPaintDetectionTaskQueue::RemoveMeshComponentFromDetectionTaskQueue(const UPrimitiveComponent* MeshComp, bool RemoveCachesTaskSettings, bool ShouldRunFailedDelegate) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_RemoveMeshComponentFromDetectionTaskQueue);
	RVPDP_CPUPROFILER_STR("Task Queue - RemoveMeshComponentFromDetectionTaskQueue");


	if (!IsValid(MeshComp)) return;


	if (ComponentDetectTaskIDs.Contains(MeshComp)) {

		TArray<int> componentTaskIDs = ComponentDetectTaskIDs.FindRef(MeshComp).ComponentTaskIDs;

		// Reverse loops as removes all with the same task ID
		for (int i = componentTaskIDs.Num() - 1; i >= 0; i--)
			RemoveTaskIDFromDetectionTaskQueue(componentTaskIDs[i], RemoveCachesTaskSettings, ShouldRunFailedDelegate);

		ComponentDetectTaskIDs.Remove(MeshComp);
	}
}

void UVertexPaintDetectionTaskQueue::RemoveMeshComponentFromPaintTaskQueue(const UPrimitiveComponent* MeshComp, bool RemoveCachesTaskSettings, bool ShouldRunFailedDelegate) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_RemoveMeshComponentFromPaintTaskQueue);
	RVPDP_CPUPROFILER_STR("Task Queue - RemoveMeshComponentFromPaintTaskQueue");

	if (!IsValid(MeshComp)) return;


	if (ComponentPaintTaskIDs.Contains(MeshComp)) {

		TArray<int> componentTaskIDs = ComponentPaintTaskIDs.FindRef(MeshComp).ComponentTaskIDs;

		// Reverse loops as removes all with the same task ID
		for (int i = componentTaskIDs.Num() - 1; i >= 0; i--)
			RemoveTaskIDFromPaintTaskQueue(componentTaskIDs[i], RemoveCachesTaskSettings, ShouldRunFailedDelegate);

		ComponentPaintTaskIDs.Remove(MeshComp);
	}
}


//-------------------------------------------------------

// Remove Task ID From Paint / Detection Task Queue

void UVertexPaintDetectionTaskQueue::RemoveTaskIDFromPaintTaskQueue(int TaskID, bool RemoveCachesTaskSettings, bool ShouldRunFailedDelegate) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_RemoveTaskIDFromPaintTaskQueue);
	RVPDP_CPUPROFILER_STR("Task Queue - RemoveTaskIDFromPaintTaskQueue");


	if (PaintTasksToStartNext.Contains(TaskID))
		PaintTasksToStartNext.Remove(TaskID);


	if (CalculateColorsPaintQueue.Contains(TaskID)) {

		// If already on going, then we don't want to remove cached settings since we need it when the task finishes. The delegate will also run when finishes so don't want to run it twice
		if (!OnGoingPaintTasksIDs.Contains(TaskID)) {

			if (RemoveCachesTaskSettings) {

				if (TasksAdditionalDataToPassThrough.Contains(TaskID))
					TasksAdditionalDataToPassThrough.Remove(TaskID);
			}

			if (ShouldRunFailedDelegate)
				TaskFinishedDelegate.Execute(CalculateColorsPaintQueue.FindRef(TaskID), false);
		}


		CalculateColorsPaintQueue.Remove(TaskID);
	}
};

void UVertexPaintDetectionTaskQueue::RemoveTaskIDFromDetectionTaskQueue(int TaskID, bool RemoveCachesTaskSettings, bool ShouldRunFailedDelegate) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_RemoveTaskIDFromDetectionTaskQueue);
	RVPDP_CPUPROFILER_STR("Task Queue - RemoveTaskIDFromDetectionTaskQueue");


	if (DetectTasksToStartNext.Contains(TaskID))
		DetectTasksToStartNext.Remove(TaskID);


	if (CalculateColorsDetectionQueue.Contains(TaskID)) {

		// If already on going, then we don't want to remove cached settings since we need it when the task finishes. The delegate will also run when finishes so don't want to run it twice
		if (!OnGoingDetectTasksIDs.Contains(TaskID)) {

			if (RemoveCachesTaskSettings) {

				if (TasksAdditionalDataToPassThrough.Contains(TaskID))
					TasksAdditionalDataToPassThrough.Remove(TaskID);
			}

			if (ShouldRunFailedDelegate)
				TaskFinishedDelegate.Execute(CalculateColorsDetectionQueue.FindRef(TaskID), false);
		}

		CalculateColorsDetectionQueue.Remove(TaskID);
	}
};


//-------------------------------------------------------

// Clear Calculate Colors Paint / Detection Tasks

void UVertexPaintDetectionTaskQueue::ClearCalculateColorsPaintTasks() {

	CalculateColorsPaintQueue.Empty();
	PaintTasksToStartNext.Empty();
}

void UVertexPaintDetectionTaskQueue::ClearCalculateColorsDetectionTasks() {

	CalculateColorsDetectionQueue.Empty();
	DetectTasksToStartNext.Empty();
}


//-------------------------------------------------------

// Get Amount Of Paint / Detection Tasks Component Has

int UVertexPaintDetectionTaskQueue::GetAmountOfDetectionTasksComponentHas(const UPrimitiveComponent* MeshComp) const {

	if (!IsValid(MeshComp) || ComponentDetectTaskIDs.Num() == 0) return 0;


	if (ComponentDetectTaskIDs.Contains(MeshComp))
		return ComponentDetectTaskIDs.FindRef(MeshComp).ComponentTaskIDs.Num();


	return 0;
}

int UVertexPaintDetectionTaskQueue::GetAmountOfPaintTasksComponentHas(const UPrimitiveComponent* MeshComp) const {

	if (!IsValid(MeshComp) || ComponentPaintTaskIDs.Num() == 0) return 0;


	if (ComponentPaintTaskIDs.Contains(MeshComp))
		return ComponentPaintTaskIDs.FindRef(MeshComp).ComponentTaskIDs.Num();


	return 0;
}


//-------------------------------------------------------

// Refreshes Paint /Detection Task Queue With Valid Mesh Components

void UVertexPaintDetectionTaskQueue::RefreshDetectionTaskQueueWithValidComponents() {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_RefreshDetectionTaskQueueWithValidComponents);
	RVPDP_CPUPROFILER_STR("Task Queue - RefreshDetectionTaskQueueWithValidComponents");

	if (CalculateColorsDetectionQueue.Num() == 0 && ComponentDetectTaskIDs.Num() == 0) return;


	TArray<int> invalidTasks;
	TMap<UPrimitiveComponent*, FRVPDPTaskQueueIDInfo> validComponentTaskIDs;

	// Rebuilds the Component and Task Array TMap with only valid components
	for (auto& componentTaskID : ComponentDetectTaskIDs) {

		if (IsValid(componentTaskID.Key)) {

			validComponentTaskIDs.Add(componentTaskID.Key, componentTaskID.Value);
		}

		else {

			invalidTasks.Append(componentTaskID.Value.ComponentTaskIDs);
		}
	}

	ComponentDetectTaskIDs = validComponentTaskIDs;


	if (CalculateColorsDetectionQueue.Num() > 0) {

		// Removes all of the Invalid IDs without having to store a bunch of heavy local vars
		for (int i = invalidTasks.Num() - 1; i >= 0; i--) {

			RemoveTaskIDFromDetectionTaskQueue(invalidTasks[i], true, true);
		}
	}
}

void UVertexPaintDetectionTaskQueue::RefreshPaintTaskQueueWithValidComponents() {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_RefreshPaintTaskQueueWithValidComponents);
	RVPDP_CPUPROFILER_STR("Task Queue - RefreshPaintTaskQueueWithValidComponents");

	if (CalculateColorsPaintQueue.Num() == 0 && ComponentPaintTaskIDs.Num() == 0) return;


	TArray<int> invalidTasks;
	TMap<UPrimitiveComponent*, FRVPDPTaskQueueIDInfo> validComponentTaskIDs;

	// Rebuilds the Component and Task Array TMap with only valid components
	for (auto& componentTaskID : ComponentPaintTaskIDs) {

		if (IsValid(componentTaskID.Key)) {

			validComponentTaskIDs.Add(componentTaskID.Key, componentTaskID.Value);
		}

		else {

			invalidTasks.Append(componentTaskID.Value.ComponentTaskIDs);
		}
	}

	ComponentPaintTaskIDs = validComponentTaskIDs;


	if (CalculateColorsPaintQueue.Num() > 0) {

		// Removes all of the Invalid IDs without having to store a bunch of heavy local vars
		for (int i = invalidTasks.Num() - 1; i >= 0; i--) {

			RemoveTaskIDFromPaintTaskQueue(invalidTasks[i], true);
		}
	}
}


//-------------------------------------------------------

// Get Tasks Additional Data To Pass Through

FRVPDPAdditionalDataToPassThroughInfo UVertexPaintDetectionTaskQueue::GetAdditionalDataToPassThroughForTask(int TaskID) const {

	if (TaskID < 0) return FRVPDPAdditionalDataToPassThroughInfo();


	if (TasksAdditionalDataToPassThrough.Contains(TaskID))
		return TasksAdditionalDataToPassThrough.FindRef(TaskID);

	return FRVPDPAdditionalDataToPassThroughInfo();
}