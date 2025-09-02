// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "VertexPaintDetectionWithComp.h"
#include "VertexPaintDetectionOnMeshNode.generated.h"

/**
 * 
 */
UCLASS()
class VERTEXPAINTDETECTION_UNCOOKED_API UVertexPaintDetectionOnMeshNode : public UVertexPaintDetectionWithComp
{
	GENERATED_BODY()
	
public:

	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
};
