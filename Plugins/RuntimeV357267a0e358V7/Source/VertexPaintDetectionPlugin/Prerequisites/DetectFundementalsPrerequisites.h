// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "FundementalsPrerequisites.h"
#include "DetectFundementalsPrerequisites.generated.h" 


//-------------------------------------------------------

// Detect Task Settings

USTRUCT(BlueprintType, meta = (DisplayName = "Detect Task Settings"))
struct FRVPDPDetectTaskSettings : public FRVPDPTaskFundamentalSettings {

	GENERATED_BODY()

	// Add anything all Detect Tasks shares here
};

