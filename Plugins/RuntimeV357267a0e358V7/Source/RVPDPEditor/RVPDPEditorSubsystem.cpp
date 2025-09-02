// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 


#include "RVPDPEditorSubsystem.h"

// Engine
#include "Engine/SkeletalMesh.h"
#include "Rendering/SkeletalMeshModel.h"
#include "Rendering/SkeletalMeshLODModel.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "StaticMeshResources.h"
#include "FileHelpers.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"
#include "GameplayTagsManager.h"
#include "GameplayTagsEditorModule.h"
#include "Templates/SharedPointer.h"
#include <fstream>
#include <string>
#include <vector>
#include "Misc/Paths.h"

// Plugin
#include "VertexPaintDetectionSettings.h"
#include "VertexPaintFunctionLibrary.h"
#include "VertexPaintMaterialDataAsset.h"
#include "VertexPaintOptimizationDataAsset.h"
#include "VertexPaintColorSnippetDataAsset.h"
#include "VertexPaintColorSnippetRefs.h"
#include "VertexPaintDetectionLog.h"

// UE5
#if ENGINE_MAJOR_VERSION == 5

#if ENGINE_MINOR_VERSION >= 2
#include "StaticMeshComponentLODInfo.h"
#endif
#endif



//--------------------------------------------------------

// Initialize

void URVPDPEditorSubsystem::Initialize(FSubsystemCollectionBase& Collection) {

	RVPDP_LOG(FRVPDPDebugSettings(false, 0, true), FColor::Cyan, "Editor Subsystem Initialized ");
}


//--------------------------------------------------------

// Save Package Wrapper

bool URVPDPEditorSubsystem::SavePackageWrapper(UObject* PackageToSave) {

	if (!PackageToSave) return false;
	if (!PackageToSave->GetPackage()) return false;


	TArray<UPackage*> packages;
	packages.Add(PackageToSave->GetPackage());

	return UEditorLoadingAndSavingUtils::SavePackages(packages, false);
}


//--------------------------------------------------------

// Add Editor Notification

void URVPDPEditorSubsystem::AddEditorNotification(FString Notification) {

	float notificationDuration = 2;


	if (auto vertexPaintDetectionSettings = GetDefault<UVertexPaintDetectionSettings>()) {

		// If set not to show notifications then just returns
		if (!vertexPaintDetectionSettings->EditorWidgetNotificationEnabled)
			return;

		notificationDuration = vertexPaintDetectionSettings->EditorWidgetNotificationDuration;
	}


	FNotificationInfo Info(FText::FromString(Notification));

	//How long before the widget automatically starts to fade out (in seconds)
	Info.ExpireDuration = notificationDuration;

	// Info.bUseLargeFont = true;
	// Info.bFireAndForget
	// Info.bUseThrobber
	// Info.Hyperlink
	// Info.Image
	// Info.FadeInDuration = 1.0f;
	// Info.FadeOutDuration = 1.0f;
	// Info.WidthOverride = 500.0f;

	FSlateNotificationManager::Get().AddNotification(Info);
}


//--------------------------------------------------------

// Set Engine Specific Button Settings

void URVPDPEditorSubsystem::SetEngineSpecificButtonSettings(TArray<UButton*> Buttons, TArray<UComboBoxString*> ComboBoxes) {

	if (Buttons.Num() == 0) return;


#if ENGINE_MAJOR_VERSION == 5

	// It turns out Buttons in UE5 has really dark tint by default, and outline of 1 which i have to turn off here

	for (UButton* button : Buttons) {

		FButtonStyle buttonStyle;

#if ENGINE_MINOR_VERSION >= 2
		buttonStyle = button->GetStyle();
#else
		buttonStyle = button->WidgetStyle;
#endif


		buttonStyle.Normal.TintColor = FSlateColor(FLinearColor::White);
		buttonStyle.Normal.OutlineSettings.Width = 0;

		buttonStyle.Hovered.TintColor = FSlateColor(FLinearColor::White);
		buttonStyle.Hovered.OutlineSettings.Width = 0;

		buttonStyle.Pressed.TintColor = FSlateColor(FLinearColor::White);
		buttonStyle.Pressed.OutlineSettings.Width = 0;

		buttonStyle.Disabled.TintColor = FSlateColor(FLinearColor::White);
		buttonStyle.Disabled.OutlineSettings.Width = 0;
		buttonStyle.Disabled.DrawAs = ESlateBrushDrawType::NoDrawType;


#if ENGINE_MINOR_VERSION >= 2
		button->SetStyle(buttonStyle);
#else
		button->WidgetStyle = buttonStyle;
#endif
	}

	for (UComboBoxString* comboBox : ComboBoxes) {

		FComboBoxStyle comboBoxStyle;

#if ENGINE_MINOR_VERSION >= 2
		comboBoxStyle = comboBox->GetWidgetStyle();
#else
		comboBoxStyle = comboBox->WidgetStyle;
#endif

		comboBoxStyle.ComboButtonStyle.ButtonStyle.Normal.DrawAs = ESlateBrushDrawType::Box;
		comboBoxStyle.ComboButtonStyle.ButtonStyle.Normal.OutlineSettings.Width = 0;

		comboBoxStyle.ComboButtonStyle.ButtonStyle.Hovered.DrawAs = ESlateBrushDrawType::Box;
		comboBoxStyle.ComboButtonStyle.ButtonStyle.Hovered.OutlineSettings.Width = 0;

		comboBoxStyle.ComboButtonStyle.ButtonStyle.Pressed.DrawAs = ESlateBrushDrawType::Box;
		comboBoxStyle.ComboButtonStyle.ButtonStyle.Pressed.OutlineSettings.Width = 0;

		comboBoxStyle.ComboButtonStyle.ButtonStyle.Disabled.DrawAs = ESlateBrushDrawType::Box;
		comboBoxStyle.ComboButtonStyle.ButtonStyle.Disabled.OutlineSettings.Width = 0;


#if ENGINE_MINOR_VERSION >= 2
		comboBox->SetWidgetStyle(comboBoxStyle);
#else
		comboBox->WidgetStyle = comboBoxStyle;
#endif
	}

#endif
}


//--------------------------------------------------------

// Register Skeletal Mesh Max Amount Of LODs To Paint

void URVPDPEditorSubsystem::RegisterSkeletalMeshMaxAmountOfLODsToPaint(USkeletalMesh* SkeletalMesh, int MaxAmountOfLODsToPaint, bool& Success) {

	Success = false;

	if (!SkeletalMesh) return;


	if (UVertexPaintOptimizationDataAsset* optimizationDataAsset = UVertexPaintFunctionLibrary::GetOptimizationDataAsset(nullptr)) {


		Success = true;

		optimizationDataAsset->RegisterSkeletalMeshMaxNumOfLODsToPaint(SkeletalMesh, MaxAmountOfLODsToPaint);

		AddEditorNotification(FString::Printf(TEXT("Registered Skeletal Mesh %s Max Paint on LOD!"), *SkeletalMesh->GetName()));

		SavePackageWrapper(optimizationDataAsset);
	}
}


//--------------------------------------------------------

// Un-Register Skeletal Mesh Max Amount Of LODs To Paint

void URVPDPEditorSubsystem::UnRegisterSkeletalMeshMaxAmountOfLODsToPaint(USkeletalMesh* SkeletalMesh, bool& Success) {

	Success = false;

	if (!SkeletalMesh) return;


	if (UVertexPaintOptimizationDataAsset* optimizationDataAsset = UVertexPaintFunctionLibrary::GetOptimizationDataAsset(nullptr)) {

		if (optimizationDataAsset->GetRegisteredSkeletalMeshInfo().Contains(SkeletalMesh)) {

			if (optimizationDataAsset->GetRegisteredSkeletalMeshInfo().FindRef(SkeletalMesh).MaxAmountOfLODsToPaint > 0) {


				optimizationDataAsset->UnRegisterFromSkeletalMeshMaxNumOfLODsToPaint(SkeletalMesh);


				AddEditorNotification(FString::Printf(TEXT("Un-Registered Skeletal Mesh %s Max Paint on LOD!"), *SkeletalMesh->GetName()));

				Success = true;
				SavePackageWrapper(optimizationDataAsset);
			}
		}
	}
}


//--------------------------------------------------------

// Register Skeletal Mesh Bone Info

