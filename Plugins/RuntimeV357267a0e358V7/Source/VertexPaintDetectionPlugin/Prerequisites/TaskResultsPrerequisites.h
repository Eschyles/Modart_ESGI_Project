// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "HitLocationPrerequisites.h"
#include "Chaos/ChaosEngineInterface.h"
#include "GameplayTagContainer.h"
#include "TaskResultsPrerequisites.generated.h" 


class UVertexPaintDetectionComponent;


//-------------------------------------------------------

// Average Color In Area Info

USTRUCT(BlueprintType, meta = (DisplayName = "Average Color In Area Info"))
struct FRVPDPAverageColorInAreaInfo {

	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	bool GotAvarageVertexColorsWithinAreaOfEffect = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The Avarage Vertex Colors within the Area when Detecting (if set an AoE), or the Avarage Vertex Colors when Painting at Location After Applying Vertex Colors. "))
	FLinearColor AvarageVertexColorsWithinAreaOfEffect = FLinearColor(0, 0, 0, 0);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	FRVPDPPhysicsSurfaceDataInfo AvaragePhysicalSurfaceInfoBasedOffTheClosestVertexMaterial;
};


//-------------------------------------------------------

// Bone Vertex Colors Info

USTRUCT(BlueprintType, meta = (DisplayName = "Bone Vertex Colors Info"))
struct FRVPDPBoneVertexColorsInfo {

	GENERATED_BODY()


	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	FName BoneName = "";

	UPROPERTY(/*VisibleAnywhere, */BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	TArray<FColor> BoneVertexColors;
};



//-------------------------------------------------------

// Vertex Colors On Each Bone Result

USTRUCT(BlueprintType, meta = (DisplayName = "Vertex Colors On Each Bone Result"))
struct FRVPDPVertexColorsOnEachBoneResult {

	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	bool SuccessFullyGotColorsForEachBone = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	TArray<FRVPDPBoneVertexColorsInfo> ColorsOfEachBone;
};


//-------------------------------------------------------

// Compare Mesh Vertex Colors To Color Array Result

USTRUCT(BlueprintType, meta = (DisplayName = "Compare Mesh Vertex Colors To Color Array Result"))
struct FRVPDPCompareMeshVertexColorsToColorArrayResult {

	GENERATED_BODY()


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If we successfully compared the Vertex Colors from the Mesh, and the ones the user sent in to compare them with. "))
	bool SuccessfullyComparedMeshVertexColorsToColorArray = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The Matching Percent between 0-100, this also includes vertices that had no color, as long as the mesh vertex color and the compare vertex colors where the same and within range, they're taken into account here. "))
	float MatchingPercent = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If success, but we skipped all of the vertices. "))
	bool SkippedAllVertices = false;
};


//-------------------------------------------------------

// Compare Mesh Vertex Colors To Color Snippet Result

USTRUCT(BlueprintType, meta = (DisplayName = "Compare Mesh Vertex Colors To Color Snippet Result"))
struct FRVPDPCompareMeshVertexColorsToColorSnippetResult {

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If we successfully compared the Vertex Colors from the Mesh to at least one snippet that the user has set it to compare to. "))
	bool SuccessfullyComparedMeshVertexColorsToColorSnippet = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The Matching Percent between 0-100 for each snippet we comppared to. "))
	TMap<FGameplayTag, float> MatchingPercentPerColorSnippet;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If success, but we skipped all of the vertices since they all matched the skip color set for the snippet. "))
	TMap<FGameplayTag, bool> SkippedAllVerticesPerSnippet;
};


//-------------------------------------------------------

// Amount Of Colors Of Each Channel - Channel Results

USTRUCT(BlueprintType, meta = (DisplayName = "Amount Of Colors Of Each Channel - Channel Results"))
struct FRVPDPAmountOfColorsOfEachChannelChannelResults {

	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	float AmountOfVerticesPaintedAtMinAmount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	float PercentPaintedAtMinAmount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	float AverageColorAmountAtMinAmount = 0;

	float AmountOfVerticesConsidered = 0;
};


//-------------------------------------------------------

// Amount Of Colors Of Each Channel Physics Results

USTRUCT(BlueprintType, meta = (DisplayName = "Amount Of Colors Of Each Channel Physics Results"))
struct FRVPDPAmountOfColorsOfEachChannelPhysicsResults : public FRVPDPAmountOfColorsOfEachChannelChannelResults {

	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	TEnumAsByte<EPhysicalSurface> PhysicsSurface = EPhysicalSurface::SurfaceType_Default;
};


//-------------------------------------------------------

// Amount Of Colors Of Each Channel Results

USTRUCT(BlueprintType, meta = (DisplayName = "Amount Of Colors Of Each Channel Results"))
struct FRVPDPAmountOfColorsOfEachChannelResults {

	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	bool SuccessfullyGotColorChannelResultsAtMinAmount = false;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	FRVPDPAmountOfColorsOfEachChannelChannelResults RedChannelResult;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	FRVPDPAmountOfColorsOfEachChannelChannelResults GreenChannelResult;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	FRVPDPAmountOfColorsOfEachChannelChannelResults BlueChannelResult;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	FRVPDPAmountOfColorsOfEachChannelChannelResults AlphaChannelResult;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If we actually got the physics surface result and the IncludePhysicsSurfaceResult Setting in the Callback Setting was set to True. \nGetting the Physics Surface colors requires a couple of more loops for every vertex and can possible increase the time to finish the task by a very small amount, but we still make it optional since there are tasks like Paint Color Snippet, that usually doesn't need to loop through all of the sections etc., which may take noticable longer if it has to do that. "))
	bool SuccessfullyGotPhysicsSurfaceResultsAtMinAmount = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If IncludePhysicsSurfaceResult and if there are any Registered Physics Surface on the Material Detected/Painted on, then we can get the amount of vertices, percent and average amount of each physics surface. We sort this as well so the first element in this TMap is the one with the highest %. "))
	TArray<FRVPDPAmountOfColorsOfEachChannelPhysicsResults> PhysicsSurfacesResults;
};


//-------------------------------------------------------

// Within Area Results

USTRUCT(BlueprintType, meta = (DisplayName = "Within Area Results"))
struct FRVPDPWithinAreaResults {

	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The Vertex Colors, Position and Normal Within the Area, if set to Include them when calling PaintWithinArea. "))
	TArray<FVertexDetectMeshDataPerLODStruct> MeshVertexDataWithinArea;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The Average, Amount and % of Colors of Each Channel & Physics Surface within the Area, if set to Include them when calling Paint Within Area. "))
	FRVPDPAmountOfColorsOfEachChannelResults AmountOfPaintedColorsOfEachChannelWithinArea;
};


//-------------------------------------------------------

// Task Result Info

USTRUCT(BlueprintType, meta = (DisplayName = "Task Result Info"))
struct FRVPDPTaskResults {

