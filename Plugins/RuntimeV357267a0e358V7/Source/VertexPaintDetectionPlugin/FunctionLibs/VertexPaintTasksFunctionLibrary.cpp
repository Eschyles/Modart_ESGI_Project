// Copyright 2022-2024 Alexander Floden, Alias Alex River. All Rights Reserved.


#include "VertexPaintTasksFunctionLibrary.h"

// Plugin
#include "VertexPaintDetectionComponent.h"
#include "VertexPaintDetectionLog.h"


//--------------------------------------------------------

// Get Closest Vertex Data On Mesh Wrapper

void UVertexPaintTasksFunctionLibrary::GetClosestVertexDataOnMesh_Wrapper(UVertexPaintDetectionComponent* RuntimeVertexPaintAndDetectionComponent, UPrimitiveComponent* MeshComponent, FRVPDPGetClosestVertexDataSettings GetClosestVertexDataStruct, FRVPDPAdditionalDataToPassThroughInfo AdditionalDataToPassThrough) {

	if (IsValid(RuntimeVertexPaintAndDetectionComponent)) {

		GetClosestVertexDataStruct.MeshComponent = MeshComponent;

		// Note nothing more should be set here, because if a C++ Class calls the Paint/Detection Function Right Away it shouldn't lose out on anything being set

		RuntimeVertexPaintAndDetectionComponent->GetClosestVertexDataOnMesh(GetClosestVertexDataStruct, AdditionalDataToPassThrough);
	}

	else {
		
		RVPDP_LOG(GetClosestVertexDataStruct.DebugSettings, FColor::Red, "Get Closest Vertex Data on Mesh Failed because the Paint Component sent in isn't valid! Perhaps the Actor or Component is being destroyed.");
	}
}


//--------------------------------------------------------

// Get All Vertex Colors Only Wrapper

void UVertexPaintTasksFunctionLibrary::GetAllVertexColorsOnly_Wrapper(UVertexPaintDetectionComponent* RuntimeVertexPaintAndDetectionComponent, UPrimitiveComponent* MeshComponent, FRVPDPGetColorsOnlySettings GetAllVertexColorsStruct, FRVPDPAdditionalDataToPassThroughInfo AdditionalDataToPassThrough) {

	if (IsValid(RuntimeVertexPaintAndDetectionComponent)) {

		GetAllVertexColorsStruct.MeshComponent = MeshComponent;

		// Note nothing more should be set here, because if a C++ Class calls the Paint/Detection Function Right Away it shouldn't lose out on anything being set

		RuntimeVertexPaintAndDetectionComponent->GetAllVertexColorsOnly(GetAllVertexColorsStruct, AdditionalDataToPassThrough);
	}

	else {

		RVPDP_LOG(GetAllVertexColorsStruct.DebugSettings, FColor::Red, "Get All Vertex Colors Failed because the Paint Component sent in isn't valid! Perhaps the Actor or Component is being destroyed.");
	}
}


//--------------------------------------------------------

// Get Colors Within Area Wrapper

void UVertexPaintTasksFunctionLibrary::GetColorsWithinArea_Wrapper(UVertexPaintDetectionComponent* RuntimeVertexPaintAndDetectionComponent, UPrimitiveComponent* MeshComponent, FRVPDPGetColorsWithinAreaSettings GetColorsWithinAreaStruct, FRVPDPAdditionalDataToPassThroughInfo AdditionalDataToPassThrough) {

	if (IsValid(RuntimeVertexPaintAndDetectionComponent)) {

		GetColorsWithinAreaStruct.MeshComponent = MeshComponent;

		// Note nothing more should be set here, because if a C++ Class calls the Paint/Detection Function Right Away it shouldn't lose out on anything being set

		RuntimeVertexPaintAndDetectionComponent->GetColorsWithinArea(GetColorsWithinAreaStruct, AdditionalDataToPassThrough);
	}

	else {

		RVPDP_LOG(GetColorsWithinAreaStruct.DebugSettings, FColor::Red, "Get Colors Within Area Failed because the Paint Component sent in isn't valid! Perhaps the Actor or Component is being destroyed.");
	}
}


//--------------------------------------------------------

// Paint on Mesh at Location Wrapper