void URVPDPEditorSubsystem::RegisterSkeletalMeshBoneInfo(USkeletalMesh* SkeletalMesh, bool& Success) {

	Success = false;

	if (!SkeletalMesh) return;


	if (UVertexPaintOptimizationDataAsset* optimizationDataAsset = UVertexPaintFunctionLibrary::GetOptimizationDataAsset(nullptr)) {


		if (optimizationDataAsset->RegisterSkeletalMeshBoneInfo(SkeletalMesh)) {


			AddEditorNotification(FString::Printf(TEXT("Registered Skeletal Mesh %s Bone Info!"), *SkeletalMesh->GetName()));

			Success = true;
			SavePackageWrapper(optimizationDataAsset);
		}
	}
}


//--------------------------------------------------------

// UnRegister Skeletal Mesh Bone Info

void URVPDPEditorSubsystem::UnRegisterSkeletalMeshBoneInfo(USkeletalMesh* SkeletalMesh, bool& Success) {

	Success = false;

	if (!SkeletalMesh) return;


	if (UVertexPaintOptimizationDataAsset* optimizationDataAsset = UVertexPaintFunctionLibrary::GetOptimizationDataAsset(nullptr)) {

		if (optimizationDataAsset->UnRegisterSkeletalMeshBoneInfo(SkeletalMesh)) {

			AddEditorNotification(FString::Printf(TEXT("Un-Registered Skeletal Mesh %s Bone Info!"), *SkeletalMesh->GetName()));

			SavePackageWrapper(optimizationDataAsset);
			Success = true;
		}
	}
}


//--------------------------------------------------------

// Register Mesh Propogate LOD Info

void URVPDPEditorSubsystem::RegisterMeshPropogateLODInfo(UObject* MeshAssets, bool& Success) {

	Success = false;

	if (!MeshAssets) return;


	if (UVertexPaintOptimizationDataAsset* optimizationDataAsset = UVertexPaintFunctionLibrary::GetOptimizationDataAsset(nullptr)) {


		if (optimizationDataAsset->RegisterMeshPropogateLODInfo(MeshAssets)) {

			AddEditorNotification(FString::Printf(TEXT("Registered Mesh %s Propogate LOD Info!"), *MeshAssets->GetName()));

			Success = true;
			SavePackageWrapper(optimizationDataAsset);
		}
	}
}


//--------------------------------------------------------

// UnRegister Mesh Propogate LOD Info

void URVPDPEditorSubsystem::UnRegisterMeshPropogateLODInfo(UObject* MeshAsset, bool& Success) {

	Success = false;

	if (!MeshAsset) return;


	if (UVertexPaintOptimizationDataAsset* optimizationDataAsset = UVertexPaintFunctionLibrary::GetOptimizationDataAsset(nullptr)) {


		if (optimizationDataAsset->UnRegisterMeshPropogateLODInfo(MeshAsset)) {

			AddEditorNotification(FString::Printf(TEXT("UnRegistered Mesh %s Propogate LOD Info!"), *MeshAsset->GetName()));

			Success = true;
			SavePackageWrapper(optimizationDataAsset);
		}
	}
}


//--------------------------------------------------------

// Add Static Mesh And Amount Of LODs To Paint

void URVPDPEditorSubsystem::AddStaticMeshAndAmountOfLODsToPaintByDefault(UStaticMesh* StaticMesh, int MaxAmountOfLODsToPaint, bool& Success) {

	Success = false;

	if (!IsValid(StaticMesh)) return;


	if (UVertexPaintOptimizationDataAsset* optimizationDataAsset = UVertexPaintFunctionLibrary::GetOptimizationDataAsset(nullptr)) {


		optimizationDataAsset->RegisterStaticMeshMaxNumOfLODsToPaint(StaticMesh, MaxAmountOfLODsToPaint);

		Success = true;
		SavePackageWrapper(optimizationDataAsset);
	}
}


//--------------------------------------------------------

// Remove Static Mesh And Amount Of LODs To Paint

void URVPDPEditorSubsystem::RemoveStaticMeshAndAmountOfLODsToPaintByDefault(UStaticMesh* StaticMesh, bool& Success) {

	Success = false;

	if (!StaticMesh) return;


	if (UVertexPaintOptimizationDataAsset* optimizationDataAsset = UVertexPaintFunctionLibrary::GetOptimizationDataAsset(nullptr)) {

		if (optimizationDataAsset->GetRegisteredStaticMeshInfo().Contains(StaticMesh)) {

			if (optimizationDataAsset->GetRegisteredStaticMeshInfo().FindRef(StaticMesh).MaxAmountOfLODsToPaint > 0) {


				optimizationDataAsset->UnRegisterFromStaticMeshMaxNumOfLODsToPaint(StaticMesh);

				Success = true;
				SavePackageWrapper(optimizationDataAsset);
			}
		}
	}
}


//--------------------------------------------------------

// Add Group Color Snippet

void URVPDPEditorSubsystem::AddGroupColorSnippet(FRVPDPGroupColorSnippetInfo GroupSnippetInfo, bool& Success) {

	Success = false;

	if (GroupSnippetInfo.GroupSnippetID.IsEmpty()) return;
	if (GroupSnippetInfo.AssociatedGroupMeshes.Num() == 0) return;


	if (UVertexPaintColorSnippetRefs* colorSnippetReferenceDataAsset = UVertexPaintFunctionLibrary::GetColorSnippetReferenceDataAsset(nullptr)) {

		colorSnippetReferenceDataAsset->GroupSnippetsAndAssociatedMeshes.Add(GroupSnippetInfo.GroupSnippetID, GroupSnippetInfo);

		Success = true;
		SavePackageWrapper(colorSnippetReferenceDataAsset);
	}

	else {

		RVPDP_LOG(FRVPDPDebugSettings(false, 0, true), FColor::Cyan, "Trying to Add Group Color Snippet but there is no Color Snippet Reference Data Asset Set in Project Settings. ");
	}
}


//--------------------------------------------------------

// Remove Group Color Snippet

void URVPDPEditorSubsystem::RemoveGroupColorSnippet(FString GroupSnippetID, bool& Success) {

	Success = false;

	if (GroupSnippetID.IsEmpty()) return;


	if (UVertexPaintColorSnippetRefs* colorSnippetReferenceDataAsset = UVertexPaintFunctionLibrary::GetColorSnippetReferenceDataAsset(nullptr)) {

		if (colorSnippetReferenceDataAsset->GroupSnippetsAndAssociatedMeshes.Contains(GroupSnippetID)) {

			colorSnippetReferenceDataAsset->GroupSnippetsAndAssociatedMeshes.Remove(GroupSnippetID);

			Success = true;
			SavePackageWrapper(colorSnippetReferenceDataAsset);
		}
	}

	else {

		RVPDP_LOG(FRVPDPDebugSettings(false, 0, true), FColor::Cyan, "Trying to Remove Group Color Snippet but there is no Color Snippet Reference Data Asset Set in Project Settings. ");
	}
}


//--------------------------------------------------------

// Add Mesh Color Snippet

