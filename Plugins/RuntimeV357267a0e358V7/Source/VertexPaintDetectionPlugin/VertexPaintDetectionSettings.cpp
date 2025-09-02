// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 


#include "VertexPaintDetectionSettings.h"
#include "VertexPaintDetectionLog.h"

//-------------------------------------------------------

// Construct

UVertexPaintDetectionSettings::UVertexPaintDetectionSettings() {

	RVPDP_LOG(FRVPDPDebugSettings(false, 0, true), FColor::Cyan, "Vertex Paint Detection Settings Construct");

#if WITH_EDITOR

	// Binds so we get a callback to whenever a settings has been changed
	SettingsChangedDelegate.AddUObject(this, &UVertexPaintDetectionSettings::VertexPaintDetectionSettingsChanged);

#endif
}

#if WITH_EDITOR

//-------------------------------------------------------

// Vertex Paint Detection Settings Changed

void UVertexPaintDetectionSettings::VertexPaintDetectionSettingsChanged(UObject* Settings, FPropertyChangedEvent& PropertyChangedEvent) const {

	RVPDP_LOG(FRVPDPDebugSettings(false, 0, true), FColor::Cyan, "Vertex Paint Detection Settings Changed");
}

#endif