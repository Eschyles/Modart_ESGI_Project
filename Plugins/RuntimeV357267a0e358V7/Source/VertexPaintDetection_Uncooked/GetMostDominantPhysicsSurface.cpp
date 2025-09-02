// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 


#include "GetMostDominantPhysicsSurface.h"


#define LOCTEXT_NAMESPACE "RuntimeVertexPaintAndDetection"

//-------------------------------------------------------

// Construct - Should be overriden by children

UGetMostDominantPhysicsSurface::UGetMostDominantPhysicsSurface() {

	FunctionReference.SetFromField<UFunction>(UVertexPaintFunctionLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UVertexPaintFunctionLibrary, GetTheMostDominantPhysicsSurface_Wrapper)), true);
}


//-------------------------------------------------------

// Get Menu Category

FText UGetMostDominantPhysicsSurface::GetMenuCategory() const {

	return FText::FromString("Runtime Vertex Paint and Detection Plugin|Utilities");
}


//-------------------------------------------------------

// Get Node Title - Should be overriden by children so they can set different title

FText UGetMostDominantPhysicsSurface::GetNodeTitle(ENodeTitleType::Type TitleType) const {

	return LOCTEXT("Get Most Dominant Physics Surface", "Get Most Dominant Physics Surface");
}

#undef LOCTEXT_NAMESPACE