// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "VertexPaintFunctionWrapper.h"
#include "MultiCapsuleTraceForUniqueMeshes.generated.h"

/**
 * 
 */
UCLASS()
class VERTEXPAINTDETECTION_UNCOOKED_API UMultiCapsuleTraceForUniqueMeshes : public UVertexPaintFunctionWrapper
{
	GENERATED_BODY()
	
public:

	UMultiCapsuleTraceForUniqueMeshes();

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;

	virtual FText GetMenuCategory() const override;
};