void URVPDPEditorSubsystem::AddMeshColorSnippet(UPrimitiveComponent* MeshComponent, FString ColorSnippetID, bool IsPartOfAGroupSnippet, FString GroupSnippedID, FVector RelativeLocationToGroupCenterPoint, float DotProductToGroupCenterPoint, UVertexPaintColorSnippetDataAsset* ColorSnippetDataAsset, bool& Success) {

	Success = false;

	if (!MeshComponent) return;
	if (!ColorSnippetDataAsset) return;
	if (ColorSnippetID.Len() <= 0) return;

	UVertexPaintColorSnippetRefs* colorSnippetReferenceDataAsset = UVertexPaintFunctionLibrary::GetColorSnippetReferenceDataAsset(nullptr);

	if (!colorSnippetReferenceDataAsset) {

		RVPDP_LOG(FRVPDPDebugSettings(false, 0, true), FColor::Cyan, "Trying to Add Mesh Color Snippet but there is no Color Snippet Reference Data Asset Set in Project Settings. ");
		return;
	}



	FRVPDPStoredColorSnippetInfo storedColorSnippetInfo;
	storedColorSnippetInfo.ColorSnippetDataAssetStoredOn = ColorSnippetDataAsset;
	storedColorSnippetInfo.ObjectColorSnippetBelongsTo = TSoftObjectPtr<UObject>(FSoftObjectPath(UVertexPaintFunctionLibrary::GetMeshComponentSourceMesh(MeshComponent)->GetPathName()));
	storedColorSnippetInfo.IsPartOfAGroupSnippet = IsPartOfAGroupSnippet;
	storedColorSnippetInfo.GroupSnippetID = GroupSnippedID;
	storedColorSnippetInfo.RelativeLocationToGroupCenterPoint = RelativeLocationToGroupCenterPoint;
	storedColorSnippetInfo.DotProductToGroupCenterPoint = DotProductToGroupCenterPoint;


	if (auto staticMeshComponent = Cast<UStaticMeshComponent>(MeshComponent)) {

		if (!staticMeshComponent->GetStaticMesh()) {

			RVPDP_LOG(FRVPDPDebugSettings(false, 0, true), FColor::Cyan, "Couldn't store Static Mesh Color Snippet because the selected Static Mesh Component a Static Mesh Set ");
			return;
		}


		if (staticMeshComponent->LODData.Num() > 0) {


			if (staticMeshComponent->LODData[0].PaintedVertices.Num() == 0) {


				if (IsPartOfAGroupSnippet) {

					// If Group snippet we just skip any meshes without any vertex colors. 

					RVPDP_LOG(FRVPDPDebugSettings(false, 0, true), FColor::Cyan, "Skips Static Mesh Color Snippet for Group Snippet because the selected Static Mesh doesn't have any Painted Vertices at LOD0 ");

					return;
				}

				else {

					RVPDP_LOG(FRVPDPDebugSettings(false, 0, true), FColor::Cyan, "Couldn't store Static Mesh Color Snippet because the selected Static Mesh doesn't have any Painted Vertices at LOD0");

					AddEditorNotification(FString::Printf(TEXT("Failed at Adding Color Snippet - Mesh has no Colors to Add!")));

					return;
				}
			}


			bool anyVertexPainted = false;

			for (FPaintedVertex paintedVert : staticMeshComponent->LODData[0].PaintedVertices) {

				if (paintedVert.Color.R > 0 || paintedVert.Color.G > 0 || paintedVert.Color.B > 0 || paintedVert.Color.A > 0) {

					anyVertexPainted = true;
					break;
				}
			}


			if (!anyVertexPainted) {

				if (IsPartOfAGroupSnippet) {

					RVPDP_LOG(FRVPDPDebugSettings(false, 0, true), FColor::Cyan, "Skips Static Mesh Color Snippet for Group Snippet because the selected Static Mesh didn't have a single vertex painted");

					return;
				}

				else {

					RVPDP_LOG(FRVPDPDebugSettings(false, 0, true), FColor::Cyan, "Couldn't store Static Mesh Color Snippet because the selected Static Mesh didn't have a single vertex painted");

					AddEditorNotification(FString::Printf(TEXT("Failed at Adding Color Snippet - Mesh has no Colors to Add!")));

					return;
				}
			}
		}

		else {

			if (IsPartOfAGroupSnippet) {

				RVPDP_LOG(FRVPDPDebugSettings(false, 0, true), FColor::Cyan, "Skips Static Mesh Color Snippet for Group Snippet because the selected Static Mesh doesn't have a LOD0");

				return;
			}

			else {

				RVPDP_LOG(FRVPDPDebugSettings(false, 0, true), FColor::Cyan, "Couldn't store Static Mesh Color Snippet because the selected Static Mesh doesn't have any a LOD0");

				AddEditorNotification(FString::Printf(TEXT("Failed at Adding Color Snippet - Mesh has no LOD0")));

				return;
			}
		}


		UObject* staticMesh = nullptr;

#if ENGINE_MAJOR_VERSION == 4

		staticMesh = staticMeshComponent->GetStaticMesh();

#elif ENGINE_MAJOR_VERSION == 5

		staticMesh = staticMeshComponent->GetStaticMesh().Get();

#endif


		FRVPDPColorSnippetDataInfo colorSnippetData;
		colorSnippetData.ObjectColorSnippetBelongsTo = staticMeshComponent->GetStaticMesh();

		FVertexDetectMeshDataPerLODStruct colorSnippetPerLOD;
		colorSnippetPerLOD.Lod = 0;
		colorSnippetPerLOD.MeshVertexColorsPerLODArray = UVertexPaintFunctionLibrary::GetStaticMeshVertexColorsAtLOD(staticMeshComponent, 0);
		colorSnippetPerLOD.AmountOfVerticesAtLOD = colorSnippetPerLOD.MeshVertexColorsPerLODArray.Num();

		colorSnippetData.ColorSnippetDataPerLOD.Add(colorSnippetPerLOD);

		ColorSnippetDataAsset->SnippetColorData.Add(ColorSnippetID, colorSnippetData);
		SavePackageWrapper(ColorSnippetDataAsset);


		FRVPDPColorSnippetReferenceDataInfo colorSnippetReferenceData;

		if (colorSnippetReferenceDataAsset->StaticMeshesColorSnippets.Contains(staticMesh))
			colorSnippetReferenceData = colorSnippetReferenceDataAsset->StaticMeshesColorSnippets.FindRef(staticMesh);

		colorSnippetReferenceData.ColorSnippetsStorageInfo.Add(ColorSnippetID, storedColorSnippetInfo);
		colorSnippetReferenceDataAsset->StaticMeshesColorSnippets.Add(staticMesh, colorSnippetReferenceData);


		SavePackageWrapper(colorSnippetReferenceDataAsset);

		Success = true;

		AddEditorNotification(FString::Printf(TEXT("Successfully Added Static Mesh Color Snippet: %s!"), *ColorSnippetID));
	}

	else if (auto skeletalMeshComponent = Cast<USkeletalMeshComponent>(MeshComponent)) {


		USkeletalMesh* skeletalMeshAsset = nullptr;

#if ENGINE_MAJOR_VERSION == 4

		skeletalMeshAsset = Cast<USkeletalMesh>(skeletalMeshComponent->SkeletalMesh);

#elif ENGINE_MAJOR_VERSION == 5


#if ENGINE_MINOR_VERSION == 0

		skeletalMeshAsset = Cast<USkeletalMesh>(skeletalMeshComponent->SkeletalMesh.Get());

#else

		skeletalMeshAsset = Cast<USkeletalMesh>(skeletalMeshComponent->GetSkeletalMeshAsset());

#endif
#endif


		FRVPDPColorSnippetDataInfo colorSnippetData;
		colorSnippetData.ObjectColorSnippetBelongsTo = skeletalMeshAsset;


		FVertexDetectMeshDataPerLODStruct colorSnippetPerLOD;
		colorSnippetPerLOD.Lod = 0;
		colorSnippetPerLOD.MeshVertexColorsPerLODArray = UVertexPaintFunctionLibrary::UVertexPaintFunctionLibrary::GetSkeletalMeshVertexColorsAtLOD(skeletalMeshComponent, 0);
		colorSnippetPerLOD.AmountOfVerticesAtLOD = colorSnippetPerLOD.MeshVertexColorsPerLODArray.Num();

		colorSnippetData.ColorSnippetDataPerLOD.Add(colorSnippetPerLOD);

		ColorSnippetDataAsset->SnippetColorData.Add(ColorSnippetID, colorSnippetData);
		SavePackageWrapper(ColorSnippetDataAsset);

		FRVPDPColorSnippetReferenceDataInfo colorSnippetReferenceData;

		if (colorSnippetReferenceDataAsset->SkeletalMeshesColorSnippets.Contains(skeletalMeshAsset))
			colorSnippetReferenceData = colorSnippetReferenceDataAsset->SkeletalMeshesColorSnippets.FindRef(skeletalMeshAsset);


		colorSnippetReferenceData.ColorSnippetsStorageInfo.Add(ColorSnippetID, storedColorSnippetInfo);
		colorSnippetReferenceDataAsset->SkeletalMeshesColorSnippets.Add(skeletalMeshAsset, colorSnippetReferenceData);

		SavePackageWrapper(colorSnippetReferenceDataAsset);

		Success = true;

		AddEditorNotification(FString::Printf(TEXT("Successfully Added Skeletal Mesh Color Snippet: %s!"), *ColorSnippetID));
	}


	// If wasn't Success, i.e. didn't save in the function then we save here as a failsafe
	AddColorSnippetTag(ColorSnippetID);

	RefreshAllAvailableCachedColorSnippetTagContainer();
}


//--------------------------------------------------------

// Move Mesh Color Snippet

