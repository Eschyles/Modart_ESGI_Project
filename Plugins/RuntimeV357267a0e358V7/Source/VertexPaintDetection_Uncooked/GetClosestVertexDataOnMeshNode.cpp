// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 


#include "GetClosestVertexDataOnMeshNode.h"


#define LOCTEXT_NAMESPACE "RuntimeVertexPaintAndDetection"

//-------------------------------------------------------

// Construct - Should be overriden by children

UGetClosestVertexDataOnMeshNode::UGetClosestVertexDataOnMeshNode() {

	FunctionReference.SetFromField<UFunction>(UVertexPaintTasksFunctionLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UVertexPaintTasksFunctionLibrary, GetClosestVertexDataOnMesh_Wrapper)), true);
}


//-------------------------------------------------------

// Get Menu Category

FText UGetClosestVertexDataOnMeshNode::GetMenuCategory() const {

	return FText::FromString("Runtime Vertex Paint and Detection Plugin|Detection");
}


//-------------------------------------------------------

// Get Node Title - Should be overriden by children so they can set different title

FText UGetClosestVertexDataOnMeshNode::GetNodeTitle(ENodeTitleType::Type TitleType) const {

	return LOCTEXT("Get Closest Vertex Data on Mesh", "Get Closest Vertex Data on Mesh");
}

#undef LOCTEXT_NAMESPACE