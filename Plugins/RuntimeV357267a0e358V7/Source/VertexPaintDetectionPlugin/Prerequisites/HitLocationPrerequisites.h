// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "CorePrerequisites.h"
#include "Chaos/ChaosEngineInterface.h"
#include "HitLocationPrerequisites.generated.h" 



//-------------------------------------------------------

// Physics Surface Data At Channel Info

USTRUCT(BlueprintType, meta = (DisplayName = "Physics Surface Data At Channel Info"))
struct FRVPDPPhysicsSurfaceDataAtChannelInfo {

	GENERATED_BODY()


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	TEnumAsByte<EPhysicalSurface> PhysicalSurfaceAtChannel = EPhysicalSurface::SurfaceType_Default;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	FString PhysicalSurfaceAsStringAtChannel = "None";

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	float PhysicalSurfaceValueAtChannel = 0.0f;
};


//-------------------------------------------------------

// Most Dominant Physics Surface Info

USTRUCT(BlueprintType, meta = (DisplayName = "Most Dominant Physics Surface Info"))
struct FRVPDPMostDominantPhysicsSurfaceInfo {

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Which EPhysical Surface had the most Color Value. Based on what surfaces is registered in the Material Data Asset for RGBA"))
	TEnumAsByte<EPhysicalSurface> MostDominantPhysicsSurface = EPhysicalSurface::SurfaceType_Default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	FString MostDominantPhysicsSurfaceAsString = "Default";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Amount that the most painted color had"))
	float MostDominantPhysicstSurfaceValue = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Which channel it was that had the most painted color, R, B, G or A. It can be multiple in case the Most Painted Surface was made up of blendade surfaces from different channels, then this will be from those channels"))
	TArray<ESurfaceAtChannel> MostDominantPhysicsSurfaceAtVertexColorChannels;
};


//-------------------------------------------------------

// Physics Surface Data Info

USTRUCT(BlueprintType, meta = (DisplayName = "Physics Surface Data Info"))
struct FRVPDPPhysicsSurfaceDataInfo {

	GENERATED_BODY()


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If we got the Physics Surface at Material"))
	bool PhysicsSurfaceSuccessfullyAcquired = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Information about the Most painted surface"))
	FRVPDPMostDominantPhysicsSurfaceInfo MostDominantPhysicsSurfaceInfo;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If the Material is registered in the Material Data Asset to Include Default Channel. The Default should be set to be the surface that is on the Material when no colors are painted, for example if Cobble is Default, and then you can paint grass and sand on top of it that blends into it then it should be included. "))
	bool MaterialRegisteredToIncludeDefaultChannel = false;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	FRVPDPPhysicsSurfaceDataAtChannelInfo PhysicsSurface_AtDefault;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	FRVPDPPhysicsSurfaceDataAtChannelInfo PhysicsSurface_AtRed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	FRVPDPPhysicsSurfaceDataAtChannelInfo PhysicsSurface_AtGreen;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	FRVPDPPhysicsSurfaceDataAtChannelInfo PhysicsSurface_AtBlue;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	FRVPDPPhysicsSurfaceDataAtChannelInfo PhysicsSurface_AtAlpha;



	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	TArray<TEnumAsByte<EPhysicalSurface>> PhysicalSurfacesAsArray;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	TArray<FString> SurfacesAsStringArray;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	TArray<float> SurfaceValuesArray;
};


//-------------------------------------------------------

// At Location Tasks Settings

USTRUCT(BlueprintType, meta = (DisplayName = "At Location Tasks Settings"))
struct FRVPDPAtLocationTasksSettings {

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Important to calculate the distance etc. from vertices to this location.  "))
	FVector HitLocation = FVector(ForceInitToZero);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	FVector HitLocationInComponentSpace = FVector(ForceInitToZero);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Not necessary unless you're using VertexNormalToHitNormalMinimumDotProduct in the Area Settings. "))
	FVector HitNormal = FVector(ForceInitToZero);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Not necessary, but if running Get Closest Vertex Data on Skeletal Mesh and haven't set to include colors, position etc. in the callback settings, i.e. we don't need to loop through all of the vertices, then with the bone we can make sure we only loop through the vertices on that section so the task will finish faster. "))
	FName HitBone = "";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "What it was Painted/Detected with, can be used when getting back the Result to do different things depending on what it was, for exampel Foot, Boot, Gunshot. "))
	FString RunTaskFor = "";
};


//-------------------------------------------------------

// Estimated Color At Hit Location Info

USTRUCT(BlueprintType, meta = (DisplayName = "Estimated Color At Hit Location Info"))
struct FRVPDPEstimatedColorAtHitLocationInfo {

	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If we successfully got an estimated color close or at the Hit Location. "))
	bool EstimatedColorAtHitLocationDataSuccessfullyAcquired = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The Estimated Color Close or At Hit Location. Could be useful if you want to run SFX or VFX, and the mesh has few vertices. If you have alot of vertices and the hit location is often very close to the closest vertex then you might not need to use this. \nThe way we calculate this is by getting the direction from the closest vertex to the hit location, and the nearest vertices around the hit location. Then when finished looping through LOD0 and we have them all and the closest vertex, we check, which of the nearby vertices has the best dot from to the direction from closest to hit location, i.e. which is the most optimal to scale toward. "))
	FLinearColor EstimatedColorAtHitLocation = FLinearColor(0, 0, 0, 0);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	FRVPDPPhysicsSurfaceDataInfo EstimatedPhysicalSurfaceInfoAtHitLocation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The location between the closest vertex and the optimal we scaled against to get the estimated color at hit location. "))
	FVector EstimatedColorAtWorldLocation = FVector(ForceInitToZero);

	FVector VertexWeEstimatedTowardWorldLocation = FVector(ForceInitToZero);

	TArray<FVector> ClosestVerticesWorldLocations;
};


//-------------------------------------------------------

// Estimated Color At Hit Location Settings

USTRUCT(BlueprintType, meta = (DisplayName = "Estimated Color At Hit Location Settings"))
struct FRVPDPEstimatedColorAtHitLocationSettings {

	GENERATED_BODY()


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "This will get the color as close to the actual Hit Location as possible, which is very useful if you have Meshes with few vertices and you want to run SFX or VFX based on the Color of the Hit location and not the closest vertex color in case there is a diff. \nThe task may take a bit longer to calculate if Mesh has alot of vertices, so you have the option to always get it, or only get it if the Mesh is below a certain amount of vertices. "))
	bool GetEstimatedColorAtHitLocation = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If True and GetEstimatedColorAtHitLocation is True, then will only get the estimated color at hit location if the Mesh has Max Amount of Vertices than what's set below. "))
	bool OnlyGetIfMeshHasMaxAmountOfVertices = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Will only get the estimated color if the mesh this max amount of vertices"))
	int MaxAmountOfVertices = 5000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Get Estimated Color to Hit Location won't work well if All nearby vertices are taken into account, since if it isn't just a Flat Mesh, but like a Porch Mesh with a Pillar in it, then the vertices on the pillar will be taken into account and mess up the calculation. So with this you can make sure that only vertices within a close normal to the hit is taken into account. The closer to 1 the more same they have to be to the hit normal, i.e. works if a completely flat floor. With -1 then all close vertices will be taken into account. \nSo something like 0.25 or 0.5 may work well so even if the surface is a bit uneven like a Hill or something then the expected vertices will be taken into account. "))
	float MinHitNormalToVertexNormalDotRequired = 0.25f;
};