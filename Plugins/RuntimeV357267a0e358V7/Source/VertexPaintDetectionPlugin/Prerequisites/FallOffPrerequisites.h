// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "FallOffPrerequisites.generated.h" 



//-------------------------------------------------------

// Start Fall Off Distance From Setting

UENUM(BlueprintType, meta = (DisplayName = "Start Fall Off Distance From Setting"))
enum class FRVPDPStartFallOffDistanceFromSetting : uint8 {

	BaseOfPaintShape = 0 UMETA(DisplayName = "Base Of Paint Shape", Tooltip = "This will base the Distance to Start FallOff From from the very Base off the Shape, for instance if Paint Within Area on a Cube/Rectangle Shpae then this will be from the bottom of the area. If paint at location then from the Center. "),
	EndOfPaintShape = 1 UMETA(DisplayName = "End Of Paint Shape", Tooltip = "This will base the Distance to Start FallOff From from the very End off the Shape, for instance if Paint Within Area on a Cube/Rectangle then this will be from the Top. If paint at location then from the Edge of the Area of Effect. Useful for paint within area things like Lakes in combination with extra extent, where it will start the falloff at the top edge of the lake and go up toward the extra extent. ")
};


//-------------------------------------------------------

// Vertex Paint Fall Off Settings

USTRUCT(BlueprintType, meta = (DisplayName = "Vertex Paint Fall Off Settings"))
struct FRVPDPVertexPaintFallOffSettings {

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	float FallOffStrength = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "How we should base the Distance to Start the Fall Off From. For instance if Paint Within Area with Box, if we should start from the Base, i.e. the Bottom, or the End, i.e. the Top, or Sphere Shape from the Centre with Base or the Outer Edge with End. \n\nVery useful in instances like a Lake, where no matter the size of it you always want to start the falloff from the top. "))
	FRVPDPStartFallOffDistanceFromSetting StartFallOffDistanceFrom = FRVPDPStartFallOffDistanceFromSetting::BaseOfPaintShape;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The distance from either the Base or the End of the Area to Start Fading in the Falloff. For example if Paint Within Area with Box, and it's set to EndOfFFallOffType, then this will be the distance downward from the Top where the fading will start. If it's set to be the Base, then the Distance upwards from the bottom. If Paint at Location with Base then will base it of the centre, if End then from the outer Edge. \n\nDepending on the FallOff type it has different use cases, for instance for Lakes in combination with Extra Extent then the End will be useful as the falloff will start at the Top. "))
	float DistanceToStartFallOffFrom = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Limits the Color Strength on the Channels we're trying to Apply, if not already over the limit, so we can always get a nice even falloff even though you keep painting. If Painting Using Physics Surface, then will limit the channels that has the physics surface, otherwise will limit the ones set below. "))
	bool LimitFallOffColor = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The Max Color between 0-1 at the very edge of the Falloff this paint job can reach. We start lerping toward this limit where the falloff starts, so you will always have a smooth falloff even though you continually run paint jobs with Add. Very useful for stuff like Lakes with some Extra Extent where if set to start the falloff at the End, so you always want a nice falloff at the top of the lake toward the extra extent and not a hard stop. "))
	float ColorLimitAtFallOffEdge = 0;
};