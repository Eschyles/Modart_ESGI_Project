// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 


#include "RVPDPToolbarButtonStyle.h"
#include "RVPDPToolbarButton.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleMacros.h"

#define RootToContentDir Style->RootToContentDir

TSharedPtr<FSlateStyleSet> FRVPDPToolbarButtonStyle::StyleInstance = nullptr;

void FRVPDPToolbarButtonStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FRVPDPToolbarButtonStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FRVPDPToolbarButtonStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("RVPDPToolbarButtonStyle"));
	return StyleSetName;
}


const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);

TSharedRef< FSlateStyleSet > FRVPDPToolbarButtonStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("RVPDPToolbarButtonStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("VertexPaintDetectionPlugin")->GetBaseDir() / TEXT("Resources"));

	Style->Set("RVPDPToolbarButton.PluginAction", new IMAGE_BRUSH(TEXT("VertexPaintPluginIcon"), Icon20x20));
	return Style;
}

void FRVPDPToolbarButtonStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FRVPDPToolbarButtonStyle::Get()
{
	return *StyleInstance;
}