// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 


#include "SetMeshComponentColorsSerialized.h"

#define LOCTEXT_NAMESPACE "RuntimeVertexPaintAndDetection"


//-------------------------------------------------------

// Construct - Should be overriden by children

USetMeshComponentColorsSerialized::USetMeshComponentColorsSerialized() {

	FunctionReference.SetFromField<UFunction>(UVertexPaintTasksFunctionLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UVertexPaintTasksFunctionLibrary, SetMeshComponentVertexColorsUsingSerializedString_Wrapper)), true);
}


//-------------------------------------------------------

// Expand Node - Should be overriden by children with their specific pin checks

void USetMeshComponentColorsSerialized::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) {

	Super::ExpandNode(CompilerContext, SourceGraph);


	if (FindPinChecked(TEXT("meshComponent"))->LinkedTo.Num() == 0) {

		CompilerContext.MessageLog.Error(*LOCTEXT("meshComponent Missing Input!", "Set Mesh Component Vertex Colors must have a valid Static or Skeletal Mesh assigned to meshComponent.").ToString());

		BreakAllNodeLinks();
		return;
	}
}


//-------------------------------------------------------

// Get Menu Category

FText USetMeshComponentColorsSerialized::GetMenuCategory() const {

	return FText::FromString("Runtime Vertex Paint and Detection Plugin|Paint");
}


//-------------------------------------------------------

// Get Node Title - Should be overriden by children so they can set different title

FText USetMeshComponentColorsSerialized::GetNodeTitle(ENodeTitleType::Type TitleType) const {

	return LOCTEXT("Set Mesh Component Vertex Colors Using Serialized String", "Set Mesh Component Vertex Colors Using Serialized String");
}

#undef LOCTEXT_NAMESPACE