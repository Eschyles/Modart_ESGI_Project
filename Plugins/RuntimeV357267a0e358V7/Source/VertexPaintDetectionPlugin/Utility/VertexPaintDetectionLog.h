// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "VertexPaintDetectionLog.generated.h"


class UWorld;


//-------------------------------------------------------

// Task Specific Debug Symbols Settings

USTRUCT(BlueprintType, meta = (DisplayName = "Task Specific Debug Symbols Settings"))
struct FRVPDPTaskSpecificDebugSymbolsSettings {

	GENERATED_BODY()

	bool operator==(const FRVPDPTaskSpecificDebugSymbolsSettings& Other) const {

		if (DrawTaskDebugSymbols == Other.DrawTaskDebugSymbols &&
			DrawTaskDebugSymbolsDuration == Other.DrawTaskDebugSymbolsDuration) {

			return true;
		}

		return false;
	}

	bool operator!=(const FRVPDPTaskSpecificDebugSymbolsSettings& Other) const {

		return !(*this == Other);
	}


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin|Debug Settings|Paint Within Area Symbols", meta = (ToolTip = "This will Draw any Debug Symbols Related to the Task being done, for example the Bounds that we use if it's a Paint Within Area Task. "))
	bool DrawTaskDebugSymbols = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin|Debug Settings|Paint Within Area Symbols")
	float DrawTaskDebugSymbolsDuration = 0.15f;
};


//-------------------------------------------------------

// Game Thread Specific Debug Settings

USTRUCT(BlueprintType, meta = (DisplayName = "Game Thread Specific Debug Settings"))
struct FRVPDPGameThreadSpecificDebugSettings {

	GENERATED_BODY()

	bool operator==(const FRVPDPGameThreadSpecificDebugSettings& Other) const {

		if (DrawVertexPositionDebugPoint == Other.DrawVertexPositionDebugPoint &&
			DrawVertexPositionDebugPointIfGotPaintApplied == Other.DrawVertexPositionDebugPointIfGotPaintApplied &&
			DrawClothVertexPositionDebugPoint == Other.DrawClothVertexPositionDebugPoint &&
			DrawVertexNormalDebugArrow == Other.DrawVertexNormalDebugArrow &&
			DrawPaintConditionsDebugSymbols == Other.DrawPaintConditionsDebugSymbols &&
			DrawGameThreadSpecificDebugSymbolsDuration == Other.DrawGameThreadSpecificDebugSymbolsDuration) {

			return true;
		}

		return false;
	}

	bool operator!=(const FRVPDPGameThreadSpecificDebugSettings& Other) const {

		return !(*this == Other);
	}


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin|Debug Settings|Vertex Symbols", meta = (ToolTip = "Draws a Debug Point for Each Vertex of the Mesh. \nNOTE! Only possible if running the Paint Job with Multithreading set to False, and can be quite expensive if the Mesh has a lot of Vertices. "))
	bool DrawVertexPositionDebugPoint = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin|Debug Settings|Vertex Symbols", meta = (ToolTip = "Draws a Debug Point for the Vertex of the Mesh that got paint applied. \nNOTE! Only possible if running the Paint Job with Multithreading set to False, and can be quite expensive if the Mesh has a lot of Vertices. "))
	bool DrawVertexPositionDebugPointIfGotPaintApplied = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin|Debug Settings|Vertex Symbols", meta = (ToolTip = "Draws a Debug Point for Each Vertex of Cloths of the Mesh we're looping through. \nNOTE! Only possible if running the Paint Job with Multithreading set to False, and can be quite expensive if the Cloth has a lot of Vertices. "))
	bool DrawClothVertexPositionDebugPoint = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin|Debug Settings|Vertex Symbols", meta = (ToolTip = "Draws a Debug Arrow for each Vertex Normal. \nNOTE! Only possible if running the Paint Job with Multithreading set to False, and can be quite expensive if the Mesh has a lot of Vertices. "))
	bool DrawVertexNormalDebugArrow = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin|Debug Settings|Paint Condition Symbols", meta = (ToolTip = "This will draw any debug symbols for paint conditions used, if the paint condition has any. For instance Line of Sight Condition will draw a Line to indicate if it has line of sight or if blocked etc. \nNOTE! Only possible if running the Paint Job with Multithreading set to False, and can be quite expensive if the Mesh has a lot of Vertices. "))
	bool DrawPaintConditionsDebugSymbols = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin|Debug Settings|Vertex Symbols", meta = (ToolTip = "Duration of the Drawn Vertex Points. Can be very low if you're painting every frame. "))
	float DrawGameThreadSpecificDebugSymbolsDuration = 0.15;
};


