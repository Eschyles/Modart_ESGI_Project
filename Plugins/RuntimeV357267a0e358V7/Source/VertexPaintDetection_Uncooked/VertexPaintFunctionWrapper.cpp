// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 


#include "VertexPaintFunctionWrapper.h"

#define LOCTEXT_NAMESPACE "RuntimeVertexPaintAndDetection"


//-------------------------------------------------------

// Construct - Should be overriden by children

UVertexPaintFunctionWrapper::UVertexPaintFunctionWrapper() {

	
}


//-------------------------------------------------------

// Expand Node - Should be overriden by children with their specific pin checks

void UVertexPaintFunctionWrapper::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) {

	Super::ExpandNode(CompilerContext, SourceGraph);

}


//-------------------------------------------------------

// Get Menu Category

FText UVertexPaintFunctionWrapper::GetMenuCategory() const {

	return FText::FromString("Runtime Vertex Paint and Detection Plugin");
}


//-------------------------------------------------------

// Notify Input Changed

void UVertexPaintFunctionWrapper::NotifyInputChanged() const {

	if (UBlueprint* BP = GetBlueprint())
	{
		FBlueprintEditorUtils::MarkBlueprintAsModified(BP);
	}

	UEdGraph* Graph = GetGraph();
	Graph->NotifyGraphChanged();
}


//-------------------------------------------------------

// Get Menu Actions

void UVertexPaintFunctionWrapper::GetMenuActions(FBlueprintActionDatabaseRegistrar& InActionRegistrar) const {

	const UClass* ActionKey = GetClass();

	if (InActionRegistrar.IsOpenForRegistration(ActionKey)) {

		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);

		InActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}


#undef LOCTEXT_NAMESPACE