// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 


#include "GetAllVertexColorsOnlyNode.h"


#define LOCTEXT_NAMESPACE "RuntimeVertexPaintAndDetection"

//-------------------------------------------------------

// Construct - Should be overriden by children

UGetAllVertexColorsOnlyNode::UGetAllVertexColorsOnlyNode() {

	FunctionReference.SetFromField<UFunction>(UVertexPaintTasksFunctionLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UVertexPaintTasksFunctionLibrary, GetAllVertexColorsOnly_Wrapper)), true);
}


//-------------------------------------------------------

// Get Menu Category

FText UGetAllVertexColorsOnlyNode::GetMenuCategory() const {

	return FText::FromString("Runtime Vertex Paint and Detection Plugin|Detection");
}


//-------------------------------------------------------

// Get Node Title - Should be overriden by children so they can set different title

FText UGetAllVertexColorsOnlyNode::GetNodeTitle(ENodeTitleType::Type TitleType) const {

	return LOCTEXT("Get All Vertex Colors Only", "Get All Vertex Colors Only");
}

#undef LOCTEXT_NAMESPACE