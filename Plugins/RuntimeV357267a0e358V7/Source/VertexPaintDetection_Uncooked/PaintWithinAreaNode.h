// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "VertexPaintDetectionOnMeshNode.h"
#include "PaintWithinAreaNode.generated.h"

/**
 * 
 */
UCLASS()
class VERTEXPAINTDETECTION_UNCOOKED_API UPaintWithinAreaNode : public UVertexPaintDetectionOnMeshNode
{
	GENERATED_BODY()
	
public:

	UPaintWithinAreaNode();

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;

	virtual FText GetMenuCategory() const override;
};
