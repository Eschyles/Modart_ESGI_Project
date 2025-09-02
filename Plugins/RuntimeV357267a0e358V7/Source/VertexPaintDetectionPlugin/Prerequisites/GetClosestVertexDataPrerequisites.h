// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "DetectFundementalsPrerequisites.h"
#include "FundementalsPrerequisites.h"
#include "HitLocationPrerequisites.h"
#include "GetClosestVertexDataPrerequisites.generated.h" 


class UMaterialInterface;


//-------------------------------------------------------

// Closest Vertex Position Info

USTRUCT(BlueprintType, meta = (DisplayName = "Closest Vertex Position Info"))
struct FRVPDPClosestVertexPositionInfo {

	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	FVector ClosestVertexPositionWorld = FVector(ForceInitToZero);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	FVector ClosestVertexPositionActorLocal = FVector(ForceInitToZero);
};


//-------------------------------------------------------

// Closest Vertex Normal Info

USTRUCT(BlueprintType, meta = (DisplayName = "Closest Vertex Normal Info"))
struct FRVPDPClosestVertexNormalInfo {

	GENERATED_BODY()


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	FVector ClosestVertexNormal = FVector(ForceInitToZero);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	FVector ClosestVertexNormalLocal = FVector(ForceInitToZero);
};


//-------------------------------------------------------

// Closest Vertex Info 

USTRUCT(BlueprintType, meta = (DisplayName = "Closest Vertex Info"))
struct FRVPDPClosestVertexInfo {

	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The Closest Vertex inded of Hit Location"))
	int ClosestVertexIndex = -1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The Closest Section the Closest Vertex was at. May be useful if painting / detecting on skeletal mesh components. "))
	int ClosestSection = -1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The Closest Vertex Color"))
	FLinearColor ClosestVertexColors = FLinearColor(0, 0, 0, 0);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	FRVPDPClosestVertexPositionInfo ClosestVertexPositionInfo;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	FRVPDPClosestVertexNormalInfo ClosestVertexNormalInfo;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The Material at the Closest Vertex"))
	UMaterialInterface* ClosestVertexMaterial = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "UVs at the Closest Vertex, each element in the array is for each UV Channel"))
	TArray<FVector2D> ClosestVertexUVAtEachUVChannel;
};


//-------------------------------------------------------

// Closest Vertex Data Results

USTRUCT(BlueprintType, meta = (DisplayName = "Closest Vertex Data Results"))
struct FRVPDPClosestVertexDataResults {

	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	bool ClosestVertexDataSuccessfullyAcquired = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	FRVPDPClosestVertexInfo ClosestVertexGeneralInfo;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	FRVPDPPhysicsSurfaceDataInfo ClosestVertexPhysicalSurfaceInfo;
};


//-------------------------------------------------------

// Closest Vertex Data Optimization Settings

USTRUCT(BlueprintType, meta = (DisplayName = "Closest Vertex Data Optimization Settings"))
struct FRVPDPClosestVertexDataOptimizationSettings {

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If Face Index is provided, which you can get if running a Line Trace on a Static Mesh with Trace Complex set to True, then we can more quickly get the Closest Vertex Data Results since we can make sure we only loop through the Section of the Face Index instead of all of them. \nHowever if Get Average Colors in Area is Set to True, or there is anything in the Callback Settings such as Include Vertex Positions, Normals etc. set to True, then we will have to loop through them all.  "))
	int OptionalStaticMeshFaceIndex = -1;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If True then will accept the vertex data of any that is within the set min range to the hit location as the closest vertex, if nothing in the callback settings requires us to loop through all vertices such as include vertex positions etc., or it's used in combination with paint at location, or it is set to get the average color within an AoE or to get estimated color at hit location! "))
	bool AcceptClosestVertexDataIfVertexIsWithinMinRange = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	float AcceptClosestVertexDataIfVertexIsWithinRange = 5;
};


//-------------------------------------------------------

// Get Closest Vertex Data Settings

USTRUCT(BlueprintType, meta = (DisplayName = "Get Closest Vertex Data Settings"))
struct FRVPDPGetClosestVertexDataSettings : public FRVPDPDetectTaskSettings {

	GENERATED_BODY()


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Can get the Average Color in an Area Range, which could be useful if you have for instance a Make-up game and want to see if the player has painted the majority of a characters cheek. Area Range has to be higher than 0 to get the average color. "))
	FRVPDPGetAverageColorSettings GetAverageColorInAreaSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	FRVPDPAtLocationTasksSettings HitFundementals;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "This will Return the color as close to the actual Hit Location as possible, which is very useful if you have Meshes with few vertices and you want to run SFX or VFX based on the Color of the Hit location and not the closest vertex color in case there is a diff, which it might have if the mesh have few vertices. \nThe task may take a bit longer to calculate if Mesh has alot of vertices, so you have the option to only get it if the Mesh has a Max Amount of Vertices. \NOTE If getVertexColorSetting is set to onlyGetColors, but this is True, then it will still loop through colors. "))
	FRVPDPEstimatedColorAtHitLocationSettings GetEstimatedColorAtHitLocationSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Contains any optimization settings for Get Closest Vertex Data. "))
	FRVPDPClosestVertexDataOptimizationSettings ClosestVertexDataOptimizationSettings;
};