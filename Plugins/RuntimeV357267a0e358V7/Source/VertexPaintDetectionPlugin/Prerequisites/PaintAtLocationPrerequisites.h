// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "PaintFundementalsPrerequisites.h"
#include "HitLocationPrerequisites.h"
#include "FallOffPrerequisites.h"
#include "PaintAtLocationPrerequisites.generated.h" 


//-------------------------------------------------------

// Paint At Location Falloff Type

UENUM(BlueprintType)
enum class EVertexPaintAtLocationFallOffType : uint8 {

	OutwardFallOff = 0 UMETA(DisplayName = "Outward Fall Off", Tooltip = "Fall Off Outwardly, Gets Weaker Toward the Max AoE and Stronger toward the Min AoE, from distanceToBaseFallOffFrom. This will most likely be the most common in many cases. "),
	InwardFallOff = 1 UMETA(DisplayName = "Inward Fall Off", Tooltip = "Fall Off Inwardly, Gets Weaker Toward the Min AoE and Stronger toward the Max AoE, from distanceToBaseFallOffFrom. "),
	SphericalFallOff = 2 UMETA(DisplayName = "Spherical Fall Off", Tooltip = "Fall Offs evenly In and Outwards to all directions. For instance if you change the distanceToBaseFallOffFrom to be Between the Min and Max AoE, then it will be stronger there, and weaker out toward Min and Max range. "),
};


//-------------------------------------------------------

// Paint At Location Fall Off Settings

USTRUCT(BlueprintType, meta = (DisplayName = "Paint At Location Fall Off Settings"))
struct FRVPDPPaintAtLocationFallOffSettings : public FRVPDPVertexPaintFallOffSettings {

	GENERATED_BODY()


	FRVPDPPaintAtLocationFallOffSettings() {

		// For Paint at Location we want this to be 1 by default as it's the expected behaviour
		FallOffStrength = 1;
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Which type of FallOff we should use. The Default is the most common use case. Hover over each of them to get more info of what they do. "))
	EVertexPaintAtLocationFallOffType PaintAtLocationFallOffType = EVertexPaintAtLocationFallOffType::OutwardFallOff;
};


//-------------------------------------------------------

// Paint At Location Area Of Effect Settings

USTRUCT(BlueprintType, meta = (DisplayName = "Paint At Location Area Of Effect Settings"))
struct FRVPDPPaintAtLocationAreaOfEffectSettings {

	GENERATED_BODY()


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Area of Effect Range to Start at. Doesn't have to be 0, can for example be 100 if the max is for example 200 if you want to paint a Hollow circle. "))
	float AreaOfEffectRangeStart = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Area of Effect Range to End at"))
	float AreaOfEffectRangeEnd = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Falloff Settings such as strength, distance to base it on, and type of FallOff we should use. The Default is the most common use case. Hover over each of them to get more info of what they do. "))
	FRVPDPPaintAtLocationFallOffSettings FallOffSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Can get the Average Color in an Area Range, which could be useful if you have for instance a Make-up game and want to see if the player has painted the majority of a characters cheek or something. Area Range has to be above 0 to get the average in that range. "))
	FRVPDPGetAverageColorSettings GetAverageColorAfterApplyingColorSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If meshComponent is a Skeletal Mesh Component and is Registered in the Optimization Data Asset through the Plugins Editor Widget, and nothing in the Callback Settings requires us to loop through all vertices such include amount of colors of each channel, colors, positions or normals, then if this is set to something we will only loop through the vertices on those Bones which may make the task finish much faster. We will also include child and parent bones if they had no collision, since we can't get a hit on those parents/childs with a trace, so if for instance tracing and hitting a hand_r, then the fingers will be included as well. "))
	TArray<FName> SpecificBonesToPaint;

	float PaintAtLocationScaleFallOffFrom = 0;
	float PaintAtLocationFallOffMaxRange = 0;
};


//-------------------------------------------------------

// Get Closest Vertex Data Combo Paint At Location Settings

USTRUCT(BlueprintType, meta = (DisplayName = "Get Closest Vertex Data Combo Paint At Location Settings"))
struct FRVPDPGetClosestVertexDataComboPaintAtLocationSettings {

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If we run Get Closest Vertex Data on Mesh and it's Event Before Painting. Useful if you for instance want to Run SFX based on the colors before we applied the Paint Job, like if a Rain Drop landed on a Dry Cap, so it's SFX could be that of the Dry Cap and not the Wet Cap, since it wasn't wet before the rain drop hit, but it got wet when it did. "))
	bool RunGetClosestVertexDataOnMeshBeforeApplyingPaint = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Can get the Average Color in an Area Range, which could be useful if you have for instance a Make-up game and want to see if the player has painted the majority of a characters cheek. Area Range has to be above 0 to get the average in that range. "))
	FRVPDPGetAverageColorSettings GetAverageColorBeforeApplyingColorSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If false it will use the Paint at Location Hit Fundementals. If true will use the detect fundementals passed through here. "))
	bool UseCustomHitSettings = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "By having a seperate detect hit, you can Paint on one location, but Detect on another location, within the same job. For instance if you have something dripping on a wall, and you want to detect ahead of where you want to paint, and depending on the result shrink/enlarge the drip. "))
	FRVPDPAtLocationTasksSettings CustomHitSettings;
};


//-------------------------------------------------------

// Paint At Location Settings

USTRUCT(BlueprintType, meta = (DisplayName = "Paint At Location Settings"))
struct FRVPDPPaintAtLocationSettings : public FRVPDPApplyColorSetting {

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	FRVPDPAtLocationTasksSettings HitFundementals;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Area of Effect of the Paint Job"))
	FRVPDPPaintAtLocationAreaOfEffectSettings PaintAtAreaSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "This will Return the color as close to the actual Hit Location as possible, which is very useful if you have Meshes with few vertices and you want to run SFX or VFX based on the Color of the Hit location and not the closest vertex color in case there is a diff, which it might have if the mesh have few vertices. \nThe task may take a bit longer to calculate if Mesh has alot of vertices, so you have the option to only get it if the Mesh has a Max Amount of Vertices. "))
	FRVPDPEstimatedColorAtHitLocationSettings GetEstimatedColorAtHitLocationSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If we should run Get Closest Vertex Data before and/or after Painting, useful if you for instance want to Run SFX before Painting, like if a Rain Drop landed on a Dry Cap, then the SFX of that should be that of a Dry Cap, then it should Paint it Wet. You can even run the Get Closest Vertex Data at a different hit location if you wish, so for instance if you have something dripping on a wall, and you want to detect ahead of where you want to paint, and depending on the result shrink/enlarge the drip.  "))
	FRVPDPGetClosestVertexDataComboPaintAtLocationSettings GetClosestVertexDataCombo;
};