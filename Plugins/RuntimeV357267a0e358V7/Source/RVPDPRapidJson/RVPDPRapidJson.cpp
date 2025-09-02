// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#include "RVPDPRapidJson.h"

#define LOCTEXT_NAMESPACE "FRVPDPRapidJsonModule"

void FRVPDPRapidJsonModule::StartupModule()
{
    // This code will execute after your module is loaded into memory

    UE_LOG(LogTemp, Log, TEXT("Runtime Vertex Paint & Detection Rapid Json Module Startup!"));
}

void FRVPDPRapidJsonModule::ShutdownModule()
{
    // This function may be called during shutdown to clean up your module


    UE_LOG(LogTemp, Log, TEXT("Runtime Vertex Paint & Detection Rapid Json Module Shutdown!"));
}


#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FRVPDPRapidJsonModule, RVPDPRapidJson)