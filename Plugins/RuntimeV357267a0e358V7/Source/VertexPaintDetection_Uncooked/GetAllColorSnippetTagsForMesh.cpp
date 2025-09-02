// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 


#include "GetAllColorSnippetTagsForMesh.h"


#define LOCTEXT_NAMESPACE "RuntimeVertexPaintAndDetection"

//-------------------------------------------------------

// Construct - Should be overriden by children

UGetAllColorSnippetTagsForMesh::UGetAllColorSnippetTagsForMesh() {

	FunctionReference.SetFromField<UFunction>(UVertexPaintFunctionLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UVertexPaintFunctionLibrary, GetAllMeshColorSnippetsAsTags_Wrapper)), true);
}


//-------------------------------------------------------

// Expand Node - Should be overriden by children with their specific pin checks

void UGetAllColorSnippetTagsForMesh::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) {

	Super::ExpandNode(CompilerContext, SourceGraph);


	if (FindPinChecked(TEXT("meshComponent"))->LinkedTo.Num() == 0) {

		CompilerContext.MessageLog.Error(*LOCTEXT("meshComponent Missing Input!", "Must have a valid Static or Skeletal Mesh assigned to meshComponent.").ToString());

		BreakAllNodeLinks();
		return;
	}
}


//-------------------------------------------------------

// Get Menu Category

FText UGetAllColorSnippetTagsForMesh::GetMenuCategory() const {

	return FText::FromString("Runtime Vertex Paint and Detection Plugin|Color Snippets");
}


//-------------------------------------------------------

// Get Node Title - Should be overriden by children so they can set different title

FText UGetAllColorSnippetTagsForMesh::GetNodeTitle(ENodeTitleType::Type TitleType) const {

	return LOCTEXT("Get All Mesh Color Snippets As Tags", "Get All Mesh Color Snippets As Tags");
}

#undef LOCTEXT_NAMESPACE