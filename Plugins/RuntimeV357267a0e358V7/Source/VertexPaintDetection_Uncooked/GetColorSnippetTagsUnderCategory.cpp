// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 


#include "GetColorSnippetTagsUnderCategory.h"



#define LOCTEXT_NAMESPACE "RuntimeVertexPaintAndDetection"


//-------------------------------------------------------

// Construct - Should be overriden by children

UGetColorSnippetTagsUnderCategory::UGetColorSnippetTagsUnderCategory() {

	FunctionReference.SetFromField<UFunction>(UVertexPaintFunctionLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UVertexPaintFunctionLibrary, GetAllMeshColorSnippetsTagsUnderTagCategory_Wrapper)), true);
}


//-------------------------------------------------------

// Expand Node - Should be overriden by children with their specific pin checks

void UGetColorSnippetTagsUnderCategory::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) {

	Super::ExpandNode(CompilerContext, SourceGraph);


	if (FindPinChecked(TEXT("meshComponent"))->LinkedTo.Num() == 0) {

		CompilerContext.MessageLog.Error(*LOCTEXT("meshComponent Missing Input!", "Must have a valid Static or Skeletal Mesh assigned to meshComponent.").ToString());

		BreakAllNodeLinks();
		return;
	}
}


//-------------------------------------------------------

// Get Menu Category

FText UGetColorSnippetTagsUnderCategory::GetMenuCategory() const {

	return FText::FromString("Runtime Vertex Paint and Detection Plugin|Color Snippets");
}


//-------------------------------------------------------

// Get Node Title - Should be overriden by children so they can set different title

FText UGetColorSnippetTagsUnderCategory::GetNodeTitle(ENodeTitleType::Type TitleType) const {

	return LOCTEXT("Get All Mesh Color Snippets Tags Under Tag Category", "Get All Mesh Color Snippets Tags Under Tag Category");
}

#undef LOCTEXT_NAMESPACE