// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 


#include "VertexPaintCalculateColorsTask.h"
#include "Async/Async.h"

// Engine
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"
#include "Engine/GameInstance.h"
#include "Rendering/ColorVertexBuffer.h"
#include "Animation/Skeleton.h"
#include "Engine/SkeletalMesh.h"
#include "Components/SkeletalMeshComponent.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "ClothingAsset.h"
#include "ClothingAssetBase.h"
#include "Math/UnrealMathUtility.h"
#include "Math/Matrix.h"
#include "Math/Transform.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Landscape.h"

// Plugin
#include "RVPDPRapidJsonFunctionLibrary.h" 
#include "VertexPaintFunctionLibrary.h"
#include "VertexPaintDetectionInterface.h"
#include "VertexPaintColorSnippetDataAsset.h"
#include "VertexPaintMaterialDataAsset.h"
#include "VertexPaintDetectionLog.h"
#include "VertexPaintOptimizationDataAsset.h"
#include "VertexPaintDetectionProfiling.h"

// UE5
#if ENGINE_MAJOR_VERSION == 5

#include "GeometryCollection/GeometryCollectionComponent.h"
#include "GeometryCollection/GeometryCollection.h"
#include "GeometryCollection/GeometryCollectionObject.h"

#if ENGINE_MINOR_VERSION >= 2
#include "BoneWeights.h"
#endif

#endif


//-------------------------------------------------------

// Set Calculate Colors Info

void FVertexPaintCalculateColorsTask::SetCalculateColorsInfo(const FRVPDPCalculateColorsInfo& CalculateColorsInfo) {

	CalculateColorsInfoGlobal = CalculateColorsInfo;
	CalculateColorsInfoGlobal.TaskResult.MeshVertexData = CalculateColorsInfo.InitialMeshVertexData;
}


//-------------------------------------------------------

// Do Work

void FVertexPaintCalculateColorsTask::DoWork() {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_DoWork);
	RVPDP_CPUPROFILER_STR("Calculate Colors Task - DoWork");

	InGameThread = IsInGameThread(); // Caches this so we don't have to go out of scope more than we need to. 
	bool taskSuccessfull = false;




	//-----------  CALCULATE COLORS TO APPLY/DETECT ----------- //
	
	if (UVertexPaintFunctionLibrary::IsWorldValid(CalculateColorsInfoGlobal.TaskFundamentalSettings.GetTaskWorld())) {


		if (IsMeshStillValid(CalculateColorsInfoGlobal)) {


			// If Paint job that makes use of color buffers
			if (DoesPaintJobUseVertexColorBuffer()) {

				CalculateColorsInfoGlobal.NewColorVertexBuffersPerLOD.SetNum(CalculateColorsInfoGlobal.LodsToLoopThrough);
			}


			switch (CalculateColorsInfoGlobal.VertexPaintDetectionType) {

			case EVertexPaintDetectionType::GetAllVertexColorDetection:

				if (UVertexPaintFunctionLibrary::ShouldTaskLoopThroughAllVerticesOnLOD(CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings, CalculateColorsInfoGlobal.GetVertexPaintComponent(), CalculateColorsInfoGlobal.PaintTaskSettings.OverrideVertexColorsToApplySettings)) {

					taskSuccessfull = CalculateColorToApply();
				}

				else {


					for (int i = 0; i < CalculateColorsInfoGlobal.LodsToLoopThrough; i++) {

						if (!CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD.IsValidIndex(i)) break;


						CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD[i].Lod = i;


						if (CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeVertexColorData) {

							if ((CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeVertexDataOnlyForLOD0 && i == 0) || !CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeVertexDataOnlyForLOD0) {

								CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD[i].MeshVertexColorsPerLODArray = UVertexPaintFunctionLibrary::GetMeshComponentVertexColorsAtLOD_Wrapper(CalculateColorsInfoGlobal.GetVertexPaintComponent(), i);

								CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD[i].AmountOfVerticesAtLOD = CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD[i].MeshVertexColorsPerLODArray.Num();
							}
						}


						// Gets the total one which is the one we use to check if we've fully painted or fully removed a mesh etc. so we don't override vertex colors if there's not gonna be any difference
						if (i == 0) {

							if (CalculateColorsInfoGlobal.TaskFundamentalSettings.DebugSettings.PrintLogsToScreen || CalculateColorsInfoGlobal.TaskFundamentalSettings.DebugSettings.PrintLogsToOutputLog) {

								CalculateColorsInfoGlobal.ColorsOfEachChannelResultsAtMinAmount_AboveZeroBeforeApplyingColor_Debug = UVertexPaintFunctionLibrary::GetAmountOfPaintedColorsForEachChannel(CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD[0].MeshVertexColorsPerLODArray, 0);
							}
						}

						taskSuccessfull = true;
					}

					if (taskSuccessfull) {

						if (CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeAmountOfPaintedColorsOfEachChannel.IncludeVertexColorChannelResultOfEachChannel && CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD.IsValidIndex(0)) {

							CalculateColorsInfoGlobal.ColorsOfEachChannelResultsAtMinAmount_BeforeApplyingColor = UVertexPaintFunctionLibrary::GetAmountOfPaintedColorsForEachChannel(CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD[0].MeshVertexColorsPerLODArray, CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeAmountOfPaintedColorsOfEachChannel.IncludeIfMinColorAmountIs);
						}
					}
				}

				break;

			case EVertexPaintDetectionType::PaintColorSnippet:

				if (CalculateColorsInfoGlobal.PaintColorSnippetSettings.ColorSnippetDataAssetInfo.ColorSnippetDataAssetSnippetIsStoredIn) {

					if (CalculateColorsInfoGlobal.PaintColorSnippetSettings.ColorSnippetDataAssetInfo.ColorSnippetDataAssetSnippetIsStoredIn->SnippetColorData.Contains(CalculateColorsInfoGlobal.PaintColorSnippetSettings.ColorSnippetDataAssetInfo.ColorSnippetID)) {

						if (CalculateColorsInfoGlobal.PaintColorSnippetSettings.ColorSnippetDataAssetInfo.ColorSnippetDataAssetSnippetIsStoredIn->SnippetColorData.FindRef(CalculateColorsInfoGlobal.PaintColorSnippetSettings.ColorSnippetDataAssetInfo.ColorSnippetID).ColorSnippetDataPerLOD.IsValidIndex(0)) {


							int amountOfVerticesAtLOD0 = UVertexPaintFunctionLibrary::GetMeshComponentAmountOfVerticesOnLOD(CalculateColorsInfoGlobal.GetVertexPaintComponent(), 0);

							ColorSnippetColors.SetNumZeroed(amountOfVerticesAtLOD0);
							ColorSnippetColors = CalculateColorsInfoGlobal.PaintColorSnippetSettings.ColorSnippetDataAssetInfo.ColorSnippetDataAssetSnippetIsStoredIn->SnippetColorData.FindRef(CalculateColorsInfoGlobal.PaintColorSnippetSettings.ColorSnippetDataAssetInfo.ColorSnippetID).ColorSnippetDataPerLOD[0].MeshVertexColorsPerLODArray;


							// If the Color snippet colors, matches the original LOD0 colors amount, i.e. it's safe to proceed
							if (ColorSnippetColors.Num() == amountOfVerticesAtLOD0) {

								if ((CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD.Num() > 1 && (CalculateColorsInfoGlobal.PropogateLOD0ToAllLODsFromStoredData || CalculateColorsInfoGlobal.PropogateLOD0ToAllLODsUsingOctree)) || ShouldPaintJobRunCalculateColorsToApply(CalculateColorsInfoGlobal.ApplyColorSettings, CalculateColorsInfoGlobal.PaintTaskSettings, CalculateColorsInfoGlobal.PaintDirectlyTaskSettings)) {

									// If we're going to loop through all of the vertices, and is set to override vertex colors to apply (and the Actor implements the interface etc.) then we set shouldApplyVertexColors to false, so depending on the result of the override, it will decide whether we should apply vertex colors or not. So if we override and not a single vertex returns that it should apply the color, then we shouldn't apply the paint job. This is a bit of a rare case for color snippets and paint jobs that sets the colors directly. 
									if (IsSetToOverrideVertexColorsAndImplementsInterface(CalculateColorsInfoGlobal.PaintTaskSettings))
										CalculateColorsInfoGlobal.PaintTaskResult.AnyVertexColorGotChanged = false;

									taskSuccessfull = CalculateColorToApply();
								}

								else {


									CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD[0].Lod = 0;
									CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD[0].AmountOfVerticesAtLOD = ColorSnippetColors.Num();


									if (CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeVertexColorData)
										CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD[0].MeshVertexColorsPerLODArray = ColorSnippetColors;


									// Can only create color snippets for skeletal and static meshes
									if (DoesPaintJobUseVertexColorBuffer()) {

										CalculateColorsInfoGlobal.NewColorVertexBuffersPerLOD[0] = new FColorVertexBuffer();
										CalculateColorsInfoGlobal.NewColorVertexBuffersPerLOD[0]->InitFromColorArray(ColorSnippetColors);
									}


									// If Paint Job like SetMeshComponentColors, Paint Color Snippet or Paint Entire Mesh then all Bones get affected. 
									if (CalculateColorsInfoGlobal.PaintTaskSettings.PaintTaskCallbackSettings.IncludeBonesThatGotPainted) {

										if (USkeletalMeshComponent* skelMeshComp = CalculateColorsInfoGlobal.GetVertexPaintSkelComponent()) {

											skelMeshComp->GetBoneNames(CalculateColorsInfoGlobal.PaintTaskResult.BonesThatGotPainted);
										}
									}


									if (CalculateColorsInfoGlobal.PaintTaskSettings.PaintTaskCallbackSettings.IncludePaintedVertexIndicesAtLOD0 && CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD.IsValidIndex(0)) {

										const int32 amountOfVerticesAtLOD = CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD[0].AmountOfVerticesAtLOD;
										CalculateColorsInfoGlobal.PaintTaskResult.VertexIndicesThatGotPainted.SetNumUninitialized(amountOfVerticesAtLOD);

										for (int i = 0; i < amountOfVerticesAtLOD; i++)
											CalculateColorsInfoGlobal.PaintTaskResult.VertexIndicesThatGotPainted[i] = i;
									}


									CalculateColorsInfoGlobal.PaintTaskResult.AnyVertexColorGotChanged = true;

									// If not looping through colors but setting them all right away then we can't determine which channels that got painted so adds them all
									SetAllColorChannelsToHaveBeenChanged();

									if (CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeAmountOfPaintedColorsOfEachChannel.IncludeVertexColorChannelResultOfEachChannel) {

										CalculateColorsInfoGlobal.ColorsOfEachChannelResultsAtMinAmount_AfterApplyingColor = UVertexPaintFunctionLibrary::GetAmountOfPaintedColorsForEachChannel(ColorSnippetColors, CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeAmountOfPaintedColorsOfEachChannel.IncludeIfMinColorAmountIs);
									}

									taskSuccessfull = true;
								}
							}

							else {

								CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Calculate Colors Task - Paint Color Snippet Task Fail as the stored Snippet Color Array Amount: %i didn't match the amount of Vertices the Mesh has at LOD0: %i! "), ColorSnippetColors.Num(), amountOfVerticesAtLOD0), FColor::Red));
							}
						}

						else {

							CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Calculate Colors Task - Paint Color Snippet Task Fail as there is Color Array stored for LOD0")), FColor::Red));
						}
					}

					else {

						CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Calculate Colors Task - Paint Color Snippet Task Fail as the Color Snippet Data Asset doesn't contain the Snippet. ")), FColor::Red));
					}
				}

				else {

					CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Calculate Colors Task - Paint Color Snippet Failed because Color Snippet Data Asset isn't Valid. Something must've gone really wrong if we've reached this point without valid ptr to it. ")), FColor::Red));
				}

				break;

			case EVertexPaintDetectionType::SetMeshVertexColorsDirectly:

				if (CalculateColorsInfoGlobal.SetVertexColorsSettings.VertexColorsAtLOD0ToSet.Num() > 0) {


					int amountOfVerticesAtLOD0 = UVertexPaintFunctionLibrary::GetMeshComponentAmountOfVerticesOnLOD(CalculateColorsInfoGlobal.GetVertexPaintComponent(), 0);


					if (amountOfVerticesAtLOD0 == CalculateColorsInfoGlobal.SetVertexColorsSettings.VertexColorsAtLOD0ToSet.Num()) {

						// If should Propogate LOD0 to other LODs or if should loop through vertices because of other reasons. 
						if ((CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD.Num() > 1 && (CalculateColorsInfoGlobal.PropogateLOD0ToAllLODsFromStoredData || CalculateColorsInfoGlobal.PropogateLOD0ToAllLODsUsingOctree)) || ShouldPaintJobRunCalculateColorsToApply(CalculateColorsInfoGlobal.ApplyColorSettings, CalculateColorsInfoGlobal.PaintTaskSettings, CalculateColorsInfoGlobal.PaintDirectlyTaskSettings)) {


							// Sets this to true already since we can't be dependent on the loop setting this to true since we're not applying color per vertex or anything and checking if it made any difference but sets all of them. 
							// CalculateColorsInfoGlobal.PaintTaskResult.AnyVertexColorGotChanged = true;

							// If we're going to loop through all of the vertices, and is set to override vertex colors to apply (and the Actor implements the interface etc.) then we set shouldApplyVertexColors to false, so depending on the result of the override, it will decide whether we should apply vertex colors or not. So if we override and not a single vertex returns that it should apply the color, then we shouldn't apply the paint job. This is a bit of a rare case for color snippets and paint jobs that sets the colors directly. 
							if (IsSetToOverrideVertexColorsAndImplementsInterface(CalculateColorsInfoGlobal.PaintTaskSettings))
								CalculateColorsInfoGlobal.PaintTaskResult.AnyVertexColorGotChanged = false;

							taskSuccessfull = CalculateColorToApply();
						}

						else {

							CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD[0].Lod = 0;
							CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD[0].AmountOfVerticesAtLOD = CalculateColorsInfoGlobal.SetVertexColorsSettings.VertexColorsAtLOD0ToSet.Num();


							if (CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeVertexColorData)
								CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD[0].MeshVertexColorsPerLODArray = CalculateColorsInfoGlobal.SetVertexColorsSettings.VertexColorsAtLOD0ToSet;


							if (DoesPaintJobUseVertexColorBuffer()) {

								CalculateColorsInfoGlobal.NewColorVertexBuffersPerLOD[0] = new FColorVertexBuffer();
								CalculateColorsInfoGlobal.NewColorVertexBuffersPerLOD[0]->InitFromColorArray(CalculateColorsInfoGlobal.SetVertexColorsSettings.VertexColorsAtLOD0ToSet);
							}

#if ENGINE_MAJOR_VERSION == 5

							else if (CalculateColorsInfoGlobal.IsDynamicMeshTask) {

								CalculateColorsInfoGlobal.DynamicMeshComponentVertexColors = CalculateColorsInfoGlobal.SetVertexColorsSettings.VertexColorsAtLOD0ToSet;
							}

							else if (CalculateColorsInfoGlobal.IsGeometryCollectionTask) {

								TArray<FLinearColor> colorsAsLinear;
								colorsAsLinear.SetNum(CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD[0].AmountOfVerticesAtLOD);

								for (int i = 0; i < CalculateColorsInfoGlobal.SetVertexColorsSettings.VertexColorsAtLOD0ToSet.Num(); i++)
									colorsAsLinear[i] = CalculateColorsInfoGlobal.SetVertexColorsSettings.VertexColorsAtLOD0ToSet[i];

								CalculateColorsInfoGlobal.GeometryCollectionComponentVertexColors = colorsAsLinear;
							}

#endif


							CalculateColorsInfoGlobal.PaintTaskResult.AnyVertexColorGotChanged = true;

							// If not looping through colors but setting them all right away then we can't determine which channels that got painted so adds them all
							SetAllColorChannelsToHaveBeenChanged();


							// If Paint Job like SetMeshComponentColors, Paint Color Snippet or Paint Entire Mesh then all Bones get affected. 
							if (CalculateColorsInfoGlobal.PaintTaskSettings.PaintTaskCallbackSettings.IncludeBonesThatGotPainted) {

								if (USkeletalMeshComponent* skelMeshComp = CalculateColorsInfoGlobal.GetVertexPaintSkelComponent()) {

									skelMeshComp->GetBoneNames(CalculateColorsInfoGlobal.PaintTaskResult.BonesThatGotPainted);
								}
							}


							if (CalculateColorsInfoGlobal.PaintTaskSettings.PaintTaskCallbackSettings.IncludePaintedVertexIndicesAtLOD0 && CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD.IsValidIndex(0)) {

								const int32 amountOfVerticesAtLOD = CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD[0].AmountOfVerticesAtLOD;
								CalculateColorsInfoGlobal.PaintTaskResult.VertexIndicesThatGotPainted.SetNumUninitialized(amountOfVerticesAtLOD);

								for (int i = 0; i < amountOfVerticesAtLOD; i++)
									CalculateColorsInfoGlobal.PaintTaskResult.VertexIndicesThatGotPainted[i] = i;
							}


							if (CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeAmountOfPaintedColorsOfEachChannel.IncludeVertexColorChannelResultOfEachChannel) {

								CalculateColorsInfoGlobal.ColorsOfEachChannelResultsAtMinAmount_AfterApplyingColor = UVertexPaintFunctionLibrary::GetAmountOfPaintedColorsForEachChannel(CalculateColorsInfoGlobal.SetVertexColorsSettings.VertexColorsAtLOD0ToSet, CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeAmountOfPaintedColorsOfEachChannel.IncludeIfMinColorAmountIs);
							}

							taskSuccessfull = true;
						}
					}

					else {

						CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Calculate Colors Task - Set Mesh Component Colors Task Fail as the VertexColorsAtLOD0ToSet didn't match the amount of Vertices the Mesh has at LOD0!")), FColor::Red));
					}
				}

				else {

					CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Calculate Colors Task - Set Mesh Component Colors Task Fail as the VertexColorsAtLOD0ToSet is 0 in length. ")), FColor::Red));
				}

				break;

			case EVertexPaintDetectionType::SetMeshVertexColorsDirectlyUsingSerializedString:

				if (CalculateColorsInfoGlobal.SetVertexColorsUsingSerializedStringSettings.SerializedColorDataAtLOD0.Len() > 0) {


					int amountOfVerticesAtLOD0 = UVertexPaintFunctionLibrary::GetMeshComponentAmountOfVerticesOnLOD(CalculateColorsInfoGlobal.GetVertexPaintComponent(), 0);

					DeserializedVertexColors.SetNumZeroed(amountOfVerticesAtLOD0);

					// De-Serializes the colors to apply
					DeserializedVertexColors = URVPDPRapidJsonFunctionLibrary::DeserializeTArrayFColor_Wrapper(CalculateColorsInfoGlobal.SetVertexColorsUsingSerializedStringSettings.SerializedColorDataAtLOD0);

					if (DeserializedVertexColors.Num() > 0) {

						if (amountOfVerticesAtLOD0 == DeserializedVertexColors.Num()) {

							// If should Propogate LOD0 to other LODs or if should loop through vertices because of other reasons. 
							if ((CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD.Num() > 1 && (CalculateColorsInfoGlobal.PropogateLOD0ToAllLODsFromStoredData || CalculateColorsInfoGlobal.PropogateLOD0ToAllLODsUsingOctree)) || ShouldPaintJobRunCalculateColorsToApply(CalculateColorsInfoGlobal.ApplyColorSettings, CalculateColorsInfoGlobal.PaintTaskSettings, CalculateColorsInfoGlobal.PaintDirectlyTaskSettings)) {

								// If we're going to loop through all of the vertices, and is set to override vertex colors to apply (and the Actor implements the interface etc.) then we set shouldApplyVertexColors to false, so depending on the result of the override, it will decide whether we should apply vertex colors or not. So if we override and not a single vertex returns that it should apply the color, then we shouldn't apply the paint job. This is a bit of a rare case for color snippets and paint jobs that sets the colors directly. 
								if (IsSetToOverrideVertexColorsAndImplementsInterface(CalculateColorsInfoGlobal.PaintTaskSettings))
									CalculateColorsInfoGlobal.PaintTaskResult.AnyVertexColorGotChanged = false;

								taskSuccessfull = CalculateColorToApply();
							}

							else {


								CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD[0].Lod = 0;
								CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD[0].AmountOfVerticesAtLOD = DeserializedVertexColors.Num();


								if (CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeVertexColorData)
									CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD[0].MeshVertexColorsPerLODArray = DeserializedVertexColors;


								if (DoesPaintJobUseVertexColorBuffer()) {

									CalculateColorsInfoGlobal.NewColorVertexBuffersPerLOD[0] = new FColorVertexBuffer();
									CalculateColorsInfoGlobal.NewColorVertexBuffersPerLOD[0]->InitFromColorArray(DeserializedVertexColors);
								}

#if ENGINE_MAJOR_VERSION == 5

								else if (CalculateColorsInfoGlobal.IsDynamicMeshTask) {

									CalculateColorsInfoGlobal.DynamicMeshComponentVertexColors = DeserializedVertexColors;
								}

								else if (CalculateColorsInfoGlobal.IsGeometryCollectionTask) {

									TArray<FLinearColor> colorsAsLinear;
									colorsAsLinear.SetNum(CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD[0].AmountOfVerticesAtLOD);

									for (int i = 0; i < DeserializedVertexColors.Num(); i++)
										colorsAsLinear[i] = DeserializedVertexColors[i];

									CalculateColorsInfoGlobal.GeometryCollectionComponentVertexColors = colorsAsLinear;
								}

#endif


								CalculateColorsInfoGlobal.PaintTaskResult.AnyVertexColorGotChanged = true;

								// If not looping through colors but setting them all right away then we can't determine which channels that got painted so adds them all
								SetAllColorChannelsToHaveBeenChanged();

								// If Paint Job like SetMeshComponentColors, Paint Color Snippet or Paint Entire Mesh then all Bones get affected. 
								if (CalculateColorsInfoGlobal.PaintTaskSettings.PaintTaskCallbackSettings.IncludeBonesThatGotPainted) {

									if (USkeletalMeshComponent* skelMeshComp = CalculateColorsInfoGlobal.GetVertexPaintSkelComponent()) {

										skelMeshComp->GetBoneNames(CalculateColorsInfoGlobal.PaintTaskResult.BonesThatGotPainted);
									}
								}


								if (CalculateColorsInfoGlobal.PaintTaskSettings.PaintTaskCallbackSettings.IncludePaintedVertexIndicesAtLOD0 && CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD.IsValidIndex(0)) {

									const int32 amountOfVerticesAtLOD = CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD[0].AmountOfVerticesAtLOD;
									CalculateColorsInfoGlobal.PaintTaskResult.VertexIndicesThatGotPainted.SetNumUninitialized(amountOfVerticesAtLOD);

									for (int i = 0; i < amountOfVerticesAtLOD; i++)
										CalculateColorsInfoGlobal.PaintTaskResult.VertexIndicesThatGotPainted[i] = i;
								}


								if (CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeAmountOfPaintedColorsOfEachChannel.IncludeVertexColorChannelResultOfEachChannel) {

									CalculateColorsInfoGlobal.ColorsOfEachChannelResultsAtMinAmount_AfterApplyingColor = UVertexPaintFunctionLibrary::GetAmountOfPaintedColorsForEachChannel(DeserializedVertexColors, CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeAmountOfPaintedColorsOfEachChannel.IncludeIfMinColorAmountIs);
								}

								taskSuccessfull = true;
							}
						}

						else {

							CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Calculate Colors Task - Set Component Mesh Colors with Serialized String Task Fail as the De-Serailized Color Array didn't match the amount of Vertices the Mesh has at LOD0!")), FColor::Red));
						}
					}

					else {

						CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Calculate Colors Task - Set Mesh Component Colors with Serialized String Task Fail as the De-Serialized Color Array was 0 in length!")), FColor::Red));
					}
				}

				else {

					CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Calculate Colors Task - Set Mesh Component Colors with Serialized String Task Fail as the String to De-Serialize is 0 in Length!")), FColor::Red));
				}

				break;

			case EVertexPaintDetectionType::PaintEntireMesh:

				if (!CalculateColorsInfoGlobal.PaintOnEntireMeshSettings.PaintOnRandomVerticesSettings.OnlyRandomizeWithinAreaOfEffectAtLocation && !CalculateColorsInfoGlobal.PaintOnEntireMeshSettings.PaintOnRandomVerticesSettings.PaintAtRandomVerticesSpreadOutOverTheEntireMesh && CalculateColorsInfoGlobal.ApplyColorSettings.ApplyVertexColorSettings.IsAnyColorChannelSetToSetColor() && !CalculateColorsInfoGlobal.ApplyColorSettings.ApplyVertexColorSettings.IsAnyColorChannelSetToAddColorAndIsNotZero()) {


					bool hasPhysicsSurfacePaintCondition = false;
					bool hasRedChannelPaintCondition = false;
					bool hasGreenChannelPaintCondition = false;
					bool hasBlueChannelPaintCondition = false;
					bool hasAlphaChannelPaintCondition = false;

					int amountOfVertexNormalWithinDotToDirectionConditions = 0;
					int amountOfOnlyAffectVerticesWithDirectionToActorOrLocationConditions = 0;
					int amountOfOnlyAffectVerticesWithinDirectionFromActorOrLocation = 0;
					int amountOfIfVertexIsAboveOrBelowWorldZ = 0;
					int amountOfIfVertexColorWithinRange = 0;
					int amountOfIfVertexHasLineOfSightTo = 0;
					int amountOfIfVertexIsOnBone = 0;
					int amountOfIfVertexIsOnMaterial = 0;

					const bool hasAnyPaintCondition = UVertexPaintFunctionLibrary::IsThereAnyPaintConditions(CalculateColorsInfoGlobal.ApplyColorSettings.ApplyVertexColorSettings, hasPhysicsSurfacePaintCondition, hasRedChannelPaintCondition, hasGreenChannelPaintCondition, hasBlueChannelPaintCondition, hasAlphaChannelPaintCondition, amountOfVertexNormalWithinDotToDirectionConditions, amountOfOnlyAffectVerticesWithDirectionToActorOrLocationConditions, amountOfOnlyAffectVerticesWithinDirectionFromActorOrLocation, amountOfIfVertexIsAboveOrBelowWorldZ, amountOfIfVertexColorWithinRange, amountOfIfVertexHasLineOfSightTo, amountOfIfVertexIsOnBone, amountOfIfVertexIsOnMaterial);

					if (CalculateColorsInfoGlobal.ApplyColorSettings.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.ApplyVertexColorUsingPhysicsSurface || ShouldPaintJobRunCalculateColorsToApply(CalculateColorsInfoGlobal.PaintOnEntireMeshSettings, CalculateColorsInfoGlobal.PaintTaskSettings, CalculateColorsInfoGlobal.PaintDirectlyTaskSettings) || hasAnyPaintCondition) {

						taskSuccessfull = CalculateColorToApply();
					}

					// If there's no reason to loop through all the vertices then initializes the color buffer right away. Checks if set to apply vertex colors using physics surface as well, because if so, then we need to loop through the vertices since we need to get the sections to get the material of each section and thus the physics surface registered at RGBA at each section and if they have the physics surface we're trying to paint. 
					else {

						for (int i = 0; i < CalculateColorsInfoGlobal.LodsToLoopThrough; i++) {

							if (!CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD.IsValidIndex(i)) break;


							// Gets the total one which is the one we use to check if we've fully painted or fully removed a mesh etc. so we don't override vertex colors if there's not gonna be any difference
							if (i == 0) {

								if (CalculateColorsInfoGlobal.TaskFundamentalSettings.DebugSettings.PrintLogsToScreen || CalculateColorsInfoGlobal.TaskFundamentalSettings.DebugSettings.PrintLogsToOutputLog) {

									CalculateColorsInfoGlobal.ColorsOfEachChannelResultsAtMinAmount_AboveZeroBeforeApplyingColor_Debug = UVertexPaintFunctionLibrary::GetAmountOfPaintedColorsForEachChannel(UVertexPaintFunctionLibrary::GetMeshComponentVertexColorsAtLOD_Wrapper(CalculateColorsInfoGlobal.GetVertexPaintComponent(), i), 0);
								}
							}


							const int amountOfVerticesAtLOD = UVertexPaintFunctionLibrary::GetMeshComponentAmountOfVerticesOnLOD(CalculateColorsInfoGlobal.GetVertexPaintComponent(), i);

							if (amountOfVerticesAtLOD <= 0)
								continue;


							FLinearColor linearColorToInitialize = FLinearColor();
							linearColorToInitialize.R = CalculateColorsInfoGlobal.ApplyColorSettings.ApplyVertexColorSettings.ApplyColorsOnRedChannel.AmountToApply;
							linearColorToInitialize.G = CalculateColorsInfoGlobal.ApplyColorSettings.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.AmountToApply;
							linearColorToInitialize.B = CalculateColorsInfoGlobal.ApplyColorSettings.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.AmountToApply;
							linearColorToInitialize.A = CalculateColorsInfoGlobal.ApplyColorSettings.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.AmountToApply;
							const FColor colorToInitialize = UVertexPaintFunctionLibrary::ReliableFLinearToFColor(linearColorToInitialize);


							CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD[i].Lod = i;
							CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD[i].AmountOfVerticesAtLOD = amountOfVerticesAtLOD;


							if (CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeVertexColorData) {

								if ((CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeVertexDataOnlyForLOD0 && i == 0) || !CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeVertexDataOnlyForLOD0) {

									CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD[i].MeshVertexColorsPerLODArray.Init(colorToInitialize, amountOfVerticesAtLOD);
								}
							}


							if (DoesPaintJobUseVertexColorBuffer()) {

								CalculateColorsInfoGlobal.NewColorVertexBuffersPerLOD[i] = new FColorVertexBuffer();
								CalculateColorsInfoGlobal.NewColorVertexBuffersPerLOD[i]->InitFromSingleColor(colorToInitialize, amountOfVerticesAtLOD);
							}

#if ENGINE_MAJOR_VERSION == 5

							else if (CalculateColorsInfoGlobal.IsDynamicMeshTask) {

								CalculateColorsInfoGlobal.DynamicMeshComponentVertexColors = CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD[i].MeshVertexColorsPerLODArray;

								CalculateColorsInfoGlobal.DynamicMeshComponentVertexColors.Init(colorToInitialize, amountOfVerticesAtLOD);
							}

							else if (CalculateColorsInfoGlobal.IsGeometryCollectionTask) {

								CalculateColorsInfoGlobal.GeometryCollectionComponentVertexColors.Init(linearColorToInitialize, amountOfVerticesAtLOD);
							}

#endif

							taskSuccessfull = true;
						}


						if (taskSuccessfull) {

							// If Setting the Entire Mesh Color, always result in change since we have no way of way of checking if they did without looping through verts, which would make the task take longer.
							CalculateColorsInfoGlobal.PaintTaskResult.AnyVertexColorGotChanged = true;


							// If not looping through colors but setting them all right away then we can't determine which channels that got painted so adds them all
							SetAllColorChannelsToHaveBeenChanged();

							// If Paint Job like SetMeshComponentColors, Paint Color Snippet or Paint Entire Mesh then all Bones get affected. 
							if (CalculateColorsInfoGlobal.PaintTaskSettings.PaintTaskCallbackSettings.IncludeBonesThatGotPainted) {

								if (USkeletalMeshComponent* skelMeshComp = CalculateColorsInfoGlobal.GetVertexPaintSkelComponent()) {

									skelMeshComp->GetBoneNames(CalculateColorsInfoGlobal.PaintTaskResult.BonesThatGotPainted);
								}
							}


							if (CalculateColorsInfoGlobal.PaintTaskSettings.PaintTaskCallbackSettings.IncludePaintedVertexIndicesAtLOD0 && CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD.IsValidIndex(0)) {

								const int32 amountOfVerticesAtLOD = CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD[0].AmountOfVerticesAtLOD;
								CalculateColorsInfoGlobal.PaintTaskResult.VertexIndicesThatGotPainted.SetNumUninitialized(amountOfVerticesAtLOD);

								for (int i = 0; i < amountOfVerticesAtLOD; i++)
									CalculateColorsInfoGlobal.PaintTaskResult.VertexIndicesThatGotPainted[i] = i;
							}
						}
					}
				}

				else {

					taskSuccessfull = CalculateColorToApply();
				}

				break;

			default:

				taskSuccessfull = CalculateColorToApply();

				break;
			}
		}

		else {

			CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Calculate Colors Task - Failed because IsMeshStillValid() Check failed right away. ")), FColor::Red));
		}
	}

	else {

		CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Calculate Colors Task - Failed because IsWorldStillValid() Check failed right away. ")), FColor::Red));
	}


	CalculateColorsInfoGlobal.TaskResult.TaskSuccessfull = taskSuccessfull;



	//----------- ADJUSTS CALLBACK DATA ----------- //

	// If Paint Job
	if (CalculateColorsInfoGlobal.IsPaintTask) {
		
		CalculateColorsInfoGlobal.TaskResult.AmountOfPaintedColorsOfEachChannel = CalculateColorsInfoGlobal.ColorsOfEachChannelResultsAtMinAmount_AfterApplyingColor;
		
		if (CalculateColorsInfoGlobal.IsDetectCombo) {

			CalculateColorsInfoGlobal.DetectComboTaskResults.AmountOfPaintedColorsOfEachChannel = CalculateColorsInfoGlobal.ColorsOfEachChannelResultsAtMinAmount_BeforeApplyingColor;
		}
	}

	// If Detect Job
	else {

		CalculateColorsInfoGlobal.TaskResult.AmountOfPaintedColorsOfEachChannel = CalculateColorsInfoGlobal.ColorsOfEachChannelResultsAtMinAmount_BeforeApplyingColor;
	}


	if (CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeVertexDataOnlyForLOD0 && CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD.Num() > 1) {

		CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Calculate Colors Task - Adjusting Amount of Mesh Data LODs as the Callback Settings is set to only Include Vertex Data for LOD0. ")), FColor::Cyan));

		CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD.SetNum(1);
		CalculateColorsInfoGlobal.DetectComboTaskResults.MeshVertexData.MeshDataPerLOD.SetNum(1);
	}


	// Clears it on each element that's left if not set to include it
	for (int i = 0; i < CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD.Num(); i++) {

		if (!CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeVertexColorData) {

			CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD[i].MeshVertexColorsPerLODArray.Empty();

			if (CalculateColorsInfoGlobal.DetectComboTaskResults.MeshVertexData.MeshDataPerLOD.IsValidIndex(i))
				CalculateColorsInfoGlobal.DetectComboTaskResults.MeshVertexData.MeshDataPerLOD[i].MeshVertexColorsPerLODArray.Empty();
		}

		if (!CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeVertexPositionData) {

			CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD[i].MeshVertexPositionsInComponentSpacePerLODArray.Empty();

			if (CalculateColorsInfoGlobal.DetectComboTaskResults.MeshVertexData.MeshDataPerLOD.IsValidIndex(i))
				CalculateColorsInfoGlobal.DetectComboTaskResults.MeshVertexData.MeshDataPerLOD[i].MeshVertexPositionsInComponentSpacePerLODArray.Empty();
		}

		if (!CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeVertexNormalData) {

			CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD[i].MeshVertexNormalsPerLODArray.Empty();

			if (CalculateColorsInfoGlobal.DetectComboTaskResults.MeshVertexData.MeshDataPerLOD.IsValidIndex(i))
				CalculateColorsInfoGlobal.DetectComboTaskResults.MeshVertexData.MeshDataPerLOD[i].MeshVertexNormalsPerLODArray.Empty();
		}

		if (!CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeVertexIndexes) {

			CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD[i].MeshVertexIndexes.Empty();

			if (CalculateColorsInfoGlobal.DetectComboTaskResults.MeshVertexData.MeshDataPerLOD.IsValidIndex(i))
				CalculateColorsInfoGlobal.DetectComboTaskResults.MeshVertexData.MeshDataPerLOD[i].MeshVertexIndexes.Empty();
		}

		if (!CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeSerializedVertexColorData) {

			CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD[i].SerializedVertexColorsData.ColorsAtLODAsJSonString = "";

			if (CalculateColorsInfoGlobal.DetectComboTaskResults.MeshVertexData.MeshDataPerLOD.IsValidIndex(i))
				CalculateColorsInfoGlobal.DetectComboTaskResults.MeshVertexData.MeshDataPerLOD[i].SerializedVertexColorsData.ColorsAtLODAsJSonString = "";
		}
	}


	if (CalculateColorsInfoGlobal.IsPaintTask) {

		PaintJobPrintDebugLogs(CalculateColorsInfoGlobal.ApplyColorSettings, CalculateColorsInfoGlobal.PaintTaskResult.AnyVertexColorGotChanged, CalculateColorsInfoGlobal.OverridenVertexColors_MadeChangeToColorsToApply, CalculateColorsInfoGlobal.ColorsOfEachChannelResultsAtMinAmount_AboveZeroBeforeApplyingColor_Debug, CalculateColorsInfoGlobal.ColorsOfEachChannelResultsAtMinAmount_BeforeApplyingColor, CalculateColorsInfoGlobal.ColorsOfEachChannelResultsAtMinAmount_AfterApplyingColor);
	}


	//-----------  TASK FINISHED ----------- //

	if (InGameThread) {

		
		if (UVertexPaintFunctionLibrary::IsWorldValid(CalculateColorsInfoGlobal.TaskFundamentalSettings.GetTaskWorld())) {

			CalculateColorsInfoGlobal.TaskResult.TaskDuration = CalculateColorsInfoGlobal.TaskFundamentalSettings.TaskWorld.Get()->RealTimeSeconds - CalculateColorsInfoGlobal.TaskStartedTimeStamp;

			if (CalculateColorsInfoGlobal.TaskResult.TaskDuration < 0)
				CalculateColorsInfoGlobal.TaskResult.TaskDuration = 0;
		}


		// If task success then can only remain a success if task still valid
		if (CalculateColorsInfoGlobal.TaskResult.TaskSuccessfull) {

			CalculateColorsInfoGlobal.TaskResult.TaskSuccessfull = IsTaskStillValid(CalculateColorsInfoGlobal);

			if (!CalculateColorsInfoGlobal.TaskResult.TaskSuccessfull) {

				CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Calculate Colors Task - Task was Successfull but no longer valid, possible has gotten removed from queue, so becomes unsuccessfull. ")), FColor::Red));
			}
		}

		CalculateColorsInfoGlobal.DetectComboTaskResults.TaskDuration = CalculateColorsInfoGlobal.TaskResult.TaskDuration;
		CalculateColorsInfoGlobal.DetectComboTaskResults.TaskSuccessfull = CalculateColorsInfoGlobal.TaskResult.TaskSuccessfull;


		for (const FRVPDPAsyncTaskDebugMessagesInfo& message : CalculateColorsResultMessage) {

			VertexPaintDetectionLog::PrintVertexPaintDetectionLog(CalculateColorsInfoGlobal.TaskFundamentalSettings.DebugSettings, message.MessageColor, message.DebugMessage);
		}


		if (AsyncTaskFinishedDelegate.IsBound())
			AsyncTaskFinishedDelegate.Execute(CalculateColorsInfoGlobal);
	}

	else {

		FVertexPaintAsyncTaskFinished taskFinishedDelegate = AsyncTaskFinishedDelegate;
		TArray<FRVPDPAsyncTaskDebugMessagesInfo> taskResultMessages = CalculateColorsResultMessage;
		TSharedPtr<FRVPDPCalculateColorsInfo> sharedCalculateColorsInfo =
			MakeShared<FRVPDPCalculateColorsInfo>(CalculateColorsInfoGlobal);


		// Broadcasts on Game Thread so the actual Applying of colors happens there
		AsyncTask(ENamedThreads::GameThread, [this, taskFinishedDelegate, sharedCalculateColorsInfo, taskResultMessages]() mutable {


			if (!sharedCalculateColorsInfo.IsValid()) {

				if (taskFinishedDelegate.IsBound())
					taskFinishedDelegate.Execute(FRVPDPCalculateColorsInfo());

				return;
			}

			FRVPDPCalculateColorsInfo& calculateColorsInfo = *sharedCalculateColorsInfo;

			// Sets Duration and Task Success here, so when task queue and game instance use it they can use use const & refs of it
			if (calculateColorsInfo.TaskFundamentalSettings.TaskWorld.IsValid() && UVertexPaintFunctionLibrary::IsWorldValid(calculateColorsInfo.TaskFundamentalSettings.GetTaskWorld())) {

				calculateColorsInfo.TaskResult.TaskDuration = calculateColorsInfo.TaskFundamentalSettings.TaskWorld.Get()->RealTimeSeconds - calculateColorsInfo.TaskStartedTimeStamp;

				if (calculateColorsInfo.TaskResult.TaskDuration < 0)
					calculateColorsInfo.TaskResult.TaskDuration = 0;
			}


			// If task success then can only remain a success if task still valid
			if (calculateColorsInfo.TaskResult.TaskSuccessfull) {

				calculateColorsInfo.TaskResult.TaskSuccessfull = this->IsTaskStillValid(calculateColorsInfo);

				if (!calculateColorsInfo.TaskResult.TaskSuccessfull) {

					taskResultMessages.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Calculate Colors Task - Async Task was Successfull but no longer valid, possible has gotten removed from queue, so becomes unsuccessfull. ")), FColor::Red));
				}
			}

			calculateColorsInfo.DetectComboTaskResults.TaskDuration = calculateColorsInfo.TaskResult.TaskDuration;
			calculateColorsInfo.DetectComboTaskResults.TaskSuccessfull = calculateColorsInfo.TaskResult.TaskSuccessfull;


			// Got a crash very rarely when printing with PrintString in async so moved it so we print them all here when in game thread.
			for (const FRVPDPAsyncTaskDebugMessagesInfo& message : taskResultMessages) {

				VertexPaintDetectionLog::PrintVertexPaintDetectionLog(calculateColorsInfo.TaskFundamentalSettings.DebugSettings, message.MessageColor, message.DebugMessage);
			}


			if (taskFinishedDelegate.IsBound())
				taskFinishedDelegate.Execute(calculateColorsInfo);
		});
	}
}


//-------------------------------------------------------

// Calculate Color To Apply

bool FVertexPaintCalculateColorsTask::CalculateColorToApply() {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_CalculateColorToApply);
	RVPDP_CPUPROFILER_STR("Calculate Colors Task - CalculateColorToApply");


	CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Calculate Colors Task - For Actor %s and Mesh: %s with %i LODs!"), *CalculateColorsInfoGlobal.VertexPaintActorName, *CalculateColorsInfoGlobal.VertexPaintComponentName, CalculateColorsInfoGlobal.LodsToLoopThrough), FColor::Cyan));


	// General
	bool shouldLoopThroughAllVerticesOnLOD = UVertexPaintFunctionLibrary::ShouldTaskLoopThroughAllVerticesOnLOD(CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings, CalculateColorsInfoGlobal.GetVertexPaintComponent(), CalculateColorsInfoGlobal.PaintTaskSettings.OverrideVertexColorsToApplySettings);

	// Detect Combo
	FRVPDPVertexDataInfo detectComboMeshVertexData;

	// Paint at Location
	FVector paintAtLocation_HitLocationInWorldSpace = FVector(ForceInitToZero);

	// Get Closest Vertex Data
	GetClosestVertexData = (CalculateColorsInfoGlobal.IsGetClosestVertexDataTask() || (CalculateColorsInfoGlobal.IsPaintAtLocationTask() && CalculateColorsInfoGlobal.IsDetectCombo));
	FVector getClosestVertexData_HitLocationInWorldSpace = FVector(ForceInitToZero);

	// Colors Within Area
	GetColorsWithinArea = (CalculateColorsInfoGlobal.IsGetColorsWithinAreaTask() || (CalculateColorsInfoGlobal.IsPaintWithinAreaTask() && CalculateColorsInfoGlobal.IsDetectCombo));

	// Skeletal Mesh
	int skeletalMeshPredictedLOD = 0;
	FSkeletalMeshRenderData* skeletalMeshRenderData = nullptr;
	TArray<FRVPDPSkeletalMeshBoneInfoPerLOD> skeletalMeshBoneInfo;

	// Paint Colors Directly
	FRVPDPPaintLimitSettings paintDirectlyLimit_RedChannel;
	FRVPDPPaintLimitSettings paintDirectlyLimit_GreenChannel;
	FRVPDPPaintLimitSettings paintDirectlyLimit_BlueChannel;
	FRVPDPPaintLimitSettings paintDirectlyLimit_AlphaChannel;

	// Cloth
	TArray<UClothingAssetBase*> cloth_ClothingAssets;
	TArray<FVector> cloth_BoneLocationInComponentSpace;
	TArray<FQuat> cloth_BoneQuaternionsInComponentSpace;

	// Override Colors to Apply
	UObject* objectToOverrideVertexColorsWith = nullptr;
	UVertexPaintDetectionComponent* objectToOverrideInstigatedByPaintComponent = nullptr;



	//-----------  LOD0 SPECIFIC PROPERTIES ----------- //

	// Paint at Location
	int paintAtLocation_ClosestVertex = -1;
	int paintAtLocation_ClosestSection = -1;
	float paintAtLocation_ClosestDistanceToClosestVertex = 1000000000;
	float paintAtLocation_AverageColorInAreaOfEffect_Red = 0;
	float paintAtLocation_AverageColorInAreaOfEffect_Green = 0;
	float paintAtLocation_AverageColorInAreaOfEffect_Blue = 0;
	float paintAtLocation_AverageColorInAreaOfEffect_Alpha = 0;
	int paintAtLocation_AmountOfVerticesWithinArea = 0;

	// Closest Vertex Data
	int getClosestVertexData_ClosestVertex = -1;
	int getClosestVertexData_ClosestSection = -1;
	int getClosestVertexData_AmountOfVerticesWithinArea = 0;
	float getClosestVertexData_ClosestDistanceToClosestVertex = 1000000000;
	bool acceptClosestVertexDataIfVertexIsWithinMinRange_GotClosestVertex = false;
	float getClosestVertexData_AverageColorWithinAreaOfEffect_BeforeApplyingColors_Red = 0;
	float getClosestVertexData_AverageColorWithinAreaOfEffect_BeforeApplyingColors_Green = 0;
	float getClosestVertexData_AverageColorWithinAreaOfEffect_BeforeApplyingColors_Blue = 0;
	float getClosestVertexData_AverageColorWithinAreaOfEffect_BeforeApplyingColors_Alpha = 0;

	// Propagate Colors to other LODs
	TArray<FBox> propogateToLODs_CompleteLODsBaseBounds;
	TArray<FExtendedPaintedVertex> propogateToLODs_PaintedVerticesAtLOD0;
	FBox propogateToLODs_CurrentLODBaseBounds;

	// Amount of Colors of Each Channel
	FRVPDPAmountOfColorsOfEachChannelResults amountOfPaintedColorsOfEachChannelAbove0_BeforeApplyingColor_Debug;
	FRVPDPAmountOfColorsOfEachChannelResults amountOfPaintedColorsOfEachChannel_BeforeApplyingColor;
	FRVPDPAmountOfColorsOfEachChannelResults amountOfPaintedColorsOfEachChannel_AfterApplyingColors;

	// Within Area Amount of Colors of Each Channel
	FRVPDPAmountOfColorsOfEachChannelResults amountOfPaintedColorsOfEachChannel_WithinArea_BeforeApplyingColors;
	FRVPDPAmountOfColorsOfEachChannelResults amountOfPaintedColorsOfEachChannel_WithinArea_AfterApplyingColors;

	// Serialized Color Data
	FString serializedColorData = "";

	// Detect Combo
	FRVPDPVertexColorsOnEachBoneResult colorsOfEachBoneBeforeApplyingColorsResult;
	FRVPDPCompareMeshVertexColorsToColorSnippetResult compareMeshVertexColorsToColorSnippetResult_BeforeApplyingColors;
	FRVPDPCompareMeshVertexColorsToColorArrayResult compareMeshVertexColorsToColorArrayResult_BeforeApplyingColors;

	// Compare Color Array
	bool compareColorsToArray = false;
	int compareColorsToArray_AmountOfVerticesConsidered_AfterApplyingColors = 0;
	int compareColorsToArray_AmountOfVerticesConsidered_BeforeApplyingColors = 0;
	float compareMeshVertexColorsToColorArrayMatchingPercent_AfterApplyingColors = 0;
	float compareMeshVertexColorsToColorArrayMatchingPercent_BeforeApplyingColors = 0;

	// Compare Color Snippet
	bool compareColorsToColorSnippet = false;
	TArray<TArray<FColor>> CompareColorSnippetColorArray;
	TArray<int> compareColorsToSnippet_AmountOfVerticesConsidered_AfterApplyingColors;
	TArray<int> compareColorsToSnippet_AmountOfVerticesConsidered_BeforeApplyingColors;
	TArray<float> compareColorSnippetMatchingPercent_AfterApplyingColors;
	TArray<float> compareColorSnippetMatchingPercent_BeforeApplyingColors;

	// Estimated Color to Hit Location
	FVector estimatedColorToHitLocation_DirectionToClosestVertex_Paint;
	float estimatedColorToHitLocation_DirectionFromHitToClosestVerticesInfo_LongestDistance_Paint = 10000000;
	int estimatedColorToHitLocation_DirectionFromHitToClosestVerticesInfo_LongestDistanceIndex_Paint = -1;
	TMap<int, FRVPDPDirectionFromHitLocationToClosestVerticesInfo> estimatedColorToHitLocation_DirectionFromHitToClosestVerticesInfo_Paint;

	FVector estimatedColorToHitLocation_DirectionToClosestVertex_Detect;
	float estimatedColorToHitLocation_DirectionFromHitToClosestVerticesInfo_LongestDistance_Detect = 10000000;
	int estimatedColorToHitLocation_DirectionFromHitToClosestVerticesInfo_LongestDistanceIndex_Detect = -1;
	TMap<int, FRVPDPDirectionFromHitLocationToClosestVerticesInfo> estimatedColorToHitLocation_DirectionFromHitToClosestVerticesInfo_Detect;



#if ENGINE_MAJOR_VERSION == 4

	TMap<int32, TArray<FVector>> cloth_VertexPositions;
	TMap<int32, TArray<FVector>> cloth_VertexNormals;

#elif ENGINE_MAJOR_VERSION == 5

	TMap<int32, TArray<FVector3f>> cloth_VertexPositions;
	TMap<int32, TArray<FVector3f>> cloth_VertexNormals;


	// Dynamic Mesh
	UE::Geometry::FDynamicMesh3* dynamicMesh_3 = nullptr;

#endif


	if (USkeletalMeshComponent* skelMeshComponent = CalculateColorsInfoGlobal.GetVertexPaintSkelComponent()) {

		USkeletalMesh* skeletalMesh = CalculateColorsInfoGlobal.GetVertexPaintSkeletalMesh();
		skeletalMeshRenderData = skelMeshComponent->GetSkeletalMeshRenderData();
		if (!IsValidSkeletalMesh(skeletalMesh, skelMeshComponent, skeletalMeshRenderData, 0)) return false;

		skeletalMeshPredictedLOD = skelMeshComponent->GetPredictedLODLevel();

		GetSkeletalMeshClothInfo(skelMeshComponent, skeletalMesh, CalculateColorsInfoGlobal.SkeletalMeshBonesNames, cloth_ClothingAssets, cloth_VertexPositions, cloth_VertexNormals, cloth_BoneLocationInComponentSpace, cloth_BoneQuaternionsInComponentSpace);


		// If we only want to loop through specific bones and is registered
		if (!shouldLoopThroughAllVerticesOnLOD && CalculateColorsInfoGlobal.SpecificSkeletalMeshBonesToLoopThrough.Num()) {

			if (UVertexPaintOptimizationDataAsset* optimizationDataAsset = UVertexPaintFunctionLibrary::GetOptimizationDataAsset(CalculateColorsInfoGlobal.TaskFundamentalSettings.TaskWorld.Get())) {

				if (optimizationDataAsset->GetRegisteredSkeletalMeshInfo().Contains(skeletalMesh)) {


					skeletalMeshBoneInfo = optimizationDataAsset->GetRegisteredSkeletalMeshInfo().FindRef(skeletalMesh).SkeletalMeshBoneInfoPerLOD;

					if (skeletalMeshBoneInfo.Num() > 0) {

						CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Calculate Colors Task - Set to only Loop Through %i Skeletal Mesh Bones!"), CalculateColorsInfoGlobal.SpecificSkeletalMeshBonesToLoopThrough.Num()), FColor::Cyan));
					}

					else {

						shouldLoopThroughAllVerticesOnLOD = true;
					}
				}

				else {

					shouldLoopThroughAllVerticesOnLOD = true;
				}
			}

			else {

				shouldLoopThroughAllVerticesOnLOD = true;
			}
		}

		// Otherwise make sure we loop through all vertices
		else {

			shouldLoopThroughAllVerticesOnLOD = true;

			// CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Calculate Colors Task - Set to only Loop Through %i Skeletal Mesh Bones but the Skeletal Mesh isn't Registered in the Optimization Tab!"), CalculateColorsInfoGlobal.SpecificSkeletalMeshBonesToLoopThrough.Num()), FColor::Cyan));
		}
	}

	if (GetClosestVertexData) {

		getClosestVertexData_HitLocationInWorldSpace = UKismetMathLibrary::TransformLocation(CalculateColorsInfoGlobal.VertexPaintComponentTransform, CalculateColorsInfoGlobal.GetClosestVertexDataSettings.HitFundementals.HitLocationInComponentSpace);
	}

	if (CalculateColorsInfoGlobal.IsPaintAtLocationTask()) {

		paintAtLocation_HitLocationInWorldSpace = UKismetMathLibrary::TransformLocation(CalculateColorsInfoGlobal.VertexPaintComponentTransform, CalculateColorsInfoGlobal.PaintAtLocationSettings.HitFundementals.HitLocationInComponentSpace);
	}

	if (CalculateColorsInfoGlobal.IsPaintWithinAreaTask() || GetColorsWithinArea) {

		if (!GetWithinAreaInfo(CalculateColorsInfoGlobal.GetVertexPaintComponent(), CalculateColorsInfoGlobal.WithinAreaSettings.ComponentsToCheckIfIsWithin)) return false;

		CalculateColorsInfoGlobal.WithinArea_Results_BeforeApplyingColors.MeshVertexDataWithinArea.SetNum(CalculateColorsInfoGlobal.LodsToLoopThrough);
		CalculateColorsInfoGlobal.WithinArea_Results_AfterApplyingColors.MeshVertexDataWithinArea.SetNum(CalculateColorsInfoGlobal.LodsToLoopThrough);
	}

	if (CalculateColorsInfoGlobal.IsPaintDirectlyTask) {

		// Always want to go through all of the vertices if paint color snippet, set mesh component colors or set mesh component colors using serialized string
		shouldLoopThroughAllVerticesOnLOD = true;

		paintDirectlyLimit_RedChannel.UsePaintLimits = CalculateColorsInfoGlobal.PaintDirectlyTaskSettings.VertexColorRedChannelsLimit.UsePaintLimits;
		paintDirectlyLimit_RedChannel.LimitColorIfTheColorWasAlreadyOverTheLimit = CalculateColorsInfoGlobal.PaintDirectlyTaskSettings.VertexColorRedChannelsLimit.LimitColorIfTheColorWasAlreadyOverTheLimit;
		paintDirectlyLimit_RedChannel.PaintLimit = CalculateColorsInfoGlobal.PaintDirectlyTaskSettings.VertexColorRedChannelsLimit.PaintLimit;

		paintDirectlyLimit_GreenChannel.UsePaintLimits = CalculateColorsInfoGlobal.PaintDirectlyTaskSettings.VertexColorGreenChannelsLimit.UsePaintLimits;
		paintDirectlyLimit_GreenChannel.LimitColorIfTheColorWasAlreadyOverTheLimit = CalculateColorsInfoGlobal.PaintDirectlyTaskSettings.VertexColorGreenChannelsLimit.LimitColorIfTheColorWasAlreadyOverTheLimit;
		paintDirectlyLimit_GreenChannel.PaintLimit = CalculateColorsInfoGlobal.PaintDirectlyTaskSettings.VertexColorGreenChannelsLimit.PaintLimit;

		paintDirectlyLimit_BlueChannel.UsePaintLimits = CalculateColorsInfoGlobal.PaintDirectlyTaskSettings.VertexColorBlueChannelsLimit.UsePaintLimits;
		paintDirectlyLimit_BlueChannel.LimitColorIfTheColorWasAlreadyOverTheLimit = CalculateColorsInfoGlobal.PaintDirectlyTaskSettings.VertexColorBlueChannelsLimit.LimitColorIfTheColorWasAlreadyOverTheLimit;
		paintDirectlyLimit_BlueChannel.PaintLimit = CalculateColorsInfoGlobal.PaintDirectlyTaskSettings.VertexColorBlueChannelsLimit.PaintLimit;

		paintDirectlyLimit_AlphaChannel.UsePaintLimits = CalculateColorsInfoGlobal.PaintDirectlyTaskSettings.VertexColorAlphaChannelsLimit.UsePaintLimits;
		paintDirectlyLimit_AlphaChannel.LimitColorIfTheColorWasAlreadyOverTheLimit = CalculateColorsInfoGlobal.PaintDirectlyTaskSettings.VertexColorAlphaChannelsLimit.LimitColorIfTheColorWasAlreadyOverTheLimit;
		paintDirectlyLimit_AlphaChannel.PaintLimit = CalculateColorsInfoGlobal.PaintDirectlyTaskSettings.VertexColorAlphaChannelsLimit.PaintLimit;
		
	}


	// If Paint Job
	if (CalculateColorsInfoGlobal.IsPaintTask) {

		if (CalculateColorsInfoGlobal.PaintTaskSettings.OverrideVertexColorsToApplySettings.OverrideVertexColorsToApply) {

			if (IsValid(CalculateColorsInfoGlobal.PaintTaskSettings.OverrideVertexColorsToApplySettings.ObjectToRunOverrideVertexColorsInterface)) {

				if (CalculateColorsInfoGlobal.PaintTaskSettings.OverrideVertexColorsToApplySettings.ObjectToRunOverrideVertexColorsInterface->GetClass()->ImplementsInterface(UVertexPaintDetectionInterface::StaticClass())) {

					objectToOverrideInstigatedByPaintComponent = CalculateColorsInfoGlobal.TaskResult.GetAssociatedPaintComponent();
					objectToOverrideVertexColorsWith = CalculateColorsInfoGlobal.PaintTaskSettings.OverrideVertexColorsToApplySettings.ObjectToRunOverrideVertexColorsInterface;
				}
			}
		}
	}


	// If Paint Job with applyColorSettings, i.e. not color snippet or set mesh component colors
	if (CalculateColorsInfoGlobal.IsPaintTask && !CalculateColorsInfoGlobal.IsPaintDirectlyTask) {


		if (CalculateColorsInfoGlobal.ApplyColorSettings.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.ApplyVertexColorUsingPhysicsSurface) {

			for (int i = 0; i < CalculateColorsInfoGlobal.ApplyColorSettings.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.PhysicalSurfacesToApply.Num(); i++) {

				if (CalculateColorsInfoGlobal.ApplyColorSettings.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.PhysicalSurfacesToApply[i].PaintConditions.IfVertexHasLineOfSightTo.Num() > 0) {

					for (int j = 0; j < CalculateColorsInfoGlobal.ApplyColorSettings.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.PhysicalSurfacesToApply[i].PaintConditions.IfVertexHasLineOfSightTo.Num(); j++) {

						CalculateColorsInfoGlobal.ApplyColorSettings.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.PhysicalSurfacesToApply[i].PaintConditions.IfVertexHasLineOfSightTo[j] = SetLineOfSightPaintConditionParams(CalculateColorsInfoGlobal.ApplyColorSettings.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.PhysicalSurfacesToApply[i].PaintConditions.IfVertexHasLineOfSightTo[j]);
					}
				}
			}
		}

		if (CalculateColorsInfoGlobal.ApplyColorSettings.ApplyVertexColorSettings.ApplyColorsOnRedChannel.PaintConditions.IfVertexHasLineOfSightTo.Num() > 0) {

			for (int i = 0; i < CalculateColorsInfoGlobal.ApplyColorSettings.ApplyVertexColorSettings.ApplyColorsOnRedChannel.PaintConditions.IfVertexHasLineOfSightTo.Num(); i++) {

				CalculateColorsInfoGlobal.ApplyColorSettings.ApplyVertexColorSettings.ApplyColorsOnRedChannel.PaintConditions.IfVertexHasLineOfSightTo[i] = SetLineOfSightPaintConditionParams(CalculateColorsInfoGlobal.ApplyColorSettings.ApplyVertexColorSettings.ApplyColorsOnRedChannel.PaintConditions.IfVertexHasLineOfSightTo[i]);
			}
		}

		if (CalculateColorsInfoGlobal.ApplyColorSettings.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.PaintConditions.IfVertexHasLineOfSightTo.Num() > 0) {

			for (int i = 0; i < CalculateColorsInfoGlobal.ApplyColorSettings.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.PaintConditions.IfVertexHasLineOfSightTo.Num(); i++) {

				CalculateColorsInfoGlobal.ApplyColorSettings.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.PaintConditions.IfVertexHasLineOfSightTo[i] = SetLineOfSightPaintConditionParams(CalculateColorsInfoGlobal.ApplyColorSettings.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.PaintConditions.IfVertexHasLineOfSightTo[i]);
			}
		}

		if (CalculateColorsInfoGlobal.ApplyColorSettings.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.PaintConditions.IfVertexHasLineOfSightTo.Num() > 0) {

			for (int i = 0; i < CalculateColorsInfoGlobal.ApplyColorSettings.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.PaintConditions.IfVertexHasLineOfSightTo.Num(); i++) {

				CalculateColorsInfoGlobal.ApplyColorSettings.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.PaintConditions.IfVertexHasLineOfSightTo[i] = SetLineOfSightPaintConditionParams(CalculateColorsInfoGlobal.ApplyColorSettings.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.PaintConditions.IfVertexHasLineOfSightTo[i]);
			}
		}

		if (CalculateColorsInfoGlobal.ApplyColorSettings.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.PaintConditions.IfVertexHasLineOfSightTo.Num() > 0) {

			for (int i = 0; i < CalculateColorsInfoGlobal.ApplyColorSettings.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.PaintConditions.IfVertexHasLineOfSightTo.Num(); i++) {

				CalculateColorsInfoGlobal.ApplyColorSettings.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.PaintConditions.IfVertexHasLineOfSightTo[i] = SetLineOfSightPaintConditionParams(CalculateColorsInfoGlobal.ApplyColorSettings.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.PaintConditions.IfVertexHasLineOfSightTo[i]);
			}
		}
	}


	// If set to Compare Colors with a color snippet then sets the ColorArrayToCompareWith here so everything works just as expected when looping through the vertices
	if (CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.CompareMeshVertexColorsToColorSnippets.CompareWithColorsSnippetDataAssetInfo.Num() > 0) {

		for (FRVPDPColorSnippetDataAssetInfo compareWithDataAssetInfo : CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.CompareMeshVertexColorsToColorSnippets.CompareWithColorsSnippetDataAssetInfo) {

			if (compareWithDataAssetInfo.ColorSnippetDataAssetSnippetIsStoredIn) {

				compareColorSnippetMatchingPercent_AfterApplyingColors.Add(0);
				compareColorSnippetMatchingPercent_BeforeApplyingColors.Add(0);

				CompareColorSnippetColorArray.Add(
					compareWithDataAssetInfo.ColorSnippetDataAssetSnippetIsStoredIn->SnippetColorData.FindRef(compareWithDataAssetInfo.ColorSnippetID).ColorSnippetDataPerLOD[0].MeshVertexColorsPerLODArray);
			}
		}
	}


	if (CalculateColorsInfoGlobal.PropogateLOD0ToAllLODsUsingOctree)
		propogateToLODs_CompleteLODsBaseBounds.SetNum(CalculateColorsInfoGlobal.LodsToLoopThrough);


	CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD.SetNum(CalculateColorsInfoGlobal.LodsToLoopThrough);

	if (CalculateColorsInfoGlobal.IsDetectCombo)
		detectComboMeshVertexData.MeshDataPerLOD.SetNum(CalculateColorsInfoGlobal.LodsToLoopThrough);


	//-----------  LOD LOOP ----------- //

	for (int currentLOD = 0; currentLOD < CalculateColorsInfoGlobal.LodsToLoopThrough; currentLOD++) {

		if (!UVertexPaintFunctionLibrary::IsWorldValid(CalculateColorsInfoGlobal.TaskFundamentalSettings.GetTaskWorld())) return false;
		if (!IsMeshStillValid(CalculateColorsInfoGlobal)) return false;


		// Fundementals
		int totalAmountOfSections = 0;
		TArray<int32> sectionsToLoopThrough;
		TArray<FColor> defaultColorFromLOD;
		TArray<FColor> colorFromLOD;

		// Vertices
		int currentLODVertex = 0;
		int totalAmountOfVertsAtLOD = 0;
		TArray<FVector> positionsInWorldSpaceFromLOD;
		TArray<FVector> positionsInComponentLocalSpaceFromLOD;
		TArray<FVector> normalsFromLOD;
		TArray<int> vertexIndexesFromLOD;
		FPositionVertexBuffer* posVertBufferAtLOD = nullptr;
		FStaticMeshVertexBuffer* meshVertBufferAtLOD = nullptr;
		bool redColorChannelChanged = false;
		bool greenColorChannelChanged = false;
		bool blueColorChannelChanged = false;
		bool alphaColorChannelChanged = false;

		// Skeletal Mesh
		FSkeletalMeshLODRenderData* skelMeshLODRenderData = nullptr;
		FSkinWeightVertexBuffer* skinWeightVertexBuffer = nullptr;
		TArray<TArray<FColor>> boneIndexColorsBeforeApplyingColors;
		TArray<TArray<FColor>> boneIndexColorsAfterApplyingColors;
		FName previousBone = "";

		// Within Area
		int withinArea_AmountOfVerticesWithinArea = 0;
		TArray<FColor> withinArea_ColorFromLOD_BeforeApplyingColors;
		TArray<FColor> withinArea_ColorFromLOD_AfterApplyingColors;
		TArray<FVector> withinArea_PositionsInComponentLocalSpaceFromLOD;
		TArray<FVector> withinArea_NormalsFromLOD;
		TArray<int> withinArea_VertexIndexesFromLOD;

		// Paint Entire Mesh at Random Vertices
		FRandomStream paintEntireMesh_RandomStream;
		float paintEntireMesh_RandomAmountOfVerticesLeftToRandomize = 0;
		TMap<FVector, FColor> paintEntireMesh_RandomVerticesDoublettesPaintedAtLOD0;
		FRVPDPPropogateToLODsInfo lodPropogateToLODInfo;

		// Cloth
		TMap<UClothingAssetBase*, FRVPDPVertexChannelsChaosClothPhysicsSettings> cloth_PhysicsSettings;
		TMap<int16, FLinearColor> cloth_AverageColor;


		if (CalculateColorsInfoGlobal.IsStaticMeshTask) {


			if (!GetStaticMeshLODInfo(CalculateColorsInfoGlobal.GetVertexPaintStaticMeshComponent(), shouldLoopThroughAllVerticesOnLOD, currentLOD, sectionsToLoopThrough, totalAmountOfSections, posVertBufferAtLOD, meshVertBufferAtLOD, totalAmountOfVertsAtLOD, CalculateColorsInfoGlobal.TaskResult.MeshVertexData, colorFromLOD))
				return false;
		}

		if (CalculateColorsInfoGlobal.IsSkeletalMeshTask) {

			if (!GetSkeletalMeshLODInfo(shouldLoopThroughAllVerticesOnLOD, skeletalMeshRenderData, skelMeshLODRenderData, currentLOD, skinWeightVertexBuffer, sectionsToLoopThrough, totalAmountOfSections, totalAmountOfVertsAtLOD, CalculateColorsInfoGlobal.TaskResult.MeshVertexData, colorFromLOD, currentLODVertex, boneIndexColorsAfterApplyingColors)) return false;


			if (CalculateColorsInfoGlobal.IsDetectCombo)
				boneIndexColorsBeforeApplyingColors.SetNum(boneIndexColorsAfterApplyingColors.Num());
		}


#if ENGINE_MAJOR_VERSION == 5


		if (CalculateColorsInfoGlobal.IsDynamicMeshTask) {

			if (UDynamicMeshComponent* dynamicMeshComp = CalculateColorsInfoGlobal.GetVertexPaintDynamicMeshComponent()) {

				if (UDynamicMesh* dynamicMesh = dynamicMeshComp->GetDynamicMesh()) {

					if (!GetDynamicMeshLODInfo(dynamicMesh, dynamicMesh_3, sectionsToLoopThrough, totalAmountOfVertsAtLOD, colorFromLOD, defaultColorFromLOD, CalculateColorsInfoGlobal.DynamicMeshComponentVertexColors))
						return false;

					totalAmountOfSections = sectionsToLoopThrough.Num();
				}
			}
		}

		if (CalculateColorsInfoGlobal.IsGeometryCollectionTask) {

			if (!GetGeometryCollectionMeshLODInfo(CalculateColorsInfoGlobal.GetVertexPaintGeometryCollectionData(), sectionsToLoopThrough, totalAmountOfVertsAtLOD, colorFromLOD, defaultColorFromLOD, CalculateColorsInfoGlobal.GeometryCollectionComponentVertexColors))
				return false;

			totalAmountOfSections = sectionsToLoopThrough.Num();
		}


#endif


		if (totalAmountOfVertsAtLOD <= 0) {

			CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Calculate Colors Task - Fail - Vertex Amount we got for Actor: %s and Mesh: %s on LOD: %i is 0!"), *CalculateColorsInfoGlobal.VertexPaintActorName, *CalculateColorsInfoGlobal.VertexPaintComponentName, currentLOD), FColor::Red));


			if (currentLOD == 0)
				return false;
			else
				continue;
		}


		// If Static or Skeletal Mesh, then should've filled the colors array by now. Dynamic Meshes and Geometry Collections has to convert while looping the vertices and fill them then. 
		if (CalculateColorsInfoGlobal.IsStaticMeshTask || CalculateColorsInfoGlobal.IsSkeletalMeshTask) {

			defaultColorFromLOD = colorFromLOD;

			if (colorFromLOD.Num() == 0) {

				CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Calculate Colors Task - Fail - Couldn't get colors for Actor: %s and Mesh: %s on LOD: %i!"), *CalculateColorsInfoGlobal.VertexPaintActorName, *CalculateColorsInfoGlobal.VertexPaintComponentName, currentLOD), FColor::Red));


				// If couldn't even get the colors on LOD 0 then something has gone wrong, then we shouldn't be in this function in the first place
				if (currentLOD == 0)
					return false;
				else
					continue;
			}

			if (colorFromLOD.Num() != totalAmountOfVertsAtLOD) {

				CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Calculate Colors Task - Fail - Colors Vertex Amount we got for Actor: %s and Mesh: %s on LOD: %i wasn't the same amount as the Position Vertex Amount. Most likely we're somehow painting on a Mesh component that got it's mesh changed but still has the old color buffer. "), *CalculateColorsInfoGlobal.VertexPaintActorName, *CalculateColorsInfoGlobal.VertexPaintComponentName, currentLOD), FColor::Red));


				// If colors doesn't match the amount on the LOD then something has gone wrong
				if (currentLOD == 0)
					return false;
				else
					continue;
			}
		}


		CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD[currentLOD].AmountOfVerticesAtLOD = totalAmountOfVertsAtLOD;
		normalsFromLOD.SetNum(totalAmountOfVertsAtLOD);
		vertexIndexesFromLOD.SetNum(totalAmountOfVertsAtLOD);
		positionsInWorldSpaceFromLOD.SetNum(totalAmountOfVertsAtLOD);
		positionsInComponentLocalSpaceFromLOD.SetNum(totalAmountOfVertsAtLOD);


		if (CalculateColorsInfoGlobal.IsPaintTask) {

			if (CalculateColorsInfoGlobal.NewColorVertexBuffersPerLOD.IsValidIndex(currentLOD)) {

				// If haven't been init earlier
				if (!CalculateColorsInfoGlobal.NewColorVertexBuffersPerLOD[currentLOD]) {

					CalculateColorsInfoGlobal.NewColorVertexBuffersPerLOD[currentLOD] = new FColorVertexBuffer();

					// CalculateColorsInfoGlobal.NewColorVertexBuffersPerLOD[currentLOD]->Init(meshLODTotalAmountOfVerts, true);

					// Will have to InitFromColorArray in case we Skip a Section, which can happen if Paint using physics surface fails to get any colors, and there's no reason for us to loop through all the vertices. The Tasks seem to take the same amount of time with InitFromColorArray vs Init anyways so. 
					CalculateColorsInfoGlobal.NewColorVertexBuffersPerLOD[currentLOD]->InitFromColorArray(colorFromLOD);
				}
			}
		}


		if (CalculateColorsInfoGlobal.IsPaintEntireMeshTask()) {

			// If set to Randomize over entire mesh, or only within AoE
			if ((CalculateColorsInfoGlobal.PaintOnEntireMeshSettings.PaintOnRandomVerticesSettings.OnlyRandomizeWithinAreaOfEffectAtLocation && CalculateColorsInfoGlobal.PaintOnEntireMeshSettings.PaintOnRandomVerticesSettings.OnlyRandomizeWithinAreaOfEffectAtLocation_ProbabilityFactor > 0) || (CalculateColorsInfoGlobal.PaintOnEntireMeshSettings.PaintOnRandomVerticesSettings.PaintAtRandomVerticesSpreadOutOverTheEntireMesh && CalculateColorsInfoGlobal.PaintOnEntireMeshSettings.PaintOnRandomVerticesSettings.PaintAtRandomVerticesSpreadOutOverTheEntireMesh_PercentToPaint > 0)) {


				GetPaintEntireMeshRandomLODInfo(CalculateColorsInfoGlobal.PaintOnEntireMeshSettings.PaintOnRandomVerticesSettings, CalculateColorsInfoGlobal.PropogateLOD0ToAllLODsUsingOctree, currentLOD, totalAmountOfVertsAtLOD, paintEntireMesh_RandomStream, paintEntireMesh_RandomAmountOfVerticesLeftToRandomize);

				if (currentLOD == 0) {

					CalculateColorsInfoGlobal.PaintOnEntireMeshSettings.PaintOnRandomVerticesSettings.RandomSeedsUsedInPaintJob = paintEntireMesh_RandomStream;
				}

				CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Calculate Colors Task - Paint Entire Mesh Set to Randomize %i Amount of Vertices over the Mesh on LOD: %i"), static_cast<int>(paintEntireMesh_RandomAmountOfVerticesLeftToRandomize), currentLOD), FColor::Cyan));
			}
		}


		if (currentLOD == 0) {

			// If should compare colors and has filled an array to compare with, either by directly setting it or getting it from a color snippet
			if (CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.CompareMeshVertexColorsToColorArray.ColorArrayToCompareWith.Num() == colorFromLOD.Num())
				compareColorsToArray = true;


			for (int i = 0; i < CompareColorSnippetColorArray.Num(); i++) {
				
				if (CompareColorSnippetColorArray[i].Num() == colorFromLOD.Num()) {

					compareColorsToColorSnippet = true;
					compareColorsToSnippet_AmountOfVerticesConsidered_AfterApplyingColors.Init(0, CompareColorSnippetColorArray.Num());

					if (CalculateColorsInfoGlobal.IsDetectCombo)
						compareColorsToSnippet_AmountOfVerticesConsidered_BeforeApplyingColors.Init(0, CompareColorSnippetColorArray.Num());

					break;
				}
			}
		}

		// If higher than LOD 0 and set to propogate to lod from stored data then we retrieve and cache it for use in the vertex loop
		else if (CalculateColorsInfoGlobal.PropogateLOD0ToAllLODsFromStoredData) {
			
			if (UVertexPaintOptimizationDataAsset* optimizationDataAsset = UVertexPaintFunctionLibrary::GetOptimizationDataAsset(CalculateColorsInfoGlobal.TaskFundamentalSettings.TaskWorld.Get())) {

				optimizationDataAsset->GetRegisteredMeshPropogateToLODInfoAtLOD(CalculateColorsInfoGlobal.VertexPaintSourceMesh, currentLOD, lodPropogateToLODInfo);
			}
		}


		if (totalAmountOfSections < sectionsToLoopThrough.Num()) {

			CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Calculate Colors Task - LOD: %i  -  Total Amount of Sections: %i  -  Amount of Sections to Loop Through: %i.  We're not looping through all to Finish the task faster!"), currentLOD, totalAmountOfSections, sectionsToLoopThrough.Num()), FColor::Cyan));
		}

		else {

			CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Calculate Colors Task - LOD: %i  -  Total Amount of Sections: %i  -  Amount of Sections to Loop Through: %i. "), currentLOD, totalAmountOfSections, sectionsToLoopThrough.Num()), FColor::Cyan));
		}


		//-----------  SECTIONS LOOP ----------- //

		for (int32 currentSection : sectionsToLoopThrough) {

			if (!UVertexPaintFunctionLibrary::IsWorldValid(CalculateColorsInfoGlobal.TaskFundamentalSettings.GetTaskWorld())) return false;
			if (!IsMeshStillValid(CalculateColorsInfoGlobal)) return false;


			CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Calculate Colors Task - Looping through Section: %i"), currentSection), FColor::Cyan));


			// Fundemental
			bool shouldApplyVertexColors = true;
			int sectionMaxAmountOfVerts = 0;

			// Painting with RGBA
			bool shouldApplyColorOnRedChannel = false;
			bool shouldApplyColorOnGreenChannel = false;
			bool shouldApplyColorOnBlueChannel = false;
			bool shouldApplyColorOnAlphaChannel = false;

			// Paint Job
			TMap<int, TArray<FRVPDPPhysicsSurfaceToApplySettings>> physicsSurfacePaintSettingsPerChannel;
			FRVPDPApplyColorSetting paintOnMeshColorSettingsForSection = CalculateColorsInfoGlobal.ApplyColorSettings; // Resets apply vertex color settings after each section to make sure we're basing this of the original and not something from the previous section

			// Skeletal Mesh
			FSkelMeshRenderSection* skeletalMeshRenderSection = nullptr;
			FRVPDPSkeletalMeshSectionInfo skeletalMeshSectionInfo;
			TArray<int32> skeletalMeshBoneFirstVertices;
			TArray<FRVPDPSkeletalMeshBoneVertexInfo> skeletalMeshBoneVertexInfos;
			int boneSkipping_PreviousIndexSkippedTo = 0;
			int boneSkipping_SectionVertexToSkipTo = -1;
			int boneSkipping_LodVertexToSkipTo = -1;

			// Materials
			UMaterialInterface* materialAtSection = nullptr;
			UMaterialInterface* materialToGetSurfacesFrom = nullptr;
			TArray<TEnumAsByte<EPhysicalSurface>> registeredPhysicsSurfacesAtMaterial;

			// Cloth
			bool shouldAffectPhysicsOnSectionsCloth = false;
			float clothSectionTotalRedVertexColor = 0;
			float clothSectionTotalGreenVertexColor = 0;
			float clothSectionTotalBlueVertexColor = 0;
			float clothSectionTotalAlphaVertexColor = 0;

#if ENGINE_MAJOR_VERSION == 4

			TArray<FVector> currentClothSectionVertexPositions;
			TArray<FVector> currentClothSectionVertexNormals;

#elif ENGINE_MAJOR_VERSION == 5

			TArray<FVector3f> currentClothSectionVertexPositions;
			TArray<FVector3f> currentClothSectionVertexNormals;

#endif


			// One for each Color channel that May have physics surface registered. 
			TArray<FRVPDPAmountOfColorsOfEachChannelPhysicsResults> physicsSurfaceResultForSection_BeforeAddingColors;
			physicsSurfaceResultForSection_BeforeAddingColors.SetNum(4);

			TArray<FRVPDPAmountOfColorsOfEachChannelPhysicsResults> physicsSurfaceResultForSection_AfterApplyingColors;
			physicsSurfaceResultForSection_AfterApplyingColors.SetNum(4);

			// If Detect/Paint Within Area and want to get physics surface result of each channel
			TArray<FRVPDPAmountOfColorsOfEachChannelPhysicsResults> physicsSurfaceResultForSection_WithinArea_BeforeApplyingColors;
			TArray<FRVPDPAmountOfColorsOfEachChannelPhysicsResults> physicsSurfaceResultForSection_WithinArea_AfterApplyingColors;

			if (CalculateColorsInfoGlobal.WithinAreaSettings.ComponentsToCheckIfIsWithin.Num() > 0 && CalculateColorsInfoGlobal.WithinAreaSettings.IncludeAmountOfColorsOfEachChannelWithinArea.IncludePhysicsSurfaceResultOfEachChannel) {

				physicsSurfaceResultForSection_WithinArea_BeforeApplyingColors.SetNum(4);
				physicsSurfaceResultForSection_WithinArea_AfterApplyingColors.SetNum(4);
			}


			if (CalculateColorsInfoGlobal.IsStaticMeshTask) {

				GetStaticMeshComponentSectionInfo(CalculateColorsInfoGlobal.GetVertexPaintStaticMeshComponent(), currentLOD, currentSection, sectionMaxAmountOfVerts, currentLODVertex, materialAtSection);
			}

			else if (CalculateColorsInfoGlobal.IsSkeletalMeshTask) {

				if (!GetSkeletalMeshComponentSectionInfo(CalculateColorsInfoGlobal.GetVertexPaintSkelComponent(), CalculateColorsInfoGlobal.PaintTaskSettings.AffectClothPhysics, currentLOD, currentSection, skeletalMeshRenderData, skelMeshLODRenderData, cloth_VertexPositions, cloth_VertexNormals, cloth_PhysicsSettings, cloth_ClothingAssets, sectionMaxAmountOfVerts, currentLODVertex, materialAtSection, skeletalMeshRenderSection, shouldAffectPhysicsOnSectionsCloth, currentClothSectionVertexPositions, currentClothSectionVertexNormals)) {

					return false;
				} 


				// If set to only paint on specific skeletal mesh bones 
				if (!shouldLoopThroughAllVerticesOnLOD && CalculateColorsInfoGlobal.SpecificSkeletalMeshBonesToLoopThrough.Num() > 0 && skeletalMeshBoneInfo.IsValidIndex(currentLOD) &&  skeletalMeshBoneInfo[currentLOD].SkeletalMeshSectionInfo.Contains(currentSection)) {


					skeletalMeshSectionInfo = skeletalMeshBoneInfo[currentLOD].SkeletalMeshSectionInfo.FindRef(currentSection);

					// If nothing is added at all then we skip
					if (skeletalMeshSectionInfo.SkeletalMeshBoneVertexInfo.Num() == 0) {

						continue;
					}

					// Or if no bone first vertices is added then we skip
					else {

						skeletalMeshSectionInfo.SkeletalMeshBoneVertexInfo.GenerateKeyArray(skeletalMeshBoneFirstVertices);
						skeletalMeshSectionInfo.SkeletalMeshBoneVertexInfo.GenerateValueArray(skeletalMeshBoneVertexInfos);

						if (skeletalMeshBoneFirstVertices.Num() == 0) continue;
					}
				}
			}

#if ENGINE_MAJOR_VERSION == 5

			else if (CalculateColorsInfoGlobal.IsDynamicMeshTask) {

				GetDynamicMeshComponentSectionInfo(CalculateColorsInfoGlobal.GetVertexPaintDynamicMeshComponent(), totalAmountOfVertsAtLOD, currentSection, sectionMaxAmountOfVerts, currentLODVertex, materialAtSection);
			}

			else if (CalculateColorsInfoGlobal.IsGeometryCollectionTask) {

				GetGeometryCollectionMeshComponentSectionInfo(CalculateColorsInfoGlobal.GetVertexPaintGeometryCollectionData(), CalculateColorsInfoGlobal.GetVertexPaintGeometryCollectionComponent(), currentSection, sectionMaxAmountOfVerts, currentLODVertex, materialAtSection);
			}

#endif


			if (materialAtSection) {

				GetMaterialToGetSurfacesFrom(CalculateColorsInfoGlobal.VertexPaintMaterialDataAsset, CalculateColorsInfoGlobal.TaskFundamentalSettings, materialAtSection, materialToGetSurfacesFrom, registeredPhysicsSurfacesAtMaterial);
			}

			
			// If paint job that uses apply colors on channels, whether we get them using physics surface or set them manually. 
			if (CalculateColorsInfoGlobal.IsPaintTask && !CalculateColorsInfoGlobal.IsPaintDirectlyTask) {
				

				// If set to Paint Using Physics Surface, and we was successfull at getting the Material, then we Get the Colors to Apply based on what's Registered in the Editor Widget. 
				if (paintOnMeshColorSettingsForSection.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.ApplyVertexColorUsingPhysicsSurface) {


					if (GetPaintOnMeshColorSettingsFromPhysicsSurface(CalculateColorsInfoGlobal.TaskFundamentalSettings, materialToGetSurfacesFrom, paintOnMeshColorSettingsForSection, physicsSurfacePaintSettingsPerChannel)) {

						// 
					}
 
					else {

						shouldApplyVertexColors = false;
					}
				}

				else {

					// Always runs this even if applying with physics surface in case we want to fall back to apply with RGBA if failed with physics surface. 
					GetShouldApplyColorsOnChannels(paintOnMeshColorSettingsForSection, shouldApplyColorOnRedChannel, shouldApplyColorOnGreenChannel, shouldApplyColorOnBlueChannel, shouldApplyColorOnAlphaChannel);

					// Adjusts this if painting with RGBA as well, if for instance running this a Within Color Range Paint Condition where you require Sand or some other physics surface to be within a range, then we need to adjust this to whatever channel Sand is on. Otherwise if painting with RGBA you could only use RGBA condition which is undesirable. 
					SetWithinColorRangePaintConditionChannelsIfUsingItWithPhysicsSurface(paintOnMeshColorSettingsForSection.ApplyVertexColorSettings.ApplyColorsOnRedChannel.PaintConditions, materialToGetSurfacesFrom);

					SetWithinColorRangePaintConditionChannelsIfUsingItWithPhysicsSurface(paintOnMeshColorSettingsForSection.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.PaintConditions, materialToGetSurfacesFrom);

					SetWithinColorRangePaintConditionChannelsIfUsingItWithPhysicsSurface(paintOnMeshColorSettingsForSection.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.PaintConditions, materialToGetSurfacesFrom);

					SetWithinColorRangePaintConditionChannelsIfUsingItWithPhysicsSurface(paintOnMeshColorSettingsForSection.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.PaintConditions, materialToGetSurfacesFrom);
				}


				// If Regular Paint Job, i.e. not a color snippet and shouldApplyVertexColors is false, maybe because we didn't get any colors to apply from physics surface, and there is no need for us to loop through all of the vertices then we can skip this section.
				if (!shouldApplyVertexColors && !shouldLoopThroughAllVerticesOnLOD) {

					continue;
				}
			}



			bool hasPhysicsSurfacePaintCondition = false;
			bool hasRedChannelPaintCondition = false;
			bool hasGreenChannelPaintCondition = false;
			bool hasBlueChannelPaintCondition = false;
			bool hasAlphaChannelPaintCondition = false;

			int amountOfVertexNormalWithinDotToDirectionConditions = 0;
			int amountOfOnlyAffectVerticesWithDirectionToActorOrLocationConditions = 0;
			int amountOfOnlyAffectVerticesWithinDirectionFromActorOrLocation = 0;
			int amountOfIfVertexIsAboveOrBelowWorldZ = 0;
			int amountOfIfVertexColorWithinRange = 0;
			int amountOfIfVertexHasLineOfSightTo = 0;
			int amountOfIfVertexIsOnBone = 0;
			int amountOfIfVertexIsOnMaterial = 0;

			const bool hasAnyPaintConditions = UVertexPaintFunctionLibrary::IsThereAnyPaintConditions(paintOnMeshColorSettingsForSection.ApplyVertexColorSettings, hasPhysicsSurfacePaintCondition, hasRedChannelPaintCondition, hasGreenChannelPaintCondition, hasBlueChannelPaintCondition, hasAlphaChannelPaintCondition, amountOfVertexNormalWithinDotToDirectionConditions, amountOfOnlyAffectVerticesWithDirectionToActorOrLocationConditions, amountOfOnlyAffectVerticesWithinDirectionFromActorOrLocation, amountOfIfVertexIsAboveOrBelowWorldZ, amountOfIfVertexColorWithinRange, amountOfIfVertexHasLineOfSightTo, amountOfIfVertexIsOnBone, amountOfIfVertexIsOnMaterial);


			bool containsPhysicsSurfaceOnRedChannel = false;
			bool containsPhysicsSurfaceOnGreenChannel = false;
			bool containsPhysicsSurfaceOnBlueChannel = false;
			bool containsPhysicsSurfaceOnAlphaChannel = false;

			bool withinArea_ContainsPhysicsSurfaceOnRedChannel = false;
			bool withinArea_ContainsPhysicsSurfaceOnGreenChannel = false;
			bool withinArea_ContainsPhysicsSurfaceOnBlueChannel = false;
			bool withinArea_ContainsPhysicsSurfaceOnAlphaChannel = false;


			if (CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeAmountOfPaintedColorsOfEachChannel.IncludeOnlyIfPhysicsSurfacesIsRegisteredToAnyVertexChannel.Num() > 0) {

				GetChannelsThatContainsPhysicsSurface(CalculateColorsInfoGlobal.TaskFundamentalSettings, CalculateColorsInfoGlobal.WithinAreaSettings, registeredPhysicsSurfacesAtMaterial, containsPhysicsSurfaceOnRedChannel, containsPhysicsSurfaceOnGreenChannel, containsPhysicsSurfaceOnBlueChannel, containsPhysicsSurfaceOnAlphaChannel);
			}


			if (CalculateColorsInfoGlobal.WithinAreaSettings.IncludeAmountOfColorsOfEachChannelWithinArea.IncludeOnlyIfPhysicsSurfacesIsRegisteredToAnyVertexChannel.Num() > 0 && CalculateColorsInfoGlobal.WithinAreaSettings.ComponentsToCheckIfIsWithin.Num() > 0) {

				GetChannelsThatContainsPhysicsSurface(CalculateColorsInfoGlobal.TaskFundamentalSettings, CalculateColorsInfoGlobal.WithinAreaSettings, registeredPhysicsSurfacesAtMaterial, withinArea_ContainsPhysicsSurfaceOnRedChannel, withinArea_ContainsPhysicsSurfaceOnGreenChannel, withinArea_ContainsPhysicsSurfaceOnBlueChannel, withinArea_ContainsPhysicsSurfaceOnAlphaChannel);
			}
			

			//-----------  VERTEX LOOP ----------- //

			for (uint32 currentSectionVertex = 0; currentSectionVertex < static_cast<uint32>(sectionMaxAmountOfVerts); currentSectionVertex++) {

				if (!UVertexPaintFunctionLibrary::IsWorldValid(CalculateColorsInfoGlobal.TaskFundamentalSettings.GetTaskWorld())) return false;
				if (!IsMeshStillValid(CalculateColorsInfoGlobal)) return false;
				if (!colorFromLOD.IsValidIndex(currentLODVertex)) return false;

				// currentLODVertex gets ++ at the end of the loop


				if (boneSkipping_LodVertexToSkipTo > 0) {

					if (boneSkipping_LodVertexToSkipTo != currentLODVertex) {

						currentLODVertex = boneSkipping_LodVertexToSkipTo;
					}

					boneSkipping_LodVertexToSkipTo = -1;
				}

				if (boneSkipping_SectionVertexToSkipTo >= 0) {

					if (boneSkipping_SectionVertexToSkipTo < sectionMaxAmountOfVerts) {

						currentSectionVertex = boneSkipping_SectionVertexToSkipTo;
					}

					boneSkipping_SectionVertexToSkipTo = -1;
				}


				// Fundementals
				bool shouldApplyColorOnAnyChannel = false;
				bool haveRunOverrideInterface = false;
				bool gotVertexNormal = false;
				bool isVertexOnCloth = false;
				FLinearColor colorFromLODAsLinear = UVertexPaintFunctionLibrary::ReliableFColorToFLinearColor(colorFromLOD[currentLODVertex]);
				FVector currentVertexActorSpace = FVector(ForceInitToZero);
				FVector currentVertexComponentSpace = FVector(ForceInitToZero);
				FVector currentVertexWorldSpace = FVector(ForceInitToZero);
				FLinearColor colorFromLODAsLinear_BeforeApplyingColors;

				// Skeletal Mesh
				uint32 currentBoneIndex = 0;
				FName currentBoneName = "";
				float vertexTotalBoneWeight = 0;

				// Closest Vertex Data / Paint at Location
				float paintAtLocation_DistanceFromVertexToHitLocation = 0;
				float getClosestVertexData_DistanceFromVertexToHitLocation = 0;

				// Within Area
				bool withinArea_VertexWasWithinArea = false;

				// 1 so if we where unable to get the vertex normal, and can't use it to properly check what this is used for, the check will just pass. 
				float paintAtLocation_CurrentVertexNormalToHitNormalDot = 1;
				float getClosestVertexData_CurrentVertexNormalToHitNormalDot = 1;

				// Paint Entire Mesh at Random Vertices
				bool paintEntireMesh_PaintCurrentVertex = false;

				// Propogating to LODs 
				FExtendedPaintedVertex paintedVertex;


				vertexIndexesFromLOD[currentLODVertex] = currentLODVertex;


#if ENGINE_MAJOR_VERSION == 5

				// Dynamic Mesh
				UE::Geometry::FVertexInfo dynamicMesh_VertexInfo;
				int dynamicMesh_MaxVertID = 0;
				int dynamicMesh_VerticesBufferMax = 0;

#endif


				if (CalculateColorsInfoGlobal.IsStaticMeshTask) {

					if (!GetStaticMeshComponentVertexInfo(posVertBufferAtLOD, meshVertBufferAtLOD, currentLODVertex, positionsInComponentLocalSpaceFromLOD[currentLODVertex], currentVertexActorSpace, normalsFromLOD[currentLODVertex], gotVertexNormal)) {

						return false;
					}
				}

				else if (CalculateColorsInfoGlobal.IsSkeletalMeshTask) {

					if (!GetSkeletalMeshComponentVertexInfo(CalculateColorsInfoGlobal.SkeletalMeshRefToLocals, CalculateColorsInfoGlobal.SkeletalMeshBonesNames, CalculateColorsInfoGlobal.TaskFundamentalSettings.DebugSettings.GameThreadSpecificDebugSymbols.DrawClothVertexPositionDebugPoint, CalculateColorsInfoGlobal.TaskFundamentalSettings.DebugSettings.GameThreadSpecificDebugSymbols.DrawGameThreadSpecificDebugSymbolsDuration, skeletalMeshRenderData, skelMeshLODRenderData, skeletalMeshRenderSection, skinWeightVertexBuffer, currentClothSectionVertexPositions, currentClothSectionVertexNormals, cloth_BoneQuaternionsInComponentSpace, cloth_BoneLocationInComponentSpace, currentLOD, currentSection, currentSectionVertex, currentLODVertex, skeletalMeshPredictedLOD, positionsInComponentLocalSpaceFromLOD[currentLODVertex], currentVertexActorSpace, normalsFromLOD[currentLODVertex], gotVertexNormal, isVertexOnCloth, currentBoneIndex, currentBoneName, vertexTotalBoneWeight)) {

						return false;
					}
				}
				
#if ENGINE_MAJOR_VERSION == 5

				else if (CalculateColorsInfoGlobal.IsDynamicMeshTask) {


					bool skipDynamicMeshVertex = false;
					if (!GetDynamicMeshComponentVertexInfo(dynamicMesh_3, totalAmountOfVertsAtLOD, currentLODVertex, colorFromLOD[currentLODVertex], positionsInComponentLocalSpaceFromLOD[currentLODVertex], currentVertexActorSpace, normalsFromLOD[currentLODVertex], gotVertexNormal, dynamicMesh_VertexInfo, dynamicMesh_MaxVertID, dynamicMesh_VerticesBufferMax, skipDynamicMeshVertex)) return false;


					if (skipDynamicMeshVertex) {

						currentLODVertex++;
						continue;
					}


					CalculateColorsInfoGlobal.DynamicMeshComponentVertexColors[currentLODVertex] = colorFromLOD[currentLODVertex];

					// Since we can't get all of the colors without looping through them, we fill the defaultColorFromLOD here while looping through the verts
					defaultColorFromLOD[currentLODVertex] = colorFromLOD[currentLODVertex];
				}
				
#if ENGINE_MINOR_VERSION >= 3

				else if (CalculateColorsInfoGlobal.IsGeometryCollectionTask) {


					if (!GetGeometryCollectionComponentVertexInfo(CalculateColorsInfoGlobal.GetVertexPaintGeometryCollectionData(), currentLODVertex, colorFromLOD[currentLODVertex], positionsInComponentLocalSpaceFromLOD[currentLODVertex], currentVertexActorSpace, normalsFromLOD[currentLODVertex], gotVertexNormal)) return false;


					// Since we can't get all of the colors without looping through them, we fill the defaultColorFromLOD here while looping through the verts
					defaultColorFromLOD[currentLODVertex] = colorFromLOD[currentLODVertex];


					// Geometry Collection TManagedArray<FLinearColor> requires FLinearColor, so to avoid looping through all the colors on game thread to build a FLinearColor array for geo collection comps we update one here. 
					CalculateColorsInfoGlobal.GeometryCollectionComponentVertexColors[currentLODVertex] = UVertexPaintFunctionLibrary::ReliableFColorToFLinearColor(colorFromLOD[currentLODVertex]);
				}
#endif
#endif


				// If set to only paint on specific skeletal mesh bones, and when we now have the current bone we can safely do checks if we should skip to another bone
				if (!shouldLoopThroughAllVerticesOnLOD && CalculateColorsInfoGlobal.SpecificSkeletalMeshBonesToLoopThrough.Num() > 0 && skeletalMeshSectionInfo.SkeletalMeshBoneVertexInfo.Num() > 0 && skeletalMeshBoneFirstVertices.Num() > 0 && skeletalMeshBoneVertexInfos.Num() > 0) {

					// If on a new bone, and if it's not a relevant one
					if (previousBone != currentBoneName && !CalculateColorsInfoGlobal.SpecificSkeletalMeshBonesToLoopThrough.Contains(currentBoneName)) {

						const int32 lastFirstBoneIndex = skeletalMeshBoneFirstVertices.Last();

						// If we're past the point of the last bone vertex, then we can skip to the next section as there is nothing more for us here. 
						if (currentLODVertex > skeletalMeshSectionInfo.SkeletalMeshBoneVertexInfo.FindRef(lastFirstBoneIndex).BoneFirstVertex) {

							break;
						}

						// Otherwise looks up the next closest vertex to skip to
						else {


							int32 closestBoneFirstIndexToJumpTo = 99999999;
							bool gotVertexToSkipTo = false;


							// Starts the look up from the previous index (0 if first time) so it goes faster. 
							for (int i = boneSkipping_PreviousIndexSkippedTo; i < skeletalMeshBoneFirstVertices.Num(); i++) {

								// If higher Lod vertex, i.e. after the current one, and closer index
								if (skeletalMeshBoneFirstVertices[i] > currentLODVertex && skeletalMeshBoneFirstVertices[i] < closestBoneFirstIndexToJumpTo) {

									// If relevant bone
									if (CalculateColorsInfoGlobal.SpecificSkeletalMeshBonesToLoopThrough.Contains(skeletalMeshBoneVertexInfos[i].BoneName)) {

										closestBoneFirstIndexToJumpTo = skeletalMeshBoneFirstVertices[i];
										boneSkipping_PreviousIndexSkippedTo = i;
										gotVertexToSkipTo = true;
									}
								}
							}


							// If got vertex to skips to then sets cached local variables and continues
							if (gotVertexToSkipTo) {

								FRVPDPSkeletalMeshBoneVertexInfo boneVertexInfoToSkipTo = skeletalMeshSectionInfo.SkeletalMeshBoneVertexInfo.FindRef(closestBoneFirstIndexToJumpTo);
								boneSkipping_LodVertexToSkipTo = boneVertexInfoToSkipTo.BoneFirstVertex;
								boneSkipping_SectionVertexToSkipTo = boneVertexInfoToSkipTo.BoneFirstSectionVertex;
								// boneVertexInfoToSkipTo.BoneName

								// UE_LOG(LogTemp, Warning, TEXT("currentSection: %i  -  currentBoneName: %s  skipping to bone: %s  Skipping From Vertex: %i  -  To Vertex: %i  -  From Section Vertex: %i  -  to Section vertex: %i"), currentSection, *currentBoneName.ToString(), *boneVertexInfoToSkipTo.BoneName.ToString(), currentLODVertex, boneSkipping_LodVertexToSkipTo, currentSectionVertex, boneSkipping_SectionVertexToSkipTo);

								continue;
							}

							// Otherwise breaks the section vertex loop so we're back at the Section loop. 
							else {

								// UE_LOG(LogTemp, Warning, TEXT("currentSection: %i  -  currentBoneName: %s  DIDNT GET vertex to skip to: %i  currentSectionVertex: %i"), currentSection, *currentBoneName.ToString(), currentLODVertex, currentSectionVertex);

								break;
							}
						}
					}
				}


				previousBone = currentBoneName;


				// For Dynamic Mesh and Geometry Collection we don't get the color until this point so sets this here
				colorFromLODAsLinear_BeforeApplyingColors = colorFromLODAsLinear;

				currentVertexComponentSpace = positionsInComponentLocalSpaceFromLOD[currentLODVertex];
				currentVertexWorldSpace = UKismetMathLibrary::TransformLocation(CalculateColorsInfoGlobal.VertexPaintActorTransform, currentVertexActorSpace);
				positionsInWorldSpaceFromLOD[currentLODVertex] = currentVertexWorldSpace;

				if (gotVertexNormal) {

					normalsFromLOD[currentLODVertex] = UKismetMathLibrary::TransformDirection(CalculateColorsInfoGlobal.VertexPaintComponentTransform, normalsFromLOD[currentLODVertex]);
				}


				// Caches the vertex normal to hit normal dots so it's more comfortable to use later on
				if (CalculateColorsInfoGlobal.IsPaintAtLocationTask()) {

					paintAtLocation_DistanceFromVertexToHitLocation = (paintAtLocation_HitLocationInWorldSpace - currentVertexWorldSpace).Size();

					// If didn't get normal then this doesn't get set, so any checks using it will just get passed since it should be at default value. Dynamic Mesh Component for instance is not 100% when getting the normal so we don't want it not to get painted because of this 
					if (gotVertexNormal) {

						paintAtLocation_CurrentVertexNormalToHitNormalDot = FMath::Clamp(FVector::DotProduct(CalculateColorsInfoGlobal.PaintAtLocationSettings.HitFundementals.HitNormal, normalsFromLOD[currentLODVertex]), -1.0f, 1.0f);
					}
				}


				if (GetClosestVertexData) {

					getClosestVertexData_DistanceFromVertexToHitLocation = (getClosestVertexData_HitLocationInWorldSpace - currentVertexWorldSpace).Size();

					if (gotVertexNormal) {

						getClosestVertexData_CurrentVertexNormalToHitNormalDot = FMath::Clamp(FVector::DotProduct(CalculateColorsInfoGlobal.GetClosestVertexDataSettings.HitFundementals.HitNormal, normalsFromLOD[currentLODVertex]), -1.0f, 1.0f);
					}
				}


				if (CalculateColorsInfoGlobal.PropogateLOD0ToAllLODsUsingOctree && currentLOD == 0 && CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD.Num() > 1) {

					paintedVertex.Position = positionsInComponentLocalSpaceFromLOD[currentLODVertex];
					paintedVertex.Normal = normalsFromLOD[currentLODVertex];
					paintedVertex.VertexIndex = currentLODVertex;
				}


				if (CalculateColorsInfoGlobal.PropogateLOD0ToAllLODsUsingOctree && CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD.Num() > 1) {

					propogateToLODs_CurrentLODBaseBounds += positionsInComponentLocalSpaceFromLOD[currentLODVertex];
				}


				if (CalculateColorsInfoGlobal.IsPaintEntireMeshTask()) {
					
					GetPaintEntireMeshVertexInfo(CalculateColorsInfoGlobal.PaintOnEntireMeshSettings.PaintOnRandomVerticesSettings, currentVertexWorldSpace, currentVertexComponentSpace, totalAmountOfVertsAtLOD, currentLOD, currentLODVertex, paintEntireMesh_RandomVerticesDoublettesPaintedAtLOD0, paintEntireMesh_RandomStream, paintEntireMesh_PaintCurrentVertex, paintEntireMesh_RandomAmountOfVerticesLeftToRandomize);
				}



				if (CalculateColorsInfoGlobal.TaskFundamentalSettings.DebugSettings.GameThreadSpecificDebugSymbols.DrawVertexNormalDebugArrow && InGameThread) {

					DrawDebugLine(CalculateColorsInfoGlobal.TaskFundamentalSettings.TaskWorld.Get(), currentVertexWorldSpace, currentVertexWorldSpace + (normalsFromLOD[currentLODVertex] * 25), FColor::Red, false, CalculateColorsInfoGlobal.TaskFundamentalSettings.DebugSettings.GameThreadSpecificDebugSymbols.DrawGameThreadSpecificDebugSymbolsDuration, 0, .5);
				}

				if (CalculateColorsInfoGlobal.TaskFundamentalSettings.DebugSettings.GameThreadSpecificDebugSymbols.DrawVertexPositionDebugPoint && InGameThread) {

					DrawDebugPoint(CalculateColorsInfoGlobal.TaskFundamentalSettings.TaskWorld.Get(), currentVertexWorldSpace, 5, FColor::Red, false, CalculateColorsInfoGlobal.TaskFundamentalSettings.DebugSettings.GameThreadSpecificDebugSymbols.DrawGameThreadSpecificDebugSymbolsDuration, 0);
				}



				// LOD 0 Related Before Applying Colors
				if (currentLOD == 0) {


					if (CalculateColorsInfoGlobal.IsSkeletalMeshTask) {

						// Had to have an array of a color array that i add to instead of adding directly to the TMap since then i had to use .Contain and .FindRef which made it so each task took longer to finish
						if (CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeColorsOfEachBone && CalculateColorsInfoGlobal.IsDetectCombo && boneIndexColorsBeforeApplyingColors.IsValidIndex(currentBoneIndex)) {

							boneIndexColorsBeforeApplyingColors[currentBoneIndex].Add(colorFromLOD[currentLODVertex]);
						}
					}


					if (CalculateColorsInfoGlobal.IsPaintAtLocationTask()) {

						if (paintAtLocation_DistanceFromVertexToHitLocation < paintAtLocation_ClosestDistanceToClosestVertex) {

							// If its less then updates distanceFromTraceHitToVertPos, so in the next loop it will check against that value
							paintAtLocation_ClosestDistanceToClosestVertex = paintAtLocation_DistanceFromVertexToHitLocation;

							paintAtLocation_ClosestVertex = currentLODVertex;
							paintAtLocation_ClosestSection = currentSection;

							estimatedColorToHitLocation_DirectionToClosestVertex_Paint = (currentVertexWorldSpace - paintAtLocation_HitLocationInWorldSpace).GetSafeNormal();
						}


						if (CalculateColorsInfoGlobal.EstimatedColorAtHitLocationSettings.GetEstimatedColorAtHitLocation && paintAtLocation_CurrentVertexNormalToHitNormalDot >= CalculateColorsInfoGlobal.EstimatedColorAtHitLocationSettings.MinHitNormalToVertexNormalDotRequired) {

							// Adds the closest 9 into a TMap containing the closest verts and the Direction, index and distance to them. This is used when we've finished looping through LOD0 verts to determine which vertex we want to scale against to calculate the colors at the Hit Location. So based on the Direction from the closest vertex, and the hit Location, we can check which of the nearby vertices has the best Dot toward it. Then based on the distance to it we lerp from the closest vertex color, to the one that had the best dot, based on the distance to it, to get a close to accurate Color at Actual Hit Location of the Mesh, which is useful if you have meshes with few vertices. 
							GetVerticesCloseToEstimatedColorToHitLocation(estimatedColorToHitLocation_DirectionFromHitToClosestVerticesInfo_LongestDistance_Paint, estimatedColorToHitLocation_DirectionFromHitToClosestVerticesInfo_LongestDistanceIndex_Paint, estimatedColorToHitLocation_DirectionFromHitToClosestVerticesInfo_Paint, paintAtLocation_HitLocationInWorldSpace, currentVertexWorldSpace, paintAtLocation_CurrentVertexNormalToHitNormalDot, currentLODVertex, totalAmountOfVertsAtLOD);
						}
					}

					
					if (GetClosestVertexData) {


						if (CalculateColorsInfoGlobal.GetClosestVertexDataSettings.GetAverageColorInAreaSettings.AreaRangeToGetAvarageColorFrom > 0) {

							// If within range
							if ((getClosestVertexData_HitLocationInWorldSpace - currentVertexWorldSpace).Size() <= CalculateColorsInfoGlobal.GetClosestVertexDataSettings.GetAverageColorInAreaSettings.AreaRangeToGetAvarageColorFrom) {


								// Vertex World Normal is within Dot to Hit Normal. So we can only detect and get average color from one side of a wall if set to 0 for instance
								if (getClosestVertexData_CurrentVertexNormalToHitNormalDot >= CalculateColorsInfoGlobal.GetClosestVertexDataSettings.GetAverageColorInAreaSettings.VertexNormalToHitNormal_MinimumDotProductToBeAccountedFor) {


									getClosestVertexData_AmountOfVerticesWithinArea++;

									getClosestVertexData_AverageColorWithinAreaOfEffect_BeforeApplyingColors_Red += colorFromLOD[currentLODVertex].R;
									getClosestVertexData_AverageColorWithinAreaOfEffect_BeforeApplyingColors_Green += colorFromLOD[currentLODVertex].G;
									getClosestVertexData_AverageColorWithinAreaOfEffect_BeforeApplyingColors_Blue += colorFromLOD[currentLODVertex].B;
									getClosestVertexData_AverageColorWithinAreaOfEffect_BeforeApplyingColors_Alpha += colorFromLOD[currentLODVertex].A;
								}
							}
						}


						if (getClosestVertexData_DistanceFromVertexToHitLocation < getClosestVertexData_ClosestDistanceToClosestVertex) {

							// If its less then updates distanceFromTraceHitToVertPos, so in the next loop it will check against that value
							getClosestVertexData_ClosestDistanceToClosestVertex = (getClosestVertexData_HitLocationInWorldSpace - currentVertexWorldSpace).Size();
							
							getClosestVertexData_ClosestVertex = currentLODVertex;
							getClosestVertexData_ClosestSection = currentSection;

							estimatedColorToHitLocation_DirectionToClosestVertex_Detect = (currentVertexWorldSpace - getClosestVertexData_HitLocationInWorldSpace).GetSafeNormal();


							// If set to accept closest vertex if within a min range, and not going to loop through all vertices, isn't paint at Location combo or set to get estimated at hit Location
							if (CalculateColorsInfoGlobal.GetClosestVertexDataSettings.ClosestVertexDataOptimizationSettings.AcceptClosestVertexDataIfVertexIsWithinMinRange && !shouldLoopThroughAllVerticesOnLOD && !CalculateColorsInfoGlobal.IsDetectCombo && !CalculateColorsInfoGlobal.EstimatedColorAtHitLocationSettings.GetEstimatedColorAtHitLocation && !CalculateColorsInfoGlobal.GetClosestVertexDataSettings.GetAverageColorInAreaSettings.GetAverageColor) {

								// If within range then breaks the section vertex loop 
								if (getClosestVertexData_ClosestDistanceToClosestVertex <= CalculateColorsInfoGlobal.GetClosestVertexDataSettings.ClosestVertexDataOptimizationSettings.AcceptClosestVertexDataIfVertexIsWithinRange) {

									CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Calculate Colors Task - Closest Vertex Data - Accepted Closest Vertex Within Range: %i"), currentLODVertex), FColor::Cyan));

									acceptClosestVertexDataIfVertexIsWithinMinRange_GotClosestVertex = true;
									break;
								}
							}
						}


						if (CalculateColorsInfoGlobal.EstimatedColorAtHitLocationSettings.GetEstimatedColorAtHitLocation && getClosestVertexData_CurrentVertexNormalToHitNormalDot >= CalculateColorsInfoGlobal.EstimatedColorAtHitLocationSettings.MinHitNormalToVertexNormalDotRequired) {

							// Runs if it's a Detect, or if it's a paint at Location with detect before/after but with no custom hit, i.e. not on a different hit Location, then there's no need to fill the detection directionFromHitToClosestVerticesInfo_Detect TMap since it can just use the same result the paint job gets. This should save some performance since there's less loops every vertex etc.
							if (!CalculateColorsInfoGlobal.IsPaintAtLocationTask() || (CalculateColorsInfoGlobal.IsPaintAtLocationTask() && !CalculateColorsInfoGlobal.PaintAtLocationSettings.GetClosestVertexDataCombo.UseCustomHitSettings)) {

								GetVerticesCloseToEstimatedColorToHitLocation(estimatedColorToHitLocation_DirectionFromHitToClosestVerticesInfo_LongestDistance_Detect, estimatedColorToHitLocation_DirectionFromHitToClosestVerticesInfo_LongestDistanceIndex_Detect, estimatedColorToHitLocation_DirectionFromHitToClosestVerticesInfo_Detect, getClosestVertexData_HitLocationInWorldSpace, currentVertexWorldSpace, getClosestVertexData_CurrentVertexNormalToHitNormalDot, currentLODVertex, totalAmountOfVertsAtLOD);
							}
						}
					}


					// If only Get Colors Within Area then we do a similar check as Paint Within Area, but without all Falloff stuff. Otherwise if paint within area but with detection before then we just run this there but get what the color was Before we applied color. 
					if (GetColorsWithinArea) {

						if (GetWithinAreaVertexInfoAtLODZeroBeforeApplyingColors(CalculateColorsInfoGlobal.WithinAreaSettings, currentLOD, currentVertexWorldSpace, withinArea_VertexWasWithinArea)) {

							if (withinArea_VertexWasWithinArea && CalculateColorsInfoGlobal.WithinAreaSettings.IncludeVertexColorsWithinArea) {

								withinArea_ColorFromLOD_BeforeApplyingColors.Add(colorFromLOD[currentLODVertex]);
							}
						}

						else {

							return false;
						}
					}


					if (CalculateColorsInfoGlobal.TaskFundamentalSettings.DebugSettings.PrintLogsToScreen || CalculateColorsInfoGlobal.TaskFundamentalSettings.DebugSettings.PrintLogsToOutputLog) {

						UpdateColorsOfEachChannelAbove0BeforeApplyingColors(colorFromLODAsLinear_BeforeApplyingColors, currentLOD, amountOfPaintedColorsOfEachChannelAbove0_BeforeApplyingColor_Debug);
					}


					// If Detect task like Get Closest Vertex Data, Get Colors Only or Get Colors Within Area
					if (CalculateColorsInfoGlobal.IsDetectTask) {

						if (CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeAmountOfPaintedColorsOfEachChannel.IncludeVertexColorChannelResultOfEachChannel) {

							// Gets the amount and % etc. of each color channel before applying any colors
							GetColorsOfEachChannelForVertex(colorFromLODAsLinear_BeforeApplyingColors, CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeAmountOfPaintedColorsOfEachChannel, containsPhysicsSurfaceOnRedChannel, containsPhysicsSurfaceOnGreenChannel, containsPhysicsSurfaceOnBlueChannel, containsPhysicsSurfaceOnAlphaChannel, amountOfPaintedColorsOfEachChannel_BeforeApplyingColor.RedChannelResult, amountOfPaintedColorsOfEachChannel_BeforeApplyingColor.GreenChannelResult, amountOfPaintedColorsOfEachChannel_BeforeApplyingColor.BlueChannelResult, amountOfPaintedColorsOfEachChannel_BeforeApplyingColor.AlphaChannelResult);
						}

						if (CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeAmountOfPaintedColorsOfEachChannel.IncludePhysicsSurfaceResultOfEachChannel) {

							// Gets the amount and % etc. of each physics channel before applying any colors
							if (registeredPhysicsSurfacesAtMaterial.Num() > 0)
								GetColorsOfEachPhysicsSurfaceForVertex(registeredPhysicsSurfacesAtMaterial, colorFromLODAsLinear_BeforeApplyingColors, CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeAmountOfPaintedColorsOfEachChannel, containsPhysicsSurfaceOnRedChannel, containsPhysicsSurfaceOnGreenChannel, containsPhysicsSurfaceOnBlueChannel, containsPhysicsSurfaceOnAlphaChannel, physicsSurfaceResultForSection_BeforeAddingColors);
						}
					}


					if (CalculateColorsInfoGlobal.IsDetectCombo) {


						if (compareColorsToArray) {
							
							CompareColorArrayToVertex(CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.CompareMeshVertexColorsToColorArray, currentLODVertex, colorFromLOD[currentLODVertex], compareColorsToArray_AmountOfVerticesConsidered_BeforeApplyingColors, compareMeshVertexColorsToColorArrayMatchingPercent_BeforeApplyingColors);
						}

						if (compareColorsToColorSnippet) { 

							CompareColorsSnippetToVertex(CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.CompareMeshVertexColorsToColorSnippets, CompareColorSnippetColorArray, currentLODVertex, colorFromLOD[currentLODVertex], compareColorsToSnippet_AmountOfVerticesConsidered_BeforeApplyingColors, compareColorSnippetMatchingPercent_BeforeApplyingColors);
						}
					}
				}


				// If Painting
				if (CalculateColorsInfoGlobal.IsPaintTask) {

					if (shouldApplyVertexColors) {

						bool vertexColorGotChanged = false;
						bool triedToApplyColorOnVertex = false;

						// If Color Snippet, Set Mesh Component Colors, Set Mesh Component Colors Using Serialized string. i.e. paint jobs that doesn't paint with RGBA or Physics Surface
						if (CalculateColorsInfoGlobal.IsPaintDirectlyTask) {


							switch (CalculateColorsInfoGlobal.VertexPaintDetectionType) {

								case EVertexPaintDetectionType::PaintColorSnippet:

									if (ColorSnippetColors.IsValidIndex(currentLODVertex)) {

										colorFromLOD[currentLODVertex] = ColorSnippetColors[currentLODVertex];
										vertexColorGotChanged = true;
									}
									break;

								case EVertexPaintDetectionType::SetMeshVertexColorsDirectly:

									if (CalculateColorsInfoGlobal.SetVertexColorsSettings.VertexColorsAtLOD0ToSet.IsValidIndex(currentLODVertex)) {

										colorFromLOD[currentLODVertex] = CalculateColorsInfoGlobal.SetVertexColorsSettings.VertexColorsAtLOD0ToSet[currentLODVertex];
										vertexColorGotChanged = true;
									}
									break;

								case EVertexPaintDetectionType::SetMeshVertexColorsDirectlyUsingSerializedString:

									if (DeserializedVertexColors.IsValidIndex(currentLODVertex)) {

										colorFromLOD[currentLODVertex] = DeserializedVertexColors[currentLODVertex];
										vertexColorGotChanged = true;
									}
									break;
							}


							triedToApplyColorOnVertex = true;

							// If we're looping through vertcies and it's a color snippet or SetMeshComponentVertexColor Job, it means the callback settings is set to return the positions, or that we want to run an override interface. If override interface then this is useful since it sends a bool wether the vertex will apply color, and for these paint jobs they all do since we're setting all vertex colors to be something. 
							shouldApplyColorOnAnyChannel = true;

							// Color Snippets doesn't apply colors using RGBA or Physics Surface so doesn't run the same logic as the others that has paint condition checks etc.
							if (paintDirectlyLimit_RedChannel.UsePaintLimits || paintDirectlyLimit_GreenChannel.UsePaintLimits || paintDirectlyLimit_BlueChannel.UsePaintLimits || paintDirectlyLimit_AlphaChannel.UsePaintLimits) {

								float getColorToApplyOnVertex_ColorRed = 0;
								float getColorToApplyOnVertex_ColorGreen = 0;
								float getColorToApplyOnVertex_ColorBlue = 0;
								float getColorToApplyOnVertex_ColorAlpha = 0;
								
								GetVertexColorAfterAnyLimitation(colorFromLOD[currentLODVertex], paintDirectlyLimit_RedChannel, paintDirectlyLimit_GreenChannel, paintDirectlyLimit_BlueChannel, paintDirectlyLimit_AlphaChannel, static_cast<float>(colorFromLOD[currentLODVertex].R), static_cast<float>(colorFromLOD[currentLODVertex].G), static_cast<float>(colorFromLOD[currentLODVertex].B), static_cast<float>(colorFromLOD[currentLODVertex].A), getColorToApplyOnVertex_ColorRed, getColorToApplyOnVertex_ColorGreen, getColorToApplyOnVertex_ColorBlue, getColorToApplyOnVertex_ColorAlpha);

								colorFromLOD[currentLODVertex].R = static_cast<uint8>(UKismetMathLibrary::FClamp(getColorToApplyOnVertex_ColorRed, 0, 255));
								colorFromLOD[currentLODVertex].G = static_cast<uint8>(UKismetMathLibrary::FClamp(getColorToApplyOnVertex_ColorGreen, 0, 255));
								colorFromLOD[currentLODVertex].B = static_cast<uint8>(UKismetMathLibrary::FClamp(getColorToApplyOnVertex_ColorBlue, 0, 255));
								colorFromLOD[currentLODVertex].A = static_cast<uint8>(UKismetMathLibrary::FClamp(getColorToApplyOnVertex_ColorAlpha, 0, 255));
							}
						}

						else if (WillAppliedColorMakeChangeOnVertex(colorFromLOD[currentLODVertex], paintOnMeshColorSettingsForSection, physicsSurfacePaintSettingsPerChannel)) {

							switch (CalculateColorsInfoGlobal.VertexPaintDetectionType) {

								case EVertexPaintDetectionType::PaintAtLocation:

								{
									// If distance from hit and the vertex is within the paint area off effect then gets the color updated with strength, falloffs etc. 
									if (paintAtLocation_DistanceFromVertexToHitLocation >= CalculateColorsInfoGlobal.PaintAtLocationSettings.PaintAtAreaSettings.AreaOfEffectRangeStart && paintAtLocation_DistanceFromVertexToHitLocation <= CalculateColorsInfoGlobal.PaintAtLocationSettings.PaintAtAreaSettings.AreaOfEffectRangeEnd) {


										const float paintAtLocationDistanceFromFalloffBase = GetPaintAtLocationDistanceFromFallOffBase(CalculateColorsInfoGlobal.PaintAtLocationSettings.PaintAtAreaSettings.FallOffSettings, paintAtLocation_DistanceFromVertexToHitLocation);

										colorFromLOD[currentLODVertex] = GetColorToApplyOnVertex(objectToOverrideInstigatedByPaintComponent, objectToOverrideVertexColorsWith, currentLOD, currentLODVertex, materialToGetSurfacesFrom, registeredPhysicsSurfacesAtMaterial, physicsSurfacePaintSettingsPerChannel, isVertexOnCloth, colorFromLODAsLinear_BeforeApplyingColors, colorFromLOD[currentLODVertex], currentVertexWorldSpace, normalsFromLOD[currentLODVertex], currentBoneName, vertexTotalBoneWeight, CalculateColorsInfoGlobal.PaintAtLocationSettings.PaintAtAreaSettings.FallOffSettings, CalculateColorsInfoGlobal.PaintAtLocationSettings.PaintAtAreaSettings.PaintAtLocationFallOffMaxRange, CalculateColorsInfoGlobal.PaintAtLocationSettings.PaintAtAreaSettings.PaintAtLocationScaleFallOffFrom, paintAtLocationDistanceFromFalloffBase, paintOnMeshColorSettingsForSection, shouldApplyColorOnRedChannel, shouldApplyColorOnGreenChannel, shouldApplyColorOnBlueChannel, shouldApplyColorOnAlphaChannel, vertexColorGotChanged, haveRunOverrideInterface, shouldApplyColorOnAnyChannel, hasAnyPaintConditions, hasPhysicsSurfacePaintCondition, hasRedChannelPaintCondition, hasGreenChannelPaintCondition, hasBlueChannelPaintCondition, hasAlphaChannelPaintCondition);

										triedToApplyColorOnVertex = true;
									}
								}

									break;

								case EVertexPaintDetectionType::PaintWithinArea:

								{

									int strongestAreaWithin_Index = -1;
									float strongestAreaWithin_Strength = 0;
									float strongestAreaWithin_DistanceFromFallOffBase = 0;

									for (int i = 0; i < CalculateColorsInfoGlobal.WithinAreaSettings.ComponentsToCheckIfIsWithin.Num(); i++) {

										bool wasVertexInArea = false;

										// If Complex shape then requires that the component is still valid as we trace for it
										if (CalculateColorsInfoGlobal.WithinAreaSettings.ComponentsToCheckIfIsWithin[i].PaintWithinAreaShape == EPaintWithinAreaShape::IsComplexShape) {

											// So we only resolve the weak ptr if its needed by the function, to make the task a bit faster for non complex 
											UPrimitiveComponent* componentToCheckIfIsWithin = CalculateColorsInfoGlobal.WithinAreaSettings.ComponentsToCheckIfIsWithin[i].GetComponentToCheckIfIsWithin();
											if (!IsValid(componentToCheckIfIsWithin)) return false;

											wasVertexInArea = IsVertexWithinArea(componentToCheckIfIsWithin, currentVertexWorldSpace, CalculateColorsInfoGlobal.WithinAreaSettings.ComponentsToCheckIfIsWithin[i]);
										}

										else {

											wasVertexInArea = IsVertexWithinArea(nullptr, currentVertexWorldSpace, CalculateColorsInfoGlobal.WithinAreaSettings.ComponentsToCheckIfIsWithin[i]);
										}


										if (wasVertexInArea) {


											const float withinAreaDistanceFromFalloffBase = GetPaintWithinAreaDistanceFromFalloffBase(CalculateColorsInfoGlobal.WithinAreaSettings.ComponentsToCheckIfIsWithin[i].FallOffSettings, CalculateColorsInfoGlobal.WithinAreaSettings.ComponentsToCheckIfIsWithin[i], currentVertexWorldSpace);


											// Does the same Falloff Check as when applying color to get which component within area that has the strongest strength, i.e. the one we want to prioritize in case the vertex is inside several components to check if within area. Otherwise if you just took the first one, there could be issues where a vertex wasnt painted with the expected strength in case it was on the outskirts of a sphere for instance but close to the center of another. 
											float strengthAfterFalloff = 1;
											if (CalculateColorsInfoGlobal.WithinAreaSettings.ComponentsToCheckIfIsWithin[i].FallOffSettings.FallOffStrength > 0) {

												float distance = UKismetMathLibrary::MapRangeClamped(withinAreaDistanceFromFalloffBase, CalculateColorsInfoGlobal.WithinAreaSettings.ComponentsToCheckIfIsWithin[i].FallOff_ScaleFrom, CalculateColorsInfoGlobal.WithinAreaSettings.ComponentsToCheckIfIsWithin[i].FallOff_WithinAreaOfEffect, 0, 1);

												float fallOffClamped = UKismetMathLibrary::FClamp(CalculateColorsInfoGlobal.WithinAreaSettings.ComponentsToCheckIfIsWithin[i].FallOffSettings.FallOffStrength, 0, 1);

												float falloff = UKismetMathLibrary::MapRangeClamped(fallOffClamped, 0, 1, 1, 0);

												strengthAfterFalloff = UKismetMathLibrary::MapRangeClamped(distance, 0, 1, 1, falloff);
											}

											if (strengthAfterFalloff > strongestAreaWithin_Strength) {

												strongestAreaWithin_Strength = strengthAfterFalloff;
												strongestAreaWithin_Index = i;
												strongestAreaWithin_DistanceFromFallOffBase = withinAreaDistanceFromFalloffBase;
											}
										}
									}


									if (strongestAreaWithin_Index >= 0) {


										withinArea_VertexWasWithinArea = true;

										// If set to get colors before paint job, then gets the colors before applying it 
										if (CalculateColorsInfoGlobal.IsGetColorsWithinAreaTask())
											withinArea_ColorFromLOD_BeforeApplyingColors.Add(colorFromLOD[currentLODVertex]);


										colorFromLOD[currentLODVertex] = GetColorToApplyOnVertex(objectToOverrideInstigatedByPaintComponent, objectToOverrideVertexColorsWith, currentLOD, currentLODVertex, materialToGetSurfacesFrom, registeredPhysicsSurfacesAtMaterial, physicsSurfacePaintSettingsPerChannel, isVertexOnCloth, colorFromLODAsLinear_BeforeApplyingColors, colorFromLOD[currentLODVertex], currentVertexWorldSpace, normalsFromLOD[currentLODVertex], currentBoneName, vertexTotalBoneWeight, CalculateColorsInfoGlobal.WithinAreaSettings.ComponentsToCheckIfIsWithin[strongestAreaWithin_Index].FallOffSettings, CalculateColorsInfoGlobal.WithinAreaSettings.ComponentsToCheckIfIsWithin[strongestAreaWithin_Index].FallOff_WithinAreaOfEffect, CalculateColorsInfoGlobal.WithinAreaSettings.ComponentsToCheckIfIsWithin[strongestAreaWithin_Index].FallOff_ScaleFrom, strongestAreaWithin_DistanceFromFallOffBase, paintOnMeshColorSettingsForSection, shouldApplyColorOnRedChannel, shouldApplyColorOnGreenChannel, shouldApplyColorOnBlueChannel, shouldApplyColorOnAlphaChannel, vertexColorGotChanged, haveRunOverrideInterface, shouldApplyColorOnAnyChannel, hasAnyPaintConditions, hasPhysicsSurfacePaintCondition, hasRedChannelPaintCondition, hasGreenChannelPaintCondition, hasBlueChannelPaintCondition, hasAlphaChannelPaintCondition);


										if (CalculateColorsInfoGlobal.WithinAreaSettings.IncludeVertexColorsWithinArea)
											withinArea_ColorFromLOD_AfterApplyingColors.Add(colorFromLOD[currentLODVertex]);


										triedToApplyColorOnVertex = true;

										// Caches if a vertex was within area so we can print debug related logs in case Any wasn't in the area when finish. 
										if (!CalculateColorsInfoGlobal.PaintWithinArea_WasAnyVertexWithinArea)
											CalculateColorsInfoGlobal.PaintWithinArea_WasAnyVertexWithinArea = triedToApplyColorOnVertex;
									}

									else if (CalculateColorsInfoGlobal.PaintWithinAreaSettings.ColorToApplyToVerticesOutsideOfArea.IsGoingToApplyAnyColorOutsideOfArea()) {

										FRVPDPApplyColorSetting applyColorSettingsOutSideOfArea;
										applyColorSettingsOutSideOfArea.ApplyVertexColorSettings.ApplyColorsOnRedChannel.AmountToApply = CalculateColorsInfoGlobal.PaintWithinAreaSettings.ColorToApplyToVerticesOutsideOfArea.RedVertexColorToApply;

										applyColorSettingsOutSideOfArea.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.AmountToApply = CalculateColorsInfoGlobal.PaintWithinAreaSettings.ColorToApplyToVerticesOutsideOfArea.GreenVertexColorToApply;

										applyColorSettingsOutSideOfArea.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.AmountToApply = CalculateColorsInfoGlobal.PaintWithinAreaSettings.ColorToApplyToVerticesOutsideOfArea.BlueVertexColorToApply;

										applyColorSettingsOutSideOfArea.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.AmountToApply = CalculateColorsInfoGlobal.PaintWithinAreaSettings.ColorToApplyToVerticesOutsideOfArea.AlphaVertexColorToApply;


										colorFromLOD[currentLODVertex] = GetColorToApplyOnVertex(objectToOverrideInstigatedByPaintComponent, objectToOverrideVertexColorsWith, currentLOD, currentLODVertex, materialToGetSurfacesFrom, registeredPhysicsSurfacesAtMaterial, physicsSurfacePaintSettingsPerChannel, isVertexOnCloth, colorFromLODAsLinear_BeforeApplyingColors, colorFromLOD[currentLODVertex], currentVertexWorldSpace, normalsFromLOD[currentLODVertex], currentBoneName, vertexTotalBoneWeight, FRVPDPVertexPaintFallOffSettings(), 0, 0, 0, applyColorSettingsOutSideOfArea, shouldApplyColorOnRedChannel, shouldApplyColorOnGreenChannel, shouldApplyColorOnBlueChannel, shouldApplyColorOnAlphaChannel, vertexColorGotChanged, haveRunOverrideInterface, shouldApplyColorOnAnyChannel, hasAnyPaintConditions, hasPhysicsSurfacePaintCondition, hasRedChannelPaintCondition, hasGreenChannelPaintCondition, hasBlueChannelPaintCondition, hasAlphaChannelPaintCondition);
									}
								}

									break;

								case EVertexPaintDetectionType::PaintEntireMesh:

								{

									if (paintEntireMesh_PaintCurrentVertex) {

										colorFromLOD[currentLODVertex] = GetColorToApplyOnVertex(objectToOverrideInstigatedByPaintComponent, objectToOverrideVertexColorsWith, currentLOD, currentLODVertex, materialToGetSurfacesFrom, registeredPhysicsSurfacesAtMaterial, physicsSurfacePaintSettingsPerChannel, isVertexOnCloth, colorFromLODAsLinear_BeforeApplyingColors, colorFromLOD[currentLODVertex], currentVertexWorldSpace, normalsFromLOD[currentLODVertex], currentBoneName, vertexTotalBoneWeight, FRVPDPVertexPaintFallOffSettings(), 0, 0, 0, paintOnMeshColorSettingsForSection, shouldApplyColorOnRedChannel, shouldApplyColorOnGreenChannel, shouldApplyColorOnBlueChannel, shouldApplyColorOnAlphaChannel, vertexColorGotChanged, haveRunOverrideInterface, shouldApplyColorOnAnyChannel, hasAnyPaintConditions, hasPhysicsSurfacePaintCondition, hasRedChannelPaintCondition, hasGreenChannelPaintCondition, hasBlueChannelPaintCondition, hasAlphaChannelPaintCondition);

										triedToApplyColorOnVertex = true;
									}

								}

									break;
							}
						}



						// If set to Propgoate LOD0 Colors to other LODs, and it's already stored, then we can look up the color to apply this LODs vertex 
						if (currentLOD > 0 && CalculateColorsInfoGlobal.PropogateLOD0ToAllLODsFromStoredData) {


							if (CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD.IsValidIndex(0)) {


								const int32 bestLOD0Vertex = lodPropogateToLODInfo.LODVerticesAssociatedLOD0Vertex.FindRef(currentLODVertex);


								if (CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD[0].MeshVertexColorsPerLODArray.IsValidIndex(bestLOD0Vertex)) {


									colorFromLOD[currentLODVertex] = CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD[0].MeshVertexColorsPerLODArray[bestLOD0Vertex];

									triedToApplyColorOnVertex = true;
								}
							}
						}


						// To make sure we haven't tried running this before, in case we did try to apply paint on a vertex earlier either successfully or unsuccessfully
						if (!haveRunOverrideInterface) {

							// Since we can get here with triedToApplyColorOnVertex either true or false, we need to check if the users has set the requirement that we should only override it if its  true 
							if (CalculateColorsInfoGlobal.PaintTaskSettings.OverrideVertexColorsToApplySettings.OverrideVertexColorsToApply && (!CalculateColorsInfoGlobal.PaintTaskSettings.OverrideVertexColorsToApplySettings.OnlyRunOverrideInterfaceIfTryingToApplyColorToVertex || (CalculateColorsInfoGlobal.PaintTaskSettings.OverrideVertexColorsToApplySettings.OnlyRunOverrideInterfaceIfTryingToApplyColorToVertex && triedToApplyColorOnVertex))) {


								if (objectToOverrideVertexColorsWith) {


									TEnumAsByte<EPhysicalSurface> overrideVertexColors_MostDominantPhysicsSurfaceAtVertexBeforeApplyingColors = EPhysicalSurface::SurfaceType_Default;
									float overrideVertexColors_MostDominantPhysicsSurfaceValueAtVertexBeforeApplyingColors = 0;

									if (CalculateColorsInfoGlobal.PaintTaskSettings.OverrideVertexColorsToApplySettings.IncludeMostDominantPhysicsSurface) {

										TArray<float> overrideVertexColors_DefaultVertexColors;
										overrideVertexColors_DefaultVertexColors.Add(UVertexPaintFunctionLibrary::ReliableFColorToFLinearColor(defaultColorFromLOD[currentLODVertex]).R);
										overrideVertexColors_DefaultVertexColors.Add(UVertexPaintFunctionLibrary::ReliableFColorToFLinearColor(defaultColorFromLOD[currentLODVertex]).G);
										overrideVertexColors_DefaultVertexColors.Add(UVertexPaintFunctionLibrary::ReliableFColorToFLinearColor(defaultColorFromLOD[currentLODVertex]).B);
										overrideVertexColors_DefaultVertexColors.Add(UVertexPaintFunctionLibrary::ReliableFColorToFLinearColor(defaultColorFromLOD[currentLODVertex]).A);

										UVertexPaintFunctionLibrary::GetTheMostDominantPhysicsSurface_Wrapper(CalculateColorsInfoGlobal.TaskFundamentalSettings.TaskWorld.Get(), materialToGetSurfacesFrom, registeredPhysicsSurfacesAtMaterial, overrideVertexColors_DefaultVertexColors, overrideVertexColors_MostDominantPhysicsSurfaceAtVertexBeforeApplyingColors, overrideVertexColors_MostDominantPhysicsSurfaceValueAtVertexBeforeApplyingColors);
									}

									haveRunOverrideInterface = true;

									FColor overrideVertexColors_ColorToApply = FColor();
									bool overrideVertexColorsToApply = false;

									IVertexPaintDetectionInterface::Execute_OverrideVertexColorToApply(objectToOverrideVertexColorsWith, CalculateColorsInfoGlobal.PaintTaskSettings.OverrideVertexColorsToApplySettings.OverrideVertexColorsToApplyID, objectToOverrideInstigatedByPaintComponent, CalculateColorsInfoGlobal.GetVertexPaintComponent(), currentLOD, currentLODVertex, materialToGetSurfacesFrom, isVertexOnCloth, currentBoneName, currentVertexWorldSpace, normalsFromLOD[currentLODVertex], defaultColorFromLOD[currentLODVertex], overrideVertexColors_MostDominantPhysicsSurfaceAtVertexBeforeApplyingColors, overrideVertexColors_MostDominantPhysicsSurfaceValueAtVertexBeforeApplyingColors, colorFromLOD[currentLODVertex], shouldApplyColorOnAnyChannel, shouldApplyColorOnAnyChannel, overrideVertexColorsToApply, overrideVertexColors_ColorToApply);

									if (overrideVertexColorsToApply) {

										CalculateColorsInfoGlobal.OverridenVertexColors_MadeChangeToColorsToApply = true;

										if (shouldApplyColorOnAnyChannel)
											colorFromLOD[currentLODVertex] = overrideVertexColors_ColorToApply;
									}
								}


								// If overriden then we can use the result from the interface
								if (shouldApplyColorOnAnyChannel) {

									triedToApplyColorOnVertex = true;
									vertexColorGotChanged = true;
								}

								// If returned to Not apply vertex color then resets some bools so shouldApplyVertexColors gets set accordingly
								else {

									triedToApplyColorOnVertex = false;
									vertexColorGotChanged = false;
								}
							}
						}


						if (triedToApplyColorOnVertex && currentLOD == 0) {


							if (CalculateColorsInfoGlobal.PaintTaskSettings.PaintTaskCallbackSettings.IncludeBonesThatGotPainted && CalculateColorsInfoGlobal.IsSkeletalMeshTask && !CalculateColorsInfoGlobal.PaintTaskResult.BonesThatGotPainted.Contains(currentBoneName)) {

								CalculateColorsInfoGlobal.PaintTaskResult.BonesThatGotPainted.Add(currentBoneName);
							}


							if (CalculateColorsInfoGlobal.PaintTaskSettings.PaintTaskCallbackSettings.IncludePaintedVertexIndicesAtLOD0) {

								CalculateColorsInfoGlobal.PaintTaskResult.VertexIndicesThatGotPainted.Add(currentLODVertex);
							}


#if ENGINE_MAJOR_VERSION == 5


							// Geometry Collection TManagedArray<FLinearColor> requires FLinearColor, so to avoid looping through all the colors on game thread to build a FLinearColor array for geo collection comps we update one here. 
							if (CalculateColorsInfoGlobal.IsGeometryCollectionTask) {

								CalculateColorsInfoGlobal.GeometryCollectionComponentVertexColors[currentLODVertex] = UVertexPaintFunctionLibrary::ReliableFColorToFLinearColor(colorFromLOD[currentLODVertex]);
							}
#endif


							if (CalculateColorsInfoGlobal.TaskFundamentalSettings.DebugSettings.GameThreadSpecificDebugSymbols.DrawVertexPositionDebugPointIfGotPaintApplied && InGameThread) {

								DrawDebugPoint(CalculateColorsInfoGlobal.TaskFundamentalSettings.TaskWorld.Get(), currentVertexWorldSpace, 7, FColor::Green, false, CalculateColorsInfoGlobal.TaskFundamentalSettings.DebugSettings.GameThreadSpecificDebugSymbols.DrawGameThreadSpecificDebugSymbolsDuration, 0);
							}
						}


						if (!CalculateColorsInfoGlobal.PaintTaskResult.AnyVertexColorGotChanged) {

							CalculateColorsInfoGlobal.PaintTaskResult.AnyVertexColorGotChanged = vertexColorGotChanged;
						}


						colorFromLODAsLinear = UVertexPaintFunctionLibrary::ReliableFColorToFLinearColor(colorFromLOD[currentLODVertex]);


						if (vertexColorGotChanged) {

							if (!redColorChannelChanged && colorFromLODAsLinear_BeforeApplyingColors.R != colorFromLODAsLinear.R) {

								redColorChannelChanged = true;

								if (!CalculateColorsInfoGlobal.PaintTaskResult.ColorAppliedToVertexColorChannels.Contains(EVertexColorChannel::RedChannel))
									CalculateColorsInfoGlobal.PaintTaskResult.ColorAppliedToVertexColorChannels.Add(EVertexColorChannel::RedChannel);
							}

							if (!greenColorChannelChanged && colorFromLODAsLinear_BeforeApplyingColors.G != colorFromLODAsLinear.G) {

								greenColorChannelChanged = true;

								if (!CalculateColorsInfoGlobal.PaintTaskResult.ColorAppliedToVertexColorChannels.Contains(EVertexColorChannel::GreenChannel))
									CalculateColorsInfoGlobal.PaintTaskResult.ColorAppliedToVertexColorChannels.Add(EVertexColorChannel::GreenChannel);
							}

							if (!blueColorChannelChanged && colorFromLODAsLinear_BeforeApplyingColors.B != colorFromLODAsLinear.B) {

								blueColorChannelChanged = true;

								if (!CalculateColorsInfoGlobal.PaintTaskResult.ColorAppliedToVertexColorChannels.Contains(EVertexColorChannel::BlueChannel))
									CalculateColorsInfoGlobal.PaintTaskResult.ColorAppliedToVertexColorChannels.Add(EVertexColorChannel::BlueChannel);
							}

							if (!alphaColorChannelChanged && colorFromLODAsLinear_BeforeApplyingColors.A != colorFromLODAsLinear.A) {

								alphaColorChannelChanged = true;

								if (!CalculateColorsInfoGlobal.PaintTaskResult.ColorAppliedToVertexColorChannels.Contains(EVertexColorChannel::AlphaChannel))
									CalculateColorsInfoGlobal.PaintTaskResult.ColorAppliedToVertexColorChannels.Add(EVertexColorChannel::AlphaChannel);
							}
						}
					}


					// Updates color buffer vertex color, so when we apply the colors we can just set to use the new buffer and don't have to run InitFromColorArray which loops through all of the verts. Runs this even if shouldApplyVertexColors is false, since the newColorBuffer still has be set with the current color, otherwise the sections that we're not applying color on will look weird. 
					if (CalculateColorsInfoGlobal.NewColorVertexBuffersPerLOD.IsValidIndex(currentLOD))
						CalculateColorsInfoGlobal.NewColorVertexBuffersPerLOD[currentLOD]->VertexColor(currentLODVertex) = colorFromLOD[currentLODVertex];
				}


				// These are the things both Get Colors Within Area, and Paint Within Area can share if the vertex was within area
				if (withinArea_VertexWasWithinArea) {

					withinArea_AmountOfVerticesWithinArea++;

					// If set to default include vertex data for LOD0 then only gets it for that, otherwise if false then gets it for All LODs. 
					if (!CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeVertexDataOnlyForLOD0 || (CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeVertexDataOnlyForLOD0 && currentLOD == 0)) {

						if (CalculateColorsInfoGlobal.WithinAreaSettings.IncludeVertexPositionsWithinArea)
							withinArea_PositionsInComponentLocalSpaceFromLOD.Add(positionsInComponentLocalSpaceFromLOD[currentLODVertex]);

						if (CalculateColorsInfoGlobal.WithinAreaSettings.IncludeVertexNormalsWithinArea)
							withinArea_NormalsFromLOD.Add(normalsFromLOD[currentLODVertex]);

						if (CalculateColorsInfoGlobal.WithinAreaSettings.IncludeVertexIndexesWithinArea)
							withinArea_VertexIndexesFromLOD.Add(currentLODVertex);
					}


					if (currentLOD == 0 && (GetColorsWithinArea || CalculateColorsInfoGlobal.IsPaintWithinAreaTask())) {

						GetWithinAreaVertexInfoAfterApplyingColorsAtLODZero(CalculateColorsInfoGlobal.WithinAreaSettings, registeredPhysicsSurfacesAtMaterial, colorFromLODAsLinear_BeforeApplyingColors, colorFromLODAsLinear, currentLOD, withinArea_VertexWasWithinArea, withinArea_ContainsPhysicsSurfaceOnRedChannel, withinArea_ContainsPhysicsSurfaceOnGreenChannel, withinArea_ContainsPhysicsSurfaceOnBlueChannel, withinArea_ContainsPhysicsSurfaceOnAlphaChannel, amountOfPaintedColorsOfEachChannel_WithinArea_BeforeApplyingColors, physicsSurfaceResultForSection_WithinArea_BeforeApplyingColors, amountOfPaintedColorsOfEachChannel_WithinArea_AfterApplyingColors, physicsSurfaceResultForSection_WithinArea_AfterApplyingColors);
					}
				}



				// LOD 0 Related After Applying Colors
				if (currentLOD == 0) {


					if (CalculateColorsInfoGlobal.IsSkeletalMeshTask) {

						// Had to have an array of a color array that i add to instead of adding directly to the TMap since then i had to use .Contain and .FindRef which made it so each task took longer to finish
						if (CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeColorsOfEachBone && boneIndexColorsAfterApplyingColors.IsValidIndex(currentBoneIndex)) {

							boneIndexColorsAfterApplyingColors[currentBoneIndex].Add(colorFromLOD[currentLODVertex]);
						}
					}


					// LOD 0 Paint Job
					if (CalculateColorsInfoGlobal.IsPaintTask) {


						if (CalculateColorsInfoGlobal.IsPaintAtLocationTask()) {


							if (CalculateColorsInfoGlobal.PaintAtLocationSettings.PaintAtAreaSettings.GetAverageColorAfterApplyingColorSettings.GetAverageColor && CalculateColorsInfoGlobal.PaintAtLocationSettings.PaintAtAreaSettings.GetAverageColorAfterApplyingColorSettings.AreaRangeToGetAvarageColorFrom > 0) {

								if (paintAtLocation_DistanceFromVertexToHitLocation <= CalculateColorsInfoGlobal.PaintAtLocationSettings.PaintAtAreaSettings.GetAverageColorAfterApplyingColorSettings.AreaRangeToGetAvarageColorFrom) {

									if (paintAtLocation_CurrentVertexNormalToHitNormalDot >= CalculateColorsInfoGlobal.PaintAtLocationSettings.PaintAtAreaSettings.GetAverageColorAfterApplyingColorSettings.VertexNormalToHitNormal_MinimumDotProductToBeAccountedFor) {

										paintAtLocation_AmountOfVerticesWithinArea++;

										paintAtLocation_AverageColorInAreaOfEffect_Red += colorFromLOD[currentLODVertex].R;
										paintAtLocation_AverageColorInAreaOfEffect_Green += colorFromLOD[currentLODVertex].G;
										paintAtLocation_AverageColorInAreaOfEffect_Blue += colorFromLOD[currentLODVertex].B;
										paintAtLocation_AverageColorInAreaOfEffect_Alpha += colorFromLOD[currentLODVertex].A;
									}
								}
							}
						}

						if (CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeAmountOfPaintedColorsOfEachChannel.IncludeVertexColorChannelResultOfEachChannel) {

							// Gets the amount and % etc. of each color channel after applying any colors. Lost quite a lot of performance if sending in and returning the parent struct rather than the individual results with &, as well as if we used a TArray<bool> rather than individual. 
							GetColorsOfEachChannelForVertex(colorFromLODAsLinear, CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeAmountOfPaintedColorsOfEachChannel, containsPhysicsSurfaceOnRedChannel, containsPhysicsSurfaceOnGreenChannel, containsPhysicsSurfaceOnBlueChannel, containsPhysicsSurfaceOnAlphaChannel, amountOfPaintedColorsOfEachChannel_AfterApplyingColors.RedChannelResult, amountOfPaintedColorsOfEachChannel_AfterApplyingColors.GreenChannelResult, amountOfPaintedColorsOfEachChannel_AfterApplyingColors.BlueChannelResult, amountOfPaintedColorsOfEachChannel_AfterApplyingColors.AlphaChannelResult);
						}

						if (CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeAmountOfPaintedColorsOfEachChannel.IncludePhysicsSurfaceResultOfEachChannel) {

							// Gets the amount and % etc. of each physics channel after applied any colors
							if (registeredPhysicsSurfacesAtMaterial.Num() > 0) {

								GetColorsOfEachPhysicsSurfaceForVertex(registeredPhysicsSurfacesAtMaterial, colorFromLODAsLinear, CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeAmountOfPaintedColorsOfEachChannel, containsPhysicsSurfaceOnRedChannel, containsPhysicsSurfaceOnGreenChannel, containsPhysicsSurfaceOnBlueChannel, containsPhysicsSurfaceOnAlphaChannel, physicsSurfaceResultForSection_AfterApplyingColors);
							}
						}


						// If set to randomize over the entire mesh, and to use Modified Engine Method, then we get the things it requires. If this vertex is a doublette of another that is on the Exact same position, then we want it to also get painted, so all doublettes at a position have the same color. This is so when we run our calculation so LOD1, 2 vertices get the same color as their closest and best LOD0 vertex, it shouldn't matter which of the doublettes the calculation chooses to be the best, as they all have the same color. Otherwise we had an issue where other LODs wouldn't get LOD0 color if using modified engine method since the result of it can be that the closest LOD0 vertex color to use for a LOD1 vertex could be a doublette of the randomized one that was on the exact same Location

						if (CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD.Num() > 1 && CalculateColorsInfoGlobal.PropogateLOD0ToAllLODsUsingOctree) {

							paintedVertex.Color = colorFromLOD[currentLODVertex];
							propogateToLODs_PaintedVerticesAtLOD0.Add(paintedVertex);

							if (paintEntireMesh_PaintCurrentVertex) {

								paintEntireMesh_RandomVerticesDoublettesPaintedAtLOD0.Add(positionsInComponentLocalSpaceFromLOD[currentLODVertex], colorFromLOD[currentLODVertex]);
							}
						}
					}


					if (compareColorsToArray) {

						CompareColorArrayToVertex(CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.CompareMeshVertexColorsToColorArray, currentLODVertex, colorFromLOD[currentLODVertex], compareColorsToArray_AmountOfVerticesConsidered_AfterApplyingColors, compareMeshVertexColorsToColorArrayMatchingPercent_AfterApplyingColors);
					}

					if (compareColorsToColorSnippet) {

						CompareColorsSnippetToVertex(CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.CompareMeshVertexColorsToColorSnippets, CompareColorSnippetColorArray, currentLODVertex, colorFromLOD[currentLODVertex], compareColorsToSnippet_AmountOfVerticesConsidered_AfterApplyingColors, compareColorSnippetMatchingPercent_AfterApplyingColors);
					}
				}



				if (CalculateColorsInfoGlobal.IsSkeletalMeshTask && shouldAffectPhysicsOnSectionsCloth) {

					// Affecting Cloth Physics is exclusive to UE5 and ChaosCloth
#if ENGINE_MAJOR_VERSION == 5

					clothSectionTotalRedVertexColor += colorFromLOD[currentLODVertex].R;
					clothSectionTotalGreenVertexColor += colorFromLOD[currentLODVertex].G;
					clothSectionTotalBlueVertexColor += colorFromLOD[currentLODVertex].B;
					clothSectionTotalAlphaVertexColor += colorFromLOD[currentLODVertex].A;
#endif
				}


				// Serializes each element while we're looping through the verts, so we don't have to loop through them again for the LOD after
				if (CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeSerializedVertexColorData) {

					if ((CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeVertexDataOnlyForLOD0 && currentLOD == 0) || !CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeVertexDataOnlyForLOD0)
						GetSerializedVertexColor(totalAmountOfVertsAtLOD, colorFromLOD[currentLODVertex], currentLODVertex, serializedColorData);
				}


				// Finished Sections Vertex Loop
				currentLODVertex++;
			}


			//-----------  FINISHED LOOPING THROUGH VERTICES - END OF SECTION LOOP ----------- //


			// Cloth Physics
			if (CalculateColorsInfoGlobal.IsSkeletalMeshTask && shouldAffectPhysicsOnSectionsCloth) {

				GetClothSectionAverageColor(skeletalMeshRenderSection, shouldAffectPhysicsOnSectionsCloth, sectionMaxAmountOfVerts, clothSectionTotalRedVertexColor, clothSectionTotalGreenVertexColor, clothSectionTotalBlueVertexColor, clothSectionTotalAlphaVertexColor, cloth_AverageColor);
			}


			// When Finished looping through section we go through the physics surface result for this section, if we've gotten it earlier in a previous section, or perhaps several vertex color channels has the same physics surface registered, then we add them up. 
			if (CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeAmountOfPaintedColorsOfEachChannel.IncludePhysicsSurfaceResultOfEachChannel || CalculateColorsInfoGlobal.WithinAreaSettings.IncludeAmountOfColorsOfEachChannelWithinArea.IncludePhysicsSurfaceResultOfEachChannel) {

				ResolveAmountOfColorsOfEachPhysicsSurfaceForSection(currentSection, registeredPhysicsSurfacesAtMaterial, physicsSurfaceResultForSection_BeforeAddingColors, amountOfPaintedColorsOfEachChannel_BeforeApplyingColor, physicsSurfaceResultForSection_AfterApplyingColors, amountOfPaintedColorsOfEachChannel_AfterApplyingColors, physicsSurfaceResultForSection_WithinArea_BeforeApplyingColors, amountOfPaintedColorsOfEachChannel_WithinArea_BeforeApplyingColors, physicsSurfaceResultForSection_WithinArea_AfterApplyingColors, amountOfPaintedColorsOfEachChannel_WithinArea_AfterApplyingColors);
			}


			// If gotten closest vertex data using the optimization setting then we make sure we break from section loop as well so we don't go through them all. 
			if (acceptClosestVertexDataIfVertexIsWithinMinRange_GotClosestVertex) {

				break;
			}
		}


		//-----------  FINISHED LOOPING THROUGH SECTIONS - END OF LOD LOOP ----------- //


		if (CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeSerializedVertexColorData) {

			if ((CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeVertexDataOnlyForLOD0 && currentLOD == 0) || !CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeVertexDataOnlyForLOD0) {

				FRVPDPSerializedColorsPerLODInfo serializedColors;
				serializedColors.Lod = currentLOD;
				serializedColors.ColorsAtLODAsJSonString = serializedColorData;

				CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD[currentLOD].SerializedVertexColorsData = serializedColors;
			}
		}


		if (currentLOD == 0) {


			FLinearColor estimatedColorAtHitLocation_BeforeApplyingColors = FLinearColor(ForceInitToZero);
			FVector estimatedColorAtHitLocationInWorldSpace = FVector(ForceInitToZero);

			if (CalculateColorsInfoGlobal.IsPaintAtLocationTask()) {

				// Closest Vertex Data
				if (paintAtLocation_ClosestVertex >= 0) {


					const TArray<FVector2D> closestVertexUV = GetClosestVertexUVs(CalculateColorsInfoGlobal.GetVertexPaintComponent(), paintAtLocation_ClosestVertex);

					// UVs
					if (closestVertexUV.Num() > 0) {

						for (int32 i = 0; i < closestVertexUV.Num(); i++) {

							const FVector2D closestVertexUV_Painting = closestVertexUV[i];

							CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Paint at Location - Closest Vertex UV at Channel: %s  -  TexCoord: %i"), *closestVertexUV_Painting.ToString(), i), FColor::Cyan));
						}
					}


					CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Paint at Location - Closest Vertex Index: %i  -  Closest Section: %i"), paintAtLocation_ClosestVertex, paintAtLocation_ClosestSection), FColor::Cyan));


					FRVPDPClosestVertexDataResults closestVertexDataResult;
					closestVertexDataResult.ClosestVertexDataSuccessfullyAcquired = true;
					closestVertexDataResult.ClosestVertexGeneralInfo.ClosestVertexMaterial = GetMaterialAtSection(paintAtLocation_ClosestSection, skelMeshLODRenderData);
					closestVertexDataResult.ClosestVertexGeneralInfo.ClosestSection = paintAtLocation_ClosestSection;
					closestVertexDataResult.ClosestVertexGeneralInfo.ClosestVertexUVAtEachUVChannel = closestVertexUV;
					closestVertexDataResult = GetClosestVertexDataResult(CalculateColorsInfoGlobal.GetVertexPaintComponent(), closestVertexDataResult, paintAtLocation_ClosestVertex, colorFromLOD[paintAtLocation_ClosestVertex], positionsInWorldSpaceFromLOD[paintAtLocation_ClosestVertex], normalsFromLOD[paintAtLocation_ClosestVertex]);
					CalculateColorsInfoGlobal.PaintAtLocation_ClosestVertexDataResult = closestVertexDataResult;

					PrintClosestVertexColorResults(closestVertexDataResult);


					// Average Color Within Area
					if (paintAtLocation_AmountOfVerticesWithinArea > 0) {

						FColor averageColorWithinArea = FColor(ForceInitToZero);
						averageColorWithinArea.R = paintAtLocation_AverageColorInAreaOfEffect_Red / paintAtLocation_AmountOfVerticesWithinArea;
						averageColorWithinArea.G = paintAtLocation_AverageColorInAreaOfEffect_Green / paintAtLocation_AmountOfVerticesWithinArea;
						averageColorWithinArea.B = paintAtLocation_AverageColorInAreaOfEffect_Blue / paintAtLocation_AmountOfVerticesWithinArea;
						averageColorWithinArea.A = paintAtLocation_AverageColorInAreaOfEffect_Alpha / paintAtLocation_AmountOfVerticesWithinArea;

						FRVPDPAverageColorInAreaInfo averageColorInArea;
						averageColorInArea.GotAvarageVertexColorsWithinAreaOfEffect = true;
						averageColorInArea.AvarageVertexColorsWithinAreaOfEffect = UVertexPaintFunctionLibrary::ReliableFColorToFLinearColor(averageColorWithinArea);

						if (closestVertexDataResult.ClosestVertexGeneralInfo.ClosestVertexMaterial) {

							averageColorInArea.AvaragePhysicalSurfaceInfoBasedOffTheClosestVertexMaterial = UVertexPaintFunctionLibrary::GetVertexColorPhysicsSurfaceDataFromMaterial(CalculateColorsInfoGlobal.GetVertexPaintComponent(), averageColorWithinArea, closestVertexDataResult.ClosestVertexGeneralInfo.ClosestVertexMaterial);
						}

						CalculateColorsInfoGlobal.PaintAtLocation_AverageColorInArea = averageColorInArea;

						CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Paint at Location - Average Color: %s"), *averageColorInArea.AvarageVertexColorsWithinAreaOfEffect.ToString()), FColor::Cyan));
					}


					// Estimated Color at Hit Location
					if (CalculateColorsInfoGlobal.EstimatedColorAtHitLocationSettings.GetEstimatedColorAtHitLocation) {


						FLinearColor estimatedColorAtHitLocation_AfterApplyingColors = FLinearColor(ForceInitToZero);
						int closestVertexToLerpTowardIndex_Paint = -1;

						if (estimatedColorToHitLocation_DirectionFromHitToClosestVerticesInfo_Paint.Num() > 0) {


							closestVertexToLerpTowardIndex_Paint = GetEstimatedColorToHitLocationVertexToLerpColorToward(estimatedColorToHitLocation_DirectionFromHitToClosestVerticesInfo_Paint, CalculateColorsInfoGlobal.PaintAtLocation_EstimatedColorAtHitLocationResult.ClosestVerticesWorldLocations, estimatedColorToHitLocation_DirectionToClosestVertex_Paint, paintAtLocation_ClosestVertex, 0.2f);


							if (closestVertexToLerpTowardIndex_Paint >= 0) {


								CalculateColorsInfoGlobal.PaintAtLocation_EstimatedColorAtHitLocationResult.VertexWeEstimatedTowardWorldLocation = positionsInWorldSpaceFromLOD[closestVertexToLerpTowardIndex_Paint];

								GetEstimatedColorToHitLocationValues(closestVertexToLerpTowardIndex_Paint, colorFromLOD[paintAtLocation_ClosestVertex], positionsInWorldSpaceFromLOD[paintAtLocation_ClosestVertex], paintAtLocation_HitLocationInWorldSpace, colorFromLOD[closestVertexToLerpTowardIndex_Paint], positionsInWorldSpaceFromLOD[closestVertexToLerpTowardIndex_Paint], estimatedColorAtHitLocation_AfterApplyingColors, estimatedColorAtHitLocationInWorldSpace);

								// Also sets a EstimatedColorAtHitLocation_BeforeApplyingColors_Paint which is used if we're also running a detection before applying colors but not on custom hit result but based on the same hit Location as the paint job.
								if (CalculateColorsInfoGlobal.IsDetectCombo) {

									GetEstimatedColorToHitLocationValues(closestVertexToLerpTowardIndex_Paint, defaultColorFromLOD[paintAtLocation_ClosestVertex], positionsInWorldSpaceFromLOD[paintAtLocation_ClosestVertex], paintAtLocation_HitLocationInWorldSpace, defaultColorFromLOD[closestVertexToLerpTowardIndex_Paint], positionsInWorldSpaceFromLOD[closestVertexToLerpTowardIndex_Paint], estimatedColorAtHitLocation_BeforeApplyingColors, estimatedColorAtHitLocationInWorldSpace);
								}
							}
						}


						if (closestVertexToLerpTowardIndex_Paint >= 0) {

							CalculateColorsInfoGlobal.PaintAtLocation_EstimatedColorAtHitLocationResult.EstimatedColorAtHitLocationDataSuccessfullyAcquired = true;
							CalculateColorsInfoGlobal.PaintAtLocation_EstimatedColorAtHitLocationResult.EstimatedColorAtHitLocation = estimatedColorAtHitLocation_AfterApplyingColors;

							CalculateColorsInfoGlobal.PaintAtLocation_EstimatedColorAtHitLocationResult.EstimatedPhysicalSurfaceInfoAtHitLocation = UVertexPaintFunctionLibrary::GetVertexColorPhysicsSurfaceDataFromMaterial(CalculateColorsInfoGlobal.TaskFundamentalSettings.GetTaskWorld(), UVertexPaintFunctionLibrary::ReliableFLinearToFColor(estimatedColorAtHitLocation_AfterApplyingColors), CalculateColorsInfoGlobal.PaintAtLocation_ClosestVertexDataResult.ClosestVertexGeneralInfo.ClosestVertexMaterial);

							CalculateColorsInfoGlobal.PaintAtLocation_EstimatedColorAtHitLocationResult.EstimatedColorAtWorldLocation = estimatedColorAtHitLocationInWorldSpace;
						}

						else if (CalculateColorsInfoGlobal.EstimatedColorAtHitLocationSettings.OnlyGetIfMeshHasMaxAmountOfVertices && CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD.IsValidIndex(0) && CalculateColorsInfoGlobal.EstimatedColorAtHitLocationSettings.MaxAmountOfVertices < CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD[0].AmountOfVerticesAtLOD) {

							CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Paint at  Location - Couldn't get Estimated Color at Hit Location since the Mesh has to many Vertices. You can increase the amount acceptable in the struct settings, however, it's only useful when the Mesh has few vertices, if Meshes has high vertex density then you don't need to use this but can use the regular closest vertex data.")), FColor::Orange));
						}
					}
				}
			}


			if (CalculateColorsInfoGlobal.IsGetClosestVertexDataTask() || (CalculateColorsInfoGlobal.IsPaintAtLocationTask() && CalculateColorsInfoGlobal.IsDetectCombo)) {


				// Closest Vertex Data
				if (getClosestVertexData_ClosestVertex >= 0) {


					// UVs
					const TArray<FVector2D> closestVertexUV = GetClosestVertexUVs(CalculateColorsInfoGlobal.GetVertexPaintComponent(), getClosestVertexData_ClosestVertex);

					if (closestVertexUV.Num() > 0) {

						for (int32 i = 0; i < closestVertexUV.Num(); i++) {

							const FVector2D closestVertexUV_Detecting = closestVertexUV[i];

							CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Get Closest Vertex Data - Closest Vertex UV at Channel: %s  -  TexCoord: %i"), *closestVertexUV_Detecting.ToString(), i), FColor::Cyan));
						}
					}


					CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Get Closest Vertex Data - Closest Vertex Index: %i  -  Closest Section: %i"), getClosestVertexData_ClosestVertex, getClosestVertexData_ClosestSection), FColor::Cyan));


					FRVPDPClosestVertexDataResults closestVertexDataResult;
					closestVertexDataResult.ClosestVertexDataSuccessfullyAcquired = true;
					closestVertexDataResult.ClosestVertexGeneralInfo.ClosestVertexMaterial = GetMaterialAtSection(getClosestVertexData_ClosestSection, skelMeshLODRenderData);
					closestVertexDataResult.ClosestVertexGeneralInfo.ClosestSection = getClosestVertexData_ClosestSection;
					closestVertexDataResult.ClosestVertexGeneralInfo.ClosestVertexUVAtEachUVChannel = closestVertexUV;
					closestVertexDataResult = GetClosestVertexDataResult(CalculateColorsInfoGlobal.GetVertexPaintComponent(), closestVertexDataResult, getClosestVertexData_ClosestVertex, defaultColorFromLOD[getClosestVertexData_ClosestVertex], positionsInWorldSpaceFromLOD[getClosestVertexData_ClosestVertex], normalsFromLOD[getClosestVertexData_ClosestVertex]);
					CalculateColorsInfoGlobal.GetClosestVertexDataResult = closestVertexDataResult;

					PrintClosestVertexColorResults(CalculateColorsInfoGlobal.GetClosestVertexDataResult);


					// Average Color Within Area
					if (getClosestVertexData_AmountOfVerticesWithinArea > 0) {

						FColor averageColorWithinArea = FColor(ForceInitToZero);
						averageColorWithinArea.R = getClosestVertexData_AverageColorWithinAreaOfEffect_BeforeApplyingColors_Red / getClosestVertexData_AmountOfVerticesWithinArea;
						averageColorWithinArea.G = getClosestVertexData_AverageColorWithinAreaOfEffect_BeforeApplyingColors_Green / getClosestVertexData_AmountOfVerticesWithinArea;
						averageColorWithinArea.B = getClosestVertexData_AverageColorWithinAreaOfEffect_BeforeApplyingColors_Blue / getClosestVertexData_AmountOfVerticesWithinArea;
						averageColorWithinArea.A = getClosestVertexData_AverageColorWithinAreaOfEffect_BeforeApplyingColors_Alpha / getClosestVertexData_AmountOfVerticesWithinArea;

						FRVPDPAverageColorInAreaInfo averageColorInArea;
						averageColorInArea.GotAvarageVertexColorsWithinAreaOfEffect = true;
						averageColorInArea.AvarageVertexColorsWithinAreaOfEffect = UVertexPaintFunctionLibrary::ReliableFColorToFLinearColor(averageColorWithinArea);

						if (closestVertexDataResult.ClosestVertexGeneralInfo.ClosestVertexMaterial) {

							averageColorInArea.AvaragePhysicalSurfaceInfoBasedOffTheClosestVertexMaterial = UVertexPaintFunctionLibrary::GetVertexColorPhysicsSurfaceDataFromMaterial(CalculateColorsInfoGlobal.GetVertexPaintComponent(), averageColorWithinArea, closestVertexDataResult.ClosestVertexGeneralInfo.ClosestVertexMaterial);
						}

						CalculateColorsInfoGlobal.GetClosestVertexData_AverageColorInArea = averageColorInArea;

						CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Get Closest Vertex Data - Average Color: %s"), *averageColorInArea.AvarageVertexColorsWithinAreaOfEffect.ToString()), FColor::Cyan));
					}


					// Estimated Color at Hit Location
					if (CalculateColorsInfoGlobal.EstimatedColorAtHitLocationSettings.GetEstimatedColorAtHitLocation) {


						int closestVertexToLerpTowardIndex_Detect = -1;

						// If just a Get Closest Vertex Data and not a combo, or a combo but uses custom hit settings, then uses the results of the detection directionFromHitToClosestVerticesInfo_Detect TMap, since it can have a different nearby vertices than the paint hit Location
						if (!CalculateColorsInfoGlobal.IsDetectCombo || CalculateColorsInfoGlobal.PaintAtLocationSettings.GetClosestVertexDataCombo.UseCustomHitSettings) {


							if (estimatedColorToHitLocation_DirectionFromHitToClosestVerticesInfo_Detect.Num() > 0) {

								closestVertexToLerpTowardIndex_Detect = GetEstimatedColorToHitLocationVertexToLerpColorToward(estimatedColorToHitLocation_DirectionFromHitToClosestVerticesInfo_Detect, CalculateColorsInfoGlobal.GetClosestVertexData_EstimatedColorAtHitLocationResult.ClosestVerticesWorldLocations, estimatedColorToHitLocation_DirectionToClosestVertex_Detect, getClosestVertexData_ClosestVertex, 0.2f);


								if (closestVertexToLerpTowardIndex_Detect >= 0) {


									CalculateColorsInfoGlobal.GetClosestVertexData_EstimatedColorAtHitLocationResult.VertexWeEstimatedTowardWorldLocation = positionsInWorldSpaceFromLOD[closestVertexToLerpTowardIndex_Detect];

									GetEstimatedColorToHitLocationValues(closestVertexToLerpTowardIndex_Detect, defaultColorFromLOD[getClosestVertexData_ClosestVertex], positionsInWorldSpaceFromLOD[getClosestVertexData_ClosestVertex], getClosestVertexData_HitLocationInWorldSpace, defaultColorFromLOD[closestVertexToLerpTowardIndex_Detect], positionsInWorldSpaceFromLOD[closestVertexToLerpTowardIndex_Detect], estimatedColorAtHitLocation_BeforeApplyingColors, estimatedColorAtHitLocationInWorldSpace);
								}
							}
						}


						if (closestVertexToLerpTowardIndex_Detect >= 0) {
							
							CalculateColorsInfoGlobal.GetClosestVertexData_EstimatedColorAtHitLocationResult.EstimatedColorAtHitLocationDataSuccessfullyAcquired = true;
							CalculateColorsInfoGlobal.GetClosestVertexData_EstimatedColorAtHitLocationResult.EstimatedColorAtHitLocation = estimatedColorAtHitLocation_BeforeApplyingColors;

							CalculateColorsInfoGlobal.GetClosestVertexData_EstimatedColorAtHitLocationResult.EstimatedPhysicalSurfaceInfoAtHitLocation = UVertexPaintFunctionLibrary::GetVertexColorPhysicsSurfaceDataFromMaterial(CalculateColorsInfoGlobal.TaskFundamentalSettings.GetTaskWorld(), UVertexPaintFunctionLibrary::ReliableFLinearToFColor(estimatedColorAtHitLocation_BeforeApplyingColors), CalculateColorsInfoGlobal.GetClosestVertexDataResult.ClosestVertexGeneralInfo.ClosestVertexMaterial);

							CalculateColorsInfoGlobal.GetClosestVertexData_EstimatedColorAtHitLocationResult.EstimatedColorAtWorldLocation = estimatedColorAtHitLocationInWorldSpace;
						}

						else if (CalculateColorsInfoGlobal.EstimatedColorAtHitLocationSettings.OnlyGetIfMeshHasMaxAmountOfVertices && CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD.IsValidIndex(0) && CalculateColorsInfoGlobal.EstimatedColorAtHitLocationSettings.MaxAmountOfVertices < CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD[0].AmountOfVerticesAtLOD) {

							CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Get Closest Vertex Data - Couldn't get Estimated Color at Hit Location since the Mesh has to many Vertices. You can increase the amount acceptable in the struct settings, however, it's only useful when the Mesh has few vertices, if Meshes has high vertex density then you don't need to use this but can use the regular closest vertex data.")), FColor::Orange));
						}

					}
				}
			}


#if ENGINE_MAJOR_VERSION == 5

			if (CalculateColorsInfoGlobal.IsDynamicMeshTask) {

				CalculateColorsInfoGlobal.DynamicMeshComponentVertexColors = colorFromLOD;
			}

#endif


			if (compareColorsToArray) {


				if (CalculateColorsInfoGlobal.IsDetectCombo) {

					compareMeshVertexColorsToColorArrayResult_BeforeApplyingColors = CalculateColorsInfoGlobal.TaskResult.CompareMeshVertexColorsToColorArrayResult = GetMatchingColorArrayResults(compareColorsToArray_AmountOfVerticesConsidered_BeforeApplyingColors, compareMeshVertexColorsToColorArrayMatchingPercent_BeforeApplyingColors);
				}
				
				CalculateColorsInfoGlobal.TaskResult.CompareMeshVertexColorsToColorArrayResult = GetMatchingColorArrayResults(compareColorsToArray_AmountOfVerticesConsidered_AfterApplyingColors, compareMeshVertexColorsToColorArrayMatchingPercent_AfterApplyingColors);
			}


			if (compareColorsToColorSnippet) {


				if (CalculateColorsInfoGlobal.IsDetectCombo) {

					compareMeshVertexColorsToColorSnippetResult_BeforeApplyingColors = GetMatchingColorSnippetsResults(CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.CompareMeshVertexColorsToColorSnippets, CompareColorSnippetColorArray, compareColorsToSnippet_AmountOfVerticesConsidered_BeforeApplyingColors, compareColorSnippetMatchingPercent_BeforeApplyingColors);
				}

				CalculateColorsInfoGlobal.TaskResult.CompareMeshVertexColorsToColorSnippetResult = GetMatchingColorSnippetsResults(CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.CompareMeshVertexColorsToColorSnippets, CompareColorSnippetColorArray, compareColorsToSnippet_AmountOfVerticesConsidered_AfterApplyingColors, compareColorSnippetMatchingPercent_AfterApplyingColors);
			}


			if (CalculateColorsInfoGlobal.TaskFundamentalSettings.DebugSettings.PrintLogsToScreen || CalculateColorsInfoGlobal.TaskFundamentalSettings.DebugSettings.PrintLogsToOutputLog) {

				CalculateColorsInfoGlobal.ColorsOfEachChannelResultsAtMinAmount_AboveZeroBeforeApplyingColor_Debug = UVertexPaintFunctionLibrary::ConsolidateColorsOfEachChannel(amountOfPaintedColorsOfEachChannelAbove0_BeforeApplyingColor_Debug, totalAmountOfVertsAtLOD);
			}


			if (CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeAmountOfPaintedColorsOfEachChannel.IncludeVertexColorChannelResultOfEachChannel) {

				amountOfPaintedColorsOfEachChannel_BeforeApplyingColor = UVertexPaintFunctionLibrary::ConsolidateColorsOfEachChannel(amountOfPaintedColorsOfEachChannel_BeforeApplyingColor, totalAmountOfVertsAtLOD);
				amountOfPaintedColorsOfEachChannel_AfterApplyingColors = UVertexPaintFunctionLibrary::ConsolidateColorsOfEachChannel(amountOfPaintedColorsOfEachChannel_AfterApplyingColors, totalAmountOfVertsAtLOD);

				CalculateColorsInfoGlobal.ColorsOfEachChannelResultsAtMinAmount_BeforeApplyingColor = amountOfPaintedColorsOfEachChannel_BeforeApplyingColor;
				CalculateColorsInfoGlobal.ColorsOfEachChannelResultsAtMinAmount_AfterApplyingColor = amountOfPaintedColorsOfEachChannel_AfterApplyingColors;
			}


			if (CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeAmountOfPaintedColorsOfEachChannel.IncludePhysicsSurfaceResultOfEachChannel) {

				amountOfPaintedColorsOfEachChannel_BeforeApplyingColor = UVertexPaintFunctionLibrary::ConsolidatePhysicsSurfaceResult(amountOfPaintedColorsOfEachChannel_BeforeApplyingColor, totalAmountOfVertsAtLOD);
				amountOfPaintedColorsOfEachChannel_AfterApplyingColors = UVertexPaintFunctionLibrary::ConsolidatePhysicsSurfaceResult(amountOfPaintedColorsOfEachChannel_AfterApplyingColors, totalAmountOfVertsAtLOD);

				CalculateColorsInfoGlobal.ColorsOfEachChannelResultsAtMinAmount_BeforeApplyingColor = amountOfPaintedColorsOfEachChannel_BeforeApplyingColor;
				CalculateColorsInfoGlobal.ColorsOfEachChannelResultsAtMinAmount_AfterApplyingColor = amountOfPaintedColorsOfEachChannel_AfterApplyingColors;
			}
		}
		

		// Setting MeshVertexData colors, position and normal arrays
		CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD[currentLOD].Lod = currentLOD;


		// If set to Propogate LOD0 to All LODs then we need the vertex positions, normals etc. for all LODs in the PropogateToLODs calculation
		if ((CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeVertexDataOnlyForLOD0 && currentLOD == 0) || !CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeVertexDataOnlyForLOD0 || CalculateColorsInfoGlobal.PropogateLOD0ToAllLODsFromStoredData || CalculateColorsInfoGlobal.PropogateLOD0ToAllLODsUsingOctree) {


			if (CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeVertexColorData || CalculateColorsInfoGlobal.PropogateLOD0ToAllLODsFromStoredData || CalculateColorsInfoGlobal.PropogateLOD0ToAllLODsUsingOctree) {

				CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD[currentLOD].MeshVertexColorsPerLODArray = colorFromLOD;

				if (CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeVertexColorData && CalculateColorsInfoGlobal.IsDetectCombo)
					detectComboMeshVertexData.MeshDataPerLOD[currentLOD].MeshVertexColorsPerLODArray = defaultColorFromLOD;
			}

			if (CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeVertexPositionData || CalculateColorsInfoGlobal.PropogateLOD0ToAllLODsUsingOctree)
				CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD[currentLOD].MeshVertexPositionsInComponentSpacePerLODArray = positionsInComponentLocalSpaceFromLOD;

			if (CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeVertexNormalData || CalculateColorsInfoGlobal.PropogateLOD0ToAllLODsUsingOctree)
				CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD[currentLOD].MeshVertexNormalsPerLODArray = normalsFromLOD;

			if (CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeVertexIndexes)
				CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD[currentLOD].MeshVertexIndexes = vertexIndexesFromLOD;
		}



		ResolveWithinAreaResults(currentLOD, withinArea_AmountOfVerticesWithinArea, withinArea_ColorFromLOD_BeforeApplyingColors, withinArea_ColorFromLOD_AfterApplyingColors, withinArea_PositionsInComponentLocalSpaceFromLOD, withinArea_NormalsFromLOD, withinArea_VertexIndexesFromLOD, amountOfPaintedColorsOfEachChannel_WithinArea_BeforeApplyingColors, amountOfPaintedColorsOfEachChannel_WithinArea_AfterApplyingColors, CalculateColorsInfoGlobal.WithinArea_Results_BeforeApplyingColors, CalculateColorsInfoGlobal.WithinArea_Results_AfterApplyingColors);


		if (CalculateColorsInfoGlobal.IsSkeletalMeshTask) {


			if (currentLOD == 0) {

				if (CalculateColorsInfoGlobal.IsDetectCombo) {

					ResolveSkeletalMeshBoneColors(currentLOD, boneIndexColorsBeforeApplyingColors, colorsOfEachBoneBeforeApplyingColorsResult.ColorsOfEachBone, colorsOfEachBoneBeforeApplyingColorsResult.SuccessFullyGotColorsForEachBone);
				}

				ResolveSkeletalMeshBoneColors(currentLOD, boneIndexColorsAfterApplyingColors, CalculateColorsInfoGlobal.TaskResult.VertexColorsOnEachBone.ColorsOfEachBone, CalculateColorsInfoGlobal.TaskResult.VertexColorsOnEachBone.SuccessFullyGotColorsForEachBone);
			}


			ResolveChaosClothPhysics(CalculateColorsInfoGlobal.GetVertexPaintSkelComponent(), CalculateColorsInfoGlobal.PaintTaskSettings.AffectClothPhysics, CalculateColorsInfoGlobal.PaintTaskResult.AnyVertexColorGotChanged, skelMeshLODRenderData, currentLOD, cloth_PhysicsSettings, cloth_AverageColor, CalculateColorsInfoGlobal.ClothPhysicsSettings);
		}


		if (CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD.Num() > 1 && CalculateColorsInfoGlobal.PropogateLOD0ToAllLODsUsingOctree) {

			propogateToLODs_CompleteLODsBaseBounds[currentLOD] = propogateToLODs_CurrentLODBaseBounds;
		}
	}


	//-----------  FINISHED LOOPING THROUGH ALL LODs ----------- //


	if (CalculateColorsInfoGlobal.IsDetectCombo) {


		CalculateColorsInfoGlobal.DetectComboTaskResults = CalculateColorsInfoGlobal.TaskResult;

		CalculateColorsInfoGlobal.DetectComboTaskResults.AmountOfPaintedColorsOfEachChannel = amountOfPaintedColorsOfEachChannel_BeforeApplyingColor;
		CalculateColorsInfoGlobal.DetectComboTaskResults.VertexColorsOnEachBone = colorsOfEachBoneBeforeApplyingColorsResult;
		CalculateColorsInfoGlobal.DetectComboTaskResults.CompareMeshVertexColorsToColorSnippetResult = compareMeshVertexColorsToColorSnippetResult_BeforeApplyingColors;
		CalculateColorsInfoGlobal.DetectComboTaskResults.CompareMeshVertexColorsToColorArrayResult = compareMeshVertexColorsToColorArrayResult_BeforeApplyingColors;

		for (int i = 0; i < CalculateColorsInfoGlobal.DetectComboTaskResults.MeshVertexData.MeshDataPerLOD.Num(); i++) {

			if (detectComboMeshVertexData.MeshDataPerLOD.IsValidIndex(i))
				CalculateColorsInfoGlobal.DetectComboTaskResults.MeshVertexData.MeshDataPerLOD[i].MeshVertexColorsPerLODArray = detectComboMeshVertexData.MeshDataPerLOD[i].MeshVertexColorsPerLODArray;
		}
	}


	CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Calculate Colors Task - Finished Looping through all LODs")), FColor::Cyan));


	// If > 0 LODs and paint entire mesh at random vertices we can propogate LOD0 Colors to the rest of the LODs so the random vertices that was selected gets the closest LOD1 colors painted as well etc. so they match. 
	if (CalculateColorsInfoGlobal.TaskResult.MeshVertexData.MeshDataPerLOD.Num() > 1 && CalculateColorsInfoGlobal.PropogateLOD0ToAllLODsUsingOctree) {


		CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Calculate Colors Task - Propogating LOD0 Colors to LODs using Modified Engine Method on Mesh Component: %s"), *CalculateColorsInfoGlobal.VertexPaintComponentName), FColor::Cyan));


		TMap<int32, FRVPDPPropogateToLODsInfo> propogateToLODInfo;

		UVertexPaintFunctionLibrary::PropagateLOD0VertexColorsToLODs(propogateToLODs_PaintedVerticesAtLOD0, CalculateColorsInfoGlobal.TaskResult.MeshVertexData, propogateToLODs_CompleteLODsBaseBounds, CalculateColorsInfoGlobal.NewColorVertexBuffersPerLOD, false, propogateToLODInfo);
	}


	// Another final check 
	if (!UVertexPaintFunctionLibrary::IsWorldValid(CalculateColorsInfoGlobal.TaskFundamentalSettings.GetTaskWorld())) return false;
	if (!IsMeshStillValid(CalculateColorsInfoGlobal)) return false;


	return true;
}


//-------------------------------------------------------

// Should Paint Job Run Calculate Colors To Apply

bool FVertexPaintCalculateColorsTask::ShouldPaintJobRunCalculateColorsToApply(const FRVPDPApplyColorSetting& PaintColorSettings, const FRVPDPPaintTaskSettings& PaintSettings, const FRVPDPPaintDirectlyTaskSettings& PaintColorsDirectlySettings) {

	if (PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnRedChannel.PaintLimit.UsePaintLimits || PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.PaintLimit.UsePaintLimits || PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.PaintLimit.UsePaintLimits || PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.PaintLimit.UsePaintLimits)
		return true;

	if (PaintColorsDirectlySettings.VertexColorRedChannelsLimit.UsePaintLimits || PaintColorsDirectlySettings.VertexColorGreenChannelsLimit.UsePaintLimits || PaintColorsDirectlySettings.VertexColorBlueChannelsLimit.UsePaintLimits || PaintColorsDirectlySettings.VertexColorAlphaChannelsLimit.UsePaintLimits)
		return true;

	if (PaintColorSettings.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.ApplyVertexColorUsingPhysicsSurface)
		return true;

	return UVertexPaintFunctionLibrary::ShouldTaskLoopThroughAllVerticesOnLOD(PaintSettings.CallbackSettings, PaintSettings.GetMeshComponent(), PaintSettings.OverrideVertexColorsToApplySettings);
}


//-------------------------------------------------------

// Is Task Still Valid

bool FVertexPaintCalculateColorsTask::IsTaskStillValid(const FRVPDPCalculateColorsInfo& CalculateColorsInfo) const {

	// Check should run on the game thread so we can check correct task queues etc. 
	if (!IsInGameThread()) return false;
	if (!UVertexPaintFunctionLibrary::IsWorldValid(CalculateColorsInfo.TaskFundamentalSettings.GetTaskWorld())) return false;
	if (!IsMeshStillValid(CalculateColorsInfo)) return false;


	if (CalculateColorsInfo.IsPaintTask) {

		if (!UVertexPaintFunctionLibrary::DoesPaintTaskQueueContainID(CalculateColorsInfo.TaskFundamentalSettings.GetTaskWorld(), CalculateColorsInfo.TaskID)) {

			return false;
		}
	}

	else if (CalculateColorsInfo.IsDetectTask) {

		if (!UVertexPaintFunctionLibrary::DoesDetectTaskQueueContainID(CalculateColorsInfo.TaskFundamentalSettings.GetTaskWorld(), CalculateColorsInfo.TaskID)) {

			return false;
		}
	}

	return true;
}


//-------------------------------------------------------

// Modified Get Typed Skinned Vertex Position - If no clothing then gets the position in component space using our own GetTypedSkinnedVertexPosition, which is otherwise called on each vertex if running ComputeSkinnedPositions. The reason why we've made our own here is so we only have to loop through a mesh sections and vertices once, instead of atleast twice if running ComputeSkinnedPositions, since it also loops through sections and vertices, then if we where to use that here we would have to loop through it again. 


#if ENGINE_MAJOR_VERSION == 4

FVector FVertexPaintCalculateColorsTask::Modified_GetTypedSkinnedVertexPosition(FSkelMeshRenderSection * Section, const FPositionVertexBuffer & PositionVertexBuffer, const FSkinWeightVertexBuffer * SkinWeightVertexBuffer, const int32 VertIndex, const TArray<FMatrix>&RefToLocals, uint32 & BoneIndex, float& BoneWeight) {

	BoneIndex = 0;

#elif ENGINE_MAJOR_VERSION == 5

FVector FVertexPaintCalculateColorsTask::Modified_GetTypedSkinnedVertexPosition(FSkelMeshRenderSection * Section, const FPositionVertexBuffer & PositionVertexBuffer, const FSkinWeightVertexBuffer * SkinWeightVertexBuffer, const int32 VertIndex, const TArray<FMatrix44f>&RefToLocals, uint32 & BoneIndex, float& BoneWeight) {

#endif

	FVector vertexPos = FVector(ForceInitToZero);
	BoneWeight = 0;

	// Do soft skinning for this vertex.
	int32 bufferVertIndex = Section->GetVertexBufferIndex() + VertIndex;
	int32 maxBoneInfluences = SkinWeightVertexBuffer->GetMaxBoneInfluences();


#if !PLATFORM_LITTLE_ENDIAN
	// uint8[] elements in LOD.VertexBufferGPUSkin have been swapped for VET_UBYTE4 vertex stream use
	for (int32 InfluenceIndex = MAX_INFLUENCES - 1; InfluenceIndex >= MAX_INFLUENCES - MaxBoneInfluences; InfluenceIndex--)
#else
	for (int32 InfluenceIndex = 0; InfluenceIndex < maxBoneInfluences; InfluenceIndex++)
#endif
	{

		const uint32 boneIndexFromBuffer = SkinWeightVertexBuffer->GetBoneIndex(bufferVertIndex, InfluenceIndex);
		
		if (Section->BoneMap.IsValidIndex(boneIndexFromBuffer)) {

			if (InfluenceIndex == 0)
				BoneIndex = Section->BoneMap[boneIndexFromBuffer];
		}

		float boneWeightFromBuffer = static_cast<float>(SkinWeightVertexBuffer->GetBoneWeight(bufferVertIndex, InfluenceIndex) / 255.0f);


#if ENGINE_MAJOR_VERSION == 5 
#if ENGINE_MINOR_VERSION >= 2

		// From 5.2 it seems it uses AnimationCore::InvMaxRawBoneWeightFloat to calculate the weight
		boneWeightFromBuffer = static_cast<float>(SkinWeightVertexBuffer->GetBoneWeight(bufferVertIndex, InfluenceIndex) * UE::AnimationCore::InvMaxRawBoneWeightFloat);

#endif
#endif


		BoneWeight += boneWeightFromBuffer;
		

		// If ref to locals then we can get the specific vertex position based on current pose
		if (RefToLocals.Num() > 0) {


			// Could get a crash here very rarely if i painted like crazy while switching meshes, skeletal meshes, this seems to have fixed that issue!
			if (bufferVertIndex < static_cast<int32>(PositionVertexBuffer.GetNumVertices()) && static_cast<int32>(BoneIndex) < RefToLocals.Num()) {

				vertexPos += static_cast<FVector>(RefToLocals[BoneIndex].TransformPosition(PositionVertexBuffer.VertexPosition(bufferVertIndex))) * boneWeightFromBuffer;
			}
		}

		// If no ref to locals then we can still get the default vertex position
		else {

			vertexPos += static_cast<FVector>(PositionVertexBuffer.VertexPosition(bufferVertIndex) * boneWeightFromBuffer);
		}
	}


	return vertexPos;
}


//-------------------------------------------------------

// Is Vertex Within Area

bool FVertexPaintCalculateColorsTask::IsVertexWithinArea(UPrimitiveComponent* ComponentToCheckIfWithin, const FVector & VertexWorldPosition, const FRVPDPComponentToCheckIfIsWithinAreaInfo& ComponentToCheckIfIsWithinInfo) const {


	const FVector vertexPositionInLocalSpace = UKismetMathLibrary::InverseTransformLocation(ComponentToCheckIfIsWithinInfo.ComponentWorldTransform, VertexWorldPosition);


	switch (ComponentToCheckIfIsWithinInfo.PaintWithinAreaShape) {

	case EPaintWithinAreaShape::IsSquareOrRectangleShape:


		// Is within X, Y and Z Relative Limits of the Shape, so even if it's a tilted rectangle we can still know if within area
		if (vertexPositionInLocalSpace.X >= ComponentToCheckIfIsWithinInfo.SquareOrRectangle_BackPosLocal.X && vertexPositionInLocalSpace.X <= ComponentToCheckIfIsWithinInfo.SquareOrRectangle_ForwardPosLocal.X) {

			if (vertexPositionInLocalSpace.Y >= ComponentToCheckIfIsWithinInfo.SquareOrRectangle_LeftPosLocal.Y && vertexPositionInLocalSpace.Y <= ComponentToCheckIfIsWithinInfo.SquareOrRectangle_RightPosLocal.Y) {

				if (vertexPositionInLocalSpace.Z >= ComponentToCheckIfIsWithinInfo.SquareOrRectangle_DownPosLocal.Z && vertexPositionInLocalSpace.Z <= ComponentToCheckIfIsWithinInfo.SquareOrRectangle_UpPosLocal.Z) {

					return true;
				}
			}
		}

		break;

	case EPaintWithinAreaShape::IsSphereShape:

		// If within Radius
		if ((ComponentToCheckIfIsWithinInfo.ComponentWorldTransform.GetLocation() - VertexWorldPosition).Size() <= ComponentToCheckIfIsWithinInfo.Sphere_Radius) {

			return true;
		}

		break;

		// Note this is very expensive, it will give a 100% check and work on meshes on any shapes unlike the simple Cube and Spheres below. 
	case EPaintWithinAreaShape::IsComplexShape:
		
		if (ComponentToCheckIfWithin) {

			bool isVertexWithinArea_ShouldTrace = false;
			FVector isVertexWithinArea_TraceEndLocation = VertexWorldPosition + FVector(0.001f, 0.001f, 0.001f);


			// If lansdcape then we also run the trace. This was necessary because the landscape bounds on the flat part didn't start until 50cm above it and it became difficult to make sure that the entire mesh got painted if it was below a flat part of the landscape. This just makes everything so much easier UX wise because otherwise we had to require the user to make a hole somewhere on the landscape to extend the bounds etc. 
			if (Cast<ALandscape>(ComponentToCheckIfWithin->GetOwner())) {

				// To check if we're within the landscape, we trace upwards, so if the vertex is underneath it, it can get a hit on the opposite side. 
				isVertexWithinArea_TraceEndLocation = (ComponentToCheckIfWithin->GetUpVector() * 100000.0f) + VertexWorldPosition;
				isVertexWithinArea_ShouldTrace = true;
			}

			// If not a landscape we only run a trace if the vertex is within the bounds. Unlike Cube/Rectangle that has to be Exactly where the mesh is even when rotated, we can use this here even when rotated. Should save some performance so we don't run unnecessary traces. 
			else if (ComponentToCheckIfIsWithinInfo.ComponentBounds.GetBox().IsInsideOrOn(VertexWorldPosition)) {

				isVertexWithinArea_ShouldTrace = true;
			}

			// NOTE it was very expensive if for instance spline meshes that also demand complex shapes Always traced if you for instance ran a paint within area with spline on a expensive mesh. So any further changes has to make sure that logic works, otherwise a task can take a really long time to finish


			if (isVertexWithinArea_ShouldTrace) {

				// DrawDebugLine(CalculateColorsInfoGlobal.vertexPaintDetectionHitActor->GetWorld(), VertexWorldPosition, isVertexWithinArea_TraceEndLocation, FColor::Red, false, 1, 0, .5);

				if (CalculateColorsInfoGlobal.TaskFundamentalSettings.TaskWorld.IsValid()) {

					TArray<FHitResult> isVertexWithinArea_HitResults;

					// If the vertex is inside the component. Multi trace in case there's something else inside where we're doing the trace
					CalculateColorsInfoGlobal.TaskFundamentalSettings.TaskWorld.Get()->LineTraceMultiByObjectType(isVertexWithinArea_HitResults, VertexWorldPosition, isVertexWithinArea_TraceEndLocation, ComponentToCheckIfWithin->GetCollisionObjectType(), ComponentToCheckIfIsWithinInfo.Complex_TraceParams);

					for (int i = 0; i < isVertexWithinArea_HitResults.Num(); i++) {

						if (ComponentToCheckIfWithin == isVertexWithinArea_HitResults[i].Component.Get()) {

							return true;
						}
					}
				}
			}
		}

		break;


	case EPaintWithinAreaShape::IsCapsuleShape:
		
		{

			const FVector AB = ComponentToCheckIfIsWithinInfo.Capsule_TopPoint_Local - ComponentToCheckIfIsWithinInfo.Capsule_BottomPoint_Local;
			const FVector AP = vertexPositionInLocalSpace - ComponentToCheckIfIsWithinInfo.Capsule_BottomPoint_Local;
			const float AB_Dot_AB = FVector::DotProduct(AB, AB);
			float t = FVector::DotProduct(AP, AB) / AB_Dot_AB;
			t = FMath::Clamp(t, 0.0f, 1.0f);

			const FVector closestPoint = ComponentToCheckIfIsWithinInfo.Capsule_BottomPoint_Local + t * AB;
			const float distanceSquared = FVector::DistSquared(vertexPositionInLocalSpace, closestPoint);

			if (distanceSquared <= FMath::Square(ComponentToCheckIfIsWithinInfo.Capsule_Radius))
				return true;
		}

		break;

	case EPaintWithinAreaShape::IsConeShape:
		
		{

			const FVector directionToVertex = VertexWorldPosition - ComponentToCheckIfIsWithinInfo.Cone_Origin;
			const float distanceToVertex = directionToVertex.Size();
			const float dotToVertex = FVector::DotProduct(ComponentToCheckIfIsWithinInfo.Cone_Direction, directionToVertex.GetSafeNormal());
			const float angleToVertex = FMath::Acos(dotToVertex);
			const float angleToVertexDegrees = FMath::RadiansToDegrees(angleToVertex);


			// If outside cone angle
			if (angleToVertexDegrees > ComponentToCheckIfIsWithinInfo.Cone_AngleInDegrees) return false;


			const float heightToCheck = UKismetMathLibrary::MapRangeClamped(angleToVertexDegrees, 0, ComponentToCheckIfIsWithinInfo.Cone_AngleInDegrees, ComponentToCheckIfIsWithinInfo.Cone_Height, ComponentToCheckIfIsWithinInfo.Cone_SlantHeight);

			// If outside height depending on the angle, i.e. if toward the edge of cone we check against that slanted height, if center we check against regular height
			if (distanceToVertex < 0 || distanceToVertex > heightToCheck) return false;

			return true;

		}

		break;

	default:
		break;
	}

	return false;
}


//-------------------------------------------------------

// Is Mesh And Actor Still Valid

bool FVertexPaintCalculateColorsTask::IsMeshStillValid(const FRVPDPCalculateColorsInfo& CalculateColorsInfo) const {

	UPrimitiveComponent* paintComponent = CalculateColorsInfo.GetVertexPaintComponent();
	if (!IsValid(paintComponent)) return false;
	if (!IsValid(CalculateColorsInfo.GetVertexPaintActor())) return false;

	
	// If the mesh has been changed mid task to something else then abort the calculation immediately
	if (CalculateColorsInfo.IsStaticMeshTask) {

		if (CalculateColorsInfo.VertexPaintSourceMesh != UVertexPaintFunctionLibrary::GetMeshComponentSourceMesh(paintComponent)) {

			return false;
		}
	}

	else if (CalculateColorsInfo.IsSkeletalMeshTask) {

		if (CalculateColorsInfo.VertexPaintSourceMesh != UVertexPaintFunctionLibrary::GetMeshComponentSourceMesh(paintComponent)) {

			return false;
		}
	}

#if ENGINE_MAJOR_VERSION == 5

	else if (CalculateColorsInfo.IsDynamicMeshTask) {
		
		// Already know that the comp is valid from the check at the top
		if (!IsValid(CalculateColorsInfo.GetVertexPaintDynamicMeshComponent()->GetDynamicMesh()))
			return false;
	}

	if (CalculateColorsInfo.IsGeometryCollectionTask) {
		
		if (CalculateColorsInfo.GetVertexPaintGeometryCollection() != CalculateColorsInfo.GetVertexPaintGeometryCollectionComponent()->GetRestCollection()) {

			return false;
		}
	}

#endif

	return true;
}


//-------------------------------------------------------

// Is Valid Skeletal Mesh

bool FVertexPaintCalculateColorsTask::IsValidSkeletalMesh(const USkeletalMesh * SkeletalMesh, const USkeletalMeshComponent * SkeletalMeshComponent, const FSkeletalMeshRenderData * SkeletalMeshRenderData, int Lod) {


	if (!IsValid(SkeletalMesh)) {

		CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Calculate Colors Task - Failed for Skeletal Mesh Component as it's Skeletal Mesh isn't valid")), FColor::Red));

		return false;
	}

	if (!IsValid(SkeletalMeshComponent)) {

		CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Calculate Colors Task - Failed for Skeletal Mesh Component as it's Skeletal Mesh Component isn't valid")), FColor::Red));

		return false;
	}

	if (!SkeletalMeshRenderData) {

		CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Calculate Colors Task - Failed for Skeletal Mesh Component as it's SkeletalMeshRenderData isn't valid. ")), FColor::Red));

		return false;
	}

	if (!SkeletalMeshRenderData->IsInitialized()) {

		CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Calculate Colors Task - Failed for Skeletal Mesh Component as it's SkeletalMeshRenderData isn't initialized. ")), FColor::Red));

		return false;
	}

	if (!SkeletalMeshRenderData->LODRenderData.IsValidIndex(Lod)) {

		CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Calculate Colors Task - Failed for Skeletal Mesh Component as it's SkeletalMeshRenderData's LodRenderData isn't valid for the current LOD we're looping through. ")), FColor::Red));

		return false;
	}

	return true;
}


//-------------------------------------------------------

// Is Vertex Close To Estimated Color To Hit Location

void FVertexPaintCalculateColorsTask::GetVerticesCloseToEstimatedColorToHitLocation(float& CurrentLongestDistance, int& CurrentLongestDistanceIndex, TMap<int, FRVPDPDirectionFromHitLocationToClosestVerticesInfo>& DirectionFromHitToClosestVertices, const FVector & HitLocationInWorldSpace, const FVector & VertexInWorldSpace, float HitNormalToVertexNormalDot, int VertexLODIndex, int AmountOfLODVertices) const {

	if (VertexLODIndex < 0) return;
	if (!CalculateColorsInfoGlobal.EstimatedColorAtHitLocationSettings.GetEstimatedColorAtHitLocation) return;
	if (CalculateColorsInfoGlobal.EstimatedColorAtHitLocationSettings.OnlyGetIfMeshHasMaxAmountOfVertices && AmountOfLODVertices > CalculateColorsInfoGlobal.EstimatedColorAtHitLocationSettings.MaxAmountOfVertices) return;
	if (HitNormalToVertexNormalDot < CalculateColorsInfoGlobal.EstimatedColorAtHitLocationSettings.MinHitNormalToVertexNormalDotRequired) return;


	int directionFromHitToClosestVerticesInfo_IndexAdded = -1;

	// If less than 9 then always adds then and doesn't require a distance check
	if (DirectionFromHitToClosestVertices.Num() < 9) {

		directionFromHitToClosestVerticesInfo_IndexAdded = DirectionFromHitToClosestVertices.Num();
	}

	// If we've already filled up 9 closest verts then we require that the current vertex has to be at least shorter than the longest in the TMap for it to replace it. 
	else if ((HitLocationInWorldSpace - VertexInWorldSpace).Size() < CurrentLongestDistance) {

		// Replaces the index where the longest distance used to be, so it's not a part of the TMap
		directionFromHitToClosestVerticesInfo_IndexAdded = CurrentLongestDistanceIndex;
	}


	// If we should replace any index because we either had < 9 in the TMap, or the current vertex had shorter distance than the longest in the TMap
	if (directionFromHitToClosestVerticesInfo_IndexAdded >= 0) {


		FRVPDPDirectionFromHitLocationToClosestVerticesInfo directionFromHitToClosestVerticesInfo_NewEntry;
		directionFromHitToClosestVerticesInfo_NewEntry.LodVertexIndex = VertexLODIndex;
		directionFromHitToClosestVerticesInfo_NewEntry.VertexWorldPosition = VertexInWorldSpace;
		directionFromHitToClosestVerticesInfo_NewEntry.DirectionFromHitDirectionToVertex = UKismetMathLibrary::GetDirectionUnitVector(HitLocationInWorldSpace, VertexInWorldSpace);
		directionFromHitToClosestVerticesInfo_NewEntry.DistanceFromHitLocation = (HitLocationInWorldSpace - VertexInWorldSpace).Size();


		DirectionFromHitToClosestVertices.Add(directionFromHitToClosestVerticesInfo_IndexAdded, directionFromHitToClosestVerticesInfo_NewEntry);

		// Loops through the current in the TMap when a new one gets added so we can update which is of them has the longest distance, so we can compare against that the next loop
		TArray<FRVPDPDirectionFromHitLocationToClosestVerticesInfo> directionFromHitToClosestVerticesInfo_Values;
		DirectionFromHitToClosestVertices.GenerateValueArray(directionFromHitToClosestVerticesInfo_Values);

		int indexWithLongestDistance = 0;
		float longestDistanceToVertex = directionFromHitToClosestVerticesInfo_Values[0].DistanceFromHitLocation;

		for (int i = 0; i < directionFromHitToClosestVerticesInfo_Values.Num(); i++) {


			if (directionFromHitToClosestVerticesInfo_Values[i].DistanceFromHitLocation > longestDistanceToVertex) {

				indexWithLongestDistance = i;
				longestDistanceToVertex = directionFromHitToClosestVerticesInfo_Values[i].DistanceFromHitLocation;
			}
		}

		// Updates the cached values so these are the new longestDistance and index, so we can check against the distance the next loop
		CurrentLongestDistanceIndex = indexWithLongestDistance;
		CurrentLongestDistance = longestDistanceToVertex;
	}
}


//-------------------------------------------------------

// Get Estimated Color To Hit Location Vertex To Lerp Color Toward

int FVertexPaintCalculateColorsTask::GetEstimatedColorToHitLocationVertexToLerpColorToward(const TMap<int, FRVPDPDirectionFromHitLocationToClosestVerticesInfo>&DirectionFromHitToClosestVertices, TArray<FVector>&ClosestVerticesToTheHitLocationInWorldLocation, const FVector & DirectionToClosestVertex, int ClosestVertexBased, float DotGraceRange) {

	if (ClosestVertexBased <= 0) return -1;
	if (DirectionFromHitToClosestVertices.Num() == 0) return -1;

	int closestVertexToLerpTowardIndex_ = -1;

	TArray< FRVPDPDirectionFromHitLocationToClosestVerticesInfo> directionFromHitLocInfo;
	DirectionFromHitToClosestVertices.GenerateValueArray(directionFromHitLocInfo);


	for (int i = 0; i < directionFromHitLocInfo.Num(); i++) {

		// The Direction from the hit Location to the closest vertex * -1, i.e the Direction we want to get the vertex to scale to
		FVector directionFromClosestVertexToHitLocation_ = DirectionToClosestVertex * -1;
		float closestVertexToLerpTowardDistance_ = 1000000;
		float closestVertexToLerpTowardDot_ = -1;

		ClosestVerticesToTheHitLocationInWorldLocation.Add(directionFromHitLocInfo[i].VertexWorldPosition);

		float currentDot = FVector::DotProduct(directionFromClosestVertexToHitLocation_, directionFromHitLocInfo[i].DirectionFromHitDirectionToVertex);
		float currentDistance = directionFromHitLocInfo[i].DistanceFromHitLocation;


		if (currentDot > closestVertexToLerpTowardDot_) {

			// If has better dot than the current best dot, but the current best with added grace range is equal or better, and has shorter range, then we keep it since short distance is high prio
			if ((closestVertexToLerpTowardDot_ + DotGraceRange) >= currentDot && closestVertexToLerpTowardDistance_ < currentDistance) {

				//
			}
			else {

				closestVertexToLerpTowardDot_ = currentDot;
				closestVertexToLerpTowardDistance_ = currentDistance;
				closestVertexToLerpTowardIndex_ = directionFromHitLocInfo[i].LodVertexIndex;
			}
		}

		// If doesn't have better dot, but with added grace period is better or equal, and has shorter range. So if something is closer, but has slightly worse dot, then it may still be choosen since distance has high prio
		else if ((currentDot + DotGraceRange) >= closestVertexToLerpTowardDot_ && currentDistance < closestVertexToLerpTowardDistance_) {

			closestVertexToLerpTowardDot_ = currentDot;
			closestVertexToLerpTowardDistance_ = currentDistance;
			closestVertexToLerpTowardIndex_ = directionFromHitLocInfo[i].LodVertexIndex;
		}
	}

	return closestVertexToLerpTowardIndex_;
}


//-------------------------------------------------------

// Get Estimated Color To Hit Location Values

void FVertexPaintCalculateColorsTask::GetEstimatedColorToHitLocationValues(int VertexToLerpToward, const FColor & ClosestVertexColor, const FVector & ClosestVertexInWorldSpace, const FVector & HitLocationInWorldSpace, const FColor & VertexToLerpTowardColor, const FVector & VertexToLerpTowardPositionInWorldSpace, FLinearColor & EstimatedColorAtHitLocation, FVector & EstimatedHitLocation) {


	// If got vertex to lerp toward, then sets colorToHitLocation_AfterApplyingColors_Paint 
	if (VertexToLerpToward >= 0) {

		// Distance from Closest Paint to Hit Location
		float distanceFromClosestVertexToHitLocation = (HitLocationInWorldSpace - ClosestVertexInWorldSpace).Size();

		// Distance to from closest vertex paint to vertex to lerp toward
		float distanceFromClosestVertexToVertexWeLerpToward = (ClosestVertexInWorldSpace - VertexToLerpTowardPositionInWorldSpace).Size();


		// Depending on the distance from the closest vertex position to the vertex we lerp toward position, we convert to Linear and scale the color so it should pretty accurate to what the actual Hit Location Color Should be. 

		EstimatedColorAtHitLocation = UKismetMathLibrary::LinearColorLerp(UVertexPaintFunctionLibrary::ReliableFColorToFLinearColor(ClosestVertexColor), UVertexPaintFunctionLibrary::ReliableFColorToFLinearColor(VertexToLerpTowardColor), UKismetMathLibrary::MapRangeClamped(distanceFromClosestVertexToHitLocation, 0, distanceFromClosestVertexToVertexWeLerpToward, 0, 1));
		// EstimatedColorAtHitLocation = UKismetMathLibrary::LinearColorLerp(ClosestVertexColor.ReinterpretAsLinear(), VertexToLerpTowardColor.ReinterpretAsLinear(), UKismetMathLibrary::MapRangeClamped(distanceFromClosestVertexToHitLocation, 0, distanceFromClosestVertexToVertexWeLerpToward, 0, 1));

		EstimatedHitLocation = UKismetMathLibrary::VLerp(ClosestVertexInWorldSpace, VertexToLerpTowardPositionInWorldSpace, UKismetMathLibrary::MapRangeClamped(distanceFromClosestVertexToHitLocation, 0, distanceFromClosestVertexToVertexWeLerpToward, 0, 1));
	}
}


//-------------------------------------------------------

// Get Spline Mesh Vertex Position In Mesh Space

FVector FVertexPaintCalculateColorsTask::GetSplineMeshVertexPositionInMeshSpace(const FVector & VertexPos, const USplineMeshComponent * SplineMeshComponent) {

	if (!IsValid(SplineMeshComponent)) return FVector(0, 0, 0);

	FVector slicePos = VertexPos;
	slicePos[SplineMeshComponent->ForwardAxis] = 0;
	const FVector SplineMeshSpaceVector = SplineMeshComponent->CalcSliceTransform(VertexPos[SplineMeshComponent->ForwardAxis]).TransformPosition(slicePos);

	return SplineMeshSpaceVector;
}


//-------------------------------------------------------

// Get Spline Mesh Vertex Normal In Mesh Space

FVector FVertexPaintCalculateColorsTask::GetSplineMeshVertexNormalInMeshSpace(const FVector & Normal, const USplineMeshComponent * SplineMeshComponent) {

	if (!IsValid(SplineMeshComponent)) return FVector(0, 0, 0);

	FVector sliceNormal = Normal;
	sliceNormal[SplineMeshComponent->ForwardAxis] = 0;
	const FVector SplineMeshSpaceVector = SplineMeshComponent->CalcSliceTransform(sliceNormal[SplineMeshComponent->ForwardAxis]).TransformVector(sliceNormal);

	return SplineMeshSpaceVector;
}


//-------------------------------------------------------

// Is Current Vertex Color Within Conditions

bool FVertexPaintCalculateColorsTask::IsCurrentVertexWithinPaintColorConditions(UMaterialInterface* VertexMaterial, const FColor& CurrentVertexColor, const FVector& CurrentVertexPosition, const FVector& CurrentVertexNormal, const FName& CurrentBoneName, float BoneWeight, const FRVPDPPaintConditionSettings& PaintConditions, float& ColorStrengthIfConditionsIsNotMet, EApplyVertexColorSetting& ColorSettingIfConditionIsNotMet) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_IsCurrentVertexWithinPaintColorConditions);
	RVPDP_CPUPROFILER_STR("Calculate Colors Task - IsCurrentVertexWithinPaintColorConditions");


	ColorStrengthIfConditionsIsNotMet = 0;


	if (PaintConditions.IsVertexNormalWithinDotToDirection.Num() > 0) {

		for (int i = 0; i < PaintConditions.IsVertexNormalWithinDotToDirection.Num(); i++) {


			const float dotProduct = FVector::DotProduct(CurrentVertexNormal, PaintConditions.IsVertexNormalWithinDotToDirection[i].DirectionToCheckAgainst);

			if (dotProduct >= PaintConditions.IsVertexNormalWithinDotToDirection[i].MinDotProductToDirectionRequired) {

				// return true;
			}

			else {

				ColorStrengthIfConditionsIsNotMet = PaintConditions.IsVertexNormalWithinDotToDirection[i].ColorStrengthIfThisConditionIsNotMet;
				ColorSettingIfConditionIsNotMet = PaintConditions.IsVertexNormalWithinDotToDirection[i].SettingIfConditionIsNotMet;

				return false;
			}
		}
	}


	if (PaintConditions.OnlyAffectVerticesWithDirectionToActorOrLocation.Num() > 0) {

		for (int i = 0; i < PaintConditions.OnlyAffectVerticesWithDirectionToActorOrLocation.Num(); i++) {

			const FVector directionFromLocation = (PaintConditions.OnlyAffectVerticesWithDirectionToActorOrLocation[i].Location - CurrentVertexPosition).GetSafeNormal();
			const float dotFromDirectionToLocation = FVector::DotProduct(CurrentVertexNormal, directionFromLocation);

			if (dotFromDirectionToLocation >= PaintConditions.OnlyAffectVerticesWithDirectionToActorOrLocation[i].MinDotProductToActorOrLocation) {

				// return true;
			}

			else {

				ColorStrengthIfConditionsIsNotMet = PaintConditions.OnlyAffectVerticesWithDirectionToActorOrLocation[i].ColorStrengthIfThisConditionIsNotMet;
				ColorSettingIfConditionIsNotMet = PaintConditions.OnlyAffectVerticesWithDirectionToActorOrLocation[i].SettingIfConditionIsNotMet;

				return false;
			}
		}
	}


	if (PaintConditions.OnlyAffectVerticesWithinDirectionFromActorOrLocation.Num() > 0) {

		for (int i = 0; i < PaintConditions.OnlyAffectVerticesWithinDirectionFromActorOrLocation.Num(); i++) {


			const FVector directionFromLocation = (CurrentVertexPosition - PaintConditions.OnlyAffectVerticesWithinDirectionFromActorOrLocation[i].Location).GetSafeNormal();
			const float dotFromDirectionToLocation = FVector::DotProduct(PaintConditions.OnlyAffectVerticesWithinDirectionFromActorOrLocation[i].Direction, directionFromLocation);

			if (dotFromDirectionToLocation >= PaintConditions.OnlyAffectVerticesWithinDirectionFromActorOrLocation[i].MinDotProductToDirection) {

				// If meet condition, then continues by checking the rest of the conditions
			}

			else {

				ColorStrengthIfConditionsIsNotMet = PaintConditions.OnlyAffectVerticesWithinDirectionFromActorOrLocation[i].ColorStrengthIfThisConditionIsNotMet;
				ColorSettingIfConditionIsNotMet = PaintConditions.OnlyAffectVerticesWithinDirectionFromActorOrLocation[i].SettingIfConditionIsNotMet;

				return false;
			}
		}
	}


	if (PaintConditions.IfVertexIsAboveOrBelowWorldZ.Num() > 0) {

		for (int i = 0; i < PaintConditions.IfVertexIsAboveOrBelowWorldZ.Num(); i++) {

			// If set to check if the position Z is above or equal to the the Z value then returns false if it's below it
			if (PaintConditions.IfVertexIsAboveOrBelowWorldZ[i].CheckIfAboveInsteadOfBelowZ) {

				if (CurrentVertexPosition.Z < PaintConditions.IfVertexIsAboveOrBelowWorldZ[i].IfVertexIsAboveOrBelowWorldZ) {

					ColorStrengthIfConditionsIsNotMet = PaintConditions.IfVertexIsAboveOrBelowWorldZ[i].ColorStrengthIfThisConditionIsNotMet;
					ColorSettingIfConditionIsNotMet = PaintConditions.IfVertexIsAboveOrBelowWorldZ[i].SettingIfConditionIsNotMet;

					return false;
				}
			}

			// If requires that the vertex position should be equal or below then fails if above it. 
			else {

				if (CurrentVertexPosition.Z > PaintConditions.IfVertexIsAboveOrBelowWorldZ[i].IfVertexIsAboveOrBelowWorldZ) {


					ColorStrengthIfConditionsIsNotMet = PaintConditions.IfVertexIsAboveOrBelowWorldZ[i].ColorStrengthIfThisConditionIsNotMet;
					ColorSettingIfConditionIsNotMet = PaintConditions.IfVertexIsAboveOrBelowWorldZ[i].SettingIfConditionIsNotMet;

					return false;
				}
			}
		}
	}


	if (PaintConditions.IfVertexColorIsWithinRange.Num() > 0) {

		for (int i = 0; i < PaintConditions.IfVertexColorIsWithinRange.Num(); i++) {

			switch (PaintConditions.IfVertexColorIsWithinRange[i].IfVertexColorChannelWithinColorRange) {

			case ESurfaceAtChannel::RedChannel:

				if (CurrentVertexColor.R >= PaintConditions.IfVertexColorIsWithinRange[i].IfHigherOrEqualThanInt && CurrentVertexColor.R <= PaintConditions.IfVertexColorIsWithinRange[i].IfLessOrEqualThanInt) {

					//
				}
				else {

					ColorStrengthIfConditionsIsNotMet = PaintConditions.IfVertexColorIsWithinRange[i].ColorStrengthIfThisConditionIsNotMet;
					ColorSettingIfConditionIsNotMet = PaintConditions.IfVertexColorIsWithinRange[i].SettingIfConditionIsNotMet;

					return false;
				}

				break;

			case ESurfaceAtChannel::GreenChannel:


				if (CurrentVertexColor.G >= PaintConditions.IfVertexColorIsWithinRange[i].IfHigherOrEqualThanInt && CurrentVertexColor.G <= PaintConditions.IfVertexColorIsWithinRange[i].IfLessOrEqualThanInt) {

					//
				}
				else {

					ColorStrengthIfConditionsIsNotMet = PaintConditions.IfVertexColorIsWithinRange[i].ColorStrengthIfThisConditionIsNotMet;
					ColorSettingIfConditionIsNotMet = PaintConditions.IfVertexColorIsWithinRange[i].SettingIfConditionIsNotMet;

					return false;
				}

				break;

			case ESurfaceAtChannel::BlueChannel:

				if (CurrentVertexColor.B >= PaintConditions.IfVertexColorIsWithinRange[i].IfHigherOrEqualThanInt && CurrentVertexColor.B <= PaintConditions.IfVertexColorIsWithinRange[i].IfLessOrEqualThanInt) {

					//
				}
				else {

					ColorStrengthIfConditionsIsNotMet = PaintConditions.IfVertexColorIsWithinRange[i].ColorStrengthIfThisConditionIsNotMet;
					ColorSettingIfConditionIsNotMet = PaintConditions.IfVertexColorIsWithinRange[i].SettingIfConditionIsNotMet;

					return false;
				}

				break;

			case ESurfaceAtChannel::AlphaChannel:

				if (CurrentVertexColor.A >= PaintConditions.IfVertexColorIsWithinRange[i].IfHigherOrEqualThanInt && CurrentVertexColor.A <= PaintConditions.IfVertexColorIsWithinRange[i].IfLessOrEqualThanInt) {

					// 
				}
				else {

					ColorStrengthIfConditionsIsNotMet = PaintConditions.IfVertexColorIsWithinRange[i].ColorStrengthIfThisConditionIsNotMet;
					ColorSettingIfConditionIsNotMet = PaintConditions.IfVertexColorIsWithinRange[i].SettingIfConditionIsNotMet;

					return false;
				}

				break;

			default:
				break;
			}
		}
	}


	if (PaintConditions.IfVertexIsOnBone.Num() > 0) {

		if (CurrentBoneName.GetStringLength() > 0) {

			bool atAnyBone = false;
			float colorToApplyIfFailed = 0;
			EApplyVertexColorSetting settingToApplyIfFailed = EApplyVertexColorSetting::EAddVertexColor;

			for (int i = 0; i < PaintConditions.IfVertexIsOnBone.Num(); i++) {

				colorToApplyIfFailed = PaintConditions.IfVertexIsOnBone[i].ColorStrengthIfThisConditionIsNotMet;
				settingToApplyIfFailed = PaintConditions.IfVertexIsOnBone[i].SettingIfConditionIsNotMet;

				if (BoneWeight >= PaintConditions.IfVertexIsOnBone[i].MinBoneWeight && PaintConditions.IfVertexIsOnBone[i].IfVertexIsAtBone == CurrentBoneName) {

					atAnyBone = true;
					break;
				}
			}

			// If still false if we had bone conditions it means that we're not on the right bone
			if (!atAnyBone) {

				ColorStrengthIfConditionsIsNotMet = colorToApplyIfFailed;
				ColorSettingIfConditionIsNotMet = settingToApplyIfFailed;

				return false;
			}
		}
	}


	if (PaintConditions.IfVertexIsOnMaterial.Num() > 0) {

		for (int i = 0; i < PaintConditions.IfVertexIsOnMaterial.Num(); i++) {


			if (PaintConditions.IfVertexIsOnMaterial[i].IfVertexIsOnMaterial && VertexMaterial) {


				if (PaintConditions.IfVertexIsOnMaterial[i].IfVertexIsOnMaterial == VertexMaterial) continue;


				// otherwise checks they're an instance, or dynamic instance and if those parents matches
				if (PaintConditions.IfVertexIsOnMaterial[i].CheckMaterialParents) {


					UMaterialInterface* vertexMaterialParent = VertexMaterial;

					if (UMaterialInstanceDynamic* materialInstanceDynamic = Cast<UMaterialInstanceDynamic>(VertexMaterial)) {

						if (UMaterialInstance* materialInstance = Cast<UMaterialInstance>(materialInstanceDynamic->Parent))
							vertexMaterialParent = materialInstance->Parent;
						else
							vertexMaterialParent = materialInstanceDynamic->Parent;
					}

					else if (UMaterialInstance* materialInstance = Cast<UMaterialInstance>(VertexMaterial)) {

						vertexMaterialParent = materialInstance->Parent;
					}


					if (PaintConditions.IfVertexIsOnMaterial[i].IfVertexIsOnMaterialParent == vertexMaterialParent) {

						//
					}

					else {

						ColorStrengthIfConditionsIsNotMet = PaintConditions.IfVertexIsOnMaterial[i].ColorStrengthIfThisConditionIsNotMet;
						ColorSettingIfConditionIsNotMet = PaintConditions.IfVertexIsOnMaterial[i].SettingIfConditionIsNotMet;
						return false;
					}
				}

				else {

					ColorStrengthIfConditionsIsNotMet = PaintConditions.IfVertexIsOnMaterial[i].ColorStrengthIfThisConditionIsNotMet;
					ColorSettingIfConditionIsNotMet = PaintConditions.IfVertexIsOnMaterial[i].SettingIfConditionIsNotMet;
					return false;
				}
			}

			else {

				ColorStrengthIfConditionsIsNotMet = PaintConditions.IfVertexIsOnMaterial[i].ColorStrengthIfThisConditionIsNotMet;
				ColorSettingIfConditionIsNotMet = PaintConditions.IfVertexIsOnMaterial[i].SettingIfConditionIsNotMet;
				return false;
			}
		}
	}


	// Does the Line of Sight last as it takes the longest to finish and don't want to run it if any other condition should make it fail
	if (PaintConditions.IfVertexHasLineOfSightTo.Num() > 0) {

		for (int i = 0; i < PaintConditions.IfVertexHasLineOfSightTo.Num(); i++) {

			FHitResult hitResult = FHitResult();
			if (CalculateColorsInfoGlobal.TaskFundamentalSettings.TaskWorld.Get()->LineTraceSingleByObjectType(hitResult, CurrentVertexPosition + (CurrentVertexNormal * PaintConditions.IfVertexHasLineOfSightTo[i].DistanceFromVertexPositionToStartTrace), PaintConditions.IfVertexHasLineOfSightTo[i].IfVertexHasLineOfSightToPosition, PaintConditions.IfVertexHasLineOfSightTo[i].LineOfSightTraceObjectQueryParams, PaintConditions.IfVertexHasLineOfSightTo[i].LineOfSightTraceParams)) {

				ColorStrengthIfConditionsIsNotMet = PaintConditions.IfVertexHasLineOfSightTo[i].ColorStrengthIfThisConditionIsNotMet;
				ColorSettingIfConditionIsNotMet = PaintConditions.IfVertexHasLineOfSightTo[i].SettingIfConditionIsNotMet;

				// If something is blocking, i.e. the vertex doesn't have line of sight
				return false;
			}
		}
	}


	// If haven't returned false by now, i.e. haven't failed a condition, then we return true.
	return true;
}


//-------------------------------------------------------

// Will Applied Paint Make Change

bool FVertexPaintCalculateColorsTask::WillAppliedColorMakeChangeOnVertex(const FColor& CurrentColorOnVertex, const FRVPDPApplyColorSetting& PaintColorSettings, const TMap<int, TArray<FRVPDPPhysicsSurfaceToApplySettings>>& PhysicsSurfacePaintSettingsPerChannel) {

	
	if (PaintColorSettings.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.ApplyVertexColorUsingPhysicsSurface) {


		uint8 vertexColorToCheckAgainst = 0;

		for (const TPair<int, TArray<FRVPDPPhysicsSurfaceToApplySettings>>& physicsSurfaceToApplyPerChannelSettings : PhysicsSurfacePaintSettingsPerChannel) {

			for (const FRVPDPPhysicsSurfaceToApplySettings& physicalSurfaceToApply : physicsSurfaceToApplyPerChannelSettings.Value) {


				if (physicalSurfaceToApply.LerpPhysicsSurfaceToTarget.LerpToTarget && physicalSurfaceToApply.LerpPhysicsSurfaceToTarget.LerpStrength > 0)
					return true;


				switch (physicsSurfaceToApplyPerChannelSettings.Key) {

				case 0:
					vertexColorToCheckAgainst = CurrentColorOnVertex.R;
					break;

				case 1:
					vertexColorToCheckAgainst = CurrentColorOnVertex.G;
					break;

				case 2:
					vertexColorToCheckAgainst = CurrentColorOnVertex.B;
					break;

				case 3:
					vertexColorToCheckAgainst = CurrentColorOnVertex.A;
					break;

				default:
					break;
				}


				if (physicalSurfaceToApply.StrengthToApply > 0 || physicalSurfaceToApply.StrengthToApply < 0) {

					// If in between min and max and we're trying to Add/Remove
					if (vertexColorToCheckAgainst > 0 || vertexColorToCheckAgainst < 255) {

						return true;
					}

					// If 0 and trying to Add, or 255 and trying to Remove
					if ((vertexColorToCheckAgainst <= 0 && physicalSurfaceToApply.StrengthToApply > 0) || (vertexColorToCheckAgainst >= 255 && physicalSurfaceToApply.StrengthToApply < 0)) {

						return true;
					}
				}
			}
		}
	}

	else if (PaintColorSettings.ApplyVertexColorSettings.IsAnyColorChannelSetToSetColor()) {

		return true;
	}

	else if (PaintColorSettings.ApplyVertexColorSettings.IsAnyColorChannelSetToSetToLerpToTarget()) {

		return true;
	}

	else if (PaintColorSettings.ApplyVertexColorSettings.IsAnyColorChannelSetToAddColorAndIsNotZero()) {

		// If color is maxed then the strength has to be -, i.e. the player has to try to remove it. If the color is at 0, then the player has to add to it. 

		// Red
		if (PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnRedChannel.AmountToApply > 0 || PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnRedChannel.AmountToApply < 0) {


			// If in between min and max and we're trying to Add/Remove
			if (CurrentColorOnVertex.R > 0 || CurrentColorOnVertex.R < 255) {

				return true;
			}

			// If 0 and trying to Add, or 255 and trying to Remove
			else if ((CurrentColorOnVertex.R <= 0 && PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnRedChannel.AmountToApply > 0) || (CurrentColorOnVertex.R >= 255 && PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnRedChannel.AmountToApply < 0)) {

				return true;
			}
		}

		// Green
		if (PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.AmountToApply > 0 || PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.AmountToApply < 0) {


			// If in between min and max and we're trying to Add/Remove
			if (CurrentColorOnVertex.G > 0 || CurrentColorOnVertex.G < 255) {

				return true;
			}

			// If 0 and trying to Add, or 255 and trying to Remove
			else if ((CurrentColorOnVertex.G <= 0 && PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.AmountToApply > 0) || (CurrentColorOnVertex.G >= 255 && PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.AmountToApply < 0)) {

				return true;
			}
		}

		// Blue
		if (PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.AmountToApply > 0 || PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.AmountToApply < 0) {


			// If in between min and max and we're trying to Add/Remove
			if (CurrentColorOnVertex.B > 0 || CurrentColorOnVertex.B < 255) {

				return true;
			}

			// If 0 and trying to Add, or 255 and trying to Remove
			else if ((CurrentColorOnVertex.B <= 0 && PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.AmountToApply > 0) || (CurrentColorOnVertex.B >= 255 && PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.AmountToApply < 0)) {

				return true;
			}
		}

		// Alpha
		if (PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.AmountToApply > 0 || PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.AmountToApply < 0) {


			// If in between min and max and we're trying to Add/Remove
			if (CurrentColorOnVertex.A > 0 || CurrentColorOnVertex.A < 255) {

				return true;
			}

			// If 0 and trying to Add, or 255 and trying to Remove
			else if ((CurrentColorOnVertex.A <= 0 && PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.AmountToApply > 0) || (CurrentColorOnVertex.A >= 255 && PaintColorSettings.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.AmountToApply < 0)) {

				return true;
			}
		}

	}


	return false;
}


//-------------------------------------------------------

// Get Vertex Color After Any Limitation

void FVertexPaintCalculateColorsTask::GetVertexColorAfterAnyLimitation(const FColor& CurrentVertexColor, const FRVPDPPaintLimitSettings& PaintLimitRedChannel, const FRVPDPPaintLimitSettings& PaintLimitGreenChannel, const FRVPDPPaintLimitSettings& PaintLimitBlueChannel, const FRVPDPPaintLimitSettings& PaintLimitAlphaChannel, float NewRedVertexColorToApply, float NewGreenVertexColorToApply, float NewBlueVertexColorToApply, float NewAlphaVertexColorToApply, float& FinalRedVertexColorToApply, float& FinalGreenVertexColorToApply, float& FinalBlueVertexColorToApply, float& FinalAlphaVertexColorToApply) {


	// Rounds to nearest because otherwise we had an issue with limiting, where if for instance we had a limit of 0.5, i.e. 127.5 here, then it would get limited to 128 when rounding at Get Colors to Apply, meaning the Next paint job it would already be past the limit and if LimitColorIfTheColorWasAlreadyOverTheLimit was false it doesn't limit it, meaning it only worked correct if running 1 paint job. 
	const float redVertexColorLimit = UKismetMathLibrary::Round(UKismetMathLibrary::MapRangeClamped(PaintLimitRedChannel.PaintLimit, 0, 1, 0, 255));
	const float greenVertexColorLimit = UKismetMathLibrary::Round(UKismetMathLibrary::MapRangeClamped(PaintLimitGreenChannel.PaintLimit, 0, 1, 0, 255));
	const float blueVertexColorLimit = UKismetMathLibrary::Round(UKismetMathLibrary::MapRangeClamped(PaintLimitBlueChannel.PaintLimit, 0, 1, 0, 255));
	const float alphaVertexColorLimit = UKismetMathLibrary::Round(UKismetMathLibrary::MapRangeClamped(PaintLimitAlphaChannel.PaintLimit, 0, 1, 0, 255));


	const float currentRedVertexColor = static_cast<float>(CurrentVertexColor.R);
	const float currentGreenVertexColor = static_cast<float>(CurrentVertexColor.G);
	const float currentBlueVertexColor = static_cast<float>(CurrentVertexColor.B);
	const float currentAlphaVertexColor = static_cast<float>(CurrentVertexColor.A);

	
	// Red
	if (PaintLimitRedChannel.UsePaintLimits) {

		// If set to limit color if already over it, then if either the original or the new vertex color is over the limit, we limit it. 
		if (PaintLimitRedChannel.LimitColorIfTheColorWasAlreadyOverTheLimit && (currentRedVertexColor > redVertexColorLimit || NewRedVertexColorToApply > redVertexColorLimit))
			NewRedVertexColorToApply = redVertexColorLimit;
		else if (currentRedVertexColor <= redVertexColorLimit && NewRedVertexColorToApply >= redVertexColorLimit)
			NewRedVertexColorToApply = redVertexColorLimit; // If we originally was below, but with the added colors we're now at or above the limit

		// If already over the Limit, then even though we're not going to lower it to what the limit is, we don't want to Add even more over the limit. 
		else if (currentRedVertexColor > redVertexColorLimit) 
			NewRedVertexColorToApply = currentRedVertexColor;
	}


	// Green
	if (PaintLimitGreenChannel.UsePaintLimits) {

		if (PaintLimitGreenChannel.LimitColorIfTheColorWasAlreadyOverTheLimit && (currentGreenVertexColor > greenVertexColorLimit || NewGreenVertexColorToApply > greenVertexColorLimit))
			NewGreenVertexColorToApply = greenVertexColorLimit;
		else if (currentGreenVertexColor <= greenVertexColorLimit && NewGreenVertexColorToApply >= greenVertexColorLimit)
			NewGreenVertexColorToApply = greenVertexColorLimit;
		else if (currentGreenVertexColor > greenVertexColorLimit)
			NewGreenVertexColorToApply = currentGreenVertexColor;
	}


	// Blue
	if (PaintLimitBlueChannel.UsePaintLimits) {

		if (PaintLimitBlueChannel.LimitColorIfTheColorWasAlreadyOverTheLimit && (currentBlueVertexColor > blueVertexColorLimit || NewBlueVertexColorToApply > blueVertexColorLimit)) 
			NewBlueVertexColorToApply = blueVertexColorLimit;
		else if (currentBlueVertexColor <= blueVertexColorLimit && NewBlueVertexColorToApply >= blueVertexColorLimit)
			NewBlueVertexColorToApply = blueVertexColorLimit;
		else if (currentBlueVertexColor > blueVertexColorLimit)
			NewBlueVertexColorToApply = currentBlueVertexColor;
	}


	// Alpha
	if (PaintLimitAlphaChannel.UsePaintLimits) {

		if (PaintLimitAlphaChannel.LimitColorIfTheColorWasAlreadyOverTheLimit && (currentAlphaVertexColor > alphaVertexColorLimit || NewAlphaVertexColorToApply > alphaVertexColorLimit))
			NewAlphaVertexColorToApply = alphaVertexColorLimit;
		else if (currentAlphaVertexColor <= alphaVertexColorLimit && NewAlphaVertexColorToApply >= alphaVertexColorLimit)
			NewAlphaVertexColorToApply = alphaVertexColorLimit;
		else if (currentAlphaVertexColor > alphaVertexColorLimit)
			NewAlphaVertexColorToApply = currentAlphaVertexColor;
	}


	FinalRedVertexColorToApply = NewRedVertexColorToApply;
	FinalGreenVertexColorToApply = NewGreenVertexColorToApply;
	FinalBlueVertexColorToApply = NewBlueVertexColorToApply;
	FinalAlphaVertexColorToApply = NewAlphaVertexColorToApply;
}


//-------------------------------------------------------

// Get Color To Apply On Vertex - Calculates strength depending on distance and falloff etc. 

FColor FVertexPaintCalculateColorsTask::GetColorToApplyOnVertex(UVertexPaintDetectionComponent* AssociatedPaintComponent, UObject* ObjectToOverrideVertexColorsWith, int CurrentLOD, int CurrentVertexIndex, UMaterialInterface* MaterialVertexIsOn, const TArray<TEnumAsByte<EPhysicalSurface>>& RegisteredPhysicsSurfacesAtMaterialVertexIsOn, const TMap<int, TArray<FRVPDPPhysicsSurfaceToApplySettings>>& physicsSurfacePaintSettingsPerChannel, bool isVertexOnCloth, const FLinearColor& currentLinearVertexColor, const FColor& currentVertexColor, const FVector& currentVertexWorldPosition, const FVector& CurrentVertexNormal, const FName& CurrentBoneName, float BoneWeight, const FRVPDPVertexPaintFallOffSettings& FallOffSettings, float FallOffRange, float ScaleFallOffFrom, float DistanceFromFalloffBaseToVertexLocation, const FRVPDPApplyColorSetting& PaintOnMeshColorSetting, bool ApplyColorOnRedChannel, bool ApplyColorOnGreenChannel, bool ApplyColorOnBlueChannel, bool ApplyColorOnAlphaChannel, bool& VertexGotColorChanged, bool& HaveRunOverrideInterface, bool& ShouldApplyColorOnAnyChannel, bool hasAnyPaintCondition, bool hasPhysicsSurfacePaintCondition, bool hasPaintConditionOnRedChannel, bool hasPaintConditionOnGreenChannel, bool hasPaintConditionOnBlueChannel, bool hasPaintConditionOnAlphaChannel) {
	
	VertexGotColorChanged = false;
	

	// Resets these global properties here as they're only used here. Using globals as this runs for each vertex, which can be thousands of times
	float tryingToApplyColor_Red = 0;
	float tryingToApplyColor_Green = 0;
	float tryingToApplyColor_Blue = 0;
	float tryingToApplyColor_Alpha = 0;

	EApplyVertexColorSetting applyVertexColorSettingRedChannel = EApplyVertexColorSetting::EAddVertexColor;
	EApplyVertexColorSetting applyVertexColorSettingGreenChannel = EApplyVertexColorSetting::EAddVertexColor;
	EApplyVertexColorSetting applyVertexColorSettingBlueChannel = EApplyVertexColorSetting::EAddVertexColor;
	EApplyVertexColorSetting applyVertexColorSettingAlphaChannel = EApplyVertexColorSetting::EAddVertexColor;

	FRVPDPPaintLimitSettings paintLimitOnRedChannel = FRVPDPPaintLimitSettings();
	FRVPDPPaintLimitSettings paintLimitOnGreenChannel = FRVPDPPaintLimitSettings();
	FRVPDPPaintLimitSettings paintLimitOnBlueChannel = FRVPDPPaintLimitSettings();
	FRVPDPPaintLimitSettings paintLimitOnAlphaChannel = FRVPDPPaintLimitSettings();

	FRVPDPLerpVertexColorToTargetSettings lerpToTargetRedChannel = FRVPDPLerpVertexColorToTargetSettings();
	FRVPDPLerpVertexColorToTargetSettings lerpToTargetGreenChannel = FRVPDPLerpVertexColorToTargetSettings();
	FRVPDPLerpVertexColorToTargetSettings lerpToTargetBlueChannel = FRVPDPLerpVertexColorToTargetSettings();
	FRVPDPLerpVertexColorToTargetSettings lerpToTargetAlphaChannel = FRVPDPLerpVertexColorToTargetSettings();

	float paintCondition_ColorStrengthIfConditionIsNotMet = 0;
	EApplyVertexColorSetting paintCondition_SettingToApplyIfConditionIsNotMet = EApplyVertexColorSetting::EAddVertexColor;
	float lerpToTargetValue = 0;


	if (PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.ApplyVertexColorUsingPhysicsSurface) {

		bool gotPhysicsSurfaceOnRed = false;
		bool gotPhysicsSurfaceOnGreen = false;
		bool gotPhysicsSurfaceOnBlue = false;
		bool gotPhysicsSurfaceOnAlpha = false;
		
		
		// Key = Vertex Channel, Value = Physics Surface Array to Apply for that channel, since we can Add several of the same physics surface in the paint settings but with seperate conditions etc.
		for (const TPair<int, TArray<FRVPDPPhysicsSurfaceToApplySettings>>& physicsSurfacesToApply : physicsSurfacePaintSettingsPerChannel) {

			for (const FRVPDPPhysicsSurfaceToApplySettings& physicalSurfaceToApply : physicsSurfacesToApply.Value) {

				
				switch (physicsSurfacesToApply.Key) {

				case 0:

					gotPhysicsSurfaceOnRed = true;
					paintLimitOnRedChannel = physicalSurfaceToApply.SurfacePaintLimit;
					lerpToTargetRedChannel = physicalSurfaceToApply.LerpPhysicsSurfaceToTarget;
					applyVertexColorSettingRedChannel = physicalSurfaceToApply.SettingToApplyWith;
					break;

				case 1:

					gotPhysicsSurfaceOnGreen = true;
					paintLimitOnGreenChannel = physicalSurfaceToApply.SurfacePaintLimit;
					lerpToTargetGreenChannel = physicalSurfaceToApply.LerpPhysicsSurfaceToTarget;
					applyVertexColorSettingGreenChannel = physicalSurfaceToApply.SettingToApplyWith;
					break;

				case 2:

					gotPhysicsSurfaceOnBlue = true;
					paintLimitOnBlueChannel = physicalSurfaceToApply.SurfacePaintLimit;
					lerpToTargetBlueChannel = physicalSurfaceToApply.LerpPhysicsSurfaceToTarget;
					applyVertexColorSettingBlueChannel = physicalSurfaceToApply.SettingToApplyWith;
					break;

				case 3:

					gotPhysicsSurfaceOnAlpha = true;
					paintLimitOnAlphaChannel = physicalSurfaceToApply.SurfacePaintLimit;
					lerpToTargetAlphaChannel = physicalSurfaceToApply.LerpPhysicsSurfaceToTarget;
					applyVertexColorSettingAlphaChannel = physicalSurfaceToApply.SettingToApplyWith;
					break;

				default:
					break;
				}


				if (!hasPhysicsSurfacePaintCondition || (hasPhysicsSurfacePaintCondition && IsCurrentVertexWithinPaintColorConditions(MaterialVertexIsOn, currentVertexColor, currentVertexWorldPosition, CurrentVertexNormal, CurrentBoneName, BoneWeight, physicalSurfaceToApply.PaintConditions, paintCondition_ColorStrengthIfConditionIsNotMet, paintCondition_SettingToApplyIfConditionIsNotMet))) {


					ShouldApplyColorOnAnyChannel = true;

					// Adds it together since there could be several of the same physics surfaces added, i.e. on the same channel, but with different conditions and strengths.


					switch (physicsSurfacesToApply.Key) {

					case 0:

						ApplyColorOnRedChannel = true;

						if (physicalSurfaceToApply.SettingToApplyWith == EApplyVertexColorSetting::EAddVertexColor)
							tryingToApplyColor_Red += physicalSurfaceToApply.StrengthToApply;

						else if (physicalSurfaceToApply.SettingToApplyWith == EApplyVertexColorSetting::ESetVertexColor)
							tryingToApplyColor_Red = physicalSurfaceToApply.StrengthToApply;

						break;

					case 1:

						ApplyColorOnGreenChannel = true;

						if (physicalSurfaceToApply.SettingToApplyWith == EApplyVertexColorSetting::EAddVertexColor)
							tryingToApplyColor_Green += physicalSurfaceToApply.StrengthToApply;

						else if (physicalSurfaceToApply.SettingToApplyWith == EApplyVertexColorSetting::ESetVertexColor)
							tryingToApplyColor_Green = physicalSurfaceToApply.StrengthToApply;

						break;

					case 2:

						ApplyColorOnBlueChannel = true;

						if (physicalSurfaceToApply.SettingToApplyWith == EApplyVertexColorSetting::EAddVertexColor)
							tryingToApplyColor_Blue += physicalSurfaceToApply.StrengthToApply;

						else if (physicalSurfaceToApply.SettingToApplyWith == EApplyVertexColorSetting::ESetVertexColor)
							tryingToApplyColor_Blue = physicalSurfaceToApply.StrengthToApply;

						break;

					case 3:

						ApplyColorOnAlphaChannel = true;

						if (physicalSurfaceToApply.SettingToApplyWith == EApplyVertexColorSetting::EAddVertexColor)
							tryingToApplyColor_Alpha += physicalSurfaceToApply.StrengthToApply;

						else if (physicalSurfaceToApply.SettingToApplyWith == EApplyVertexColorSetting::ESetVertexColor)
							tryingToApplyColor_Alpha = physicalSurfaceToApply.StrengthToApply;

						break;

					default:
						break;
					}
				}

				// Otherwise we can fall back to another color strength. If set to 0 then no difference will be made
				else {

					// If this strength is 0 and Add, then we move on to the next physics surface and checks that. 
					if (physicalSurfaceToApply.SettingToApplyWith == EApplyVertexColorSetting::EAddVertexColor) {

						if (paintCondition_ColorStrengthIfConditionIsNotMet != 0)
							ShouldApplyColorOnAnyChannel = true;
						else
							continue;
					}

					else if (physicalSurfaceToApply.SettingToApplyWith == EApplyVertexColorSetting::ESetVertexColor) {

						ShouldApplyColorOnAnyChannel = true;
					}


					switch (physicsSurfacesToApply.Key) {

					case 0:

						ApplyColorOnRedChannel = true;
						tryingToApplyColor_Red = paintCondition_ColorStrengthIfConditionIsNotMet;
						applyVertexColorSettingRedChannel = paintCondition_SettingToApplyIfConditionIsNotMet;

						break;

					case 1:

						ApplyColorOnGreenChannel = true;
						tryingToApplyColor_Green = paintCondition_ColorStrengthIfConditionIsNotMet;
						applyVertexColorSettingGreenChannel = paintCondition_SettingToApplyIfConditionIsNotMet;

						break;

					case 2:

						ApplyColorOnBlueChannel = true;
						tryingToApplyColor_Blue = paintCondition_ColorStrengthIfConditionIsNotMet;
						applyVertexColorSettingBlueChannel = paintCondition_SettingToApplyIfConditionIsNotMet;

						break;

					case 3:

						ApplyColorOnAlphaChannel = true;
						tryingToApplyColor_Alpha = paintCondition_ColorStrengthIfConditionIsNotMet;
						applyVertexColorSettingAlphaChannel = paintCondition_SettingToApplyIfConditionIsNotMet;

						break;

					default:
						break;
					}
				}
			}
		}



		if (!gotPhysicsSurfaceOnRed) {

			if (PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.ApplyWithSettingOnChannelsWithoutThePhysicsSurface == EApplyVertexColorSetting::EAddVertexColor) {

				if (PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.StrengtOnChannelsWithoutThePhysicsSurfaces != 0) {

					tryingToApplyColor_Red = PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.StrengtOnChannelsWithoutThePhysicsSurfaces;
					ApplyColorOnRedChannel = true;
					applyVertexColorSettingRedChannel = EApplyVertexColorSetting::EAddVertexColor;
				}
			}

			else if (PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.ApplyWithSettingOnChannelsWithoutThePhysicsSurface == EApplyVertexColorSetting::ESetVertexColor) {

				tryingToApplyColor_Red = PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.StrengtOnChannelsWithoutThePhysicsSurfaces;
				ApplyColorOnRedChannel = true;
				applyVertexColorSettingRedChannel = EApplyVertexColorSetting::ESetVertexColor;
			}
		}

		if (!gotPhysicsSurfaceOnGreen) {

			if (PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.ApplyWithSettingOnChannelsWithoutThePhysicsSurface == EApplyVertexColorSetting::EAddVertexColor) {

				if (PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.StrengtOnChannelsWithoutThePhysicsSurfaces != 0) {

					tryingToApplyColor_Green = PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.StrengtOnChannelsWithoutThePhysicsSurfaces;
					ApplyColorOnGreenChannel = true;
					applyVertexColorSettingGreenChannel = EApplyVertexColorSetting::EAddVertexColor;
				}
			}

			else if (PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.ApplyWithSettingOnChannelsWithoutThePhysicsSurface == EApplyVertexColorSetting::ESetVertexColor) {

				tryingToApplyColor_Green = PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.StrengtOnChannelsWithoutThePhysicsSurfaces;
				ApplyColorOnGreenChannel = true;
				applyVertexColorSettingGreenChannel = EApplyVertexColorSetting::ESetVertexColor;
			}
		}

		if (!gotPhysicsSurfaceOnBlue) {

			if (PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.ApplyWithSettingOnChannelsWithoutThePhysicsSurface == EApplyVertexColorSetting::EAddVertexColor) {

				if (PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.StrengtOnChannelsWithoutThePhysicsSurfaces != 0) {

					tryingToApplyColor_Blue = PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.StrengtOnChannelsWithoutThePhysicsSurfaces;
					ApplyColorOnBlueChannel = true;
					applyVertexColorSettingBlueChannel = EApplyVertexColorSetting::EAddVertexColor;
				}
			}

			else if (PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.ApplyWithSettingOnChannelsWithoutThePhysicsSurface == EApplyVertexColorSetting::ESetVertexColor) {

				tryingToApplyColor_Blue = PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.StrengtOnChannelsWithoutThePhysicsSurfaces;
				ApplyColorOnBlueChannel = true;
				applyVertexColorSettingBlueChannel = EApplyVertexColorSetting::ESetVertexColor;
			}
		}

		if (!gotPhysicsSurfaceOnAlpha) {

			if (PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.ApplyWithSettingOnChannelsWithoutThePhysicsSurface == EApplyVertexColorSetting::EAddVertexColor) {

				if (PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.StrengtOnChannelsWithoutThePhysicsSurfaces != 0) {

					tryingToApplyColor_Alpha = PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.StrengtOnChannelsWithoutThePhysicsSurfaces;
					ApplyColorOnAlphaChannel = true;
					applyVertexColorSettingAlphaChannel = EApplyVertexColorSetting::EAddVertexColor;
				}
			}

			else if (PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.ApplyWithSettingOnChannelsWithoutThePhysicsSurface == EApplyVertexColorSetting::ESetVertexColor) {

				tryingToApplyColor_Alpha = PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.StrengtOnChannelsWithoutThePhysicsSurfaces;
				ApplyColorOnAlphaChannel = true;
				applyVertexColorSettingAlphaChannel = EApplyVertexColorSetting::ESetVertexColor;
			}
		}
	}

	else {

		applyVertexColorSettingRedChannel = PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsOnRedChannel.ApplyWithSetting;
		applyVertexColorSettingGreenChannel = PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.ApplyWithSetting;
		applyVertexColorSettingBlueChannel = PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.ApplyWithSetting;
		applyVertexColorSettingAlphaChannel = PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.ApplyWithSetting;

		paintLimitOnRedChannel = PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsOnRedChannel.PaintLimit;
		paintLimitOnGreenChannel = PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.PaintLimit;
		paintLimitOnBlueChannel = PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.PaintLimit;
		paintLimitOnAlphaChannel = PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.PaintLimit;

		lerpToTargetRedChannel = PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsOnRedChannel.LerpVertexColorToTarget;
		lerpToTargetGreenChannel = PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.LerpVertexColorToTarget;
		lerpToTargetBlueChannel = PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.LerpVertexColorToTarget;
		lerpToTargetAlphaChannel = PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.LerpVertexColorToTarget;


		if (hasAnyPaintCondition) {


			if (hasPaintConditionOnRedChannel) {

				if (IsCurrentVertexWithinPaintColorConditions(MaterialVertexIsOn, currentVertexColor, currentVertexWorldPosition, CurrentVertexNormal, CurrentBoneName, BoneWeight, PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsOnRedChannel.PaintConditions, paintCondition_ColorStrengthIfConditionIsNotMet, paintCondition_SettingToApplyIfConditionIsNotMet)) {

					ShouldApplyColorOnAnyChannel = true;
					tryingToApplyColor_Red = PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsOnRedChannel.AmountToApply;
				}

				else {

					tryingToApplyColor_Red = paintCondition_ColorStrengthIfConditionIsNotMet;
					applyVertexColorSettingRedChannel = paintCondition_SettingToApplyIfConditionIsNotMet;


					if (paintCondition_SettingToApplyIfConditionIsNotMet == EApplyVertexColorSetting::EAddVertexColor) {

						if (paintCondition_ColorStrengthIfConditionIsNotMet != 0)
							ShouldApplyColorOnAnyChannel = true;
					}

					else if (paintCondition_SettingToApplyIfConditionIsNotMet == EApplyVertexColorSetting::ESetVertexColor) {

						ShouldApplyColorOnAnyChannel = true;
					}
				}
			}

			else {

				ShouldApplyColorOnAnyChannel = true;
				tryingToApplyColor_Red = PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsOnRedChannel.AmountToApply;
			}


			if (hasPaintConditionOnGreenChannel) {

				if (IsCurrentVertexWithinPaintColorConditions(MaterialVertexIsOn, currentVertexColor, currentVertexWorldPosition, CurrentVertexNormal, CurrentBoneName, BoneWeight, PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.PaintConditions, paintCondition_ColorStrengthIfConditionIsNotMet, paintCondition_SettingToApplyIfConditionIsNotMet)) {

					ShouldApplyColorOnAnyChannel = true;
					tryingToApplyColor_Green = PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.AmountToApply;
				}

				else {

					tryingToApplyColor_Green = paintCondition_ColorStrengthIfConditionIsNotMet;
					applyVertexColorSettingGreenChannel = paintCondition_SettingToApplyIfConditionIsNotMet;


					if (paintCondition_SettingToApplyIfConditionIsNotMet == EApplyVertexColorSetting::EAddVertexColor) {

						if (paintCondition_ColorStrengthIfConditionIsNotMet != 0)
							ShouldApplyColorOnAnyChannel = true;
					}

					else if (paintCondition_SettingToApplyIfConditionIsNotMet == EApplyVertexColorSetting::ESetVertexColor) {

						ShouldApplyColorOnAnyChannel = true;
					}
				}
			}

			else {

				ShouldApplyColorOnAnyChannel = true;
				tryingToApplyColor_Green = PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.AmountToApply;
			}


			if (hasPaintConditionOnBlueChannel) {

				if (IsCurrentVertexWithinPaintColorConditions(MaterialVertexIsOn, currentVertexColor, currentVertexWorldPosition, CurrentVertexNormal, CurrentBoneName, BoneWeight, PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.PaintConditions, paintCondition_ColorStrengthIfConditionIsNotMet, paintCondition_SettingToApplyIfConditionIsNotMet)) {

					ShouldApplyColorOnAnyChannel = true;
					tryingToApplyColor_Blue = PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.AmountToApply;
				}

				else {

					tryingToApplyColor_Blue = paintCondition_ColorStrengthIfConditionIsNotMet;
					applyVertexColorSettingBlueChannel = paintCondition_SettingToApplyIfConditionIsNotMet;


					if (paintCondition_SettingToApplyIfConditionIsNotMet == EApplyVertexColorSetting::EAddVertexColor) {

						if (paintCondition_ColorStrengthIfConditionIsNotMet != 0)
							ShouldApplyColorOnAnyChannel = true;
					}

					else if (paintCondition_SettingToApplyIfConditionIsNotMet == EApplyVertexColorSetting::ESetVertexColor) {

						ShouldApplyColorOnAnyChannel = true;
					}
				}
			}

			else {

				ShouldApplyColorOnAnyChannel = true;
				tryingToApplyColor_Blue = PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.AmountToApply;
			}


			if (hasPaintConditionOnAlphaChannel) {

				if (IsCurrentVertexWithinPaintColorConditions(MaterialVertexIsOn, currentVertexColor, currentVertexWorldPosition, CurrentVertexNormal, CurrentBoneName, BoneWeight, PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.PaintConditions, paintCondition_ColorStrengthIfConditionIsNotMet, paintCondition_SettingToApplyIfConditionIsNotMet)) {

					ShouldApplyColorOnAnyChannel = true;
					tryingToApplyColor_Alpha = PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.AmountToApply;
				}

				else {

					tryingToApplyColor_Alpha = paintCondition_ColorStrengthIfConditionIsNotMet;
					applyVertexColorSettingAlphaChannel = paintCondition_SettingToApplyIfConditionIsNotMet;


					if (paintCondition_SettingToApplyIfConditionIsNotMet == EApplyVertexColorSetting::EAddVertexColor) {

						if (paintCondition_ColorStrengthIfConditionIsNotMet != 0)
							ShouldApplyColorOnAnyChannel = true;
					}

					else if (paintCondition_SettingToApplyIfConditionIsNotMet == EApplyVertexColorSetting::ESetVertexColor) {

						ShouldApplyColorOnAnyChannel = true;
					}
				}
			}

			else {

				ShouldApplyColorOnAnyChannel = true;
				tryingToApplyColor_Alpha = PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.AmountToApply;
			}
		}

		else {


			ShouldApplyColorOnAnyChannel = true;

			tryingToApplyColor_Red = PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsOnRedChannel.AmountToApply;
			tryingToApplyColor_Green = PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.AmountToApply;
			tryingToApplyColor_Blue = PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.AmountToApply;
			tryingToApplyColor_Alpha = PaintOnMeshColorSetting.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.AmountToApply;
		}
	}


	FColor getColorToApplyOnVertex_ColorToApply = currentVertexColor;
	float getColorToApplyOnVertex_ColorRed = static_cast<float>(currentVertexColor.R);
	float getColorToApplyOnVertex_ColorGreen = static_cast<float>(currentVertexColor.G);
	float getColorToApplyOnVertex_ColorBlue = static_cast<float>(currentVertexColor.B);
	float getColorToApplyOnVertex_ColorAlpha = static_cast<float>(currentVertexColor.A);


	float getColorToApplyOnVertex_StrengthAfterFalloff = 1;


	if (FallOffSettings.FallOffStrength > 0) {

		// Depending on the vertex pos, sets a distance  between 0-1. Starts the falloff where the full area effect ends
		const float getColorToApplyOnVertex_Distance = UKismetMathLibrary::MapRangeClamped(DistanceFromFalloffBaseToVertexLocation, ScaleFallOffFrom, FallOffRange, 0, 1);

		/* Usedful Debug to check the falloff strength visually

		FColor color = FColor::Purple;
		color = UKismetMathLibrary::LinearColorLerp(FLinearColor::Red, FLinearColor::Green, getColorToApplyOnVertex_Distance).ToFColor(false);

		if (!CalculateColorsInfoGlobal.TaskFundamentalSettings.MultiThreadSettings.UseMultithreadingForCalculations)
			DrawDebugPoint(CalculateColorsInfoGlobal.vertexPaintDetectionHitActor->GetWorld(), currentVertexWorldPosition, 5, color, false, CalculateColorsInfoGlobal.drawVertexesDebugPointsDuration, 0);
		*/

		// Clamps falloff to 0-1 in case user has set something else
		const float getColorToApplyOnVertex_FallOffClamped = UKismetMathLibrary::FClamp(FallOffSettings.FallOffStrength, 0, 1);

		// Converts to 1-0
		const float getColorToApplyOnVertex_Falloff = UKismetMathLibrary::MapRangeClamped(getColorToApplyOnVertex_FallOffClamped, 0, 1, 1, 0);

		// If falloff is 0, then it gives 1 even at the longest distance, if falloff is 1, then it scales to 0 at longest distance. 
		getColorToApplyOnVertex_StrengthAfterFalloff = UKismetMathLibrary::MapRangeClamped(getColorToApplyOnVertex_Distance, 0, 1, 1, getColorToApplyOnVertex_Falloff);


		if (ApplyColorOnRedChannel)
			tryingToApplyColor_Red *= getColorToApplyOnVertex_StrengthAfterFalloff;

		if (ApplyColorOnGreenChannel)
			tryingToApplyColor_Green *= getColorToApplyOnVertex_StrengthAfterFalloff;

		if (ApplyColorOnBlueChannel)
			tryingToApplyColor_Blue *= getColorToApplyOnVertex_StrengthAfterFalloff;

		if (ApplyColorOnAlphaChannel)
			tryingToApplyColor_Alpha *= getColorToApplyOnVertex_StrengthAfterFalloff;
	}


	// If set to limit colors at edge and falloff is less than one and have begun
	if (FallOffSettings.LimitFallOffColor && getColorToApplyOnVertex_StrengthAfterFalloff < 1) {


		float fallOffLimit_MaxAmountAtEdge_Adding = UKismetMathLibrary::MapRangeClamped(getColorToApplyOnVertex_StrengthAfterFalloff, 1, 0, 1, FallOffSettings.ColorLimitAtFallOffEdge);
		float fallOffLimit_MaxAmountAtEdge_Removing = UKismetMathLibrary::MapRangeClamped(getColorToApplyOnVertex_StrengthAfterFalloff, 0, 1, 1, FallOffSettings.ColorLimitAtFallOffEdge);
		float fallOffLimit_TotalColorAmountIfApplying = 0;

		// If set to limit the max paint strength at the falloff edge, then we check if we will get over that limit, depending on where on the falloff the vertex is. If at 1, i.e. the very edge of it, then we check using what we've set as the max allowed amount. If we're gonna be over it, then limits the paint we're trying to apply. 
		if (ApplyColorOnRedChannel) {

			fallOffLimit_TotalColorAmountIfApplying = UKismetMathLibrary::FClamp((currentLinearVertexColor.R + tryingToApplyColor_Red), 0, 1);

			// If trying to Add but isn't already full
			if (tryingToApplyColor_Red > 0 && currentLinearVertexColor.R < 1) {

				if (fallOffLimit_TotalColorAmountIfApplying > fallOffLimit_MaxAmountAtEdge_Adding) {

					// If going to go past the limit, then only removes the diff up until the limit and not past it. If already past the limit then just 0 so we don't refill places if the falloff source is moving away. 
					if (currentLinearVertexColor.R < fallOffLimit_MaxAmountAtEdge_Adding)
						tryingToApplyColor_Red = UKismetMathLibrary::FClamp(fallOffLimit_MaxAmountAtEdge_Adding - currentLinearVertexColor.R, -1, 1);
					else
						tryingToApplyColor_Red = 0;
				}
			}

			// If trying to remove and there is anything to remove
			else if (tryingToApplyColor_Red < 0 && currentLinearVertexColor.R > 0) {

				if (fallOffLimit_TotalColorAmountIfApplying < fallOffLimit_MaxAmountAtEdge_Removing) {

					if (currentLinearVertexColor.R > fallOffLimit_MaxAmountAtEdge_Removing)
						tryingToApplyColor_Red = UKismetMathLibrary::FClamp(fallOffLimit_MaxAmountAtEdge_Removing - currentLinearVertexColor.R, -1, 1);
					else
						tryingToApplyColor_Red = 0;
				}
			}
		}

		if (ApplyColorOnGreenChannel) {

			fallOffLimit_TotalColorAmountIfApplying = UKismetMathLibrary::FClamp((currentLinearVertexColor.G + tryingToApplyColor_Green), 0, 1);

			if (tryingToApplyColor_Green > 0 && currentLinearVertexColor.G < 1) {

				if (fallOffLimit_TotalColorAmountIfApplying > fallOffLimit_MaxAmountAtEdge_Adding) {


					if (currentLinearVertexColor.G < fallOffLimit_MaxAmountAtEdge_Adding)
						tryingToApplyColor_Green = UKismetMathLibrary::FClamp(fallOffLimit_MaxAmountAtEdge_Adding - currentLinearVertexColor.G, -1, 1);
					else
						tryingToApplyColor_Green = 0;
				}
			}

			else if (tryingToApplyColor_Green < 0 && currentLinearVertexColor.G > 0) {

				if (fallOffLimit_TotalColorAmountIfApplying < fallOffLimit_MaxAmountAtEdge_Removing) {


					if (currentLinearVertexColor.G > fallOffLimit_MaxAmountAtEdge_Removing)
						tryingToApplyColor_Green = UKismetMathLibrary::FClamp(fallOffLimit_MaxAmountAtEdge_Removing - currentLinearVertexColor.G, -1, 1);
					else
						tryingToApplyColor_Green = 0;
				}
			}
		}

		if (ApplyColorOnBlueChannel) {


			fallOffLimit_TotalColorAmountIfApplying = UKismetMathLibrary::FClamp((currentLinearVertexColor.B + tryingToApplyColor_Blue), 0, 1);


			if (tryingToApplyColor_Blue > 0 && currentLinearVertexColor.B < 1) {

				if (fallOffLimit_TotalColorAmountIfApplying > fallOffLimit_MaxAmountAtEdge_Adding) {


					if (currentLinearVertexColor.B < fallOffLimit_MaxAmountAtEdge_Adding)
						tryingToApplyColor_Blue = UKismetMathLibrary::FClamp(fallOffLimit_MaxAmountAtEdge_Adding - currentLinearVertexColor.B, -1, 1);
					else
						tryingToApplyColor_Blue = 0;
				}
			}

			else if (tryingToApplyColor_Blue < 0 && currentLinearVertexColor.B > 0) {

				if (fallOffLimit_TotalColorAmountIfApplying < fallOffLimit_MaxAmountAtEdge_Removing) {


					if (currentLinearVertexColor.B > fallOffLimit_MaxAmountAtEdge_Removing)
						tryingToApplyColor_Blue = UKismetMathLibrary::FClamp(fallOffLimit_MaxAmountAtEdge_Removing - currentLinearVertexColor.B, -1, 1);
					else
						tryingToApplyColor_Blue = 0;
				}
			}
		}

		if (ApplyColorOnAlphaChannel) {

			fallOffLimit_TotalColorAmountIfApplying = UKismetMathLibrary::FClamp((currentLinearVertexColor.A + tryingToApplyColor_Alpha), 0, 1);

			if (tryingToApplyColor_Alpha > 0 && currentLinearVertexColor.A < 1) {

				if (fallOffLimit_TotalColorAmountIfApplying > fallOffLimit_MaxAmountAtEdge_Adding) {

					if (currentLinearVertexColor.A < fallOffLimit_MaxAmountAtEdge_Adding)
						tryingToApplyColor_Alpha = UKismetMathLibrary::FClamp(fallOffLimit_MaxAmountAtEdge_Adding - currentLinearVertexColor.A, -1, 1);
					else
						tryingToApplyColor_Alpha = 0;
				}
			}

			else if (tryingToApplyColor_Alpha < 0 && currentLinearVertexColor.A > 0) {

				if (fallOffLimit_TotalColorAmountIfApplying < fallOffLimit_MaxAmountAtEdge_Removing) {


					if (currentLinearVertexColor.A > fallOffLimit_MaxAmountAtEdge_Removing)
						tryingToApplyColor_Alpha = UKismetMathLibrary::FClamp(fallOffLimit_MaxAmountAtEdge_Removing - currentLinearVertexColor.A, -1, 1);
					else
						tryingToApplyColor_Alpha = 0;
				}
			}
		}
	}


	if (ApplyColorOnRedChannel) {

		if (lerpToTargetRedChannel.LerpToTarget) {

			lerpToTargetValue = UKismetMathLibrary::MapRangeClamped(lerpToTargetRedChannel.TargetValue, 0, 1, 0, 255);
			getColorToApplyOnVertex_ColorRed = FMath::Lerp(getColorToApplyOnVertex_ColorRed, lerpToTargetValue, lerpToTargetRedChannel.LerpStrength);
		}

		else if (applyVertexColorSettingRedChannel == EApplyVertexColorSetting::EAddVertexColor) {

			tryingToApplyColor_Red = UKismetMathLibrary::MapRangeClamped(tryingToApplyColor_Red, -1, 1, -255, 255);
			getColorToApplyOnVertex_ColorRed += tryingToApplyColor_Red;
			getColorToApplyOnVertex_ColorRed = UKismetMathLibrary::FClamp(getColorToApplyOnVertex_ColorRed, 0, 255);
		}

		else if (applyVertexColorSettingRedChannel == EApplyVertexColorSetting::ESetVertexColor) {

			getColorToApplyOnVertex_ColorRed = UKismetMathLibrary::MapRangeClamped(tryingToApplyColor_Red, 0, 1, 0, 255);
		}
	}

	if (ApplyColorOnGreenChannel) {


		if (lerpToTargetGreenChannel.LerpToTarget) {

			lerpToTargetValue = UKismetMathLibrary::MapRangeClamped(lerpToTargetGreenChannel.TargetValue, 0, 1, 0, 255);
			getColorToApplyOnVertex_ColorGreen = FMath::Lerp(getColorToApplyOnVertex_ColorGreen, lerpToTargetValue, lerpToTargetGreenChannel.LerpStrength);
		}

		else if (applyVertexColorSettingGreenChannel == EApplyVertexColorSetting::EAddVertexColor) {

			tryingToApplyColor_Green = UKismetMathLibrary::MapRangeClamped(tryingToApplyColor_Green, -1, 1, -255, 255);
			getColorToApplyOnVertex_ColorGreen += tryingToApplyColor_Green;
			getColorToApplyOnVertex_ColorGreen = UKismetMathLibrary::FClamp(getColorToApplyOnVertex_ColorGreen, 0, 255);
		}

		else if (applyVertexColorSettingGreenChannel == EApplyVertexColorSetting::ESetVertexColor) {

			getColorToApplyOnVertex_ColorGreen = UKismetMathLibrary::MapRangeClamped(tryingToApplyColor_Green, 0, 1, 0, 255);
		}
	}

	if (ApplyColorOnBlueChannel) {


		if (lerpToTargetBlueChannel.LerpToTarget) {

			lerpToTargetValue = UKismetMathLibrary::MapRangeClamped(lerpToTargetBlueChannel.TargetValue, 0, 1, 0, 255);
			getColorToApplyOnVertex_ColorBlue = FMath::Lerp(getColorToApplyOnVertex_ColorBlue, lerpToTargetValue, lerpToTargetBlueChannel.LerpStrength);
		}

		else if (applyVertexColorSettingBlueChannel == EApplyVertexColorSetting::EAddVertexColor) {

			tryingToApplyColor_Blue = UKismetMathLibrary::MapRangeClamped(tryingToApplyColor_Blue, -1, 1, -255, 255);
			getColorToApplyOnVertex_ColorBlue += tryingToApplyColor_Blue;
			getColorToApplyOnVertex_ColorBlue = UKismetMathLibrary::FClamp(getColorToApplyOnVertex_ColorBlue, 0, 255);
		}

		else if (applyVertexColorSettingBlueChannel == EApplyVertexColorSetting::ESetVertexColor) {

			getColorToApplyOnVertex_ColorBlue = UKismetMathLibrary::MapRangeClamped(tryingToApplyColor_Blue, 0, 1, 0, 255);
		}
	}

	if (ApplyColorOnAlphaChannel) {


		if (lerpToTargetAlphaChannel.LerpToTarget) {

			lerpToTargetValue = UKismetMathLibrary::MapRangeClamped(lerpToTargetAlphaChannel.TargetValue, 0, 1, 0, 255);
			getColorToApplyOnVertex_ColorAlpha = FMath::Lerp(getColorToApplyOnVertex_ColorAlpha, lerpToTargetValue, lerpToTargetAlphaChannel.LerpStrength);
		}

		else if (applyVertexColorSettingAlphaChannel == EApplyVertexColorSetting::EAddVertexColor) {

			tryingToApplyColor_Alpha = UKismetMathLibrary::MapRangeClamped(tryingToApplyColor_Alpha, -1, 1, -255, 255);
			getColorToApplyOnVertex_ColorAlpha += tryingToApplyColor_Alpha;
			getColorToApplyOnVertex_ColorAlpha = UKismetMathLibrary::FClamp(getColorToApplyOnVertex_ColorAlpha, 0, 255);
		}

		else if (applyVertexColorSettingAlphaChannel == EApplyVertexColorSetting::ESetVertexColor) {

			getColorToApplyOnVertex_ColorAlpha = UKismetMathLibrary::MapRangeClamped(tryingToApplyColor_Alpha, 0, 1, 0, 255);
		}
	}


	if (paintLimitOnRedChannel.UsePaintLimits || paintLimitOnGreenChannel.UsePaintLimits || paintLimitOnBlueChannel.UsePaintLimits || paintLimitOnAlphaChannel.UsePaintLimits) {

		GetVertexColorAfterAnyLimitation(currentVertexColor, paintLimitOnRedChannel, paintLimitOnGreenChannel, paintLimitOnBlueChannel, paintLimitOnAlphaChannel, getColorToApplyOnVertex_ColorRed, getColorToApplyOnVertex_ColorGreen, getColorToApplyOnVertex_ColorBlue, getColorToApplyOnVertex_ColorAlpha, getColorToApplyOnVertex_ColorRed, getColorToApplyOnVertex_ColorGreen, getColorToApplyOnVertex_ColorBlue, getColorToApplyOnVertex_ColorAlpha);
	}


	if (ApplyColorOnRedChannel) {

		if (getColorToApplyOnVertex_ColorRed > 0)
			getColorToApplyOnVertex_ColorToApply.R = static_cast<uint8>(FMath::RoundToInt(UKismetMathLibrary::FClamp(getColorToApplyOnVertex_ColorRed, 0, 255)));
		else
			getColorToApplyOnVertex_ColorToApply.R = 0;
	}
	
	if (ApplyColorOnGreenChannel) {

		if (getColorToApplyOnVertex_ColorGreen > 0)
			getColorToApplyOnVertex_ColorToApply.G = static_cast<uint8>(FMath::RoundToInt(UKismetMathLibrary::FClamp(getColorToApplyOnVertex_ColorGreen, 0, 255)));
		else
			getColorToApplyOnVertex_ColorToApply.G = 0;
	}
	
	if (ApplyColorOnBlueChannel) {

		if (getColorToApplyOnVertex_ColorBlue > 0)
			getColorToApplyOnVertex_ColorToApply.B = static_cast<uint8>(FMath::RoundToInt(UKismetMathLibrary::FClamp(getColorToApplyOnVertex_ColorBlue, 0, 255)));
		else
			getColorToApplyOnVertex_ColorToApply.B = 0;
	}
	
	if (ApplyColorOnAlphaChannel) {

		if (getColorToApplyOnVertex_ColorAlpha > 0)
			getColorToApplyOnVertex_ColorToApply.A = static_cast<uint8>(FMath::RoundToInt(UKismetMathLibrary::FClamp(getColorToApplyOnVertex_ColorAlpha, 0, 255)));
		else
			getColorToApplyOnVertex_ColorToApply.A = 0;
	}
	

	// If set to do so then we run an Interface with Vertex Information, and returns whether we should apply the color, and with what amount. This means that the user can in whatever code class or BP that implements the interface create their own paint conditions! NOTE doesn't do the OnlyRunOverrideInterfaceIfTryingToApplyColorToVertex check since just by getting here we know we're trying to apply colors so we should run this either way. 
	if (PaintOnMeshColorSetting.OverrideVertexColorsToApplySettings.OverrideVertexColorsToApply) {
		
		if (ObjectToOverrideVertexColorsWith) {

			TEnumAsByte<EPhysicalSurface> overrideVertexColors_MostDominantPhysicsSurfaceAtVertexBeforeApplyingColors = EPhysicalSurface::SurfaceType_Default;
			float overrideVertexColors_MostDominantPhysicsSurfaceValueAtVertexBeforeApplyingColors = 0;

			if (PaintOnMeshColorSetting.OverrideVertexColorsToApplySettings.IncludeMostDominantPhysicsSurface) {


				TArray<float> overrideVertexColors_DefaultVertexColors;
				overrideVertexColors_DefaultVertexColors.Add(UVertexPaintFunctionLibrary::ReliableFColorToFLinearColor(currentVertexColor).R);
				overrideVertexColors_DefaultVertexColors.Add(UVertexPaintFunctionLibrary::ReliableFColorToFLinearColor(currentVertexColor).G);
				overrideVertexColors_DefaultVertexColors.Add(UVertexPaintFunctionLibrary::ReliableFColorToFLinearColor(currentVertexColor).B);
				overrideVertexColors_DefaultVertexColors.Add(UVertexPaintFunctionLibrary::ReliableFColorToFLinearColor(currentVertexColor).A);

				if (CalculateColorsInfoGlobal.TaskFundamentalSettings.TaskWorld.IsValid()) {

					UVertexPaintFunctionLibrary::GetTheMostDominantPhysicsSurface_Wrapper(CalculateColorsInfoGlobal.TaskFundamentalSettings.TaskWorld.Get(), MaterialVertexIsOn, RegisteredPhysicsSurfacesAtMaterialVertexIsOn, overrideVertexColors_DefaultVertexColors, overrideVertexColors_MostDominantPhysicsSurfaceAtVertexBeforeApplyingColors, overrideVertexColors_MostDominantPhysicsSurfaceValueAtVertexBeforeApplyingColors);
				}
			}

			HaveRunOverrideInterface = true;
			
			FColor overrideVertexColors_ColorToApply = FColor();
			bool overrideVertexColorsToApply = false;
			
			IVertexPaintDetectionInterface::Execute_OverrideVertexColorToApply(ObjectToOverrideVertexColorsWith, PaintOnMeshColorSetting.OverrideVertexColorsToApplySettings.OverrideVertexColorsToApplyID, AssociatedPaintComponent, PaintOnMeshColorSetting.GetMeshComponent(), CurrentLOD, CurrentVertexIndex, MaterialVertexIsOn, isVertexOnCloth, CurrentBoneName, currentVertexWorldPosition, CurrentVertexNormal, currentVertexColor, overrideVertexColors_MostDominantPhysicsSurfaceAtVertexBeforeApplyingColors, overrideVertexColors_MostDominantPhysicsSurfaceValueAtVertexBeforeApplyingColors, getColorToApplyOnVertex_ColorToApply, ShouldApplyColorOnAnyChannel, ShouldApplyColorOnAnyChannel, overrideVertexColorsToApply, overrideVertexColors_ColorToApply);

			// Clamps it so if you set like Red to be 0.000001, which would've been 0 when converted to FColor, but we clamp it to a higher value so it will be the smallest amount of 1 
			if (overrideVertexColorsToApply) {

				CalculateColorsInfoGlobal.OverridenVertexColors_MadeChangeToColorsToApply = true;
				getColorToApplyOnVertex_ColorToApply = overrideVertexColors_ColorToApply;
			}
		}
	}


	if (!ShouldApplyColorOnAnyChannel)
		return currentVertexColor;


	// If the current color and the color applied isn't the same it means that the paint resulted in change, which can be used in the Paint Event Dispatcher to only do things if a change occured
	if (currentVertexColor != getColorToApplyOnVertex_ColorToApply)
		VertexGotColorChanged = true;

	return getColorToApplyOnVertex_ColorToApply;
}


//-------------------------------------------------------

// Clamp Linear Color

FLinearColor FVertexPaintCalculateColorsTask::ClampLinearColor(FLinearColor LinearColor) {


	// Clamps it so if the user sets like 0.000001, we will return at the lowest amount of 1 when converted to FColor which range from 0-255

	if (LinearColor.R > 0)
		LinearColor.R = UKismetMathLibrary::FClamp(LinearColor.R, 0.005, 1);

	if (LinearColor.G > 0)
		LinearColor.G = UKismetMathLibrary::FClamp(LinearColor.G, 0.005, 1);

	if (LinearColor.B > 0)
		LinearColor.B = UKismetMathLibrary::FClamp(LinearColor.B, 0.005, 1);

	if (LinearColor.A > 0)
		LinearColor.A = UKismetMathLibrary::FClamp(LinearColor.A, 0.005, 1);

	return LinearColor;
}


//-------------------------------------------------------

// Does Vertex Colors Match

bool FVertexPaintCalculateColorsTask::DoesVertexColorsMatch(const FColor & VertexColor, const FColor & CompareColor, int32 ErrorTolerance, bool CompareOnRedChannel, bool CompareOnGreenChannel, bool CompareOnBlueChannel, bool CompareOnAlphaChannel) {


	// Diffs between 0-255, 255 is the biggest diff. int32 since otherwise we have issue when subtracting uint8 where it slings around. 
	const int32 redChannelDiff = FMath::Abs(static_cast<int32>(VertexColor.R) - static_cast<int32>(CompareColor.R));
	const int32 greenChannelDiff = FMath::Abs(static_cast<int32>(VertexColor.G) - static_cast<int32>(CompareColor.G));
	const int32 blueChannelDiff = FMath::Abs(static_cast<int32>(VertexColor.B) - static_cast<int32>(CompareColor.B));
	const int32 alphaChannelDiff = FMath::Abs(static_cast<int32>(VertexColor.A) - static_cast<int32>(CompareColor.A));

	if (CompareOnRedChannel && redChannelDiff > ErrorTolerance)
		return false;

	if (CompareOnGreenChannel && greenChannelDiff > ErrorTolerance)
		return false;

	if (CompareOnBlueChannel && blueChannelDiff > ErrorTolerance)
		return false;

	if (CompareOnAlphaChannel && alphaChannelDiff > ErrorTolerance)
		return false;


	return true;
}


//-------------------------------------------------------

// Is Set To Override Vertex Colors And Implements Interface

bool FVertexPaintCalculateColorsTask::IsSetToOverrideVertexColorsAndImplementsInterface(const FRVPDPPaintTaskSettings& VertexPaintSettings) {


	if (VertexPaintSettings.OverrideVertexColorsToApplySettings.OverrideVertexColorsToApply) {

		if (VertexPaintSettings.OverrideVertexColorsToApplySettings.ObjectToRunOverrideVertexColorsInterface && VertexPaintSettings.OverrideVertexColorsToApplySettings.ObjectToRunOverrideVertexColorsInterface->GetClass()->ImplementsInterface(UVertexPaintDetectionInterface::StaticClass())) {

			return true;
		}
	}

	return false;
}


//-------------------------------------------------------

// Get Paint Condition Adjusted If Set To Use Physics Surface

void FVertexPaintCalculateColorsTask::SetWithinColorRangePaintConditionChannelsIfUsingItWithPhysicsSurface(FRVPDPPaintConditionSettings& ChannelsPaintCondition, UMaterialInterface* Material) const {

	if (ChannelsPaintCondition.IfVertexColorIsWithinRange.Num() == 0) return;
	if (!IsValid(Material)) return;
	if (!CalculateColorsInfoGlobal.VertexPaintMaterialDataAsset) return;

	Material = CalculateColorsInfoGlobal.VertexPaintMaterialDataAsset->GetRegisteredMaterialFromMaterialInterface(Material);
	if (!IsValid(Material)) return; 


	for (int i = ChannelsPaintCondition.IfVertexColorIsWithinRange.Num()-1; i >= 0; i--) {

		// If set to Default then there's nothing to do here. If also the Channel is default, then we clear it out since it's an unnecessary condition. 
		if (ChannelsPaintCondition.IfVertexColorIsWithinRange[i].IfChannelWithPhysicsSurfaceIsWithinColorRange == EPhysicalSurface::SurfaceType_Default) {

			if (ChannelsPaintCondition.IfVertexColorIsWithinRange[i].IfVertexColorChannelWithinColorRange == ESurfaceAtChannel::Default)
				ChannelsPaintCondition.IfVertexColorIsWithinRange.RemoveAt(i);

			continue;
		}

		// If set to use the Within Color Range Condition using Physics Surface instead of Vertex Color Channel



		TEnumAsByte<EPhysicalSurface> withinColorRangePhysicsSurface = ChannelsPaintCondition.IfVertexColorIsWithinRange[i].IfChannelWithPhysicsSurfaceIsWithinColorRange;
		FRVPDPRegisteredMaterialSetting materialDataAssetInfo = CalculateColorsInfoGlobal.VertexPaintMaterialDataAsset->GetVertexPaintMaterialInterface().FindRef(Material);


		// First checks if the set physics surface is directly registered to any of the channels, and if so set the channel to be that. 
		if (materialDataAssetInfo.PaintedAtRed == withinColorRangePhysicsSurface) {

			ChannelsPaintCondition.IfVertexColorIsWithinRange[i].IfVertexColorChannelWithinColorRange = ESurfaceAtChannel::RedChannel;
			continue;
		}

		else if (materialDataAssetInfo.PaintedAtGreen == withinColorRangePhysicsSurface) {

			ChannelsPaintCondition.IfVertexColorIsWithinRange[i].IfVertexColorChannelWithinColorRange = ESurfaceAtChannel::GreenChannel;
			continue;
		}

		else if (materialDataAssetInfo.PaintedAtBlue == withinColorRangePhysicsSurface) {

			ChannelsPaintCondition.IfVertexColorIsWithinRange[i].IfVertexColorChannelWithinColorRange = ESurfaceAtChannel::BlueChannel;
			continue;
		}

		else if (materialDataAssetInfo.PaintedAtAlpha == withinColorRangePhysicsSurface) {

			ChannelsPaintCondition.IfVertexColorIsWithinRange[i].IfVertexColorChannelWithinColorRange = ESurfaceAtChannel::AlphaChannel;
			continue;
		}


		// Otherwise we fallback to checking registered parents. So if the user has set that a parent physics surface should be used with the condition, for instance Water, then we can still link it to a vertex channel if we find one that has it. 


		// Gets the Parents of the physicSurfaceWithinColorRange we want to use, so we can check if any of the registered physics surface is a part of the same family, and which channel. If so we can have the condition take effect on the channel that physics surface is registered to. 
		TArray<TEnumAsByte<EPhysicalSurface>> parentsOfWithinRangePhysicsSurfaces = CalculateColorsInfoGlobal.VertexPaintMaterialDataAsset->GetParentsOfPhysicsSurface(withinColorRangePhysicsSurface);

		bool setChannelFromParent = false;

		// Checks if the physics surface parents we have as within range condition, has the parents of whatever is registered in the channel. 
		for (TEnumAsByte<EPhysicalSurface> physicsSurfaceParentAtRedChannel : CalculateColorsInfoGlobal.VertexPaintMaterialDataAsset->GetParentsOfPhysicsSurface(materialDataAssetInfo.PaintedAtRed)) {

			if (parentsOfWithinRangePhysicsSurfaces.Contains(physicsSurfaceParentAtRedChannel)) {

				ChannelsPaintCondition.IfVertexColorIsWithinRange[i].IfVertexColorChannelWithinColorRange = ESurfaceAtChannel::RedChannel;
				setChannelFromParent = true;
				break;
			}
		}

		if (setChannelFromParent) continue;


		for (TEnumAsByte<EPhysicalSurface> physicsSurfaceParentAtGreenChannel : CalculateColorsInfoGlobal.VertexPaintMaterialDataAsset->GetParentsOfPhysicsSurface(materialDataAssetInfo.PaintedAtGreen)) {

			if (parentsOfWithinRangePhysicsSurfaces.Contains(physicsSurfaceParentAtGreenChannel)) {

				ChannelsPaintCondition.IfVertexColorIsWithinRange[i].IfVertexColorChannelWithinColorRange = ESurfaceAtChannel::GreenChannel;
				setChannelFromParent = true;
				break;
			}
		}

		if (setChannelFromParent) continue;


		for (TEnumAsByte<EPhysicalSurface> physicsSurfaceParentAtBlueChannel : CalculateColorsInfoGlobal.VertexPaintMaterialDataAsset->GetParentsOfPhysicsSurface(materialDataAssetInfo.PaintedAtBlue)) {

			if (parentsOfWithinRangePhysicsSurfaces.Contains(physicsSurfaceParentAtBlueChannel)) {

				ChannelsPaintCondition.IfVertexColorIsWithinRange[i].IfVertexColorChannelWithinColorRange = ESurfaceAtChannel::BlueChannel;
				setChannelFromParent = true;
				break;
			}
		}

		if (setChannelFromParent) continue;


		for (TEnumAsByte<EPhysicalSurface> physicsSurfaceParentAtAlphaChannel : CalculateColorsInfoGlobal.VertexPaintMaterialDataAsset->GetParentsOfPhysicsSurface(materialDataAssetInfo.PaintedAtAlpha)) {

			if (parentsOfWithinRangePhysicsSurfaces.Contains(physicsSurfaceParentAtAlphaChannel)) {

				ChannelsPaintCondition.IfVertexColorIsWithinRange[i].IfVertexColorChannelWithinColorRange = ESurfaceAtChannel::AlphaChannel;
				setChannelFromParent = true;
				break;
			}
		}

		if (setChannelFromParent) continue;


		// If was unable to get which channel we want to check with the Within Color Range Condition, then we can't use it, so we remove it. 

		ChannelsPaintCondition.IfVertexColorIsWithinRange.RemoveAt(i);
	}
}


//-------------------------------------------------------

// Get Colors Of Each Channel For Vertex

void FVertexPaintCalculateColorsTask::GetColorsOfEachChannelForVertex(const FLinearColor & CurrentVertexColor, const FRVPDPIncludeAmountOfPaintedColorsOfEachChannelSettings & IncludeAmountOfPaintedColorsOfEachValueSetting, bool HasRequiredPhysSurfaceOnRedChannel, bool HasRequiredPhysSurfaceOnGreenChannel, bool HasRequiredPhysSurfaceOnBlueChannel, bool HasRequiredPhysSurfaceOnAlphaChannel, FRVPDPAmountOfColorsOfEachChannelChannelResults & RedChannelResult, FRVPDPAmountOfColorsOfEachChannelChannelResults & GreenChannelResult, FRVPDPAmountOfColorsOfEachChannelChannelResults & BlueChannelResult, FRVPDPAmountOfColorsOfEachChannelChannelResults & AlphaChannelResult) {

	if (!IncludeAmountOfPaintedColorsOfEachValueSetting.IncludeVertexColorChannelResultOfEachChannel) return;


	// If has requirement that the channel has to have specific physics surfaces
	if (IncludeAmountOfPaintedColorsOfEachValueSetting.IncludeOnlyIfPhysicsSurfacesIsRegisteredToAnyVertexChannel.Num() > 0) {

		if (HasRequiredPhysSurfaceOnRedChannel) {

			// Eventhough it's not within the min requirements we gather how many where even considered, i.e. we can later check if any channel even got any considered, and if not then none had the requirements, like any of the physics surfaces required and thus shouldn't be successfull. 
			RedChannelResult.AmountOfVerticesConsidered++;

			if (CurrentVertexColor.R >= IncludeAmountOfPaintedColorsOfEachValueSetting.IncludeIfMinColorAmountIs) {

				RedChannelResult.AmountOfVerticesPaintedAtMinAmount++;
				RedChannelResult.AverageColorAmountAtMinAmount += CurrentVertexColor.R;
			}
		}

		if (HasRequiredPhysSurfaceOnGreenChannel) {

			GreenChannelResult.AmountOfVerticesConsidered++;

			if (CurrentVertexColor.G >= IncludeAmountOfPaintedColorsOfEachValueSetting.IncludeIfMinColorAmountIs) {

				GreenChannelResult.AmountOfVerticesPaintedAtMinAmount++;
				GreenChannelResult.AverageColorAmountAtMinAmount += CurrentVertexColor.G;
			}
		}

		if (HasRequiredPhysSurfaceOnBlueChannel) {

			BlueChannelResult.AmountOfVerticesConsidered++;

			if (CurrentVertexColor.B >= IncludeAmountOfPaintedColorsOfEachValueSetting.IncludeIfMinColorAmountIs) {

				BlueChannelResult.AmountOfVerticesPaintedAtMinAmount++;
				BlueChannelResult.AverageColorAmountAtMinAmount += CurrentVertexColor.B;
			}
		}

		if (HasRequiredPhysSurfaceOnAlphaChannel) {

			AlphaChannelResult.AmountOfVerticesConsidered++;

			if (CurrentVertexColor.A >= IncludeAmountOfPaintedColorsOfEachValueSetting.IncludeIfMinColorAmountIs) {

				AlphaChannelResult.AmountOfVerticesPaintedAtMinAmount++;
				AlphaChannelResult.AverageColorAmountAtMinAmount += CurrentVertexColor.A;
			}
		}
	}

	else {

		RedChannelResult.AmountOfVerticesConsidered++;

		if (CurrentVertexColor.R >= IncludeAmountOfPaintedColorsOfEachValueSetting.IncludeIfMinColorAmountIs) {

			RedChannelResult.AmountOfVerticesPaintedAtMinAmount++;
			RedChannelResult.AverageColorAmountAtMinAmount += CurrentVertexColor.R;
		}


		GreenChannelResult.AmountOfVerticesConsidered++;

		if (CurrentVertexColor.G >= IncludeAmountOfPaintedColorsOfEachValueSetting.IncludeIfMinColorAmountIs) {

			GreenChannelResult.AmountOfVerticesPaintedAtMinAmount++;
			GreenChannelResult.AverageColorAmountAtMinAmount += CurrentVertexColor.G;
		}

		BlueChannelResult.AmountOfVerticesConsidered++;

		if (CurrentVertexColor.B >= IncludeAmountOfPaintedColorsOfEachValueSetting.IncludeIfMinColorAmountIs) {

			BlueChannelResult.AmountOfVerticesPaintedAtMinAmount++;
			BlueChannelResult.AverageColorAmountAtMinAmount += CurrentVertexColor.B;
		}


		AlphaChannelResult.AmountOfVerticesConsidered++;

		if (CurrentVertexColor.A >= IncludeAmountOfPaintedColorsOfEachValueSetting.IncludeIfMinColorAmountIs) {

			AlphaChannelResult.AmountOfVerticesPaintedAtMinAmount++;
			AlphaChannelResult.AverageColorAmountAtMinAmount += CurrentVertexColor.A;
		}
	}
}


//-------------------------------------------------------

// Get Colors Of Each Physics Surface For Vertex

void FVertexPaintCalculateColorsTask::GetColorsOfEachPhysicsSurfaceForVertex(const TArray<TEnumAsByte<EPhysicalSurface>>&RegisteredPhysicsSurfaceAtMaterial, const FLinearColor & CurrentVertexColor, const FRVPDPIncludeAmountOfPaintedColorsOfEachChannelSettings & IncludeAmountOfPaintedColorsOfEachValueSetting, bool HasRequiredPhysSurfaceOnRedChannel, bool HasRequiredPhysSurfaceOnGreenChannel, bool HasRequiredPhysSurfaceOnBlueChannel, bool HasRequiredPhysSurfaceOnAlphaChannel, TArray<FRVPDPAmountOfColorsOfEachChannelPhysicsResults>& PhysicsSurfaceResultToFill) {

	if (!IncludeAmountOfPaintedColorsOfEachValueSetting.IncludePhysicsSurfaceResultOfEachChannel) return;
	if (RegisteredPhysicsSurfaceAtMaterial.Num() == 0) return;


	// If has requires specific physics surfaces to be registered
	if (IncludeAmountOfPaintedColorsOfEachValueSetting.IncludeOnlyIfPhysicsSurfacesIsRegisteredToAnyVertexChannel.Num() > 0) {

		// if haven't gotten any
		if (!HasRequiredPhysSurfaceOnRedChannel && !HasRequiredPhysSurfaceOnGreenChannel && !HasRequiredPhysSurfaceOnBlueChannel && !HasRequiredPhysSurfaceOnAlphaChannel) return;
	}



	// Loops through surfaces registered at channel. If there just one registered at one channel this will still be 4
	for (int i = 0; i < RegisteredPhysicsSurfaceAtMaterial.Num(); i++) {

		if (RegisteredPhysicsSurfaceAtMaterial[i] != EPhysicalSurface::SurfaceType_Default) {

			float colorValue = 0;

			// Depending on which element we check different color channels
			switch (i) {

			case 0:

				// If requires that specific physics surfaces then skips if whatever physics surface that's on this channel isn't registered. 
				if (IncludeAmountOfPaintedColorsOfEachValueSetting.IncludeOnlyIfPhysicsSurfacesIsRegisteredToAnyVertexChannel.Num() > 0) {

					if (!HasRequiredPhysSurfaceOnRedChannel)
						continue;
				}

				colorValue = CurrentVertexColor.R;
				break;

			case 1:

				if (IncludeAmountOfPaintedColorsOfEachValueSetting.IncludeOnlyIfPhysicsSurfacesIsRegisteredToAnyVertexChannel.Num() > 0) {

					if (!HasRequiredPhysSurfaceOnGreenChannel)
						continue;
				}

				colorValue = CurrentVertexColor.G;
				break;

			case 2:

				if (IncludeAmountOfPaintedColorsOfEachValueSetting.IncludeOnlyIfPhysicsSurfacesIsRegisteredToAnyVertexChannel.Num() > 0) {

					if (!HasRequiredPhysSurfaceOnBlueChannel)
						continue;
				}

				colorValue = CurrentVertexColor.B;
				break;

			case 3:

				if (IncludeAmountOfPaintedColorsOfEachValueSetting.IncludeOnlyIfPhysicsSurfacesIsRegisteredToAnyVertexChannel.Num() > 0) {

					if (!HasRequiredPhysSurfaceOnAlphaChannel)
						continue;
				}

				colorValue = CurrentVertexColor.A;
				break;

			default:
				break;
			}


			PhysicsSurfaceResultToFill[i].AmountOfVerticesConsidered++;

			if (colorValue >= IncludeAmountOfPaintedColorsOfEachValueSetting.IncludeIfMinColorAmountIs) {

				PhysicsSurfaceResultToFill[i].AmountOfVerticesPaintedAtMinAmount++;
				PhysicsSurfaceResultToFill[i].AverageColorAmountAtMinAmount += colorValue;
			}
		}
	}
}


//-------------------------------------------------------

// Get If Physics Surfaces Is Registered To Vertex Color Channels

void FVertexPaintCalculateColorsTask::GetIfPhysicsSurfacesIsRegisteredToVertexColorChannels(const UWorld* World, bool IncludeVertexColorChannelResultOfEachChannel, bool IncludePhysicsSurfaceResultOfEachChannel, const TArray<TEnumAsByte<EPhysicalSurface>>& IncludeOnlyIfPhysicsSurfaceIsRegisteredToAnyChannel, const TArray<TEnumAsByte<EPhysicalSurface>>& PhysicsSurfacesAtMaterial, bool& ContainsPhysicsSurfaceOnRedChannel, bool& ContainsPhysicsSurfaceOnGreenChannel, bool& ContainsPhysicsSurfaceOnBlueChannel, bool& ContainsPhysicsSurfaceOnAlphaChannel) {

	if (IncludeOnlyIfPhysicsSurfaceIsRegisteredToAnyChannel.Num() == 0) return;
	if (!World) return;


	if (IncludeVertexColorChannelResultOfEachChannel || IncludePhysicsSurfaceResultOfEachChannel) {

		for (int i = 0; i < PhysicsSurfacesAtMaterial.Num(); i++) {

			for (auto physicsSurface : IncludeOnlyIfPhysicsSurfaceIsRegisteredToAnyChannel) {

				// If physics surface belongs to the family or is the specified surface we require to include. For instance if set to only include Wet physics surface, and the channel has Cobble-Puddle (which is registered under the Wet Family), then it will pass, same if set to only include Cobble-Puddle since it's the same as the channel has. 
				if (PhysicsSurfacesAtMaterial[i] == physicsSurface || UVertexPaintFunctionLibrary::DoesPhysicsSurfaceBelongToPhysicsSurfaceFamily(World, PhysicsSurfacesAtMaterial[i], physicsSurface)) {

					switch (i) {

					case 0:
						ContainsPhysicsSurfaceOnRedChannel = true;
						break;
					case 1:
						ContainsPhysicsSurfaceOnGreenChannel = true;
						break;
					case 2:
						ContainsPhysicsSurfaceOnBlueChannel = true;
						break;
					case 3:
						ContainsPhysicsSurfaceOnAlphaChannel = true;
						break;
					default:
						break;
					}

					break;
				}
			}
		}
	}
}


//-------------------------------------------------------

// Set All Color Channels To Have Been Changed

void FVertexPaintCalculateColorsTask::SetAllColorChannelsToHaveBeenChanged() {

	if (!CalculateColorsInfoGlobal.PaintTaskResult.ColorAppliedToVertexColorChannels.Contains(EVertexColorChannel::RedChannel))
		CalculateColorsInfoGlobal.PaintTaskResult.ColorAppliedToVertexColorChannels.Add(EVertexColorChannel::RedChannel);

	if (!CalculateColorsInfoGlobal.PaintTaskResult.ColorAppliedToVertexColorChannels.Contains(EVertexColorChannel::GreenChannel))
		CalculateColorsInfoGlobal.PaintTaskResult.ColorAppliedToVertexColorChannels.Add(EVertexColorChannel::GreenChannel);

	if (!CalculateColorsInfoGlobal.PaintTaskResult.ColorAppliedToVertexColorChannels.Contains(EVertexColorChannel::BlueChannel))
		CalculateColorsInfoGlobal.PaintTaskResult.ColorAppliedToVertexColorChannels.Add(EVertexColorChannel::BlueChannel);

	if (!CalculateColorsInfoGlobal.PaintTaskResult.ColorAppliedToVertexColorChannels.Contains(EVertexColorChannel::AlphaChannel))
		CalculateColorsInfoGlobal.PaintTaskResult.ColorAppliedToVertexColorChannels.Add(EVertexColorChannel::AlphaChannel);
}


//-------------------------------------------------------

// Get Skeletal Mesh Cloth Info

#if ENGINE_MAJOR_VERSION == 4

void FVertexPaintCalculateColorsTask::GetSkeletalMeshClothInfo(USkeletalMeshComponent* SkeletalMeshComponent, USkeletalMesh* SkeletalMesh, TArray<FName> BoneNames, TArray<UClothingAssetBase*>& ClothingAssets, TMap<int32, TArray<FVector>>& ClothvertexPositions, TMap<int32, TArray<FVector>>& ClothVertexNormals, TArray<FVector>& ClothBoneLocationInComponentSpace, TArray<FQuat>& ClothBoneQuaternionsInComponentSpace) const {

#elif ENGINE_MAJOR_VERSION == 5

void FVertexPaintCalculateColorsTask::GetSkeletalMeshClothInfo(USkeletalMeshComponent* SkeletalMeshComponent, USkeletalMesh* SkeletalMesh, TArray<FName> BoneNames, TArray<UClothingAssetBase*>&ClothingAssets, TMap<int32, TArray<FVector3f>>&ClothvertexPositions, TMap<int32, TArray<FVector3f>>&ClothVertexNormals, TArray<FVector>&ClothBoneLocationInComponentSpace, TArray<FQuat>&ClothBoneQuaternionsInComponentSpace) const {

#endif

	if (!SkeletalMesh) return;
	if (!IsValid(SkeletalMeshComponent)) return;

	const FSkeletalMeshRenderData* skeletalMeshRenderData = SkeletalMeshComponent->GetSkeletalMeshRenderData();

	if (!skeletalMeshRenderData->LODRenderData.IsValidIndex(SkeletalMeshComponent->GetPredictedLODLevel())) return;
	if (!skeletalMeshRenderData->LODRenderData[SkeletalMeshComponent->GetPredictedLODLevel()].HasClothData()) return;


	TMap<int32, FClothSimulData> clothSimulationData;

	if (InGameThread)
		clothSimulationData = SkeletalMeshComponent->GetCurrentClothingData_GameThread();
	else
		clothSimulationData = SkeletalMeshComponent->GetCurrentClothingData_AnyThread();

	ClothingAssets = SkeletalMesh->GetMeshClothingAssets();


	for (int i = 0; i < ClothingAssets.Num(); i++) {

		FClothSimulData simulationData = clothSimulationData.FindRef(i);

		if (clothSimulationData.Contains(i)) {

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4


			// From 5.4 and up we couldn't just get the cloth pos immediately because of memory optimizations but have to loop through them. But since you usually don't have that much cloth i don't think it should affect task speed to much. 

			int amountOfClothVertices = simulationData.Positions.Num();

			// Initialize the arrays only once and ensure the types match the ones returned from your data accessors.
			TArray<UE::Math::TVector<float>, FDefaultAllocator> clothVertexPositionsFromSimulData;
			TArray<UE::Math::TVector<float>, FDefaultAllocator> clothVertexNormalsFromSimulData;

			clothVertexPositionsFromSimulData.SetNumUninitialized(amountOfClothVertices);
			clothVertexNormalsFromSimulData.SetNumUninitialized(amountOfClothVertices);

			// Directly copy the data from the arrays returned by the accessor methods.
			for (int32 j = 0; j < amountOfClothVertices; j++) {
				clothVertexPositionsFromSimulData[j] = simulationData.Positions[j];
				clothVertexNormalsFromSimulData[j] = simulationData.Normals[j];
			}

			ClothvertexPositions.Add(i, clothVertexPositionsFromSimulData);
			ClothVertexNormals.Add(i, clothVertexNormalsFromSimulData);

#else
			ClothvertexPositions.Add(i, simulationData.Positions);
			ClothVertexNormals.Add(i, simulationData.Normals);
#endif
		}


#if ENGINE_MAJOR_VERSION == 5

		if (ClothingAssets[i]) {

			if (auto clothingAssetCommon = Cast<UClothingAssetCommon>(ClothingAssets[i])) {

				if (BoneNames.IsValidIndex(clothingAssetCommon->ReferenceBoneIndex)) {

					FName boneName = BoneNames[clothingAssetCommon->ReferenceBoneIndex];
					FVector boneLocation = SkeletalMeshComponent->GetBoneLocation(boneName, EBoneSpaces::ComponentSpace);
					FQuat boneQuat = SkeletalMeshComponent->GetBoneQuaternion(boneName, EBoneSpaces::ComponentSpace);

					ClothBoneLocationInComponentSpace.Add(boneLocation);
					ClothBoneQuaternionsInComponentSpace.Add(boneQuat);
				}
			}
		}
#endif
	}
}


//-------------------------------------------------------

// Get Paint Within Area Info

bool FVertexPaintCalculateColorsTask::GetWithinAreaInfo(const UPrimitiveComponent* MeshComponent, TArray<FRVPDPComponentToCheckIfIsWithinAreaInfo>& ComponentsToCheckIfWithin) {

	if (ComponentsToCheckIfWithin.Num() == 0) return false;


	FTransform origoTransform = FTransform();
	origoTransform.SetIdentity();

	// Since complex shape we can be dependent on the component still existing and on the components current Location because we're tracing for it anyway. 
	for (int i = 0; i < ComponentsToCheckIfWithin.Num(); i++) {


		// When Task finally starts and its complex shape, we updates world transform based on where the component is now, since with complex we're dependent on the component actually existing since we're tracing for it
		if (ComponentsToCheckIfWithin[i].PaintWithinAreaShape == EPaintWithinAreaShape::IsComplexShape) {

			if (!IsValid(ComponentsToCheckIfWithin[i].GetComponentToCheckIfIsWithin())) return false;


			ComponentsToCheckIfWithin[i].ComponentWorldTransform = ComponentsToCheckIfWithin[i].GetComponentToCheckIfIsWithin()->GetComponentTransform();

			// Ignores self as we only want to check if hit on ComponentToCheckIfIsWithin
			FCollisionQueryParams complexShapeQueryParams;
			complexShapeQueryParams.AddIgnoredActor(MeshComponent->GetOwner());
			complexShapeQueryParams.bTraceComplex = ComponentsToCheckIfWithin[i].TraceComplexIfComplexShape;
			ComponentsToCheckIfWithin[i].Complex_TraceParams = complexShapeQueryParams;
		}

		// Other shapes we cache what we need and can transform it back and is not dependent on the actual component to check if within is still valid
		else {

			// Transforms from Local to World again so can check if the vertices are within the area where it was when the task got created.
			const FVector compsToCheckIfWithinWorldLocation = UKismetMathLibrary::TransformLocation(origoTransform, ComponentsToCheckIfWithin[i].ComponentRelativeLocationToMeshBeingPainted);
			ComponentsToCheckIfWithin[i].ComponentWorldTransform.SetLocation(compsToCheckIfWithinWorldLocation);


			const FRotator compsToCheckIfWithinWorldRotation = UKismetMathLibrary::TransformRotation(origoTransform, ComponentsToCheckIfWithin[i].ComponentRelativeRotationToMeshBeingPainted);
			ComponentsToCheckIfWithin[i].ComponentWorldTransform.SetRotation(compsToCheckIfWithinWorldRotation.Quaternion());


			const FVector compsToCheckIfWithinWorldScale = UKismetMathLibrary::TransformLocation(origoTransform, ComponentsToCheckIfWithin[i].ComponentRelativeScaleToMeshBeingPainted);
			ComponentsToCheckIfWithin[i].ComponentWorldTransform.SetScale3D(compsToCheckIfWithinWorldScale);
		}
	}

	return true;
}


//-------------------------------------------------------

// Set Line Of Sight Paint Condition Params

FRVPDPDoesVertexHasLineOfSightPaintConditionSettings FVertexPaintCalculateColorsTask::SetLineOfSightPaintConditionParams(FRVPDPDoesVertexHasLineOfSightPaintConditionSettings LineOfSightPaintConditionSettings) {


	FCollisionObjectQueryParams lineOfSightTraceObjectQueryParams(LineOfSightPaintConditionSettings.CheckLineOfSightAgainstObjectTypes);
	LineOfSightPaintConditionSettings.LineOfSightTraceObjectQueryParams = lineOfSightTraceObjectQueryParams;

	// Avoid profiling tools in shipping since they can add overhead, especially since we're tracing for every vertex
#if (UE_BUILD_DEVELOPMENT || UE_BUILD_DEBUG || WITH_EDITOR)
	FCollisionQueryParams lineOfSightTraceQueryParams("LineOfSightPaintConditionTrace", SCENE_QUERY_STAT_ONLY(LineOfSightPaintConditionTrace), LineOfSightPaintConditionSettings.TraceForComplex);
#else
	FCollisionQueryParams lineOfSightTraceQueryParams("LineOfSightPaintConditionTrace", LineOfSightPaintConditionSettings.TraceForComplex);
#endif

	lineOfSightTraceQueryParams.bReturnFaceIndex = false;
	lineOfSightTraceQueryParams.bReturnPhysicalMaterial = false;
	lineOfSightTraceQueryParams.AddIgnoredActors(LineOfSightPaintConditionSettings.LineOfSightTraceActorsToIgnore);
	lineOfSightTraceQueryParams.AddIgnoredComponents(LineOfSightPaintConditionSettings.LineOfSightTraceMeshComponentsToIgnore);
	LineOfSightPaintConditionSettings.LineOfSightTraceParams = lineOfSightTraceQueryParams;

	return LineOfSightPaintConditionSettings;
}


//-------------------------------------------------------

// Get Static Mesh LOD Info

bool FVertexPaintCalculateColorsTask::GetStaticMeshLODInfo(UStaticMeshComponent* StaticMeshComponent, bool LoopThroughAllVerticesOnLOD, int Lod, TArray<int32>& SectionsToLoopThrough, int& TotalAmountOfSections, FPositionVertexBuffer*& PositionVertexBuffer, FStaticMeshVertexBuffer*& MeshVertexBuffer, int& TotalAmountOfVerticesAtLOD, FRVPDPVertexDataInfo& MeshVertexData, TArray<FColor>& ColorsAtLOD) {

	TotalAmountOfSections = 0;
	SectionsToLoopThrough.Empty();
	PositionVertexBuffer = nullptr;
	MeshVertexBuffer = nullptr;
	TotalAmountOfVerticesAtLOD = 0;

	if (Lod < 0) return false;
	if (!IsValid(StaticMeshComponent)) return false;

	UStaticMesh* staticMesh = StaticMeshComponent->GetStaticMesh();
	if (!staticMesh) return false;

	FStaticMeshRenderData* staticMeshRenderData = staticMesh->GetRenderData();
	if (!staticMeshRenderData) return false;


	TotalAmountOfSections = staticMeshRenderData->LODResources[Lod].Sections.Num();
	int sectionIndexFromFaceIndex = -1;
	
	// If Static mesh, and only a Closest Vertex Data Task and not Paint at Location Combo, then if not going to loop through all vertices we can make sure to only loop through the section of the face index if it was provided. 
	if (!LoopThroughAllVerticesOnLOD && CalculateColorsInfoGlobal.IsGetClosestVertexDataTask() && CalculateColorsInfoGlobal.GetClosestVertexDataSettings.ClosestVertexDataOptimizationSettings.OptionalStaticMeshFaceIndex >= 0 && !CalculateColorsInfoGlobal.GetClosestVertexDataSettings.GetAverageColorInAreaSettings.GetAverageColor && CalculateColorsInfoGlobal.VertexPaintStaticMeshComponent->GetMaterialFromCollisionFaceIndex(CalculateColorsInfoGlobal.GetClosestVertexDataSettings.ClosestVertexDataOptimizationSettings.OptionalStaticMeshFaceIndex, sectionIndexFromFaceIndex)) {

		SectionsToLoopThrough.Add(sectionIndexFromFaceIndex);
	}

	else {

		for (int32 i = 0; i < TotalAmountOfSections; i++) {

			SectionsToLoopThrough.Add(i);

			// Can get the Min and Max Vertex Index, as well as the Material of this Section
			// UE_LOG(LogTemp, Warning, TEXT("LOD: %i  -  MinVertexIndex: %i  -  MaxVertexIndex: %i   -  Material Index: %i  "), Lod, CalculateColorsInfoGlobal.VertexPaintStaticMeshComponent->GetStaticMesh()->GetRenderData()->LODResources[Lod].Sections[i].MinVertexIndex, CalculateColorsInfoGlobal.VertexPaintStaticMeshComponent->GetStaticMesh()->GetRenderData()->LODResources[Lod].Sections[i].MaxVertexIndex, CalculateColorsInfoGlobal.VertexPaintStaticMeshComponent->GetStaticMesh()->GetRenderData()->LODResources[Lod].Sections[i].MaterialIndex);
		}
	}


	PositionVertexBuffer = &staticMeshRenderData->LODResources[Lod].VertexBuffers.PositionVertexBuffer;
	MeshVertexBuffer = &staticMeshRenderData->LODResources[Lod].VertexBuffers.StaticMeshVertexBuffer;
	TotalAmountOfVerticesAtLOD = PositionVertexBuffer->GetNumVertices();

	ColorsAtLOD.SetNum(TotalAmountOfVerticesAtLOD);


	// If this has been pre-filled, perhaps if we ran Paint Color Snippet before we started this then we use what's been filled in MeshVertexColorsPerLODArray
	if (MeshVertexData.MeshDataPerLOD[Lod].MeshVertexColorsPerLODArray.Num() == 0) {

		ColorsAtLOD = UVertexPaintFunctionLibrary::GetStaticMeshVertexColorsAtLOD(StaticMeshComponent, Lod);

		MeshVertexData.MeshDataPerLOD[Lod].MeshVertexColorsPerLODArray = ColorsAtLOD;
	}

	else {

		ColorsAtLOD = MeshVertexData.MeshDataPerLOD[Lod].MeshVertexColorsPerLODArray;
	}

	return true;
}


//-------------------------------------------------------

// Get Skeletal Mesh LOD Info

bool FVertexPaintCalculateColorsTask::GetSkeletalMeshLODInfo(bool LoopThroughAllVerticesOnLOD, FSkeletalMeshRenderData* SkeletalMeshRenderData, FSkeletalMeshLODRenderData * &SkeletalMeshLODRenderData, int Lod, FSkinWeightVertexBuffer * &SkinWeightBuffer, TArray<int32>&SectionsToLoopThrough, int& TotalAmountOfSections, int& TotalAmountOfVerticesAtLOD, FRVPDPVertexDataInfo& MeshVertexData, TArray<FColor>&ColorsAtLOD, int& VertexIndexToStartAt, TArray<TArray<FColor>>&BoneIndexColors) {

	SectionsToLoopThrough.Empty();
	TotalAmountOfSections = 0;
	TotalAmountOfVerticesAtLOD = 0;
	ColorsAtLOD.Empty();
	VertexIndexToStartAt = 0;
	BoneIndexColors.Empty();

	USkeletalMeshComponent* skelMeshComponent = CalculateColorsInfoGlobal.GetVertexPaintSkelComponent();
	USkeletalMesh* skelMesh = CalculateColorsInfoGlobal.GetVertexPaintSkeletalMesh();

	if (!IsValidSkeletalMesh(skelMesh, skelMeshComponent, SkeletalMeshRenderData, Lod)) return false;

	SkeletalMeshLODRenderData = &SkeletalMeshRenderData->LODRenderData[Lod];
	if (!SkeletalMeshLODRenderData) return false;

	SkinWeightBuffer = skelMeshComponent->GetSkinWeightBuffer(Lod);
	if (!SkinWeightBuffer) return false;


	TotalAmountOfVerticesAtLOD = SkeletalMeshLODRenderData->GetNumVertices();
	TotalAmountOfSections = SkeletalMeshLODRenderData->RenderSections.Num();


	// If this has been pre-filled, perhaps if we ran Paint Color Snippet before we started this then we use what's been filled in MeshVertexColorsPerLODArray
	if (MeshVertexData.MeshDataPerLOD[Lod].MeshVertexColorsPerLODArray.Num() == 0) {

		ColorsAtLOD = UVertexPaintFunctionLibrary::GetSkeletalMeshVertexColorsAtLOD(skelMeshComponent, Lod);

		MeshVertexData.MeshDataPerLOD[Lod].MeshVertexColorsPerLODArray = ColorsAtLOD;
	}

	else {

		ColorsAtLOD = MeshVertexData.MeshDataPerLOD[Lod].MeshVertexColorsPerLODArray;
	}

	
	if (CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeColorsOfEachBone) {

		BoneIndexColors.SetNum(SkeletalMeshLODRenderData->ActiveBoneIndices.Num());
	}


	for (int j = 0; j < SkeletalMeshLODRenderData->RenderSections.Num(); j++) {


		if (LoopThroughAllVerticesOnLOD || CalculateColorsInfoGlobal.SpecificSkeletalMeshBonesToLoopThrough.Num() == 0) {

			SectionsToLoopThrough.Add(j);
		}

		else if (CalculateColorsInfoGlobal.SpecificSkeletalMeshBonesToLoopThrough.Num() > 0) {

			bool gotSectionRelatedToBone = false;

			// Loops through all bones on section
			for (FBoneIndexType bone : SkeletalMeshLODRenderData->RenderSections[j].BoneMap) {

				if (CalculateColorsInfoGlobal.SpecificSkeletalMeshBonesToLoopThrough.Contains(CalculateColorsInfoGlobal.SkeletalMeshBonesNames[bone])) {

					// Add all sections related to the bone so we loop through all of them to get the closest vertex and material etc. 
					if (!SectionsToLoopThrough.Contains(j))
						SectionsToLoopThrough.Add(j);

					// For the first section find we set vertex index to start at. 
					if (!gotSectionRelatedToBone)
						VertexIndexToStartAt = SkeletalMeshLODRenderData->RenderSections[j].BaseVertexIndex;

					gotSectionRelatedToBone = true;
				}
			}
		}
	}

	return true;
}


#if ENGINE_MAJOR_VERSION == 5

//-------------------------------------------------------

// Get Dynamic Mesh LOD Info

bool FVertexPaintCalculateColorsTask::GetDynamicMeshLODInfo(UDynamicMesh* DynamicMesh, UE::Geometry::FDynamicMesh3 * &DynamicMesh3, TArray<int32>&SectionsToLoopThrough, int& TotalAmountOfVerticesAtLOD, TArray<FColor>&ColorsAtLOD, TArray<FColor>&DefaultColorsFromLOD, TArray<FColor>& DynamicMeshCompVertexColors) {

	SectionsToLoopThrough.Empty();
	TotalAmountOfVerticesAtLOD = 0;
	ColorsAtLOD.Empty();
	DefaultColorsFromLOD.Empty();
	DynamicMeshCompVertexColors.Empty();

	if (!IsValid(DynamicMesh)) return false;

	DynamicMesh3 = &DynamicMesh->GetMeshRef();
	if (!DynamicMesh3) return false;


	SectionsToLoopThrough.Add(0);
	TotalAmountOfVerticesAtLOD = DynamicMesh3->MaxVertexID();

	ColorsAtLOD.SetNum(TotalAmountOfVerticesAtLOD); // fills this array while we're looping through the verts as we can't just get them all right away here like static and skel meshes without having to loop through the verts here as well
	DefaultColorsFromLOD.SetNum(TotalAmountOfVerticesAtLOD);
	DynamicMeshCompVertexColors.SetNum(TotalAmountOfVerticesAtLOD);

	return true;
}


//-------------------------------------------------------

// Get Geometry Collection Mesh LOD Info

bool FVertexPaintCalculateColorsTask::GetGeometryCollectionMeshLODInfo(FGeometryCollection* GeometryCollectionData, TArray<int32>& SectionsToLoopThrough, int& TotalAmountOfVerticesAtLOD, TArray<FColor>& ColorsAtLOD, TArray<FColor>& DefaultColorsFromLOD, TArray<FLinearColor>& GeometryCollectionVertexColorsFromLOD) {

	SectionsToLoopThrough.Empty();
	TotalAmountOfVerticesAtLOD = 0;
	ColorsAtLOD.Empty();
	DefaultColorsFromLOD.Empty();
	GeometryCollectionVertexColorsFromLOD.Empty();

	if (!GeometryCollectionData) return false;


	// There seems to be something bugged with the sections when it comes to geometry collection components. For instance we could get 10 000 color num, and the first sections .MaxIndex reached that number, but also the second section, but it said the second section first index was like 35 000, way beyond the limit of the total amount of vertices. Tested it with several meshes so instead of looping through all of the sections we just loop through the first which seem to be the most accurate. 
	SectionsToLoopThrough.Add(0);
	// for (int i = 0; i < geometryCollectionData->Sections.Num(); i++)
	// 	SectionsToLoopThrough.Add(i);

	TotalAmountOfVerticesAtLOD = GeometryCollectionData->Color.Num();


	ColorsAtLOD.SetNum(TotalAmountOfVerticesAtLOD); // fills this array while we're looping through the verts as we can't just get them all right away here like static and skel meshes without having to loop through the verts here as well
	DefaultColorsFromLOD.SetNum(TotalAmountOfVerticesAtLOD);
	GeometryCollectionVertexColorsFromLOD.SetNum(TotalAmountOfVerticesAtLOD);

	return true;
}

#endif


//-------------------------------------------------------

// Get Paint Entire Mesh Random LOD Info

void FVertexPaintCalculateColorsTask::GetPaintEntireMeshRandomLODInfo(const FRVPDPPaintOnEntireMeshAtRandomVerticesSettings& PaintEntireMeshRandomVertices, bool PropogateLOD0ToAll, int Lod, int AmountOfVerticesAtLOD, FRandomStream& RandomSeedToUse, float& AmountOfVerticesToRandomize) {

	RandomSeedToUse = FRandomStream();
	AmountOfVerticesToRandomize = 0;


	// Can override the random seed, which is useful if for instance a server want clients to paint using the same seeds so they get the same random pattern. 
	if (PaintEntireMeshRandomVertices.OverrideRandomSeeds)
		RandomSeedToUse = PaintEntireMeshRandomVertices.SeedToOverrideWith;

	else
		RandomSeedToUse.GenerateNewSeed();


	if (PaintEntireMeshRandomVertices.PaintAtRandomVerticesSpreadOutOverTheEntireMesh && PaintEntireMeshRandomVertices.PaintAtRandomVerticesSpreadOutOverTheEntireMesh_PercentToPaint > 0) {


		if (PropogateLOD0ToAll) {

			// If gonna propogate LOD0 Colors to other LODs then we only randomize on LOD0
			if (Lod == 0)
				AmountOfVerticesToRandomize = UKismetMathLibrary::MapRangeClamped(PaintEntireMeshRandomVertices.PaintAtRandomVerticesSpreadOutOverTheEntireMesh_PercentToPaint, 0, 100, 0, AmountOfVerticesAtLOD);
		}

		else {

			AmountOfVerticesToRandomize = UKismetMathLibrary::MapRangeClamped(PaintEntireMeshRandomVertices.PaintAtRandomVerticesSpreadOutOverTheEntireMesh_PercentToPaint, 0, 100, 0, AmountOfVerticesAtLOD);
		}
	}

	else if (PaintEntireMeshRandomVertices.OnlyRandomizeWithinAreaOfEffectAtLocation && PaintEntireMeshRandomVertices.OnlyRandomizeWithinAreaOfEffectAtLocation_ProbabilityFactor > 0) {

		// If set to randomize within area then we don't increase the probability depending on how many verts left to randomize, as we're not using the Percent because that's for randomizing a percent over the entire mesh
		AmountOfVerticesToRandomize = AmountOfVerticesAtLOD;
	}
}


//-------------------------------------------------------

// Get Static Mesh Component Section Info

void FVertexPaintCalculateColorsTask::GetStaticMeshComponentSectionInfo(UStaticMeshComponent* StaticMeshComponent, int Lod, int Section, int& SectionMaxAmountOfVertices, int& CurrentLODVertex, UMaterialInterface*& MaterialSection) const {

	SectionMaxAmountOfVertices = 0;
	CurrentLODVertex = 0;
	MaterialSection = nullptr;

	if (!StaticMeshComponent) return;
	if (!StaticMeshComponent->GetStaticMesh()) return;
	if (!StaticMeshComponent->GetStaticMesh()->GetRenderData()) return;
	if (!StaticMeshComponent->GetStaticMesh()->GetRenderData()->LODResources.IsValidIndex(Lod)) return;
	if (!StaticMeshComponent->GetStaticMesh()->GetRenderData()->LODResources[Lod].Sections.IsValidIndex(Section)) return;


	const uint32 minVertexIndex = StaticMeshComponent->GetStaticMesh()->GetRenderData()->LODResources[Lod].Sections[Section].MinVertexIndex;
	const uint32 maxVertexIndex = StaticMeshComponent->GetStaticMesh()->GetRenderData()->LODResources[Lod].Sections[Section].MaxVertexIndex;

	if (Section > 0) {

		// Sets this Section max amount of verts, not the max amount of loops in total up to this Section. So if the Section goes between vertex 300 - 600, we will only loop through 300
		SectionMaxAmountOfVertices = maxVertexIndex - minVertexIndex;
	}

	else {

		// If the very first Section then we need to +1 since MaxVertexIndex isn't the total amount of vertices but just the max. For instance if you have a Mesh with 1801 vertices, i.e. 0 - 1800, then MaxVertexIndex would be 1800, so we can't just use that because then we would loop through 0-1799. 
		SectionMaxAmountOfVertices = maxVertexIndex + 1;
	}


	// Makes sure the Lod vertex is in sync with the Section we're on. Necessary in case we skip this Section if it's a paint job with apply using physics surface, but it failed and there's no reason to loop through verts. 
	CurrentLODVertex = minVertexIndex;

	// In case the previous Section had higher MaxVertIndex, if so it could become -
	SectionMaxAmountOfVertices = FMath::Abs(SectionMaxAmountOfVertices);

	MaterialSection = StaticMeshComponent->GetMaterial(StaticMeshComponent->GetStaticMesh()->GetRenderData()->LODResources[Lod].Sections[Section].MaterialIndex);
}


//-------------------------------------------------------

// Get Skeletal Mesh Component Section Info

#if ENGINE_MAJOR_VERSION == 4

bool FVertexPaintCalculateColorsTask::GetSkeletalMeshComponentSectionInfo(USkeletalMeshComponent* SkelComponent, bool AffectClothPhysics, int Lod, int Section, FSkeletalMeshRenderData* SkelMeshRenderData, FSkeletalMeshLODRenderData* SkelMeshLODRenderData, const TMap<int32, TArray<FVector>>& ClothVertexPositions, const TMap<int32, TArray<FVector>>& ClothVertexNormals, const TMap<UClothingAssetBase*, FRVPDPVertexChannelsChaosClothPhysicsSettings>& ClothsPhysicsSettings, const TArray<UClothingAssetBase*>& ClothingAssets, int& SectionMaxAmountOfVertices, int& CurrentLODVertex, UMaterialInterface*& MaterialSection, FSkelMeshRenderSection*& SkelMeshRenderSection, bool& ShouldAffectClothPhysics, TArray<FVector>& ClothSectionVertexPositions, TArray<FVector>& ClothSectionVertexNormals) {

#elif ENGINE_MAJOR_VERSION == 5

bool FVertexPaintCalculateColorsTask::GetSkeletalMeshComponentSectionInfo(USkeletalMeshComponent* SkelComponent, bool AffectClothPhysics, int Lod, int Section, FSkeletalMeshRenderData * SkelMeshRenderData, FSkeletalMeshLODRenderData * SkelMeshLODRenderData, const TMap<int32, TArray<FVector3f>>&ClothVertexPositions, const TMap<int32, TArray<FVector3f>>&ClothVertexNormals, const TMap<UClothingAssetBase*, FRVPDPVertexChannelsChaosClothPhysicsSettings>&ClothsPhysicsSettings, const TArray<UClothingAssetBase*>&ClothingAssets, int& SectionMaxAmountOfVertices, int& CurrentLODVertex, UMaterialInterface * &MaterialSection, FSkelMeshRenderSection * &SkelMeshRenderSection, bool& ShouldAffectClothPhysics, TArray<FVector3f>&ClothSectionVertexPositions, TArray<FVector3f>&ClothSectionVertexNormals) {

#endif

	SectionMaxAmountOfVertices = 0;
	CurrentLODVertex = 0;
	MaterialSection = nullptr;
	SkelMeshRenderSection = nullptr;
	ShouldAffectClothPhysics = false;
	ClothSectionVertexPositions.Empty();
	ClothSectionVertexNormals.Empty();


	// If invalid Section then something have gone so we end the task. There was a very very rare crash later with cloth this seems to fix
	if (!SkelMeshLODRenderData->RenderSections.IsValidIndex(Section)) {

		CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Calculate Colors Task - Failed because invalid Render Section: %i!"), Section), FColor::Red));

		return false;
	}

	
	SkelMeshRenderSection = &SkelMeshLODRenderData->RenderSections[Section];

	if (!SkelMeshRenderSection) {

		CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Calculate Colors Task - Failed because invalid Render Section: %i!"), Section), FColor::Red));

		return false;
	}


	if (IsValid(SkelComponent) && SkelComponent->GetMaterials().Num() > 0) {

		MaterialSection = SkelComponent->GetMaterial(SkelMeshRenderSection->MaterialIndex);
	}


	// Makes sure the Lod vertex is in sync with the Section we're on. Necessary in case we skip this Section if it's a paint job with apply using physics surface, but it failed and there's no reason to loop through verts
	CurrentLODVertex = SkelMeshRenderSection->BaseVertexIndex;
	SectionMaxAmountOfVertices = SkelMeshRenderSection->GetNumVertices();
	

	// for each Section we check if it's cloth and if so gets pos and normal of the cloth. Sets a local var here for use later on so we don't have to do .FindRef several times
	if (SkelMeshRenderSection->HasClothingData() && ClothingAssets.IsValidIndex(SkelMeshRenderSection->CorrespondClothAssetIndex) && Lod == SkelComponent->GetPredictedLODLevel()) {


		const UClothingAssetBase* currentClothingAssetBase = ClothingAssets[SkelMeshRenderSection->CorrespondClothAssetIndex];

		// Caches a bool so we don't have to run a .Contain check on something every vertex. 
		if (AffectClothPhysics) {

			if (ClothsPhysicsSettings.Contains(currentClothingAssetBase))
				ShouldAffectClothPhysics = true;
		}


		if (ClothVertexPositions.Contains(SkelMeshRenderSection->CorrespondClothAssetIndex))
			ClothSectionVertexPositions = ClothVertexPositions.FindRef(SkelMeshRenderSection->CorrespondClothAssetIndex);


		if (ClothVertexNormals.Contains(SkelMeshRenderSection->CorrespondClothAssetIndex))
			ClothSectionVertexNormals = ClothVertexNormals.FindRef(SkelMeshRenderSection->CorrespondClothAssetIndex);
	}

	return true;
}


#if ENGINE_MAJOR_VERSION == 5

//-------------------------------------------------------

// Get Dynamic Mesh Component Section Info

void FVertexPaintCalculateColorsTask::GetDynamicMeshComponentSectionInfo(UDynamicMeshComponent* DynamicMeshComponent, int LodAmountOfVertices, int Section, int& SectionMaxAmountOfVertices, int& CurrentLODVertex, UMaterialInterface*& MaterialSection) {

	// Dynamic Mesh Components seem to only be able to have 1 LOD and Material. Review this if there is a way to have multiple materials

	CurrentLODVertex = 0;
	SectionMaxAmountOfVertices = LodAmountOfVertices;
	MaterialSection = nullptr;

	if (!DynamicMeshComponent) return;

	if (DynamicMeshComponent->GetMaterials().IsValidIndex(Section))
		MaterialSection = DynamicMeshComponent->GetMaterials()[Section];
}


//-------------------------------------------------------

// Get Geometry Collection Mesh Component Section Info

void FVertexPaintCalculateColorsTask::GetGeometryCollectionMeshComponentSectionInfo(FGeometryCollection* GeometryCollectionData, UGeometryCollectionComponent* GeometryCollectionComponent, int Section, int& SectionMaxAmountOfVertices, int& CurrentLODVertex, UMaterialInterface*& MaterialSection) {

	SectionMaxAmountOfVertices = 0;
	CurrentLODVertex = 0;

	if (!GeometryCollectionData) return;
	if (!GeometryCollectionComponent) return;


	if (Section > 0)
		SectionMaxAmountOfVertices = GeometryCollectionData->Sections[Section].MaxVertexIndex - GeometryCollectionData->Sections[Section].MinVertexIndex;
	else
		SectionMaxAmountOfVertices = GeometryCollectionData->Sections[Section].MaxVertexIndex + 1;


	const int materialID = GeometryCollectionData->Sections[Section].MaterialID;

	CurrentLODVertex = GeometryCollectionData->Sections[Section].MinVertexIndex;
	SectionMaxAmountOfVertices = FMath::Abs(SectionMaxAmountOfVertices);
	MaterialSection = GeometryCollectionComponent->GetMaterial(materialID);

	// GeometryCollectionData->Sections[Section].HitProxy
	// GeometryCollectionData->Sections[Section].NumTriangles

	// Got strange result here if looping through all sections for geometry collection components, like the second sections .FirstIndex could be way beyond the max amount of vertices. 
}

#endif


//-------------------------------------------------------

// Get Material To Get Surfaces From

void FVertexPaintCalculateColorsTask::GetMaterialToGetSurfacesFrom(UVertexPaintMaterialDataAsset* MaterialDataAsset, const FRVPDPTaskFundamentalSettings& TaskFundementalSettings, UMaterialInterface* MaterialAtSection, UMaterialInterface*& MaterialToGetSurfacesFrom, TArray<TEnumAsByte<EPhysicalSurface>>& RegisteredPhysicsSurfacesAtMaterial) {

	MaterialToGetSurfacesFrom = nullptr;
	RegisteredPhysicsSurfacesAtMaterial.Empty();

	if (!MaterialAtSection) return;


	// If it's registered, then we get whether MaterialAtSection material, or it's parent (if it's an instance) is registered, so we can you stuff like find what physics surfaces is painted at what channel etc. using this. Otherwise we had an issue if an instance wasn't registered, but the parent was, then we expect to work off of what the parent has. 
	if (MaterialDataAsset)
		MaterialToGetSurfacesFrom = MaterialDataAsset->GetRegisteredMaterialFromMaterialInterface(MaterialAtSection);
	else
		MaterialToGetSurfacesFrom = MaterialAtSection;


	if (MaterialToGetSurfacesFrom && TaskFundementalSettings.TaskWorld.IsValid()) {

		RegisteredPhysicsSurfacesAtMaterial = UVertexPaintFunctionLibrary::GetPhysicsSurfacesRegisteredToMaterial(TaskFundementalSettings.TaskWorld.Get(), MaterialToGetSurfacesFrom);
	}
}


//-------------------------------------------------------

// Get Color Channels To Primarily Apply

void FVertexPaintCalculateColorsTask::GetShouldApplyColorsOnChannels(const FRVPDPApplyColorSetting& PaintOnMeshColorSettingsForSection, bool& ShouldApplyOnRedChannel, bool& ShouldApplyOnGreenChannel, bool& ShouldApplyOnBlueChannel, bool& ShouldApplyOnAlphaChannel) {

	ShouldApplyOnRedChannel = false;
	ShouldApplyOnGreenChannel = false;
	ShouldApplyOnBlueChannel = false;
	ShouldApplyOnAlphaChannel = false;


	if (PaintOnMeshColorSettingsForSection.ApplyVertexColorSettings.ApplyColorsOnRedChannel.LerpVertexColorToTarget.LerpToTarget && PaintOnMeshColorSettingsForSection.ApplyVertexColorSettings.ApplyColorsOnRedChannel.LerpVertexColorToTarget.LerpStrength > 0) {

		ShouldApplyOnRedChannel = true;
	}

	else if (PaintOnMeshColorSettingsForSection.ApplyVertexColorSettings.ApplyColorsOnRedChannel.ApplyWithSetting == EApplyVertexColorSetting::EAddVertexColor) {

		if (PaintOnMeshColorSettingsForSection.ApplyVertexColorSettings.ApplyColorsOnRedChannel.AmountToApply != 0)
			ShouldApplyOnRedChannel = true;
		else
			ShouldApplyOnRedChannel = false;
	}

	else if (PaintOnMeshColorSettingsForSection.ApplyVertexColorSettings.ApplyColorsOnRedChannel.ApplyWithSetting == EApplyVertexColorSetting::ESetVertexColor) {

		ShouldApplyOnRedChannel = true;
	}



	if (PaintOnMeshColorSettingsForSection.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.LerpVertexColorToTarget.LerpToTarget && PaintOnMeshColorSettingsForSection.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.LerpVertexColorToTarget.LerpStrength > 0) {

		ShouldApplyOnGreenChannel = true;
	}

	else if (PaintOnMeshColorSettingsForSection.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.ApplyWithSetting == EApplyVertexColorSetting::EAddVertexColor) {

		if (PaintOnMeshColorSettingsForSection.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.AmountToApply != 0)
			ShouldApplyOnGreenChannel = true;
		else
			ShouldApplyOnGreenChannel = false;
	}

	else if (PaintOnMeshColorSettingsForSection.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.ApplyWithSetting == EApplyVertexColorSetting::ESetVertexColor) {

		ShouldApplyOnGreenChannel = true;
	}



	if (PaintOnMeshColorSettingsForSection.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.LerpVertexColorToTarget.LerpToTarget && PaintOnMeshColorSettingsForSection.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.LerpVertexColorToTarget.LerpStrength > 0) {

		ShouldApplyOnBlueChannel = true;
	}

	else if (PaintOnMeshColorSettingsForSection.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.ApplyWithSetting == EApplyVertexColorSetting::EAddVertexColor) {

		if (PaintOnMeshColorSettingsForSection.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.AmountToApply != 0)
			ShouldApplyOnBlueChannel = true;
		else
			ShouldApplyOnBlueChannel = false;
	}

	else if (PaintOnMeshColorSettingsForSection.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.ApplyWithSetting == EApplyVertexColorSetting::ESetVertexColor) {

		ShouldApplyOnBlueChannel = true;
	}



	if (PaintOnMeshColorSettingsForSection.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.LerpVertexColorToTarget.LerpToTarget && PaintOnMeshColorSettingsForSection.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.LerpVertexColorToTarget.LerpStrength > 0) {

		ShouldApplyOnAlphaChannel = true;
	}

	else if (PaintOnMeshColorSettingsForSection.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.ApplyWithSetting == EApplyVertexColorSetting::EAddVertexColor) {

		if (PaintOnMeshColorSettingsForSection.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.AmountToApply != 0)
			ShouldApplyOnAlphaChannel = true;
		else
			ShouldApplyOnAlphaChannel = false;
	}

	else if (PaintOnMeshColorSettingsForSection.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.ApplyWithSetting == EApplyVertexColorSetting::ESetVertexColor) {

		ShouldApplyOnAlphaChannel = true;
	}
}


//-------------------------------------------------------

// Get Paint On Mesh Color Settings From Physics Surface

bool FVertexPaintCalculateColorsTask::GetPaintOnMeshColorSettingsFromPhysicsSurface(const FRVPDPTaskFundamentalSettings& TaskFundementalSettings, UMaterialInterface* MaterialToGetSurfacesFrom, const FRVPDPApplyColorSetting& PaintOnMeshColorSettingsForSection, TMap<int, TArray<FRVPDPPhysicsSurfaceToApplySettings>>& PhysicsSurfaceToApplySettingsPerVertexChannel) {

	PhysicsSurfaceToApplySettingsPerVertexChannel.Empty();

	if (!PaintOnMeshColorSettingsForSection.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.ApplyVertexColorUsingPhysicsSurface) return false;


	TArray<FRVPDPPhysicsSurfaceToApplySettings> physicsSurfaceToApplySettingsOnRedChannel;
	TArray<FRVPDPPhysicsSurfaceToApplySettings> physicsSurfaceToApplySettingsOnGreenChannel;
	TArray<FRVPDPPhysicsSurfaceToApplySettings> physicsSurfaceToApplySettingsOnBlueChannel;
	TArray<FRVPDPPhysicsSurfaceToApplySettings> physicsSurfaceToApplySettingsOnAlphaChannel;

	bool physicsSurfaceAtRed = false;
	bool physicsSurfaceAtGreen = false;
	bool physicsSurfaceAtBlue = false;
	bool physicsSurfaceAtAlpha = false;


	for (FRVPDPPhysicsSurfaceToApplySettings physicalSurfaceToApply : PaintOnMeshColorSettingsForSection.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.PhysicalSurfacesToApply) {

		if (TaskFundementalSettings.TaskWorld.IsValid()) {

			UVertexPaintFunctionLibrary::GetChannelsPhysicsSurfaceIsRegisteredTo(TaskFundementalSettings.TaskWorld.Get(), MaterialToGetSurfacesFrom, physicalSurfaceToApply.SurfaceToApply, physicsSurfaceAtRed, physicsSurfaceAtGreen, physicsSurfaceAtBlue, physicsSurfaceAtAlpha);
		}


		if (physicsSurfaceAtRed || physicsSurfaceAtGreen || physicsSurfaceAtBlue || physicsSurfaceAtAlpha) {


			SetWithinColorRangePaintConditionChannelsIfUsingItWithPhysicsSurface(physicalSurfaceToApply.PaintConditions, MaterialToGetSurfacesFrom);


			if (physicsSurfaceAtRed)
				physicsSurfaceToApplySettingsOnRedChannel.Add(physicalSurfaceToApply);
			

			if (physicsSurfaceAtGreen)
				physicsSurfaceToApplySettingsOnGreenChannel.Add(physicalSurfaceToApply);
			

			if (physicsSurfaceAtBlue)
				physicsSurfaceToApplySettingsOnBlueChannel.Add(physicalSurfaceToApply);
			

			if (physicsSurfaceAtAlpha)
				physicsSurfaceToApplySettingsOnAlphaChannel.Add(physicalSurfaceToApply);
		}

		else {

			FString physicsSurfaceAsString = *StaticEnum<EPhysicalSurface>()->GetDisplayNameTextByValue(physicalSurfaceToApply.SurfaceToApply.GetValue()).ToString();

			if (MaterialToGetSurfacesFrom) {

				CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Calculate Colors Task - Set to Apply Colors using Physics Surface but GetPaintOnMeshColorSettingsFromPhysicsSurface Failed to get Colors to Apply on Material: %s for Physics Surface: %s. Perhaps the Mesh we're trying to Paint doesn't have it, or it's Parent/Child Physics Surface Registered, or the Parent/Child hierarchy isnt registered. "), *MaterialToGetSurfacesFrom->GetName(), *physicsSurfaceAsString), FColor::Red));
			}

			else {

				CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Calculate Colors Task - Set to Apply Colors using Physics Surface but GetPaintOnMeshColorSettingsFromPhysicsSurface Failed to get Colors for Physics Surface: %s. Perhaps the Mesh we're trying to Paint doesn't have it, or it's Parent/Child Physics Surface Registered, or the Parent/Child hierarchy isnt registered. "), *physicsSurfaceAsString), FColor::Red));
			}
		}
	}


	if (physicsSurfaceToApplySettingsOnRedChannel.Num() > 0)
		PhysicsSurfaceToApplySettingsPerVertexChannel.Add(0, physicsSurfaceToApplySettingsOnRedChannel);

	if (physicsSurfaceToApplySettingsOnGreenChannel.Num() > 0)
		PhysicsSurfaceToApplySettingsPerVertexChannel.Add(1, physicsSurfaceToApplySettingsOnGreenChannel);

	if (physicsSurfaceToApplySettingsOnBlueChannel.Num() > 0)
		PhysicsSurfaceToApplySettingsPerVertexChannel.Add(2, physicsSurfaceToApplySettingsOnBlueChannel);

	if (physicsSurfaceToApplySettingsOnAlphaChannel.Num() > 0)
		PhysicsSurfaceToApplySettingsPerVertexChannel.Add(3, physicsSurfaceToApplySettingsOnAlphaChannel);


	if (PhysicsSurfaceToApplySettingsPerVertexChannel.Num() > 0)
		return true;


	return false;
}


//-------------------------------------------------------

// Get Which Channels Contains Physics Surface

void FVertexPaintCalculateColorsTask::GetChannelsThatContainsPhysicsSurface(const FRVPDPTaskFundamentalSettings& TaskFundementalSettings, const FRVPDPWithinAreaSettings& WithinAreaSettings, const TArray<TEnumAsByte<EPhysicalSurface>>& RegisteredPhysicsSurfacesAtMaterial, bool& ContainsPhysicsSurfaceOnRedChannel, bool& ContainsPhysicsSurfaceOnGreenChannel, bool& ContainsPhysicsSurfaceOnBlueChannel, bool& ContainsPhysicsSurfaceOnAlphaChannel) {


	ContainsPhysicsSurfaceOnRedChannel = false;
	ContainsPhysicsSurfaceOnGreenChannel = false;
	ContainsPhysicsSurfaceOnBlueChannel = false;
	ContainsPhysicsSurfaceOnAlphaChannel = false;

	if (TaskFundementalSettings.TaskWorld.IsValid()) {

		// If include color of each channel has requirement that the channel has to have specific physics surfaces
		GetIfPhysicsSurfacesIsRegisteredToVertexColorChannels(TaskFundementalSettings.TaskWorld.Get(), TaskFundementalSettings.CallbackSettings.IncludeAmountOfPaintedColorsOfEachChannel.IncludeVertexColorChannelResultOfEachChannel, TaskFundementalSettings.CallbackSettings.IncludeAmountOfPaintedColorsOfEachChannel.IncludePhysicsSurfaceResultOfEachChannel, TaskFundementalSettings.CallbackSettings.IncludeAmountOfPaintedColorsOfEachChannel.IncludeOnlyIfPhysicsSurfacesIsRegisteredToAnyVertexChannel, RegisteredPhysicsSurfacesAtMaterial, ContainsPhysicsSurfaceOnRedChannel, ContainsPhysicsSurfaceOnGreenChannel, ContainsPhysicsSurfaceOnBlueChannel, ContainsPhysicsSurfaceOnAlphaChannel);

		// If Paint or Detect Within Area
		if (WithinAreaSettings.ComponentsToCheckIfIsWithin.Num() > 0) {

			GetIfPhysicsSurfacesIsRegisteredToVertexColorChannels(TaskFundementalSettings.TaskWorld.Get(), WithinAreaSettings.IncludeAmountOfColorsOfEachChannelWithinArea.IncludeVertexColorChannelResultOfEachChannel, WithinAreaSettings.IncludeAmountOfColorsOfEachChannelWithinArea.IncludePhysicsSurfaceResultOfEachChannel, WithinAreaSettings.IncludeAmountOfColorsOfEachChannelWithinArea.IncludeOnlyIfPhysicsSurfacesIsRegisteredToAnyVertexChannel, RegisteredPhysicsSurfacesAtMaterial, ContainsPhysicsSurfaceOnRedChannel, ContainsPhysicsSurfaceOnGreenChannel, ContainsPhysicsSurfaceOnBlueChannel, ContainsPhysicsSurfaceOnAlphaChannel);
		}
	}
}


//-------------------------------------------------------

// Get Static Mesh Component Vertex Info

bool FVertexPaintCalculateColorsTask::GetStaticMeshComponentVertexInfo(FPositionVertexBuffer* PositionVertBuffer, const FStaticMeshVertexBuffer* StaticMeshVertexBuffer, int LodVertexIndex, FVector& VertexPositionInComponentSpace, FVector& VertexPositionInActorSpace, FVector& VertexNormal, bool& GotVertexNormal) {

	VertexPositionInComponentSpace = FVector(ForceInitToZero);
	VertexPositionInActorSpace = FVector(ForceInitToZero);
	VertexNormal = FVector(ForceInitToZero);
	GotVertexNormal = false;

	if (!PositionVertBuffer) return false;
	if (!StaticMeshVertexBuffer) return false;


#if ENGINE_MAJOR_VERSION == 4

	VertexPositionInComponentSpace = PositionVertBuffer->VertexPosition(LodVertexIndex);

#elif ENGINE_MAJOR_VERSION == 5

	VertexPositionInComponentSpace = static_cast<FVector>(PositionVertBuffer->VertexPosition(LodVertexIndex));

#endif


	VertexNormal = FVector(StaticMeshVertexBuffer->VertexTangentZ(LodVertexIndex).X, StaticMeshVertexBuffer->VertexTangentZ(LodVertexIndex).Y, StaticMeshVertexBuffer->VertexTangentZ(LodVertexIndex).Z);

	GotVertexNormal = true;


	if (CalculateColorsInfoGlobal.IsSplineMeshTask) {

		USplineMeshComponent* splineMeshComponent = CalculateColorsInfoGlobal.GetVertexPaintSplineMeshComponent();
		VertexPositionInComponentSpace = GetSplineMeshVertexPositionInMeshSpace(VertexPositionInComponentSpace, splineMeshComponent);
		VertexNormal = GetSplineMeshVertexNormalInMeshSpace(VertexNormal, splineMeshComponent);
	}

	else if (CalculateColorsInfoGlobal.IsInstancedMeshTask) {

		const FTransform instancedMeshTransform = CalculateColorsInfoGlobal.InstancedMeshTransform;

		// Takes the Rotation into account as well since otherwise it was only accurate if the instance was rotated as 0, 0, 0. Otherwise if painting one side of the instance the other side could start getting painted
		const FMatrix rotationMatrix = FRotationMatrix(instancedMeshTransform.GetRotation().Rotator());
		const FVector rotatedVertexPosition = rotationMatrix.TransformPosition(VertexPositionInComponentSpace);
		
		VertexPositionInComponentSpace = rotatedVertexPosition + instancedMeshTransform.GetLocation();
	}
	

	VertexPositionInActorSpace = UKismetMathLibrary::InverseTransformLocation(CalculateColorsInfoGlobal.VertexPaintActorTransform, UKismetMathLibrary::TransformLocation(CalculateColorsInfoGlobal.VertexPaintComponentTransform, VertexPositionInComponentSpace));

	return true;
}


//-------------------------------------------------------

// Get Skeletal Mesh Component Vertex Info

#if ENGINE_MAJOR_VERSION == 4

bool FVertexPaintCalculateColorsTask::GetSkeletalMeshComponentVertexInfo(const TArray<FMatrix>& SkeletalMeshRefToLocals, const TArray<FName>& SkeletalMeshBoneNames, bool DrawClothVertexDebugPoint, float DrawClothVertexDebugPointDuration, FSkeletalMeshRenderData* SkeletalMeshRenderData, FSkeletalMeshLODRenderData* SkeletalMeshLODRenderData, FSkelMeshRenderSection* SkeletalMeshRenderSection, const FSkinWeightVertexBuffer* SkeletalMeshSkinWeightBuffer, const TArray<FVector>& ClothSectionVertexPositions, const TArray<FVector>& ClothSectionVertexNormals, const TArray<FQuat>& ClothBoneQuaternionsInComponentSpace, const TArray<FVector>& ClothBoneLocationInComponentSpace, int Lod, int Section, int SectionVertexIndex, int LodVertexIndex, int PredictedLOD, FVector& VertexPositionInComponentSpace, FVector& VertexPositionInActorSpace, FVector& VertexNormal, bool& GotVertexNormal, bool& IsVertexOnCloth, uint32& BoneIndex, FName& BoneName, float& BoneWeight) {

#elif ENGINE_MAJOR_VERSION == 5

bool FVertexPaintCalculateColorsTask::GetSkeletalMeshComponentVertexInfo(const TArray<FMatrix44f>& SkeletalMeshRefToLocals, const TArray<FName>& SkeletalMeshBoneNames, bool DrawClothVertexDebugPoint, float DrawClothVertexDebugPointDuration, FSkeletalMeshRenderData * SkeletalMeshRenderData, FSkeletalMeshLODRenderData * SkeletalMeshLODRenderData, FSkelMeshRenderSection * SkeletalMeshRenderSection, const FSkinWeightVertexBuffer * SkeletalMeshSkinWeightBuffer, const TArray<FVector3f>& ClothSectionVertexPositions, const TArray<FVector3f>& ClothSectionVertexNormals, const TArray<FQuat>&ClothBoneQuaternionsInComponentSpace, const TArray<FVector>&ClothBoneLocationInComponentSpace, int Lod, int Section, int SectionVertexIndex, int LodVertexIndex, int PredictedLOD, FVector& VertexPositionInComponentSpace, FVector& VertexPositionInActorSpace, FVector & VertexNormal, bool& GotVertexNormal, bool& IsVertexOnCloth, uint32& BoneIndex, FName& BoneName, float& BoneWeight) {

#endif

	VertexPositionInComponentSpace = FVector(ForceInitToZero);
	VertexPositionInActorSpace = FVector(ForceInitToZero);
	VertexNormal = FVector(ForceInitToZero);
	GotVertexNormal = false;
	IsVertexOnCloth = false;
	BoneIndex = 0;
	BoneName = "";
	BoneWeight = 0;


	if (!SkeletalMeshLODRenderData) {

		CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Calculate Colors Task - Failed because Skel Mesh LOD Render Data is not valid")), FColor::Red));

		return false;
	}


	if (!SkeletalMeshLODRenderData->RenderSections.IsValidIndex(Section)) {

		CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Calculate Colors Task - Failed because we're trying to loop through an invalid Section of the skeletal mesh LOD render data. Possibly the mesh has changed since the task started or during it? ")), FColor::Red));

		return false;
	}

	if (LodVertexIndex >= static_cast<int>(SkeletalMeshLODRenderData->GetNumVertices())) {

		CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Calculate Colors Task - Task Failed because skeletal mesh current vertex we're looping through is more than the skeletal mesh render data number of vertices. Possibly the Mesh has gotteh changed during paint job")), FColor::Red));

		return false;
	}


	// Extremely rarely got crashes when run .VertexTangentZ below on StaticMeshVertexBuffer ln 217 when switching skeletal meshes and painting every frame. This seems to have fixed that issue. 
	if (!SkeletalMeshLODRenderData->StaticVertexBuffers.StaticMeshVertexBuffer.IsInitialized() || !SkeletalMeshLODRenderData->StaticVertexBuffers.StaticMeshVertexBuffer.TangentsVertexBuffer.IsInitialized() || !SkeletalMeshLODRenderData->StaticVertexBuffers.StaticMeshVertexBuffer.GetTangentData()) {

		CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Calculate Colors Task - Failed because skeletal mesh's static vertex buffers tangent vertex buffer isn't initialized!")), FColor::Red));

		return false;
	}


#if ENGINE_MAJOR_VERSION == 4

	VertexNormal = SkeletalMeshLODRenderData->StaticVertexBuffers.StaticMeshVertexBuffer.VertexTangentZ(LodVertexIndex);

#elif ENGINE_MAJOR_VERSION == 5

	VertexNormal = static_cast<FVector>(SkeletalMeshLODRenderData->StaticVertexBuffers.StaticMeshVertexBuffer.VertexTangentZ(LodVertexIndex));

#endif


	GotVertexNormal = true;


	// If cloth, then gets and adjust position from cloth simul data when on the viewable LOD
	if (SkeletalMeshRenderSection->HasClothingData() && Lod == PredictedLOD) {

		if (!SkeletalMeshLODRenderData->ClothVertexBuffer.IsInitialized()) {

			CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Calculate Colors Task - Task Failed skeletal mesh with cloth's cloth vertex buffer isn't initialized eventhough it's supposed to have cloth. Perhaps the mesh has changed since the task was created or during it? ")), FColor::Red));

			return false;
		}

		
		if (ClothSectionVertexPositions.IsValidIndex(SectionVertexIndex) && ClothSectionVertexNormals.IsValidIndex(SectionVertexIndex) && ClothBoneQuaternionsInComponentSpace.IsValidIndex(SkeletalMeshRenderSection->CorrespondClothAssetIndex)) {

			IsVertexOnCloth = true;
			VertexPositionInComponentSpace = static_cast<FVector>(ClothSectionVertexPositions[SectionVertexIndex]);
			VertexNormal = static_cast<FVector>(ClothSectionVertexNormals[SectionVertexIndex]);


			// UE5 with Chaos Cloth required a conversion of the cloth positions and rotation to get them aligned to the bone the cloth is attached to
#if ENGINE_MAJOR_VERSION == 5

			// Rotates the Vector according to the bone it's attached to
			VertexPositionInComponentSpace = UKismetMathLibrary::Quat_RotateVector(ClothBoneQuaternionsInComponentSpace[SkeletalMeshRenderSection->CorrespondClothAssetIndex], static_cast<FVector>(ClothSectionVertexPositions[SectionVertexIndex]));

			// Since the Vertex Positions we get from Cloth is component 0, 0, 0 we had to add the bone component local here to get it at the right Location, so we now have the correct rotation and Location for each cloth vertex 
			VertexPositionInComponentSpace += ClothBoneLocationInComponentSpace[SkeletalMeshRenderSection->CorrespondClothAssetIndex];

#endif


			if (DrawClothVertexDebugPoint && InGameThread) {

				DrawDebugPoint(CalculateColorsInfoGlobal.TaskFundamentalSettings.GetTaskWorld(), UKismetMathLibrary::TransformLocation(CalculateColorsInfoGlobal.VertexPaintActorTransform, UKismetMathLibrary::InverseTransformLocation(CalculateColorsInfoGlobal.VertexPaintActorTransform, UKismetMathLibrary::TransformLocation(CalculateColorsInfoGlobal.VertexPaintComponentTransform, VertexPositionInComponentSpace))), 5, FColor::Red, false, DrawClothVertexDebugPointDuration, 0);
			}
		}

		else {
			
			// If no clothing then gets the position in component space using our own GetTypedSkinnedVertexPosition, which is otherwise called on each vertex if running ComputeSkinnedPositions. The reason why we've made our own here is so we only have to loop through a mesh sections and vertices once, instead of atleast twice if running ComputeSkinnedPositions, since it also loops through sections and vertices, then if we where to use that here we would have to loop through it again. 
			VertexPositionInComponentSpace = Modified_GetTypedSkinnedVertexPosition(SkeletalMeshRenderSection, SkeletalMeshLODRenderData->StaticVertexBuffers.PositionVertexBuffer, SkeletalMeshSkinWeightBuffer, SectionVertexIndex, SkeletalMeshRefToLocals, BoneIndex, BoneWeight);
		}
	}

	else {

		VertexPositionInComponentSpace = Modified_GetTypedSkinnedVertexPosition(SkeletalMeshRenderSection, SkeletalMeshLODRenderData->StaticVertexBuffers.PositionVertexBuffer, SkeletalMeshSkinWeightBuffer, SectionVertexIndex, SkeletalMeshRefToLocals, BoneIndex, BoneWeight);
	}


	if (!SkeletalMeshBoneNames.IsValidIndex(BoneIndex)) {

		CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Calculate Colors Task - Task Failed because skeletal mesh's current bone index isn't valid to the cached array we got when creating the task. Possibly the skeletal mesh has changed since then or during the task. ")), FColor::Red));

		return false;
	}


	// When gotten the bone index from GetTypedSkinnedVertexPosition we can get the bone name
	BoneName = SkeletalMeshBoneNames[BoneIndex];

	VertexPositionInActorSpace = UKismetMathLibrary::InverseTransformLocation(CalculateColorsInfoGlobal.VertexPaintActorTransform, UKismetMathLibrary::TransformLocation(CalculateColorsInfoGlobal.VertexPaintComponentTransform, VertexPositionInComponentSpace));


	return true;
}


//-------------------------------------------------------

// Get Dynamic Mesh Component Vertex Info

#if ENGINE_MAJOR_VERSION == 5

bool FVertexPaintCalculateColorsTask::GetDynamicMeshComponentVertexInfo(const UE::Geometry::FDynamicMesh3* DynamicMesh3, int TotalAmountOfVerticesAtLOD, int LodVertexIndex, FColor& VertexColor, FVector& VertexPositionInComponentSpace, FVector& VertexPositionInActorSpace, FVector& VertexNormal, bool& GotVertexNormal, UE::Geometry::FVertexInfo& DynamicMeshVertexInfo, int& DynamicMeshMaxVertexID, int& DynamicMeshVerticesBufferMax, bool& SkipDynamicMeshVertex) {

	VertexColor = FColor();
	VertexPositionInComponentSpace = FVector(ForceInitToZero);
	VertexPositionInActorSpace = FVector(ForceInitToZero);
	VertexNormal = FVector(ForceInitToZero);
	GotVertexNormal = false;
	DynamicMeshMaxVertexID = 0;
	DynamicMeshVerticesBufferMax = 0;
	SkipDynamicMeshVertex = false;


	if (!DynamicMesh3) return false;

	DynamicMeshMaxVertexID = DynamicMesh3->MaxVertexID();
	DynamicMeshVerticesBufferMax = DynamicMesh3->GetVerticesBuffer().Num();


	// These could rarely diff where one was not 0 but the other was if running a bunch of paint jobs while it was deforming every frame
	if (DynamicMeshMaxVertexID <= 0 || DynamicMeshVerticesBufferMax <= 0) {

		CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Calculate Colors Task - Failed because Dynamic Mesh MaxVertexID and VerticesBuffer amount isn't in sync!")), FColor::Red));

		return false;
	}


	// Checks if vertex count has changed, so even if the index is valid, but the deformation has occured and changed the amount While we where looping through the verts we can abort the task. 
	if (LodVertexIndex >= DynamicMeshMaxVertexID || DynamicMeshMaxVertexID != TotalAmountOfVerticesAtLOD) {

		CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Calculate Colors Task - Failed because Dynamic Mesh MaxVertexID is higher than the current Lod vertex, or it's not the same amount as the total amount of vertices when task started. ")), FColor::Red));

		return false;
	}


	// Also checks Lod vertex is within the vertices buffer amoutn. This was necessary because eventhough we could pass the IsVertexCheck, we could get a crash at GetVertex below where it tried to access a Vertices property, eventhough it passed a VertexRefCounts valid check, so apparently they can diff. 
	if (LodVertexIndex >= DynamicMeshVerticesBufferMax || DynamicMeshVerticesBufferMax != TotalAmountOfVerticesAtLOD) {

		CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Calculate Colors Task - Failed because Dynamic Mesh vertices buffer max vertex count is higher than the current Lod vertex, or it's not the same amount as the total amount of vertices when task started. ")), FColor::Red));

		return false;
	}


	// When a Dynamic Mesh become deformed all vertices aren't valid or exist, so on these we skip to the next iteration.
	if (!DynamicMesh3->IsVertex(LodVertexIndex)) {

		LodVertexIndex++;
		SkipDynamicMeshVertex = true;

		return true;
	}


	DynamicMeshVertexInfo = DynamicMesh3->GetVertexInfo(LodVertexIndex);


	if (DynamicMeshVertexInfo.bHaveC) {

		// Since dynamic meshes store their colors as FVector3f, i.e. RGB only, we return alpha as 0 instead of 1 as it would be otherwise. 
		VertexColor = (FColor(DynamicMeshVertexInfo.Color.X, DynamicMeshVertexInfo.Color.Y, DynamicMeshVertexInfo.Color.Z, 0));
	}


	VertexPositionInComponentSpace = (FVector)DynamicMeshVertexInfo.Position;

	if (DynamicMeshVertexInfo.bHaveN) {

		VertexNormal = static_cast<FVector>(DynamicMeshVertexInfo.Normal);
		GotVertexNormal = true;
	}


	VertexPositionInActorSpace = UKismetMathLibrary::InverseTransformLocation(CalculateColorsInfoGlobal.VertexPaintActorTransform, UKismetMathLibrary::TransformLocation(CalculateColorsInfoGlobal.VertexPaintComponentTransform, VertexPositionInComponentSpace));

	return true;
}


//-------------------------------------------------------

// Get Geometry Collection Component Vertex Info

// VertexPaintGeometryCollectionComponent->GetRootCurrentTransform() didn't exist on 5.0 so added this here. We don't support painting and detecting prior to 5.3 
#if ENGINE_MINOR_VERSION >= 3

bool FVertexPaintCalculateColorsTask::GetGeometryCollectionComponentVertexInfo(FGeometryCollection* GeometryCollectionData, int LodVertexIndex, FColor& VertexColor, FVector& VertexPositionInComponentSpace, FVector& VertexPositionInActorSpace, FVector& VertexNormal, bool& GotVertexNormal) {

	VertexColor = FColor();
	VertexPositionInComponentSpace = FVector(ForceInitToZero);
	VertexPositionInActorSpace = FVector(ForceInitToZero);
	VertexNormal = FVector(ForceInitToZero);
	GotVertexNormal = false;


	if (!GeometryCollectionData) return false;


	VertexNormal = (FVector)GeometryCollectionData->Normal[LodVertexIndex];
	VertexPositionInComponentSpace = (FVector)GeometryCollectionData->Vertex[LodVertexIndex];

	GotVertexNormal = true;

	// Have had issues when using .ToFColor thousands of time in async where it can occasionally not convert it correctly, where a few out of a thousands vertices may go from fully colored to 0, 0, 0, 0. Not sure why, but avoiding .ToFColor and doing your own conversion fixed the issue. 
	VertexColor = UVertexPaintFunctionLibrary::ReliableFLinearToFColor(GeometryCollectionData->Color[LodVertexIndex]);


	// Geometry Collection Components GetComponentTransform doesn't change apparently, so you have to use GetRootCurrentTransform to get the correct one. 
	VertexPositionInActorSpace = UKismetMathLibrary::InverseTransformLocation(CalculateColorsInfoGlobal.VertexPaintActorTransform, UKismetMathLibrary::TransformLocation(CalculateColorsInfoGlobal.VertexPaintComponentTransform, VertexPositionInComponentSpace));


	return true;
}

#endif
#endif


//-------------------------------------------------------

// Get Paint Entire Mesh Vertex Info

void FVertexPaintCalculateColorsTask::GetPaintEntireMeshVertexInfo(const FRVPDPPaintOnEntireMeshAtRandomVerticesSettings& PaintEntireMeshRandomVerticesSettings, const FVector& VertexPositionInWorldSpace, const FVector& VertexPositionInComponentSpace, int TotalAmountOfVerticesAtLOD, int Lod, int LodVertexIndex, const TMap<FVector, FColor>& VertexDublettesPaintedAtLOD0, const FRandomStream& RandomStream, bool& ShouldPaintCurrentVertex, float& AmountOfVerticesLeftToRandomize) {

	ShouldPaintCurrentVertex = false;


	// If set to randomize over the entire mesh
	if (PaintEntireMeshRandomVerticesSettings.PaintAtRandomVerticesSpreadOutOverTheEntireMesh || PaintEntireMeshRandomVerticesSettings.OnlyRandomizeWithinAreaOfEffectAtLocation) {


		// This array only gets filled if Motified Engine Method. If this vertex is a doublette of another that is on the Exact same position, then we want it to also get painted, so all doublettes at a position have the same color. This is so when we run our calculation so LOD1, 2 vertices get the same color as their closest and best LOD0 vertex, it shouldn't matter which of the doublettes the calculation chooses to be the best, as they all have the same color. Otherwise we had an issue where other LODs wouldn't get LOD0 color if using modified engine method since the result of it can be that the closest LOD0 vertex color to use for a LOD1 vertex could be a doublette of the randomized one that was on the exact same Location
		if (Lod == 0 && VertexDublettesPaintedAtLOD0.Contains(VertexPositionInComponentSpace)) {

			ShouldPaintCurrentVertex = true;
		}

		// AmountOfVerticesLeftToRandomize will reset if Not to propogate to other LODs, i.e. do a complete randomization for each LOD
		else if (AmountOfVerticesLeftToRandomize > 0) {

			// For instance 5 left to randomize out of total 100 verts that's left, = 0.05, 5% chance. initial AmountOfVerticesLeftToRandomize Will be set depending on the PaintAtRandomVerticesSpreadOutOverTheEntireMesh_PercentToPaint
			float currentProbability = AmountOfVerticesLeftToRandomize / (TotalAmountOfVerticesAtLOD - LodVertexIndex);


			// If set to randomize within AoE at Location then checks distance,if too far away then probability drops to 0 as we should never randomizee to them. 
			if (PaintEntireMeshRandomVerticesSettings.OnlyRandomizeWithinAreaOfEffectAtLocation) {

				if ((PaintEntireMeshRandomVerticesSettings.OnlyRandomizeWithinAreaOfEffectAtLocation_Location - VertexPositionInWorldSpace).Size() > PaintEntireMeshRandomVerticesSettings.OnlyRandomizeWithinAreaOfEffectAtLocation_AreaOfEffect)
					currentProbability = 0;

				else
					currentProbability = PaintEntireMeshRandomVerticesSettings.OnlyRandomizeWithinAreaOfEffectAtLocation_ProbabilityFactor;
			}


			if (currentProbability > 0) {

#if ENGINE_MAJOR_VERSION == 5

#if ENGINE_MINOR_VERSION == 0
				if (UKismetMathLibrary::RandomBoolWithWeightFromStream(currentProbability, RandomStream)) {
#elif ENGINE_MINOR_VERSION == 1
				if (UKismetMathLibrary::RandomBoolWithWeightFromStream(currentProbability, RandomStream)) {
#else
				// From 5.2 it's changed with FRandomStream as first argument
				if (UKismetMathLibrary::RandomBoolWithWeightFromStream(RandomStream, currentProbability)) {
#endif

#else
				// If this vert was choosen, then set paintCurrentVertex to true so we actually paint on this and doesn't just run it
				if (UKismetMathLibrary::RandomBoolWithWeightFromStream(currentProbability, RandomStream)) {
#endif

					AmountOfVerticesLeftToRandomize--;
					ShouldPaintCurrentVertex = true;
				}

				// If not choosen to paint on this vertex, we still let the loop continue, so we can get complete cloth data to apply etc. 
				else {

					ShouldPaintCurrentVertex = false;
				}
			}

			else {

				ShouldPaintCurrentVertex = false;
			}
		}
	}

	else {

		ShouldPaintCurrentVertex = true;
	}
}


//-------------------------------------------------------

// Get Within Area Vertex Info At LOD Zero Before Applying Colors

bool FVertexPaintCalculateColorsTask::GetWithinAreaVertexInfoAtLODZeroBeforeApplyingColors(const FRVPDPWithinAreaSettings& WithinAreaSettings, int Lod, const FVector& VertexPositionInWorldSpace, bool& VertexIsWithinArea) {

	VertexIsWithinArea = false;

	// Only returns false if task should actually fail
	if (Lod != 0) return true;


	for (int i = 0; i < WithinAreaSettings.ComponentsToCheckIfIsWithin.Num(); i++) {

		// If Complex shape then requires that the component is still valid as we trace for it
		if (WithinAreaSettings.ComponentsToCheckIfIsWithin[i].PaintWithinAreaShape == EPaintWithinAreaShape::IsComplexShape) {

			// So we only resolve the weak ptr if its needed by the function, to make the task a bit faster for non complex 
			UPrimitiveComponent* componentToCheckIfIsWithin = WithinAreaSettings.ComponentsToCheckIfIsWithin[i].GetComponentToCheckIfIsWithin();
			if (!IsValid(componentToCheckIfIsWithin)) return false;

			if (IsVertexWithinArea(componentToCheckIfIsWithin, VertexPositionInWorldSpace, WithinAreaSettings.ComponentsToCheckIfIsWithin[i])) {

				VertexIsWithinArea = true;
				return true;
			}
		}

		else if (IsVertexWithinArea(nullptr, VertexPositionInWorldSpace, WithinAreaSettings.ComponentsToCheckIfIsWithin[i])) {

			VertexIsWithinArea = true;
			return true;
		}
	}

	return true;
}


//-------------------------------------------------------

// Get Within Area Vertex Info At LOD Zero Before Applying Colors

void FVertexPaintCalculateColorsTask::UpdateColorsOfEachChannelAbove0BeforeApplyingColors(const FLinearColor& VertexColorAsLinear, int Lod, FRVPDPAmountOfColorsOfEachChannelResults& ColorsOfEachChannelAbove0) {

	if (Lod != 0) return;


	ColorsOfEachChannelAbove0.RedChannelResult.AmountOfVerticesConsidered++;
	ColorsOfEachChannelAbove0.GreenChannelResult.AmountOfVerticesConsidered++;
	ColorsOfEachChannelAbove0.BlueChannelResult.AmountOfVerticesConsidered++;
	ColorsOfEachChannelAbove0.AlphaChannelResult.AmountOfVerticesConsidered++;

	// Always get the total amount for each channel if there is anything on it so we can after the task is done check if all is 0 or 1, if we're trying to add colors to an already fully painted mesh
	if (VertexColorAsLinear.R > 0) {

		ColorsOfEachChannelAbove0.RedChannelResult.AmountOfVerticesPaintedAtMinAmount++;
		ColorsOfEachChannelAbove0.RedChannelResult.AverageColorAmountAtMinAmount += VertexColorAsLinear.R;
	}

	if (VertexColorAsLinear.G > 0) {

		ColorsOfEachChannelAbove0.GreenChannelResult.AmountOfVerticesPaintedAtMinAmount++;
		ColorsOfEachChannelAbove0.GreenChannelResult.AverageColorAmountAtMinAmount += VertexColorAsLinear.G;
	}

	if (VertexColorAsLinear.B > 0) {

		ColorsOfEachChannelAbove0.BlueChannelResult.AmountOfVerticesPaintedAtMinAmount++;
		ColorsOfEachChannelAbove0.BlueChannelResult.AverageColorAmountAtMinAmount += VertexColorAsLinear.B;
	}

	if (VertexColorAsLinear.A > 0) {

		ColorsOfEachChannelAbove0.AlphaChannelResult.AmountOfVerticesPaintedAtMinAmount++;
		ColorsOfEachChannelAbove0.AlphaChannelResult.AverageColorAmountAtMinAmount += VertexColorAsLinear.A;
	}
}


//-------------------------------------------------------

// Get Within Area Vertex Info After Applying Colors

void FVertexPaintCalculateColorsTask::GetWithinAreaVertexInfoAfterApplyingColorsAtLODZero(const FRVPDPWithinAreaSettings& WithinAreaSettings, const TArray<TEnumAsByte<EPhysicalSurface>>& RegisteredPhysicsSurfacesAtMaterial, const FLinearColor& VertexColorAsLinearBeforeApplyingColor, const FLinearColor& VertexColorAsLinearAfterApplyingColor, int Lod, bool VertexWasWithinArea, bool ContainsPhysicsSurfaceOnRedChannel, bool ContainsPhysicsSurfaceOnGreenChannel, bool ContainsPhysicsSurfaceOnBlueChannel, bool ContainsPhysicsSurfaceOnAlphaChannel, FRVPDPAmountOfColorsOfEachChannelResults& AmountOfPaintedColorsOfEachChannelWithinArea_BeforeApplyingColors, TArray<FRVPDPAmountOfColorsOfEachChannelPhysicsResults>& PhysicsSurfaceResultForSection_BeforeApplyingColors, FRVPDPAmountOfColorsOfEachChannelResults& AmountOfPaintedColorsOfEachChannelWithinArea_AfterApplyingColors, TArray<FRVPDPAmountOfColorsOfEachChannelPhysicsResults>& PhysicsSurfaceResultForSection_AfterApplyingColors) {

	if (!VertexWasWithinArea) return;
	if (Lod != 0) return;


	if (GetColorsWithinArea) {

		if (WithinAreaSettings.IncludeAmountOfColorsOfEachChannelWithinArea.IncludeVertexColorChannelResultOfEachChannel) {

			GetColorsOfEachChannelForVertex(VertexColorAsLinearBeforeApplyingColor, WithinAreaSettings.IncludeAmountOfColorsOfEachChannelWithinArea, ContainsPhysicsSurfaceOnRedChannel, ContainsPhysicsSurfaceOnGreenChannel, ContainsPhysicsSurfaceOnBlueChannel, ContainsPhysicsSurfaceOnAlphaChannel, AmountOfPaintedColorsOfEachChannelWithinArea_BeforeApplyingColors.RedChannelResult, AmountOfPaintedColorsOfEachChannelWithinArea_BeforeApplyingColors.GreenChannelResult, AmountOfPaintedColorsOfEachChannelWithinArea_BeforeApplyingColors.BlueChannelResult, AmountOfPaintedColorsOfEachChannelWithinArea_BeforeApplyingColors.AlphaChannelResult);
		}

		if (WithinAreaSettings.IncludeAmountOfColorsOfEachChannelWithinArea.IncludePhysicsSurfaceResultOfEachChannel && RegisteredPhysicsSurfacesAtMaterial.Num() > 0) {

			GetColorsOfEachPhysicsSurfaceForVertex(RegisteredPhysicsSurfacesAtMaterial, VertexColorAsLinearBeforeApplyingColor, WithinAreaSettings.IncludeAmountOfColorsOfEachChannelWithinArea, ContainsPhysicsSurfaceOnRedChannel, ContainsPhysicsSurfaceOnGreenChannel, ContainsPhysicsSurfaceOnBlueChannel, ContainsPhysicsSurfaceOnAlphaChannel, PhysicsSurfaceResultForSection_BeforeApplyingColors);
		}
	}


	if (WithinAreaSettings.IncludeAmountOfColorsOfEachChannelWithinArea.IncludeVertexColorChannelResultOfEachChannel) {

		GetColorsOfEachChannelForVertex(VertexColorAsLinearAfterApplyingColor, WithinAreaSettings.IncludeAmountOfColorsOfEachChannelWithinArea, ContainsPhysicsSurfaceOnRedChannel, ContainsPhysicsSurfaceOnGreenChannel, ContainsPhysicsSurfaceOnBlueChannel, ContainsPhysicsSurfaceOnAlphaChannel, AmountOfPaintedColorsOfEachChannelWithinArea_AfterApplyingColors.RedChannelResult, AmountOfPaintedColorsOfEachChannelWithinArea_AfterApplyingColors.GreenChannelResult, AmountOfPaintedColorsOfEachChannelWithinArea_AfterApplyingColors.BlueChannelResult, AmountOfPaintedColorsOfEachChannelWithinArea_AfterApplyingColors.AlphaChannelResult);
	}

	if (WithinAreaSettings.IncludeAmountOfColorsOfEachChannelWithinArea.IncludePhysicsSurfaceResultOfEachChannel && RegisteredPhysicsSurfacesAtMaterial.Num() > 0) {

		GetColorsOfEachPhysicsSurfaceForVertex(RegisteredPhysicsSurfacesAtMaterial, VertexColorAsLinearAfterApplyingColor, WithinAreaSettings.IncludeAmountOfColorsOfEachChannelWithinArea, ContainsPhysicsSurfaceOnRedChannel, ContainsPhysicsSurfaceOnGreenChannel, ContainsPhysicsSurfaceOnBlueChannel, ContainsPhysicsSurfaceOnAlphaChannel, PhysicsSurfaceResultForSection_AfterApplyingColors);
	}
}


//-------------------------------------------------------

// Get Serialized Vertex Color

void FVertexPaintCalculateColorsTask::GetSerializedVertexColor(int AmountOfLODVertices, const FColor& VertexColor, int LodVertexIndex, FString& SerializedColorData) {

	// Has to be bookended with [] so it matches our serialized TArray format
	if (LodVertexIndex == 0)
		SerializedColorData += "[";

	// Needs , inbetween to match the regular serialized TArray format, but not on the first one
	if (LodVertexIndex > 0)
		SerializedColorData += ",";

	SerializedColorData += URVPDPRapidJsonFunctionLibrary::SerializeFColor_Wrapper(VertexColor);

	// If last vertex then wraps up with ]
	if (LodVertexIndex == AmountOfLODVertices - 1)
		SerializedColorData += "]";
}


//-------------------------------------------------------

// Get Cloth Section Average Color

void FVertexPaintCalculateColorsTask::GetClothSectionAverageColor(FSkelMeshRenderSection* SkelMeshRenderSection, bool ShouldAffectPhysicsOnClothSection, int SectionMaxAmountOfVertices, float ClothTotalRedVertexColorsOnSection, float ClothTotalGreenVertexColorsOnSection, float ClothTotalBlueVertexColorsOnSection, float ClothTotalAlphaVertexColorsOnSection, TMap<int16, FLinearColor>& ClothsSectionAvarageColor) {


	// Cloth Physics
	if (ShouldAffectPhysicsOnClothSection && SkelMeshRenderSection) {

		// Affecting Cloth Physics is exclusive to UE5 and ChaosCloth
#if ENGINE_MAJOR_VERSION == 5

		ClothTotalRedVertexColorsOnSection /= SectionMaxAmountOfVertices;
		ClothTotalGreenVertexColorsOnSection /= SectionMaxAmountOfVertices;
		ClothTotalBlueVertexColorsOnSection /= SectionMaxAmountOfVertices;
		ClothTotalAlphaVertexColorsOnSection /= SectionMaxAmountOfVertices;

		ClothTotalRedVertexColorsOnSection = UKismetMathLibrary::MapRangeClamped(ClothTotalRedVertexColorsOnSection, 0, 255, 0, 1);
		ClothTotalGreenVertexColorsOnSection = UKismetMathLibrary::MapRangeClamped(ClothTotalGreenVertexColorsOnSection, 0, 255, 0, 1);
		ClothTotalBlueVertexColorsOnSection = UKismetMathLibrary::MapRangeClamped(ClothTotalBlueVertexColorsOnSection, 0, 255, 0, 1);
		ClothTotalAlphaVertexColorsOnSection = UKismetMathLibrary::MapRangeClamped(ClothTotalAlphaVertexColorsOnSection, 0, 255, 0, 1);

		// Adds the Cloths that actually got painted and average color
		ClothsSectionAvarageColor.Add(SkelMeshRenderSection->CorrespondClothAssetIndex, FLinearColor(ClothTotalRedVertexColorsOnSection, ClothTotalGreenVertexColorsOnSection, ClothTotalBlueVertexColorsOnSection, ClothTotalAlphaVertexColorsOnSection));
#endif
	}
}


//-------------------------------------------------------

// Resolve Amount Of Colors Of Each Channel

void FVertexPaintCalculateColorsTask::ResolveAmountOfColorsOfEachPhysicsSurfaceForSection(int Section, const TArray<TEnumAsByte<EPhysicalSurface>>& RegisteredPhysicsSurfacesAtMaterial, TArray<FRVPDPAmountOfColorsOfEachChannelPhysicsResults>& PhysicsSurfaceResultForSection_BeforeApplyingColors, FRVPDPAmountOfColorsOfEachChannelResults& AmountOfPaintedColorsOfEachChannel_BeforeApplyingColors, TArray<FRVPDPAmountOfColorsOfEachChannelPhysicsResults>& PhysicsSurfaceResultForSection_AfterApplyingColors, FRVPDPAmountOfColorsOfEachChannelResults& AmountOfPaintedColorsOfEachChannel_AfterApplyingColors, TArray<FRVPDPAmountOfColorsOfEachChannelPhysicsResults>& PhysicsSurfaceResultForSection_WithinArea_BeforeApplyingColors, FRVPDPAmountOfColorsOfEachChannelResults& AmountOfPaintedColorsOfEachChannel_WithinArea_BeforeApplyingColors, TArray<FRVPDPAmountOfColorsOfEachChannelPhysicsResults>& PhysicsSurfaceResultForSection_WithinArea_AfterApplyingColors, FRVPDPAmountOfColorsOfEachChannelResults& AmountOfPaintedColorsOfEachChannel_WithinArea_AfterApplyingColors) {


	// When Finished looping through Section we go through the physics surface result for this Section, if we've gotten it earlier in a previous Section, or perhaps several vertex color channels has the same physics surface registered, then we add them up. 
	if (CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeAmountOfPaintedColorsOfEachChannel.IncludePhysicsSurfaceResultOfEachChannel || CalculateColorsInfoGlobal.WithinAreaSettings.IncludeAmountOfColorsOfEachChannelWithinArea.IncludePhysicsSurfaceResultOfEachChannel) {


		if (RegisteredPhysicsSurfacesAtMaterial.Num() > 0) {


			for (int i = 0; i < RegisteredPhysicsSurfacesAtMaterial.Num(); i++) {


				if (RegisteredPhysicsSurfacesAtMaterial[i] != EPhysicalSurface::SurfaceType_Default) {


					// Before and/or After Applying Color
					if (CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeAmountOfPaintedColorsOfEachChannel.IncludePhysicsSurfaceResultOfEachChannel) {


						// If Detection
						if (PhysicsSurfaceResultForSection_BeforeApplyingColors.IsValidIndex(i) && CalculateColorsInfoGlobal.IsDetectTask) {


							PhysicsSurfaceResultForSection_BeforeApplyingColors[i].PhysicsSurface = RegisteredPhysicsSurfacesAtMaterial[i];
							bool foundPhysicsSurface = false;


							// If can find that it has already added the physics surface result, then continues adding onto it. For instance if the mesh has several sections with the same physics surfaces registered on the materials used. 
							for (int j = 0; j < AmountOfPaintedColorsOfEachChannel_BeforeApplyingColors.PhysicsSurfacesResults.Num(); j++) {

								if (AmountOfPaintedColorsOfEachChannel_BeforeApplyingColors.PhysicsSurfacesResults[j].PhysicsSurface == RegisteredPhysicsSurfacesAtMaterial[i]) {


									foundPhysicsSurface = true;

									AmountOfPaintedColorsOfEachChannel_BeforeApplyingColors.PhysicsSurfacesResults[j].AmountOfVerticesConsidered += PhysicsSurfaceResultForSection_BeforeApplyingColors[i].AmountOfVerticesConsidered;

									AmountOfPaintedColorsOfEachChannel_BeforeApplyingColors.PhysicsSurfacesResults[j].AmountOfVerticesPaintedAtMinAmount += PhysicsSurfaceResultForSection_BeforeApplyingColors[i].AmountOfVerticesPaintedAtMinAmount;

									AmountOfPaintedColorsOfEachChannel_BeforeApplyingColors.PhysicsSurfacesResults[j].AverageColorAmountAtMinAmount += PhysicsSurfaceResultForSection_BeforeApplyingColors[i].AverageColorAmountAtMinAmount;

									AmountOfPaintedColorsOfEachChannel_BeforeApplyingColors.PhysicsSurfacesResults[j].PercentPaintedAtMinAmount += PhysicsSurfaceResultForSection_BeforeApplyingColors[i].PercentPaintedAtMinAmount;

									break;
								}
							}

							// If first time Adding the Physics Surface Result for the Section
							if (!foundPhysicsSurface) {

								AmountOfPaintedColorsOfEachChannel_BeforeApplyingColors.PhysicsSurfacesResults.Add(PhysicsSurfaceResultForSection_BeforeApplyingColors[i]);
							}
						}


						// After Applying Color
						if (PhysicsSurfaceResultForSection_AfterApplyingColors.IsValidIndex(i) && CalculateColorsInfoGlobal.IsPaintTask) {


							PhysicsSurfaceResultForSection_AfterApplyingColors[i].PhysicsSurface = RegisteredPhysicsSurfacesAtMaterial[i];
							bool foundPhysicsSurface = false;

							for (int j = 0; j < AmountOfPaintedColorsOfEachChannel_AfterApplyingColors.PhysicsSurfacesResults.Num(); j++) {


								if (AmountOfPaintedColorsOfEachChannel_AfterApplyingColors.PhysicsSurfacesResults[j].PhysicsSurface == RegisteredPhysicsSurfacesAtMaterial[i]) {


									foundPhysicsSurface = true;

									AmountOfPaintedColorsOfEachChannel_AfterApplyingColors.PhysicsSurfacesResults[j].AmountOfVerticesConsidered += PhysicsSurfaceResultForSection_AfterApplyingColors[i].AmountOfVerticesConsidered;

									AmountOfPaintedColorsOfEachChannel_AfterApplyingColors.PhysicsSurfacesResults[j].AmountOfVerticesPaintedAtMinAmount += PhysicsSurfaceResultForSection_AfterApplyingColors[i].AmountOfVerticesPaintedAtMinAmount;

									AmountOfPaintedColorsOfEachChannel_AfterApplyingColors.PhysicsSurfacesResults[j].AverageColorAmountAtMinAmount += PhysicsSurfaceResultForSection_AfterApplyingColors[i].AverageColorAmountAtMinAmount;

									AmountOfPaintedColorsOfEachChannel_AfterApplyingColors.PhysicsSurfacesResults[j].PercentPaintedAtMinAmount += PhysicsSurfaceResultForSection_AfterApplyingColors[i].PercentPaintedAtMinAmount;

									break;
								}
							}

							if (!foundPhysicsSurface) {

								AmountOfPaintedColorsOfEachChannel_AfterApplyingColors.PhysicsSurfacesResults.Add(PhysicsSurfaceResultForSection_AfterApplyingColors[i]);
							}
						}
					}


					// Within Area
					if (CalculateColorsInfoGlobal.WithinAreaSettings.IncludeAmountOfColorsOfEachChannelWithinArea.IncludePhysicsSurfaceResultOfEachChannel) {


						if (PhysicsSurfaceResultForSection_WithinArea_BeforeApplyingColors.IsValidIndex(i)) {


							PhysicsSurfaceResultForSection_WithinArea_BeforeApplyingColors[i].PhysicsSurface = RegisteredPhysicsSurfacesAtMaterial[i];
							bool foundPhysicsSurface = false;

							for (int j = 0; j < AmountOfPaintedColorsOfEachChannel_WithinArea_BeforeApplyingColors.PhysicsSurfacesResults.Num(); j++) {

								if (AmountOfPaintedColorsOfEachChannel_WithinArea_BeforeApplyingColors.PhysicsSurfacesResults[j].PhysicsSurface == RegisteredPhysicsSurfacesAtMaterial[i]) {


									foundPhysicsSurface = true;

									AmountOfPaintedColorsOfEachChannel_WithinArea_BeforeApplyingColors.PhysicsSurfacesResults[j].AmountOfVerticesConsidered += PhysicsSurfaceResultForSection_WithinArea_BeforeApplyingColors[i].AmountOfVerticesConsidered;

									AmountOfPaintedColorsOfEachChannel_WithinArea_BeforeApplyingColors.PhysicsSurfacesResults[j].AmountOfVerticesPaintedAtMinAmount += PhysicsSurfaceResultForSection_WithinArea_BeforeApplyingColors[i].AmountOfVerticesPaintedAtMinAmount;

									AmountOfPaintedColorsOfEachChannel_WithinArea_BeforeApplyingColors.PhysicsSurfacesResults[j].AverageColorAmountAtMinAmount += PhysicsSurfaceResultForSection_WithinArea_BeforeApplyingColors[i].AverageColorAmountAtMinAmount;

									AmountOfPaintedColorsOfEachChannel_WithinArea_BeforeApplyingColors.PhysicsSurfacesResults[j].PercentPaintedAtMinAmount += PhysicsSurfaceResultForSection_WithinArea_BeforeApplyingColors[i].PercentPaintedAtMinAmount;

									break;
								}
							}

							if (!foundPhysicsSurface) {

								AmountOfPaintedColorsOfEachChannel_WithinArea_BeforeApplyingColors.PhysicsSurfacesResults.Add(PhysicsSurfaceResultForSection_WithinArea_BeforeApplyingColors[i]);
							}
						}


						if (PhysicsSurfaceResultForSection_WithinArea_AfterApplyingColors.IsValidIndex(i)) {


							PhysicsSurfaceResultForSection_WithinArea_AfterApplyingColors[i].PhysicsSurface = RegisteredPhysicsSurfacesAtMaterial[i];
							bool foundPhysicsSurface = false;

							for (int j = 0; j < AmountOfPaintedColorsOfEachChannel_WithinArea_AfterApplyingColors.PhysicsSurfacesResults.Num(); j++) {

								if (AmountOfPaintedColorsOfEachChannel_WithinArea_AfterApplyingColors.PhysicsSurfacesResults[j].PhysicsSurface == RegisteredPhysicsSurfacesAtMaterial[i]) {


									foundPhysicsSurface = true;

									AmountOfPaintedColorsOfEachChannel_WithinArea_AfterApplyingColors.PhysicsSurfacesResults[j].AmountOfVerticesConsidered += PhysicsSurfaceResultForSection_WithinArea_AfterApplyingColors[i].AmountOfVerticesConsidered;

									AmountOfPaintedColorsOfEachChannel_WithinArea_AfterApplyingColors.PhysicsSurfacesResults[j].AmountOfVerticesPaintedAtMinAmount += PhysicsSurfaceResultForSection_WithinArea_AfterApplyingColors[i].AmountOfVerticesPaintedAtMinAmount;

									AmountOfPaintedColorsOfEachChannel_WithinArea_AfterApplyingColors.PhysicsSurfacesResults[j].AverageColorAmountAtMinAmount += PhysicsSurfaceResultForSection_WithinArea_AfterApplyingColors[i].AverageColorAmountAtMinAmount;

									AmountOfPaintedColorsOfEachChannel_WithinArea_AfterApplyingColors.PhysicsSurfacesResults[j].PercentPaintedAtMinAmount += PhysicsSurfaceResultForSection_WithinArea_AfterApplyingColors[i].PercentPaintedAtMinAmount;

									break;
								}
							}

							if (!foundPhysicsSurface) {

								AmountOfPaintedColorsOfEachChannel_WithinArea_AfterApplyingColors.PhysicsSurfacesResults.Add(PhysicsSurfaceResultForSection_WithinArea_AfterApplyingColors[i]);
							}
						}
					}
				}
			}
		}
	}
}


//-------------------------------------------------------

// Get Material At Section

UMaterialInterface* FVertexPaintCalculateColorsTask::GetMaterialAtSection(int Section, FSkeletalMeshLODRenderData* SkeletalMeshLODRenderData) {


	// Getting closest materials
	if (CalculateColorsInfoGlobal.IsStaticMeshTask) {

		if (const UStaticMesh* staticMesh = Cast<UStaticMesh>(CalculateColorsInfoGlobal.VertexPaintSourceMesh)) {

			if (auto staticMeshRenderData = staticMesh->GetRenderData()) {

				if (staticMeshRenderData->LODResources.IsValidIndex(0) && staticMeshRenderData->LODResources[0].Sections.IsValidIndex(Section)) {

					return CalculateColorsInfoGlobal.GetVertexPaintStaticMeshComponent()->GetMaterial(staticMeshRenderData->LODResources[0].Sections[Section].MaterialIndex);
				}
			}
		}
	}

	else if (CalculateColorsInfoGlobal.IsSkeletalMeshTask) {

		if (USkeletalMeshComponent* skeletalMeshComponent = CalculateColorsInfoGlobal.GetVertexPaintSkelComponent()) {

			if (SkeletalMeshLODRenderData && SkeletalMeshLODRenderData->RenderSections.IsValidIndex(Section)) {

				return skeletalMeshComponent->GetMaterial(SkeletalMeshLODRenderData->RenderSections[Section].MaterialIndex);
			}
		}
	}

#if ENGINE_MAJOR_VERSION == 5

	else if (CalculateColorsInfoGlobal.IsDynamicMeshTask) {

		if (UDynamicMeshComponent* dynamicMeshComponent = CalculateColorsInfoGlobal.GetVertexPaintDynamicMeshComponent()) {

			return dynamicMeshComponent->GetMaterial(Section);
		}
	}

	else if (CalculateColorsInfoGlobal.IsGeometryCollectionTask) {

		if (UGeometryCollectionComponent* geometryCollectionComponent = CalculateColorsInfoGlobal.GetVertexPaintGeometryCollectionComponent()) {

			return geometryCollectionComponent->GetMaterial(Section);
		}
	}
#endif

	return nullptr;
}


//-------------------------------------------------------

// Resolve Within Area Results

void FVertexPaintCalculateColorsTask::ResolveWithinAreaResults(int Lod, int AmountOfVerticesWithinArea, const TArray<FColor>& VertexColorsWithinArea_BeforeApplyingColors, const TArray<FColor>& VertexColorsWithinArea_AfterApplyingColors, const TArray<FVector>& VertexPositionsInComponentSpaceWithinArea, const TArray<FVector>& VertexNormalsWithinArea, const TArray<int>& VertexIndexesWithinArea, FRVPDPAmountOfColorsOfEachChannelResults& ColorsOfEachChannel_WithinArea_BeforeApplyingColors, FRVPDPAmountOfColorsOfEachChannelResults& ColorsOfEachChannel_WithinArea_AfterApplyingColors, FRVPDPWithinAreaResults& WithinAreaResultsBeforeApplyingColors, FRVPDPWithinAreaResults& WithinAreaResultsAfterApplyingColors) {


	const bool paintWithinArea = CalculateColorsInfoGlobal.IsPaintWithinAreaTask();
	

	// Sets MeshVertexData and colors of each channel etc. for Paint and GetColorsWithin area for the vertices just within the area
	if (CalculateColorsInfoGlobal.WithinAreaSettings.ComponentsToCheckIfIsWithin.Num() > 0) {


		if (GetColorsWithinArea) {

			WithinAreaResultsBeforeApplyingColors.MeshVertexDataWithinArea[Lod].Lod = Lod;
			WithinAreaResultsBeforeApplyingColors.MeshVertexDataWithinArea[Lod].AmountOfVerticesAtLOD = AmountOfVerticesWithinArea;

			if (CalculateColorsInfoGlobal.WithinAreaSettings.IncludeVertexColorsWithinArea) {

				WithinAreaResultsBeforeApplyingColors.MeshVertexDataWithinArea[Lod].MeshVertexColorsPerLODArray = VertexColorsWithinArea_BeforeApplyingColors;
			}

			if (CalculateColorsInfoGlobal.WithinAreaSettings.IncludeVertexPositionsWithinArea) {

				WithinAreaResultsBeforeApplyingColors.MeshVertexDataWithinArea[Lod].MeshVertexPositionsInComponentSpacePerLODArray = VertexPositionsInComponentSpaceWithinArea;
			}

			if (CalculateColorsInfoGlobal.WithinAreaSettings.IncludeVertexNormalsWithinArea) {

				WithinAreaResultsBeforeApplyingColors.MeshVertexDataWithinArea[Lod].MeshVertexNormalsPerLODArray = VertexNormalsWithinArea;
			}

			if (CalculateColorsInfoGlobal.WithinAreaSettings.IncludeVertexIndexesWithinArea) {

				WithinAreaResultsBeforeApplyingColors.MeshVertexDataWithinArea[Lod].MeshVertexIndexes = VertexIndexesWithinArea;
			}
		}

		if (paintWithinArea) {

			WithinAreaResultsAfterApplyingColors.MeshVertexDataWithinArea[Lod].Lod = Lod;
			WithinAreaResultsAfterApplyingColors.MeshVertexDataWithinArea[Lod].AmountOfVerticesAtLOD = AmountOfVerticesWithinArea;

			if (CalculateColorsInfoGlobal.WithinAreaSettings.IncludeVertexColorsWithinArea) {

				WithinAreaResultsAfterApplyingColors.MeshVertexDataWithinArea[Lod].MeshVertexColorsPerLODArray = VertexColorsWithinArea_AfterApplyingColors;
			}

			if (CalculateColorsInfoGlobal.WithinAreaSettings.IncludeVertexPositionsWithinArea) {

				WithinAreaResultsAfterApplyingColors.MeshVertexDataWithinArea[Lod].MeshVertexPositionsInComponentSpacePerLODArray = VertexPositionsInComponentSpaceWithinArea;
			}

			if (CalculateColorsInfoGlobal.WithinAreaSettings.IncludeVertexNormalsWithinArea) {

				WithinAreaResultsAfterApplyingColors.MeshVertexDataWithinArea[Lod].MeshVertexNormalsPerLODArray = VertexNormalsWithinArea;
			}

			if (CalculateColorsInfoGlobal.WithinAreaSettings.IncludeVertexIndexesWithinArea) {

				WithinAreaResultsAfterApplyingColors.MeshVertexDataWithinArea[Lod].MeshVertexIndexes = VertexIndexesWithinArea;
			}
		}


		// Setting Colors of Each channel result for vertices within area

		if (Lod == 0) {

			// Consolidates colors of each channel
			if (CalculateColorsInfoGlobal.WithinAreaSettings.IncludeAmountOfColorsOfEachChannelWithinArea.IncludeVertexColorChannelResultOfEachChannel) {

				if (GetColorsWithinArea) {

					ColorsOfEachChannel_WithinArea_BeforeApplyingColors = UVertexPaintFunctionLibrary::ConsolidateColorsOfEachChannel(ColorsOfEachChannel_WithinArea_BeforeApplyingColors, AmountOfVerticesWithinArea);
				}
				
				if (paintWithinArea) {

					ColorsOfEachChannel_WithinArea_AfterApplyingColors = UVertexPaintFunctionLibrary::ConsolidateColorsOfEachChannel(ColorsOfEachChannel_WithinArea_AfterApplyingColors, AmountOfVerticesWithinArea);
				}
			}

			// Consolidates colors of each physics surface
			if (CalculateColorsInfoGlobal.WithinAreaSettings.IncludeAmountOfColorsOfEachChannelWithinArea.IncludePhysicsSurfaceResultOfEachChannel) {

				if (GetColorsWithinArea) {

					ColorsOfEachChannel_WithinArea_BeforeApplyingColors = UVertexPaintFunctionLibrary::ConsolidatePhysicsSurfaceResult(ColorsOfEachChannel_WithinArea_BeforeApplyingColors, AmountOfVerticesWithinArea);
				}

				if (paintWithinArea) {

					ColorsOfEachChannel_WithinArea_AfterApplyingColors = UVertexPaintFunctionLibrary::ConsolidatePhysicsSurfaceResult(ColorsOfEachChannel_WithinArea_AfterApplyingColors, AmountOfVerticesWithinArea);
				}
			}

			if (GetColorsWithinArea) {

				WithinAreaResultsBeforeApplyingColors.AmountOfPaintedColorsOfEachChannelWithinArea = ColorsOfEachChannel_WithinArea_BeforeApplyingColors;
			}

			if (paintWithinArea) {

				WithinAreaResultsAfterApplyingColors.AmountOfPaintedColorsOfEachChannelWithinArea = ColorsOfEachChannel_WithinArea_AfterApplyingColors;
			}
		}
	}
}


//-------------------------------------------------------

// Resolve Skeletal Mesh Bone Colors

void FVertexPaintCalculateColorsTask::ResolveSkeletalMeshBoneColors(int Lod, const TArray<TArray<FColor>>& BoneVertexColors, TArray<FRVPDPBoneVertexColorsInfo>& SkeletalMeshColorsOfEachBone, bool& SuccessfullyGoBoneColors) {

	SuccessfullyGoBoneColors = false;
	SkeletalMeshColorsOfEachBone.Empty();

	if (!CalculateColorsInfoGlobal.IsSkeletalMeshTask) return;


	// Had to have an array of a color array that i add to instead of adding directly to the TMap since then i had to use .Contain and .FindRef which made it so each task took longer to finish
	if (Lod == 0 && CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.IncludeColorsOfEachBone) {

		for (int i = 0; i < BoneVertexColors.Num(); i++) {

			// If have added colors, i.e. a valid bone
			if (BoneVertexColors[i].Num() > 0) {

				FRVPDPBoneVertexColorsInfo boneColorsStruct;
				boneColorsStruct.BoneName = CalculateColorsInfoGlobal.SkeletalMeshBonesNames[i];
				boneColorsStruct.BoneVertexColors = BoneVertexColors[i];

				SkeletalMeshColorsOfEachBone.Add(boneColorsStruct);
			}
		}

		SuccessfullyGoBoneColors = true;
	}
}


//-------------------------------------------------------

// Resolve Chaos Cloth Physics

void FVertexPaintCalculateColorsTask::ResolveChaosClothPhysics(USkeletalMeshComponent* SkelComponent, bool AffectClothPhysics, bool VertexColorsChanged, const FSkeletalMeshLODRenderData* SkeletalMeshLODRenderData, int Lod, const TMap<UClothingAssetBase*, FRVPDPVertexChannelsChaosClothPhysicsSettings>& ChaosClothPhysicsSettings, const TMap<int16, FLinearColor>& ClothAverageColor, TMap<UClothingAssetBase*, FRVPDPChaosClothPhysicsSettings>& ClothPhysicsSettingsBasedOnAverageColor) {

	if (!SkelComponent) return;
	if (!SkeletalMeshLODRenderData) return;
	if (!AffectClothPhysics) return;


	// Affected Cloth Physics is exclusive to UE5 and ChaosCloth
#if ENGINE_MAJOR_VERSION == 5


	if (SkeletalMeshLODRenderData->HasClothData() && VertexColorsChanged && Lod == SkelComponent->GetPredictedLODLevel()) {


		TArray<UClothingAssetBase*> clothingAssetBase;
		ChaosClothPhysicsSettings.GetKeys(clothingAssetBase);

		TArray<FRVPDPVertexChannelsChaosClothPhysicsSettings> physicsSettingsAtColors;
		ChaosClothPhysicsSettings.GenerateValueArray(physicsSettingsAtColors);

		TArray<int16> paintedCloths;
		ClothAverageColor.GetKeys(paintedCloths);

		for (int j = 0; j < paintedCloths.Num(); j++) {

			if (!clothingAssetBase.IsValidIndex(j)) continue;
			if (!physicsSettingsAtColors.IsValidIndex(j)) continue;

			ClothPhysicsSettingsBasedOnAverageColor.Add(
				clothingAssetBase[j],
				UVertexPaintFunctionLibrary::GetChaosClothPhysicsSettingsBasedOnAverageVertexColor(clothingAssetBase[j], ClothAverageColor.FindRef(j), physicsSettingsAtColors[j])
			);
		}
	}
#endif

}


//-------------------------------------------------------

// Does Paint job Use Vertex Color Buffer

bool FVertexPaintCalculateColorsTask::DoesPaintJobUseVertexColorBuffer() const {

	
#if ENGINE_MAJOR_VERSION == 5
	
	return (CalculateColorsInfoGlobal.IsPaintTask && !CalculateColorsInfoGlobal.IsDynamicMeshTask && !CalculateColorsInfoGlobal.IsGeometryCollectionTask);

#elif ENGINE_MAJOR_VERSION == 4

	return (CalculateColorsInfoGlobal.IsPaintTask);

#else

	return false;

#endif
}


//-------------------------------------------------------

// Get Paint At Location Distance From Fall Off Base

float FVertexPaintCalculateColorsTask::GetPaintAtLocationDistanceFromFallOffBase(const FRVPDPPaintAtLocationFallOffSettings& PaintAtLocationFallOffSettings, float DistanceFromVertexToHitLocation) const {


	switch (PaintAtLocationFallOffSettings.PaintAtLocationFallOffType) {

	case EVertexPaintAtLocationFallOffType::InwardFallOff:

		return DistanceFromVertexToHitLocation;
		break;

	case EVertexPaintAtLocationFallOffType::OutwardFallOff:

		return DistanceFromVertexToHitLocation;
		break;

	case EVertexPaintAtLocationFallOffType::SphericalFallOff:

		if (DistanceFromVertexToHitLocation > PaintAtLocationFallOffSettings.DistanceToStartFallOffFrom) {

			return PaintAtLocationFallOffSettings.DistanceToStartFallOffFrom - DistanceFromVertexToHitLocation;
		}

		else if (DistanceFromVertexToHitLocation < PaintAtLocationFallOffSettings.DistanceToStartFallOffFrom) {

			return DistanceFromVertexToHitLocation - PaintAtLocationFallOffSettings.DistanceToStartFallOffFrom;
		}

		break;

	default:
		break;
	}

	return 0;
}


//-------------------------------------------------------

// Get Paint Within Area Distance From Falloff Base

float FVertexPaintCalculateColorsTask::GetPaintWithinAreaDistanceFromFalloffBase(const FRVPDPPaintWithinAreaFallOffSettings& PaintWithinAreaFallOffSettings, const FRVPDPComponentToCheckIfIsWithinAreaInfo& ComponentToCheckIfIsWithinInfo, const FVector& CurrentVertexInWorldSpace) const {


	if (PaintWithinAreaFallOffSettings.FallOffStrength <= 0) return 0;


	switch (ComponentToCheckIfIsWithinInfo.PaintWithinAreaShape) {

	case EPaintWithinAreaShape::IsSquareOrRectangleShape:


		switch (PaintWithinAreaFallOffSettings.PaintWithinAreaFallOffType) {

		case EVertexPaintWithinAreaFallOffType::SphericalFromCenter:

			return (ComponentToCheckIfIsWithinInfo.ComponentWorldTransform.GetLocation() - CurrentVertexInWorldSpace).Size();

		case EVertexPaintWithinAreaFallOffType::GradiantUpward: {

			const FVector vertexInLocalSpace = ComponentToCheckIfIsWithinInfo.ComponentWorldTransform.InverseTransformPosition(CurrentVertexInWorldSpace);
			const float falloffBaseZ = -ComponentToCheckIfIsWithinInfo.SquareOrRectangle_ZHalfSize;
			return FMath::Abs(vertexInLocalSpace.Z - falloffBaseZ);
		}

		case EVertexPaintWithinAreaFallOffType::GradiantDownward: {

			const FVector vertexInLocalSpace = ComponentToCheckIfIsWithinInfo.ComponentWorldTransform.InverseTransformPosition(CurrentVertexInWorldSpace);
			const float falloffBaseZ = ComponentToCheckIfIsWithinInfo.SquareOrRectangle_ZHalfSize;
			return FMath::Abs(vertexInLocalSpace.Z - falloffBaseZ);
		}

		case EVertexPaintWithinAreaFallOffType::InverseGradiant: {

			const FVector vertexInLocalSpace = ComponentToCheckIfIsWithinInfo.ComponentWorldTransform.InverseTransformPosition(CurrentVertexInWorldSpace);
			const float distanceFromCenterZ = FMath::Abs(vertexInLocalSpace.Z);
			return distanceFromCenterZ;
		}

		default:
			break;
		}
		break;



	case EPaintWithinAreaShape::IsSphereShape:


		switch (PaintWithinAreaFallOffSettings.PaintWithinAreaFallOffType) {

		case EVertexPaintWithinAreaFallOffType::SphericalFromCenter:

			return (ComponentToCheckIfIsWithinInfo.ComponentWorldTransform.GetLocation() - CurrentVertexInWorldSpace).Size();

		case EVertexPaintWithinAreaFallOffType::GradiantUpward: {

			const FVector vertexInLocalSpace = ComponentToCheckIfIsWithinInfo.ComponentWorldTransform.InverseTransformPosition(CurrentVertexInWorldSpace);
			const float falloffBaseZ = -ComponentToCheckIfIsWithinInfo.Sphere_Radius;
			return FMath::Abs(vertexInLocalSpace.Z - falloffBaseZ);
		}

		case EVertexPaintWithinAreaFallOffType::GradiantDownward: {

			const FVector vertexInLocalSpace = ComponentToCheckIfIsWithinInfo.ComponentWorldTransform.InverseTransformPosition(CurrentVertexInWorldSpace);
			const float falloffBaseZ = ComponentToCheckIfIsWithinInfo.Sphere_Radius;
			return FMath::Abs(vertexInLocalSpace.Z - falloffBaseZ);
		}

		case EVertexPaintWithinAreaFallOffType::InverseGradiant: {

			const FVector vertexInLocalSpace = ComponentToCheckIfIsWithinInfo.ComponentWorldTransform.InverseTransformPosition(CurrentVertexInWorldSpace);
			const float distanceFromCenterZ = FMath::Abs(vertexInLocalSpace.Z);
			return distanceFromCenterZ;
		}

		default:
			break;
		}
		break;



	case EPaintWithinAreaShape::IsComplexShape:


		switch (PaintWithinAreaFallOffSettings.PaintWithinAreaFallOffType) {

		case EVertexPaintWithinAreaFallOffType::SphericalFromCenter:

			return (ComponentToCheckIfIsWithinInfo.ActorBounds_Origin - CurrentVertexInWorldSpace).Size();

		case EVertexPaintWithinAreaFallOffType::GradiantUpward: {

			const FVector vertexInLocalSpace = ComponentToCheckIfIsWithinInfo.ComponentWorldTransform.InverseTransformPosition(CurrentVertexInWorldSpace);
			const float falloffBaseZ = -ComponentToCheckIfIsWithinInfo.ActorBounds_Extent.Z / 2;
			return FMath::Abs(vertexInLocalSpace.Z - falloffBaseZ);
		}

		case EVertexPaintWithinAreaFallOffType::GradiantDownward: {

			const FVector vertexInLocalSpace = ComponentToCheckIfIsWithinInfo.ComponentWorldTransform.InverseTransformPosition(CurrentVertexInWorldSpace);
			const float falloffBaseZ = ComponentToCheckIfIsWithinInfo.ActorBounds_Extent.Z / 2;
			return FMath::Abs(vertexInLocalSpace.Z - falloffBaseZ);
		}

		case EVertexPaintWithinAreaFallOffType::InverseGradiant: {

			const FVector vertexInLocalSpace = ComponentToCheckIfIsWithinInfo.ComponentWorldTransform.InverseTransformPosition(CurrentVertexInWorldSpace);
			const float distanceFromCenterZ = FMath::Abs(vertexInLocalSpace.Z);
			return distanceFromCenterZ;
		}

		default:
			break;
		}
		break;


	case EPaintWithinAreaShape::IsCapsuleShape:


		switch (PaintWithinAreaFallOffSettings.PaintWithinAreaFallOffType) {

		case EVertexPaintWithinAreaFallOffType::SphericalFromCenter:

			return (ComponentToCheckIfIsWithinInfo.ComponentWorldTransform.GetLocation() - CurrentVertexInWorldSpace).Size();

		case EVertexPaintWithinAreaFallOffType::GradiantUpward: {

			const FVector vertexInLocalSpace = ComponentToCheckIfIsWithinInfo.ComponentWorldTransform.InverseTransformPosition(CurrentVertexInWorldSpace);
			const float falloffBaseZ = -ComponentToCheckIfIsWithinInfo.Capsule_HalfHeight;
			return FMath::Abs(vertexInLocalSpace.Z - falloffBaseZ);
		}

		case EVertexPaintWithinAreaFallOffType::GradiantDownward: {

			const FVector vertexInLocalSpace = ComponentToCheckIfIsWithinInfo.ComponentWorldTransform.InverseTransformPosition(CurrentVertexInWorldSpace);
			const float falloffBaseZ = ComponentToCheckIfIsWithinInfo.Capsule_HalfHeight;
			return FMath::Abs(vertexInLocalSpace.Z - falloffBaseZ);
		}

		case EVertexPaintWithinAreaFallOffType::InverseGradiant: {

			const FVector vertexInLocalSpace = ComponentToCheckIfIsWithinInfo.ComponentWorldTransform.InverseTransformPosition(CurrentVertexInWorldSpace);
			const float distanceFromCenterZ = FMath::Abs(vertexInLocalSpace.Z);
			return distanceFromCenterZ;
		}

		default:
			break;
		}

		break;


	case EPaintWithinAreaShape::IsConeShape:


		switch (PaintWithinAreaFallOffSettings.PaintWithinAreaFallOffType) {

		case EVertexPaintWithinAreaFallOffType::SphericalFromCenter:

			// For Cone we have to use component bounds origin to calculate the falloff correctly since the component location is at the Top of the cone
			return (ComponentToCheckIfIsWithinInfo.ComponentBounds.Origin - CurrentVertexInWorldSpace).Size();

		case EVertexPaintWithinAreaFallOffType::GradiantUpward: {

			const FVector vertexInLocalSpace = ComponentToCheckIfIsWithinInfo.Cone_CenterBottomTransform.InverseTransformPosition(CurrentVertexInWorldSpace);
			const float distanceFromCenterZ = FMath::Abs(vertexInLocalSpace.Z);
			return distanceFromCenterZ;
		}

		case EVertexPaintWithinAreaFallOffType::GradiantDownward: {

			const FVector vertexInLocalSpace = ComponentToCheckIfIsWithinInfo.ComponentWorldTransform.InverseTransformPosition(CurrentVertexInWorldSpace);
			const float distanceFromCenterZ = FMath::Abs(vertexInLocalSpace.Z);
			return distanceFromCenterZ;
		}

		case EVertexPaintWithinAreaFallOffType::InverseGradiant: {

			const FVector vertexInLocalSpace = ComponentToCheckIfIsWithinInfo.Cone_CenterTransform.InverseTransformPosition(CurrentVertexInWorldSpace);
			const float distanceFromCenterZ = FMath::Abs(vertexInLocalSpace.Z);
			return distanceFromCenterZ;
		}

		default:
			break;
		}

		break;

	default:
		break;
	}

	return 0;
}


//-------------------------------------------------------

// Compare Colors Snippet To Vertex

void FVertexPaintCalculateColorsTask::CompareColorsSnippetToVertex(const FRVPDPCompareMeshVertexColorsToColorSnippetsSettings& CompareColorSnippetSettings, const TArray<TArray<FColor>>& CompareSnippetsColorsArray, int CurrentLODVertex, const FColor& VertexColor, TArray<int>& AmountOfVerticesConsidered, TArray<float>& MatchingPercent) {


	// Loops through the different snippets we want to compare to
	for (int i = 0; i < CompareSnippetsColorsArray.Num(); i++) {
		

		const FColor colorToCompareWith = CompareSnippetsColorsArray[i][CurrentLODVertex];

		if (CompareColorSnippetSettings.SkipEmptyVertices && colorToCompareWith == CompareColorSnippetSettings.TransformedEmptyVertexColor) {
			
			// UE_LOG(LogTemp, Warning, TEXT("Vertex Skipped: %i  -  Compare Snippet: %s"), currentLODVertex, *CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.CompareMeshVertexColorsToColorSnippets.CompareWithColorsSnippetTag[0].ToString());
		}

		else {

			if (AmountOfVerticesConsidered.IsValidIndex(i))
				AmountOfVerticesConsidered[i]++;

			if (DoesVertexColorsMatch(VertexColor, colorToCompareWith, CompareColorSnippetSettings.TransformedErrorTolerance, CompareColorSnippetSettings.CompareRedChannel, CompareColorSnippetSettings.CompareGreenChannel, CompareColorSnippetSettings.CompareBlueChannel, CompareColorSnippetSettings.CompareAlphaChannel)) {

				// UE_LOG(LogTemp, Warning, TEXT("Vertex Matched: %i  VertexColor: %s  -  Compare with Color: %s  -  Compare Snippet: %s"), currentLODVertex, *colorFromLOD[currentLODVertex].ToString(), *colorToCompareWith.ToString(), *CalculateColorsInfoGlobal.TaskFundamentalSettings.CallbackSettings.CompareMeshVertexColorsToColorSnippets.CompareWithColorsSnippetTag[0].ToString());

				if (MatchingPercent.IsValidIndex(i)) {

					MatchingPercent[i]++;
				}
			}
		}
	}
}


//-------------------------------------------------------

// Get Matching Color Snippets Results

FRVPDPCompareMeshVertexColorsToColorSnippetResult FVertexPaintCalculateColorsTask::GetMatchingColorSnippetsResults(const FRVPDPCompareMeshVertexColorsToColorSnippetsSettings& CompareColorSnippetSettings, const TArray<TArray<FColor>>& CompareSnippetsColorsArray, const TArray<int>& AmountOfVerticesConsidered, TArray<float>& MatchingPercent) {


	FRVPDPCompareMeshVertexColorsToColorSnippetResult compareColorSnippetResult;


	for (int i = 0; i < CompareSnippetsColorsArray.Num(); i++) {


		if (CompareColorSnippetSettings.CompareWithColorsSnippetDataAssetInfo.IsValidIndex(i)) {

			if (MatchingPercent.IsValidIndex(i) && AmountOfVerticesConsidered.IsValidIndex(i)) {


				const FGameplayTag colorSnippetAsTag = FGameplayTag::RequestGameplayTag(FName(*CompareColorSnippetSettings.CompareWithColorsSnippetDataAssetInfo[i].ColorSnippetID));


				compareColorSnippetResult.SuccessfullyComparedMeshVertexColorsToColorSnippet = true;
				bool skippedAllvertices = false;

				if (AmountOfVerticesConsidered[i] > 0) {

					MatchingPercent[i] /= AmountOfVerticesConsidered[i];
					MatchingPercent[i] *= 100; // So it goes from 0-100 instead of 0-1
				}

				else {

					skippedAllvertices = true;
				}


				CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Successfully Compared Mesh Vertex Colors to Color Snippet: %s  -  Matching Percent: %f  -  Amount of Vertices Considered: %i  -  Skipped All Vertices: %i"), *colorSnippetAsTag.ToString(), MatchingPercent[i], AmountOfVerticesConsidered[i], skippedAllvertices), FColor::Cyan));


				compareColorSnippetResult.SkippedAllVerticesPerSnippet.Add(colorSnippetAsTag, skippedAllvertices);
				compareColorSnippetResult.MatchingPercentPerColorSnippet.Add(colorSnippetAsTag, MatchingPercent[i]);
			}
		}
	}

	return compareColorSnippetResult;
}


//-------------------------------------------------------

// Compare Color Array To Vertex

void FVertexPaintCalculateColorsTask::CompareColorArrayToVertex(const FRVPDPCompareMeshVertexColorsToColorArraySettings& CompareColorArraySettings, int CurrentLODVertex, const FColor& VertexColor, int& AmountOfVerticesConsidered, float& MatchingPercent) {


	const FColor colorToCompareWith = CompareColorArraySettings.ColorArrayToCompareWith[CurrentLODVertex];
	const FColor emptyVertexColor = CompareColorArraySettings.TransformedEmptyVertexColor;

	if (CompareColorArraySettings.SkipEmptyVertices && colorToCompareWith == emptyVertexColor) {

		// UE_LOG(LogTemp, Warning, TEXT("Skipping Vertex: %i"), currentLODVertex);
	}

	else {

		AmountOfVerticesConsidered++;

		if (DoesVertexColorsMatch(VertexColor, colorToCompareWith, CompareColorArraySettings.TransformedErrorTolerance, CompareColorArraySettings.CompareRedChannel, CompareColorArraySettings.CompareGreenChannel, CompareColorArraySettings.CompareBlueChannel, CompareColorArraySettings.CompareAlphaChannel)) {

			MatchingPercent++;
		}
	}
}


//-------------------------------------------------------

// Get Matching Color Array Results

FRVPDPCompareMeshVertexColorsToColorArrayResult FVertexPaintCalculateColorsTask::GetMatchingColorArrayResults(int AmountOfVerticesConsidered, float MatchingPercent) {


	FRVPDPCompareMeshVertexColorsToColorArrayResult compareColorArrayResult;


	compareColorArrayResult.SuccessfullyComparedMeshVertexColorsToColorArray = true;

	if (AmountOfVerticesConsidered > 0) {

		if (MatchingPercent > 0) {

			MatchingPercent /= AmountOfVerticesConsidered;
			MatchingPercent *= 100; // So it goes from 0-100 instead of 0-1
		}
	}

	else {

		compareColorArrayResult.SkippedAllVertices = true;
	}

	compareColorArrayResult.MatchingPercent = MatchingPercent;


	CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Successfully Compared Mesh Vertex Colors to Color Array. Matching Percent: %f  -  Amount of Vertices Considered: %i  -  Skipped All Vertices: %i"), MatchingPercent, AmountOfVerticesConsidered, compareColorArrayResult.SkippedAllVertices), FColor::Cyan));

	return compareColorArrayResult;
}


//-------------------------------------------------------

// Get Closest Vertex UVs

TArray<FVector2D> FVertexPaintCalculateColorsTask::GetClosestVertexUVs(UPrimitiveComponent* MeshComponent, int32 ClosestVertex) {

	if (ClosestVertex < 0) return TArray<FVector2D>();
	if (!UVertexPaintFunctionLibrary::IsWorldValid(CalculateColorsInfoGlobal.TaskFundamentalSettings.GetTaskWorld())) return TArray<FVector2D>();
	if (!IsMeshStillValid(CalculateColorsInfoGlobal)) return TArray<FVector2D>();


	TArray<FVector2D> closestVertexUVs;

	if (UStaticMeshComponent* staticMeshComponent = Cast<UStaticMeshComponent>(MeshComponent)) {

		if (!staticMeshComponent->GetStaticMesh()) return TArray<FVector2D>();
		if (!staticMeshComponent->GetStaticMesh()->GetRenderData()) return TArray<FVector2D>();
		if (!staticMeshComponent->GetStaticMesh()->GetRenderData()->LODResources.IsValidIndex(0)) return TArray<FVector2D>();


		if (staticMeshComponent->GetStaticMesh()->GetRenderData()->LODResources[0].VertexBuffers.StaticMeshVertexBuffer.IsInitialized() && staticMeshComponent->GetStaticMesh()->GetRenderData()->LODResources[0].VertexBuffers.StaticMeshVertexBuffer.TexCoordVertexBuffer.IsInitialized()) {

			// Had to use GetNumTexCoords since GetStaticMesh()->GetNumUVChannels(0); has a #if Editor only data
			for (uint32 i = 0; i < staticMeshComponent->GetStaticMesh()->GetRenderData()->LODResources[0].VertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords(); i++) {

#if ENGINE_MAJOR_VERSION == 4

				const FVector2D UV = staticMeshComponent->GetStaticMesh()->GetRenderData()->LODResources[0].VertexBuffers.StaticMeshVertexBuffer.GetVertexUV(ClosestVertex, i);
				closestVertexUVs.Add(UV);

#elif ENGINE_MAJOR_VERSION == 5

				const FVector2D UV = static_cast<FVector2D>(staticMeshComponent->GetStaticMesh()->GetRenderData()->LODResources[0].VertexBuffers.StaticMeshVertexBuffer.GetVertexUV(ClosestVertex, i));
				closestVertexUVs.Add(UV);
#endif
			}
		}
	}

	else if (USkeletalMeshComponent* skeletalMeshComponent = Cast<USkeletalMeshComponent>(MeshComponent)) {

		const UObject* sourceMesh = UVertexPaintFunctionLibrary::GetMeshComponentSourceMesh(MeshComponent);
		if (!sourceMesh) return TArray<FVector2D>();

		const USkeletalMesh* skeletalMesh = Cast<USkeletalMesh>(sourceMesh);
		if (!skeletalMesh) return TArray<FVector2D>();
		if (!skeletalMesh->IsValidLowLevel()) return TArray<FVector2D>();
		if (!IsValidSkeletalMesh(skeletalMesh, skeletalMeshComponent, skeletalMeshComponent->GetSkeletalMeshRenderData(), 0)) return TArray<FVector2D>();

		// Skeletal meshes could get crashes easier if this is running while the mesh gets destroyed so had to add a bunch of extra checks. 
		FSkeletalMeshLODRenderData& LODData = skeletalMeshComponent->GetSkeletalMeshRenderData()->LODRenderData[0];
		if (!LODData.StaticVertexBuffers.StaticMeshVertexBuffer.IsValid()) return TArray<FVector2D>();
		if (!LODData.StaticVertexBuffers.StaticMeshVertexBuffer.IsInitialized()) return TArray<FVector2D>();
		if (!LODData.StaticVertexBuffers.StaticMeshVertexBuffer.TexCoordVertexBuffer.IsInitialized()) return TArray<FVector2D>();
		if (LODData.StaticVertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords() == 0) return TArray<FVector2D>();
		if (ClosestVertex >= static_cast<int32>(LODData.StaticVertexBuffers.StaticMeshVertexBuffer.GetNumVertices())) return TArray<FVector2D>();


		for (uint32 i = 0; i < LODData.StaticVertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords(); i++) {


#if ENGINE_MAJOR_VERSION == 4

			const FVector2D UV = LODData.StaticVertexBuffers.StaticMeshVertexBuffer.GetVertexUV(ClosestVertex, i);
			closestVertexUVs.Add(UV);

#elif ENGINE_MAJOR_VERSION == 5

			const FVector2D UV = static_cast<FVector2D>(LODData.StaticVertexBuffers.StaticMeshVertexBuffer.GetVertexUV(ClosestVertex, i));
			closestVertexUVs.Add(UV);

#endif
		}
	}

#if ENGINE_MAJOR_VERSION == 5


	else if (UDynamicMeshComponent* dynamicMeshComponent = Cast<UDynamicMeshComponent>(MeshComponent)) {

		if (dynamicMeshComponent->GetDynamicMesh()) {


			UE::Geometry::FDynamicMesh3* dynamicMesh3 = nullptr;
			dynamicMesh3 = &dynamicMeshComponent->GetDynamicMesh()->GetMeshRef();

			if (dynamicMesh3) {

				UE::Geometry::FVertexInfo vertexInfo = UE::Geometry::FVertexInfo();


				if (dynamicMesh3->IsVertex(ClosestVertex)) {

					dynamicMesh3->GetVertex(ClosestVertex, vertexInfo, false, false, true);

					if (vertexInfo.bHaveUV) {

						closestVertexUVs.Add(static_cast<FVector2D>(vertexInfo.UV));
					}
				}
			}
		}
	}


	// GeometryCollectionData.Get()->GetUV didn't exist in 5.0, and we can't paint or detect on them until 5.3

#if ENGINE_MINOR_VERSION >= 3

	else if (UGeometryCollectionComponent* geometryCollectionComponent = Cast<UGeometryCollectionComponent>(MeshComponent)) {

		if (const UGeometryCollection* geometryCollection = geometryCollectionComponent->GetRestCollection()) {

			if (FGeometryCollection* geometryCollectionData = geometryCollection->GetGeometryCollection().Get()) {

				for (int i = 0; i < geometryCollectionData->NumUVLayers(); i++) {


					FVector2D geometryCollectionUV = static_cast<FVector2D>(geometryCollectionData->GetUV(ClosestVertex, i));

					closestVertexUVs.Add(geometryCollectionUV);
				}
			}
		}
	}
#endif
#endif


	return closestVertexUVs;
}


//-------------------------------------------------------

// Paint Job Print Debug Logs

void FVertexPaintCalculateColorsTask::PaintJobPrintDebugLogs(const FRVPDPApplyColorSetting& ApplyColorSettings, bool AnyVertexColorGotChanged, bool OverrideColorsMadeChange, const FRVPDPAmountOfColorsOfEachChannelResults& ColorsOfEachChannel_AboveZeroBeforeApplyingColor, const FRVPDPAmountOfColorsOfEachChannelResults& ColorsOfEachChannelResultsAtMinAmount_BeforeApplyingColor, const FRVPDPAmountOfColorsOfEachChannelResults& ColorsOfEachChannelResultsAtMinAmount_AfterApplyingColor) {

	if (!CalculateColorsInfoGlobal.IsPaintTask) return;


	// Prints info if we're for instance trying to Add color to an already fully painted mesh, if not painting with physics surface or overriding vertex colors to apply, so we can reliably use the RGBA colors to apply here. If painting with phys surface or overriding then can't in a clean and performant way here check how much that we for instance overrided for every vertex.


	if (AnyVertexColorGotChanged) {

		CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Any Vertex Color Got Changed")), FColor::Cyan));
	}

	else {

		CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Any Vertex Color Didn't Get Changed By Paint Job!")), FColor::Cyan));
	}


	if (ColorsOfEachChannel_AboveZeroBeforeApplyingColor.SuccessfullyGotColorChannelResultsAtMinAmount && !ApplyColorSettings.ApplyVertexColorSettings.ApplyColorsUsingPhysicsSurface.ApplyVertexColorUsingPhysicsSurface && !OverrideColorsMadeChange) {


		// Checks if channel was fully painted and we where trying to add to it
		if (ColorsOfEachChannel_AboveZeroBeforeApplyingColor.RedChannelResult.AverageColorAmountAtMinAmount == 1 && ApplyColorSettings.ApplyVertexColorSettings.ApplyColorsOnRedChannel.AmountToApply > 0 && ApplyColorSettings.ApplyVertexColorSettings.ApplyColorsOnRedChannel.ApplyWithSetting == EApplyVertexColorSetting::EAddVertexColor) {

			CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Trying to Apply Color to Red with Add on an Already Fully Colored Mesh. So no visual difference will occur.")), FColor::Cyan));
		}

		if (ColorsOfEachChannel_AboveZeroBeforeApplyingColor.GreenChannelResult.AverageColorAmountAtMinAmount == 1 && ApplyColorSettings.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.AmountToApply > 0 && ApplyColorSettings.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.ApplyWithSetting == EApplyVertexColorSetting::EAddVertexColor) {

			CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Trying to Apply Color to Green with Add on an Already Fully Colored Mesh. So no visual difference will occur.")), FColor::Cyan));
		}

		if (ColorsOfEachChannel_AboveZeroBeforeApplyingColor.BlueChannelResult.AverageColorAmountAtMinAmount == 1 && ApplyColorSettings.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.AmountToApply > 0 && ApplyColorSettings.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.ApplyWithSetting == EApplyVertexColorSetting::EAddVertexColor) {

			CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Trying to Apply Color to Blue with Add on an Already Fully Colored Mesh. So no visual difference will occur.")), FColor::Cyan));
		}

		if (ColorsOfEachChannel_AboveZeroBeforeApplyingColor.AlphaChannelResult.AverageColorAmountAtMinAmount == 1 && ApplyColorSettings.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.AmountToApply > 0 && ApplyColorSettings.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.ApplyWithSetting == EApplyVertexColorSetting::EAddVertexColor) {

			CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Trying to Apply Color to Alpha with Add on an Already Fully Colored Mesh. So no visual difference will occur. ")), FColor::Cyan));
		}


		// Checks if channel was empty and we where trying to remove from it
		if (ColorsOfEachChannel_AboveZeroBeforeApplyingColor.RedChannelResult.AverageColorAmountAtMinAmount == 0 && ApplyColorSettings.ApplyVertexColorSettings.ApplyColorsOnRedChannel.AmountToApply < 0 && ApplyColorSettings.ApplyVertexColorSettings.ApplyColorsOnRedChannel.ApplyWithSetting == EApplyVertexColorSetting::EAddVertexColor) {

			CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Trying to Remove Color to Red Channel with Add but Mesh doesn't have any on Red Channel at all. So no visual difference will occur. ")), FColor::Cyan));
		}

		if (ColorsOfEachChannel_AboveZeroBeforeApplyingColor.GreenChannelResult.AverageColorAmountAtMinAmount == 0 && ApplyColorSettings.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.AmountToApply < 0 && ApplyColorSettings.ApplyVertexColorSettings.ApplyColorsOnGreenChannel.ApplyWithSetting == EApplyVertexColorSetting::EAddVertexColor) {

			CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Trying to Remove Color to Green Channel with Add but Mesh doesn't have any on Green Channel at all. So no visual difference will occur. ")), FColor::Cyan));
		}


		if (ColorsOfEachChannel_AboveZeroBeforeApplyingColor.BlueChannelResult.AverageColorAmountAtMinAmount == 0 && ApplyColorSettings.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.AmountToApply < 0 && ApplyColorSettings.ApplyVertexColorSettings.ApplyColorsOnBlueChannel.ApplyWithSetting == EApplyVertexColorSetting::EAddVertexColor) {

			CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Trying to Remove Color to Blue Channel with Add but Mesh doesn't have any on Blue Channel at all. So no visual difference will occur. ")), FColor::Cyan));
		}

		if (ColorsOfEachChannel_AboveZeroBeforeApplyingColor.AlphaChannelResult.AverageColorAmountAtMinAmount == 0 && ApplyColorSettings.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.AmountToApply < 0 && ApplyColorSettings.ApplyVertexColorSettings.ApplyColorsOnAlphaChannel.ApplyWithSetting == EApplyVertexColorSetting::EAddVertexColor) {

			CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Trying to Remove Color to Alpha Channel with Add but Mesh doesn't have any on Alpha Channel at all. So no visual difference will occur. ")), FColor::Cyan));
		}
	}


	// Prints Amount, Percent and Average Color Before Applying any colors, i.e. if Detection Paint job or if ran a Detection before a Paint at Location
	if (ColorsOfEachChannelResultsAtMinAmount_BeforeApplyingColor.SuccessfullyGotColorChannelResultsAtMinAmount) {

		CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Before Applying Any Colors - Amount of Vertices with min Red Painted: %f - Amount of Vertices with min Green Painted: %f - Amount of Vertices with min Blue Painted: %f - Amount of Vertices with min Alpha Painted: %f"), ColorsOfEachChannelResultsAtMinAmount_BeforeApplyingColor.RedChannelResult.AmountOfVerticesPaintedAtMinAmount, ColorsOfEachChannelResultsAtMinAmount_BeforeApplyingColor.GreenChannelResult.AmountOfVerticesPaintedAtMinAmount, ColorsOfEachChannelResultsAtMinAmount_BeforeApplyingColor.BlueChannelResult.AmountOfVerticesPaintedAtMinAmount, ColorsOfEachChannelResultsAtMinAmount_BeforeApplyingColor.AlphaChannelResult.AmountOfVerticesPaintedAtMinAmount), FColor::Cyan));

		CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Before Applying Any Colors - Amount of Red Channel Percent Painted: %f - Amount of Green Channel Percent Painted: %f - Amount of Blue Channel Percent Painted: %f - Amount of Alpha Channel Percent Painted: %f"), ColorsOfEachChannelResultsAtMinAmount_BeforeApplyingColor.RedChannelResult.PercentPaintedAtMinAmount, ColorsOfEachChannelResultsAtMinAmount_BeforeApplyingColor.GreenChannelResult.PercentPaintedAtMinAmount, ColorsOfEachChannelResultsAtMinAmount_BeforeApplyingColor.BlueChannelResult.PercentPaintedAtMinAmount, ColorsOfEachChannelResultsAtMinAmount_BeforeApplyingColor.AlphaChannelResult.PercentPaintedAtMinAmount), FColor::Cyan));

		CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Before Applying Any Colors - Average Amount of Red Color: %f - Average Amount of Green Color: %f - Average Amount of Blue Color: %f - Average Amount of Alpha Color: %f"), ColorsOfEachChannelResultsAtMinAmount_BeforeApplyingColor.RedChannelResult.AverageColorAmountAtMinAmount, ColorsOfEachChannelResultsAtMinAmount_BeforeApplyingColor.GreenChannelResult.AverageColorAmountAtMinAmount, ColorsOfEachChannelResultsAtMinAmount_BeforeApplyingColor.BlueChannelResult.AverageColorAmountAtMinAmount, ColorsOfEachChannelResultsAtMinAmount_BeforeApplyingColor.AlphaChannelResult.AverageColorAmountAtMinAmount), FColor::Cyan));
	}

	if (ColorsOfEachChannelResultsAtMinAmount_BeforeApplyingColor.SuccessfullyGotPhysicsSurfaceResultsAtMinAmount) {


		for (const FRVPDPAmountOfColorsOfEachChannelPhysicsResults& physicsSurfaceResults : ColorsOfEachChannelResultsAtMinAmount_BeforeApplyingColor.PhysicsSurfacesResults) {

			CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Before Applying Any Colors - Got Physics Surface Result at Min Amount for Physics Surface: %s - Amount of Vertices: %f - Amount of Percent: %f - Average Amount: %f"), *StaticEnum<EPhysicalSurface>()->GetDisplayNameTextByValue(physicsSurfaceResults.PhysicsSurface.GetValue()).ToString(), physicsSurfaceResults.AmountOfVerticesPaintedAtMinAmount, physicsSurfaceResults.PercentPaintedAtMinAmount, physicsSurfaceResults.AverageColorAmountAtMinAmount), FColor::Cyan));
		}
	}


	// Prints Amount, Percent and Average Color After Applying any colors, i.e. if it was a Paint Job
	if (ColorsOfEachChannelResultsAtMinAmount_AfterApplyingColor.SuccessfullyGotColorChannelResultsAtMinAmount) {

		CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("After Applying Any Colors - Amount of Vertices with min Red Painted: %f - Amount of Vertices with min Green Painted: %f - Amount of Vertices with min Blue Painted: %f - Amount of Vertices with min Alpha Painted: %f"), ColorsOfEachChannelResultsAtMinAmount_AfterApplyingColor.RedChannelResult.AmountOfVerticesPaintedAtMinAmount, ColorsOfEachChannelResultsAtMinAmount_AfterApplyingColor.GreenChannelResult.AmountOfVerticesPaintedAtMinAmount, ColorsOfEachChannelResultsAtMinAmount_AfterApplyingColor.BlueChannelResult.AmountOfVerticesPaintedAtMinAmount, ColorsOfEachChannelResultsAtMinAmount_AfterApplyingColor.AlphaChannelResult.AmountOfVerticesPaintedAtMinAmount), FColor::Cyan));

		CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("After Applying Any Colors - Amount of Red Channel Percent Painted: %f - Amount of Green Channel Percent Painted: %f - Amount of Blue Channel Percent Painted: %f - Amount of Alpha Channel Percent Painted: %f"), ColorsOfEachChannelResultsAtMinAmount_AfterApplyingColor.RedChannelResult.PercentPaintedAtMinAmount, ColorsOfEachChannelResultsAtMinAmount_AfterApplyingColor.GreenChannelResult.PercentPaintedAtMinAmount, ColorsOfEachChannelResultsAtMinAmount_AfterApplyingColor.BlueChannelResult.PercentPaintedAtMinAmount, ColorsOfEachChannelResultsAtMinAmount_AfterApplyingColor.AlphaChannelResult.PercentPaintedAtMinAmount), FColor::Cyan));

		CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("After Applying Any Colors - Average Amount of Red Color: %f - Average Amount of Green Color: %f - Average Amount of Blue Color: %f - Average Amount of Alpha Color: %f"), ColorsOfEachChannelResultsAtMinAmount_AfterApplyingColor.RedChannelResult.AverageColorAmountAtMinAmount, ColorsOfEachChannelResultsAtMinAmount_AfterApplyingColor.GreenChannelResult.AverageColorAmountAtMinAmount, ColorsOfEachChannelResultsAtMinAmount_AfterApplyingColor.BlueChannelResult.AverageColorAmountAtMinAmount, ColorsOfEachChannelResultsAtMinAmount_AfterApplyingColor.AlphaChannelResult.AverageColorAmountAtMinAmount), FColor::Cyan));
	}

	if (ColorsOfEachChannelResultsAtMinAmount_AfterApplyingColor.SuccessfullyGotPhysicsSurfaceResultsAtMinAmount) {

		for (const FRVPDPAmountOfColorsOfEachChannelPhysicsResults& physicsSurfaceResults : ColorsOfEachChannelResultsAtMinAmount_AfterApplyingColor.PhysicsSurfacesResults) {

			CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("After Applying Any Colors - Got Physics Surface Result at Min Amount for Physics Surface: %s - Amount of Vertices: %f - Amount of Percent: %f - Average Amount: %f"), *StaticEnum<EPhysicalSurface>()->GetDisplayNameTextByValue(physicsSurfaceResults.PhysicsSurface.GetValue()).ToString(), physicsSurfaceResults.AmountOfVerticesPaintedAtMinAmount, physicsSurfaceResults.PercentPaintedAtMinAmount, physicsSurfaceResults.AverageColorAmountAtMinAmount), FColor::Cyan));
		}
	}
}


//-------------------------------------------------------

// Get Closest Vertex Data Result

FRVPDPClosestVertexDataResults FVertexPaintCalculateColorsTask::GetClosestVertexDataResult(const UPrimitiveComponent* MeshComponent, FRVPDPClosestVertexDataResults ClosestVertexDataResult, const int& ClosestVertexIndex, const FColor& ClosestVertexColor, const FVector& ClosestVertexPosition, const FVector& ClosestVertexNormal) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetClosestVertexDataResult);
	RVPDP_CPUPROFILER_STR("Vertex Paint Game Instance Subsystem - GetClosestVertexDataResult");

	if (!ClosestVertexDataResult.ClosestVertexDataSuccessfullyAcquired || ClosestVertexIndex == -1) return ClosestVertexDataResult;


	// Transforms from component local to world pos
	ClosestVertexDataResult.ClosestVertexGeneralInfo.ClosestVertexPositionInfo.ClosestVertexPositionWorld = ClosestVertexPosition;

	// Then from world to Actor local
	ClosestVertexDataResult.ClosestVertexGeneralInfo.ClosestVertexPositionInfo.ClosestVertexPositionActorLocal = UKismetMathLibrary::InverseTransformLocation(MeshComponent->GetOwner()->GetTransform(), ClosestVertexDataResult.ClosestVertexGeneralInfo.ClosestVertexPositionInfo.ClosestVertexPositionWorld);

	ClosestVertexDataResult.ClosestVertexGeneralInfo.ClosestVertexColors = UVertexPaintFunctionLibrary::ReliableFColorToFLinearColor(ClosestVertexColor);


	// Transforms from component local normal to world normal
	ClosestVertexDataResult.ClosestVertexGeneralInfo.ClosestVertexNormalInfo.ClosestVertexNormal = ClosestVertexNormal;

	ClosestVertexDataResult.ClosestVertexGeneralInfo.ClosestVertexNormalInfo.ClosestVertexNormalLocal = UKismetMathLibrary::InverseTransformDirection(MeshComponent->GetOwner()->GetTransform(), ClosestVertexNormal);


	ClosestVertexDataResult.ClosestVertexGeneralInfo.ClosestVertexIndex = ClosestVertexIndex;


	// If got the material then gets physics surface, if it's registered in the material data asset. 
	if (IsValid(ClosestVertexDataResult.ClosestVertexGeneralInfo.ClosestVertexMaterial)) {

		ClosestVertexDataResult.ClosestVertexPhysicalSurfaceInfo = UVertexPaintFunctionLibrary::GetVertexColorPhysicsSurfaceDataFromMaterial(MeshComponent, ClosestVertexColor, ClosestVertexDataResult.ClosestVertexGeneralInfo.ClosestVertexMaterial);
	}


	return ClosestVertexDataResult;
}


//-------------------------------------------------------

// Print Closest Vertex Color Results

void FVertexPaintCalculateColorsTask::PrintClosestVertexColorResults(const FRVPDPClosestVertexDataResults& ClosestVertexDataResult) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_PrintClosestVertexColorResults);
	RVPDP_CPUPROFILER_STR("Vertex Paint Game Instance Subsystem - PrintClosestVertexColorResults");


	
	CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Closest Vertex Color: %s  -  Closest Vertex Section: %i"), *ClosestVertexDataResult.ClosestVertexGeneralInfo.ClosestVertexColors.ToString(), ClosestVertexDataResult.ClosestVertexGeneralInfo.ClosestSection), FColor::Cyan));


	if (IsValid(ClosestVertexDataResult.ClosestVertexGeneralInfo.ClosestVertexMaterial)) {

		CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Closest Material: %s"), *ClosestVertexDataResult.ClosestVertexGeneralInfo.ClosestVertexMaterial->GetName()), FColor::Cyan));
	}


	CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Closest Vertex World Position: %s  -   Closest Vertex Local Position: %s"), *ClosestVertexDataResult.ClosestVertexGeneralInfo.ClosestVertexPositionInfo.ClosestVertexPositionWorld.ToString(), *ClosestVertexDataResult.ClosestVertexGeneralInfo.ClosestVertexPositionInfo.ClosestVertexPositionActorLocal.ToString()), FColor::Cyan));


	CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Closest Vertex Normal World: %s  -  Closest Vertex Normal Local: %s"), *ClosestVertexDataResult.ClosestVertexGeneralInfo.ClosestVertexNormalInfo.ClosestVertexNormal.ToString(), *ClosestVertexDataResult.ClosestVertexGeneralInfo.ClosestVertexNormalInfo.ClosestVertexNormalLocal.ToString()), FColor::Cyan));


	// Physics Surfaces
	if (ClosestVertexDataResult.ClosestVertexPhysicalSurfaceInfo.PhysicsSurfaceSuccessfullyAcquired) {


		CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Physics Surface and Color Values at Closest Vertex: Default: %s: %f  -  Red: %s: %f  -  Green: %s: %f  -  Blue: %s: %f  -  Alpha: %s: %f"), *ClosestVertexDataResult.ClosestVertexPhysicalSurfaceInfo.PhysicsSurface_AtDefault.PhysicalSurfaceAsStringAtChannel, ClosestVertexDataResult.ClosestVertexPhysicalSurfaceInfo.PhysicsSurface_AtDefault.PhysicalSurfaceValueAtChannel, *ClosestVertexDataResult.ClosestVertexPhysicalSurfaceInfo.PhysicsSurface_AtRed.PhysicalSurfaceAsStringAtChannel, ClosestVertexDataResult.ClosestVertexPhysicalSurfaceInfo.PhysicsSurface_AtRed.PhysicalSurfaceValueAtChannel, *ClosestVertexDataResult.ClosestVertexPhysicalSurfaceInfo.PhysicsSurface_AtGreen.PhysicalSurfaceAsStringAtChannel, ClosestVertexDataResult.ClosestVertexPhysicalSurfaceInfo.PhysicsSurface_AtGreen.PhysicalSurfaceValueAtChannel, *ClosestVertexDataResult.ClosestVertexPhysicalSurfaceInfo.PhysicsSurface_AtBlue.PhysicalSurfaceAsStringAtChannel, ClosestVertexDataResult.ClosestVertexPhysicalSurfaceInfo.PhysicsSurface_AtBlue.PhysicalSurfaceValueAtChannel, *ClosestVertexDataResult.ClosestVertexPhysicalSurfaceInfo.PhysicsSurface_AtAlpha.PhysicalSurfaceAsStringAtChannel, ClosestVertexDataResult.ClosestVertexPhysicalSurfaceInfo.PhysicsSurface_AtAlpha.PhysicalSurfaceValueAtChannel), FColor::Cyan));
	}

	// If couldn't get physical surface but just closest colors
	else {

		CalculateColorsResultMessage.Add(FRVPDPAsyncTaskDebugMessagesInfo(FString::Printf(TEXT("Couldn't get Physical Surfaces at Closest Vertex, perhaps the Hit Material isn't registered in the Material Data Asset?")), FColor::Orange));
	}
}
