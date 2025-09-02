// Copyright 2022-2024 Alexander Floden, Alias Alex River. All Rights Reserved.


#include "AutoAddColorComponent.h"

// Engine
#include "TimerManager.h"
#include "Components/PrimitiveComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/World.h"

// Plugin
#include "VertexPaintFunctionLibrary.h"
#include "VertexPaintDetectionLog.h"
#include "VertexPaintDetectionProfiling.h"


//-------------------------------------------------------

// Construct

UAutoAddColorComponent::UAutoAddColorComponent() {

	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = true;
}


//-------------------------------------------------------

// Begin Play

void UAutoAddColorComponent::BeginPlay() {

	Super::BeginPlay();

	DebugSettings.PrintLogsToScreen = PrintDebugLogsToScreen;
	DebugSettings.PrintLogsToScreen_Duration = PrintDebugLogsToScreen_Duration;
	DebugSettings.PrintLogsToOutputLog = PrintDebugLogsToOutputLog;

	SetComponentTickEnabled(ShouldAlwaysTick());
}


//-------------------------------------------------------

// Update

void UAutoAddColorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_AutoAddColorComponentTickComponent);
	RVPDP_CPUPROFILER_STR("Auto Add Color Component - Tick");

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (AutoPaintingMeshesQueuedUpWithDelay.Num() == 0) return;



	TArray<UPrimitiveComponent*> meshComponents;
	AutoPaintingMeshesQueuedUpWithDelay.GenerateKeyArray(meshComponents);

	TArray<FRVPDPAutoAddColorDelaySettings> delaySettings;
	AutoPaintingMeshesQueuedUpWithDelay.GenerateValueArray(delaySettings);


	bool verifyCachedTasks = false;


	for (int i = meshComponents.Num() - 1; i >= 0; i--) {

		if (!IsValid(meshComponents[i])) {

			verifyCachedTasks = true;
			continue;
		}


		if (delaySettings[i].DelayBetweenTasks > 0) {


			delaySettings[i].DelayBetweenTasksCounter += DeltaTime;


			if (delaySettings[i].DelayBetweenTasksCounter >= delaySettings[i].DelayBetweenTasks) {

				AutoPaintingMeshesQueuedUpWithDelay.Remove(meshComponents[i]);

				SetComponentTickEnabled(ShouldAlwaysTick());

				AutoPaintDelayFinished(meshComponents[i]);
			}

			else {

				AutoPaintingMeshesQueuedUpWithDelay.Add(meshComponents[i], delaySettings[i]);
			}
		}
	}


	if (verifyCachedTasks)
		VerifyAutoPaintingMeshComponents();
}


//-------------------------------------------------------

// Mesh Added To Be Auto Painted

void UAutoAddColorComponent::MeshAddedToBeAutoPainted(UPrimitiveComponent* meshComponent, const FRVPDPAutoAddColorSettings& AutoAddColorSettings) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_MeshAddedToBeAutoPainted);
	RVPDP_CPUPROFILER_STR("Auto Add Component - MeshAddedToBeAutoPainted");

	if (!IsValid(meshComponent)) return;
	if (!IsValid(meshComponent->GetOwner())) return;


	if (!meshComponent->GetOwner()->OnDestroyed.IsAlreadyBound(this, &UAutoAddColorComponent::AutoPaintedActorDestroyed)) {

		meshComponent->GetOwner()->OnDestroyed.AddDynamic(this, &UAutoAddColorComponent::AutoPaintedActorDestroyed);
	}

	
	AutoPaintingMeshes.Add(meshComponent, AutoAddColorSettings);

	IsAutoPaintingEnabled = true;

	if (!AutoAddColorSettings.IsPaused) {

		UVertexPaintFunctionLibrary::RegisterPaintTaskCallbacksToObjectFromSpecifiedMeshComponent_Wrapper(meshComponent, this);
	}
}


//-------------------------------------------------------

// Is Mesh Being Painted Or Is Queued Up

bool UAutoAddColorComponent::IsMeshBeingPaintedOrIsQueuedUp(UPrimitiveComponent* MeshComponent) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_IsMeshBeingPaintedOrIsQueuedUp);
	RVPDP_CPUPROFILER_STR("Auto Add Component - IsMeshBeingPaintedOrIsQueuedUp");

	if (AutoPaintingMeshesQueuedUpWithDelay.Contains(MeshComponent)) return true;
	if (GetCurrentTasksMeshComponents().Contains(MeshComponent)) return true;

	return false;
}


//-------------------------------------------------------

// Paint Task Finished On Registered Mesh Component

