// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 


#include "GetColorsWithinAreaNode.h"


#define LOCTEXT_NAMESPACE "RuntimeVertexPaintAndDetection"

//-------------------------------------------------------

// Construct - Should be overriden by children

UGetColorsWithinAreaNode::UGetColorsWithinAreaNode() {

	FunctionReference.SetFromField<UFunction>(UVertexPaintTasksFunctionLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UVertexPaintTasksFunctionLibrary, GetColorsWithinArea_Wrapper)), true);
}


//-------------------------------------------------------

// Get Menu Category

FText UGetColorsWithinAreaNode::GetMenuCategory() const {

	return FText::FromString("Runtime Vertex Paint and Detection Plugin|Detection");
}


//-------------------------------------------------------

// Get Node Title - Should be overriden by children so they can set different title

FText UGetColorsWithinAreaNode::GetNodeTitle(ENodeTitleType::Type TitleType) const {

	return LOCTEXT("Get Colors Within Area", "Get Colors Within Area");
}

#undef LOCTEXT_NAMESPACE