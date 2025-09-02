// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 


#include "GetColorsDetectionTasksAmount.h"


#define LOCTEXT_NAMESPACE "RuntimeVertexPaintAndDetection"

//-------------------------------------------------------

// Construct - Should be overriden by children

UGetColorsDetectionTasksAmount::UGetColorsDetectionTasksAmount() {

	FunctionReference.SetFromField<UFunction>(UVertexPaintFunctionLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UVertexPaintFunctionLibrary, GetCalculateColorsDetectionTasksAmount_Wrapper)), true);
}


//-------------------------------------------------------

// Get Menu Category

FText UGetColorsDetectionTasksAmount::GetMenuCategory() const {

	return FText::FromString("Runtime Vertex Paint and Detection Plugin|Utilities");
}


//-------------------------------------------------------

// Get Node Title - Should be overriden by children so they can set different title

FText UGetColorsDetectionTasksAmount::GetNodeTitle(ENodeTitleType::Type TitleType) const {

	return LOCTEXT("Get Calculate Colors Detection Tasks Amount", "Get Calculate Colors Detection Tasks Amount");
}

#undef LOCTEXT_NAMESPACE