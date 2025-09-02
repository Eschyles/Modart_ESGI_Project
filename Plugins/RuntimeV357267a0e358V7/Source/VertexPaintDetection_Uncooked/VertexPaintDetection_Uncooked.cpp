// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#include "VertexPaintDetection_Uncooked.h"

#define LOCTEXT_NAMESPACE "FVertexPaintDetection_UncookedModule"

void FVertexPaintDetection_UncookedModule::StartupModule()
{
	UE_LOG(LogTemp, Log, TEXT("Runtime Vertex Paint and Detection Plugin - Uncooked Module Startup!"));
}

void FVertexPaintDetection_UncookedModule::ShutdownModule()
{
	UE_LOG(LogTemp, Log, TEXT("Runtime Vertex Paint and Detection Plugin - Uncooked Module Shutdown!"));
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FVertexPaintDetection_UncookedModule, VertexPaintDetection_Uncooked)