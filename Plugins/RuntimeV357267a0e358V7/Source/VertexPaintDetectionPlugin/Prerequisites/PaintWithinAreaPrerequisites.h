// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "PaintFundementalsPrerequisites.h"
#include "WithinAreaPrerequisites.h"
#include "PaintWithinAreaPrerequisites.generated.h" 


//-------------------------------------------------------

// Paint Settings Outside Of Area

USTRUCT(BlueprintType, meta = (DisplayName = "Paint Settings Outside Of Area"))
struct FRVPDPPaintSettingsOutsideOfArea {

	GENERATED_BODY()

	bool IsGoingToApplyAnyColorOutsideOfArea() const {

		return RedVertexColorToApply != 0 || GreenVertexColorToApply != 0 || BlueVertexColorToApply != 0 || AlphaVertexColorToApply != 0;
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Should be between -1 and 1. Temporary solution so you can Add colors outside of the Area. A proper one that uses the same apply vertex color setting as the regular with paint conditions, paint using physics surface etc. will be added in the next patch!"))
	float RedVertexColorToApply = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Should be between -1 and 1. Temporary solution so you can Add colors outside of the Area. A proper one that uses the same apply vertex color setting as the regular with paint conditions, paint using physics surface etc. will be added in the next patch!"))
	float GreenVertexColorToApply = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Should be between -1 and 1. Temporary solution so you can Add colors outside of the Area. A proper one that uses the same apply vertex color setting as the regular with paint conditions, paint using physics surface etc. will be added in the next patch!"))
	float BlueVertexColorToApply = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Should be between -1 and 1. Temporary solution so you can Add colors outside of the Area. A proper one that uses the same apply vertex color setting as the regular with paint conditions, paint using physics surface etc. will be added in the next patch!"))
	float AlphaVertexColorToApply = 0;
};


//-------------------------------------------------------

// Paint Within Area Settings

USTRUCT(BlueprintType, meta = (DisplayName = "Paint Within Area Settings"))
struct FRVPDPPaintWithinAreaSettings : public FRVPDPApplyColorSetting {

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The Within Area Settings such as the Primitive Components to check if within, for example a Cube, Rectangle, Sphere or even a Skeletal Mesh if you opt for Complex Shape, and if we should return any vertex data of the vertices that's within the area, and the colors of each channel info like average color, % etc. "))
	FRVPDPWithinAreaSettings WithinAreaSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Runs Get Colors Within Area as well, which can get the colors, amount of colors of each channel and % etc. within the area Before applying the colors from this paint within area if set to do so. Useful if you want to see how much of a difference the Paint Within Area did paint job did. "))
	bool GetColorsWithinAreaCombo = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Temporary solution so you can Add colors outside of the Area. A proper one that uses the same apply vertex color setting as the regular with paint conditions, paint using physics surface etc. will be added in the next patch!"))
	FRVPDPPaintSettingsOutsideOfArea ColorToApplyToVerticesOutsideOfArea = FRVPDPPaintSettingsOutsideOfArea();
};