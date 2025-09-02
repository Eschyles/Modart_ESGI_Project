// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "DetectFundementalsPrerequisites.h"
#include "GetColorsOnlyPrerequisites.generated.h" 


//-------------------------------------------------------

// Get Colors Only Settings

USTRUCT(BlueprintType, meta = (DisplayName = "Get Colors Only Settings"))
struct FRVPDPGetColorsOnlySettings : public FRVPDPDetectTaskSettings {

	GENERATED_BODY()

	// NOTE That there is a bug in BP (atleast 5.0) where if you drag out and Make a Struct, then it will still be whatever is set in the parent. This will only take affect if not doing that, or Splitting the Struct in the Node. 
	FRVPDPGetColorsOnlySettings() {

		// Since this task literaly is GetColors we automatically set this to true since it may be what the user expects. 
		CallbackSettings.IncludeVertexColorData = true;
	}
};