void UAutoAddColorComponent::PaintTaskFinishedOnRegisteredMeshComponent_Implementation(const FRVPDPTaskResults& TaskResultInfo, const FRVPDPPaintTaskResultInfo& PaintTaskResultInfo, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalData) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_PaintTaskFinishedOn);
	RVPDP_CPUPROFILER_STR("Auto Add Component - PaintTaskFinished");

	UPrimitiveComponent* meshComponent = TaskResultInfo.GetMeshComponent();


	// Logs if paint component is still valid
	if (UVertexPaintDetectionComponent* associatedComponent = TaskResultInfo.GetAssociatedPaintComponent()) {

		if (associatedComponent == this) {

			if (IsValid(meshComponent) && IsValid(meshComponent->GetOwner())) {

				RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s - Finished Paint Task for Actor: %s and Mesh: %s! Task Successfull: %i", *GetName(), *meshComponent->GetOwner()->GetName(), *meshComponent->GetName(), TaskResultInfo.TaskSuccessfull);
			}

			else {

				RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s - Finished Paint Task for Mesh that isn't valid anymore", *GetName());

				VerifyAutoPaintingMeshComponents();
				ShouldStartNewRoundOfTasks();
				return;
			}
		}

		else {

			if (IsValid(meshComponent) && IsValid(meshComponent->GetOwner())) {

				RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s - Another Paint Component: %s Finished Paint Task for Actor: %s and Mesh: %s! Task Successfull: %i", *GetName(), *associatedComponent->GetName(), *meshComponent->GetOwner()->GetName(), *meshComponent->GetName(), TaskResultInfo.TaskSuccessfull);
			}

			else {

				RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s - Another Paint Component: %s Finished Paint Task for Mesh that isn't valid anymore", *GetName(), *associatedComponent->GetName());

				VerifyAutoPaintingMeshComponents();
				ShouldStartNewRoundOfTasks();
				return;
			}
		}
	}

	else {

		if (IsValid(meshComponent) && IsValid(meshComponent->GetOwner())) {

			RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s - Task Finished Paint Task for Actor: %s and Mesh: %s but the instigating Paint Component isn't valid Anymore! Task Successfull: %i", *GetName(), *meshComponent->GetOwner()->GetName(), *meshComponent->GetName(), TaskResultInfo.TaskSuccessfull);
		}

		else {

			RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s - Finished Paint Task for Mesh that isn't valid anymore, by a paint component that isn't valid anymore", *GetName());

			VerifyAutoPaintingMeshComponents();
			ShouldStartNewRoundOfTasks();
			return;
		}
	}

	if (IsMeshBeingPaintedOrIsQueuedUp(meshComponent)) return;


	FRVPDPAutoAddColorSettings autoAddColorSettings;

	if (AutoPaintingMeshes.Contains(meshComponent)) {

		autoAddColorSettings = AutoPaintingMeshes.FindRef(meshComponent);
	}

	// If not a part of the list anymore, possible Stopped painting the Mesh in the middle of a task, then it's safe to unregister from it
	else {

		UVertexPaintFunctionLibrary::UnRegisterPaintTaskCallbacksToObjectFromSpecifiedMeshComponent_Wrapper(meshComponent, this);
		ShouldStartNewRoundOfTasks();
		return;
	}


	// If Paused, possible in the middle of a task, then it's safe to unregister from the callbacks so we don't get anymore for this mesh. 
	if (autoAddColorSettings.IsPaused) {

		RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s - A Task Finished for Mesh %s but this Paint Component has Paused it's auto Painting on it so doesn't start a new one!", *GetName(), *meshComponent->GetName());

		UVertexPaintFunctionLibrary::UnRegisterPaintTaskCallbacksToObjectFromSpecifiedMeshComponent_Wrapper(meshComponent, this);
		ShouldStartNewRoundOfTasks();
		return;
	}


	if (OnlyStartNewRoundOfTasksIfAllHasBeenFinished) {


		// Require that if anything should be able to replace the latest cached paint task results, they have to have the data it requires depending on it's color settings.
		FRVPDPApplyColorSettings applyVertexColorSettings;
		GetApplyVertexColorSettings(meshComponent, applyVertexColorSettings);


		TArray<EVertexColorChannel> paintsOnChannels;
		if (DoesAutoPaintOnMeshPaintUsingVertexColorChannels(paintsOnChannels, applyVertexColorSettings)) {

			if (!TaskResultInfo.AmountOfPaintedColorsOfEachChannel.SuccessfullyGotColorChannelResultsAtMinAmount) {

				ShouldStartNewRoundOfTasks();
				return;
			}
		}


		TArray<FRVPDPPhysicsSurfaceToApplySettings> physicsSurfacesToApply;
		if (DoesAutoPaintOnMeshPaintUsingPhysicsSurface(physicsSurfacesToApply, applyVertexColorSettings)) {

			if (!TaskResultInfo.AmountOfPaintedColorsOfEachChannel.SuccessfullyGotPhysicsSurfaceResultsAtMinAmount) {

				ShouldStartNewRoundOfTasks();
				return;
			}
		}


		const int totalTasksInitiatedByComponent = GetTotalTasksInitiatedByComponent();

		if (totalTasksInitiatedByComponent > 0) {

			RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s - Set to Only Start New Task when all is Finished, so caches the task results so can do it all when there is no more expected callbacks. Total Tasks Left that's been Initiated by Component: %i. ", *GetName(), totalTasksInitiatedByComponent);
		}


		FRVPDPAutoPaintTaskResults startNewRoundOfTasksInfo;
		startNewRoundOfTasksInfo.MeshComponent = meshComponent;
		startNewRoundOfTasksInfo.TaskResultInfo = TaskResultInfo;
		startNewRoundOfTasksInfo.PaintTaskResultInfo = PaintTaskResultInfo;
		startNewRoundOfTasksInfo.AdditionalData = AdditionalData;

		// Caches the results so when we start a new round of tasks we can run this based of off the result it had so it can then determine if it should queue up a new one
		autoAddColorSettings.OnlyStartNewRoundOfTasks_FinishedCachedResults = startNewRoundOfTasksInfo;
		autoAddColorSettings.OnlyStartNewRoundOfTasks_HasFinishedCachedResults = true;
		AutoPaintingMeshes.Add(meshComponent, autoAddColorSettings);


		if (TaskResultInfo.GetAssociatedPaintComponent() == this) {

			if (MinimumDurationOfRoundOfTasks > 0)
				TotalDurationRoundOfTasksTook += TaskResultInfo.TaskDuration;
		}


		ShouldStartNewRoundOfTasks();
		return;
	}


	if (autoAddColorSettings.OnlyStartNewTaskIfChangeWasMadeByOwningPaintComponent && TaskResultInfo.GetAssociatedPaintComponent() == this && !PaintTaskResultInfo.AnyVertexColorGotChanged) {

		RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s - Finished Paint Task for Actor: %s and Mesh: %s didn't do any difference on vertex colors, and the auto paint component is set to only start new tasks if change is made!", *GetName(), *meshComponent->GetOwner()->GetName(), *meshComponent->GetName());

		return;
	}


	if (IsValid(meshComponent) && IsValid(meshComponent->GetOwner())) {

		RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s - Checks if Should Start New Task for Actor: %s and Mesh Component: %s!", *GetName(), *meshComponent->GetOwner()->GetName(), *meshComponent->GetName());
	}


	ShouldStartNewTaskOnComponent(meshComponent, TaskResultInfo, PaintTaskResultInfo);
}


//-------------------------------------------------------

// Should Start New Task On Component

