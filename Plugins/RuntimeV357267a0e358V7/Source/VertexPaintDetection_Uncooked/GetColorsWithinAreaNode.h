// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "VertexPaintDetectionOnMeshNode.h"
#include "GetColorsWithinAreaNode.generated.h"

/**
 * 
 */
UCLASS()
class VERTEXPAINTDETECTION_UNCOOKED_API UGetColorsWithinAreaNode : public UVertexPaintDetectionOnMeshNode
{
	GENERATED_BODY()
		
public:

	UGetColorsWithinAreaNode();

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;

	virtual FText GetMenuCategory() const override;
};
