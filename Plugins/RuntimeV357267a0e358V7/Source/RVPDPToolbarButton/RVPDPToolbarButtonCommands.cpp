// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#include "RVPDPToolbarButtonCommands.h"

#define LOCTEXT_NAMESPACE "FRVPDPToolbarButtonModule"

void FRVPDPToolbarButtonCommands::RegisterCommands()
{
#if ENGINE_MAJOR_VERSION == 4

	UI_COMMAND(PluginAction, "Runtime Vertex \nPaint & Detection", "Open/Closes the Runtime Vertex Paint & Detection Plugin Editor Widget", EUserInterfaceActionType::Button, FInputGesture());

#elif ENGINE_MAJOR_VERSION == 5

	UI_COMMAND(PluginAction, "Runtime Vertex \nPaint & Detection", "Open/Closes the Runtime Vertex Paint & Detection Plugin Editor Widget", EUserInterfaceActionType::Button, FInputChord());

#endif
}

#undef LOCTEXT_NAMESPACE
