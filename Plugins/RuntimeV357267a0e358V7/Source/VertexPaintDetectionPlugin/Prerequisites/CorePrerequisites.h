// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "Chaos/ChaosEngineInterface.h"
#include "CorePrerequisites.generated.h" 


//-------------------------------------------------------

// Apply Vertex Color Setting

UENUM(BlueprintType)
enum class EApplyVertexColorSetting : uint8 {

	EAddVertexColor = 0 UMETA(DisplayName = "Add Vertex Color", Tooltip = "Adds Color onto Existing ones until it reaches max"),
	ESetVertexColor = 1 UMETA(DisplayName = "Set Vertex Color", Tooltip = "Sets Existing color to be replaced by whatever we Set"),
};

//-------------------------------------------------------

// Paint and Detection Types

enum class EVertexPaintDetectionType : uint8 {

	GetClosestVertexDataDetection, 
	GetAllVertexColorDetection,
	GetColorsWithinArea,
	PaintAtLocation,
	PaintWithinArea,
	PaintEntireMesh,
	PaintColorSnippet,
	SetMeshVertexColorsDirectly,
	SetMeshVertexColorsDirectlyUsingSerializedString
};


//-------------------------------------------------------

// Vertex Color Channel

UENUM(BlueprintType)
enum class EVertexColorChannel : uint8 {

	RedChannel = 0 UMETA(DisplayName = "Red Channel"),
	GreenChannel = 1 UMETA(DisplayName = "Green Channel"),
	BlueChannel = 2 UMETA(DisplayName = "Blue Channel"),
	AlphaChannel = 3 UMETA(DisplayName = "Alpha Channel"),
};


//-------------------------------------------------------

// Surface At Channel

UENUM(BlueprintType)
enum class ESurfaceAtChannel : uint8 {

	Default = 0 UMETA(DisplayName = "Default"),
	RedChannel = 1 UMETA(DisplayName = "Red Channel"),
	GreenChannel = 2 UMETA(DisplayName = "Green Channel"),
	BlueChannel = 3 UMETA(DisplayName = "Blue Channel"),
	AlphaChannel = 4 UMETA(DisplayName = "Alpha Channel"),
};


//-------------------------------------------------------

// Vertex Color Channels

USTRUCT(BlueprintType, meta = (DisplayName = "Vertex Color Channels"))
struct FRVPDPVertexColorChannels {

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	TArray<EVertexColorChannel> VertexColorChannels;
};


//-------------------------------------------------------

// Serialized Colors Per LOD Info

USTRUCT(BlueprintType, meta = (DisplayName = "Serialized Colors Per LOD Info"))
struct FRVPDPSerializedColorsPerLODInfo {

	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	int Lod = 0;

	UPROPERTY(/*VisibleAnywhere, */BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	FString ColorsAtLODAsJSonString;
};


//-------------------------------------------------------

// Vertex Detect Mesh Data Per LOD Struct - NOTE Can't change the name of this struct as then Stored Color Snippet data gets corrupt. 

USTRUCT(BlueprintType, meta = (DisplayName = "Vertex Data Per LOD Info"))
struct FVertexDetectMeshDataPerLODStruct {

	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	int Lod = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	int AmountOfVerticesAtLOD = 0;

	UPROPERTY(/*VisibleAnywhere, */BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Only returns filled in the callback event if set to include vertex colors in the callback settings. "))
	TArray<FColor> MeshVertexColorsPerLODArray;

	UPROPERTY(/*VisibleAnywhere, */BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	FRVPDPSerializedColorsPerLODInfo SerializedVertexColorsData;

	UPROPERTY(/*VisibleAnywhere, */BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Only returns filled in the callback event if set to include vertex positions in the callback settings. "))
	TArray<FVector> MeshVertexPositionsInComponentSpacePerLODArray;

	UPROPERTY(/*VisibleAnywhere, */BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Only returns filled in the callback event if set to include vertex normals in the callback settings. "))
	TArray<FVector> MeshVertexNormalsPerLODArray;

	UPROPERTY(/*VisibleAnywhere, */BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Returns the vertex index, this is particularly useful if this is run with a Within Area, where you will get the true vertex indexes of those within area. "))
	TArray<int> MeshVertexIndexes;
};


//-------------------------------------------------------

// Vertex Data Info

USTRUCT(BlueprintType, meta = (DisplayName = "Vertex Data Info"))
struct FRVPDPVertexDataInfo {

	GENERATED_BODY()


	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The Skeletal or Static Mesh Source"))
	TSoftObjectPtr<UObject> MeshSource = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Soft ptr to the Primitive Mesh Component"))
	TSoftObjectPtr<UPrimitiveComponent> MeshComp = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The Vertex Data Per LOD"))
	TArray<FVertexDetectMeshDataPerLODStruct> MeshDataPerLOD;

	TArray<FColor> GetVertexColorDataAtLOD0() {

		if (MeshDataPerLOD.Num() > 0)
			return MeshDataPerLOD[0].MeshVertexColorsPerLODArray;

		return TArray<FColor>();
	}

	TArray<FVector> GetVertexPositionDataAtLOD0() {

		if (MeshDataPerLOD.Num() > 0)
			return MeshDataPerLOD[0].MeshVertexPositionsInComponentSpacePerLODArray;

		return TArray<FVector>();
	}

	TArray<FVector> GetVertexNormalDataAtLOD0() {

		if (MeshDataPerLOD.Num() > 0)
			return MeshDataPerLOD[0].MeshVertexNormalsPerLODArray;

		return TArray<FVector>();
	}
};
