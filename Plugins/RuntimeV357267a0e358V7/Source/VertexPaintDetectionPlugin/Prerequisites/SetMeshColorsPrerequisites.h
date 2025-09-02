// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "PaintFundementalsPrerequisites.h"
#include "SetMeshColorsPrerequisites.generated.h" 


//-------------------------------------------------------

// Set Vertex Colors Settings

USTRUCT(BlueprintType, meta = (DisplayName = "Set Vertex Colors Settings"))
struct FRVPDPSetVertexColorsSettings : public FRVPDPPaintDirectlyTaskSettings {

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The Vertex Colors to Set at LOD0. "))
	TArray<FColor> VertexColorsAtLOD0ToSet;
};