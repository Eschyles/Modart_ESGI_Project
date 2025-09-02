// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "VertexPaintDetectionOnMeshNode.h"
#include "GetClosestVertexDataOnMeshNode.generated.h"

/**
 * 
 */
UCLASS()
class VERTEXPAINTDETECTION_UNCOOKED_API UGetClosestVertexDataOnMeshNode : public UVertexPaintDetectionOnMeshNode
{
	GENERATED_BODY()
	
public:

	UGetClosestVertexDataOnMeshNode();

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;

	virtual FText GetMenuCategory() const override;
};
