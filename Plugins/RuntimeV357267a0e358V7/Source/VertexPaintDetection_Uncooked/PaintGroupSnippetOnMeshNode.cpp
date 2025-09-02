// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 


#include "PaintGroupSnippetOnMeshNode.h"


#define LOCTEXT_NAMESPACE "RuntimeVertexPaintAndDetection"

//-------------------------------------------------------

// Construct - Should be overriden by children

UPaintGroupSnippetOnMeshNode::UPaintGroupSnippetOnMeshNode() {

	FunctionReference.SetFromField<UFunction>(UVertexPaintTasksFunctionLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UVertexPaintTasksFunctionLibrary, PaintGroupSnippetOnMesh_Wrapper)), true);
}


//-------------------------------------------------------

// Expand Node - Should be overriden by children with their specific pin checks
// Got crash and had issue with checking if groupSnippetMeshes was > 0. TODO return to this later when we have time. 
void UPaintGroupSnippetOnMeshNode::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) {

	Super::ExpandNode(CompilerContext, SourceGraph);

	if (FindPinChecked(TEXT("groupSnippetMeshes"))->LinkedTo.Num() == 0) {

		CompilerContext.MessageLog.Error(*LOCTEXT("No Group Snippet Meshes Connected!", "Should be an Array of Minimum 2 Meshes Connected since it is the minimum requirement to Create a Group Snippet. ").ToString());

		BreakAllNodeLinks();
		return;
	}
}

//-------------------------------------------------------

// Get Menu Category

FText UPaintGroupSnippetOnMeshNode::GetMenuCategory() const {

	return FText::FromString("Runtime Vertex Paint and Detection Plugin|Paint");
}


//-------------------------------------------------------

// Get Node Title - Should be overriden by children so they can set different title

FText UPaintGroupSnippetOnMeshNode::GetNodeTitle(ENodeTitleType::Type TitleType) const {

	return LOCTEXT("Paint Group Snippet on Mesh", "Paint Group Snippet on Mesh");
}

#undef LOCTEXT_NAMESPACE