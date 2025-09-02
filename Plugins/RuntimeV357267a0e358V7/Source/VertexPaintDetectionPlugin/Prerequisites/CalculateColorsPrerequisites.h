// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Chaos/ChaosEngineInterface.h"

// Prerequisites
#include "TaskResultsPrerequisites.h"
#include "ChaosClothPhysicsPrerequisites.h"
#include "CorePrerequisites.h"

// Paint Job Prerequisites
#include "SetColorsWithStringPrerequisites.h"
#include "SetMeshColorsPrerequisites.h"
#include "PaintSnippetsPrerequisites.h"
#include "PaintAtLocationPrerequisites.h"
#include "PaintEntireMeshPrerequisites.h"
#include "PaintWithinAreaPrerequisites.h"

// Detect Job Prerequisites
#include "GetClosestVertexDataPrerequisites.h"
#include "GetColorsOnlyPrerequisites.h"
#include "GetColorsWithinAreaPrerequisites.h"

#include "CalculateColorsPrerequisites.generated.h" 


class UVertexPaintMaterialDataAsset;
class UClothingAssetBase;
class UMaterialInterface;
class FColorVertexBuffer;
class USplineMeshComponent;
class UInstancedStaticMeshComponent;
class UDynamicMeshComponent;
class UGeometryCollectionComponent;
class UGeometryCollection;
class FGeometryCollection;
class USkeletalMesh;


//-------------------------------------------------------

// Calculate Colors Info

USTRUCT()
struct FRVPDPCalculateColorsInfo {

	GENERATED_BODY()

	
	AActor* GetVertexPaintActor() const {

		if (VertexPaintActor.IsValid())
			return VertexPaintActor.Get();

		return nullptr;
	}
	
	UPrimitiveComponent* GetVertexPaintComponent() const {

		if (VertexPaintComponent.IsValid())
			return VertexPaintComponent.Get();

		return nullptr;
	}
	
	UStaticMeshComponent* GetVertexPaintStaticMeshComponent() const {

		if (VertexPaintStaticMeshComponent.IsValid())
			return VertexPaintStaticMeshComponent.Get();

		return nullptr;
	}

	UInstancedStaticMeshComponent* GetVertexPaintInstancedStaticMeshComponent() const {

		if (VertexPaintInstancedStaticMeshComponent.IsValid())
			return VertexPaintInstancedStaticMeshComponent.Get();

		return nullptr;
	}

	USplineMeshComponent* GetVertexPaintSplineMeshComponent() const {

		if (VertexPaintSplineMeshComponent.IsValid())
			return VertexPaintSplineMeshComponent.Get();

		return nullptr;
	}

	USkeletalMeshComponent* GetVertexPaintSkelComponent() const {

		if (VertexPaintSkelComponent.IsValid())
			return VertexPaintSkelComponent.Get();

		return nullptr;
	}

	USkeletalMesh* GetVertexPaintSkeletalMesh() const {

		if (VertexPaintSkeletalMesh.IsValid())
			return VertexPaintSkeletalMesh.Get();

		return nullptr;
	}

	UDynamicMeshComponent* GetVertexPaintDynamicMeshComponent() const {

		if (VertexPaintDynamicMeshComponent.IsValid())
			return VertexPaintDynamicMeshComponent.Get();

		return nullptr;
	}

	UGeometryCollectionComponent* GetVertexPaintGeometryCollectionComponent() const {

		if (VertexPaintGeometryCollectionComponent.IsValid())
			return VertexPaintGeometryCollectionComponent.Get();

		return nullptr;
	}

	UGeometryCollection* GetVertexPaintGeometryCollection() const {

		if (VertexPaintGeometryCollection.IsValid())
			return VertexPaintGeometryCollection.Get();

		return nullptr;
	}
	
	FGeometryCollection* GetVertexPaintGeometryCollectionData() const {

		if (VertexPaintGeometryCollectionData)
			return VertexPaintGeometryCollectionData;

		return nullptr;
	}

	bool IsGetClosestVertexDataTask() const {

		if (VertexPaintDetectionType == EVertexPaintDetectionType::GetClosestVertexDataDetection)
			return true;

		return false;
	}

	bool IsGetColorsWithinAreaTask() const {

		if (VertexPaintDetectionType == EVertexPaintDetectionType::GetColorsWithinArea)
			return true;

		return false;
	}

	bool IsGetAllVertexColorsTask() const {

		if (VertexPaintDetectionType == EVertexPaintDetectionType::GetAllVertexColorDetection)
			return true;

		return false;
	}

