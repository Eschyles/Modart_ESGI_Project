// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "VertexPaintFunctionWrapper.h"
#include "GetColorsDetectionTasksAmount.generated.h"

/**
 * 
 */
UCLASS()
class VERTEXPAINTDETECTION_UNCOOKED_API UGetColorsDetectionTasksAmount : public UVertexPaintFunctionWrapper
{
	GENERATED_BODY()
	
public:

	UGetColorsDetectionTasksAmount();

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;

	virtual FText GetMenuCategory() const override;
};
