// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 


#include "GetCalculateColorsTasksAmount.h"


#define LOCTEXT_NAMESPACE "RuntimeVertexPaintAndDetection"

//-------------------------------------------------------

// Construct - Should be overriden by children

UGetCalculateColorsTasksAmount::UGetCalculateColorsTasksAmount() {

	FunctionReference.SetFromField<UFunction>(UVertexPaintFunctionLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UVertexPaintFunctionLibrary, GetCalculateColorsPaintTasksAmount_Wrapper)), true);
}


//-------------------------------------------------------

// Get Menu Category

FText UGetCalculateColorsTasksAmount::GetMenuCategory() const {

	return FText::FromString("Runtime Vertex Paint and Detection Plugin|Utilities");
}


//-------------------------------------------------------

// Get Node Title - Should be overriden by children so they can set different title

FText UGetCalculateColorsTasksAmount::GetNodeTitle(ENodeTitleType::Type TitleType) const {

	return LOCTEXT("Get Calculate Colors Paint Tasks Amount", "Get Calculate Colors Paint Tasks Amount");
}

#undef LOCTEXT_NAMESPACE