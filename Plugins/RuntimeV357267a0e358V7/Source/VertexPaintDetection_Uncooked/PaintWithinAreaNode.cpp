// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 


#include "PaintWithinAreaNode.h"


#define LOCTEXT_NAMESPACE "RuntimeVertexPaintAndDetection"


//-------------------------------------------------------

// Construct - Should be overriden by children

UPaintWithinAreaNode::UPaintWithinAreaNode() {

	FunctionReference.SetFromField<UFunction>(UVertexPaintTasksFunctionLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UVertexPaintTasksFunctionLibrary, PaintOnMeshWithinArea_Wrapper)), true);
}


//-------------------------------------------------------

// Get Menu Category

FText UPaintWithinAreaNode::GetMenuCategory() const {

	return FText::FromString("Runtime Vertex Paint and Detection Plugin|Paint");
}


//-------------------------------------------------------

// Get Node Title - Should be overriden by children so they can set different title

FText UPaintWithinAreaNode::GetNodeTitle(ENodeTitleType::Type TitleType) const {

	return LOCTEXT("Paint on Mesh Within Area", "Paint on Mesh Within Area");
}

#undef LOCTEXT_NAMESPACE