	GENERATED_BODY()


	UVertexPaintDetectionComponent* GetAssociatedPaintComponent() const {

		if (AssociatedPaintComponent.IsValid())
			return AssociatedPaintComponent.Get();

		return nullptr;
	}

	UPrimitiveComponent* GetMeshComponent() const {

		if (MeshComponent.IsValid())
			return MeshComponent.Get();

		return nullptr;
	}


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If the Paint/Detect Task was Successfull"))
	int TaskID = -1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If the Paint/Detect Task was Successfull"))
	bool TaskSuccessfull = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "How long in Seconds the Task took to finish. \nCan be useful to sync paint jobs if you for instance is Painting many meshes at the same time, like if it's raining and you're running Paint Entire Mesh at Random Vertices. In those cases some tasks may finish much faster than others since they may have different amount of vertices, and if you start a new task when the old one is finished it means that some meshes will get wet too fast compared to other meshes. With this you can check, what is the highest duration a task ever took, and if a task finishes before that, maybe add a delay before you start a new task or something similar with the delay duration to be the difference to the finished task duration and the highest duration. "))
	float TaskDuration = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If Painted using Paint Component then it was used with this. There are some paint jobs like SetMeshComponentVertexColors and SetMeshComponentVertexColorsUsingSerializedString where using a component is optional. "))
	TWeakObjectPtr<UVertexPaintDetectionComponent> AssociatedPaintComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The Component the Task Finished Painting or Detecting. "))
	TWeakObjectPtr<UPrimitiveComponent> MeshComponent = nullptr;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The Mesh Component, Source Mesh and includes Vertex Color, Position and Normal for each LOD if they've been set to be included in the Paint/Detect Jobs Callback Settings.  "))
	FRVPDPVertexDataInfo MeshVertexData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If you in the Callback Settings set to Compare Vertex Colors to a Color Array, to for exmaple check if the player has painted a certain pattern or something, then this will be the result of that. "))
	FRVPDPCompareMeshVertexColorsToColorArrayResult CompareMeshVertexColorsToColorArrayResult;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If you in the Callback Settings set to Compare Mesh Vertex Colors to Color Snippets, then this is the result of that. "))
	FRVPDPCompareMeshVertexColorsToColorSnippetResult CompareMeshVertexColorsToColorSnippetResult;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If we included in the Callback Settings to get colors of each channel (is true by default) then you will get the Percent, Average Amount, and How many vertices on each RGBA Vertex Color Channel, so you can for instance check if the Red Channel has is over a certain %. \nYou can in the callback settings also set to include the results for each physics surface, then you will get %, average and amount for each physics surface as well, otherwise that TMap won't be filled.  "))
	FRVPDPAmountOfColorsOfEachChannelResults AmountOfPaintedColorsOfEachChannel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Colors of each bone at LOD0. Can get set to be included in the Tasks Callback Settings.  "))
	FRVPDPVertexColorsOnEachBoneResult VertexColorsOnEachBone;
};


//-------------------------------------------------------

// Paint Task Result Info

USTRUCT(BlueprintType, meta = (DisplayName = "Paint Task Result Info"))
struct FRVPDPPaintTaskResultInfo {

	GENERATED_BODY()


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If the Finished Task was a Paint Job of some kind, and it changed any vertex colors. For example if you run paint at location at an area that already has the colors that you want to paint, then no difference would occur so we didn't apply any new colors, or if you tried to paint on a mesh that's already fully painted, or tried to remove colors if it didn't have any colors. "))
	bool AnyVertexColorGotChanged = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The Bones that got color applied to any of their vertices. If Paint Entire Mesh, Set Mesh Component Colors or Paint Color Snippet, then all bones will be here. "))
	TArray<FName> BonesThatGotPainted;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Vertex"))
	TArray<int32> VertexIndicesThatGotPainted;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If Paint Job, then these are the Vertex Color Channels that got changed by that paint job. If a Paint job like Color Snippet, SetMeshComponentVertexColors, SetMeshComponentVertexColorsUsingSerialized string with no settings that requires them to loop through all vertices, then we can't check which color channels got changed so then all of them will be added, making an assumption they all got changed in that use case. "))
	TArray<EVertexColorChannel> ColorAppliedToVertexColorChannels;
};