//-------------------------------------------------------

// Debug Settings

USTRUCT(BlueprintType, meta = (DisplayName = "Debug Settings"))
struct FRVPDPDebugSettings {

	GENERATED_BODY()

	bool operator==(const FRVPDPDebugSettings& Other) const {

		if (WorldTaskWasCreatedIn.Get() == Other.WorldTaskWasCreatedIn.Get() &&
			PrintLogsToScreen == Other.PrintLogsToScreen &&
			PrintLogsToScreen_Duration == Other.PrintLogsToScreen_Duration &&
			PrintLogsToOutputLog == Other.PrintLogsToOutputLog &&
			GameThreadSpecificDebugSymbols == Other.GameThreadSpecificDebugSymbols &&
			TaskSpecificDebugSymbols == Other.TaskSpecificDebugSymbols) {

			return true;
		}

		return false;
	}

	bool operator!=(const FRVPDPDebugSettings& Other) const {

		return !(*this == Other);
	}


	FRVPDPDebugSettings()
		: WorldTaskWasCreatedIn(nullptr),
		PrintLogsToScreen(false),
		PrintLogsToScreen_Duration(5),
		PrintLogsToOutputLog(false) {}

	FRVPDPDebugSettings(bool PrintToScreen, float PrintToScreenDuration, bool PrintToOutputLog)
		: WorldTaskWasCreatedIn(nullptr),
		PrintLogsToScreen(PrintToScreen),
		PrintLogsToScreen_Duration(PrintToScreenDuration),
		PrintLogsToOutputLog(PrintToOutputLog) {}

	FRVPDPDebugSettings(UWorld* World, bool PrintToScreen, float PrintToScreenDuration, bool PrintToOutputLog)
		: WorldTaskWasCreatedIn(World),
		PrintLogsToScreen(PrintToScreen),
		PrintLogsToScreen_Duration(PrintToScreenDuration),
		PrintLogsToOutputLog(PrintToOutputLog) {}


	UPROPERTY()
	TWeakObjectPtr<UWorld> WorldTaskWasCreatedIn = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin|Debug Settings|Logs", meta = (ToolTip = "If should print logs to screen."))
	bool PrintLogsToScreen = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin|Debug Settings|Logs", meta = (ToolTip = "Duration of the Print Strings on the Screen"))
	float PrintLogsToScreen_Duration = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin|Debug Settings|Logs", meta = (ToolTip = "If should print logs to output log."))
	bool PrintLogsToOutputLog = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin|Debug Settings|Vertex Symbols", meta = (ToolTip = "Has options for drawing debug symbols for each vertex and paint conditions. Will only be visible if the task is run on the Game Thread, i.e. Multithreading set to false. "))
	FRVPDPGameThreadSpecificDebugSettings GameThreadSpecificDebugSymbols;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin|Debug Settings|Paint Within Area Symbols", meta = (ToolTip = "This will Draw any Debug Symbols Related to the Task being done, for example the Bounds that we use if it's a Paint Within Area Task. "))
	FRVPDPTaskSpecificDebugSymbolsSettings TaskSpecificDebugSymbols;
};



DECLARE_LOG_CATEGORY_EXTERN(RuntimeVertexColorPlugin, Log, All);


#define RVPDP_LOG(DebugSettings, ScreenTextColor, Format, ...) \
    VertexPaintDetectionLog::PrintVertexPaintDetectionLog(DebugSettings, ScreenTextColor, FString::Printf(TEXT(Format), ##__VA_ARGS__))



class VERTEXPAINTDETECTIONPLUGIN_API VertexPaintDetectionLog {

public:

	static void PrintVertexPaintDetectionLog(const FRVPDPDebugSettings& DebugSettings, const FColor& ScreenTextColor, const FString& Text);

};