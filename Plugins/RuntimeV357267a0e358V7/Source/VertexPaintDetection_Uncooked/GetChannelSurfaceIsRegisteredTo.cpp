// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 


#include "GetChannelSurfaceIsRegisteredTo.h"


#define LOCTEXT_NAMESPACE "RuntimeVertexPaintAndDetection"

//-------------------------------------------------------

// Construct - Should be overriden by children

UGetChannelSurfaceIsRegisteredTo::UGetChannelSurfaceIsRegisteredTo() {

	FunctionReference.SetFromField<UFunction>(UVertexPaintFunctionLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UVertexPaintFunctionLibrary, GetAllVertexColorChannelsPhysicsSurfaceIsRegisteredTo_Wrapper)), true);
}


//-------------------------------------------------------

// Expand Node - Should be overriden by children with their specific pin checks

void UGetChannelSurfaceIsRegisteredTo::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) {

	Super::ExpandNode(CompilerContext, SourceGraph);


	if (FindPinChecked(TEXT("material"))->LinkedTo.Num() == 0) {

		CompilerContext.MessageLog.Error(*LOCTEXT("material Missing Input!", "Must set a Valid Material").ToString());

		BreakAllNodeLinks();
		return;
	}
}


//-------------------------------------------------------

// Get Menu Category

FText UGetChannelSurfaceIsRegisteredTo::GetMenuCategory() const {

	return FText::FromString("Runtime Vertex Paint and Detection Plugin|Utilities");
}


//-------------------------------------------------------

// Get Node Title - Should be overriden by children so they can set different title

FText UGetChannelSurfaceIsRegisteredTo::GetNodeTitle(ENodeTitleType::Type TitleType) const {

	return LOCTEXT("Get All Vertex Color Channels Physics Surface Is Registered To", "Get All Vertex Color Channels Physics Surface Is Registered To");
}

#undef LOCTEXT_NAMESPACE