void UVertexPaintTasksFunctionLibrary::PaintOnMeshAtLocation_Wrapper(UVertexPaintDetectionComponent* RuntimeVertexPaintAndDetectionComponent, UPrimitiveComponent* MeshComponent, FRVPDPPaintAtLocationSettings PaintAtLocationStruct, FRVPDPAdditionalDataToPassThroughInfo AdditionalDataToPassThrough) {

	if (IsValid(RuntimeVertexPaintAndDetectionComponent)) {

		PaintAtLocationStruct.MeshComponent = MeshComponent;

		// Note nothing more should be set here, because if a C++ Class calls the Paint/Detection Function Right Away it shouldn't lose out on anything being set

		RuntimeVertexPaintAndDetectionComponent->PaintOnMeshAtLocation(PaintAtLocationStruct, AdditionalDataToPassThrough);
	}

	else {

		RVPDP_LOG(PaintAtLocationStruct.DebugSettings, FColor::Red, "Paint at Location Failed because the Paint Component sent in isn't valid! Perhaps the Actor or Component is being destroyed.");
	}
}


//--------------------------------------------------------

// Paint Within Area Wrapper

void UVertexPaintTasksFunctionLibrary::PaintOnMeshWithinArea_Wrapper(UVertexPaintDetectionComponent* RuntimeVertexPaintAndDetectionComponent, UPrimitiveComponent* MeshComponent, FRVPDPPaintWithinAreaSettings PaintWithinAreaStruct, FRVPDPAdditionalDataToPassThroughInfo AdditionalDataToPassThrough) {

	if (IsValid(RuntimeVertexPaintAndDetectionComponent)) {

		PaintWithinAreaStruct.MeshComponent = MeshComponent;
		// Note nothing more should be set here, because if a C++ Class calls the Paint/Detection Function Right Away it shouldn't lose out on anything being set

		RuntimeVertexPaintAndDetectionComponent->PaintOnMeshWithinArea(PaintWithinAreaStruct, AdditionalDataToPassThrough);
	}

	else {

		RVPDP_LOG(PaintWithinAreaStruct.DebugSettings, FColor::Red, "Paint on Mesh Within Area Failed because the Paint Component sent in isn't valid! Perhaps the Actor or Component is being destroyed.");
	}
}


//--------------------------------------------------------

// Paint on Entire Mesh Wrapper

void UVertexPaintTasksFunctionLibrary::PaintOnEntireMesh_Wrapper(UVertexPaintDetectionComponent* RuntimeVertexPaintAndDetectionComponent, UPrimitiveComponent* MeshComponent, FRVPDPPaintOnEntireMeshSettings PaintOnEntireMeshStruct, FRVPDPAdditionalDataToPassThroughInfo AdditionalDataToPassThrough) {

	if (IsValid(RuntimeVertexPaintAndDetectionComponent)) {

		PaintOnEntireMeshStruct.MeshComponent = MeshComponent;

		// Note nothing more should be set here, because if a C++ Class calls the Paint/Detection Function Right Away it shouldn't lose out on anything being set

		RuntimeVertexPaintAndDetectionComponent->PaintOnEntireMesh(PaintOnEntireMeshStruct, AdditionalDataToPassThrough);
	}

	else {

		RVPDP_LOG(PaintOnEntireMeshStruct.DebugSettings, FColor::Red, "Paint on Entire Mesh Failed because the Paint Component sent in isn't valid! Perhaps the Actor or Component is being destroyed.");
	}
}


//--------------------------------------------------------

// Paint Color Snippet On Mesh

void UVertexPaintTasksFunctionLibrary::PaintColorSnippetOnMesh_Wrappers(UVertexPaintDetectionComponent* RuntimeVertexPaintAndDetectionComponent, UPrimitiveComponent* MeshComponent, FRVPDPPaintColorSnippetSettings PaintColorSnippetStruct, FRVPDPAdditionalDataToPassThroughInfo AdditionalDataToPassThrough) {

	if (IsValid(RuntimeVertexPaintAndDetectionComponent)) {

		PaintColorSnippetStruct.MeshComponent = MeshComponent;

		// Note nothing more should be set here, because if a C++ Class calls the Paint/Detection Function Right Away it shouldn't lose out on anything being set

		RuntimeVertexPaintAndDetectionComponent->PaintColorSnippetOnMesh(PaintColorSnippetStruct, AdditionalDataToPassThrough);
	}

	else {

		RVPDP_LOG(PaintColorSnippetStruct.DebugSettings, FColor::Red, "Paint Color Snippet on Mesh Failed because the Vertex Paint Component sent in isn't valid! Perhaps the Actor or Component is being destroyed, or it hasn't been properly initialized.");
	}
}


//--------------------------------------------------------

// Paint Group Snippet On Mesh

