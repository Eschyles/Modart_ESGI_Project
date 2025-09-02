// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 


#include "GetPhysMatUsingPhysicsSurface.h"


#define LOCTEXT_NAMESPACE "RuntimeVertexPaintAndDetection"

//-------------------------------------------------------

// Construct - Should be overriden by children

UGetPhysMatUsingPhysicsSurface::UGetPhysMatUsingPhysicsSurface() {

	FunctionReference.SetFromField<UFunction>(UVertexPaintFunctionLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UVertexPaintFunctionLibrary, GetPhysicalMaterialUsingPhysicsSurface_Wrapper)), true);
}


//-------------------------------------------------------

// Get Menu Category

FText UGetPhysMatUsingPhysicsSurface::GetMenuCategory() const {

	return FText::FromString("Runtime Vertex Paint and Detection Plugin|Utilities");
}


//-------------------------------------------------------

// Get Node Title - Should be overriden by children so they can set different title

FText UGetPhysMatUsingPhysicsSurface::GetNodeTitle(ENodeTitleType::Type TitleType) const {

	return LOCTEXT("Get Physical Material Using Physics Surface", "Get Physical Material Using Physics Surface");
}

#undef LOCTEXT_NAMESPACE