	bool IsPaintAtLocationTask() const {

		if (VertexPaintDetectionType == EVertexPaintDetectionType::PaintAtLocation)
		 	return true;

		return false;
	}

	bool IsPaintWithinAreaTask() const {

		if (VertexPaintDetectionType == EVertexPaintDetectionType::PaintWithinArea)
			return true;

		return false;
	}

	bool IsPaintEntireMeshTask() const {

		if (VertexPaintDetectionType == EVertexPaintDetectionType::PaintEntireMesh)
			return true;

		return false;
	}

	bool IsPaintColorSnippetTask() const {

		if (VertexPaintDetectionType == EVertexPaintDetectionType::PaintColorSnippet)
			return true;

		return false;
	}

	bool IsSetMeshVertexColorsDirectlyTask() const {

		if (VertexPaintDetectionType == EVertexPaintDetectionType::SetMeshVertexColorsDirectly)
			return true;

		return false;
	}

	bool IsSetMeshVertexColorsDirectlyUsingSerializedStringTask() const {

		if (VertexPaintDetectionType == EVertexPaintDetectionType::SetMeshVertexColorsDirectlyUsingSerializedString)
			return true;

		return false;
	}
	
	
	//---------- RESULTS ----------//

	// TODO Move this into a seperate CalculateColorsResults struct to keep things clean and more lightweight. This hasn't scaled well since the plugins has grown so much

	// Fundamentals
	FRVPDPTaskResults TaskResult;
	FRVPDPPaintTaskResultInfo PaintTaskResult;
	TArray<FColorVertexBuffer*> NewColorVertexBuffersPerLOD;

	// Paint at Location
	FRVPDPClosestVertexDataResults PaintAtLocation_ClosestVertexDataResult;
	FRVPDPEstimatedColorAtHitLocationInfo PaintAtLocation_EstimatedColorAtHitLocationResult;
	FRVPDPAverageColorInAreaInfo PaintAtLocation_AverageColorInArea;

	// Get Closest Vertex Data
	FRVPDPClosestVertexDataResults GetClosestVertexDataResult;
	FRVPDPEstimatedColorAtHitLocationInfo GetClosestVertexData_EstimatedColorAtHitLocationResult;
	FRVPDPAverageColorInAreaInfo GetClosestVertexData_AverageColorInArea;

	// Amount of Colors of Each Channel
	FRVPDPAmountOfColorsOfEachChannelResults ColorsOfEachChannelResultsAtMinAmount_BeforeApplyingColor;
	FRVPDPAmountOfColorsOfEachChannelResults ColorsOfEachChannelResultsAtMinAmount_AfterApplyingColor;
	FRVPDPAmountOfColorsOfEachChannelResults ColorsOfEachChannelResultsAtMinAmount_AboveZeroBeforeApplyingColor_Debug; // Only used for debugging to print results if you're trying to apply to an already full channel etc. 

	// Paint / Get Colors Within Area
	bool PaintWithinArea_WasAnyVertexWithinArea = false;
	FRVPDPWithinAreaResults WithinArea_Results_BeforeApplyingColors;
	FRVPDPWithinAreaResults WithinArea_Results_AfterApplyingColors;
	FRVPDPAmountOfColorsOfEachChannelResults WithinArea_ColorsOfEachChannelResultsAtMinAmount;

	// Misc
	TArray<FColor> DynamicMeshComponentVertexColors; // Doesn't use color buffers per LOD so fills a simple color array
	TArray<FLinearColor> GeometryCollectionComponentVertexColors; // Doesn't use color buffers per LOD, but Geometry collection TManagedArray<FLinearColor> uses FLinearColor, so to avoid having to loop through all the vertices on the game thread when finished for geo collection comps, they have their own array that gets filled and used. 
	bool OverridenVertexColors_MadeChangeToColorsToApply = false;



	//---------- SETTINGS ----------//

	// General
	int32 TaskID = -1;
	float TaskStartedTimeStamp = 0;
	int LodsToLoopThrough = 1;
	bool PropogateLOD0ToAllLODsUsingOctree = false;
	bool PropogateLOD0ToAllLODsFromStoredData = false;
	FRVPDPVertexDataInfo InitialMeshVertexData;
	EVertexPaintDetectionType VertexPaintDetectionType;
	FRVPDPTaskFundamentalSettings TaskFundamentalSettings;
	FString VertexPaintComponentName = ""; // So we can do logs etc. without having to worry if ptr still valid
	FString VertexPaintActorName = "";
	FTransform VertexPaintActorTransform = FTransform::Identity;
	FTransform VertexPaintComponentTransform = FTransform::Identity;