void URVPDPEditorSubsystem::MoveMeshColorSnippet(FString ColorSnippetID, UVertexPaintColorSnippetDataAsset* ColorSnippetDataAssetToMoveFrom, UVertexPaintColorSnippetDataAsset* ColorSnippetDataAssetToMoveTo, bool& Success) {

	Success = false;

	if (!ColorSnippetDataAssetToMoveFrom) return;
	if (!ColorSnippetDataAssetToMoveTo) return;
	if (ColorSnippetID.Len() <= 0) return;


	if (ColorSnippetDataAssetToMoveTo->SnippetColorData.Contains(ColorSnippetID)) {

		RVPDP_LOG(FRVPDPDebugSettings(false, 0, true), FColor::Cyan, "Trying to Move Mesh Color Snippet to a Data Asset that already has it.");
		return;
	}


	UVertexPaintColorSnippetRefs* colorSnippetReferenceDataAsset = UVertexPaintFunctionLibrary::GetColorSnippetReferenceDataAsset(nullptr);

	if (!colorSnippetReferenceDataAsset) {

		RVPDP_LOG(FRVPDPDebugSettings(false, 0, true), FColor::Cyan, "Trying to Move Mesh Color Snippet but there is no Color Snippet Reference Data Asset Set in Project Settings.");
		return;
	}

	if (!colorSnippetReferenceDataAsset->ContainsColorSnippet(ColorSnippetID, false, nullptr)) {

		RVPDP_LOG(FRVPDPDebugSettings(false, 0, true), FColor::Cyan, "Trying to Move Mesh Color Snippet but the Color Snippet isn't registered in the Reference Data Asset. ");
		return;
	}


	if (ColorSnippetDataAssetToMoveFrom->SnippetColorData.Contains(ColorSnippetID)) {

		UObject* snippetMeshToMove = colorSnippetReferenceDataAsset->GetObjectFromSnippetID(ColorSnippetID).Get();

		FRVPDPColorSnippetDataInfo colorSnippetStruct = ColorSnippetDataAssetToMoveFrom->SnippetColorData.FindRef(ColorSnippetID);

		ColorSnippetDataAssetToMoveFrom->SnippetColorData.Remove(ColorSnippetID);
		ColorSnippetDataAssetToMoveTo->SnippetColorData.Add(ColorSnippetID, colorSnippetStruct);


		FRVPDPColorSnippetReferenceDataInfo colorSnippetReferenceData;

		if (Cast<USkeletalMesh>(snippetMeshToMove)) {

			if (colorSnippetReferenceDataAsset->SkeletalMeshesColorSnippets.Contains(snippetMeshToMove)) {

				colorSnippetReferenceData = colorSnippetReferenceDataAsset->SkeletalMeshesColorSnippets.FindRef(snippetMeshToMove);

				if (colorSnippetReferenceData.ColorSnippetsStorageInfo.Contains(ColorSnippetID)) {

					FRVPDPStoredColorSnippetInfo storedColorSnippetInfo = colorSnippetReferenceData.ColorSnippetsStorageInfo.FindRef(ColorSnippetID);
					storedColorSnippetInfo.ColorSnippetDataAssetStoredOn = ColorSnippetDataAssetToMoveTo;

					colorSnippetReferenceData.ColorSnippetsStorageInfo.Add(ColorSnippetID, storedColorSnippetInfo);
					colorSnippetReferenceDataAsset->SkeletalMeshesColorSnippets.Add(snippetMeshToMove, colorSnippetReferenceData);
				}
			}
		}

		else if (Cast<UStaticMesh>(snippetMeshToMove)) {

			if (colorSnippetReferenceDataAsset->StaticMeshesColorSnippets.Contains(snippetMeshToMove)) {

				colorSnippetReferenceData = colorSnippetReferenceDataAsset->StaticMeshesColorSnippets.FindRef(snippetMeshToMove);

				if (colorSnippetReferenceData.ColorSnippetsStorageInfo.Contains(ColorSnippetID)) {

					FRVPDPStoredColorSnippetInfo storedColorSnippetInfo = colorSnippetReferenceData.ColorSnippetsStorageInfo.FindRef(ColorSnippetID);
					storedColorSnippetInfo.ColorSnippetDataAssetStoredOn = ColorSnippetDataAssetToMoveTo;

					colorSnippetReferenceData.ColorSnippetsStorageInfo.Add(ColorSnippetID, storedColorSnippetInfo);
					colorSnippetReferenceDataAsset->StaticMeshesColorSnippets.Add(snippetMeshToMove, colorSnippetReferenceData);
				}
			}
		}


		SavePackageWrapper(ColorSnippetDataAssetToMoveFrom);
		SavePackageWrapper(ColorSnippetDataAssetToMoveTo);
		SavePackageWrapper(colorSnippetReferenceDataAsset);

		AddEditorNotification(FString::Printf(TEXT("Successfully Moved Color Snippet: %s!"), *ColorSnippetID));

		Success = true;
	}

	else {

		RVPDP_LOG(FRVPDPDebugSettings(false, 0, true), FColor::Cyan, "Trying to Move Mesh Color Snippet but the Color Snippet isn't registered in the Color Snippet Data Asset. ");

		return;
	}
}


//--------------------------------------------------------

// Move Group Color Snippet

void URVPDPEditorSubsystem::MoveGroupColorSnippet(FString groupColorSnippetID, UVertexPaintColorSnippetDataAsset* ColorSnippetDataAssetToMoveTo) {

	if (!ColorSnippetDataAssetToMoveTo) return;
	if (groupColorSnippetID.IsEmpty()) return;


	if (UVertexPaintColorSnippetRefs* colorSnippetReferenceDataAsset = UVertexPaintFunctionLibrary::GetColorSnippetReferenceDataAsset(nullptr)) {

		if (colorSnippetReferenceDataAsset->GroupSnippetsAndAssociatedMeshes.Contains(groupColorSnippetID)) {

			FRVPDPGroupColorSnippetInfo groupSnippetInfo = colorSnippetReferenceDataAsset->GroupSnippetsAndAssociatedMeshes.FindRef(groupColorSnippetID);
			groupSnippetInfo.ColorSnippetDataAssetStoredOn = ColorSnippetDataAssetToMoveTo;

			colorSnippetReferenceDataAsset->GroupSnippetsAndAssociatedMeshes.Add(groupColorSnippetID, groupSnippetInfo);

			SavePackageWrapper(colorSnippetReferenceDataAsset);
		}
	}

	else {

		RVPDP_LOG(FRVPDPDebugSettings(false, 0, true), FColor::Cyan, "Trying to Move Group Snippet but there is no Color Snippet Reference Data Asset Set in Project Settings. ");
		return;
	}
}

//--------------------------------------------------------

// Add Color Snippet Tag

bool URVPDPEditorSubsystem::AddColorSnippetTag(FString Tag) {

	if (DoesColorSnippetTagExist(Tag)) return false;
	if (!UVertexPaintFunctionLibrary::GetColorSnippetReferenceDataAsset(nullptr)) return false;


	if (auto gameplayTagEditorModule = &IGameplayTagsEditorModule::Get()) {

		if (gameplayTagEditorModule) {

			gameplayTagEditorModule->AddNewGameplayTagToINI(Tag, ColorSnippetDevComments, "");
			return true;
		}
	}

	return false;
}


//--------------------------------------------------------

// Does Color Snippet Tag Exist

bool URVPDPEditorSubsystem::DoesColorSnippetTagExist(FString Tag) {

	if (Tag.IsEmpty()) return false;


	if (UGameplayTagsManager* gameplayTagManager = &UGameplayTagsManager::Get()) {

		FName colorSnippetTagName(*Tag);
		TSharedPtr<FGameplayTagNode> colorSnippetTagNodePtr = gameplayTagManager->FindTagNode(colorSnippetTagName);

		return colorSnippetTagNodePtr.IsValid();
	}

	return false;
}


//--------------------------------------------------------

// Get All Color Snippet Tags As String