bool UAutoAddColorComponent::ShouldStartNewTaskOnComponent(UPrimitiveComponent* MeshComponent, const FRVPDPTaskResults& TaskResultInfo, const FRVPDPPaintTaskResultInfo& PaintTaskResultInfo, bool WasTriggeredFromNewRound) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_ShouldStartNewTaskOnComponent);
	RVPDP_CPUPROFILER_STR("Auto Add Component - ShouldStartNewTaskOnComponent");

	if (!MeshComponent || MeshComponent->IsBeingDestroyed()) {

		VerifyAutoPaintingMeshComponents();
		ShouldStartNewRoundOfTasks();
		return false;
	}

	if (IsMeshBeingPaintedOrIsQueuedUp(MeshComponent)) return false;


	FRVPDPAutoAddColorSettings autoAddColorSettings = AutoPaintingMeshes.FindRef(MeshComponent);
	bool isFullyPainted = false;
	bool isCompletelyEmpty = false;


	if (CanStartNewTaskBasedOnTaskResult(TaskResultInfo, PaintTaskResultInfo, isFullyPainted, isCompletelyEmpty)) {

		float delayBetweenTask = autoAddColorSettings.DelayBetweenTasks;

		// If this is the component that finished the task, then always use a 1 frame delay to allow for other auto paint components to queue up something as well, for instance if this is a auto paint within area for water, and there is another for entire mesh for drying. 
		if (ApplyOneFrameDelayBetweenTasksForInstigatingComponent && TaskResultInfo.GetAssociatedPaintComponent() == this && delayBetweenTask <= 0) {

			delayBetweenTask = 0.01f;

			// If a New Round Trigger after a delay, then we do not need to enforce the 1 frame delay if it already took longer for the new round to start. 
			if (WasTriggeredFromNewRound) {


				float delayBetweenRoundsOfTasks = TotalDurationRoundOfTasksTook;

				// If there is a set minimum duration that a round of tasks has to take, but finished faster, then we get what the delay between rounds should've been here. 
				if (MinimumDurationOfRoundOfTasks > 0 && TotalDurationRoundOfTasksTook < MinimumDurationOfRoundOfTasks)
					delayBetweenRoundsOfTasks = MinimumDurationOfRoundOfTasks - TotalDurationRoundOfTasksTook;


				if (delayBetweenRoundsOfTasks > 0)
					delayBetweenTask = 0;
			}
		}


		if (delayBetweenTask > 0) {


			RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s - Will Start another Task on Actor: %s and Mesh: %s after %f Delay", *GetName(), *MeshComponent->GetOwner()->GetName(), *MeshComponent->GetName(), delayBetweenTask);


			AutoPaintingMeshes.Add(MeshComponent, autoAddColorSettings);

			FRVPDPAutoAddColorDelaySettings delaySettings;
			delaySettings.DelayBetweenTasks = delayBetweenTask;
			delaySettings.DelayBetweenTasksCounter = 0;
			AutoPaintingMeshesQueuedUpWithDelay.Add(MeshComponent, delaySettings);

			SetComponentTickEnabled(true);
		}

		else {

			RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s - Starts Another Task for Actor: %s and Mesh: %s", *GetName(), *MeshComponent->GetOwner()->GetName(), *MeshComponent->GetName());

			StartAutoPaintTask(MeshComponent);
		}

		return true;
	}

	else {


		if (isFullyPainted) {

			RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s - Shouldn't start another Task for Actor: %s and Mesh %s as it is Fully Painted with what the user is trying to Add.", *GetName(), *MeshComponent->GetOwner()->GetName(), *MeshComponent->GetName());
		}

		else if (isCompletelyEmpty) {

			RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s - Shouldn't start another Task for Actor: %s and Mesh %s as it is Completely Empty of what the user is trying to remove.", *GetName(), *MeshComponent->GetOwner()->GetName(), *MeshComponent->GetName());
		}

		else {

			RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s - Shouldn't start another Task for Actor: %s and Mesh %s. ", *GetName(), *MeshComponent->GetOwner()->GetName(), *MeshComponent->GetName());
		}


		if (autoAddColorSettings.RemoveMeshFromAutoPaintingIfFullyPainted && isFullyPainted) {

			RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s - Mesh %s in Actor %s is Fully Painted and is set to Stop being Auto Painted if so, so removes it from the list. ", *GetName(), *MeshComponent->GetName(), *MeshComponent->GetOwner()->GetName());

			StopAutoPaintingMesh(MeshComponent);
		}

		if (autoAddColorSettings.RemoveMeshFromAutoPaintingIfCompletelyEmpty && isCompletelyEmpty) {

			RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s - Mesh %s in Actor %s is Completely Empty and is set to Stop being Auto Painted if so, so removes it from the list. ", *GetName(), *MeshComponent->GetName(), *MeshComponent->GetOwner()->GetName());

			StopAutoPaintingMesh(MeshComponent);
		}
	}


	return false;
}


//-------------------------------------------------------

// Auto Paint Delay Finished

void UAutoAddColorComponent::AutoPaintDelayFinished(UPrimitiveComponent* MeshComponent) {


	if (MeshComponent) {

		if (!IsAutoPaintingEnabled) return;
		if (!AutoPaintingMeshes.Contains(MeshComponent)) return; // If during the delay removed the mesh to be auto painted
		if (AutoPaintingMeshes.FindRef(MeshComponent).IsPaused) return;


		RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s - AutoPaintDelayFinished Finished for Actor %s and Mesh %s, Starts Auto Paint Task", *GetName(), *MeshComponent->GetOwner()->GetName(), *MeshComponent->GetName());

		StartAutoPaintTask(MeshComponent);
	}

	// If not valid mesh anymore after the delay we cleanup any null keys
	else {

		VerifyAutoPaintingMeshComponents();
	}
}


//-------------------------------------------------------

// Should Start New Round Of Tasks

void UAutoAddColorComponent::ShouldStartNewRoundOfTasks() {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_ShouldStartNewRoundOfTasks);
	RVPDP_CPUPROFILER_STR("Auto Add Component - ShouldStartNewRoundOfTasks");

	if (!OnlyStartNewRoundOfTasksIfAllHasBeenFinished) return;
	if (AutoPaintingMeshes.Num() == 0) return;
	if (GetTotalTasksInitiatedByComponent() > 0) return;
	if (IsNewRoundOfTasksGoingToStartAfterDelay()) return;
	if (IsAnyTaskGoingToStartAfterDelay()) return; // If not expecting any more callbacks either from a task being run or one that's going start after a delay


	float delayBetweenRoundsOfTasks = 0;

	// If there is a set minimum duration that a round of tasks has to take, but we finished faster, then delay it so we reach the minimum duration
	if (MinimumDurationOfRoundOfTasks > 0 && TotalDurationRoundOfTasksTook < MinimumDurationOfRoundOfTasks)
		delayBetweenRoundsOfTasks = MinimumDurationOfRoundOfTasks - TotalDurationRoundOfTasksTook;

	// Can have a delay between rounds of tasks as well, so if you for instance have a centralized system that dries meshes in rounds so they dry at the same pace, then you can get more control of the speed of it. 
	if (delayBetweenRoundsOfTasks > 0) {


		RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s - Start New Round of Tasks after a Delay so does so after %f seconds. ", *GetName(), delayBetweenRoundsOfTasks);

		FTimerDelegate timerDelegate;
		timerDelegate.BindUObject(this, &UAutoAddColorComponent::StartNewRoundOfTasks);

		GetWorld()->GetTimerManager().SetTimer(OnlyStartNewRoundOfTasksDelayTimer, timerDelegate, delayBetweenRoundsOfTasks, false);
	}

	else {

		StartNewRoundOfTasks();
	}
}


//-------------------------------------------------------

// Start New Round Of Tasks

