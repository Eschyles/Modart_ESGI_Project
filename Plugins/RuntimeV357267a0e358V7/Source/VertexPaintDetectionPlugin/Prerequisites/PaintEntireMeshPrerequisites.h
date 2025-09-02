// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "PaintFundementalsPrerequisites.h"
#include "PaintEntireMeshPrerequisites.generated.h" 


//-------------------------------------------------------

// Paint Entire Mesh Propogate To LODs Method

UENUM(BlueprintType)
enum class EPaintEntireMeshPropogateToLODsMethod : uint8 {

	ModifiedEngineMethod = 0 UMETA(DisplayName = "Modified Engine Method", Tooltip = "If we should propogate LOD0 Colors to the other LODs"),
	DontPropogateLOD0ToAllLODs = 1 UMETA(DisplayName = "Dont Propogate LOD0 To All LODs"),
};


//-------------------------------------------------------

// Paint On Entire Mesh At Random Vertices Settings

USTRUCT(BlueprintType, meta = (DisplayName = "Paint On Entire Mesh At Random Vertices Settings"))
struct FRVPDPPaintOnEntireMeshAtRandomVerticesSettings {

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If true then paints on unique Random Vertices over the Entire Mesh. "))
	bool PaintAtRandomVerticesSpreadOutOverTheEntireMesh = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If choosen to paint random vertices over the entire mesh then this is the % from 0-100. "))
	float PaintAtRandomVerticesSpreadOutOverTheEntireMesh_PercentToPaint = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If set to Not propogate LOD0 Colors to the Rest of the LODs, it will randomize for each LOD, meaning LOD0 can look pretty different from for instance LOD1 since they've both run the randomization and could've picked completely different vertices, but this option is the fastest. \nThe Modified Engine Method is copied from the engines own solution when you Mesh Paint in Editor Time but is modified so it can run in runtime and will loop through all vertices in LODs above 0 and make sure they get the color of the closest and most optimal LOD0 vertex. "))
	EPaintEntireMeshPropogateToLODsMethod PaintAtRandomVerticesSpreadOutOverTheEntireMesh_PropogateLOD0ToAllLODsMethod = EPaintEntireMeshPropogateToLODsMethod::ModifiedEngineMethod;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If True then we will only Randomize within an Area of Effect around the location set, and not bother using the Percent to Randomize over the Entire Mesh since we're just doing it within an area. Instead you have to set the Probability Factor yourself, how big the odds or that a vertex within the area will be painted. "))
	bool OnlyRandomizeWithinAreaOfEffectAtLocation = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The Area Of Effect to Randomize within. "))
	float OnlyRandomizeWithinAreaOfEffectAtLocation_AreaOfEffect = 250;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The Area of Effect based on this Location we should Randomize within. "))
	FVector OnlyRandomizeWithinAreaOfEffectAtLocation_Location = FVector(ForceInitToZero);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The Probability of a Vertex Within the Area to get randomly painted between 0 - 1. If for instance 0.1 then there is a 10% chance a vertex within the area will get painted for instance. "))
	float OnlyRandomizeWithinAreaOfEffectAtLocation_ProbabilityFactor = 0.1f;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If True, then the Paint Job will use the Seed passed through here when Randomizing either on Entire Mesh or within Area of Effect. "))
	bool OverrideRandomSeeds = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The Seed to override with. "))
	FRandomStream SeedToOverrideWith;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "No matter if you override the seed or not, this is the actual random seed used for the paint job. Can be used if the Server for instance want to send it to clients so they can paint entire mesh at random vertices with the same seed and get the same result. "))
	FRandomStream RandomSeedsUsedInPaintJob;
};


//-------------------------------------------------------

// Paint On Entire Mesh Settings

USTRUCT(BlueprintType, meta = (DisplayName = "Paint On Entire Mesh Settings"))
struct FRVPDPPaintOnEntireMeshSettings : public FRVPDPApplyColorSetting {

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Settings for Painting on Random Vertices over the Entire Mesh. Useful if you want to paint a mesh as if it's raining on it or something similar.  "))
	FRVPDPPaintOnEntireMeshAtRandomVerticesSettings PaintOnRandomVerticesSettings;
};