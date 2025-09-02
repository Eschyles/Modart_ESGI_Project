// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "ColorSnippetPrerequisites.h"
#include "RVPDPEditorSubsystem.generated.h"


class UVertexPaintColorSnippetDataAsset;
class UVertexPaintColorSnippetRefs;
class UVertexPaintOptimizationDataAsset;
class UVertexPaintMaterialDataAsset;
class UButton;
class UComboBoxString;


UCLASS()
class RVPDPEDITOR_API URVPDPEditorSubsystem : public UEditorSubsystem {

	GENERATED_BODY()

private:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;

public:

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Adds a Editor Notification in the Bottom Right Corner"))
		void AddEditorNotification(FString Notification);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Can set Button Settings depending on Engine Version"))
		void SetEngineSpecificButtonSettings(TArray<UButton*> Buttons, TArray<UComboBoxString*> ComboBoxes);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Auto Saves this package so the user doesn't have to manually press save after doing changes"))
		bool SavePackageWrapper(UObject* PackageToSave);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin")
		void RegisterSkeletalMeshMaxAmountOfLODsToPaint(USkeletalMesh* SkeletalMesh, int MaxAmountOfLODsToPaint, bool& Success);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin")
		void UnRegisterSkeletalMeshMaxAmountOfLODsToPaint(USkeletalMesh* SkeletalMesh, bool& Success);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin")
		void RegisterSkeletalMeshBoneInfo(USkeletalMesh* SkeletalMesh, bool& Success);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin")
		void UnRegisterSkeletalMeshBoneInfo(USkeletalMesh* SkeletalMesh, bool& Success);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin")
		void RegisterMeshPropogateLODInfo(UObject* MeshAssets, bool& Success);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin")
		void UnRegisterMeshPropogateLODInfo(UObject* MeshAsset, bool& Success);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin")
		void AddStaticMeshAndAmountOfLODsToPaintByDefault(UStaticMesh* StaticMesh, int MaxAmountOfLODsToPaint, bool& Success);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin")
		void RemoveStaticMeshAndAmountOfLODsToPaintByDefault(UStaticMesh* StaticMesh, bool& Success);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin")
		void AddMeshColorSnippet(UPrimitiveComponent* MeshComponent, FString ColorSnippetID, bool IsPartOfAGroupSnippet, FString GroupSnippedID, FVector RelativeLocationToGroupCenterPoint, float DotProductToGroupCenterPoint, UVertexPaintColorSnippetDataAsset* ColorSnippetDataAsset, bool& Success);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin")
		void AddGroupColorSnippet(FRVPDPGroupColorSnippetInfo GroupSnippetInfo, bool& Success);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin")
		void RemoveGroupColorSnippet(FString GroupSnippetID, bool& Success);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin")
		void MoveMeshColorSnippet(FString ColorSnippetID, UVertexPaintColorSnippetDataAsset* ColorSnippetDataAssetToMoveFrom, UVertexPaintColorSnippetDataAsset* ColorSnippetDataAssetToMoveTo, bool& Success);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin")
		void MoveGroupColorSnippet(FString ColorSnippetID, UVertexPaintColorSnippetDataAsset* ColorSnippetDataAssetToMoveTo);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin")
		void UpdateMeshColorSnippetID(FString PrevColorSnippetID, FString NewColorSnippetID, bool IsPartOfGroupSnippet, FString GroupSnippedID, UVertexPaintColorSnippetDataAsset* ColorSnippetDataAsset, bool& Success);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin")
		void UpdateGroupColorSnippetID(FString PrevGroupColorSnippetID, FString NewGroupColorSnippetID);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin")
		void RemoveMeshColorSnippet(FString ColorSnippetID, UVertexPaintColorSnippetDataAsset* ColorSnippetDataAsset);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin")
		bool ApplyVertexColorToMeshAtLOD0(UPrimitiveComponent* MeshComponent, TArray<FColor> VertexColorsAtLOD0);

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin")
		int GetStaticMeshVerticesAmount_Wrapper(UStaticMesh* Mesh, int Lod);

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin")
		int GetSkeletalMeshVerticesAmount_Wrapper(USkeletalMesh* Mesh, int Lod);

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin")
		int GetStaticMeshLODCount_Wrapper(UStaticMesh* Mesh);

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin")
		int GetSkeletalMeshLODCount_Wrapper(USkeletalMesh* SkeletalMesh);

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin")
		int GetSkeletalMeshComponentVertexCount_Wrapper(USkeletalMeshComponent* SkeletalMeshComponent, int Lod);

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin")
		int GetStaticMeshComponentVertexCount_Wrapper(UStaticMeshComponent* StaticMeshComponent, int Lod);

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin", meta = (WorldContext = "WorldContextObject"))
		UWorld* GetPersistentLevelsWorld_Wrapper(const UObject* WorldContext);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Sets which Optimization Data Asset on the custom settings. Also tells the subsystem to update it's references"))
		void SetCustomSettingsOptimizationsDataAssetToUse(UObject* DataAsset);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Sets which vertex paint material to use that stores all materials we've added with vertexcolors. "))
		void SetCustomSettingsVertexPaintMaterialToUse(UObject* DataAsset);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Sets which color snippet reference data asset to use that stores references to Color snippet Data Asset. "))
		void SetCustomSettingsVertexPaintColorSnippetReferenceDataAssetToUse(UObject* DataAsset);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin")
		void LoadEssentialDataAssets();

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin")
		bool AddColorSnippetTag(FString Tag);

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin")
		bool DoesColorSnippetTagExist(FString Tag);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin")
		TArray<FString> GetAllColorSnippetTagsDirectlyFromIni();

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin")
		bool RemoveColorSnippetTag(FString Tag);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin")
		void ClearAllAvailableColorSnippetCacheTagContainer();

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin")
		void RenameColorSnippetTag(FString OldTag, FString NewTag);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin")
		void RefreshAllAvailableCachedColorSnippetTagContainer();


	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Editor Utilities", meta = (ToolTip = "Gets all the objects of a specific class from the project. Note that it doesn't work with BP assets. \nIf Editor Only!"))
		TArray<UObject*> GetObjectsOfClass(TSubclassOf<UObject> ObjectsToGet);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin|Editor Utilities", meta = (ToolTip = "Blueprint callable function to get the Object path from soft ptr ref so we can avoid resolving them and bringing them into memory in order to display path names etc. \nIf Editor Only!"))
		FSoftObjectPath GetSoftObjectPathFromSoftObjectPtr(TSoftObjectPtr<UObject> Object);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin|Editor Utilities", meta = (ToolTip = "Blueprint callable function to get the Object name from soft ptr ref so we can avoid resolving them and bringing them into memory in order to display names etc. \nIf Editor Only!"))
		FString GetObjectNameFromSoftObjectPtr(TSoftObjectPtr<UObject> Object);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Editor Utilities", meta = (ToolTip = "Gets all the objects of a specific class from the project but in soft pointer form. \nNOTE Loading all things may be very heavy (and not necessary) if getting things like Materials or Static Meshes! \nIf Editor Only!", DeterminesOutputType = "ObjectsToGet"))
		TArray<TSoftObjectPtr<UObject>> GetObjectsOfClassAsSoftPtrs(TSubclassOf<UObject> ObjectsToGet, bool LoadObjects);


private:

	// NOTE We still have the old name of the plugin here when creating and getting color snippets, otherwise we would have issues getting old color snippets. 
	FString ColorSnippetDevComments = "Runtime Vertex Color Paint & Detection Plugin";
};