void UAutoAddColorComponent::StartNewRoundOfTasks() {

	if (!IsAutoPaintingEnabled) return;

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_StartNewRoundOfTasks);
	RVPDP_CPUPROFILER_STR("Auto Add Component - StartNewRoundOfTasks");

	RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s - Starts New Round Of Tasks. ", *GetName());


	FRVPDPStartNewRoundOfTasksInfo newRoundOfTaskInfo;

	bool shouldVerifyMeshComponents = false;

	TArray<UPrimitiveComponent*> meshComponents;
	AutoPaintingMeshes.GetKeys(meshComponents);

	TArray<FRVPDPAutoAddColorSettings> autoAddColorSettings;
	AutoPaintingMeshes.GenerateValueArray(autoAddColorSettings);

	for (int i = 0; i < meshComponents.Num(); i++) {

		// If mesh not valid after the round delay we can make sure we verifies them
		if (!meshComponents[i] || meshComponents[i]->IsBeingDestroyed()) {

			shouldVerifyMeshComponents = true;
			continue;
		}

		if (autoAddColorSettings[i].IsPaused) continue;
		if (!autoAddColorSettings[i].OnlyStartNewRoundOfTasks_HasFinishedCachedResults) continue;


		FRVPDPAutoPaintTaskResults autoPaintTaskResults = autoAddColorSettings[i].OnlyStartNewRoundOfTasks_FinishedCachedResults;
		autoAddColorSettings[i].OnlyStartNewRoundOfTasks_FinishedCachedResults = FRVPDPAutoPaintTaskResults();
		autoAddColorSettings[i].OnlyStartNewRoundOfTasks_HasFinishedCachedResults = false;
		AutoPaintingMeshes.Add(meshComponents[i], autoAddColorSettings[i]);

		// When StartAutoPaintTask() is run we run Super and a Delegate before the actual Tasks get added, giving anything that binds to that delegate the possibility to update the paint settings. 
		if (ShouldStartNewTaskOnComponent(meshComponents[i], autoPaintTaskResults.TaskResultInfo, autoPaintTaskResults.PaintTaskResultInfo, true)) {

			newRoundOfTaskInfo.NewRoundOfTasks.Add(meshComponents[i], autoPaintTaskResults);
		}
	}

	StartedNewRoundOfTasksDelegate.Broadcast(newRoundOfTaskInfo);


	if (shouldVerifyMeshComponents)
		VerifyAutoPaintingMeshComponents();

	// Resets this at end of the round timer so we can use it when starting the new round to see how long the duration was, if it was 0 then we can skip the 1 frame delay we otherwise do
	TotalDurationRoundOfTasksTook = 0;
}


//-------------------------------------------------------

// Can Start New Task Based On Task Result

