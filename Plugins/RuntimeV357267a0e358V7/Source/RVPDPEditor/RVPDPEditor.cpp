// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#include "RVPDPEditor.h"

#define LOCTEXT_NAMESPACE "FRVPDPEditor"

void FRVPDPEditor::StartupModule()
{
	UE_LOG(LogTemp, Log, TEXT("Runtime Vertex Paint and Detection Plugin - Editor Module Startup!"));
}

void FRVPDPEditor::ShutdownModule()
{
	UE_LOG(LogTemp, Log, TEXT("Runtime Vertex Paint and Detection Plugin - Editor Module Shutdown!"));
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FRVPDPEditor, RVPDPEditor)