TArray<FString> URVPDPEditorSubsystem::GetAllColorSnippetTagsDirectlyFromIni() {


	/* NOTE We DON'T want to use this to get all color snippets since this gets updated when manually added and removed etc., we want to be able to pull them from the .ini directly, so in case the user changes them manually etc. we can clean them up.
	if (!UVertexPaintFunctionLibrary::GetColorSnippetReferenceDataAsset()) return TArray<FString>();


	TArray<FGameplayTag> allAvailableGameplayTags;
	UVertexPaintFunctionLibrary::GetColorSnippetReferenceDataAsset()->AllAvailableColorSnippets.GetGameplayTagArray(allAvailableGameplayTags);


	TArray<FString> allAvailableGameplayTagsAsStrings;

	for (auto tag : allAvailableGameplayTags)
		allAvailableGameplayTagsAsStrings.Add(tag.GetTagName().ToString());

	return allAvailableGameplayTagsAsStrings;
	*/


	TArray<FString> tagsAsString;

	// Filters for the tag that has our dev comment
	std::string devCommentToFilterFor = std::string(TCHAR_TO_UTF8(*ColorSnippetDevComments));
	std::string pathToProjectRoot = std::string(TCHAR_TO_UTF8(*FPaths::ProjectDir()));

	// Open the DefaultGameplayTags.ini file for reading
	std::ifstream inFile(pathToProjectRoot + "/Config/DefaultGameplayTags.ini");

	// Check if the file was opened successfully
	if (!inFile.is_open()) {
		// Handle error
		return tagsAsString;
	}

	// Read the contents of the file into a string
	std::string fileContents((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());

	// Close the input file
	inFile.close();


	// Find all tags in the file contents that have the specified dev comment
	std::vector<std::string> tags;
	size_t pos = 0;

	while ((pos = fileContents.find("+GameplayTagList=", pos)) != std::string::npos) {

		// Find the end of the line containing this tag
		size_t lineEnd = fileContents.find('\n', pos);

		if (lineEnd == std::string::npos) {
			lineEnd = fileContents.length();
		}

		// Extract this tag's dev comment
		size_t devCommentStart = fileContents.find("DevComment=\"", pos);
		if (devCommentStart == std::string::npos || devCommentStart >= lineEnd) {
			// This tag doesn't have a dev comment, so skip it
			pos = lineEnd + 1;
			continue;
		}

		devCommentStart += strlen("DevComment=\"");
		size_t devCommentEnd = fileContents.find('\"', devCommentStart);
		if (devCommentEnd == std::string::npos || devCommentEnd >= lineEnd) {

			// This tag's dev comment is malformed, so skip it
			pos = lineEnd + 1;
			continue;
		}

		std::string thisDevComment = fileContents.substr(devCommentStart, devCommentEnd - devCommentStart);

		// Check if this tag has the specified dev comment
		if (devCommentToFilterFor.empty() || thisDevComment == devCommentToFilterFor) {

			// Extract this tag's name
			size_t tagStart = fileContents.find("Tag=\"", pos);
			if (tagStart == std::string::npos || tagStart >= lineEnd) {

				// This tag is malformed, so skip it
				pos = lineEnd + 1;
				continue;
			}

			tagStart += strlen("Tag=\"");
			size_t tagEnd = fileContents.find('\"', tagStart);

			if (tagEnd == std::string::npos || tagEnd >= lineEnd) {

				// This tag is malformed, so skip it
				pos = lineEnd + 1;
				continue;
			}

			std::string thisTag = fileContents.substr(tagStart, tagEnd - tagStart);

			// Add this tag to the list of tags with the specified dev comment
			tags.push_back(thisTag);
		}

		// Move to the next line in the file contents
		pos = lineEnd + 1;
	}

	// Convert the vector of tags to an array of strings
	std::vector<const char*> cStrings(tags.size());

	for (size_t i = 0; i < tags.size(); ++i) {

		cStrings[i] = tags[i].c_str();
	}

	// Converts to FString and adds to array
	for (const char* cString : cStrings) {

		tagsAsString.Add(FString(cString));
	}

	return tagsAsString;

}


//--------------------------------------------------------

// Remove Mesh Color Snippet

void URVPDPEditorSubsystem::RemoveMeshColorSnippet(FString ColorSnippetID, UVertexPaintColorSnippetDataAsset* ColorSnippetDataAsset) {

	if (!ColorSnippetDataAsset) return;
	if (ColorSnippetID.Len() <= 0) return;


	UVertexPaintColorSnippetRefs* colorSnippetReferenceDataAsset = UVertexPaintFunctionLibrary::GetColorSnippetReferenceDataAsset(nullptr);

	if (!colorSnippetReferenceDataAsset) {

		RVPDP_LOG(FRVPDPDebugSettings(false, 0, true), FColor::Cyan, "Trying to Remove Mesh Color Snippet but there is no Color Snippet Reference Mesh Data Asset Set in Project Settings.");
		return;
	}


	FString childSnippetString = ColorSnippetID;
	FName childSnippetName = FName(*childSnippetString);

	// Verify references
	FAssetIdentifier TagId = FAssetIdentifier(FGameplayTag::StaticStruct(), childSnippetName);
	TArray<FAssetIdentifier> Referencers;

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistryModule.Get().GetReferencers(TagId, Referencers, UE::AssetRegistry::EDependencyCategory::SearchableName);

	if (Referencers.Num() > 0) {

		AddEditorNotification(FString::Printf(TEXT("Cannot Delete Color Snippet: %s  -  It's Gameplay Tag is stilled Referenced somewhere!"), *ColorSnippetID));
		return;
	}


	if (colorSnippetReferenceDataAsset->GroupSnippetsAndAssociatedMeshes.Contains(ColorSnippetID)) {


		TMap<FString, FRVPDPStoredColorSnippetInfo> storedColorSnippetInfo = colorSnippetReferenceDataAsset->GetAllColorSnippetsInSpecifiedDataAsset(ColorSnippetDataAsset, true);

		TArray<FString> groupChildSnippetIDs;
		storedColorSnippetInfo.GetKeys(groupChildSnippetIDs);

		TArray<FRVPDPStoredColorSnippetInfo> groupChildSnippetInfo;
		storedColorSnippetInfo.GenerateValueArray(groupChildSnippetInfo);


		// Removes all childs
		for (int i = 0; i < groupChildSnippetInfo.Num(); i++) {

			if (groupChildSnippetInfo[i].IsPartOfAGroupSnippet && groupChildSnippetInfo[i].GroupSnippetID == ColorSnippetID) {

				ColorSnippetDataAsset->SnippetColorData.Remove(groupChildSnippetIDs[i]);
				colorSnippetReferenceDataAsset->RemoveColorSnippet(groupChildSnippetIDs[i]);
				RemoveColorSnippetTag(groupChildSnippetIDs[i]);
			}
		}

		// And Lastly removes from the Group TMap as well
		colorSnippetReferenceDataAsset->GroupSnippetsAndAssociatedMeshes.Remove(ColorSnippetID);

		SavePackageWrapper(ColorSnippetDataAsset);
		SavePackageWrapper(colorSnippetReferenceDataAsset);
	}

	else {

		if (ColorSnippetDataAsset->SnippetColorData.Contains(ColorSnippetID)) {

			ColorSnippetDataAsset->SnippetColorData.Remove(ColorSnippetID);
			SavePackageWrapper(ColorSnippetDataAsset);
		}

		colorSnippetReferenceDataAsset->RemoveColorSnippet(ColorSnippetID);
		RemoveColorSnippetTag(ColorSnippetID);
		SavePackageWrapper(colorSnippetReferenceDataAsset);
	}


	RefreshAllAvailableCachedColorSnippetTagContainer();
}


//--------------------------------------------------------

// Remove Color Snippet Tag

bool URVPDPEditorSubsystem::RemoveColorSnippetTag(FString Tag) {

	if (Tag.IsEmpty()) return false;
	if (!UVertexPaintFunctionLibrary::GetColorSnippetReferenceDataAsset(nullptr)) return false;


	if (UGameplayTagsManager* gameplayTagManager = &UGameplayTagsManager::Get()) {

		FName colorSnippetIDTagName(*Tag);
		TSharedPtr<FGameplayTagNode> colorSnippetTagNodePtr = gameplayTagManager->FindTagNode(colorSnippetIDTagName);

		if (colorSnippetTagNodePtr.IsValid()) {

			if (auto gameplayTagEditorModule = &IGameplayTagsEditorModule::Get()) {

				gameplayTagEditorModule->DeleteTagFromINI(colorSnippetTagNodePtr);

				return true;
			}
		}
	}

	return false;
}


//--------------------------------------------------------

// Rename Color Snippet Tag

void URVPDPEditorSubsystem::RenameColorSnippetTag(FString OldTag, FString NewTag) {

	if (OldTag.IsEmpty()) return;
	if (NewTag.IsEmpty()) return;


	RemoveColorSnippetTag(OldTag);
	AddColorSnippetTag(NewTag);

	/* Couldn't use the RenameTagInINI since it ask you to Restart the Editor
	if (auto gameplayTagManager = &UGameplayTagsManager::Get()) {

		FName prevColorSnippetTagName(*OldTag);
		TSharedPtr<FGameplayTagNode> colorSnippetTagNodePtr = gameplayTagManager->FindTagNode(prevColorSnippetTagName);

		if (colorSnippetTagNodePtr.IsValid()) {

			if (auto gameplayTagEditorModule = &IGameplayTagsEditorModule::Get())
				gameplayTagEditorModule->RenameTagInINI(OldTag, NewTag);
		}
	}
	*/
}


//--------------------------------------------------------

// Clear Cached Color Snippet Tag Container

void URVPDPEditorSubsystem::ClearAllAvailableColorSnippetCacheTagContainer() {

	
	if (UVertexPaintColorSnippetRefs* colorSnippetReferenceDataAsset = UVertexPaintFunctionLibrary::GetColorSnippetReferenceDataAsset(nullptr)) {

		// First clears them
		TArray<FGameplayTag> currentGameplayTags;
		colorSnippetReferenceDataAsset->AllAvailableColorSnippets.GetGameplayTagArray(currentGameplayTags);

		if (currentGameplayTags.Num() > 0) {

			FGameplayTagContainer currentGameplayTagContainer = colorSnippetReferenceDataAsset->AllAvailableColorSnippets;
			colorSnippetReferenceDataAsset->AllAvailableColorSnippets.RemoveTags(currentGameplayTagContainer);
		}

		SavePackageWrapper(colorSnippetReferenceDataAsset);
	}
}


//--------------------------------------------------------

// Refresh Cached Color Snippet Tag Container

void URVPDPEditorSubsystem::RefreshAllAvailableCachedColorSnippetTagContainer() {

	UVertexPaintColorSnippetRefs* colorSnippetReferenceDataAsset = UVertexPaintFunctionLibrary::GetColorSnippetReferenceDataAsset(nullptr);
	if (!colorSnippetReferenceDataAsset) return;


	if (UGameplayTagsManager* gameplayTagManager = &UGameplayTagsManager::Get()) {


		TArray<FRVPDPColorSnippetReferenceDataInfo> totalColorSnippetReferences;
		TArray<FRVPDPColorSnippetReferenceDataInfo> staticMeshColorSnippetReference;
		TArray<FRVPDPColorSnippetReferenceDataInfo> skeletalMeshColorSnippetReference;
		colorSnippetReferenceDataAsset->StaticMeshesColorSnippets.GenerateValueArray(staticMeshColorSnippetReference);
		colorSnippetReferenceDataAsset->SkeletalMeshesColorSnippets.GenerateValueArray(skeletalMeshColorSnippetReference);


		totalColorSnippetReferences.Append(staticMeshColorSnippetReference);
		totalColorSnippetReferences.Append(skeletalMeshColorSnippetReference);

		int amountOfSnippets = 0;

		for (FRVPDPColorSnippetReferenceDataInfo snippetRef : totalColorSnippetReferences)
			amountOfSnippets += snippetRef.ColorSnippetsStorageInfo.Num();


		bool changeOnDataAsset = false;

		// If different amount then gonna clear later and refresh
		if (amountOfSnippets != colorSnippetReferenceDataAsset->AllAvailableColorSnippets.Num())
			changeOnDataAsset = true;


		TArray<FGameplayTag> tagsToAdd;

		// Then fills it up with whatever is registered to the Static and Skeletal Mesh snippets
		for (FRVPDPColorSnippetReferenceDataInfo snippetRef : totalColorSnippetReferences) {

			TArray<FString> colorSnippetStrings;
			snippetRef.ColorSnippetsStorageInfo.GetKeys(colorSnippetStrings);

			for (FString snippetString : colorSnippetStrings) {

				FName colorSnippetIDTagName(*snippetString);
				FGameplayTag requestedTag = gameplayTagManager->RequestGameplayTag(colorSnippetIDTagName);

				if (requestedTag.IsValid()) {

					tagsToAdd.Add(requestedTag);

					// If doesn't have a tag, then we set so we clear the data asset later and fill it will all of them. So we only save it if we actually do a change
					if (!colorSnippetReferenceDataAsset->AllAvailableColorSnippets.HasTag(requestedTag)) {

						changeOnDataAsset = true;
					}
				}
			}
		}

		// If discovered that all available tags aren't the same amount, or there was tags missing in the all available color snippets, then we clear and then add the correct ones. Has this check so we don't save the color snippet reference data asset unless we actually need to. 
		if (changeOnDataAsset) {


			RVPDP_LOG(FRVPDPDebugSettings(false, 0, true), FColor::Cyan, "Missmatch with amount of Tags Registered on Color Snippet Reference Data Asset, Re-stores them!");

			ClearAllAvailableColorSnippetCacheTagContainer();

			for (FGameplayTag tag : tagsToAdd)
				colorSnippetReferenceDataAsset->AllAvailableColorSnippets.AddTag(tag);

			SavePackageWrapper(colorSnippetReferenceDataAsset);
		}
	}
}


//--------------------------------------------------------

// Update Mesh Color Snippet ID

void URVPDPEditorSubsystem::UpdateMeshColorSnippetID(FString PrevColorSnippetID, FString NewColorSnippetID, bool IsPartOfGroupSnippet, FString GroupSnippedID, UVertexPaintColorSnippetDataAsset* ColorSnippetDataAsset, bool& Success) {

	Success = false;

	if (!ColorSnippetDataAsset) return;
	if (PrevColorSnippetID.Len() <= 0) return;
	if (NewColorSnippetID.Len() <= 0) return;


	UVertexPaintColorSnippetRefs* colorSnippetReferenceDataAsset = UVertexPaintFunctionLibrary::GetColorSnippetReferenceDataAsset(nullptr);

	if (!colorSnippetReferenceDataAsset) {

		RVPDP_LOG(FRVPDPDebugSettings(false, 0, true), FColor::Cyan, "Trying to Update Mesh Color Snippet but there is no Color Snippet Reference Data Asset Set in Project Settings.");
		return;
	}


	if (!colorSnippetReferenceDataAsset->ContainsColorSnippet(PrevColorSnippetID, false, nullptr)) {

		RVPDP_LOG(FRVPDPDebugSettings(false, 0, true), FColor::Cyan, "Trying to Update Color Snippet ID but the Color Snippet isn't registered in the Reference Data Asset. ");
		return;
	}


	// Updates the Color Snippet Data Asset First
	if (ColorSnippetDataAsset->SnippetColorData.Contains(PrevColorSnippetID)) {

		FRVPDPColorSnippetDataInfo colorSnippetStruct = ColorSnippetDataAsset->SnippetColorData.FindRef(PrevColorSnippetID);
		ColorSnippetDataAsset->SnippetColorData.Remove(PrevColorSnippetID);
		ColorSnippetDataAsset->SnippetColorData.Add(NewColorSnippetID, colorSnippetStruct);

		SavePackageWrapper(ColorSnippetDataAsset);
	}


	// Then the Color Snippet Reference Data Asset
	auto mesh = colorSnippetReferenceDataAsset->GetObjectFromSnippetID(PrevColorSnippetID);


	FRVPDPColorSnippetReferenceDataInfo colorSnippetReference;

	if (colorSnippetReferenceDataAsset->SkeletalMeshesColorSnippets.Contains(mesh.Get())) {

		colorSnippetReference = colorSnippetReferenceDataAsset->SkeletalMeshesColorSnippets.FindRef(mesh.Get());
	}

	else if (colorSnippetReferenceDataAsset->StaticMeshesColorSnippets.Contains(mesh.Get())) {

		colorSnippetReference = colorSnippetReferenceDataAsset->StaticMeshesColorSnippets.FindRef(mesh.Get());
	}


	if (colorSnippetReference.ColorSnippetsStorageInfo.Num() > 0) {

		if (colorSnippetReference.ColorSnippetsStorageInfo.Contains(PrevColorSnippetID)) {

			FRVPDPStoredColorSnippetInfo storedColorSnippetInfo = colorSnippetReference.ColorSnippetsStorageInfo.FindRef(PrevColorSnippetID);

			if (IsPartOfGroupSnippet)
				storedColorSnippetInfo.GroupSnippetID = GroupSnippedID;

			colorSnippetReference.ColorSnippetsStorageInfo.Remove(PrevColorSnippetID);
			colorSnippetReference.ColorSnippetsStorageInfo.Add(NewColorSnippetID, storedColorSnippetInfo);

			if (Cast<USkeletalMesh>(mesh.Get())) {

				colorSnippetReferenceDataAsset->SkeletalMeshesColorSnippets.Add(mesh.Get(), colorSnippetReference);
			}

			else if (Cast<UStaticMesh>(mesh.Get())) {

				colorSnippetReferenceDataAsset->StaticMeshesColorSnippets.Add(mesh.Get(), colorSnippetReference);
			}
		}
	}


	Success = true;

	AddEditorNotification(FString::Printf(TEXT("Successfully Updated Color Snippet to: %s!"), *NewColorSnippetID));
	RenameColorSnippetTag(PrevColorSnippetID, NewColorSnippetID);
	SavePackageWrapper(colorSnippetReferenceDataAsset);
	RefreshAllAvailableCachedColorSnippetTagContainer();
}


//--------------------------------------------------------

// Update Group Color Snippet ID

void URVPDPEditorSubsystem::UpdateGroupColorSnippetID(FString PrevGroupColorSnippetID, FString NewGroupColorSnippetID) {

	if (PrevGroupColorSnippetID.IsEmpty()) return;
	if (NewGroupColorSnippetID.IsEmpty()) return;


	if (UVertexPaintColorSnippetRefs* colorSnippetReferenceDataAsset = UVertexPaintFunctionLibrary::GetColorSnippetReferenceDataAsset(nullptr)) {

		if (colorSnippetReferenceDataAsset->GroupSnippetsAndAssociatedMeshes.Contains(PrevGroupColorSnippetID)) {

			FRVPDPGroupColorSnippetInfo groupSnippetInfo = colorSnippetReferenceDataAsset->GroupSnippetsAndAssociatedMeshes.FindRef(PrevGroupColorSnippetID);

			groupSnippetInfo.GroupSnippetID = NewGroupColorSnippetID;
			colorSnippetReferenceDataAsset->GroupSnippetsAndAssociatedMeshes.Remove(PrevGroupColorSnippetID);
			colorSnippetReferenceDataAsset->GroupSnippetsAndAssociatedMeshes.Add(NewGroupColorSnippetID, groupSnippetInfo);

			SavePackageWrapper(colorSnippetReferenceDataAsset);
		}
	}

	else {

		RVPDP_LOG(FRVPDPDebugSettings(false, 0, true), FColor::Cyan, "Trying to Update Group Snippet but there is no Color Snippet Reference Data Asset Set in Project Settings. ");
		return;
	}
}

//--------------------------------------------------------

// Apply Vertex Color Data To Mesh

bool URVPDPEditorSubsystem::ApplyVertexColorToMeshAtLOD0(UPrimitiveComponent* MeshComponent, TArray<FColor> VertexColorsAtLOD0) {

	if (!IsValid(MeshComponent)) return false;
	if (VertexColorsAtLOD0.Num() == 0) return false;


	if (auto staticMeshComp = Cast<UStaticMeshComponent>(MeshComponent)) {

		staticMeshComp->SetLODDataCount(1, staticMeshComp->GetStaticMesh()->GetNumLODs());
		staticMeshComp->Modify();

		if (staticMeshComp->LODData.IsValidIndex(0)) {

			if (staticMeshComp->LODData[0].OverrideVertexColors) {

				staticMeshComp->LODData[0].ReleaseOverrideVertexColorsAndBlock(); // UMeshPaintingToolset::SetInstanceColorDataForLOD ran this and it seem to fix an issue if updating a color snippet, then previewing it again where you couldn't reset it afterwards. 

			}

			staticMeshComp->LODData[0].OverrideVertexColors = new FColorVertexBuffer;
			staticMeshComp->LODData[0].OverrideVertexColors->InitFromColorArray(VertexColorsAtLOD0);

			BeginInitResource(staticMeshComp->LODData[0].OverrideVertexColors);

			staticMeshComp->MarkRenderStateDirty();
			return true;
		}
	}

	else if (auto skeletalMeshComp = Cast<USkeletalMeshComponent>(MeshComponent)) {


		USkeletalMesh* skelMesh = nullptr;

#if ENGINE_MAJOR_VERSION == 4

		skelMesh = skeletalMeshComp->SkeletalMesh;

#elif ENGINE_MAJOR_VERSION == 5

#if ENGINE_MINOR_VERSION == 0

		skelMesh = skeletalMeshComp->SkeletalMesh.Get();

#else

		skelMesh = skeletalMeshComp->GetSkeletalMeshAsset();

#endif
#endif


		if (skeletalMeshComp->LODInfo.IsValidIndex(0)) {

			// Dirty the mesh
			skelMesh->SetFlags(RF_Transactional);
			skelMesh->Modify();
			skelMesh->SetHasVertexColors(true);
			skelMesh->SetVertexColorGuid(FGuid::NewGuid());

			// Release the static mesh's resources.
			skelMesh->ReleaseResources();

			// Flush the resource release commands to the rendering thread to ensure that the build doesn't occur while a resource is still
			// allocated, and potentially accessing the USkeletalMesh.
			skelMesh->ReleaseResourcesFence.Wait();

			// const int32 NumLODs = Mesh->GetLODNum();
			const int32 NumLODs = 1;
			if (NumLODs > 0)
			{
				TUniquePtr<FSkinnedMeshComponentRecreateRenderStateContext> RecreateRenderStateContext = MakeUnique<FSkinnedMeshComponentRecreateRenderStateContext>(skelMesh);

				for (int32 LODIndex = 0; LODIndex < NumLODs; ++LODIndex)
				{
					FSkeletalMeshLODRenderData& LODData = skelMesh->GetResourceForRendering()->LODRenderData[LODIndex];
					FPositionVertexBuffer& PositionVertexBuffer = LODData.StaticVertexBuffers.PositionVertexBuffer;


					// Check if the provided vertex colors match the mesh's vertex count
					int32 VertexCount = PositionVertexBuffer.GetNumVertices();
					if (VertexColorsAtLOD0.Num() != VertexCount)
					{
						RVPDP_LOG(FRVPDPDebugSettings(false, 0, true), FColor::Cyan, "The number of provided vertex colors does not match the mesh's vertex count. Vertex Colors Num: %i  -  PositionVertexBuffer amount: %i ", VertexColorsAtLOD0.Num(), VertexCount);
						continue;
					}

					// Make sure the existing color buffer is initialized
					if (!LODData.StaticVertexBuffers.ColorVertexBuffer.IsInitialized())
					{
						LODData.StaticVertexBuffers.ColorVertexBuffer.Init(VertexCount);
					}

					for (int32 j = 0; j < VertexCount; j++)
					{
						if (VertexColorsAtLOD0.IsValidIndex(j))
						{
							LODData.StaticVertexBuffers.ColorVertexBuffer.VertexColor(j) = VertexColorsAtLOD0[j];
						}
					}

					ENQUEUE_RENDER_COMMAND(RefreshColorBuffer)(
						[&LODData](FRHICommandListImmediate& RHICmdList)
						{
							BeginInitResource(&LODData.StaticVertexBuffers.ColorVertexBuffer);
						});
				}

				skelMesh->InitResources();
			}

			return true;
		}
	}

	return false;
}


//--------------------------------------------------------

// Set Custom Settings Optimizations Data Asset To Use

void URVPDPEditorSubsystem::SetCustomSettingsOptimizationsDataAssetToUse(UObject* DataAsset) {

	// if (!IsValid(DataAsset)) return; // No valid check here as we should be able to set it to nothing if we want to


	if (UVertexPaintDetectionSettings* developerSettings = GetMutableDefault<UVertexPaintDetectionSettings>()) {

		if (UVertexPaintOptimizationDataAsset* optimizationsDataAsset = Cast<UVertexPaintOptimizationDataAsset>(DataAsset))
			developerSettings->OptimizationDataAssetToUse = optimizationsDataAsset;
		else
			developerSettings->OptimizationDataAssetToUse = nullptr;


		developerSettings->SaveConfig();


#if ENGINE_MAJOR_VERSION == 4

		developerSettings->UpdateDefaultConfigFile();

#elif ENGINE_MAJOR_VERSION == 5

		developerSettings->TryUpdateDefaultConfigFile();
#endif
	}
}


//----------------------------------------------------------------------------------------------------------------

// Set Custom Settings Vertex Paint Material To Use

void URVPDPEditorSubsystem::SetCustomSettingsVertexPaintMaterialToUse(UObject* DataAsset) {

	// if (!IsValid(DataAsset)) return;  // No valid check here as we should be able to set it to nothing if we want to


	if (UVertexPaintDetectionSettings* developerSettings = GetMutableDefault<UVertexPaintDetectionSettings>()) {

		if (UVertexPaintMaterialDataAsset* paintOnMaterialDataAsset = Cast<UVertexPaintMaterialDataAsset>(DataAsset))
			developerSettings->MaterialsDataAssetToUse = paintOnMaterialDataAsset;
		else
			developerSettings->MaterialsDataAssetToUse = nullptr;


		developerSettings->SaveConfig();


#if ENGINE_MAJOR_VERSION == 4

		developerSettings->UpdateDefaultConfigFile();

#elif ENGINE_MAJOR_VERSION == 5

		developerSettings->TryUpdateDefaultConfigFile();
#endif
	}
}


//----------------------------------------------------------------------------------------------------------------

// Set Custom Settings Vertex Paint Color Snippet Reference Data Asset To Use

void URVPDPEditorSubsystem::SetCustomSettingsVertexPaintColorSnippetReferenceDataAssetToUse(UObject* DataAsset) {

	// if (!IsValid(DataAsset)) return; // No valid check here as we should be able to set it to nothing if we want to


	if (UVertexPaintDetectionSettings* developerSettings = GetMutableDefault<UVertexPaintDetectionSettings>()) {

		if (UVertexPaintColorSnippetRefs* skeletalMeshDataAsset = Cast<UVertexPaintColorSnippetRefs>(DataAsset))
			developerSettings->ColorSnippetReferencesDataAssetToUse = skeletalMeshDataAsset;
		else
			developerSettings->ColorSnippetReferencesDataAssetToUse = nullptr;

		developerSettings->SaveConfig();


#if ENGINE_MAJOR_VERSION == 4

		developerSettings->UpdateDefaultConfigFile();

#elif ENGINE_MAJOR_VERSION == 5

		developerSettings->TryUpdateDefaultConfigFile();
#endif
	}
}


//--------------------------------------------------------

// Load Essential Data Assets - Used by for instance Editor Widget to make sure they're loaded when it's open, to avoid race condition if you close the editor with the widget open and the next time you open the editor

void URVPDPEditorSubsystem::LoadEssentialDataAssets() {

	UVertexPaintFunctionLibrary::GetColorSnippetReferenceDataAsset(nullptr);
	UVertexPaintFunctionLibrary::GetOptimizationDataAsset(nullptr);
	UVertexPaintFunctionLibrary::GetVertexPaintMaterialDataAsset(nullptr);
}


//--------------------------------------------------------

// Get Objects Of Class

TArray<UObject*> URVPDPEditorSubsystem::GetObjectsOfClass(TSubclassOf<UObject> ObjectsToGet) {

	if (!ObjectsToGet.Get()) return TArray<UObject*>();


	TArray<FDirectoryPath> additionalDirectoryPathsToAllow;

	if (auto vertexPaintDetectionSettings = GetDefault<UVertexPaintDetectionSettings>())
		additionalDirectoryPathsToAllow = vertexPaintDetectionSettings->EditorWidgetAdditionalPathsToLookUpAssets;

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> AssetData;
	TArray<UObject*> objects;


#if ENGINE_MAJOR_VERSION == 4

	AssetRegistryModule.Get().GetAssetsByClass(ObjectsToGet.Get()->GetFName(), AssetData);

#elif ENGINE_MAJOR_VERSION == 5

#if ENGINE_MINOR_VERSION == 0

	AssetRegistryModule.Get().GetAssetsByClass(ObjectsToGet.Get()->GetFName(), AssetData);


#else

	AssetRegistryModule.Get().GetAssetsByClass(ObjectsToGet.Get()->GetClassPathName(), AssetData);

#endif
#endif


	for (FAssetData asset : AssetData) {


		// If asset it not in the content folder
		if (!asset.ToSoftObjectPath().ToString().StartsWith("/Game", ESearchCase::IgnoreCase)) {

			if (additionalDirectoryPathsToAllow.Num() > 0) {

				bool isAllowed = false;

				for (FDirectoryPath directoryPath : additionalDirectoryPathsToAllow) {

					if (asset.ToSoftObjectPath().ToString().StartsWith(directoryPath.Path, ESearchCase::IgnoreCase)) {

						isAllowed = true;
						break;
					}
				}

				if (!isAllowed) continue;
			}

			else {

				continue;
			}
		}

		if (asset.GetAsset())
			objects.Add(asset.GetAsset());
	}

	return objects;
}

//--------------------------------------------------------

// Get Soft Object Path From Soft Object Ptr

FSoftObjectPath URVPDPEditorSubsystem::GetSoftObjectPathFromSoftObjectPtr(TSoftObjectPtr<UObject> Object) {

	if (!Object) return FSoftObjectPath();

	return Object.ToSoftObjectPath();
}


//--------------------------------------------------------

// Get Soft Object Name From Soft Object Ptr

FString URVPDPEditorSubsystem::GetObjectNameFromSoftObjectPtr(TSoftObjectPtr<UObject> Object) {

	if (!Object) return FString();

	return Object.GetAssetName();
}


//--------------------------------------------------------

// Get Object Of Class as Soft Ptrs

TArray<TSoftObjectPtr<UObject>> URVPDPEditorSubsystem::GetObjectsOfClassAsSoftPtrs(TSubclassOf<UObject> ObjectsToGet, bool LoadObjects) {

	if (!ObjectsToGet.Get()) return TArray<TSoftObjectPtr<UObject>>();



	TArray<FDirectoryPath> additionalDirectoryPathsToAllow;

	if (auto vertexPaintDetectionSettings = GetDefault<UVertexPaintDetectionSettings>())
		additionalDirectoryPathsToAllow = vertexPaintDetectionSettings->EditorWidgetAdditionalPathsToLookUpAssets;

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> AssetData;

#if ENGINE_MAJOR_VERSION == 4

	AssetRegistryModule.Get().GetAssetsByClass(ObjectsToGet.Get()->GetFName(), AssetData);

#elif ENGINE_MAJOR_VERSION == 5

#if ENGINE_MINOR_VERSION == 0

	AssetRegistryModule.Get().GetAssetsByClass(ObjectsToGet.Get()->GetFName(), AssetData);


#else

	AssetRegistryModule.Get().GetAssetsByClass(ObjectsToGet.Get()->GetClassPathName(), AssetData);

#endif
#endif


	TArray<TSoftObjectPtr<UObject>> softObjectPtrs;
	TSoftObjectPtr<UObject> SoftObject;

	for (FAssetData asset : AssetData) {


		// If asset it not in the content folder
		if (!asset.ToSoftObjectPath().ToString().StartsWith("/Game", ESearchCase::IgnoreCase)) {

			if (additionalDirectoryPathsToAllow.Num() > 0) {

				bool isAllowed = false;

				for (FDirectoryPath directoryPath : additionalDirectoryPathsToAllow) {

					if (asset.ToSoftObjectPath().ToString().StartsWith(directoryPath.Path, ESearchCase::IgnoreCase)) {

						isAllowed = true;
						break;
					}
				}

				if (!isAllowed) continue;
			}

			else {

				continue;
			}
		}

		SoftObject = asset.ToSoftObjectPath();


		if (LoadObjects)
			SoftObject.LoadSynchronous();

		if (!SoftObject) continue;

		softObjectPtrs.Add(SoftObject);
	}

	return softObjectPtrs;
}


//-------------------------------------------------------

// Wrappers

int URVPDPEditorSubsystem::GetStaticMeshVerticesAmount_Wrapper(UStaticMesh* Mesh, int Lod) {

	if (!IsValid(Mesh)) return 0;

	if (!Mesh->GetRenderData()->LODResources.IsValidIndex(Lod)) return 0;


	return Mesh->GetRenderData()->LODResources[Lod].GetNumVertices();
}

int URVPDPEditorSubsystem::GetSkeletalMeshVerticesAmount_Wrapper(USkeletalMesh* Mesh, int Lod) {

	if (!IsValid(Mesh)) return 0;

	if (!Mesh->GetImportedModel()->LODModels.IsValidIndex(Lod)) return 0;


	return Mesh->GetImportedModel()->LODModels[Lod].NumVertices;
}

int URVPDPEditorSubsystem::GetStaticMeshLODCount_Wrapper(UStaticMesh* Mesh) {

	if (!Mesh) return -1;

	return Mesh->GetNumSourceModels();
}

int URVPDPEditorSubsystem::GetSkeletalMeshLODCount_Wrapper(USkeletalMesh* SkeletalMesh) {

	if (!SkeletalMesh) return -1;

	return SkeletalMesh->GetLODNum();
}

int URVPDPEditorSubsystem::GetSkeletalMeshComponentVertexCount_Wrapper(USkeletalMeshComponent* SkeletalMeshComponent, int Lod) {

	if (!SkeletalMeshComponent) return 0;
	if (!SkeletalMeshComponent->GetSkeletalMeshRenderData()->LODRenderData.IsValidIndex(Lod)) return 0;

	return SkeletalMeshComponent->GetSkeletalMeshRenderData()->LODRenderData[Lod].GetNumVertices();
}

int URVPDPEditorSubsystem::GetStaticMeshComponentVertexCount_Wrapper(UStaticMeshComponent* StaticMeshComponent, int Lod) {

	if (!StaticMeshComponent) return 0;


	if (StaticMeshComponent->GetStaticMesh()->GetRenderData()->LODResources.IsValidIndex(Lod))
		return StaticMeshComponent->GetStaticMesh()->GetRenderData()->LODResources[Lod].GetNumVertices();

	return 0;
}

UWorld* URVPDPEditorSubsystem::GetPersistentLevelsWorld_Wrapper(const UObject* WorldContext) {

	if (!IsValid(WorldContext)) return nullptr;

	return WorldContext->GetWorld();
}


//-------------------------------------------------------

// Deinitialize

void URVPDPEditorSubsystem::Deinitialize() {

	RVPDP_LOG(FRVPDPDebugSettings(false, 0, true), FColor::Cyan, "Editor Subsystem De-Initialized ");
}