bool UAutoAddColorComponent::CanStartNewTaskBasedOnTaskResult(const FRVPDPTaskResults& TaskResultInfo, const FRVPDPPaintTaskResultInfo& PaintTaskResultInfo, bool& IsFullyPainted, bool& IsCompletelyEmpty) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_CanStartNewTaskBasedOnTaskResult);
	RVPDP_CPUPROFILER_STR("Auto Add Component - CanStartNewTaskBasedOnTaskResult");

	IsFullyPainted = false;
	IsCompletelyEmpty = false;

	if (!IsAutoPaintingEnabled)
		return false;

	if (!TaskResultInfo.TaskSuccessfull)
		return false;

	UPrimitiveComponent* meshComponent = TaskResultInfo.GetMeshComponent();
	if (!meshComponent)
		return false;

	if (!AutoPaintingMeshes.Contains(meshComponent))
		return false;

	if (IsMeshBeingPaintedOrIsQueuedUp(meshComponent))
		return false;


	FRVPDPApplyColorSettings applyVertexColorSettings;
	if (!GetApplyVertexColorSettings(meshComponent, applyVertexColorSettings))
		return false;


	FRVPDPAutoAddColorSettings autoAddColorSettings = AutoPaintingMeshes.FindRef(meshComponent);


	if (autoAddColorSettings.AlwaysForceStartNewTasksForTheMesh) {

		RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s - Set to Force Start new Task for Actor: %s and Mesh %s so skips checking if Fully Painted, Empty or if Change was Made. ", *GetName(), *meshComponent->GetOwner()->GetName(), *meshComponent->GetName());

		return true;
	}


	if (autoAddColorSettings.OnlyStartNewTaskIfChangeWasMadeByOwningPaintComponent && TaskResultInfo.GetAssociatedPaintComponent() == this) {

		if (!PaintTaskResultInfo.AnyVertexColorGotChanged) {

			RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s - Shouldn't start another Task for Actor: %s and Mesh %s as its set to only start new tasks if a change is made and none was made on the Mesh. ", *GetName(), *meshComponent->GetOwner()->GetName(), *meshComponent->GetName());

			return false;
		}
	}



	bool shouldStartNewTask = false;

	if (TaskResultInfo.AmountOfPaintedColorsOfEachChannel.SuccessfullyGotColorChannelResultsAtMinAmount) {


		TArray<EVertexColorChannel> paintsOnChannels;

		if (DoesAutoPaintOnMeshPaintUsingVertexColorChannels(paintsOnChannels, applyVertexColorSettings)) {


			FRVPDPAmountOfColorsOfEachChannelChannelResults channelResult;
			FString channelAsString = "";
			int fullOnAmountOfChannels = 0;
			int emptyOnAmountOfChannels = 0;
			FRVPDPApplyColorsUsingVertexChannelSettings applyColorsUsingVertexChannel;
			float shouldStartNewTaskWithStrengthToApply = 0;
			bool isLerpingToTarget = false;

			for (int i = 0; i < paintsOnChannels.Num(); i++) {

				isLerpingToTarget = false;


				switch (paintsOnChannels[i]) {

				case EVertexColorChannel::RedChannel:

					channelAsString = "Red";
					channelResult = TaskResultInfo.AmountOfPaintedColorsOfEachChannel.RedChannelResult;
					applyColorsUsingVertexChannel = applyVertexColorSettings.ApplyColorsOnRedChannel;
					break;

				case EVertexColorChannel::GreenChannel:

					channelAsString = "Green";
					channelResult = TaskResultInfo.AmountOfPaintedColorsOfEachChannel.GreenChannelResult;
					applyColorsUsingVertexChannel = applyVertexColorSettings.ApplyColorsOnGreenChannel;
					break;

				case EVertexColorChannel::BlueChannel:

					channelAsString = "Blue";
					channelResult = TaskResultInfo.AmountOfPaintedColorsOfEachChannel.BlueChannelResult;
					applyColorsUsingVertexChannel = applyVertexColorSettings.ApplyColorsOnBlueChannel;
					break;

				case EVertexColorChannel::AlphaChannel:

					channelAsString = "Alpha";
					channelResult = TaskResultInfo.AmountOfPaintedColorsOfEachChannel.AlphaChannelResult;
					applyColorsUsingVertexChannel = applyVertexColorSettings.ApplyColorsOnAlphaChannel;
					break;

				default:
					break;
				}


				// First Checks if trying to Lerp to a Target, and if we haven't reached the target, only want to do the other checks if not set to lerp to a target
				if (applyColorsUsingVertexChannel.LerpVertexColorToTarget.LerpToTarget) {

					// Lerp to Target requires that any vertex should've gotten changed as well
					if (PaintTaskResultInfo.AnyVertexColorGotChanged) {

						if (UKismetMathLibrary::NearlyEqual_FloatFloat(applyColorsUsingVertexChannel.LerpVertexColorToTarget.TargetValue, channelResult.AverageColorAmountAtMinAmount, LerpToTargetErrorToleranceIfEqual)) {

							// UE_LOG(LogTemp, Warning, TEXT("Channel Lerp To Target Equal"));
						}

						else {

							isLerpingToTarget = true;
							shouldStartNewTaskWithStrengthToApply = applyColorsUsingVertexChannel.AmountToApply;
							shouldStartNewTask = true;
							break;
						}
					}
				}

				// If Adding color for this channel, then checks if fully painting, otherwise keeps adding
				else if (applyColorsUsingVertexChannel.AmountToApply > 0) {

					// Keep adding until over min limit
					if (channelResult.AverageColorAmountAtMinAmount >= ConsiderMeshFullyPaintedIfOverOrEqual) {

						fullOnAmountOfChannels++;
					}

					else {

						shouldStartNewTaskWithStrengthToApply = applyColorsUsingVertexChannel.AmountToApply;
						shouldStartNewTask = true;
						break;
					}
				}

				// If Removing on this channel, then Checks if Empty, if not then keep removing
				else if (applyColorsUsingVertexChannel.AmountToApply < 0) {

					// Keep removing until there is no vertices left
					if (channelResult.AverageColorAmountAtMinAmount > 0) {

						shouldStartNewTaskWithStrengthToApply = applyColorsUsingVertexChannel.AmountToApply;
						shouldStartNewTask = true;

						break;
					}
					else {

						emptyOnAmountOfChannels++;
					}
				}
			}


			if (shouldStartNewTask) {

				RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s - Average Amount at Min: %f  -  Should Start New Task on Actor: %s and Mesh: %s. Trying to Add on Channel: %s with Strength: %f. Is Set to Lerp to Target: %i", *GetName(), channelResult.AverageColorAmountAtMinAmount, *meshComponent->GetOwner()->GetName(), *meshComponent->GetName(), *channelAsString, shouldStartNewTaskWithStrengthToApply, isLerpingToTarget);

				return true;
			}


			// If fully painted or emptied on all channels and that's why we're not starting any new ones. 
			if (paintsOnChannels.Num() == fullOnAmountOfChannels)
				IsFullyPainted = true;

			if (paintsOnChannels.Num() == emptyOnAmountOfChannels)
				IsCompletelyEmpty = true;


			RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s - Successfully Got Colors of Each Channel Result, and all channels are either Fully Painted if Adding, or Completely Empty if Removing, so shouldn't start another Task for Actor: %s and Mesh %s. ", *GetName(), *meshComponent->GetOwner()->GetName(), *meshComponent->GetName());

			return false;
		}
	}

	else {

		RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s - CanStartNewTaskBasedOnTaskResult - Amount of Colors of Each Channel was unavailable, so can't check if should start new task using RGBA. If the Task was instigated by something else, like the Player, make sure that paint jobs Callback Settings Amount of Colors of Each Channel Settings are correct! ", *GetName());
	}


	if (TaskResultInfo.AmountOfPaintedColorsOfEachChannel.SuccessfullyGotPhysicsSurfaceResultsAtMinAmount) {

		TArray<FRVPDPPhysicsSurfaceToApplySettings> physicsSurfacesToApplySettings;
		if (DoesAutoPaintOnMeshPaintUsingPhysicsSurface(physicsSurfacesToApplySettings, applyVertexColorSettings)) {

			int fullOnAmountOfPhysicsSurfaces = 0;
			int emptyOnAmountOfPhysicsSurfaces = 0;
			TEnumAsByte<EPhysicalSurface> shouldStartNewTaskOnPhysicsSurface;
			bool isLerpingToTarget = false;


			for (const FRVPDPAmountOfColorsOfEachChannelPhysicsResults& physicsSurfaceResults : TaskResultInfo.AmountOfPaintedColorsOfEachChannel.PhysicsSurfacesResults) {

				isLerpingToTarget = false;
				

				TArray<TEnumAsByte<EPhysicalSurface>> parentPhysicsSurfaces = UVertexPaintFunctionLibrary::GetParentsOfPhysicsSurface_Wrapper(this, physicsSurfaceResults.PhysicsSurface);


				// If has Physics Parents, then checks if the applied surface is the parent. For instance if applying Wet, on a material that may not have Wet, but a child like Cobble-Wet, so the physicsSurfacesResult doesn't have Wet but Cobble-Wet, it can still check the parent if that's a match. 
				if (parentPhysicsSurfaces.Num() > 0) {

					for (TEnumAsByte<EPhysicalSurface> physicsSurfaceResultParent : parentPhysicsSurfaces) {

						for (const FRVPDPPhysicsSurfaceToApplySettings& physicalSurfaceToApplySettings : physicsSurfacesToApplySettings) {

							if (physicalSurfaceToApplySettings.SurfaceToApply == physicsSurfaceResultParent) {


								// First Checks if trying to Lerp to a Target, and if we haven't reached the target, only want to do the other checks if not set to lerp to a target
								if (physicalSurfaceToApplySettings.LerpPhysicsSurfaceToTarget.LerpToTarget) {


									// Lerp to Target requires that any vertex should've gotten changed as well
									if (PaintTaskResultInfo.AnyVertexColorGotChanged) {

										if (UKismetMathLibrary::NearlyEqual_FloatFloat(physicalSurfaceToApplySettings.LerpPhysicsSurfaceToTarget.TargetValue, physicsSurfaceResults.AverageColorAmountAtMinAmount, LerpToTargetErrorToleranceIfEqual)) {

											// UE_LOG(LogTemp, Warning, TEXT("Phys Surface Lerp To Target Equal"));
										}

										else {

											isLerpingToTarget = true;
											shouldStartNewTaskOnPhysicsSurface = physicalSurfaceToApplySettings.SurfaceToApply;
											shouldStartNewTask = true;
											break;
										}
									}
								}

								// If Adding colors, then checks if over min, otherwise keeps adding
								else if (physicalSurfaceToApplySettings.StrengthToApply > 0) {

									if (physicsSurfaceResults.AverageColorAmountAtMinAmount >= ConsiderMeshFullyPaintedIfOverOrEqual) {

										fullOnAmountOfPhysicsSurfaces++;
									}

									else {

										shouldStartNewTaskOnPhysicsSurface = physicsSurfaceResults.PhysicsSurface;
										shouldStartNewTask = true;
										break;
									}
								}

								// If Removing, then Checks if Empty, if not then keep removing
								else if (physicalSurfaceToApplySettings.StrengthToApply < 0) {

									if (physicsSurfaceResults.AverageColorAmountAtMinAmount > 0) {

										shouldStartNewTaskOnPhysicsSurface = physicsSurfaceResults.PhysicsSurface;
										shouldStartNewTask = true;
										break;
									}
									else {

										emptyOnAmountOfPhysicsSurfaces++;
									}
								}
							}
						}

						if (shouldStartNewTask)
							break;
					}
				}

				// If no physics parents then checks the applied physics surface if it was a part of the result. 
				else {

					for (const FRVPDPPhysicsSurfaceToApplySettings& physicalSurfaceToApplySettings : physicsSurfacesToApplySettings) {

						if (physicsSurfaceResults.PhysicsSurface == physicalSurfaceToApplySettings.SurfaceToApply) {


							// First Checks if trying to Lerp to a Target, and if we haven't reached the target, only want to do the other checks if not set to lerp to a target
							if (physicalSurfaceToApplySettings.LerpPhysicsSurfaceToTarget.LerpToTarget) {

								// Lerp to Target requires that any vertex should've gotten changed as well
								if (PaintTaskResultInfo.AnyVertexColorGotChanged) {

									if (UKismetMathLibrary::NearlyEqual_FloatFloat(physicalSurfaceToApplySettings.LerpPhysicsSurfaceToTarget.TargetValue, physicsSurfaceResults.AverageColorAmountAtMinAmount, LerpToTargetErrorToleranceIfEqual)) {

										// UE_LOG(LogTemp, Warning, TEXT("Phys Surface Lerp To Target Equal"));
									}

									else {

										isLerpingToTarget = true;
										shouldStartNewTaskOnPhysicsSurface = physicalSurfaceToApplySettings.SurfaceToApply;
										shouldStartNewTask = true;
										break;
									}
								}
							}

							// If Adding colors, then checks if over min, otherwise keeps adding
							else if (physicalSurfaceToApplySettings.StrengthToApply > 0) {

								if (physicsSurfaceResults.AverageColorAmountAtMinAmount >= ConsiderMeshFullyPaintedIfOverOrEqual) {

									fullOnAmountOfPhysicsSurfaces++;
								}

								else {

									shouldStartNewTaskOnPhysicsSurface = physicalSurfaceToApplySettings.SurfaceToApply;
									shouldStartNewTask = true;
									break;
								}
							}

							// If Removing, then Checks if Empty, if not then keep removing
							else if (physicalSurfaceToApplySettings.StrengthToApply < 0) {

								if (physicsSurfaceResults.AverageColorAmountAtMinAmount > 0) {

									shouldStartNewTaskOnPhysicsSurface = physicalSurfaceToApplySettings.SurfaceToApply;
									shouldStartNewTask = true;
									break;
								}
								else {

									emptyOnAmountOfPhysicsSurfaces++;
								}
							}
						}
					}
				}


				if (shouldStartNewTask) {

					const FString physicsSurfaceAsString = *StaticEnum<EPhysicalSurface>()->GetDisplayNameTextByValue(shouldStartNewTaskOnPhysicsSurface.GetValue()).ToString();

					RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s - Physics Surface Result Average Amount: %f  -  Should Start New Task on Actor: %s and Mesh: %s with Phyiscs Surface: %s. Is Set to Lerp to Target: %i", *GetName(), physicsSurfaceResults.AverageColorAmountAtMinAmount, *meshComponent->GetOwner()->GetName(), *meshComponent->GetName(), *physicsSurfaceAsString, isLerpingToTarget);

					return true;
				}
			}


			// If fully painted or emptied on all surfaces and that's why we're not starting any new ones. 
			if (physicsSurfacesToApplySettings.Num() == fullOnAmountOfPhysicsSurfaces)
				IsFullyPainted = true;

			if (physicsSurfacesToApplySettings.Num() == emptyOnAmountOfPhysicsSurfaces)
				IsCompletelyEmpty = true;


			RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s - Successfully Got Physics Surface Result, but the surface we're applying on is either Fully Painted if Adding, or Completely Empty if Removing. So shouldn't start another Task for Actor: %s and Mesh %s. ", *GetName(), *meshComponent->GetOwner()->GetName(), *meshComponent->GetName());

			return false;
		}
	}

	else {

		RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s - CanStartNewTaskBasedOnTaskResult - Amount of Colors of Each Channel for Physics Surfaces was unavailable, so can't check if should start new task using physics surfaces amounts on each channel. If the Task was instigated by something else, like the Player, make sure that paint jobs Callback Settings Amount of Colors of Each Channel Settings are correct! ", *GetName());
	}


	return false;
}


