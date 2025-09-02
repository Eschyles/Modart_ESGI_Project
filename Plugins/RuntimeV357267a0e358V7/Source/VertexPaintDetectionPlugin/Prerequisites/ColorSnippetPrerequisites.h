// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "CorePrerequisites.h"
#include "ColorSnippetPrerequisites.generated.h" 


class UVertexPaintColorSnippetDataAsset;


//-------------------------------------------------------

// Paint Color Snippet Setting

UENUM(BlueprintType, meta = (DisplayName = "Paint Color Snippet Setting"))
enum class FRVPDPPaintColorSnippetSetting : uint8 {

	PaintSingleSnippet = 0 UMETA(DisplayName = "Paint Single Snippet", Tooltip = "Will just Paint the Selected Snippet as Expected. "),
	PaintGroupSnippet = 1 UMETA(DisplayName = "Paint Group Snippet", Tooltip = "Will Paint a Group Snippet if one has been selected and the correct Meshes that is used by the group is connected. ")
};


//-------------------------------------------------------

// Stored Color Snippet Info

USTRUCT(BlueprintType, meta = (DisplayName = "Stored Color Snippet Info"))
struct FRVPDPStoredColorSnippetInfo {

	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The Vertex Color Snippet Data Asset that stores the snippet datas for this mesh, if any has been added from the Editor Widget when in editor. NOTE Can't be added in runtime! \nCan be used if you for instance want a bunch of zombies with different blood patterns on them, then you can store a bunch of different snippets and randomize which to use when spawning a zombie. When we look them up the data asset is loaded into memory but not cached so it can get GC since we only need to use it for the remainder of the execution. "))
	TSoftObjectPtr<UVertexPaintColorSnippetDataAsset> ColorSnippetDataAssetStoredOn;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Object the colors belong to"))
	TSoftObjectPtr<UObject> ObjectColorSnippetBelongsTo = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If this belongs to be Group Snippet. "))
	bool IsPartOfAGroupSnippet = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The Group Snippet this belongs to. "))
	FString GroupSnippetID = "";

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The Relative Location this had to the Group Meshes when stored. This is used to identify each mesh when painting again so we can get which mesh should be painted with which snippets, so we can be independent of soft ptrs pointing to components in specific levels but be able to spawn in stuff and have it work as well. "))
	FVector RelativeLocationToGroupCenterPoint = FVector(ForceInitToZero);

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The Dot Product this mesh had to the group center point, used to check if something has been rotated incorrectly, where if so we dont want to paint the snippet since then it won't look accurate. "))
	float DotProductToGroupCenterPoint = 0;
};


//-------------------------------------------------------

// Color Snippet Reference Data Info

USTRUCT(BlueprintType, meta = (DisplayName = "Color Snippet Reference Data Info"))
struct FRVPDPColorSnippetReferenceDataInfo {

	GENERATED_BODY()
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Information about the Stored Snippet, such as which Color Snippet Data Asset we can get the colors, if it was part of a Group, if so the Relative Location of the group. "))
	TMap<FString, FRVPDPStoredColorSnippetInfo> ColorSnippetsStorageInfo;
};


//-------------------------------------------------------

// Color Snippet Data Info

USTRUCT(BlueprintType, meta = (DisplayName = "Color Snippet Data Info"))
struct FRVPDPColorSnippetDataInfo {

	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Object the colors belong to"))
	TSoftObjectPtr<UObject> ObjectColorSnippetBelongsTo = nullptr;


	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The Color Snippet Data Per LOD"))
	TArray<FVertexDetectMeshDataPerLODStruct> ColorSnippetDataPerLOD;
};


//-------------------------------------------------------

// Group Color Snippet Info

USTRUCT(BlueprintType, meta = (DisplayName = "Group Color Snippet Info"))
struct FRVPDPGroupColorSnippetInfo {

	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	FString GroupSnippetID = "";

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	TSoftObjectPtr<UVertexPaintColorSnippetDataAsset> ColorSnippetDataAssetStoredOn;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	TArray<TSoftObjectPtr<UObject>> AssociatedGroupMeshes;
};


//-------------------------------------------------------

// Color Snippet Data Asset Info

USTRUCT(BlueprintType, meta = (DisplayName = "Color Snippet Data Asset Info"))
struct FRVPDPColorSnippetDataAssetInfo {

	GENERATED_BODY()

	bool operator==(const FRVPDPColorSnippetDataAssetInfo& Other) const {

		if (ColorSnippetID == Other.ColorSnippetID && 
			ColorSnippetDataAssetSnippetIsStoredIn == Other.ColorSnippetDataAssetSnippetIsStoredIn) {

			return true;
		}

		return false;
	}

	bool operator!=(const FRVPDPColorSnippetDataAssetInfo& Other) const {

		return !(*this == Other);
	}


	UPROPERTY(BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The Color Snippet Tag but in FString Format"))
	FString ColorSnippetID = "";

	// Gets set after we've loaded in the data asset so we in async task can de-serialize the color snippet from it and get the colors in TArray<FColor> Format. 
	UPROPERTY()
	UVertexPaintColorSnippetDataAsset* ColorSnippetDataAssetSnippetIsStoredIn = nullptr;
};