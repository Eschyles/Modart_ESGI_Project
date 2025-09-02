// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "VertexPaintDetectionOnMeshNode.h"
#include "PaintOnEntireMeshNode.generated.h"

/**
 * 
 */
UCLASS()
class VERTEXPAINTDETECTION_UNCOOKED_API UPaintOnEntireMeshNode : public UVertexPaintDetectionOnMeshNode
{
	GENERATED_BODY()

public:

	UPaintOnEntireMeshNode();

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;

	virtual FText GetMenuCategory() const override;
};