//-------------------------------------------------------

// Start Auto Paint Task

void UAutoAddColorComponent::StartAutoPaintTask(UPrimitiveComponent* MeshComponent) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_StartAutoPaintTask);
	RVPDP_CPUPROFILER_STR("Auto Add Component - StartAutoPaintTask");


	if (!MeshComponent || MeshComponent->IsBeingDestroyed()) {

		VerifyAutoPaintingMeshComponents();
		return;
	}

	// Should be Overriden by children with super and with their specific task they want to start. 

	StartedAutoPaintingMeshDelegate.Broadcast(MeshComponent);
}


//-------------------------------------------------------

// Start New Paint Task If Allowed

void UAutoAddColorComponent::StartNewPaintTaskIfAllowed(UPrimitiveComponent* MeshComponent, const FRVPDPAutoAddColorSettings& AutoAddColorSettings) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_StartNewPaintTaskIfAllowed);
	RVPDP_CPUPROFILER_STR("Auto Add Within Area Component - StartNewPaintTaskIfAllowed");

	if (!IsValid(MeshComponent)) return;


	if (!AutoAddColorSettings.IsPaused && !IsMeshBeingPaintedOrIsQueuedUp(MeshComponent)) {

		// If new round will start, but doesn't have cached result then we still start the task, since otherwise it won't be part of the round at all and will never get painted. 
		if (IsNewRoundOfTasksGoingToStartAfterDelay()) {

			if (!AutoAddColorSettings.OnlyStartNewRoundOfTasks_HasFinishedCachedResults)
				StartAutoPaintTask(MeshComponent);
		}

		else {

			StartAutoPaintTask(MeshComponent);
		}
	}
}


//-------------------------------------------------------

// Get Apply Vertex Color Settings

bool UAutoAddColorComponent::GetApplyVertexColorSettings(UPrimitiveComponent* MeshComponent, FRVPDPApplyColorSettings& ApplyVertexColorSettings) {

	// Should be overriden by children

	return false;
}


//-------------------------------------------------------

// Does Auto Paint On Mesh Paint Using Vertex Color Channels

