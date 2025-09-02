// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "VertexPaintFunctionLibrary.h"
#include "Async/Async.h"
#include "TaskResultsPrerequisites.h"
#include "Async/AsyncWork.h"


struct FRVPDPColorsOfEachChannelAsyncTask {


	DECLARE_DELEGATE_OneParam(FOnTaskComplete, const FRVPDPAmountOfColorsOfEachChannelResults&);
	FOnTaskComplete OnColorsOfEachChannelAsyncTaskCompleteDelegate;


	TStatId GetStatId() const {

		RETURN_QUICK_DECLARE_CYCLE_STAT(FRVPDPColorsOfEachChannelAsyncTask, STATGROUP_ThreadPoolAsyncTasks);
	}


	bool CanAbandon() {

		return true;
	}

	void Abandon() {

		// Cleanup or finalize any remaining logic if the task is abandoned
	}


	TArray<FColor> VertexColorsToGetAmountOfForEachChannel;
	float MinColorAmountToBeConsideredToUse = 0;

	FRVPDPColorsOfEachChannelAsyncTask(const TArray<FColor>& VertexColors, float MinColorAmountToBeConsidered) {

		VertexColorsToGetAmountOfForEachChannel = VertexColors;
		MinColorAmountToBeConsideredToUse = MinColorAmountToBeConsidered;
	}


	void DoWork() {

		const FRVPDPAmountOfColorsOfEachChannelResults amountOfPaintedColorsOfEachChannelResult = UVertexPaintFunctionLibrary::GetAmountOfPaintedColorsForEachChannel(VertexColorsToGetAmountOfForEachChannel, MinColorAmountToBeConsideredToUse);


		if (IsInGameThread()) {

			OnColorsOfEachChannelAsyncTaskCompleteDelegate.ExecuteIfBound(amountOfPaintedColorsOfEachChannelResult);
		}

		else {

			FOnTaskComplete onTaskCompleteDelegate = OnColorsOfEachChannelAsyncTaskCompleteDelegate;

			// Broadcasts on Game Thread so the actual Applying of colors happens there
			AsyncTask(ENamedThreads::GameThread, [this, onTaskCompleteDelegate, amountOfPaintedColorsOfEachChannelResult]() mutable {

				onTaskCompleteDelegate.ExecuteIfBound(amountOfPaintedColorsOfEachChannelResult);
			});
		}
	}
};