void UVertexPaintTasksFunctionLibrary::PaintGroupSnippetOnMesh_Wrapper(UVertexPaintDetectionComponent* RuntimeVertexPaintAndDetectionComponent, TArray<UPrimitiveComponent*> GroupSnippetMeshes, FRVPDPPaintGroupSnippetSettings PaintGroupSnippetStruct, FRVPDPAdditionalDataToPassThroughInfo AdditionalDataToPassThrough) {

	if (IsValid(RuntimeVertexPaintAndDetectionComponent)) {

		if (PaintGroupSnippetStruct.PaintColorSnippetSetting != FRVPDPPaintColorSnippetSetting::PaintGroupSnippet) {
			
			RVPDP_LOG(PaintGroupSnippetStruct.DebugSettings, FColor::Cyan, "Paint Group Snippet on Mesh Failed the Paint Color Snippet Setting is not set to Paint Group Snippet.");

			return;
		}

		if (GroupSnippetMeshes.Num() > 0) {

			PaintGroupSnippetStruct.MeshComponent = GroupSnippetMeshes[0];
			PaintGroupSnippetStruct.PaintGroupSnippetMeshes = GroupSnippetMeshes;

			// Note nothing more should be set here, because if a C++ Class calls the Paint/Detection Function Right Away it shouldn't lose out on anything being set

			RuntimeVertexPaintAndDetectionComponent->PaintColorSnippetOnMesh(PaintGroupSnippetStruct, AdditionalDataToPassThrough);
		}

		else {

			RVPDP_LOG(PaintGroupSnippetStruct.DebugSettings, FColor::Red, "Paint Group Snippet on Mesh Failed Because no Group Snippet Meshes was Connected.");

			return;
		}
	}

	else {

		RVPDP_LOG(PaintGroupSnippetStruct.DebugSettings, FColor::Red, "Paint Group Snippet on Mesh Failed because the Vertex Paint Component sent in isn't valid! Perhaps the Actor or Component is being destroyed, or it hasn't been properly initialized.");

		return;
	}
}


//--------------------------------------------------------

// Set Mesh Component Vertex Colors

void UVertexPaintTasksFunctionLibrary::SetMeshComponentVertexColors_Wrapper(UVertexPaintDetectionComponent* RuntimeVertexPaintAndDetectionComponent, UPrimitiveComponent* MeshComponent, FRVPDPSetVertexColorsSettings SetMeshComponentVertexColorsSettings, FRVPDPAdditionalDataToPassThroughInfo AdditionalDataToPassThrough) {

	if (IsValid(RuntimeVertexPaintAndDetectionComponent)) {

		SetMeshComponentVertexColorsSettings.MeshComponent = MeshComponent;

		// Note nothing more should be set here, because if a C++ Class calls the Paint/Detection Function Right Away it shouldn't lose out on anything being set

		RuntimeVertexPaintAndDetectionComponent->SetMeshComponentVertexColors(SetMeshComponentVertexColorsSettings, AdditionalDataToPassThrough);
	}

	else {

		RVPDP_LOG(SetMeshComponentVertexColorsSettings.DebugSettings, FColor::Red, "Paint Set Mesh Component Colors Failed because the Vertex Paint Component sent in isn't valid! Perhaps the Actor or Component is being destroyed, or it hasn't been properly initialized.");
	}
}


//--------------------------------------------------------

// Set Mesh Component Vertex Colors Using Serialized String

void UVertexPaintTasksFunctionLibrary::SetMeshComponentVertexColorsUsingSerializedString_Wrapper(UVertexPaintDetectionComponent* RuntimeVertexPaintAndDetectionComponent, UPrimitiveComponent* MeshComponent, FRVPDPSetVertexColorsUsingSerializedStringSettings SetMeshComponentVertexColorsUsingSerializedStringSettings, FRVPDPAdditionalDataToPassThroughInfo AdditionalDataToPassThrough) {

	if (IsValid(RuntimeVertexPaintAndDetectionComponent)) {

		SetMeshComponentVertexColorsUsingSerializedStringSettings.MeshComponent = MeshComponent;

		// Note nothing more should be set here, because if a C++ Class calls the Paint/Detection Function Right Away it shouldn't lose out on anything being set

		RuntimeVertexPaintAndDetectionComponent->SetMeshComponentVertexColorsUsingSerializedString(SetMeshComponentVertexColorsUsingSerializedStringSettings, AdditionalDataToPassThrough);
	}

	else {

		RVPDP_LOG(SetMeshComponentVertexColorsUsingSerializedStringSettings.DebugSettings, FColor::Red, "Set Mesh Component Vertex Colors Using Serialized String Failed because the Vertex Paint Component sent in isn't valid! Perhaps the Actor or Component is being destroyed, or it hasn't been properly initialized.");
	}
}