bool UAutoAddColorComponent::DoesAutoPaintOnMeshPaintUsingVertexColorChannels(TArray<EVertexColorChannel>& PaintsOnChannels, const FRVPDPApplyColorSettings& ApplyVertexColorsSettings) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_DoesAutoPaintOnMeshPaintUsingVertexColorChannels);
	RVPDP_CPUPROFILER_STR("Auto Add Component - DoesAutoPaintOnMeshPaintUsingVertexColorChannels");


	// If painting with physics surface then we shouldn't check channels
	if (ApplyVertexColorsSettings.ApplyColorsUsingPhysicsSurface.ApplyVertexColorUsingPhysicsSurface) return false;


	if (ApplyVertexColorsSettings.ApplyColorsOnRedChannel.AmountToApply != 0 || ApplyVertexColorsSettings.ApplyColorsOnRedChannel.LerpVertexColorToTarget.LerpToTarget) {

		PaintsOnChannels.Add(EVertexColorChannel::RedChannel);
	}

	if (ApplyVertexColorsSettings.ApplyColorsOnGreenChannel.AmountToApply != 0 || ApplyVertexColorsSettings.ApplyColorsOnGreenChannel.LerpVertexColorToTarget.LerpToTarget) {

		PaintsOnChannels.Add(EVertexColorChannel::GreenChannel);
	}

	if (ApplyVertexColorsSettings.ApplyColorsOnBlueChannel.AmountToApply != 0 || ApplyVertexColorsSettings.ApplyColorsOnBlueChannel.LerpVertexColorToTarget.LerpToTarget) {

		PaintsOnChannels.Add(EVertexColorChannel::BlueChannel);
	}

	if (ApplyVertexColorsSettings.ApplyColorsOnAlphaChannel.AmountToApply != 0 || ApplyVertexColorsSettings.ApplyColorsOnAlphaChannel.LerpVertexColorToTarget.LerpToTarget) {

		PaintsOnChannels.Add(EVertexColorChannel::AlphaChannel);
	}

	return true;
}


//-------------------------------------------------------

// Does Auto Paint On Mesh Paint Using Physics Surface

bool UAutoAddColorComponent::DoesAutoPaintOnMeshPaintUsingPhysicsSurface(TArray<FRVPDPPhysicsSurfaceToApplySettings>& PhysicsSurfacesToApply, const FRVPDPApplyColorSettings& ApplyVertexColorsSettings) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_DoesAutoPaintOnMeshPaintUsingPhysicsSurface);
	RVPDP_CPUPROFILER_STR("Auto Add Component - DoesAutoPaintOnMeshPaintUsingPhysicsSurface");


	if (!ApplyVertexColorsSettings.ApplyColorsUsingPhysicsSurface.ApplyVertexColorUsingPhysicsSurface) return false;

	PhysicsSurfacesToApply = ApplyVertexColorsSettings.ApplyColorsUsingPhysicsSurface.PhysicalSurfacesToApply;

	return true;
}


//-------------------------------------------------------

// Adjust Callback Settings

void UAutoAddColorComponent::AdjustCallbackSettings(const FRVPDPApplyColorSettings& ApplyVertexColorSettings, FRVPDPTaskCallbackSettings& CallbackSettings) {


	CallbackSettings.IncludeAmountOfPaintedColorsOfEachChannel.IncludeIfMinColorAmountIs = FRVPDPIncludeAmountOfPaintedColorsOfEachChannelSettings().IncludeIfMinColorAmountIs;

	// Makes sure we Include what we need from the Callback settings so when PaintTaskFinished runs for a job we've instigated here, we can properly check if we should start another task. 
	if (ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.ApplyVertexColorUsingPhysicsSurface) {

		CallbackSettings.IncludeAmountOfPaintedColorsOfEachChannel.IncludePhysicsSurfaceResultOfEachChannel = true;
	}

	else {

		CallbackSettings.IncludeAmountOfPaintedColorsOfEachChannel.IncludeVertexColorChannelResultOfEachChannel = true;
	}
}


//-------------------------------------------------------

// Auto Painted Actor Destroyed

void UAutoAddColorComponent::AutoPaintedActorDestroyed(AActor* DestroyedActor) {


	for (const TPair<UPrimitiveComponent*, FRVPDPAutoAddColorSettings>& autoPaintingMesh : AutoPaintingMeshes) {

		if (autoPaintingMesh.Key && autoPaintingMesh.Key->GetOwner() == DestroyedActor) {

			RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s  -  Stops Auto Painting Component: %s related to Actor: %s because the Actor was Destroyed!", *GetName(), *autoPaintingMesh.Key->GetName(), *DestroyedActor->GetName());

			StopAutoPaintingMesh(autoPaintingMesh.Key);
			return;
		}
	}
}


//-------------------------------------------------------

// Stop All Auto Painting

void UAutoAddColorComponent::StopAllAutoPainting() {


	RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s  -  Set to Stop All Auto Painting. ", *GetName());

	TArray<UPrimitiveComponent*> meshComponents;
	AutoPaintingMeshes.GetKeys(meshComponents);

	for (UPrimitiveComponent* meshComponent : meshComponents)
		StopAutoPaintingMesh(meshComponent);

	AutoPaintingMeshes.Empty();
}


//-------------------------------------------------------

// Stop Auto Painting Mesh

void UAutoAddColorComponent::StopAutoPaintingMesh(UPrimitiveComponent* MeshComponent) {

	if (!MeshComponent) return;
	if (MeshComponent->IsBeingDestroyed()) return;
	if (!IsValid(MeshComponent)) return;
	if (!AutoPaintingMeshes.Contains(MeshComponent)) return;


	RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s  -  Stops Auto Painting on Actor: %s and Mesh: %s", *GetName(), *MeshComponent->GetOwner()->GetName(), *MeshComponent->GetName());


	AutoPaintingMeshes.Remove(MeshComponent);
	AutoPaintingMeshesQueuedUpWithDelay.Remove(MeshComponent);

	SetComponentTickEnabled(ShouldAlwaysTick());


	if (AutoPaintingMeshes.Num() > 0) {

		
		if (IsValid(MeshComponent) && IsValid(MeshComponent->GetOwner())) {


			if (MeshComponent->GetOwner()->OnDestroyed.IsAlreadyBound(this, &UAutoAddColorComponent::AutoPaintedActorDestroyed)) {

				MeshComponent->GetOwner()->OnDestroyed.RemoveDynamic(this, &UAutoAddColorComponent::AutoPaintedActorDestroyed);
			}
		}
	}

	else {

		IsAutoPaintingEnabled = false;

		if (UVertexPaintFunctionLibrary::IsWorldValid(GetWorld())) {

			if (GetWorld()->GetTimerManager().IsTimerActive(OnlyStartNewRoundOfTasksDelayTimer))
				GetWorld()->GetTimerManager().ClearTimer(OnlyStartNewRoundOfTasksDelayTimer);
		}
	}
}


//-------------------------------------------------------

// Pause All Auto Painting

void UAutoAddColorComponent::PauseAllAutoPainting() {


	RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s  -  Set to Pause All Auto Painting. ", *GetName());


	for (const TPair<UPrimitiveComponent*, FRVPDPAutoAddColorSettings>& autoPainting : AutoPaintingMeshes)
		PauseAutoPaintingMesh(autoPainting.Key);
}


//-------------------------------------------------------

// Can Auto Painted Mesh Get Paused

bool UAutoAddColorComponent::CanAutoPaintedMeshGetPaused(const UPrimitiveComponent* MeshComponent) const {

	if (!MeshComponent) return false;
	if (!AutoPaintingMeshes.Contains(MeshComponent)) return false;

	return AutoPaintingMeshes.FindRef(MeshComponent).CanEverGetPaused;
}


//-------------------------------------------------------

