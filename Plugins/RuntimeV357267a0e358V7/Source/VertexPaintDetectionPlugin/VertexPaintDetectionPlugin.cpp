// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#include "VertexPaintDetectionPlugin.h"
#include "VertexPaintDetectionLog.h"

#define LOCTEXT_NAMESPACE "FVertexPaintDetectionPluginModule"


//-------------------------------
// Startup Module

void FVertexPaintDetectionPluginModule::StartupModule() {

	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	RVPDP_LOG(FRVPDPDebugSettings(false, 0, true), FColor::Cyan, "Vertex Paint Detection Module Startup!");
}

//-------------------------------
// Shutdown Module

void FVertexPaintDetectionPluginModule::ShutdownModule() {

	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	RVPDP_LOG(FRVPDPDebugSettings(false, 0, true), FColor::Cyan, "Vertex Paint Detection Module Shutdown!");
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FVertexPaintDetectionPluginModule, VertexPaintDetectionPlugin)