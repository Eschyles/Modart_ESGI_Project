// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 


#include "GetMeshComponentVertexColorsNode.h"


#define LOCTEXT_NAMESPACE "RuntimeVertexPaintAndDetection"

//-------------------------------------------------------

// Construct - Should be overriden by children

UGetMeshComponentVertexColorsNode::UGetMeshComponentVertexColorsNode() {

	FunctionReference.SetFromField<UFunction>(UVertexPaintFunctionLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UVertexPaintFunctionLibrary, GetMeshComponentVertexColors_Wrapper)), true);
}


//-------------------------------------------------------

// Expand Node - Should be overriden by children with their specific pin checks

void UGetMeshComponentVertexColorsNode::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) {

	Super::ExpandNode(CompilerContext, SourceGraph);


	if (FindPinChecked(TEXT("meshComponent"))->LinkedTo.Num() == 0) {

		CompilerContext.MessageLog.Error(*LOCTEXT("meshComponent Missing Input!", "Get Mesh Component Vertex Colors must have a valid Mesh assigned to meshComponent.").ToString());

		BreakAllNodeLinks();
		return;
	}
}


//-------------------------------------------------------

// Get Menu Category

FText UGetMeshComponentVertexColorsNode::GetMenuCategory() const {

	return FText::FromString("Runtime Vertex Paint and Detection Plugin|Utilities");
}


//-------------------------------------------------------

// Get Node Title - Should be overriden by children so they can set different title

FText UGetMeshComponentVertexColorsNode::GetNodeTitle(ENodeTitleType::Type TitleType) const {

	return LOCTEXT("Get Mesh Component Vertex Colors", "Get Mesh Component Vertex Colors");
}

#undef LOCTEXT_NAMESPACE