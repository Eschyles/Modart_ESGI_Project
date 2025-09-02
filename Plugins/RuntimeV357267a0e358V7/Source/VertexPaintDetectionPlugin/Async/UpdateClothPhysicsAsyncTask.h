// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "VertexPaintFunctionLibrary.h"
#include "Async/Async.h"
#include "Async/AsyncWork.h"


struct FRVPDPUpdateClothPhysicsAsyncTask {

	DECLARE_DELEGATE_OneParam(FOnTaskComplete, const TArray<FRVPDPChaosClothPhysicsSettings>&);
	FOnTaskComplete OnUpdateClothPhysicsAsyncTaskCompleteDelegate;


	TStatId GetStatId() const {

		RETURN_QUICK_DECLARE_CYCLE_STAT(FRVPDPUpdateClothPhysicsAsyncTask, STATGROUP_ThreadPoolAsyncTasks);
	}

	bool CanAbandon() {

		return true;
	}

	void Abandon() {

		// Cleanup or finalize any remaining logic if the task is abandoned
	}


	UPROPERTY()
	USkeletalMeshComponent* SkelMeshComponent = nullptr;

	TArray<FRVPDPChaosClothPhysicsSettings> ChaosClothPhysicsSettings;


	FRVPDPUpdateClothPhysicsAsyncTask(USkeletalMeshComponent* SkeletalMeshComponent) {

		SkelMeshComponent = SkeletalMeshComponent;
	}


	void DoWork() {


#if ENGINE_MAJOR_VERSION >= 5

		if (SkelMeshComponent) {

			TMap<UClothingAssetBase*, FRVPDPChaosClothPhysicsSettings> clothPhysicsSettingsResult = UVertexPaintFunctionLibrary::GetChaosClothPhysicsSettingsBasedOnExistingColors(SkelMeshComponent);

			clothPhysicsSettingsResult.GenerateValueArray(ChaosClothPhysicsSettings);
		}

#endif


		if (IsInGameThread()) {

			OnUpdateClothPhysicsAsyncTaskCompleteDelegate.ExecuteIfBound(ChaosClothPhysicsSettings);
		}

		else {

			FOnTaskComplete onUpdateClothPhysicsAsyncTaskComplete = OnUpdateClothPhysicsAsyncTaskCompleteDelegate;
			TArray<FRVPDPChaosClothPhysicsSettings> clothPhysicsSettings = ChaosClothPhysicsSettings;

			// Broadcasts on Game Thread so the actual Applying of colors happens there
			AsyncTask(ENamedThreads::GameThread, [this, onUpdateClothPhysicsAsyncTaskComplete, clothPhysicsSettings]() mutable {

				onUpdateClothPhysicsAsyncTaskComplete.ExecuteIfBound(clothPhysicsSettings);
			});
		}
	}
};