// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "VertexPaintFunctionWrapper.h"
#include "GetMeshVertexColorsAtLODNode.generated.h"

/**
 * 
 */
UCLASS()
class VERTEXPAINTDETECTION_UNCOOKED_API UGetMeshVertexColorsAtLODNode : public UVertexPaintFunctionWrapper
{
	GENERATED_BODY()

public:

	UGetMeshVertexColorsAtLODNode();

	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;

	virtual FText GetMenuCategory() const override;
};
