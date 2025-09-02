// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "K2Node_CallFunction.h"

#include "KismetCompiler.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"

#include "VertexPaintDetectionPlugin.h"
#include "VertexPaintTasksFunctionLibrary.h"
#include "VertexPaintFunctionLibrary.h"

#include "VertexPaintFunctionWrapper.generated.h"

/**
 * 
 */
UCLASS()
class VERTEXPAINTDETECTION_UNCOOKED_API UVertexPaintFunctionWrapper : public UK2Node_CallFunction {

	GENERATED_BODY()
	
public:

	UVertexPaintFunctionWrapper();

	virtual FText GetMenuCategory() const override;

	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& InActionRegistrar) const override;

	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;

protected:

	void NotifyInputChanged() const;
};
