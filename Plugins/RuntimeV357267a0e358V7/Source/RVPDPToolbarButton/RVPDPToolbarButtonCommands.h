// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "RVPDPToolbarButtonStyle.h"

class FRVPDPToolbarButtonCommands : public TCommands<FRVPDPToolbarButtonCommands>
{
public:

	FRVPDPToolbarButtonCommands()
		: TCommands<FRVPDPToolbarButtonCommands>(TEXT("RVPDPToolbarButton"), NSLOCTEXT("Contexts", "RVPDPToolbarButton", "Runtime Vertex Paint & Detection Plugin"), NAME_None, FRVPDPToolbarButtonStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > PluginAction;
};
