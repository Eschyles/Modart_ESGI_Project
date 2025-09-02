// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 


#include "GetAllColorSnippetsForMeshNode.h"


#define LOCTEXT_NAMESPACE "RuntimeVertexPaintAndDetection"

//-------------------------------------------------------

// Construct - Should be overriden by children

UGetAllColorSnippetsForMeshNode::UGetAllColorSnippetsForMeshNode() {

	FunctionReference.SetFromField<UFunction>(UVertexPaintFunctionLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UVertexPaintFunctionLibrary, GetAllMeshColorSnippetsAsString_Wrapper)), true);
}


//-------------------------------------------------------

// Get Menu Category

FText UGetAllColorSnippetsForMeshNode::GetMenuCategory() const {

	return FText::FromString("Runtime Vertex Paint and Detection Plugin|Color Snippets");
}


//-------------------------------------------------------

// Get Node Title - Should be overriden by children so they can set different title

FText UGetAllColorSnippetsForMeshNode::GetNodeTitle(ENodeTitleType::Type TitleType) const {

	return LOCTEXT("Get All Mesh Color Snippets As Strings", "Get All Mesh Color Snippets As Strings");
}

#undef LOCTEXT_NAMESPACE