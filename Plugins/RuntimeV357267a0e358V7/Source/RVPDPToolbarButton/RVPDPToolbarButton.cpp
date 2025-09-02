// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 


#include "RVPDPToolbarButton.h"
#include "RVPDPToolbarButtonStyle.h"
#include "RVPDPToolbarButtonCommands.h"
#include "Misc/MessageDialog.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "WidgetBlueprint.h"
#include "EditorUtilityWidget.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "EditorUtilitySubsystem.h"

#include "ToolMenus.h"

static const FName RVPDPToolbarButtonTabName("RVPDPToolbarButton");

#define LOCTEXT_NAMESPACE "FRVPDPToolbarButtonModule"

void FRVPDPToolbarButtonModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	UE_LOG(LogTemp, Log, TEXT("Runtime Vertex Paint and Detection Plugin - UE5 Editor Toolbar Button Module Startup!"));

	FRVPDPToolbarButtonStyle::Initialize();
	FRVPDPToolbarButtonStyle::ReloadTextures();

	FRVPDPToolbarButtonCommands::Register();

	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FRVPDPToolbarButtonCommands::Get().PluginAction,
		FExecuteAction::CreateRaw(this, &FRVPDPToolbarButtonModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FRVPDPToolbarButtonModule::RegisterMenus));
}

void FRVPDPToolbarButtonModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UE_LOG(LogTemp, Log, TEXT("Runtime Vertex Paint and Detection Plugin - UE5 Editor Toolbar Button Module Shutdown! "));

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FRVPDPToolbarButtonStyle::Shutdown();

	FRVPDPToolbarButtonCommands::Unregister();
}

void FRVPDPToolbarButtonModule::PluginButtonClicked()
{

	UE_LOG(LogTemp, Log, TEXT("Runtime Vertex Paint and Detection Plugin - Pressed Editor Utility Button"));

	/*
	// Put your "OnButtonClicked" stuff here
	FText DialogText = FText::Format(
							LOCTEXT("PluginButtonDialogText", "Add code to {0} in {1} to override this button's actions"),
							FText::FromString(TEXT("FRVPDPToolbarButtonModule::PluginButtonClicked()")),
							FText::FromString(TEXT("RVPDPToolbarButton.cpp"))
					   );
	FMessageDialog::Open(EAppMsgType::Ok, DialogText);

	UE_LOG(Log, Warning, TEXT("Vertex Paint and Detection Plugin - Pressed Editor Utility Button"));
	*/

	auto editorSubsystem = GEditor->GetEditorSubsystem<UEditorUtilitySubsystem>();

	// Creates the Editor Utility Widget Blueprint when pressing the Button
	if (editorSubsystem->DoesTabExist(EditorWidgetID)) {

		editorSubsystem->CloseTabByID(EditorWidgetID);
	}

	else {

		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		TArray<FAssetData> AssetData;

		// auto editorWidgetAsset = AssetRegistryModule.Get().GetAssetByObjectPath("/VertexPaintDetectionPlugin/EditorUtility/EWBP_VertexPaint_PluginSettings.EWBP_VertexPaint_PluginSettings", true).GetAsset();


#if ENGINE_MINOR_VERSION == 0

		UObject* editorWidgetAsset = AssetRegistryModule.Get().GetAssetByObjectPath("/VertexPaintDetectionPlugin/EditorUtility/Widgets/EWBP_VertexPaint_PluginSettings.EWBP_VertexPaint_PluginSettings", true).GetAsset();

#else

		FSoftObjectPath objectPath;
		objectPath.SetPath("/VertexPaintDetectionPlugin/EditorUtility/Widgets/EWBP_VertexPaint_PluginSettings.EWBP_VertexPaint_PluginSettings");

		UObject* editorWidgetAsset = AssetRegistryModule.Get().GetAssetByObjectPath(objectPath, true).GetAsset();

#endif

		if (editorWidgetAsset) {

			auto widgetBlueprint = Cast<UWidgetBlueprint>(editorWidgetAsset);

			if (widgetBlueprint) {

				if (auto editorWidgetBlueprint = Cast<UEditorUtilityWidgetBlueprint>(editorWidgetAsset))
					editorSubsystem->SpawnAndRegisterTabAndGetID(editorWidgetBlueprint, EditorWidgetID);
			}
		}
	}
}

void FRVPDPToolbarButtonModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FRVPDPToolbarButtonCommands::Get().PluginAction, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("PluginTools");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FRVPDPToolbarButtonCommands::Get().PluginAction));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FRVPDPToolbarButtonModule, RVPDPToolbarButton)