// Pause Auto Painting Mesh

void UAutoAddColorComponent::PauseAutoPaintingMesh(UPrimitiveComponent* MeshComponent) {

	if (!MeshComponent) return;
	if (!AutoPaintingMeshes.Contains(MeshComponent)) return;


	FRVPDPAutoAddColorSettings autoAddColorSettings = AutoPaintingMeshes.FindRef(MeshComponent);

	if (!autoAddColorSettings.CanEverGetPaused) return;
	if (autoAddColorSettings.IsPaused) return;

	RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s  -  Set to Pause Auto Painting on Actor: %s and Mesh: %s", *GetName(), *MeshComponent->GetOwner()->GetName(), *MeshComponent->GetName());

	autoAddColorSettings.IsPaused = true;
	autoAddColorSettings.OnlyStartNewRoundOfTasks_HasFinishedCachedResults = false;
	AutoPaintingMeshes.Add(MeshComponent, autoAddColorSettings);
}


//-------------------------------------------------------

// Is Auto Painted Mesh Paused

bool UAutoAddColorComponent::IsAutoPaintedMeshPaused(const UPrimitiveComponent* MeshComponent) const {

	FRVPDPAutoAddColorSettings autoAddColorSettings = AutoPaintingMeshes.FindRef(MeshComponent);

	if (autoAddColorSettings.IsPaused) return true;

	return false;
}


//-------------------------------------------------------

// Resume All Auto Painting

void UAutoAddColorComponent::ResumeAllAutoPainting() {


	RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s  -  Set to Pause All Auto Painting. ", *GetName());


	for (const TPair<UPrimitiveComponent*, FRVPDPAutoAddColorSettings>& autoPainting : AutoPaintingMeshes)
		ResumeAutoPaintingMesh(autoPainting.Key);
}


//-------------------------------------------------------

// Resume Auto Painting Mesh

void UAutoAddColorComponent::ResumeAutoPaintingMesh(UPrimitiveComponent* MeshComponent) {

	if (!MeshComponent) return;
	if (!AutoPaintingMeshes.Contains(MeshComponent)) return;

	FRVPDPAutoAddColorSettings autoAddColorSettings = AutoPaintingMeshes.FindRef(MeshComponent);

	if (!autoAddColorSettings.IsPaused) return;

	autoAddColorSettings.IsPaused = false;
	AutoPaintingMeshes.Add(MeshComponent, autoAddColorSettings);

	UVertexPaintFunctionLibrary::RegisterPaintTaskCallbacksToObjectFromSpecifiedMeshComponent_Wrapper(MeshComponent, this);

	RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s  -  Set to Resume Auto Painting on Actor %s and Mesh: %s", *GetName(), *MeshComponent->GetOwner()->GetName(), *MeshComponent->GetName());

	StartAutoPaintTask(MeshComponent);
}


//-------------------------------------------------------

// Is Any Task Going To Start After Delay

bool UAutoAddColorComponent::IsAnyTaskGoingToStartAfterDelay() {

	if (AutoPaintingMeshesQueuedUpWithDelay.Num() > 0) return true;

	return false;
}


//-------------------------------------------------------

// Verify Auto Painting Mesh Components

void UAutoAddColorComponent::VerifyAutoPaintingMeshComponents() {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_VerifyAutoPaintingMeshComponents);
	RVPDP_CPUPROFILER_STR("Auto Add Component - VerifyAutoPaintingMeshComponents");


	RVPDP_LOG(DebugSettings, FColor::Cyan, "Auto Paint Component: %s - Verify All Cached Mesh Components.", *GetName());

	if (AutoPaintingMeshes.Num() > 0) {


		TArray<UPrimitiveComponent*> meshComponents;
		AutoPaintingMeshes.GetKeys(meshComponents);

		TArray<FRVPDPAutoAddColorSettings> autoAddColorSettings;
		AutoPaintingMeshes.GenerateValueArray(autoAddColorSettings);
		AutoPaintingMeshes.Empty();


		TArray<FRVPDPAutoAddColorDelaySettings> delaySettings;


		for (int i = 0; i < meshComponents.Num(); i++) {

			if (IsValid(meshComponents[i])) {

				AutoPaintingMeshes.Add(meshComponents[i], autoAddColorSettings[i]);
			}
		}

		if (AutoPaintingMeshes.Num() == 0)
			IsAutoPaintingEnabled = false;


		meshComponents.Empty();
		AutoPaintingMeshesQueuedUpWithDelay.GetKeys(meshComponents);
		AutoPaintingMeshesQueuedUpWithDelay.GenerateValueArray(delaySettings);
		AutoPaintingMeshesQueuedUpWithDelay.Empty();

		for (int i = 0; i < meshComponents.Num(); i++) {

			if (IsValid(meshComponents[i])) {

				AutoPaintingMeshesQueuedUpWithDelay.Add(meshComponents[i], delaySettings[i]);
			}
		}

		SetComponentTickEnabled(ShouldAlwaysTick());
	}

	VerifiedAutoPaintingMeshesDelegate.Broadcast();
}


//-------------------------------------------------------

// Is New Round Of Tasks Going To Start After Delay

bool UAutoAddColorComponent::IsNewRoundOfTasksGoingToStartAfterDelay() const {

	// Checks time left as well since it can return active on the same frame as it finishes and the time remaining is 0
	if ((GetWorld()->GetTimerManager().IsTimerActive(OnlyStartNewRoundOfTasksDelayTimer) && GetWorld()->GetTimerManager().GetTimerRemaining(OnlyStartNewRoundOfTasksDelayTimer) > 0))
		return true;

	return false;
}


//-------------------------------------------------------

// Set If Auto Painted Mesh Should Only Start New Task If Change Was Made

void UAutoAddColorComponent::SetIfAutoPaintedMeshShouldOnlyStartNewTaskIfChangeWasMade(UPrimitiveComponent* MeshComponent, bool OnlyStartNewTaskIfChangeWasMade) {

	if (!MeshComponent) return;
	if (!AutoPaintingMeshes.Contains(MeshComponent)) return;


	FRVPDPAutoAddColorSettings autoColorSettings = AutoPaintingMeshes.FindRef(MeshComponent);
	autoColorSettings.OnlyStartNewTaskIfChangeWasMadeByOwningPaintComponent = OnlyStartNewTaskIfChangeWasMade;

	AutoPaintingMeshes.Add(MeshComponent, autoColorSettings);
}


//-------------------------------------------------------

// Get If Mesh Should Only Start New Task If Change Was Made

bool UAutoAddColorComponent::GetIfMeshShouldOnlyStartNewTaskIfChangeWasMade(const UPrimitiveComponent* MeshComponent) const {

	if (!MeshComponent) return false;
	if (!AutoPaintingMeshes.Contains(MeshComponent)) return false;

	return AutoPaintingMeshes.FindRef(MeshComponent).OnlyStartNewTaskIfChangeWasMadeByOwningPaintComponent;
}