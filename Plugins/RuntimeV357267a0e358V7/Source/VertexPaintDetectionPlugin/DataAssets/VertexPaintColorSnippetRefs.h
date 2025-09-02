// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ColorSnippetPrerequisites.h"
#include "GameplayTagContainer.h"
#include "VertexPaintColorSnippetRefs.generated.h"


class UVertexPaintColorSnippetDataAsset;


UCLASS(Blueprintable, BlueprintType)
class VERTEXPAINTDETECTIONPLUGIN_API UVertexPaintColorSnippetRefs : public UDataAsset {

	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Color Snippets")
	TMap<FString, FRVPDPStoredColorSnippetInfo> GetAllColorSnippetsAndDataAssetForObject(const UObject* Object);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Color Snippets", meta = (ToolTip = "Gets all registered color snippets in specified Data Asset. NOTE Intended for Editor Scripting, uses LoadSynchronous etc. so not recommended in runtime. Can optionally include all group snipepts as well, i.e. the one that gets appended with _0, _1 etc.  "))
	TMap<FString, FRVPDPStoredColorSnippetInfo> GetAllColorSnippetsInSpecifiedDataAsset(const UVertexPaintColorSnippetDataAsset* ColorSnippetDataAsset, bool IncludeChildGroupSnippets);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Color Snippets", meta = (ToolTip = "Get All Group Snippet in the specific color snippet data asset. "))
	TMap<FString, FRVPDPGroupColorSnippetInfo> GetAllGroupSnippetsInSpecifiedDataAsset(const UVertexPaintColorSnippetDataAsset* ColorSnippetDataAsset);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Color Snippets", meta = (WorldContext = "WorldContextObject", ToolTip = "Get all the Color Snippet Infos such as which data asset its stored at, relative location, dot product etc. for snippets belonging to a group. "))
	TMap<FString, FRVPDPStoredColorSnippetInfo> GetChildSnippetInfosAssociatedWithGroupSnippetID(const UObject* WorldContextObject, const FString& GroupSnippetID);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Color Snippets", meta = (WorldContext = "WorldContextObject", ToolTip = "Will get and check if the meshComponent we pass through, matches what the Group Snippet ID requires, i.e. the correct amount of components, with right source mesh, relative location etc. If any of the meshes it at an off location or rotated differently then it will fail, since it doesn't fully match and we shouldn't be allowed to Paint / Preview it.  "))
	bool CheckAndGetTheComponentsThatMatchGroupChildSnippets(const UObject* WorldContextObject, const FString& GroupSnippetID, TArray<UPrimitiveComponent*> MeshComponents, TMap<FString, UPrimitiveComponent*>& ChildSnippetsAndMatchingMeshes);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Color Snippets", meta = (ToolTip = " "))
	bool ContainsColorSnippet(FString SnippetID, bool OptionalHasToBeStoredInDataAsset, UVertexPaintColorSnippetDataAsset* DataAsset);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Color Snippets", meta = (ToolTip = " "))
	TSoftObjectPtr<UObject> GetObjectFromSnippetID(const FString& SnippetID);

	void RemoveSnippetObject(const UObject* Object);

	void RemoveColorSnippet(const FString& SnippetID);



	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin|Color Snippets")
		TMap<TSoftObjectPtr<UObject>, FRVPDPColorSnippetReferenceDataInfo> StaticMeshesColorSnippets;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin|Color Snippets")
		TMap<TSoftObjectPtr<UObject>, FRVPDPColorSnippetReferenceDataInfo> SkeletalMeshesColorSnippets;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin|Color Snippets")
		TMap<FString, FRVPDPGroupColorSnippetInfo> GroupSnippetsAndAssociatedMeshes;

	// You can add new tags even if visible anywhere so we skip that for this
	UPROPERTY(/*VisibleAnywhere*/)
		FGameplayTagContainer AllAvailableColorSnippets;
};