	// Static Mesh
	bool IsStaticMeshTask = false;
	bool IsSplineMeshTask = false;
	bool IsInstancedMeshTask = false;
	FTransform InstancedMeshTransform = FTransform::Identity;

	// Skeletal Mesh
	bool IsSkeletalMeshTask = false;
	TArray<FName> SkeletalMeshBonesNames;
	TArray<FName> SpecificSkeletalMeshBonesToLoopThrough;

#if ENGINE_MAJOR_VERSION == 4
	TArray<FMatrix> SkeletalMeshRefToLocals;
#elif ENGINE_MAJOR_VERSION == 5
	TArray<FMatrix44f> SkeletalMeshRefToLocals;
#endif

	// Dynamic Mesh
	bool IsDynamicMeshTask = false;

	// Geometry Collection
	bool IsGeometryCollectionTask = false;

	// Detect Combo
	bool IsDetectCombo = false;
	FRVPDPTaskResults DetectComboTaskResults;

	// Detections
	bool IsDetectTask = false;
	FRVPDPDetectTaskSettings DetectSettings;
	FRVPDPGetClosestVertexDataSettings GetClosestVertexDataSettings;
	FRVPDPGetColorsOnlySettings GetAllVertexColorsSettings;
	FRVPDPGetColorsWithinAreaSettings GetColorsWithinAreaSettings;

	// Paints
	bool IsPaintTask = false;
	bool IsPaintDirectlyTask = false;

	// TODO Remove these from this Struct and instead have the TaskQueue Cache them similiar to how we do it with AdditionalDataToPassThrough, making this struct more lightweight and cleaner. 
	FRVPDPPaintTaskSettings PaintTaskSettings;
	FRVPDPApplyColorSetting ApplyColorSettings;
	FRVPDPPaintAtLocationSettings PaintAtLocationSettings;
	FRVPDPWithinAreaSettings WithinAreaSettings;
	FRVPDPPaintWithinAreaSettings PaintWithinAreaSettings;
	FRVPDPPaintOnEntireMeshSettings PaintOnEntireMeshSettings;
	FRVPDPPaintColorSnippetSettings PaintColorSnippetSettings;
	FRVPDPPaintDirectlyTaskSettings PaintDirectlyTaskSettings;
	FRVPDPSetVertexColorsSettings SetVertexColorsSettings;
	FRVPDPSetVertexColorsUsingSerializedStringSettings SetVertexColorsUsingSerializedStringSettings;

	// Other
	FRVPDPEstimatedColorAtHitLocationSettings EstimatedColorAtHitLocationSettings;

	// Ptrs
	UPROPERTY()
	TMap<UClothingAssetBase*, FRVPDPChaosClothPhysicsSettings> ClothPhysicsSettings;

	UPROPERTY()
	TWeakObjectPtr<AActor> VertexPaintActor = nullptr;

	UPROPERTY()
	TWeakObjectPtr<UPrimitiveComponent> VertexPaintComponent = nullptr;

	UPROPERTY()
	TWeakObjectPtr<UStaticMeshComponent> VertexPaintStaticMeshComponent = nullptr;

	UPROPERTY()
	TWeakObjectPtr<UInstancedStaticMeshComponent> VertexPaintInstancedStaticMeshComponent = nullptr;

	UPROPERTY()
	TWeakObjectPtr<USplineMeshComponent> VertexPaintSplineMeshComponent = nullptr;

	UPROPERTY()
	TWeakObjectPtr<USkeletalMeshComponent> VertexPaintSkelComponent = nullptr;

	UPROPERTY()
	TWeakObjectPtr<USkeletalMesh> VertexPaintSkeletalMesh = nullptr;

	UPROPERTY()
	const UObject* VertexPaintSourceMesh = nullptr;

	UPROPERTY()
	UVertexPaintMaterialDataAsset* VertexPaintMaterialDataAsset = nullptr;

	UPROPERTY()
	TWeakObjectPtr<UDynamicMeshComponent> VertexPaintDynamicMeshComponent = nullptr;

	UPROPERTY()
	TWeakObjectPtr<UGeometryCollectionComponent> VertexPaintGeometryCollectionComponent = nullptr;

	UPROPERTY()
	TWeakObjectPtr<UGeometryCollection> VertexPaintGeometryCollection = nullptr;

	// UPROPERTY() // Not a UStruct
	FGeometryCollection* VertexPaintGeometryCollectionData = nullptr;

};
