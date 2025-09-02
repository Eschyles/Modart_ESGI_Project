// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 


#include "VertexPaintFunctionLibrary.h"

// Engine
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Engine/EngineTypes.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Materials/MaterialInstance.h"
#include "PhysicsEngine/BodySetup.h"
#include "Engine/GameInstance.h"
#include "Runtime/Engine/Classes/Engine/LatentActionManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/AssetManager.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"
#include "Engine/SkeletalMesh.h"
#include "Components/SkeletalMeshComponent.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Math/GenericOctree.h"

// Plugin
#include "AutoAddColorComponent.h"
#include "ColorsOfEachChannelLatentAction.h"
#include "ColorsOfEachChannelAsyncTask.h"
#include "UpdateClothPhysicsAsyncTask.h"
#include "UpdateClothPhysicsLatentAction.h"
#include "GetColorSnippetColorsLatentAction.h"
#include "AsyncLoadSoftPtrsLatentAction.h"
#include "VertexPaintDetectionInterface.h"
#include "VertexPaintMaterialDataAsset.h"
#include "VertexPaintDetectionSettings.h"
#include "VertexPaintColorSnippetRefs.h"
#include "VertexPaintColorSnippetDataAsset.h"
#include "VertexPaintOptimizationDataAsset.h"
#include "VertexPaintDetectionTaskQueue.h"
#include "VertexPaintDetectionLog.h"
#include "VertexPaintDetectionProfiling.h"
#include "VertexPaintDetectionGISubSystem.h" // Circular dependency here strictly to make things more user friendly where users can get things from the GI using getters from the function library, rather than having to remember that some things are in the GI and they have to get that first etc. 

// UE5
#if ENGINE_MAJOR_VERSION == 5

#include "Components/DynamicMeshComponent.h"

#include "GeometryCollection/GeometryCollectionComponent.h"
#include "GeometryCollection/GeometryCollection.h"
#include "GeometryCollection/GeometryCollectionObject.h"
#include "ChaosCloth/ChaosClothingSimulationInteractor.h"
#include "ClothingAsset.h"

#include "PhysicsEngine/PhysicsAsset.h"

#if ENGINE_MINOR_VERSION >= 2
#include "StaticMeshComponentLODInfo.h"
#include "Engine/Level.h"
#endif

#if ENGINE_MINOR_VERSION >= 4
#include "Rendering/RenderCommandPipes.h"
#endif

#if ENGINE_MINOR_VERSION >= 5
#include "PhysicsEngine/SkeletalBodySetup.h"
#endif

#endif


//--------------------------------------------------------

// Is World Valid

bool UVertexPaintFunctionLibrary::IsWorldValid(const UWorld* World) {

	if (!World) return false;
	if (!IsValid(World)) return false;
	if (World->bIsTearingDown) return false;
	if (!World->IsGameWorld()) return false;

	return true;
}


//--------------------------------------------------------

// Get Vertex Paint Game Instance Subsystem

UVertexPaintDetectionGISubSystem* UVertexPaintFunctionLibrary::GetVertexPaintGameInstanceSubsystem(const UObject* WorldContextObject) {

	if (!IsValid(WorldContextObject)) return nullptr;

	if (UGameInstance* gameInstance = UGameplayStatics::GetGameInstance(WorldContextObject)) {

		if (UVertexPaintDetectionGISubSystem* gameInstanceSubsystem = gameInstance->GetSubsystem<UVertexPaintDetectionGISubSystem>())
			return gameInstanceSubsystem;
	}

	return nullptr;
}


//-------------------------------------------------------

// Get Calculate Colors Paint Task Amount

TMap<UPrimitiveComponent*, int> UVertexPaintFunctionLibrary::GetCalculateColorsPaintTasksAmount_Wrapper(const UObject* WorldContextObject) {

	if (auto paintTaskQueue = GetVertexPaintTaskQueue(WorldContextObject))
		return paintTaskQueue->GetCalculateColorsPaintTasksAmountPerComponent();

	return TMap<UPrimitiveComponent*, int>();
}


//-------------------------------------------------------

// Get Calculate Colors Detection Task Amount

TMap<UPrimitiveComponent*, int> UVertexPaintFunctionLibrary::GetCalculateColorsDetectionTasksAmount_Wrapper(const UObject* WorldContextObject) {

	if (auto paintTaskQueue = GetVertexPaintTaskQueue(WorldContextObject))
		return paintTaskQueue->GetCalculateColorsDetectionTasksAmountPerComponent();

	return TMap<UPrimitiveComponent*, int>();
}


//-------------------------------------------------------

// Get Amount Of On Going Paint Tasks

int UVertexPaintFunctionLibrary::GetAmountOfOnGoingPaintTasks(const UObject* WorldContextObject) {


	if (auto paintTaskQueue = GetVertexPaintTaskQueue(WorldContextObject))
		return paintTaskQueue->GetAmountOfOnGoingPaintTasks();

	return 0;
}


//-------------------------------------------------------

// Get Amount Of On Going Detect Tasks

int UVertexPaintFunctionLibrary::GetAmountOfOnGoingDetectTasks(const UObject* WorldContextObject) {


	if (auto paintTaskQueue = GetVertexPaintTaskQueue(WorldContextObject))
		return paintTaskQueue->GetAmountOfOnGoingDetectTasks();

	return 0;
}


//--------------------------------------------------------

// Get All Available Color Snippets on Mesh

void UVertexPaintFunctionLibrary::GetAllMeshColorSnippetsAsString_Wrapper(UPrimitiveComponent* MeshComponent, TMap<FString, FRVPDPStoredColorSnippetInfo>& AvailableColorSnippetsAndDataAssets) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetAllMeshColorSnippetsAsString_Wrapper);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - GetAllMeshColorSnippetsAsString_Wrapper");

	AvailableColorSnippetsAndDataAssets.Empty();

	if (!IsValid(MeshComponent)) return;


	auto colorSnippetReferenceDataAsset = GetColorSnippetReferenceDataAsset(MeshComponent);

	if (!colorSnippetReferenceDataAsset) {

		RVPDP_LOG(FRVPDPDebugSettings(false, 0, true), FColor::Cyan, "GetAllAvailableColorSnippetsOnMesh Fail because there's no Color Snippet Reference Data Asset Set in Settings so we can't get color snippet data assets a mesh has! ");

		return;
	}


	TMap<FString, FRVPDPStoredColorSnippetInfo> colorSnippets;

	if (auto staticMeshComp = Cast<UStaticMeshComponent>(MeshComponent)) {

		colorSnippets = colorSnippetReferenceDataAsset->GetAllColorSnippetsAndDataAssetForObject(staticMeshComp->GetStaticMesh());
	}

	else if (auto skeletalMeshComponent = Cast<USkeletalMeshComponent>(MeshComponent)) {

		const UObject* meshAsset = UVertexPaintFunctionLibrary::GetMeshComponentSourceMesh(skeletalMeshComponent);

		colorSnippets = colorSnippetReferenceDataAsset->GetAllColorSnippetsAndDataAssetForObject(meshAsset);
	}

	// If is stored on a data asset
	if (colorSnippets.Num() > 0)
		AvailableColorSnippetsAndDataAssets = colorSnippets;
}


//--------------------------------------------------------

// Get All Mesh Color Snippets As Tags

void UVertexPaintFunctionLibrary::GetAllMeshColorSnippetsAsTags_Wrapper(UPrimitiveComponent* MeshComponent, TMap<FGameplayTag, TSoftObjectPtr<UVertexPaintColorSnippetDataAsset>>& AvailableColorSnippetTagsAndDataAssets) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetAllMeshColorSnippetsAsTags_Wrapper);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - GetAllMeshColorSnippetsAsTags_Wrapper");

	AvailableColorSnippetTagsAndDataAssets.Empty();

	if (!IsValid(MeshComponent) || !GetColorSnippetReferenceDataAsset(MeshComponent)) return;


	TMap<FString, FRVPDPStoredColorSnippetInfo> availableColorSnippetsAndDataAssets;
	GetAllMeshColorSnippetsAsString_Wrapper(MeshComponent, availableColorSnippetsAndDataAssets);


	if (availableColorSnippetsAndDataAssets.Num() > 0) {

		TArray<FString> availableColorSnippets;
		availableColorSnippetsAndDataAssets.GetKeys(availableColorSnippets);

		TArray<FRVPDPStoredColorSnippetInfo> colorSnippetDataAssets;
		availableColorSnippetsAndDataAssets.GenerateValueArray(colorSnippetDataAssets);


		FGameplayTag topLevelTag = FGameplayTag::RequestGameplayTag(FName("ColorSnippets"));
		FGameplayTagContainer tagContainer;
		tagContainer.AddTag(FGameplayTag::RequestGameplayTag(topLevelTag.GetTagName(), false));
		FGameplayTagContainer availableColorSnippetTagsContainer = GetColorSnippetReferenceDataAsset(MeshComponent)->AllAvailableColorSnippets.Filter(tagContainer);


		TArray<FGameplayTag> allAvailableTagsUnderCategory;
		availableColorSnippetTagsContainer.GetGameplayTagArray(allAvailableTagsUnderCategory);


		for (const FGameplayTag& tag : allAvailableTagsUnderCategory) {

			// If Mesh has Snippet ID that matches Tag under category 
			if (availableColorSnippets.Contains(tag.GetTagName().ToString())) {

				int arrayIndex = -1;

				availableColorSnippets.Find(tag.GetTagName().ToString(), arrayIndex);

				if (colorSnippetDataAssets.IsValidIndex(arrayIndex))
					AvailableColorSnippetTagsAndDataAssets.Add(tag, colorSnippetDataAssets[arrayIndex].ColorSnippetDataAssetStoredOn);
			}
		}
	}
}


//--------------------------------------------------------

// Get All Tags Under Tag Category

TArray<FGameplayTag> UVertexPaintFunctionLibrary::GetAllTagsUnderTagCategory(const UObject* WorldContextObject, FGameplayTag TagCategory) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetAllTagsUnderTagCategory);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - GetAllTagsUnderTagCategory");

	if (!TagCategory.IsValid() || !WorldContextObject) return TArray<FGameplayTag>();
	if (!GetColorSnippetReferenceDataAsset(WorldContextObject)) return TArray<FGameplayTag>();


	FGameplayTagContainer tagContainer;
	tagContainer.AddTag(FGameplayTag::RequestGameplayTag(TagCategory.GetTagName(), false));
	FGameplayTagContainer allAvailableColorSnippetTagContainer = GetColorSnippetReferenceDataAsset(WorldContextObject)->AllAvailableColorSnippets.Filter(tagContainer);

	TArray<FGameplayTag> allAvailableTagsUnderCategory;
	allAvailableColorSnippetTagContainer.GetGameplayTagArray(allAvailableTagsUnderCategory);

	return allAvailableTagsUnderCategory;
}


//--------------------------------------------------------

// Get All Color Snippets Under Group Snippet As String

TArray<FString> UVertexPaintFunctionLibrary::GetAllColorSnippetsUnderGroupSnippetAsString(const UObject* WorldContextObject, const FString& GroupSnippetID) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetAllColorSnippetsUnderGroupSnippetAsString);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - GetAllColorSnippetsUnderGroupSnippetAsString");

	if (!WorldContextObject) return TArray<FString>();
	if (GroupSnippetID.IsEmpty()) return TArray<FString>();
	if (!GetColorSnippetReferenceDataAsset(WorldContextObject)) TArray<FString>();


	TArray<FString> tagsAsString;

	for (const FGameplayTag& tag : GetColorSnippetReferenceDataAsset(WorldContextObject)->AllAvailableColorSnippets) {

		if (tag.MatchesTag(FGameplayTag::RequestGameplayTag(FName(*GroupSnippetID))))
			tagsAsString.Add(tag.ToString());
	}


	return tagsAsString;
}


//--------------------------------------------------------

// Get All Mesh Color Snippets Tags Under Tag Category

void UVertexPaintFunctionLibrary::GetAllMeshColorSnippetsTagsUnderTagCategory_Wrapper(UPrimitiveComponent* MeshComponent, FGameplayTag TagCategory, TMap<FGameplayTag, TSoftObjectPtr<UVertexPaintColorSnippetDataAsset>>& AvailableColorSnippetTagsAndDataAssetsUnderTagCategory) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetAllMeshColorSnippetsTagsUnderTagCategory_Wrapper);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - GetAllMeshColorSnippetsTagsUnderTagCategory_Wrapper");

	AvailableColorSnippetTagsAndDataAssetsUnderTagCategory.Empty();

	if (!IsValid(MeshComponent) || !GetColorSnippetReferenceDataAsset(MeshComponent)) return;


	TMap<FGameplayTag, TSoftObjectPtr<UVertexPaintColorSnippetDataAsset>> availableColorSnippetTagsAndDataAssets;
	GetAllMeshColorSnippetsAsTags_Wrapper(MeshComponent, availableColorSnippetTagsAndDataAssets);

	TArray<FGameplayTag> availableColorSnippetTags;
	availableColorSnippetTagsAndDataAssets.GetKeys(availableColorSnippetTags);

	TArray<TSoftObjectPtr<UVertexPaintColorSnippetDataAsset>> colorSnippetDataAssets;
	availableColorSnippetTagsAndDataAssets.GenerateValueArray(colorSnippetDataAssets);


	for (const FGameplayTag& tag : GetAllTagsUnderTagCategory(MeshComponent, TagCategory)) {

		// If Mesh has Snippet Tag that matches Tag under category 
		if (availableColorSnippetTags.Contains(tag)) {

			int arrayIndex = -1;
			availableColorSnippetTags.Find(tag, arrayIndex);

			if (colorSnippetDataAssets.IsValidIndex(arrayIndex))
				AvailableColorSnippetTagsAndDataAssetsUnderTagCategory.Add(tag, colorSnippetDataAssets[arrayIndex]);
		}
	}
}


//--------------------------------------------------------

// Get Mesh Color Snippets Tags In Tag Category

void UVertexPaintFunctionLibrary::GetMeshColorSnippetsTagsInTagCategory_Wrapper(UPrimitiveComponent* MeshComponent, FGameplayTag TagCategory, TMap<FGameplayTag, TSoftObjectPtr<UVertexPaintColorSnippetDataAsset>>& AvailableColorSnippetTagsAndDataAssetsUnderTagCategory) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetMeshColorSnippetsTagsInTagCategory_Wrapper);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - GetMeshColorSnippetsTagsInTagCategory_Wrapper");

	AvailableColorSnippetTagsAndDataAssetsUnderTagCategory.Empty();

	if (!IsValid(MeshComponent) || !GetColorSnippetReferenceDataAsset(MeshComponent)) return;


	TMap<FGameplayTag, TSoftObjectPtr<UVertexPaintColorSnippetDataAsset>> availableColorSnippetTagsAndDataAssets;
	GetAllMeshColorSnippetsAsTags_Wrapper(MeshComponent, availableColorSnippetTagsAndDataAssets);

	TArray<FGameplayTag> availableColorSnippetTags;
	availableColorSnippetTagsAndDataAssets.GetKeys(availableColorSnippetTags);

	TArray<TSoftObjectPtr<UVertexPaintColorSnippetDataAsset>> colorSnippetDataAssets;
	availableColorSnippetTagsAndDataAssets.GenerateValueArray(colorSnippetDataAssets);


	for (const FGameplayTag& tag : GetAllTagsUnderTagCategory(MeshComponent, TagCategory)) {

		FString tagAsString = tag.GetTagName().ToString();

		// Plus 1 so we don't have the . after the parent category
		FString tagStringWithoutParentCategory = tagAsString.RightChop(TagCategory.GetTagName().ToString().Len() + 1);

		int charIndexWithPeriod = -1;

		// If has a ., i.e. this is not tag that is directly in the tag category but another under that. 
		if (tagStringWithoutParentCategory.FindChar('.', charIndexWithPeriod))
			continue;


		// If Mesh has Snippet Tag that matches Tag under category 
		if (availableColorSnippetTags.Contains(tag)) {

			int arrayIndex = -1;
			availableColorSnippetTags.Find(tag, arrayIndex);

			if (colorSnippetDataAssets.IsValidIndex(arrayIndex))
				AvailableColorSnippetTagsAndDataAssetsUnderTagCategory.Add(tag, colorSnippetDataAssets[arrayIndex]);
		}
	}
}


//--------------------------------------------------------

// Get Mesh Color Snippet Tag In Group

FGameplayTag UVertexPaintFunctionLibrary::GetMeshColorSnippetChildFromGroupSnippet(UPrimitiveComponent* MeshComponent, const FGameplayTag& GroupSnippet, TArray<UPrimitiveComponent*> GroupSnippetMeshes, bool& Success) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetMeshColorSnippetChildFromGroupSnippet);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - GetMeshColorSnippetChildFromGroupSnippet");

	Success = false;

	if (!MeshComponent) return FGameplayTag();
	if (!GroupSnippet.IsValid()) return FGameplayTag();
	if (GroupSnippetMeshes.Num() == 0) return FGameplayTag();
	if (!GetColorSnippetReferenceDataAsset(MeshComponent)) return FGameplayTag();



	TMap<FString, UPrimitiveComponent*> childSnippetsAndMatchingMeshes;
	if (GetColorSnippetReferenceDataAsset(MeshComponent)->CheckAndGetTheComponentsThatMatchGroupChildSnippets(MeshComponent, GroupSnippet.ToString(), GroupSnippetMeshes, childSnippetsAndMatchingMeshes)) {

		TArray<FString> childSnippets;
		childSnippetsAndMatchingMeshes.GetKeys(childSnippets);

		TArray<UPrimitiveComponent*> meshesInGroup;
		childSnippetsAndMatchingMeshes.GenerateValueArray(meshesInGroup);

		int meshGroupIndex = -1;

		if (meshesInGroup.Find(MeshComponent, meshGroupIndex)) {

			FString childSnippetString = childSnippets[meshGroupIndex];
			FName childSnippetName = FName(*childSnippetString);

			FGameplayTag childSnippetTag = FGameplayTag::RequestGameplayTag(childSnippetName, false);

			Success = childSnippetTag.IsValid();

			return childSnippetTag;
		}
	}

	return FGameplayTag();
}


//--------------------------------------------------------

// Get Amount Of Color Snippet Childs From Group Snippet

int UVertexPaintFunctionLibrary::GetAmountOfColorSnippetChildsFromGroupSnippet(const UObject* WorldContextObject, FGameplayTag GroupSnippet) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetAmountOfColorSnippetChildsFromGroupSnippet);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - GetAmountOfColorSnippetChildsFromGroupSnippet");

	if (!WorldContextObject) return 0;
	if (!GroupSnippet.IsValid()) return 0;


	return GetAllTagsUnderTagCategory(WorldContextObject, GroupSnippet).Num();
}


//--------------------------------------------------------

// Get The Group Snippet A Child Snippet Belongs To

bool UVertexPaintFunctionLibrary::GetTheGroupSnippetAChildSnippetBelongsTo(const UObject* WorldContextObject, FGameplayTag childSnippet, FGameplayTag& groupSnippetChildBelongsTo) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetTheGroupSnippetAChildSnippetBelongsTo);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - GetTheGroupSnippetAChildSnippetBelongsTo");

	groupSnippetChildBelongsTo = FGameplayTag();

	if (!WorldContextObject) return false;
	if (!GetColorSnippetReferenceDataAsset(WorldContextObject)) return false;
	if (!childSnippet.ToString().StartsWith("ColorSnippets.GroupSnippets.", ESearchCase::Type::CaseSensitive)) return false;


	FString groupSnippetAsString = "";
	int32 childPartWeWwantToCut;

	if (childSnippet.ToString().FindLastChar('.', childPartWeWwantToCut))
		groupSnippetAsString = childSnippet.ToString().Left(childPartWeWwantToCut);
	else
		return false;

	if (!GetColorSnippetReferenceDataAsset(WorldContextObject)->GroupSnippetsAndAssociatedMeshes.Contains(groupSnippetAsString)) return false;


	FName childSnippetName = FName(*groupSnippetAsString);
	groupSnippetChildBelongsTo = FGameplayTag::RequestGameplayTag(childSnippetName, false);

	return true;
}


//--------------------------------------------------------

// Get Color Snippet Reference Data Asset

UVertexPaintColorSnippetRefs* UVertexPaintFunctionLibrary::GetColorSnippetReferenceDataAsset(const UObject* OptionalWorldContextObject) {

	if (OptionalWorldContextObject) {

		if (auto gameInstanceSubsystem = GetVertexPaintGameInstanceSubsystem(OptionalWorldContextObject))
			return gameInstanceSubsystem->GetCachedColorSnippetReferenceDataAsset();
	}

	// If can't provide a valid world we can still get it if in game thread. But if async then we need a valid world so we can get the cached one since it's not safe to run .Get() and ofc. LoadSynchronous in async. 
	if (IsInGameThread()) {

		if (auto vertexPaintDetectionSettings = GetDefault<UVertexPaintDetectionSettings>())
			return vertexPaintDetectionSettings->ColorSnippetReferencesDataAssetToUse.LoadSynchronous();
	}

	return nullptr;
}


//--------------------------------------------------------

// Get Optimization Data Asset

UVertexPaintOptimizationDataAsset* UVertexPaintFunctionLibrary::GetOptimizationDataAsset(const UObject* OptionalWorldContextObject) {


	if (OptionalWorldContextObject) {

		if (auto gameInstanceSubsystem = GetVertexPaintGameInstanceSubsystem(OptionalWorldContextObject))
			return gameInstanceSubsystem->GetCachedOptimizationDataAsset();
	}

	// If can't provide a valid world we can still get it if in game thread. But if async then we need a valid world so we can get the cached one since it's not safe to run .Get() and ofc. LoadSynchronous in async. 
	if (IsInGameThread()) {

		if (auto vertexPaintDetectionSettings = GetDefault<UVertexPaintDetectionSettings>())
			return vertexPaintDetectionSettings->OptimizationDataAssetToUse.LoadSynchronous();
	}

	return nullptr;
}


//--------------------------------------------------------

// Get Vertex Paint Material Data Asset

UVertexPaintMaterialDataAsset* UVertexPaintFunctionLibrary::GetVertexPaintMaterialDataAsset(const UObject* OptionalWorldContextObject) {


	if (OptionalWorldContextObject && IsWorldValid(OptionalWorldContextObject->GetWorld())) {

		if (auto gameInstanceSubsystem = GetVertexPaintGameInstanceSubsystem(OptionalWorldContextObject))
			return gameInstanceSubsystem->GetCachedMaterialDataAsset();
	}

	// If can't provide a valid world we can still get it if in game thread. But if async then we need a valid world so we can get the cached one since it's not safe to run .Get() and ofc. LoadSynchronous in async. 
	if (IsInGameThread()) {

		if (auto vertexPaintDetectionSettings = GetDefault<UVertexPaintDetectionSettings>())
			return vertexPaintDetectionSettings->MaterialsDataAssetToUse.LoadSynchronous();
	}

	return nullptr;
}


//-------------------------------------------------------

// Get Max LOD To Paint On

int UVertexPaintFunctionLibrary::GetAmountOfLODsToPaintOn(UPrimitiveComponent* MeshComp, bool OverrideLODToPaintUpOn, int OverrideUpToLOD) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetAmountOfLODsToPaintOn);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - GetAmountOfLODsToPaintOn");

	if (!IsValid(MeshComp)) return 1;


	int lodsToPaint = 1;
	int maxLODsAvailable = 1;


	if (auto staticMesh = Cast<UStaticMeshComponent>(MeshComp)) {

		if (!staticMesh->GetStaticMesh()) return 1;

		if (FStaticMeshRenderData* staticMeshRenderData = staticMesh->GetStaticMesh()->GetRenderData())
			maxLODsAvailable = staticMeshRenderData->LODResources.Num();
		else 
			return 1;


		lodsToPaint = maxLODsAvailable;


		if (auto optimizationDataAsset = GetOptimizationDataAsset(MeshComp)) {


			const int32 LODsLimitToPaint = optimizationDataAsset->GetStaticMeshMaxNumOfLODsToPaint(staticMesh->GetStaticMesh());

			// If anything is registered
			if (LODsLimitToPaint > 0) {

				if (LODsLimitToPaint <= maxLODsAvailable)
					lodsToPaint = LODsLimitToPaint;
			}
		}


		if (OverrideLODToPaintUpOn) {

			if (maxLODsAvailable >= OverrideUpToLOD) {

				return OverrideUpToLOD;
			}

			else {

				// If lodsToPaint was less than override LOD, then just returns the maximum LODs we have so we get as close to the Override LOD 
				return maxLODsAvailable;
			}
		}

		return lodsToPaint;
	}

	else if (auto skeletalMeshComp = Cast<USkeletalMeshComponent>(MeshComp)) {


		USkeletalMesh* skelMesh = nullptr;

#if ENGINE_MAJOR_VERSION == 4

		skelMesh = skeletalMeshComp->SkeletalMesh;

#elif ENGINE_MAJOR_VERSION == 5

#if ENGINE_MINOR_VERSION == 0

		skelMesh = skeletalMeshComp->SkeletalMesh.Get();

#else

		skelMesh = skeletalMeshComp->GetSkeletalMeshAsset();

#endif
#endif

		if (!skelMesh) return 1;


		if (FSkeletalMeshRenderData* skelMeshRenderData = skeletalMeshComp->GetSkeletalMeshRenderData())
			maxLODsAvailable = skelMeshRenderData->LODRenderData.Num();
		else
			return 1;


		lodsToPaint = maxLODsAvailable;

		if (auto optimizationDataAsset = GetOptimizationDataAsset(MeshComp)) {


			const int32 LODsLimitToPaint = optimizationDataAsset->GetSkeletalMeshMaxNumOfLODsToPaint(skelMesh);

			// If anything is registered
			if (LODsLimitToPaint > 0) {

				if (LODsLimitToPaint <= maxLODsAvailable)
					lodsToPaint = LODsLimitToPaint;
			}
		}


		if (OverrideLODToPaintUpOn) {

			if (maxLODsAvailable >= OverrideUpToLOD) {

				return OverrideUpToLOD;
			}

			else {

				return maxLODsAvailable;
			}
		}

		return lodsToPaint;
	}

	return 1;
}


//--------------------------------------------------------

// Multi Sphere Trace For Closest Unique Meshes And Bones

bool UVertexPaintFunctionLibrary::MultiSphereTraceForClosestUniqueMeshesAndBones_Wrapper(const UObject* WorldContextObject, FVector Location, float Radius, ETraceTypeQuery TraceChannel, const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypesToTraceFor, const TArray<AActor*>& ActorsToIgnore, bool TraceComplex, bool IgnoreSelf, EDrawDebugTrace::Type DrawDebugType, TArray<FRVPDPTraceForClosestUniqueMeshesAndBonesPrerequisite>& ClosestUniqueMeshesWithBones, float DebugDrawTime) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_MultiSphereTraceForClosestUniqueMeshesAndBones);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - MultiSphereTraceForClosestUniqueMeshesAndBones");

	ClosestUniqueMeshesWithBones.Empty();

	if (!WorldContextObject) return false;
	if (Radius <= 0) return false;


	TArray<FHitResult> hitResults;

	// Capsule Trace either with objects or visibility so we could get the Face Index. Wasn't possible with Sphere Trace in case the user wants it. 
	if (ObjectTypesToTraceFor.Num() > 0) {

		// Can't have the start and end be the Exact same vector but had to * 1.00001f so it's a bit different
		if (!UKismetSystemLibrary::SphereTraceMultiForObjects(WorldContextObject, Location, Location * 1.00001f, Radius, ObjectTypesToTraceFor, TraceComplex, ActorsToIgnore, DrawDebugType, hitResults, IgnoreSelf, FLinearColor::Red, FLinearColor::Green, DebugDrawTime))
			return false;
	}
	else {

		if (!UKismetSystemLibrary::SphereTraceMulti(WorldContextObject, Location, Location * 1.00001f, Radius, TraceChannel, TraceComplex, ActorsToIgnore, DrawDebugType, hitResults, IgnoreSelf, FLinearColor::Red, FLinearColor::Green, DebugDrawTime))
			return false;
	}


	TMap<UPrimitiveComponent*, FRVPDPTraceForClosestUniqueMeshesAndBonesPrerequisite> uniqueMeshesAndBonesPerMesh;
	TMap<UPrimitiveComponent*, float> closestDistancesPerMesh;
	TMap<UPrimitiveComponent*, TArray<FName>> hitBonesPerMesh;


	// Makes sure we only get one from each mesh and the closest hit result
	for (int i = 0; i < hitResults.Num(); i++) {

		if (!IsValid(hitResults[i].Component.Get())) continue; 


		const float distanceFromLocation = (hitResults[i].Location - Location).Size();
		TArray<FName> hitBonesOnMesh;
		FRVPDPTraceForClosestUniqueMeshesAndBonesPrerequisite closestUniqueMeshesAndBones;


		// If mesh already been added, but this hit is closer, then updates it
		if (uniqueMeshesAndBonesPerMesh.Contains(hitResults[i].Component.Get())) {


			// Hit Bones
			hitBonesOnMesh = hitBonesPerMesh.FindRef(hitResults[i].Component.Get());

			if (hitResults[i].BoneName != NAME_None)
				hitBonesOnMesh.Add(hitResults[i].BoneName);
			
			hitBonesPerMesh.Add(hitResults[i].Component.Get(), hitBonesOnMesh);


			closestUniqueMeshesAndBones = uniqueMeshesAndBonesPerMesh.FindRef(hitResults[i].Component.Get());

			// Always updates the bones in case the later bones in the hit results isn't closer but still within the trace Radius
			closestUniqueMeshesAndBones.MeshBonesWithinTrace = hitBonesOnMesh;


			// Updates closest hit result
			if (distanceFromLocation < closestDistancesPerMesh.FindRef(hitResults[i].Component.Get())) {

				closestDistancesPerMesh.Add(hitResults[i].Component.Get(), distanceFromLocation);
				closestUniqueMeshesAndBones.ClosestHitResultOfMesh = hitResults[i];
			}

			uniqueMeshesAndBonesPerMesh.Add(hitResults[i].Component.Get(), closestUniqueMeshesAndBones);
		}

		else {

			closestDistancesPerMesh.Add(hitResults[i].Component.Get(), distanceFromLocation);

			if (hitResults[i].BoneName != NAME_None)
				hitBonesOnMesh.Add(hitResults[i].BoneName);

			hitBonesPerMesh.Add(hitResults[i].Component.Get(), hitBonesOnMesh);

			closestUniqueMeshesAndBones.ClosestHitResultOfMesh = hitResults[i];
			closestUniqueMeshesAndBones.MeshBonesWithinTrace = hitBonesOnMesh;

			uniqueMeshesAndBonesPerMesh.Add(hitResults[i].Component.Get(), closestUniqueMeshesAndBones);
		}
	}

	uniqueMeshesAndBonesPerMesh.GenerateValueArray(ClosestUniqueMeshesWithBones);

	return true;
}


//--------------------------------------------------------

// Set Static Mesh And Release Resources

void UVertexPaintFunctionLibrary::SetStaticMeshAndReleaseResources(UStaticMeshComponent* StaticMeshComponent, UStaticMesh* NewMesh, bool ClearVertexColorsOfChangedMesh) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_SetStaticMeshAndReleaseResources);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - SetStaticMeshAndReleaseResources");

	if (!IsValid(StaticMeshComponent)) return;
	if (!IsValid(NewMesh)) return;


	for (int i = 0; i < StaticMeshComponent->LODData.Num(); i++) {

		if (StaticMeshComponent->LODData[i].OverrideVertexColors)
			StaticMeshComponent->LODData[i].ReleaseOverrideVertexColorsAndBlock();
	}


	// Necessary otherwise it had the same amount of LODs as the previous static mesh
	StaticMeshComponent->SetLODDataCount(NewMesh->GetNumLODs(), NewMesh->GetNumLODs());
	StaticMeshComponent->SetStaticMesh(NewMesh);


	if (ClearVertexColorsOfChangedMesh) {

		for (int i = 0; i < StaticMeshComponent->LODData.Num(); i++) {

			int amountOfVertices = NewMesh->GetRenderData()->LODResources[i].VertexBuffers.PositionVertexBuffer.GetNumVertices();
			StaticMeshComponent->LODData[i].OverrideVertexColors = new FColorVertexBuffer();
			StaticMeshComponent->LODData[i].OverrideVertexColors->InitFromSingleColor(FColor(0, 0, 0, 0), amountOfVertices);

			BeginInitResource(StaticMeshComponent->LODData[i].OverrideVertexColors);
		}

		StaticMeshComponent->MarkRenderStateDirty();
	}
}


//--------------------------------------------------------

// Set Skeletal Mesh And Release Resources

void UVertexPaintFunctionLibrary::SetSkeletalMeshAndReleaseResources(USkeletalMeshComponent* SkeletalMeshComponent, USkeletalMesh* NewMesh, bool ClearVertexColorsOfChangedMesh) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_SetSkeletalMeshAndReleaseResources);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - SetSkeletalMeshAndReleaseResources");

	if (!IsValid(SkeletalMeshComponent)) return;
	if (!IsValid(NewMesh)) return;


	FSkelMeshComponentLODInfo* LODInfo = nullptr;

	for (int i = 0; i < SkeletalMeshComponent->LODInfo.Num(); i++) {


		LODInfo = &SkeletalMeshComponent->LODInfo[i];

		if (LODInfo->OverrideVertexColors)
			LODInfo->ReleaseOverrideVertexColorsAndBlock();
	}



#if ENGINE_MAJOR_VERSION == 4

	SkeletalMeshComponent->SetSkeletalMesh(NewMesh, false);

#elif ENGINE_MAJOR_VERSION == 5

#if ENGINE_MINOR_VERSION == 0

	SkeletalMeshComponent->SetSkeletalMesh(NewMesh, false);

#else

	SkeletalMeshComponent->SetSkeletalMeshAsset(NewMesh);

#endif
#endif


	if (ClearVertexColorsOfChangedMesh) {

		for (int i = 0; i < SkeletalMeshComponent->LODInfo.Num(); i++) {

			LODInfo = &SkeletalMeshComponent->LODInfo[i];

			int amountOfVertices = SkeletalMeshComponent->GetSkeletalMeshRenderData()->LODRenderData[i].StaticVertexBuffers.PositionVertexBuffer.GetNumVertices();
			LODInfo->OverrideVertexColors = new FColorVertexBuffer();
			LODInfo->OverrideVertexColors->InitFromSingleColor(FColor(0, 0, 0, 0), amountOfVertices);

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4

			BeginInitResource(LODInfo->OverrideVertexColors, &UE::RenderCommandPipe::SkeletalMesh);
#else
			BeginInitResource(LODInfo->OverrideVertexColors);
#endif

		}

		SkeletalMeshComponent->MarkRenderStateDirty();
	}
}


//--------------------------------------------------------

// Sort String Array Alphabetically

TArray<FString> UVertexPaintFunctionLibrary::VertexPaintDetectionPlugin_SortStringArrayAlphabetically(TArray<FString> Strings) {


	for (int i = 0; i < Strings.Num(); ++i) {

		for (int j = i + 1; j < Strings.Num(); ++j) {

			if (Strings[i] > Strings[j]) {

				FString String = Strings[i];
				Strings[i] = Strings[j];
				Strings[j] = String;
			}
		}
	}

	return Strings;
}


//--------------------------------------------------------

// Sort Assets Names Alphabetically

TMap<int, FString> UVertexPaintFunctionLibrary::VertexPaintDetectionPlugin_SortAssetsNamesAlphabetically(TMap<int, FString> AssetIndexAndName) {


	TArray<FString> names;
	AssetIndexAndName.GenerateValueArray(names);

	TArray<int> indexes;
	AssetIndexAndName.GetKeys(indexes);


	for (int i = 0; i < names.Num(); ++i) {

		for (int j = i + 1; j < names.Num(); ++j) {

			if (names[i] > names[j]) {

				FString name = names[i];
				int index = indexes[i];

				names[i] = names[j];
				indexes[i] = indexes[j];

				names[j] = name;
				indexes[j] = index;
			}
		}
	}

	TMap<int, FString> indexesAndTheirName;

	for (int i = 0; i < indexes.Num(); i++) {

		indexesAndTheirName.Add(indexes[i], names[i]);
	}

	return indexesAndTheirName;
}


//--------------------------------------------------------

// Draw Primitive Bounds Box

void UVertexPaintFunctionLibrary::DrawPrimitiveComponentBoundsBox(UPrimitiveComponent* PrimitiveComponent, float Lifetime, float Thickness, FLinearColor ColorToDraw) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_DrawPrimitiveComponentBoundsBox);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - DrawPrimitiveComponentBoundsBox");

	if (!IsInGameThread()) return;
	if (!IsValid(PrimitiveComponent)) return;
	if (!IsValid(PrimitiveComponent->GetWorld())) return;
	if (!PrimitiveComponent->GetWorld()->IsGameWorld()) return;
	if (PrimitiveComponent->GetWorld()->bIsTearingDown) return;


	if (Cast<USkeletalMeshComponent>(PrimitiveComponent)) {

		DrawDebugBox(PrimitiveComponent->GetWorld(), PrimitiveComponent->Bounds.GetBox().GetCenter(), PrimitiveComponent->Bounds.GetBox().GetExtent(), ColorToDraw.ToFColor(false), false, Lifetime, 0, Thickness);
	}

	// In order to take the added collision of static meshes into account we had to get the AggGeom
	else if (Cast<UStaticMeshComponent>(PrimitiveComponent)) {

		FBoxSphereBounds AggGeomBounds;
		PrimitiveComponent->GetBodySetup()->AggGeom.CalcBoxSphereBounds(AggGeomBounds, PrimitiveComponent->GetComponentToWorld());

		DrawDebugBox(PrimitiveComponent->GetWorld(), PrimitiveComponent->Bounds.GetBox().GetCenter(), AggGeomBounds.GetBox().GetExtent(), ColorToDraw.ToFColor(false), false, Lifetime, 0, Thickness);
	}
}


//--------------------------------------------------------

// Get Color Snippet Colors Async

void UVertexPaintFunctionLibrary::GetColorSnippetVertexColorsAsync(UObject* WorldContextObject, FGameplayTag ColorSnippetTag, FLatentActionInfo LatentInfo, TSoftObjectPtr<UObject>& ObjectSnippetIsAssociatedWith, TArray<FColor>& ColorSnippetVertexColors, bool& Success) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetColorSnippetVertexColorsAsync);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - GetColorSnippetVertexColorsAsync");

	Success = false;

	if (!WorldContextObject) return;
	if (!IsWorldValid(WorldContextObject->GetWorld())) return;
	if (ColorSnippetTag.ToString().IsEmpty()) return;
	if (!GetColorSnippetReferenceDataAsset(WorldContextObject)) return;


	FSoftObjectPath colorSnippetDataAssetToLoadPath;
	FString colorSnippetID = ColorSnippetTag.ToString();

	ObjectSnippetIsAssociatedWith = GetColorSnippetReferenceDataAsset(WorldContextObject)->GetObjectFromSnippetID(colorSnippetID);

	if (!ObjectSnippetIsAssociatedWith.IsValid()) {

		RVPDP_LOG(FRVPDPDebugSettings(false, 0, true), FColor::Cyan, "GetColorSnippetVertexColorsAsync: Fail - Couldn't get Valid UObject related to Color Snippet. ");

		return;
	}


	if (GetColorSnippetReferenceDataAsset(WorldContextObject)->SkeletalMeshesColorSnippets.Contains(ObjectSnippetIsAssociatedWith)) {

		if (GetColorSnippetReferenceDataAsset(WorldContextObject)->SkeletalMeshesColorSnippets.Find(ObjectSnippetIsAssociatedWith)->ColorSnippetsStorageInfo.Contains(colorSnippetID)) {

			colorSnippetDataAssetToLoadPath = GetColorSnippetReferenceDataAsset(WorldContextObject)->SkeletalMeshesColorSnippets.Find(ObjectSnippetIsAssociatedWith)->ColorSnippetsStorageInfo.Find(colorSnippetID)->ColorSnippetDataAssetStoredOn.ToSoftObjectPath();
		}
	}

	else if (GetColorSnippetReferenceDataAsset(WorldContextObject)->StaticMeshesColorSnippets.Contains(ObjectSnippetIsAssociatedWith)) {

		if (GetColorSnippetReferenceDataAsset(WorldContextObject)->StaticMeshesColorSnippets.Find(ObjectSnippetIsAssociatedWith)->ColorSnippetsStorageInfo.Contains(colorSnippetID)) {

			colorSnippetDataAssetToLoadPath = GetColorSnippetReferenceDataAsset(WorldContextObject)->StaticMeshesColorSnippets.Find(ObjectSnippetIsAssociatedWith)->ColorSnippetsStorageInfo.Find(colorSnippetID)->ColorSnippetDataAssetStoredOn.ToSoftObjectPath();
		}
	}


	if (colorSnippetDataAssetToLoadPath.ToString().IsEmpty()) {

		RVPDP_LOG(FRVPDPDebugSettings(false, 0, true), FColor::Cyan, "GetColorSnippetVertexColorsAsync: Fail - Couldn't get valid FSoftObjectPath to color snippet data asset to load. ");

		return;
	}


	FGetColorSnippetColorsLatentAction* latentAction = new FGetColorSnippetColorsLatentAction(LatentInfo);
	WorldContextObject->GetWorld()->GetLatentActionManager().AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, latentAction);

	// Check if already loaded
	if (colorSnippetDataAssetToLoadPath.ResolveObject()) {

		if (UVertexPaintColorSnippetDataAsset* colorSnippetDataAsset = Cast<UVertexPaintColorSnippetDataAsset>(colorSnippetDataAssetToLoadPath.ResolveObject())) {

			if (colorSnippetDataAsset->SnippetColorData.Contains(colorSnippetID)) {

				// RVPDP_LOG(FVertexPaintDebugSettings(false, 0, true), FColor::Cyan, "Color Snippet Data Asset already Loaded. ");

				ColorSnippetVertexColors = colorSnippetDataAsset->SnippetColorData.Find(colorSnippetID)->ColorSnippetDataPerLOD[0].MeshVertexColorsPerLODArray;
				Success = true;
				latentAction->MarkAsCompleted();
				return;
			}
		}
	}


	TArray<FSoftObjectPath> assetsToLoad;
	assetsToLoad.Add(colorSnippetDataAssetToLoadPath);

	FStreamableManager& streamableManager = UAssetManager::GetStreamableManager();

	// Assigning the StreamableHandle first so we can use the streamableHandle in the lambda when completed. 
	TSharedPtr<FStreamableHandle> streamableHandle = streamableManager.RequestAsyncLoad(assetsToLoad, FStreamableDelegate::CreateLambda([]() {
		// 
		}));


	// Update the completion callback with the necessary logic
	streamableHandle->BindCompleteDelegate(FStreamableDelegate::CreateLambda([streamableHandle, latentAction, colorSnippetID, &ColorSnippetVertexColors, &Success]() mutable {

		if (streamableHandle->HasLoadCompleted()) {

			if (UVertexPaintColorSnippetDataAsset* colorSnippetDataAsset = Cast<UVertexPaintColorSnippetDataAsset>(streamableHandle->GetLoadedAsset())) {

				if (colorSnippetDataAsset->SnippetColorData.Contains(colorSnippetID)) {

					RVPDP_LOG(FRVPDPDebugSettings(false, 0, true), FColor::Cyan, "Color Snippet Data Asset Async Load Finished. ");

					ColorSnippetVertexColors = colorSnippetDataAsset->SnippetColorData.Find(colorSnippetID)->ColorSnippetDataPerLOD[0].MeshVertexColorsPerLODArray;
					Success = true;
				}
			}
		}

		latentAction->MarkAsCompleted();
	}));
}


//--------------------------------------------------------

// Get Fill Area Between Two Locations Info

void UVertexPaintFunctionLibrary::GetFillAreaBetweenTwoLocationsInfo(const FVector& FromLocation, const FVector& ToLocation, float DesiredBoxThickness, FVector& BoxExtent, FVector& MiddlePointBetweenLocations, FRotator& RotationFromAndToLocation) {


	const float distanceBetweenLocations = (FromLocation - ToLocation).Size();
	MiddlePointBetweenLocations = (FromLocation + ToLocation) / 2;
	BoxExtent = FVector(distanceBetweenLocations / 2, DesiredBoxThickness, DesiredBoxThickness);
	RotationFromAndToLocation = UKismetMathLibrary::GetDirectionUnitVector(FromLocation, ToLocation).ToOrientationRotator();
}


//--------------------------------------------------------

// Get Static Mesh Body Collision Info

bool UVertexPaintFunctionLibrary::GetStaticMeshBodyCollisionInfo(UPrimitiveComponent* StaticMeshComponent, TArray<FVector>& BoxCenters, TArray<FVector>& BoxExtents, TArray<FRotator>& BoxRotations, TArray<FVector>& SphereCenters, TArray<float>& SphereRadiuses, TArray<FVector>& CapsuleCenters, TArray<FRotator>& CapsuleRotations, TArray<float>& CapsuleRadiuses, TArray<float>& CapsuleLengths) {

	BoxCenters.Empty();
	BoxExtents.Empty();
	BoxRotations.Empty();

	SphereCenters.Empty();
	SphereRadiuses.Empty();

	CapsuleCenters.Empty();
	CapsuleRotations.Empty();
	CapsuleRadiuses.Empty();
	CapsuleLengths.Empty();

	if (!IsValid(StaticMeshComponent)) return false;
	if (!StaticMeshComponent->GetBodySetup()) return false;


	const UBodySetup* bodySetup = StaticMeshComponent->GetBodySetup();

	for (int i = 0; i < bodySetup->AggGeom.SphereElems.Num(); i++) {

		const FVector center = bodySetup->AggGeom.SphereElems[i].Center;
		const float radius = bodySetup->AggGeom.SphereElems[i].Radius;

		SphereCenters.Add(center);
		SphereRadiuses.Add(radius);
	}

	for (int i = 0; i < bodySetup->AggGeom.BoxElems.Num(); i++) {

		const FVector center = bodySetup->AggGeom.BoxElems[i].Center;
		const FVector extent = FVector(bodySetup->AggGeom.BoxElems[i].X, bodySetup->AggGeom.BoxElems[i].Y, bodySetup->AggGeom.BoxElems[i].Z);
		const FRotator rotation = bodySetup->AggGeom.BoxElems[i].Rotation;

		BoxCenters.Add(center);
		BoxExtents.Add(extent);
		BoxRotations.Add(rotation);
	}

	for (int i = 0; i < bodySetup->AggGeom.TaperedCapsuleElems.Num(); i++) {

		const FVector center = bodySetup->AggGeom.TaperedCapsuleElems[i].Center;
		const FRotator rotation = bodySetup->AggGeom.TaperedCapsuleElems[i].Rotation;
		const float radius = bodySetup->AggGeom.TaperedCapsuleElems[i].Radius0;
		const float length = bodySetup->AggGeom.TaperedCapsuleElems[i].Length;

		CapsuleCenters.Add(center);
		CapsuleRotations.Add(rotation);
		CapsuleRadiuses.Add(radius);
		CapsuleLengths.Add(length);
	}

	return true;
}


//--------------------------------------------------------

// Async Load Assets

void UVertexPaintFunctionLibrary::AsyncLoadAssets(UObject* WorldContextObject, FLatentActionInfo LatentInfo, TArray<TSoftObjectPtr<UObject>> AssetsToLoad, bool PrintResultsToScreen, bool PrintResultsToLog, TArray<UObject*>& LoadedAssets, bool& Success, int32 AssetLoadPriority) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_AsyncLoadAssets);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - AsyncLoadAssets");

	Success = false;
	LoadedAssets.Empty();

	if (!WorldContextObject) return;
	if (!IsWorldValid(WorldContextObject->GetWorld())) return;


	FAsyncLoadSoftPtrsLatentAction* latentAction = new FAsyncLoadSoftPtrsLatentAction(LatentInfo);
	WorldContextObject->GetWorld()->GetLatentActionManager().AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, latentAction);


	if (AssetsToLoad.Num() == 0) {

		if (PrintResultsToLog)
			RVPDP_LOG(FRVPDPDebugSettings(false, 0, true), FColor::Cyan, "No Assets to Load. ");

		Success = true;
		latentAction->MarkAsCompleted();
		return;
	}


	TArray<FSoftObjectPath> assetsPathsToLoad;
	int amountOfLoadedAsset = 0;

	for (TSoftObjectPtr<UObject> assetToLoad : AssetsToLoad) {

		if (assetToLoad.IsNull()) continue;

		FSoftObjectPath assetPath = assetToLoad.ToSoftObjectPath();

		assetsPathsToLoad.Add(assetPath);

		// If already loaded
		if (assetPath.ResolveObject()) {

			LoadedAssets.Add(assetPath.ResolveObject());
			amountOfLoadedAsset++;
		}
	}

	// If all valid assets are already loaded 
	if (amountOfLoadedAsset == assetsPathsToLoad.Num()) {

		if (PrintResultsToLog)
			RVPDP_LOG(FRVPDPDebugSettings(false, 0, true), FColor::Cyan, "Assets Already Loaded! ");

		Success = true;
		latentAction->MarkAsCompleted();
		return;
	}

	else {

		LoadedAssets.Empty();
	}


	FStreamableManager& streamableManager = UAssetManager::GetStreamableManager();

	// Assigning the StreamableHandle first so we can use the streamableHandle in the lambda when completed. 
	TSharedPtr<FStreamableHandle> streamableHandle = streamableManager.RequestAsyncLoad(assetsPathsToLoad, FStreamableDelegate::CreateLambda([]() {
		// 
	}), AssetLoadPriority);


	UWorld* world = WorldContextObject->GetWorld();

	// Update the completion callback with the necessary logic
	streamableHandle->BindCompleteDelegate(FStreamableDelegate::CreateLambda([streamableHandle, latentAction, world, PrintResultsToScreen, PrintResultsToLog, &LoadedAssets, &Success]() mutable {


		if (!IsWorldValid(world))
		{

			if (PrintResultsToLog) {

				RVPDP_LOG(FRVPDPDebugSettings(false, 0, PrintResultsToLog), FColor::Red, "Async Loading Assets Failed. World Not valid anymore ");
			}

			return;
		}


		if (!streamableHandle.IsValid()) {

			if (PrintResultsToScreen || PrintResultsToLog) {

				RVPDP_LOG(FRVPDPDebugSettings(world, PrintResultsToScreen, 0, PrintResultsToLog), FColor::Red, "Async Loading Assets Failed. Streamable Handle not Valid ");
			}

			Success = false;

			if (latentAction)
				latentAction->MarkAsCompleted();

			return;
		}


		if (!streamableHandle->HasLoadCompleted()) {

			if (PrintResultsToScreen || PrintResultsToLog) {

				RVPDP_LOG(FRVPDPDebugSettings(world, PrintResultsToScreen, 0, PrintResultsToLog), FColor::Red, "Async Loading Assets Failed. Delegate finished but Load Complete is false. ");
			}

			Success = false;

			if (latentAction)
				latentAction->MarkAsCompleted();

			return;
		}


		if (PrintResultsToScreen || PrintResultsToLog) {

			RVPDP_LOG(FRVPDPDebugSettings(world, PrintResultsToScreen, 0, PrintResultsToLog), FColor::Green, "Async Loading Assets Complete. ");
		}


		streamableHandle->GetLoadedAssets(LoadedAssets);
		Success = true;

		if (latentAction)
			latentAction->MarkAsCompleted();
	}));
}


//--------------------------------------------------------

// Is Assets Loaded

bool UVertexPaintFunctionLibrary::IsAssetsLoaded(UObject* WorldContextObject, TArray<TSoftObjectPtr<UObject>> AssetsToCheck) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_IsAssetsLoaded);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - IsAssetsLoaded");

	if (!WorldContextObject) return false;
	if (!IsWorldValid(WorldContextObject->GetWorld())) return false;
	if (AssetsToCheck.Num() == 0) return true;



	TArray<FSoftObjectPath> assetsPathsToLoad;
	int amountOfLoadedAsset = 0;

	for (TSoftObjectPtr<UObject> assetToLoad : AssetsToCheck) {

		if (assetToLoad.IsNull()) continue;

		FSoftObjectPath assetPath = assetToLoad.ToSoftObjectPath();

		assetsPathsToLoad.Add(assetPath);

		// If already loaded
		if (assetPath.ResolveObject())
			amountOfLoadedAsset++;
	}


	if (amountOfLoadedAsset == assetsPathsToLoad.Num())
		return true;


	return false;
}


//--------------------------------------------------------

// Get Amount Of Painted Colors For Each Channel - Async Version

void UVertexPaintFunctionLibrary::GetAmountOfPaintedColorsForEachChannelAsync(UObject* WorldContextObject, FLatentActionInfo LatentInfo, const TArray<FColor>& VertexColors, FRVPDPAmountOfColorsOfEachChannelResults& AmountOfColorsOfEachChannel, float MinColorAmountToBeConsidered) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetAmountOfPaintedColorsForEachChannelAsync);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - GetAmountOfPaintedColorsForEachChannelAsync");

	if (!WorldContextObject) return;


	if (UWorld* World = WorldContextObject->GetWorld()) {


		// Instantiate the AsyncTask
		FRVPDPColorsOfEachChannelAsyncTask* calculateColorsOfEachChannelAsyncTask = new FRVPDPColorsOfEachChannelAsyncTask(VertexColors, MinColorAmountToBeConsidered);


		// Create our custom latent action and add to manager
		FColorsOfEachChannelLatentAction* LatentAction = new FColorsOfEachChannelLatentAction(LatentInfo, calculateColorsOfEachChannelAsyncTask);
		World->GetLatentActionManager().AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, LatentAction);


		// Bind the delegate and sets the result
		calculateColorsOfEachChannelAsyncTask->OnColorsOfEachChannelAsyncTaskCompleteDelegate.BindLambda([&AmountOfColorsOfEachChannel, LatentAction](const FRVPDPAmountOfColorsOfEachChannelResults& amountOfPaintedColorsOfEachChannelResult) {

			// RVPDP_LOG(FRVPDPDebugSettings(false, 0, true), FColor::Cyan, "Got Amount of Painted Colors For Each Channel Finished. ");

			AmountOfColorsOfEachChannel = amountOfPaintedColorsOfEachChannelResult;
			LatentAction->MarkAsCompleted();
		});


		(new FAutoDeleteAsyncTask<FRVPDPColorsOfEachChannelAsyncTask>(*calculateColorsOfEachChannelAsyncTask))->StartBackgroundTask();
	}
}


//--------------------------------------------------------

// Get Amount Of Painted Colors For Each Channel

FRVPDPAmountOfColorsOfEachChannelResults UVertexPaintFunctionLibrary::GetAmountOfPaintedColorsForEachChannel(const TArray<FColor>& VertexColors, float MinColorAmountToBeConsidered) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetAmountOfPaintedColorsForEachChannel);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - GetAmountOfPaintedColorsForEachChannel");

	if (VertexColors.Num() == 0) return FRVPDPAmountOfColorsOfEachChannelResults();


	FRVPDPAmountOfColorsOfEachChannelResults amountOfPaintedColorsOfEachChannel;


	for (int i = 0; i < VertexColors.Num(); i++) {

		FLinearColor linearColor = UVertexPaintFunctionLibrary::ReliableFColorToFLinearColor(VertexColors[i]);
		// linearColor = color.ReinterpretAsLinear();


		// These are used by the async task to check if even the vertex channel was considered, since with paint/detect task you have option to only include those that has a physics surface registered to them, and if they're not, they won't even be considered and if none where considered we didn't run any more logic in FillAmountOfPaintedColorsOfEachChannel()
		amountOfPaintedColorsOfEachChannel.RedChannelResult.AmountOfVerticesConsidered++;
		amountOfPaintedColorsOfEachChannel.GreenChannelResult.AmountOfVerticesConsidered++;
		amountOfPaintedColorsOfEachChannel.BlueChannelResult.AmountOfVerticesConsidered++;
		amountOfPaintedColorsOfEachChannel.AlphaChannelResult.AmountOfVerticesConsidered++;


		if (linearColor.R >= MinColorAmountToBeConsidered) {

			amountOfPaintedColorsOfEachChannel.RedChannelResult.AmountOfVerticesPaintedAtMinAmount++;
			amountOfPaintedColorsOfEachChannel.RedChannelResult.AverageColorAmountAtMinAmount += linearColor.R;
		}
		if (linearColor.G >= MinColorAmountToBeConsidered) {

			amountOfPaintedColorsOfEachChannel.GreenChannelResult.AmountOfVerticesPaintedAtMinAmount++;
			amountOfPaintedColorsOfEachChannel.GreenChannelResult.AverageColorAmountAtMinAmount += linearColor.G;
		}
		if (linearColor.B >= MinColorAmountToBeConsidered) {

			amountOfPaintedColorsOfEachChannel.BlueChannelResult.AmountOfVerticesPaintedAtMinAmount++;
			amountOfPaintedColorsOfEachChannel.BlueChannelResult.AverageColorAmountAtMinAmount += linearColor.B;
		}
		if (linearColor.A >= MinColorAmountToBeConsidered) {

			amountOfPaintedColorsOfEachChannel.AlphaChannelResult.AmountOfVerticesPaintedAtMinAmount++;
			amountOfPaintedColorsOfEachChannel.AlphaChannelResult.AverageColorAmountAtMinAmount += linearColor.A;
		}
	}

	// After gotten amount of vertices painted at each color etc., we can use it to set the amounts. This is used by the async task as well
	amountOfPaintedColorsOfEachChannel = ConsolidateColorsOfEachChannel(amountOfPaintedColorsOfEachChannel, VertexColors.Num());


	return amountOfPaintedColorsOfEachChannel;
}


//--------------------------------------------------------

// Set Mesh Constant Vertex Colors and Enables Them

UDynamicMesh* UVertexPaintFunctionLibrary::SetMeshConstantVertexColorsAndEnablesThem(UDynamicMesh* TargetMesh, FLinearColor Color, FGeometryScriptColorFlags Flags, bool bClearExisting, UGeometryScriptDebug* Debug) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_SetMeshConstantVertexColorsAndEnablesThem);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - SetMeshConstantVertexColorsAndEnablesThem");

	if (!TargetMesh) return nullptr;


	UE::Geometry::FDynamicMesh3* dynamicMesh3 = nullptr;
	dynamicMesh3 = &TargetMesh->GetMeshRef();

	if (dynamicMesh3) {

		// So we can run EnableVertexColors() again after. Otherwise it would only work once, because it returns if HasVertexColors is true, which created a problem if we painted or detected on the dynamic mesh (which enables colors if there aren't any), then ran this function, because then SetMeshConstantVertexColor() would run but we couldn't actually Set the colors to what SetMeshConstantVertexColor because EnableVertexColors just returned.
		dynamicMesh3->DiscardVertexColors();

		UGeometryScriptLibrary_MeshVertexColorFunctions::SetMeshConstantVertexColor(TargetMesh, Color, Flags, bClearExisting, Debug);

		// Converts these like this instead of with .ToFColor since we could get issues where for instance a few vertices out of 10k got their color reset to 0. This fixed the issue.
		FColor colorToApply;
		colorToApply.R = static_cast<uint8>(UKismetMathLibrary::MapRangeClamped(Color.R, 0, 1, 0, 255));
		colorToApply.G = static_cast<uint8>(UKismetMathLibrary::MapRangeClamped(Color.G, 0, 1, 0, 255));
		colorToApply.B = static_cast<uint8>(UKismetMathLibrary::MapRangeClamped(Color.B, 0, 1, 0, 255));
		colorToApply.A = 0;

		
		dynamicMesh3->EnableVertexColors(FVector3f(colorToApply.R, colorToApply.G, colorToApply.B));
	}


	return TargetMesh;
}


//--------------------------------------------------------

// Consolidate Colors Of Each Channel

FRVPDPAmountOfColorsOfEachChannelResults UVertexPaintFunctionLibrary::ConsolidateColorsOfEachChannel(FRVPDPAmountOfColorsOfEachChannelResults AmountOfColorsOfEachChannel, int AmountOfVertices) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_ConsolidateColorsOfEachChannel);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - ConsolidateColorsOfEachChannel");

	if (AmountOfVertices <= 0) return AmountOfColorsOfEachChannel;

	// If haven't gotten anything on any channel then isn't successfull. 
	if (AmountOfColorsOfEachChannel.RedChannelResult.AmountOfVerticesConsidered <= 0 && AmountOfColorsOfEachChannel.GreenChannelResult.AmountOfVerticesConsidered <= 0 && AmountOfColorsOfEachChannel.BlueChannelResult.AmountOfVerticesConsidered <= 0 && AmountOfColorsOfEachChannel.AlphaChannelResult.AmountOfVerticesConsidered <= 0) return AmountOfColorsOfEachChannel;

	AmountOfColorsOfEachChannel.RedChannelResult.AverageColorAmountAtMinAmount = AmountOfColorsOfEachChannel.RedChannelResult.AverageColorAmountAtMinAmount / AmountOfVertices;
	AmountOfColorsOfEachChannel.GreenChannelResult.AverageColorAmountAtMinAmount = AmountOfColorsOfEachChannel.GreenChannelResult.AverageColorAmountAtMinAmount / AmountOfVertices;
	AmountOfColorsOfEachChannel.BlueChannelResult.AverageColorAmountAtMinAmount = AmountOfColorsOfEachChannel.BlueChannelResult.AverageColorAmountAtMinAmount / AmountOfVertices;
	AmountOfColorsOfEachChannel.AlphaChannelResult.AverageColorAmountAtMinAmount = AmountOfColorsOfEachChannel.AlphaChannelResult.AverageColorAmountAtMinAmount / AmountOfVertices;


	AmountOfColorsOfEachChannel.RedChannelResult.PercentPaintedAtMinAmount = AmountOfColorsOfEachChannel.RedChannelResult.AmountOfVerticesPaintedAtMinAmount / AmountOfVertices;
	AmountOfColorsOfEachChannel.GreenChannelResult.PercentPaintedAtMinAmount = AmountOfColorsOfEachChannel.GreenChannelResult.AmountOfVerticesPaintedAtMinAmount / AmountOfVertices;
	AmountOfColorsOfEachChannel.BlueChannelResult.PercentPaintedAtMinAmount = AmountOfColorsOfEachChannel.BlueChannelResult.AmountOfVerticesPaintedAtMinAmount / AmountOfVertices;
	AmountOfColorsOfEachChannel.AlphaChannelResult.PercentPaintedAtMinAmount = AmountOfColorsOfEachChannel.AlphaChannelResult.AmountOfVerticesPaintedAtMinAmount / AmountOfVertices;

	AmountOfColorsOfEachChannel.RedChannelResult.PercentPaintedAtMinAmount *= 100;
	AmountOfColorsOfEachChannel.GreenChannelResult.PercentPaintedAtMinAmount *= 100;
	AmountOfColorsOfEachChannel.BlueChannelResult.PercentPaintedAtMinAmount *= 100;
	AmountOfColorsOfEachChannel.AlphaChannelResult.PercentPaintedAtMinAmount *= 100;

	AmountOfColorsOfEachChannel.SuccessfullyGotColorChannelResultsAtMinAmount = true;

	return AmountOfColorsOfEachChannel;
}


//--------------------------------------------------------

// Consolidate Physics Surface Result

FRVPDPAmountOfColorsOfEachChannelResults UVertexPaintFunctionLibrary::ConsolidatePhysicsSurfaceResult(FRVPDPAmountOfColorsOfEachChannelResults AmountOfColorsOfEachChannel, int AmountOfVertices) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_ConsolidatePhysicsSurfaceResult);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - ConsolidatePhysicsSurfaceResult");

	if (AmountOfVertices <= 0) return AmountOfColorsOfEachChannel;
	if (AmountOfColorsOfEachChannel.PhysicsSurfacesResults.Num() == 0) return AmountOfColorsOfEachChannel;

	bool gotFromAnyPhysicsSurface = false;


	for (int i = AmountOfColorsOfEachChannel.PhysicsSurfacesResults.Num() - 1; i >= 0; i--) {

		if (AmountOfColorsOfEachChannel.PhysicsSurfacesResults[i].AmountOfVerticesConsidered == 0) continue;


		AmountOfColorsOfEachChannel.PhysicsSurfacesResults[i].AverageColorAmountAtMinAmount = AmountOfColorsOfEachChannel.PhysicsSurfacesResults[i].AverageColorAmountAtMinAmount / AmountOfVertices;
		AmountOfColorsOfEachChannel.PhysicsSurfacesResults[i].PercentPaintedAtMinAmount = AmountOfColorsOfEachChannel.PhysicsSurfacesResults[i].AmountOfVerticesPaintedAtMinAmount / AmountOfVertices;
		AmountOfColorsOfEachChannel.PhysicsSurfacesResults[i].PercentPaintedAtMinAmount *= 100;

		gotFromAnyPhysicsSurface = true;
	}


	// If no vertices was considered on even one physics surface then it isn't successfull. 
	if (!gotFromAnyPhysicsSurface) return AmountOfColorsOfEachChannel;


	// Create a TArray of pairs to hold the elements and their float values
	TArray<TPair<TEnumAsByte<EPhysicalSurface>, FRVPDPAmountOfColorsOfEachChannelPhysicsResults>> SortedArray;
	for (const auto& Pair : AmountOfColorsOfEachChannel.PhysicsSurfacesResults) {

		SortedArray.Add(TPair<TEnumAsByte<EPhysicalSurface>, FRVPDPAmountOfColorsOfEachChannelPhysicsResults>(Pair.PhysicsSurface, Pair));
	}

	// Sort the array based on painted percent
	SortedArray.Sort([](const TPair<TEnumAsByte<EPhysicalSurface>, FRVPDPAmountOfColorsOfEachChannelPhysicsResults>& A, const TPair<TEnumAsByte<EPhysicalSurface>, FRVPDPAmountOfColorsOfEachChannelChannelResults>& B) {
		return A.Value.PercentPaintedAtMinAmount > B.Value.PercentPaintedAtMinAmount;
		});


	TArray<FRVPDPAmountOfColorsOfEachChannelPhysicsResults> sortedPhysicsSurfaceResults;
	for (const auto& Pair : SortedArray) {

		sortedPhysicsSurfaceResults.Add(Pair.Value);
	}

	AmountOfColorsOfEachChannel.PhysicsSurfacesResults = sortedPhysicsSurfaceResults;
	AmountOfColorsOfEachChannel.SuccessfullyGotPhysicsSurfaceResultsAtMinAmount = true;

	return AmountOfColorsOfEachChannel;
}


//--------------------------------------------------------

// Get Skeletal Mesh

USkeletalMesh* UVertexPaintFunctionLibrary::VertexPaintDetectionPlugin_GetSkeletalMesh(USkeletalMeshComponent* SkeletalMeshComp) {

	if (!IsValid(SkeletalMeshComp)) return nullptr;


#if ENGINE_MAJOR_VERSION == 4

	return SkeletalMeshComp->SkeletalMesh;

#elif ENGINE_MAJOR_VERSION == 5

#if ENGINE_MINOR_VERSION == 0

	return SkeletalMeshComp->SkeletalMesh.Get();

#else

	return SkeletalMeshComp->GetSkeletalMeshAsset();

#endif
#endif
}


//--------------------------------------------------------

// Get Mesh Component Vertex Colors

FRVPDPVertexDataInfo UVertexPaintFunctionLibrary::GetMeshComponentVertexColors_Wrapper(UPrimitiveComponent* meshComponent, bool& Success, bool GetColorsForAllLODs, int GetColorsUpToLOD) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetMeshComponentVertexColors);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - GetMeshComponentVertexColors");

	Success = false;
	if (!IsValid(meshComponent)) return FRVPDPVertexDataInfo();
	if (!GetColorsForAllLODs && GetColorsUpToLOD < 0) return FRVPDPVertexDataInfo();


	FRVPDPVertexDataInfo vertexDataInfo;
	TArray<FVertexDetectMeshDataPerLODStruct> vertexDataPerLOD;
	int amountOfLODsToGet = GetColorsUpToLOD + 1;


	if (auto staticMeshComponent = Cast<UStaticMeshComponent>(meshComponent)) {

		if (!IsValid(staticMeshComponent->GetStaticMesh())) return FRVPDPVertexDataInfo();


		vertexDataInfo.MeshComp = staticMeshComponent;
		vertexDataInfo.MeshSource = staticMeshComponent->GetStaticMesh();

		if (GetColorsForAllLODs)
			amountOfLODsToGet = staticMeshComponent->GetStaticMesh()->GetRenderData()->LODResources.Num();


		for (int i = 0; i < amountOfLODsToGet; i++) {

			FVertexDetectMeshDataPerLODStruct meshDataPerLod;
			meshDataPerLod.Lod = i;
			meshDataPerLod.MeshVertexColorsPerLODArray = GetStaticMeshVertexColorsAtLOD(staticMeshComponent, i);

			// If actually got any colors, i.e. the Lod was valid
			if (meshDataPerLod.MeshVertexColorsPerLODArray.Num() > 0)
				vertexDataPerLOD.Add(meshDataPerLod);
		}

		vertexDataInfo.MeshDataPerLOD = vertexDataPerLOD;
		Success = true;

		return vertexDataInfo;
	}

	else if (auto skeletalMeshComponent = Cast<USkeletalMeshComponent>(meshComponent)) {

		const UObject* skelMesh = UVertexPaintFunctionLibrary::GetMeshComponentSourceMesh(skeletalMeshComponent);

		if (!IsValid(skelMesh)) return FRVPDPVertexDataInfo();


		vertexDataInfo.MeshComp = skeletalMeshComponent;
		vertexDataInfo.MeshSource = TSoftObjectPtr<UObject>(FSoftObjectPath(skelMesh->GetPathName()));

		if (GetColorsForAllLODs)
			amountOfLODsToGet = skeletalMeshComponent->GetSkeletalMeshRenderData()->LODRenderData.Num();


		for (int i = 0; i < amountOfLODsToGet; i++) {

			FVertexDetectMeshDataPerLODStruct meshDataPerLod;
			meshDataPerLod.Lod = i;
			meshDataPerLod.MeshVertexColorsPerLODArray = GetSkeletalMeshVertexColorsAtLOD(skeletalMeshComponent, i);

			// If actually got any colors, i.e. the Lod was valid
			if (meshDataPerLod.MeshVertexColorsPerLODArray.Num() > 0)
				vertexDataPerLOD.Add(meshDataPerLod);
		}

		vertexDataInfo.MeshDataPerLOD = vertexDataPerLOD;
		Success = true;

		return vertexDataInfo;
	}

#if ENGINE_MAJOR_VERSION == 5

	else if (auto dynamicMeshComponent = Cast<UDynamicMeshComponent>(meshComponent)) {

		vertexDataInfo.MeshComp = dynamicMeshComponent;
		// vertexMeshData.MeshSource = ;

		FVertexDetectMeshDataPerLODStruct meshDataPerLod;
		meshDataPerLod.Lod = 0;
		meshDataPerLod.MeshVertexColorsPerLODArray = GetDynamicMeshVertexColors(dynamicMeshComponent);

		// If actually got any colors, i.e. the Lod was valid
		if (meshDataPerLod.MeshVertexColorsPerLODArray.Num() > 0)
			vertexDataPerLOD.Add(meshDataPerLod);

		vertexDataInfo.MeshDataPerLOD = vertexDataPerLOD;
		Success = true;
	}

	else if (auto geometryCollectionComponent = Cast<UGeometryCollectionComponent>(meshComponent)) {


		vertexDataInfo.MeshComp = geometryCollectionComponent;
		vertexDataInfo.MeshSource = TSoftObjectPtr<UObject>(FSoftObjectPath(geometryCollectionComponent->GetRestCollection()->GetPathName()));

		FVertexDetectMeshDataPerLODStruct meshDataPerLod;
		meshDataPerLod.Lod = 0;
		meshDataPerLod.MeshVertexColorsPerLODArray = GetGeometryCollectionVertexColors(geometryCollectionComponent);

		// If actually got any colors, i.e. the Lod was valid
		if (meshDataPerLod.MeshVertexColorsPerLODArray.Num() > 0)
			vertexDataPerLOD.Add(meshDataPerLod);

		vertexDataInfo.MeshDataPerLOD = vertexDataPerLOD;
		Success = true;
	}

#endif

	return FRVPDPVertexDataInfo();
}


//--------------------------------------------------------

// Get Mesh Component Amount Of Vertices On LOD

int UVertexPaintFunctionLibrary::GetMeshComponentAmountOfVerticesOnLOD(UPrimitiveComponent* MeshComponent, int Lod) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetMeshComponentAmountOfVerticesOnLOD);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - GetMeshComponentAmountOfVerticesOnLOD");

	if (!IsValid(MeshComponent)) return 0;


	if (auto staticMeshComponent = Cast<UStaticMeshComponent>(MeshComponent)) {

		if (!IsValid(staticMeshComponent->GetStaticMesh())) return 0;
		if (!staticMeshComponent->GetStaticMesh()->bAllowCPUAccess) return 0;


		if (staticMeshComponent->LODData.IsValidIndex(Lod)) {


			if (staticMeshComponent->LODData[Lod].OverrideVertexColors)
				return staticMeshComponent->LODData[Lod].OverrideVertexColors->GetNumVertices();

			else if (staticMeshComponent->GetStaticMesh()->GetRenderData()->LODResources.IsValidIndex(Lod))
				return staticMeshComponent->GetStaticMesh()->GetRenderData()->LODResources[Lod].VertexBuffers.PositionVertexBuffer.GetNumVertices();
		}

		else if (staticMeshComponent->GetStaticMesh()->GetRenderData()->LODResources.IsValidIndex(Lod)) {

			return staticMeshComponent->GetStaticMesh()->GetRenderData()->LODResources[Lod].VertexBuffers.PositionVertexBuffer.GetNumVertices();
		}
	}

	else if (auto skeletalMeshComponent = Cast<USkeletalMeshComponent>(MeshComponent)) {


		if (skeletalMeshComponent->LODInfo.IsValidIndex(Lod)) {

			if (skeletalMeshComponent->LODInfo[Lod].OverrideVertexColors)
				return skeletalMeshComponent->LODInfo[Lod].OverrideVertexColors->GetNumVertices();

			else if (skeletalMeshComponent->GetSkeletalMeshRenderData()->LODRenderData.IsValidIndex(Lod))
				return skeletalMeshComponent->GetSkeletalMeshRenderData()->LODRenderData[Lod].StaticVertexBuffers.PositionVertexBuffer.GetNumVertices();
		}

		else if (skeletalMeshComponent->GetSkeletalMeshRenderData()) {

			if (skeletalMeshComponent->GetSkeletalMeshRenderData()->LODRenderData.IsValidIndex(Lod))
				return skeletalMeshComponent->GetSkeletalMeshRenderData()->LODRenderData[Lod].StaticVertexBuffers.PositionVertexBuffer.GetNumVertices();
		}
	}

#if ENGINE_MAJOR_VERSION == 5

	else if (auto dynamicMeshComponent = Cast<UDynamicMeshComponent>(MeshComponent)) {

		if (!dynamicMeshComponent->GetDynamicMesh()) return 0;

		UE::Geometry::FDynamicMesh3* dynamicMesh3 = nullptr;

		dynamicMesh3 = &dynamicMeshComponent->GetDynamicMesh()->GetMeshRef();

		if (!dynamicMesh3) return 0;

		return dynamicMesh3->VertexCount();
	}

	else if (auto geometryCollectionComponent = Cast<UGeometryCollectionComponent>(MeshComponent)) {


		if (const UGeometryCollection* geometryCollection = geometryCollectionComponent->GetRestCollection()) {
			if (!geometryCollection) return 0;

			TSharedPtr<FGeometryCollection, ESPMode::ThreadSafe> geometryCollectionData = geometryCollection->GetGeometryCollection();


			// geometryCollectionData->VertexCount // Uses Color.Num() since VertexCount is unreliable. Could return like 15 when we expect 20 000. 
			if (geometryCollectionData.Get() && geometryCollectionData.IsValid())
				return geometryCollectionData->Color.Num();
		}
	}

#endif

	return 0;
}


//--------------------------------------------------------

// Get Mesh Component Vertex Colors At Specific LOD

TArray<FColor> UVertexPaintFunctionLibrary::GetMeshComponentVertexColorsAtLOD_Wrapper(UPrimitiveComponent* MeshComponent, int Lod) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetMeshComponentVertexColorsAtLOD);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - GetMeshComponentVertexColorsAtLOD");

	if (!IsValid(MeshComponent)) return TArray<FColor>();


	TArray<FColor> vertexColors;


	if (auto staticMeshComponent = Cast<UStaticMeshComponent>(MeshComponent)) {

		if (Lod < 0) return vertexColors;

		vertexColors = GetStaticMeshVertexColorsAtLOD(staticMeshComponent, Lod);
	}

	else if (auto skeletalMeshComponent = Cast<USkeletalMeshComponent>(MeshComponent)) {

		if (Lod < 0) return vertexColors;

		vertexColors = GetSkeletalMeshVertexColorsAtLOD(skeletalMeshComponent, Lod);
	}

#if ENGINE_MAJOR_VERSION == 5

	else if (auto dynamicMeshComponent = Cast<UDynamicMeshComponent>(MeshComponent)) {

		vertexColors = GetDynamicMeshVertexColors(dynamicMeshComponent);
	}

	else if (auto geometryCollectionComponent = Cast<UGeometryCollectionComponent>(MeshComponent)) {


		vertexColors = GetGeometryCollectionVertexColors(geometryCollectionComponent);
	}

#endif


	return vertexColors;
}


//--------------------------------------------------------

// Get Skeletal Mesh Vertex Colors At LOD

TArray<FColor> UVertexPaintFunctionLibrary::GetSkeletalMeshVertexColorsAtLOD(USkeletalMeshComponent* SkeletalMeshComponent, int Lod) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetSkeletalMeshVertexColorsAtLOD);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - GetSkeletalMeshVertexColorsAtLOD");

	if (!IsValid(SkeletalMeshComponent)) return TArray<FColor>();


	TArray<FColor> colorFromLOD;

	if (SkeletalMeshComponent->LODInfo.IsValidIndex(Lod)) {

		// If been painted on before then can get the current Instance Color
		if (SkeletalMeshComponent->LODInfo[Lod].OverrideVertexColors) {

			SkeletalMeshComponent->LODInfo[Lod].OverrideVertexColors->GetVertexColors(colorFromLOD);
		}
		else {

			if (SkeletalMeshComponent->GetSkeletalMeshRenderData()->LODRenderData.IsValidIndex(Lod))
				SkeletalMeshComponent->GetSkeletalMeshRenderData()->LODRenderData[Lod].StaticVertexBuffers.ColorVertexBuffer.GetVertexColors(colorFromLOD);
		}
	}

	else {

		if (SkeletalMeshComponent->GetSkeletalMeshRenderData()->LODRenderData.IsValidIndex(Lod))
			SkeletalMeshComponent->GetSkeletalMeshRenderData()->LODRenderData[Lod].StaticVertexBuffers.ColorVertexBuffer.GetVertexColors(colorFromLOD);
	}


	// In one instance when i tested on a side project with a purchased character, it got 0 in Num when first time painting at it, i.e. got vertex colors from SkeletalMeshRenderData. This only occured if the character hadn't gotten any paint on it and was imported with FColor(255,255,255,255) so it was an easy solution to just fill arrays to that color in this rare instance. So unlike the static mesh solution i couldn't initialize colorFromLOD to be a length before running GetVertexColors because then i couldn't detect this rare case. 
	if (colorFromLOD.Num() > 0) {

		// 
	}

	else {

		if (SkeletalMeshComponent->GetSkeletalMeshRenderData()->LODRenderData.IsValidIndex(Lod))
			colorFromLOD.Init(FColor(255, 255, 255, 255), SkeletalMeshComponent->GetSkeletalMeshRenderData()->LODRenderData[Lod].GetNumVertices());
	}

	return colorFromLOD;
}


//--------------------------------------------------------

// Get Static Mesh Vertex Colors At LOD

TArray<FColor> UVertexPaintFunctionLibrary::GetStaticMeshVertexColorsAtLOD(UStaticMeshComponent* StaticMeshComponent, int Lod) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetStaticMeshVertexColorsAtLOD);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - GetStaticMeshVertexColorsAtLOD");

	if (!IsValid(StaticMeshComponent)) return TArray<FColor>();
	UStaticMesh* staticMesh = StaticMeshComponent->GetStaticMesh();
	if (!staticMesh) return TArray<FColor>();
	if (!staticMesh->bAllowCPUAccess) return TArray<FColor>();

	FStaticMeshRenderData* staticMeshRenderData = staticMesh->GetRenderData();
	if (!staticMeshRenderData) return TArray<FColor>();
	if (!staticMeshRenderData->LODResources.IsValidIndex(Lod)) return TArray<FColor>();


	TArray<FColor> colorFromLOD;
	int totalAmountOfVerticesAtLOD = staticMeshRenderData->LODResources[Lod].VertexBuffers.PositionVertexBuffer.GetNumVertices();

	if (totalAmountOfVerticesAtLOD <= 0) return TArray<FColor>();


	colorFromLOD.SetNum(totalAmountOfVerticesAtLOD);

	// Depending if override vertex colors is valid, we get the Vertex Colors from it, i.e. the instanced colors
	if (StaticMeshComponent->LODData.IsValidIndex(Lod)) {

		if (StaticMeshComponent->LODData[Lod].OverrideVertexColors) {

			StaticMeshComponent->LODData[Lod].OverrideVertexColors->GetVertexColors(colorFromLOD);
		}

		else {

			staticMeshRenderData->LODResources[Lod].VertexBuffers.ColorVertexBuffer.GetVertexColors(colorFromLOD);
		}
	}

	else {

		// If color buffer isn't initialized it means its default colors are White and it hasn't been painted either in editor or in runtime, if this is the case we init the array with white so even unstored, unpainted cpu meshes with all default white vertex colors can be painted and look as they should. 
		if (FColorVertexBuffer* colVertBufferAtLOD = &staticMeshRenderData->LODResources[Lod].VertexBuffers.ColorVertexBuffer) {

			if (colVertBufferAtLOD->IsInitialized()) {

				colVertBufferAtLOD->GetVertexColors(colorFromLOD);
			}
			else {

				colorFromLOD.Init(FColor::White, totalAmountOfVerticesAtLOD);
			}
		}

		else {

			colorFromLOD.Init(FColor::White, totalAmountOfVerticesAtLOD);
		}
	}

	return colorFromLOD;
}


//--------------------------------------------------------

// Get Vertex Paint Task Queue

UVertexPaintDetectionTaskQueue* UVertexPaintFunctionLibrary::GetVertexPaintTaskQueue(const UObject* WorldContextObject) {

	if (!WorldContextObject) return nullptr;

	if (auto gameInstanceSubsystem = GetVertexPaintGameInstanceSubsystem(WorldContextObject))
		return gameInstanceSubsystem->GetVertexPaintTaskQueue();

	return nullptr;
}


//--------------------------------------------------------

// Does Paint Task Queue Contain ID

bool UVertexPaintFunctionLibrary::DoesPaintTaskQueueContainID(const UObject* WorldContextObject, int32 TaskID) {

	if (!WorldContextObject) return false;

	if (auto paintTaskQueue = GetVertexPaintTaskQueue(WorldContextObject))
		return paintTaskQueue->GetCalculateColorsPaintTasks().Contains(TaskID);

	return false;
}


//--------------------------------------------------------

// Does Detect Task Queue Contain ID

bool UVertexPaintFunctionLibrary::DoesDetectTaskQueueContainID(const UObject* WorldContextObject, int32 TaskID) {

	if (!WorldContextObject) return false;

	if (auto paintTaskQueue = GetVertexPaintTaskQueue(WorldContextObject))
		return paintTaskQueue->GetCalculateColorsDetectionTasks().Contains(TaskID);

	return false;
}


#if ENGINE_MAJOR_VERSION == 5


//--------------------------------------------------------

// Get Dynamic Mesh Vertex Colors

TArray<FColor> UVertexPaintFunctionLibrary::GetDynamicMeshVertexColors(UDynamicMeshComponent* DynamicMeshComponent) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetDynamicMeshVertexColors);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - GetDynamicMeshVertexColors");


	TArray<FColor> colorFromLOD;

	if (IsValid(DynamicMeshComponent)) {

		if (DynamicMeshComponent->GetDynamicMesh()) {

			UE::Geometry::FDynamicMesh3* dynamicMesh3 = nullptr;
			dynamicMesh3 = &DynamicMeshComponent->GetDynamicMesh()->GetMeshRef();

			if (dynamicMesh3) {

				colorFromLOD.SetNumUninitialized(dynamicMesh3->MaxVertexID());

				for (int i = 0; i < dynamicMesh3->MaxVertexID(); i++) {

					UE::Geometry::FVertexInfo vertexInfo = UE::Geometry::FVertexInfo();
					vertexInfo = dynamicMesh3->GetVertexInfo(i);

					if (vertexInfo.bHaveC)
						colorFromLOD[i] = (FColor(vertexInfo.Color.X, vertexInfo.Color.Y, vertexInfo.Color.Z, 0));
				}
			}
		}
	}

	return colorFromLOD;
}


//--------------------------------------------------------

// Get Geometry Collection Vertex Colors

TArray<FColor> UVertexPaintFunctionLibrary::GetGeometryCollectionVertexColors(UGeometryCollectionComponent* GeometryCollectionComponent) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetGeometryCollectionVertexColors);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - GetGeometryCollectionVertexColors");


	TArray<FColor> colorFromLOD;


	// Geometry Collection Vertex Colors Only available from 5.3 and up

#if ENGINE_MINOR_VERSION >= 3

	if (IsValid(GeometryCollectionComponent)) {

		UGeometryCollection* geometryCollection = const_cast<UGeometryCollection*>(GeometryCollectionComponent->GetRestCollection());

		if (geometryCollection) {

			TSharedPtr<FGeometryCollection, ESPMode::ThreadSafe> geometryCollectionData = geometryCollection->GetGeometryCollection();

			if (geometryCollectionData.Get()) {

				for (FLinearColor colors : geometryCollectionData->Color)
					colorFromLOD.Add(UVertexPaintFunctionLibrary::ReliableFLinearToFColor(colors));
			}
		}
	}

#endif

	return colorFromLOD;
}
#endif


//-------------------------------------------------------

// Collision Channel to Object Type

EObjectTypeQuery UVertexPaintFunctionLibrary::CollisionChannelToObjectType(ECollisionChannel CollisionChannel) {

	// Has this here since i don't want to include UnrealTypes in.h
	return UEngineTypes::ConvertToObjectType(CollisionChannel);
}


//-------------------------------------------------------

// Object Type to Collision Channel

ECollisionChannel UVertexPaintFunctionLibrary::ObjectTypeToCollisionChannel(EObjectTypeQuery ObjectType) {

	// Has this here since i don't want to include UnrealTypes in.h
	return UEngineTypes::ConvertToCollisionChannel(ObjectType);
}


//-------------------------------------------------------

// Is In Editor

bool UVertexPaintFunctionLibrary::IsPlayInEditor(const UObject* WorldContextObject) {

	if (!IsValid(WorldContextObject)) return false;
	if (!IsValid(WorldContextObject->GetWorld())) return false;
	if (!IsWorldValid(WorldContextObject->GetWorld())) return false;

	return WorldContextObject->GetWorld()->IsPlayInEditor();
}


//-------------------------------------------------------

// Does Physics Surface Belong To Parent Surface

bool UVertexPaintFunctionLibrary::DoesPhysicsSurfaceBelongToPhysicsSurfaceFamily(const UObject* WorldContextObject, TEnumAsByte<EPhysicalSurface> PhysicsSurface, TEnumAsByte<EPhysicalSurface> ParentOfPhysicsSurfaceFamily) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_DoesPhysicsSurfaceBelongToPhysicsSurfaceFamily);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - DoesPhysicsSurfaceBelongToPhysicsSurfaceFamily");

	if (PhysicsSurface == EPhysicalSurface::SurfaceType_Default) return false;
	if (ParentOfPhysicsSurfaceFamily == EPhysicalSurface::SurfaceType_Default) return false;

	if (auto materialDataAsset = UVertexPaintFunctionLibrary::GetVertexPaintMaterialDataAsset(WorldContextObject)) {

		if (!materialDataAsset->GetPhysicsSurfaceFamilies().Contains(ParentOfPhysicsSurfaceFamily)) return false;

		// If its the parent surface itself. 
		if (PhysicsSurface == ParentOfPhysicsSurfaceFamily) return true;

		if (materialDataAsset->GetPhysicsSurfaceFamilies().FindRef(ParentOfPhysicsSurfaceFamily).ChildSurfaces.Contains(PhysicsSurface)) return true;
	}

	return false;
}


//-------------------------------------------------------

// Get Parent Physics Surface

TEnumAsByte<EPhysicalSurface> UVertexPaintFunctionLibrary::GetFirstParentOfPhysicsSurface(const UObject* WorldContextObject, TEnumAsByte<EPhysicalSurface> PhysicsSurface) {

	if (!IsValid(WorldContextObject)) return PhysicsSurface;


	TArray<TEnumAsByte<EPhysicalSurface>> physicalSurfaceParents = GetParentsOfPhysicsSurface_Wrapper(WorldContextObject, PhysicsSurface);
	if (physicalSurfaceParents.Num() > 0)
		return physicalSurfaceParents[0];

	return PhysicsSurface;
}


//-------------------------------------------------------

// Get Vertex Paint Material Interface

TMap<TSoftObjectPtr<UMaterialInterface>, FRVPDPRegisteredMaterialSetting> UVertexPaintFunctionLibrary::GetVertexPaintMaterialInterface_Wrapper(const UObject* WorldContextObject) {

	if (auto materialDataAsset = UVertexPaintFunctionLibrary::GetVertexPaintMaterialDataAsset(WorldContextObject))
		return materialDataAsset->GetVertexPaintMaterialInterface();

	return TMap<TSoftObjectPtr<UMaterialInterface>, FRVPDPRegisteredMaterialSetting>();
}


//-------------------------------------------------------

// Is Material Added To Paint On Material Data Asset

bool UVertexPaintFunctionLibrary::IsMaterialAddedToPaintOnMaterialDataAsset_Wrapper(const UObject* WorldContextObject, TSoftObjectPtr<UMaterialInterface> Material) {

	if (auto materialDataAsset = UVertexPaintFunctionLibrary::GetVertexPaintMaterialDataAsset(WorldContextObject))
		return materialDataAsset->IsMaterialAddedToPaintOnMaterialDataAsset(Material);

	return false;
}


//-------------------------------------------------------

// Get Parents Of Physics Surface

TArray<TEnumAsByte<EPhysicalSurface>> UVertexPaintFunctionLibrary::GetParentsOfPhysicsSurface_Wrapper(const UObject* WorldContextObject, TEnumAsByte<EPhysicalSurface> PhysicalSurface) {

	if (auto materialDataAsset = UVertexPaintFunctionLibrary::GetVertexPaintMaterialDataAsset(WorldContextObject))
		return materialDataAsset->GetParentsOfPhysicsSurface(PhysicalSurface);

	return TArray<TEnumAsByte<EPhysicalSurface>>();
}


//-------------------------------------------------------

// Get Physics Surface Families

TMap<TEnumAsByte<EPhysicalSurface>, FRVPDPRegisteredPhysicsSurfacesSettings> UVertexPaintFunctionLibrary::GetPhysicsSurfaceFamilies_Wrapper(const UObject* WorldContextObject) {

	if (auto materialDataAsset = UVertexPaintFunctionLibrary::GetVertexPaintMaterialDataAsset(WorldContextObject))
		return materialDataAsset->GetPhysicsSurfaceFamilies();

	return TMap<TEnumAsByte<EPhysicalSurface>, FRVPDPRegisteredPhysicsSurfacesSettings>();
}


//-------------------------------------------------------

// Get Physics Surfaces Registered To Material

TArray<TEnumAsByte<EPhysicalSurface>> UVertexPaintFunctionLibrary::GetPhysicsSurfacesRegisteredToMaterial(const UObject* WorldContextObject, UMaterialInterface* Material) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetPhysicsSurfacesRegisteredToMaterial);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - GetPhysicsSurfacesRegisteredToMaterial");

	if (!IsValid(Material)) return TArray<TEnumAsByte<EPhysicalSurface>>();

	if (auto materialDataAsset = UVertexPaintFunctionLibrary::GetVertexPaintMaterialDataAsset(WorldContextObject)) {


		UMaterialInterface* materialToGetPhysicsSurfaces = materialDataAsset->GetRegisteredMaterialFromMaterialInterface(Material);


		materialToGetPhysicsSurfaces = materialDataAsset->GetRegisteredMaterialFromMaterialInterface(materialToGetPhysicsSurfaces);
		if (!IsValid(materialToGetPhysicsSurfaces)) {

			return TArray<TEnumAsByte<EPhysicalSurface>>();
		}

		// if (!MaterialDataAsset->GetVertexPaintMaterialInterface().Contains(materialToGetPhysicsSurfaces)) return TArray<TEnumAsByte<EPhysicalSurface>>();

		TArray<TEnumAsByte<EPhysicalSurface>> physicsSurfacesRegisteredToMaterial;

		// Fills Elements for each channel, even if it may have Default so it will be easier to work with 
		physicsSurfacesRegisteredToMaterial.Add(materialDataAsset->GetVertexPaintMaterialInterface().FindRef(materialToGetPhysicsSurfaces).PaintedAtRed);
		physicsSurfacesRegisteredToMaterial.Add(materialDataAsset->GetVertexPaintMaterialInterface().FindRef(materialToGetPhysicsSurfaces).PaintedAtGreen);
		physicsSurfacesRegisteredToMaterial.Add(materialDataAsset->GetVertexPaintMaterialInterface().FindRef(materialToGetPhysicsSurfaces).PaintedAtBlue);
		physicsSurfacesRegisteredToMaterial.Add(materialDataAsset->GetVertexPaintMaterialInterface().FindRef(materialToGetPhysicsSurfaces).PaintedAtAlpha);

		return physicsSurfacesRegisteredToMaterial;
	}

	return TArray<TEnumAsByte<EPhysicalSurface>>();
}


//-------------------------------------------------------

// Get Channels Physics Surface Is Registered To

void UVertexPaintFunctionLibrary::GetChannelsPhysicsSurfaceIsRegisteredTo(const UObject* WorldContextObject, UMaterialInterface* MaterialToApplyColorsTo, const TEnumAsByte<EPhysicalSurface>& PhysicalSurface, bool& AtRedChannel, bool& AtGreenChannel, bool& AtBlueChannel, bool& AtAlphaChannel) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetChannelsPhysicsSurfaceIsRegisteredTo);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - GetChannelsPhysicsSurfaceIsRegisteredTo");

	AtRedChannel = false;
	AtGreenChannel = false;
	AtBlueChannel = false;
	AtAlphaChannel = false;

	if (PhysicalSurface == EPhysicalSurface::SurfaceType_Default) return;

	UVertexPaintMaterialDataAsset* materialDataAsset = GetVertexPaintMaterialDataAsset(WorldContextObject);
	if (!materialDataAsset) return;

	MaterialToApplyColorsTo = materialDataAsset->GetRegisteredMaterialFromMaterialInterface(MaterialToApplyColorsTo);
	if (!IsValid(MaterialToApplyColorsTo)) return;



	TEnumAsByte<EPhysicalSurface> physicsSurfaceRegisteredAtRed = materialDataAsset->GetVertexPaintMaterialInterface().FindRef(MaterialToApplyColorsTo).PaintedAtRed;
	TEnumAsByte<EPhysicalSurface> physicsSurfaceRegisteredAtGreen = materialDataAsset->GetVertexPaintMaterialInterface().FindRef(MaterialToApplyColorsTo).PaintedAtGreen;
	TEnumAsByte<EPhysicalSurface> physicsSurfaceRegisteredAtBlue = materialDataAsset->GetVertexPaintMaterialInterface().FindRef(MaterialToApplyColorsTo).PaintedAtBlue;
	TEnumAsByte<EPhysicalSurface> physicsSurfaceRegisteredAtAlpha = materialDataAsset->GetVertexPaintMaterialInterface().FindRef(MaterialToApplyColorsTo).PaintedAtAlpha;


	// First Directly checks if the physics surface is registered at any of the channels. 
	if (PhysicalSurface == physicsSurfaceRegisteredAtRed)
		AtRedChannel = true;

	if (PhysicalSurface == physicsSurfaceRegisteredAtGreen)
		AtGreenChannel = true;

	if (PhysicalSurface == physicsSurfaceRegisteredAtBlue)
		AtBlueChannel = true;

	if (PhysicalSurface == physicsSurfaceRegisteredAtAlpha)
		AtAlphaChannel = true;


	// If already gotten something on all we dont have to check the rest
	if (AtRedChannel && AtGreenChannel && AtBlueChannel && AtAlphaChannel)
		return;


	// Gets Parents of Physical Surface, or if Physical Surface Is a parent, it will return itself. 
	TArray<TEnumAsByte<EPhysicalSurface>> parentsSurfaces = materialDataAsset->GetParentsOfPhysicsSurface(PhysicalSurface);

	if (parentsSurfaces.Num() <= 0) return;


	// Then checks on the Physical Surfaces Parents, i.e. if the Physical Surface was for instance Cobble-Sand, and Sand was a Parent, then it will check if Sand is registered on any, so if we're applying Cobble-Sand we can apply to the channel that has Sand. 
	if (!AtRedChannel && parentsSurfaces.Contains(physicsSurfaceRegisteredAtRed))
		AtRedChannel = true;

	if (!AtGreenChannel && parentsSurfaces.Contains(physicsSurfaceRegisteredAtGreen))
		AtGreenChannel = true;

	if (!AtBlueChannel && parentsSurfaces.Contains(physicsSurfaceRegisteredAtBlue))
		AtBlueChannel = true;

	if (!AtAlphaChannel && parentsSurfaces.Contains(physicsSurfaceRegisteredAtAlpha))
		AtAlphaChannel = true;


	// If already gotten something on all we dont have to check the rest
	if (AtRedChannel && AtGreenChannel && AtBlueChannel && AtAlphaChannel)
		return;


	// Checks the Family of the Parent as well, the Children and Blendabe Surfaces
	for (auto parentSurfaceTemp : parentsSurfaces) {

		// If already gotten something on all we dont have to check the rest
		if (AtRedChannel && AtGreenChannel && AtBlueChannel && AtAlphaChannel)
			return;


		FRVPDPRegisteredPhysicsSurfacesSettings physicsSurfaceFamily;

		// Checks the Family of the Parent Surface, if any of it's children is registered at any of the channels. So if Painting with Sand, but has a Child Cobble-Sand registered to a Channel, then we can Apply on that. Same if we're painting with a Child like Cobble-Sand, but neither it or its parent Sand is registered on the Material, but another child of Sand is registered like FootSand or something, then we can apply on that. 
		if (materialDataAsset->GetPhysicsSurfaceFamilies().Contains(parentSurfaceTemp)) {

			physicsSurfaceFamily = materialDataAsset->GetPhysicsSurfaceFamilies().FindRef(parentSurfaceTemp);

			if (physicsSurfaceFamily.ChildSurfaces.Num() > 0) {

				if (!AtRedChannel) {

					if (physicsSurfaceFamily.ChildSurfaces.Contains(physicsSurfaceRegisteredAtRed))
						AtRedChannel = true;
				}

				if (!AtGreenChannel) {

					if (physicsSurfaceFamily.ChildSurfaces.Contains(physicsSurfaceRegisteredAtGreen))
						AtGreenChannel = true;
				}

				if (!AtBlueChannel) {

					if (physicsSurfaceFamily.ChildSurfaces.Contains(physicsSurfaceRegisteredAtBlue))
						AtBlueChannel = true;
				}

				if (!AtAlphaChannel) {

					if (physicsSurfaceFamily.ChildSurfaces.Contains(physicsSurfaceRegisteredAtAlpha))
						AtAlphaChannel = true;
				}
			}
		}


		// If already gotten something on all we dont have to check the rest
		if (AtRedChannel && AtGreenChannel && AtBlueChannel && AtAlphaChannel)
			return;


		// Checks Physics Surfaces that it can Blend into as well, if we're trying to apply a blended surface or if we should apply it on parents or childs of it. In the Example Project for instance we have a Cobble Material with Cobble-Sand on Red Channel, and Cobble-Puddle on Blue, which can blend into Cobble-Mud, which is a child of Mud. So if for instance a wheel that has Mud that is driving over the Cobble Material, we can with this get Cobble-Mud if set to applyOnChannelsThatsChildOfPhysicsSurface, and that it's a child of Mud, which means the Wheel with Mud can paint off itself on the channels that formed Cobble-Mud. Same way other way around if applyOnChannelsWithSamePhysicsParents is true, where a clean wheel can get painted with Mud by Cobble-Mud. 

		// Gets Physics Surface Blend Settings on the Material we're Applying Colors on, for instance the Cobble Material from the Example Project has some registered 
		for (auto& blendSetting : materialDataAsset->GetVertexPaintMaterialInterface().FindRef(MaterialToApplyColorsTo).PhysicsSurfaceBlendingSettings) {


			if (AtRedChannel && AtGreenChannel && AtBlueChannel && AtAlphaChannel)
				break;

			// If we're trying to apply a Blendable Surface, for instance Cobble-Mud, Or we've set to affect childs of a parent, for instance Mud, and we find the Blendable Cobble-Mud as a child
			if (blendSetting.Key == PhysicalSurface || physicsSurfaceFamily.ChildSurfaces.Contains(blendSetting.Key) || parentsSurfaces.Contains(blendSetting.Key)) {

				TArray<TEnumAsByte<EPhysicalSurface>> blendableParentsPhysicsSurface = materialDataAsset->GetParentsOfPhysicsSurface(blendSetting.Key);


				// Now we know that the Blended Surface checks out, and we just need to get which vertex color channels that was the result of that blend, so we can set those channels to return the correct amount. To do this we loop through the physics surfaces that resultet in the blend, and then runs GetVertexColorChannelsPhysicsSurfaceIsRegisteredTo on that physics surface
				for (auto physicsSurfacesThatBlendedIntoEachother : blendSetting.Value.PhysicsSurfacesThatCanBlend) {


					if (AtRedChannel && AtGreenChannel && AtBlueChannel && AtAlphaChannel)
						break;

					bool successfullyGotChannelPhysicsSurfaceIsRegisteredToLocal = false;
					auto channelPhysicsSurfaceIsRegisteredTo = GetVertexColorChannelsPhysicsSurfaceIsRegisteredTo(WorldContextObject, MaterialToApplyColorsTo, physicsSurfacesThatBlendedIntoEachother, successfullyGotChannelPhysicsSurfaceIsRegisteredToLocal);

					if (successfullyGotChannelPhysicsSurfaceIsRegisteredToLocal) {

						for (ESurfaceAtChannel vertexColorChannel : channelPhysicsSurfaceIsRegisteredTo) {

							switch (vertexColorChannel) {

							case ESurfaceAtChannel::Default:
								break;

							case ESurfaceAtChannel::RedChannel:

								if (!AtRedChannel)
									AtRedChannel = true;
								break;

							case ESurfaceAtChannel::GreenChannel:

								if (!AtGreenChannel)
									AtGreenChannel = true;
								break;

							case ESurfaceAtChannel::BlueChannel:

								if (!AtBlueChannel)
									AtBlueChannel = true;
								break;

							case ESurfaceAtChannel::AlphaChannel:

								if (!AtAlphaChannel)
									AtAlphaChannel = true;
								break;

							default:
								break;
							}
						}
					}
				}
			}
		}
	}
}


//-------------------------------------------------------

// Get The Most Dominant Physics Surface

bool UVertexPaintFunctionLibrary::GetTheMostDominantPhysicsSurface_Wrapper(const UObject* WorldContextObject, UMaterialInterface* OptionalMaterialPhysicsSurfaceWasDetectedOn, TArray<TEnumAsByte<EPhysicalSurface>> PhysicsSurfaces, TArray<float> PhysicsSurfaceValues, TEnumAsByte<EPhysicalSurface>& MostDominantPhysicsSurfaceFromArray, float& MostDominantPhysicsSurfaceColorValue) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetTheMostDominantPhysicsSurface);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - GetTheMostDominantPhysicsSurface");

	MostDominantPhysicsSurfaceFromArray = EPhysicalSurface::SurfaceType_Default;
	MostDominantPhysicsSurfaceColorValue = 0;

	if (PhysicsSurfaces.Num() == 0) return false;
	if (PhysicsSurfaceValues.Num() == 0) return false;
	if (PhysicsSurfaces.Num() != PhysicsSurfaceValues.Num()) return false;

	float strongestSurfaceValue = 0;
	TEnumAsByte<EPhysicalSurface> strongestSurface = EPhysicalSurface::SurfaceType_Default;


	// First just gets the surface with the strongest value
	for (int i = 0; i < PhysicsSurfaces.Num(); i++) {

		// Has to have something. So if you send in an array where everything is 0 then you shouldn't be able to get a dominant surface
		if (PhysicsSurfaceValues[i] > 0) {

			if (PhysicsSurfaceValues[i] >= strongestSurfaceValue) {

				strongestSurface = PhysicsSurfaces[i];
				strongestSurfaceValue = PhysicsSurfaceValues[i];
			}
		}
	}


	// If material is registered then checks if any of the surfaces can blend
	if (UVertexPaintMaterialDataAsset* materialDataAsset = GetVertexPaintMaterialDataAsset(WorldContextObject)) {


		OptionalMaterialPhysicsSurfaceWasDetectedOn = materialDataAsset->GetRegisteredMaterialFromMaterialInterface(OptionalMaterialPhysicsSurfaceWasDetectedOn);

		if (OptionalMaterialPhysicsSurfaceWasDetectedOn) {


			TArray<FRVPDPPhysicsSurfaceBlendSettings> surfaceBlendSettings;

			materialDataAsset->GetVertexPaintMaterialInterface().FindRef(OptionalMaterialPhysicsSurfaceWasDetectedOn).PhysicsSurfaceBlendingSettings.GenerateValueArray(surfaceBlendSettings);


			// Then checks if any of the surface we got as a parameter can blendand if their value combined allows them to blendand is then the strongest surface
			if (surfaceBlendSettings.Num() > 0) {


				TArray<TEnumAsByte<EPhysicalSurface>> blendedSurfaceResults;
				TArray<float> blendedSurfaceValueResults;
				TArray<int> amountOfSurfacesThatBlends;

				for (const FRVPDPPhysicsSurfaceBlendSettings& blendSetting : surfaceBlendSettings) {


					bool doesntContainPhysicsSurfaceOrNotMinAmount = false;
					float blendedSurfaces_TotalAmount = 0;

					// If we know we have all phys surfaces requires, then checks if each has min required color amount that they require to blend
					for (auto physSurfaceThatBlend : blendSetting.PhysicsSurfacesThatCanBlend) {

						if (!PhysicsSurfaces.Contains(physSurfaceThatBlend)) {

							doesntContainPhysicsSurfaceOrNotMinAmount = true;
							break;
						}


						const float physicsSurfaceValue = PhysicsSurfaceValues[PhysicsSurfaces.Find(physSurfaceThatBlend.GetValue())];

						if (physicsSurfaceValue >= blendSetting.MinAmountOnEachSurfaceToBeAbleToBlend) {

							blendedSurfaces_TotalAmount += physicsSurfaceValue;
						}

						else {

							doesntContainPhysicsSurfaceOrNotMinAmount = true;
							break;
						}
					}

					if (doesntContainPhysicsSurfaceOrNotMinAmount) continue;


					blendedSurfaceResults.Add(blendSetting.PhysicsSurfaceToResultIn);
					blendedSurfaceValueResults.Add(blendedSurfaces_TotalAmount);
					amountOfSurfacesThatBlends.Add(blendSetting.PhysicsSurfacesThatCanBlend.Num());
				}


				// If got any blendes surfaces then checks if they're stronger than the strongest surface we got earlier when we checked them invidiually and updates it. If there are several blendes surfaces, the one that combines has the strongest value will be considered the strongest surface
				if (blendedSurfaceValueResults.Num() > 0) {

					for (int i = 0; i < blendedSurfaceValueResults.Num(); i++) {

						if (blendedSurfaceValueResults[i] > strongestSurfaceValue) {

							// Since it's made up of several surfaces, that in total could be more than 1, for example if Red Channel (Sand) and Blue Channel (Wet) combine to form Mud, then the blended surface result is Mud with Value 2 when the expected result is Mud with Value 1. 
							strongestSurfaceValue = blendedSurfaceValueResults[i] / amountOfSurfacesThatBlends[i];
							strongestSurface = blendedSurfaceResults[i];
						}
					}
				}
			}
		}
	}

	// Only returns true if we actually got a surface, if they where all 0 so strongest surface is the default 0 then we don't want to return anything surface
	if (strongestSurface != EPhysicalSurface::SurfaceType_Default && strongestSurfaceValue > 0) {

		MostDominantPhysicsSurfaceFromArray = strongestSurface;
		MostDominantPhysicsSurfaceColorValue = strongestSurfaceValue;

		return true;
	}

	return false;
}


//-------------------------------------------------------

// Get Vertex Color Channels Physics Surface Is Registered To Wrapper

TArray<ESurfaceAtChannel> UVertexPaintFunctionLibrary::GetAllVertexColorChannelsPhysicsSurfaceIsRegisteredTo_Wrapper(const UObject* WorldContextObject, UMaterialInterface* Material, TEnumAsByte<EPhysicalSurface> PhysicsSurface, bool& Successful) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetAllVertexColorChannelsPhysicsSurfaceIsRegisteredTo);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - GetAllVertexColorChannelsPhysicsSurfaceIsRegisteredTo");

	Successful = false;
	TArray< ESurfaceAtChannel> surfaceAtChannels; // Array in case it's a blendable where there are several channels that make up the physics surface

	if (!IsValid(Material)) return surfaceAtChannels;

	auto materialDataAsset = GetVertexPaintMaterialDataAsset(WorldContextObject);
	if (!materialDataAsset) return surfaceAtChannels;

	Material = materialDataAsset->GetRegisteredMaterialFromMaterialInterface(Material);
	if (!IsValid(Material)) return surfaceAtChannels;


	FRVPDPRegisteredMaterialSetting registeredMaterialSettings = materialDataAsset->GetVertexPaintMaterialInterface().FindRef(Material);

	// Gets if physics surface is registered to R, G, B or A for this Material, if so then returns
	surfaceAtChannels = GetVertexColorChannelsPhysicsSurfaceIsRegisteredTo(WorldContextObject, Material, PhysicsSurface, Successful);

	if (Successful) {

		return surfaceAtChannels;
	}

	// If couldn't find any, then checks if there are several blendables that make up this surface, and get the surface channels for them. 
	for (auto& blendSetting : registeredMaterialSettings.PhysicsSurfaceBlendingSettings) {

		if (blendSetting.Key == PhysicsSurface) {

			for (auto blendedPhysSurfacesThatMakeUpThePhysicsSurface : blendSetting.Value.PhysicsSurfacesThatCanBlend) {

				TArray< ESurfaceAtChannel> blendableChannelsThatMakeUpPhysicsSurface = GetVertexColorChannelsPhysicsSurfaceIsRegisteredTo(WorldContextObject, Material, blendedPhysSurfacesThatMakeUpThePhysicsSurface, Successful);

				if (Successful)
					surfaceAtChannels.Append(blendableChannelsThatMakeUpPhysicsSurface);
			}

			if (Successful)
				return surfaceAtChannels;
		}
	}

	return surfaceAtChannels;
}


//-------------------------------------------------------

// Get Vertex Color Channels Physics Surface Is Registered To - This just checks the RGBA channels and not Blendables like the one above

TArray<ESurfaceAtChannel> UVertexPaintFunctionLibrary::GetVertexColorChannelsPhysicsSurfaceIsRegisteredTo(const UObject* WorldContextObject, UMaterialInterface* Material, TEnumAsByte<EPhysicalSurface> PhysicsSurface, bool& Successful) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetVertexColorChannelsPhysicsSurfaceIsRegisteredTo);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - GetVertexColorChannelsPhysicsSurfaceIsRegisteredTo");

	Successful = false;

	if (!IsValid(Material)) return TArray<ESurfaceAtChannel>();

	UVertexPaintMaterialDataAsset* materialDataAsset = GetVertexPaintMaterialDataAsset(WorldContextObject);
	if (!materialDataAsset) return TArray<ESurfaceAtChannel>();


	Material = materialDataAsset->GetRegisteredMaterialFromMaterialInterface(Material);
	if (!IsValid(Material)) {

		return TArray<ESurfaceAtChannel>();
	}


	FRVPDPRegisteredMaterialSetting registeredMaterialSettings = materialDataAsset->GetVertexPaintMaterialInterface().FindRef(Material);

	TArray<ESurfaceAtChannel> atSurfaceChannels;


	if (registeredMaterialSettings.AtDefault == PhysicsSurface) {

		Successful = true;
		atSurfaceChannels.Add(ESurfaceAtChannel::Default);
	}

	if (registeredMaterialSettings.PaintedAtRed == PhysicsSurface) {

		Successful = true;
		atSurfaceChannels.Add(ESurfaceAtChannel::RedChannel);
	}

	if (registeredMaterialSettings.PaintedAtGreen == PhysicsSurface) {

		Successful = true;
		atSurfaceChannels.Add(ESurfaceAtChannel::GreenChannel);
	}

	if (registeredMaterialSettings.PaintedAtBlue == PhysicsSurface) {

		Successful = true;
		atSurfaceChannels.Add(ESurfaceAtChannel::BlueChannel);
	}

	if (registeredMaterialSettings.PaintedAtAlpha == PhysicsSurface) {

		Successful = true;
		atSurfaceChannels.Add(ESurfaceAtChannel::AlphaChannel);
	}

	return atSurfaceChannels;
}


//--------------------------------------------------------

// Get Clothing Assets

TArray<UClothingAssetBase*> UVertexPaintFunctionLibrary::GetClothAssets(USkeletalMesh* SkeletalMesh) {

	if (!IsValid(SkeletalMesh)) return TArray<UClothingAssetBase*>();

	return SkeletalMesh->GetMeshClothingAssets();
}


//-------------------------------------------------------

// Set Cloth Physics

void UVertexPaintFunctionLibrary::SetChaosClothPhysics(USkeletalMeshComponent* SkeletalMeshComponent, UClothingAssetBase* ClothingAsset, const FRVPDPChaosClothPhysicsSettings& ClothPhysicsSettings) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_SetChaosClothPhysics);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - SetChaosClothPhysics");

	if (!IsValid(SkeletalMeshComponent)) return;
	if (!ClothingAsset) return;


#if ENGINE_MAJOR_VERSION >= 5

	if (UClothingSimulationInteractor* clothingSimulationInteractor = SkeletalMeshComponent->GetClothingSimulationInteractor()) {

		if (UChaosClothingSimulationInteractor* chaosClothingSimilationInteractor = Cast<UChaosClothingSimulationInteractor>(clothingSimulationInteractor)) {

			if (UClothingInteractor* clothingInteractor = chaosClothingSimilationInteractor->GetClothingInteractor(ClothingAsset->GetName())) {

				if (UChaosClothingInteractor* chaosClothingInteractor = Cast<UChaosClothingInteractor>(clothingInteractor)) {


					if (ClothPhysicsSettings.SetDamping)
						chaosClothingInteractor->SetDamping(ClothPhysicsSettings.ClothDampingSettings.SetDamping_dampingCoefficient);

					if (ClothPhysicsSettings.SetGravity)
						chaosClothingInteractor->SetGravity(ClothPhysicsSettings.ClothGravitySettings.SetGravity_gravityScale, ClothPhysicsSettings.ClothGravitySettings.SetGravity_overrideGravity, ClothPhysicsSettings.ClothGravitySettings.SetGravity_gravityOverride);

					if (ClothPhysicsSettings.SetWind)
						chaosClothingInteractor->SetWind(ClothPhysicsSettings.ClothWindSettings.SetWind_drag, ClothPhysicsSettings.ClothWindSettings.SetWind_lift, ClothPhysicsSettings.ClothWindSettings.SetWind_airDensity, ClothPhysicsSettings.ClothWindSettings.SetWind_windVelocity);

					if (ClothPhysicsSettings.SetAnimDrive)
						chaosClothingInteractor->SetAnimDrive(ClothPhysicsSettings.ClothAnimDriveSettings.SetAnimDrive_Stiffness, ClothPhysicsSettings.ClothAnimDriveSettings.SetAnimDrive_Damping);

					if (ClothPhysicsSettings.SetCollision)
						chaosClothingInteractor->SetCollision(ClothPhysicsSettings.ClothCollisionSettings.SetCollision_CollisionThickness, ClothPhysicsSettings.ClothCollisionSettings.SetCollision_FrictionCoefficient, ClothPhysicsSettings.ClothCollisionSettings.SetCollision_UseCCD, ClothPhysicsSettings.ClothCollisionSettings.SetCollision_SelfCollisionThickness);

					if (ClothPhysicsSettings.SetLongRangeAttachment)
						chaosClothingInteractor->SetLongRangeAttachment(ClothPhysicsSettings.ClothLongRangeAttachmentSettings.LongRangeAttachment_TetherThickness, ClothPhysicsSettings.ClothLongRangeAttachmentSettings.LongRangeAttachment_TetherScale);

					if (ClothPhysicsSettings.SetMaterial)
						chaosClothingInteractor->SetMaterial(ClothPhysicsSettings.ClothMaterialSettings.Material_EdgeStiffness, ClothPhysicsSettings.ClothMaterialSettings.Material_BendingStiffness, ClothPhysicsSettings.ClothMaterialSettings.Material_AreaStiffness);

					if (ClothPhysicsSettings.SetPhysicsVelocityScale)
						chaosClothingInteractor->SetVelocityScale(ClothPhysicsSettings.ClothPhysicsVelocityScaleSettings.PhysicsVelocityScale_LinearVelocityScale, ClothPhysicsSettings.ClothPhysicsVelocityScaleSettings.PhysicVelocityScale_AngularVelocityScale, ClothPhysicsSettings.ClothPhysicsVelocityScaleSettings.PhysicsVelocityScale_FictitiousAngularVelocityScale);

#if ENGINE_MINOR_VERSION >= 1

					if (ClothPhysicsSettings.SetAirPressure) {
						chaosClothingInteractor->SetPressure(ClothPhysicsSettings.ClothPhysicssAirPressureSettings.SetPressure_Pressure);
					}
#endif
				}
			}
		}
	}

#endif
}


//-------------------------------------------------------

// Updated Chaos Cloth Physics With Existing Colors Async

void UVertexPaintFunctionLibrary::UpdateChaosClothPhysicsWithExistingColorsAsync(UObject* WorldContextObject, FLatentActionInfo LatentInfo, USkeletalMeshComponent* skeletalMeshComponent) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_UpdateChaosClothPhysicsWithExistingColorsAsync);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - UpdateChaosClothPhysicsWithExistingColorsAsync");


	// Updating Cloth Physics is a UE5 feature
#if ENGINE_MAJOR_VERSION >= 5

	if (UWorld* World = WorldContextObject->GetWorld()) {


		// Instantiate the AsyncTask
		FRVPDPUpdateClothPhysicsAsyncTask* updateClothPhysicsAsyncTask = new FRVPDPUpdateClothPhysicsAsyncTask(skeletalMeshComponent);


		// Create our custom latent action and add to manager
		FUpdateClothPhysicsLatentAction* LatentAction = new FUpdateClothPhysicsLatentAction(LatentInfo, updateClothPhysicsAsyncTask);
		World->GetLatentActionManager().AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, LatentAction);


		// Bind the delegate and sets the result
		updateClothPhysicsAsyncTask->OnUpdateClothPhysicsAsyncTaskCompleteDelegate.BindLambda([skeletalMeshComponent, LatentAction](const TArray<FRVPDPChaosClothPhysicsSettings>& chaosClothPhysicsSettings) {


			USkeletalMesh* skelMesh = nullptr;

#if ENGINE_MINOR_VERSION == 0
			skelMesh = skeletalMeshComponent->SkeletalMesh.Get();
#else
			skelMesh = skeletalMeshComponent->GetSkeletalMeshAsset();
#endif


			TArray<UClothingAssetBase*> skeletalMeshClothingAssets = skelMesh->GetMeshClothingAssets();

			for (int i = 0; i < skeletalMeshClothingAssets.Num(); i++) {

				if (!chaosClothPhysicsSettings.IsValidIndex(i)) break;


				UVertexPaintFunctionLibrary::SetChaosClothPhysics(skeletalMeshComponent, skeletalMeshClothingAssets[i], chaosClothPhysicsSettings[i]);
			}


			LatentAction->MarkAsCompleted();
		});

		(new FAutoDeleteAsyncTask<FRVPDPUpdateClothPhysicsAsyncTask>(*updateClothPhysicsAsyncTask))->StartBackgroundTask();
	}
#endif

}


//-------------------------------------------------------

// Updated Chaos Cloth Physics With Existing Colors

void UVertexPaintFunctionLibrary::UpdateChaosClothPhysicsWithExistingColors(USkeletalMeshComponent* SkeletalMeshComponent) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_UpdateChaosClothPhysicsWithExistingColors);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - UpdateChaosClothPhysicsWithExistingColors");

#if ENGINE_MAJOR_VERSION >= 5

	if (!IsValid(SkeletalMeshComponent)) return;


	for (auto& clothPhysicsSettings : GetChaosClothPhysicsSettingsBasedOnExistingColors(SkeletalMeshComponent)) {

		UVertexPaintFunctionLibrary::SetChaosClothPhysics(SkeletalMeshComponent,
			clothPhysicsSettings.Key,
			clothPhysicsSettings.Value);
	}

#endif
}

#if ENGINE_MAJOR_VERSION == 5

//-------------------------------------------------------

// Update Chaos Cloth Physics

TMap<UClothingAssetBase*, FRVPDPChaosClothPhysicsSettings> UVertexPaintFunctionLibrary::GetChaosClothPhysicsSettingsBasedOnExistingColors(USkeletalMeshComponent* SkeletalMeshComponent) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetChaosClothPhysicsSettingsBasedOnExistingColors);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - GetChaosClothPhysicsSettingsBasedOnExistingColors");

	TMap<UClothingAssetBase*, FRVPDPChaosClothPhysicsSettings> clothsPhysicsSetting;

	if (!IsValid(SkeletalMeshComponent)) return clothsPhysicsSetting;
	if (!IsWorldValid(SkeletalMeshComponent->GetWorld())) return clothsPhysicsSetting;
	if (!SkeletalMeshComponent->GetSkeletalMeshRenderData()) return clothsPhysicsSetting;
	if (!SkeletalMeshComponent->GetSkeletalMeshRenderData()->LODRenderData.IsValidIndex(SkeletalMeshComponent->GetPredictedLODLevel())) return clothsPhysicsSetting;
	if (!SkeletalMeshComponent->LODInfo.IsValidIndex(SkeletalMeshComponent->GetPredictedLODLevel())) return clothsPhysicsSetting;

	const FSkeletalMeshLODRenderData& skelMeshRenderData = SkeletalMeshComponent->GetSkeletalMeshRenderData()->LODRenderData[SkeletalMeshComponent->GetPredictedLODLevel()];
	FSkelMeshComponentLODInfo* skeletalMeshLODInfo = &SkeletalMeshComponent->LODInfo[SkeletalMeshComponent->GetPredictedLODLevel()];

	if (!skeletalMeshLODInfo) return clothsPhysicsSetting;
	if (!skelMeshRenderData.HasClothData()) return clothsPhysicsSetting;


	TMap<UClothingAssetBase*, FRVPDPVertexChannelsChaosClothPhysicsSettings> clothsAndTheirPhysicsSettings;

	if (UKismetSystemLibrary::DoesImplementInterface(SkeletalMeshComponent->GetOwner(), UVertexPaintDetectionInterface::StaticClass()))
		clothsAndTheirPhysicsSettings = IVertexPaintDetectionInterface::Execute_GetSkeletalMeshClothPhysicsSettings(SkeletalMeshComponent->GetOwner(), SkeletalMeshComponent);

	if (clothsAndTheirPhysicsSettings.Num() == 0) return clothsPhysicsSetting;


	USkeletalMesh* skelMesh = nullptr;

#if ENGINE_MINOR_VERSION == 0
	skelMesh = SkeletalMeshComponent->SkeletalMesh.Get();
#else
	skelMesh = SkeletalMeshComponent->GetSkeletalMeshAsset();
#endif


	TArray<UClothingAssetBase*> clothingAssets = skelMesh->GetMeshClothingAssets();

	// Loops through Cloths
	for (int i = 0; i < clothingAssets.Num(); i++) {


		UClothingAssetBase* clothAsset = clothingAssets[i];

		if (!clothsAndTheirPhysicsSettings.Contains(clothAsset)) continue;


		// Loops through Render Sections on current LOD
		for (int j = 0; j < skelMeshRenderData.RenderSections.Num(); j++) {


			// When on the same section as the Cloth
			if (i == skelMeshRenderData.RenderSections[j].CorrespondClothAssetIndex) {


				// Gets start and end vertex, which could be for instance 2300-2600 or something. So we get vertex colors below for just these verts
				int sectionStartVertexIndex = skelMeshRenderData.RenderSections[j].BaseVertexIndex;
				int sectionEndVertexIndex = sectionStartVertexIndex + skelMeshRenderData.RenderSections[j].GetNumVertices();
				int sectionTotalAmountOfVertices = skelMeshRenderData.RenderSections[j].GetNumVertices();
				float totalRedColor = 0;
				float totalGreenColor = 0;
				float totalBlueColor = 0;
				float totalAlphaColor = 0;


				FColor sectionsVertexColor;
				FLinearColor sectionsAverageVertexColor;

				for (int32 k = sectionStartVertexIndex; k < sectionEndVertexIndex; k++) {

					// If been painted on before then can get the current Instance Color
					if (skeletalMeshLODInfo->OverrideVertexColors)
						sectionsVertexColor = skeletalMeshLODInfo->OverrideVertexColors->VertexColor(k);

					// If haven't been painted on before then we get the Default Color
					else
						sectionsVertexColor = skelMeshRenderData.StaticVertexBuffers.ColorVertexBuffer.VertexColor(k);

					totalRedColor += sectionsVertexColor.R;
					totalGreenColor += sectionsVertexColor.G;
					totalBlueColor += sectionsVertexColor.B;
					totalAlphaColor += sectionsVertexColor.A;
				}


				totalRedColor /= sectionTotalAmountOfVertices;
				totalGreenColor /= sectionTotalAmountOfVertices;
				totalBlueColor /= sectionTotalAmountOfVertices;
				totalAlphaColor /= sectionTotalAmountOfVertices;

				sectionsAverageVertexColor.R = UKismetMathLibrary::MapRangeClamped(totalRedColor, 0, 255, 0, 1);
				sectionsAverageVertexColor.G = UKismetMathLibrary::MapRangeClamped(totalGreenColor, 0, 255, 0, 1);
				sectionsAverageVertexColor.B = UKismetMathLibrary::MapRangeClamped(totalBlueColor, 0, 255, 0, 1);
				sectionsAverageVertexColor.A = UKismetMathLibrary::MapRangeClamped(totalAlphaColor, 0, 255, 0, 1);


				clothsPhysicsSetting.Add(clothAsset,
					GetChaosClothPhysicsSettingsBasedOnAverageVertexColor(clothAsset, sectionsAverageVertexColor, clothsAndTheirPhysicsSettings.FindRef(clothAsset)
					));
			}
		}
	}

	return clothsPhysicsSetting;
}


//-------------------------------------------------------

// Get Chaos Cloth Physics Settings Based On Vertex Colors

FRVPDPChaosClothPhysicsSettings UVertexPaintFunctionLibrary::GetChaosClothPhysicsSettingsBasedOnAverageVertexColor(const UClothingAssetBase* ClothingAsset, FLinearColor AverageColorOnClothingAsset, const FRVPDPVertexChannelsChaosClothPhysicsSettings& ClothPhysicsSettings) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetChaosClothPhysicsSettingsBasedOnAverageVertexColor);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - GetChaosClothPhysicsSettingsBasedOnAverageVertexColor");

	if (!IsValid(ClothingAsset)) return FRVPDPChaosClothPhysicsSettings();

	// https://docs.unrealengine.com/4.27/en-US/API/Plugins/ChaosCloth/ChaosCloth/UChaosClothingInteractor/


	TArray<float> clothPhysicsSettingsAtColorAvarageValue;
	TArray<FRVPDPChaosClothPhysicsAtVertexColorChannelSettings> clothPhysicsSettingsAtColor;


	clothPhysicsSettingsAtColorAvarageValue.Add(AverageColorOnClothingAsset.R);
	clothPhysicsSettingsAtColor.Add(ClothPhysicsSettings.ClothPhysicsSettingsAtRedChannel);

	clothPhysicsSettingsAtColorAvarageValue.Add(AverageColorOnClothingAsset.G);
	clothPhysicsSettingsAtColor.Add(ClothPhysicsSettings.ClothPhysicsSettingsAtGreenChannel);

	clothPhysicsSettingsAtColorAvarageValue.Add(AverageColorOnClothingAsset.B);
	clothPhysicsSettingsAtColor.Add(ClothPhysicsSettings.ClothPhysicsSettingsAtBlueChannel);

	clothPhysicsSettingsAtColorAvarageValue.Add(AverageColorOnClothingAsset.A);
	clothPhysicsSettingsAtColor.Add(ClothPhysicsSettings.ClothPhysicsSettingsAtAlphaChannel);



	// NOTE Removed the minRequiredPaintAmount to affect Gravity or damping etc. because it caused an issue when Removing colors and trying to reset the cloth, since it couldn't do anything if not above 0.25 or something

	FRVPDPChaosClothPhysicsSettings chaosClothPhysicsSettingsLocal;

	// Resets all the settings so we base wind lift etc. all from 0 and not += onto the default values
	chaosClothPhysicsSettingsLocal.ResetAllClothSettings();

	for (int i = 0; i < clothPhysicsSettingsAtColor.Num(); i++) {


		// Damping
		if (clothPhysicsSettingsAtColor[i].SetDamping) {

			chaosClothPhysicsSettingsLocal.SetDamping = true;

			chaosClothPhysicsSettingsLocal.ClothDampingSettings.SetDamping_dampingCoefficient += FMath::Lerp(clothPhysicsSettingsAtColor[i].ClothDampingSettingsWithNoColorPaintedAtChannel.SetDamping_dampingCoefficient, clothPhysicsSettingsAtColor[i].ClothDampingSettingsWithFullColorPaintedAtChannel.SetDamping_dampingCoefficient, clothPhysicsSettingsAtColorAvarageValue[i]);
		}

		// Gravity
		if (clothPhysicsSettingsAtColor[i].SetGravity) {

			chaosClothPhysicsSettingsLocal.SetGravity = true;

			chaosClothPhysicsSettingsLocal.ClothGravitySettings.SetGravity_gravityScale += FMath::Lerp(clothPhysicsSettingsAtColor[i].ClothGravitySettingsWithNoColorPaintedAtChannel.SetGravity_gravityScale, clothPhysicsSettingsAtColor[i].ClothGravitySettingsWithFullColorPaintedAtChannel.SetGravity_gravityScale, clothPhysicsSettingsAtColorAvarageValue[i]);

			chaosClothPhysicsSettingsLocal.ClothGravitySettings.SetGravity_gravityOverride += FMath::Lerp(clothPhysicsSettingsAtColor[i].ClothGravitySettingsWithNoColorPaintedAtChannel.SetGravity_gravityOverride, clothPhysicsSettingsAtColor[i].ClothGravitySettingsWithFullColorPaintedAtChannel.SetGravity_gravityOverride, clothPhysicsSettingsAtColorAvarageValue[i]);

			// If at least 1 color has set it to override gravity then that is what it will do
			if (clothPhysicsSettingsAtColorAvarageValue[i] > 0.5) {

				// If already true then can't set it to false
				if (!chaosClothPhysicsSettingsLocal.ClothGravitySettings.SetGravity_overrideGravity)
					chaosClothPhysicsSettingsLocal.ClothGravitySettings.SetGravity_overrideGravity = clothPhysicsSettingsAtColor[i].ClothGravitySettingsWithFullColorPaintedAtChannel.SetGravity_overrideGravity;
			}

			else {

				if (!chaosClothPhysicsSettingsLocal.ClothGravitySettings.SetGravity_overrideGravity)
					chaosClothPhysicsSettingsLocal.ClothGravitySettings.SetGravity_overrideGravity = clothPhysicsSettingsAtColor[i].ClothGravitySettingsWithNoColorPaintedAtChannel.SetGravity_overrideGravity;
			}
		}

		// Wind
		if (clothPhysicsSettingsAtColor[i].SetWind) {

			chaosClothPhysicsSettingsLocal.SetWind = true;

			chaosClothPhysicsSettingsLocal.ClothWindSettings.SetWind_drag += FMath::Lerp(clothPhysicsSettingsAtColor[i].ClothWindSettingsWithNoColorPaintedAtChannel.SetWind_drag, clothPhysicsSettingsAtColor[i].ClothWindSettingsWithFullColorPaintedAtChannel.SetWind_drag, clothPhysicsSettingsAtColorAvarageValue[i]);

			chaosClothPhysicsSettingsLocal.ClothWindSettings.SetWind_lift += FMath::Lerp(clothPhysicsSettingsAtColor[i].ClothWindSettingsWithNoColorPaintedAtChannel.SetWind_lift, clothPhysicsSettingsAtColor[i].ClothWindSettingsWithFullColorPaintedAtChannel.SetWind_lift, clothPhysicsSettingsAtColorAvarageValue[i]);

			chaosClothPhysicsSettingsLocal.ClothWindSettings.SetWind_airDensity += FMath::Lerp(clothPhysicsSettingsAtColor[i].ClothWindSettingsWithNoColorPaintedAtChannel.SetWind_airDensity, clothPhysicsSettingsAtColor[i].ClothWindSettingsWithFullColorPaintedAtChannel.SetWind_airDensity, clothPhysicsSettingsAtColorAvarageValue[i]);

			chaosClothPhysicsSettingsLocal.ClothWindSettings.SetWind_windVelocity += FMath::Lerp(clothPhysicsSettingsAtColor[i].ClothWindSettingsWithNoColorPaintedAtChannel.SetWind_windVelocity, clothPhysicsSettingsAtColor[i].ClothWindSettingsWithFullColorPaintedAtChannel.SetWind_windVelocity, clothPhysicsSettingsAtColorAvarageValue[i]);
		}

		// Anim Drive
		if (clothPhysicsSettingsAtColor[i].SetAnimDrive) {

			chaosClothPhysicsSettingsLocal.SetAnimDrive = true;

			chaosClothPhysicsSettingsLocal.ClothAnimDriveSettings.SetAnimDrive_Stiffness += FMath::Lerp(clothPhysicsSettingsAtColor[i].ClothAnimDriveSettingsWithNoColorPaintedAtChannel.SetAnimDrive_Stiffness, clothPhysicsSettingsAtColor[i].ClothAnimDriveSettingsWithFullColorPaintedAtChannel.SetAnimDrive_Stiffness, clothPhysicsSettingsAtColorAvarageValue[i]);

			chaosClothPhysicsSettingsLocal.ClothAnimDriveSettings.SetAnimDrive_Damping += FMath::Lerp(clothPhysicsSettingsAtColor[i].ClothAnimDriveSettingsWithNoColorPaintedAtChannel.SetAnimDrive_Damping, clothPhysicsSettingsAtColor[i].ClothAnimDriveSettingsWithFullColorPaintedAtChannel.SetAnimDrive_Damping, clothPhysicsSettingsAtColorAvarageValue[i]);

		}

		// Collision
		if (clothPhysicsSettingsAtColor[i].SetCollision) {

			chaosClothPhysicsSettingsLocal.SetCollision = true;

			chaosClothPhysicsSettingsLocal.ClothCollisionSettings.SetCollision_CollisionThickness += FMath::Lerp(clothPhysicsSettingsAtColor[i].ClothCollisionSettingsWithNoColorPaintedAtChannel.SetCollision_CollisionThickness, clothPhysicsSettingsAtColor[i].ClothCollisionSettingsWithFullColorPaintedAtChannel.SetCollision_CollisionThickness, clothPhysicsSettingsAtColorAvarageValue[i]);

			chaosClothPhysicsSettingsLocal.ClothCollisionSettings.SetCollision_FrictionCoefficient += FMath::Lerp(clothPhysicsSettingsAtColor[i].ClothCollisionSettingsWithNoColorPaintedAtChannel.SetCollision_FrictionCoefficient, clothPhysicsSettingsAtColor[i].ClothCollisionSettingsWithFullColorPaintedAtChannel.SetCollision_FrictionCoefficient, clothPhysicsSettingsAtColorAvarageValue[i]);

			chaosClothPhysicsSettingsLocal.ClothCollisionSettings.SetCollision_SelfCollisionThickness += FMath::Lerp(clothPhysicsSettingsAtColor[i].ClothCollisionSettingsWithNoColorPaintedAtChannel.SetCollision_SelfCollisionThickness, clothPhysicsSettingsAtColor[i].ClothCollisionSettingsWithFullColorPaintedAtChannel.SetCollision_SelfCollisionThickness, clothPhysicsSettingsAtColorAvarageValue[i]);


			// If at least 1 color has set it to use CCD then that is what it will do
			if (clothPhysicsSettingsAtColorAvarageValue[i] > 0.5) {

				if (!chaosClothPhysicsSettingsLocal.ClothCollisionSettings.SetCollision_UseCCD)
					chaosClothPhysicsSettingsLocal.ClothCollisionSettings.SetCollision_UseCCD = clothPhysicsSettingsAtColor[i].ClothCollisionSettingsWithFullColorPaintedAtChannel.SetCollision_UseCCD;
			}

			else {

				if (!chaosClothPhysicsSettingsLocal.ClothCollisionSettings.SetCollision_UseCCD)
					chaosClothPhysicsSettingsLocal.ClothCollisionSettings.SetCollision_UseCCD = clothPhysicsSettingsAtColor[i].ClothCollisionSettingsWithNoColorPaintedAtChannel.SetCollision_UseCCD;
			}
		}

		// Long Range Attachment
		if (clothPhysicsSettingsAtColor[i].SetLongRangeAttachment) {

			chaosClothPhysicsSettingsLocal.SetLongRangeAttachment = true;

			chaosClothPhysicsSettingsLocal.ClothLongRangeAttachmentSettings.LongRangeAttachment_TetherThickness += FMath::Lerp(clothPhysicsSettingsAtColor[i].ClothLongRangeAttachmentSettingsWithNoColorPaintedAtChannel.LongRangeAttachment_TetherThickness, clothPhysicsSettingsAtColor[i].ClothLongRangeAttachmentSettingsWithFullColorPaintedAtChannel.LongRangeAttachment_TetherThickness, clothPhysicsSettingsAtColorAvarageValue[i]);

			chaosClothPhysicsSettingsLocal.ClothLongRangeAttachmentSettings.LongRangeAttachment_TetherScale += FMath::Lerp(clothPhysicsSettingsAtColor[i].ClothLongRangeAttachmentSettingsWithNoColorPaintedAtChannel.LongRangeAttachment_TetherScale, clothPhysicsSettingsAtColor[i].ClothLongRangeAttachmentSettingsWithFullColorPaintedAtChannel.LongRangeAttachment_TetherScale, clothPhysicsSettingsAtColorAvarageValue[i]);
		}

		// Material
		if (clothPhysicsSettingsAtColor[i].SetMaterial) {

			chaosClothPhysicsSettingsLocal.SetMaterial = true;

			chaosClothPhysicsSettingsLocal.ClothMaterialSettings.Material_EdgeStiffness += FMath::Lerp(clothPhysicsSettingsAtColor[i].ClothMaterialSettingsWithNoColorPaintedAtChannel.Material_EdgeStiffness, clothPhysicsSettingsAtColor[i].ClothMaterialSettingsWithFullColorPaintedAtChannel.Material_EdgeStiffness, clothPhysicsSettingsAtColorAvarageValue[i]);

			chaosClothPhysicsSettingsLocal.ClothMaterialSettings.Material_BendingStiffness += FMath::Lerp(clothPhysicsSettingsAtColor[i].ClothMaterialSettingsWithNoColorPaintedAtChannel.Material_BendingStiffness, clothPhysicsSettingsAtColor[i].ClothMaterialSettingsWithFullColorPaintedAtChannel.Material_BendingStiffness, clothPhysicsSettingsAtColorAvarageValue[i]);

			chaosClothPhysicsSettingsLocal.ClothMaterialSettings.Material_AreaStiffness += FMath::Lerp(clothPhysicsSettingsAtColor[i].ClothMaterialSettingsWithNoColorPaintedAtChannel.Material_AreaStiffness, clothPhysicsSettingsAtColor[i].ClothMaterialSettingsWithFullColorPaintedAtChannel.Material_AreaStiffness, clothPhysicsSettingsAtColorAvarageValue[i]);
		}

		// Physics Velocity Scale
		if (clothPhysicsSettingsAtColor[i].SetPhysicsVelocityScale) {

			chaosClothPhysicsSettingsLocal.SetPhysicsVelocityScale = true;

			chaosClothPhysicsSettingsLocal.ClothPhysicsVelocityScaleSettings.PhysicsVelocityScale_LinearVelocityScale += FMath::Lerp(clothPhysicsSettingsAtColor[i].ClothPhysicsVelocityScaleSettingsWithNoColorPaintedAtChannel.PhysicsVelocityScale_LinearVelocityScale, clothPhysicsSettingsAtColor[i].ClothPhysicsVelocityScaleSettingsWithFullColorPaintedAtChannel.PhysicsVelocityScale_LinearVelocityScale, clothPhysicsSettingsAtColorAvarageValue[i]);

			chaosClothPhysicsSettingsLocal.ClothPhysicsVelocityScaleSettings.PhysicVelocityScale_AngularVelocityScale += FMath::Lerp(clothPhysicsSettingsAtColor[i].ClothPhysicsVelocityScaleSettingsWithNoColorPaintedAtChannel.PhysicVelocityScale_AngularVelocityScale, clothPhysicsSettingsAtColor[i].ClothPhysicsVelocityScaleSettingsWithFullColorPaintedAtChannel.PhysicVelocityScale_AngularVelocityScale, clothPhysicsSettingsAtColorAvarageValue[i]);

			chaosClothPhysicsSettingsLocal.ClothPhysicsVelocityScaleSettings.PhysicsVelocityScale_FictitiousAngularVelocityScale += FMath::Lerp(clothPhysicsSettingsAtColor[i].ClothPhysicsVelocityScaleSettingsWithNoColorPaintedAtChannel.PhysicsVelocityScale_FictitiousAngularVelocityScale, clothPhysicsSettingsAtColor[i].ClothPhysicsVelocityScaleSettingsWithFullColorPaintedAtChannel.PhysicsVelocityScale_FictitiousAngularVelocityScale, clothPhysicsSettingsAtColorAvarageValue[i]);
		}

		// Air Pressure
		if (clothPhysicsSettingsAtColor[i].SetAirPressure) {

			chaosClothPhysicsSettingsLocal.SetAirPressure = true;

			chaosClothPhysicsSettingsLocal.ClothPhysicssAirPressureSettings.SetPressure_Pressure += FMath::Lerp(clothPhysicsSettingsAtColor[i].ClothPhysicssAirPressureWithNoColorPaintedAtChannel.SetPressure_Pressure, clothPhysicsSettingsAtColor[i].ClothPhysicssAirPressureWithFullColorPaintedAtChannel.SetPressure_Pressure, clothPhysicsSettingsAtColorAvarageValue[i]);
		}
	}

	return chaosClothPhysicsSettingsLocal;
}

#endif



//-------------------------------------------------------

// Get Physical Material Using Physics Surface

UPhysicalMaterial* UVertexPaintFunctionLibrary::GetPhysicalMaterialUsingPhysicsSurface_Wrapper(const UObject* WorldContextObject, TSubclassOf<UPhysicalMaterial> PhysicalMaterialClass, TEnumAsByte<EPhysicalSurface> PhysicsSurface) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetPhysicalMaterialUsingPhysicsSurface_Wrapper);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - GetPhysicalMaterialUsingPhysicsSurface_Wrapper");

	if (PhysicsSurface == EPhysicalSurface::SurfaceType_Default) return nullptr;
	if (!PhysicalMaterialClass) return nullptr;
	if (!IsValid(WorldContextObject)) return nullptr;
	if (!IsValid(GetVertexPaintGameInstanceSubsystem(WorldContextObject))) return nullptr;


	if (auto physicsMaterial = GetVertexPaintGameInstanceSubsystem(WorldContextObject)->GetAllCachedPhysicsMaterialAssets().FindRef(PhysicsSurface)) {

		// If set to only check in specific class then makes sure that the physical material is that class
		if (PhysicalMaterialClass->GetClass() && physicsMaterial->GetClass() == PhysicalMaterialClass) {

			return physicsMaterial;
		}
		else {

			return physicsMaterial;
		}
	}

	return nullptr;
}


//-------------------------------------------------------

// Remove Component From Paint Task Queue

void UVertexPaintFunctionLibrary::RemoveComponentFromPaintTaskQueue(UPrimitiveComponent* Component) {

	if (!IsValid(Component)) return;
	if (!IsWorldValid(Component->GetWorld())) return;
	if (!IsValid(GetVertexPaintGameInstanceSubsystem(Component->GetWorld()))) return;


	if (auto paintTaskQueue = GetVertexPaintTaskQueue(Component->GetWorld()))
		return paintTaskQueue->RemoveMeshComponentFromPaintTaskQueue(Component, true, true);
}


//-------------------------------------------------------

// Remove Component From Detect Queue

void UVertexPaintFunctionLibrary::RemoveComponentFromDetectTaskQueue(UPrimitiveComponent* Component) {

	if (!IsValid(Component)) return;
	if (!IsWorldValid(Component->GetWorld())) return;
	if (!IsValid(GetVertexPaintGameInstanceSubsystem(Component->GetWorld()))) return;


	if (auto paintTaskQueue = GetVertexPaintTaskQueue(Component->GetWorld()))
		return paintTaskQueue->RemoveMeshComponentFromDetectionTaskQueue(Component, true, true);
}


//-------------------------------------------------------

// Get Mesh Component Source Mesh

const UObject* UVertexPaintFunctionLibrary::GetMeshComponentSourceMesh(UPrimitiveComponent* MeshComponent) {

	if (!IsValid(MeshComponent)) return nullptr;


	const UObject* sourceMesh = nullptr;

	if (auto staticMeshComp = Cast<UStaticMeshComponent>(MeshComponent)) {

		sourceMesh = staticMeshComp->GetStaticMesh();
	}

	else if (auto skelMeshComp = Cast<USkeletalMeshComponent>(MeshComponent)) {

#if ENGINE_MAJOR_VERSION == 4

		sourceMesh = skelMeshComp->SkeletalMesh;

#elif ENGINE_MAJOR_VERSION == 5

#if ENGINE_MINOR_VERSION == 0

		sourceMesh = skelMeshComp->SkeletalMesh.Get();

#else

		sourceMesh = skelMeshComp->GetSkeletalMeshAsset();

#endif
#endif
	}

#if ENGINE_MAJOR_VERSION == 5

	else if (auto dynamicMeshComponent = Cast<UDynamicMeshComponent>(MeshComponent)) {

		// Dynamic Mesh Comps doesn't have a source mesh
	}

	else if (auto geometryCollectionComponent = Cast<UGeometryCollectionComponent>(MeshComponent)) {

		sourceMesh = geometryCollectionComponent->GetRestCollection();
	}
#endif

	return sourceMesh;
}


//--------------------------------------------------------

// CalcAABB Without Uniform Check

FBox UVertexPaintFunctionLibrary::CalcAABBWithoutUniformCheck(const USkinnedMeshComponent* MeshComp, const FTransform& LocalToWorld) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_CalcAABBWithoutUniformCheck);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - CalcAABBWithoutUniformCheck");

	// This got copied from UPhysicsAsset::CalcAABB on 1Dec 2023 UE5.0 with just the IsUniform check commented out, since we couldn't use it with Paint Within Area if either X, Y, Z had different scale from one another. 

	FBox Box(ForceInit);

#if ENGINE_MAJOR_VERSION == 5

	if (!MeshComp)
	{
		return Box;
	}

	const TArray<int32>* BodyIndexRefs = NULL;
	TArray<int32> AllBodies;
	// If we want to consider all bodies, make array with all body indices in
	if (MeshComp->bConsiderAllBodiesForBounds)
	{
		AllBodies.AddUninitialized(MeshComp->GetPhysicsAsset()->SkeletalBodySetups.Num());
		for (int32 i = 0; i < MeshComp->GetPhysicsAsset()->SkeletalBodySetups.Num(); i++)
		{
			AllBodies[i] = i;
		}
		BodyIndexRefs = &AllBodies;
	}
	// Otherwise, use the cached shortlist of bodies to consider
	else
	{
		BodyIndexRefs = &MeshComp->GetPhysicsAsset()->BoundsBodies;
	}

	// Then iterate over bodies we want to consider, calculating bounding box for each
	const int32 BodySetupNum = (*BodyIndexRefs).Num();

	for (int32 i = 0; i < BodySetupNum; i++)
	{
		const int32 BodyIndex = (*BodyIndexRefs)[i];
		bool bConsideredForBounds = false;
		FName boneName = "";
		FKAggregateGeom aggGeom;

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5

		if (USkeletalBodySetup* skeletalBodySetup = MeshComp->GetPhysicsAsset()->SkeletalBodySetups[BodyIndex].Get()) {

			bConsideredForBounds = skeletalBodySetup->bConsiderForBounds;
			boneName = skeletalBodySetup->BoneName;
			aggGeom = skeletalBodySetup->AggGeom;
		}

#else

		if (UBodySetup* bodySetup = MeshComp->GetPhysicsAsset()->SkeletalBodySetups[BodyIndex].Get()) {

			bConsideredForBounds = bodySetup->bConsiderForBounds;
			boneName = bodySetup->BoneName;
			aggGeom = bodySetup->AggGeom;
		}

#endif


		// Check if setup should be considered for bounds, or if all bodies should be considered anyhow
		if (bConsideredForBounds || MeshComp->bConsiderAllBodiesForBounds)
		{
			if (i + 1 < BodySetupNum)
			{
				int32 NextIndex = (*BodyIndexRefs)[i + 1];
				FPlatformMisc::Prefetch(MeshComp->GetPhysicsAsset()->SkeletalBodySetups[NextIndex]);
				FPlatformMisc::Prefetch(MeshComp->GetPhysicsAsset()->SkeletalBodySetups[NextIndex], PLATFORM_CACHE_LINE_SIZE);
			}

			int32 BoneIndex = MeshComp->GetBoneIndex(boneName);
			if (BoneIndex != INDEX_NONE)
			{
				const FTransform WorldBoneTransform = MeshComp->GetBoneTransform(BoneIndex, LocalToWorld);
				FBox BodySetupBounds = aggGeom.CalcAABB(WorldBoneTransform);

				// When the transform contains a negative scale CalcAABB could return a invalid FBox that has Min and Max reversed
				// @TODO: Maybe CalcAABB should handle that inside and never return a reversed FBox
				if (BodySetupBounds.Min.X > BodySetupBounds.Max.X)
				{
					Swap(BodySetupBounds.Min.X, BodySetupBounds.Max.X);
				}

				if (BodySetupBounds.Min.Y > BodySetupBounds.Max.Y)
				{
					Swap(BodySetupBounds.Min.Y, BodySetupBounds.Max.Y);
				}

				if (BodySetupBounds.Min.Z > BodySetupBounds.Max.Z)
				{
					Swap(BodySetupBounds.Min.Z, BodySetupBounds.Max.Z);
				}

				Box += BodySetupBounds;
			}
		}
	}
	//}
	// else
	//{
	//	UE_LOG(LogPhysics, Log, TEXT("UPhysicsAsset::CalcAABB : Non-uniform scale factor. You will not be able to collide with it.  Turn off collision and wrap it with a blocking volume.  MeshComp: %s  SkelMesh: %s"), *MeshComp->GetFullName(), MeshComp->SkeletalMesh ? *MeshComp->SkeletalMesh->GetFullName() : TEXT("NULL"));
	//}

	if (!Box.IsValid)
	{
		Box = FBox(LocalToWorld.GetLocation(), LocalToWorld.GetLocation());
	}

	const float MinBoundSize = 1.f;
	const FVector BoxSize = Box.GetSize();

	if (BoxSize.GetMin() < MinBoundSize)
	{
		const FVector ExpandByDelta(FMath::Max<FVector::FReal>(0, MinBoundSize - BoxSize.X), FMath::Max<FVector::FReal>(0, MinBoundSize - BoxSize.Y), FMath::Max<FVector::FReal>(0, MinBoundSize - BoxSize.Z));
		Box = Box.ExpandBy(ExpandByDelta * 0.5f);	//expand by applies to both directions with GetSize applies to total size so divide by 2
	}

#elif ENGINE_MAJOR_VERSION == 4

	if (!MeshComp)
	{
		return Box;
	}

	FVector Scale3D = LocalToWorld.GetScale3D();
	// if (Scale3D.IsUniform())
	// {
	const TArray<int32>* BodyIndexRefs = NULL;
	TArray<int32> AllBodies;
	// If we want to consider all bodies, make array with all body indices in
	if (MeshComp->bConsiderAllBodiesForBounds)
	{
		AllBodies.AddUninitialized(MeshComp->GetPhysicsAsset()->SkeletalBodySetups.Num());
		for (int32 i = 0; i < MeshComp->GetPhysicsAsset()->SkeletalBodySetups.Num(); i++)
		{
			AllBodies[i] = i;
		}
		BodyIndexRefs = &AllBodies;
	}
	// Otherwise, use the cached shortlist of bodies to consider
	else
	{
		BodyIndexRefs = &MeshComp->GetPhysicsAsset()->BoundsBodies;
	}

	// Then iterate over bodies we want to consider, calculating bounding box for each
	const int32 BodySetupNum = (*BodyIndexRefs).Num();

	for (int32 i = 0; i < BodySetupNum; i++)
	{
		const int32 BodyIndex = (*BodyIndexRefs)[i];
		UBodySetup* bs = MeshComp->GetPhysicsAsset()->SkeletalBodySetups[BodyIndex];

		// Check if setup should be considered for bounds, or if all bodies should be considered anyhow
		if (bs->bConsiderForBounds || MeshComp->bConsiderAllBodiesForBounds)
		{
			if (i + 1 < BodySetupNum)
			{
				int32 NextIndex = (*BodyIndexRefs)[i + 1];
				FPlatformMisc::Prefetch(MeshComp->GetPhysicsAsset()->SkeletalBodySetups[NextIndex]);
				FPlatformMisc::Prefetch(MeshComp->GetPhysicsAsset()->SkeletalBodySetups[NextIndex], PLATFORM_CACHE_LINE_SIZE);
			}

			int32 BoneIndex = MeshComp->GetBoneIndex(bs->BoneName);
			if (BoneIndex != INDEX_NONE)
			{
				const FTransform WorldBoneTransform = MeshComp->GetBoneTransform(BoneIndex, LocalToWorld);
				FBox BodySetupBounds = bs->AggGeom.CalcAABB(WorldBoneTransform);

				// When the transform contains a negative scale CalcAABB could return a invalid FBox that has Min and Max reversed
				// @TODO: Maybe CalcAABB should handle that inside and never return a reversed FBox
				if (BodySetupBounds.Min.X > BodySetupBounds.Max.X)
				{
					Swap<float>(BodySetupBounds.Min.X, BodySetupBounds.Max.X);
				}

				if (BodySetupBounds.Min.Y > BodySetupBounds.Max.Y)
				{
					Swap<float>(BodySetupBounds.Min.Y, BodySetupBounds.Max.Y);
				}

				if (BodySetupBounds.Min.Z > BodySetupBounds.Max.Z)
				{
					Swap<float>(BodySetupBounds.Min.Z, BodySetupBounds.Max.Z);
				}

				Box += BodySetupBounds;
			}
		}
	}
	// }
	// else
	//{
	//	UE_LOG(LogPhysics, Log, TEXT("UPhysicsAsset::CalcAABB : Non-uniform scale factor. You will not be able to collide with it.  Turn off collision and wrap it with a blocking volume.  MeshComp: %s  SkelMesh: %s"), *MeshComp->GetFullName(), MeshComp->SkeletalMesh ? *MeshComp->SkeletalMesh->GetFullName() : TEXT("NULL"));
	//}

	if (!Box.IsValid)
	{
		Box = FBox(LocalToWorld.GetLocation(), LocalToWorld.GetLocation());
	}

	const float MinBoundSize = 1.f;
	const FVector BoxSize = Box.GetSize();

	if (BoxSize.GetMin() < MinBoundSize)
	{
		const FVector ExpandByDelta(FMath::Max(0.f, MinBoundSize - BoxSize.X), FMath::Max(0.f, MinBoundSize - BoxSize.Y), FMath::Max(0.f, MinBoundSize - BoxSize.Z));
		Box = Box.ExpandBy(ExpandByDelta * 0.5f);	//expand by applies to both directions with GetSize applies to total size so divide by 2
	}

#endif

	return Box;
}


//--------------------------------------------------------

// Get Substring After Last Character

FString UVertexPaintFunctionLibrary::GetSubstringAfterLastCharacter(const FString& String, const FString& Character) {

	if (String.IsEmpty() || Character.IsEmpty()) return FString();

	const TCHAR CharToFind = Character[0];

	int32 LastCharPos;
	if (String.FindLastChar(CharToFind, LastCharPos)) {
		return String.Mid(LastCharPos + 1);
	}

	return String;
}


//--------------------------------------------------------

// Get All Physics Surfaces

TArray<TEnumAsByte<EPhysicalSurface>> UVertexPaintFunctionLibrary::GetAllPhysicsSurfaces() {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetAllPhysicsSurfaces);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - GetAllPhysicsSurfaces");

	TArray<TEnumAsByte<EPhysicalSurface>> physicalSurfaces;

	TEnumAsByte<EPhysicalSurface> cachedPhysicalSurface = EPhysicalSurface::SurfaceType1;

	for (int i = 0; i < GetDefault<UPhysicsSettings>()->PhysicalSurfaces.Num(); i++) {

		auto physSurfaceName = GetDefault<UPhysicsSettings>()->PhysicalSurfaces[i];


		/*
		Checks if the first elements, or if any elements in between physics surfaces are None, if so, we have to Add whatever Surface Type is meant to be there, so filling surfaces to choose from in the Editor Widget, they match what actual surface type we can select.

		For example if there is a Gap between Surface type 12 and 14, where 13 is None, then it won't be apart of GetDefault<UPhysicsSettings>()->PhysicalSurfaces, so if we fill a drop down list without it, and select a physics surface that comes After 13, then the selected index from the drop down list will not match the actual physics surface we selected because the amount is different. The total amount up to the last registered physics surface may be 18, but we only get 17 because 13 is None, which means we may think we select 17 but actually getting 16, meaning the wrong thing was registered at the vertex color channel than the user intended.

		So to fix this we simply Add onto the list we return with the missing physics surface, so we actually return the complete list up until the very last registered physics surface.

		This should be a pretty rare edge case but better to safeguard so no one runs into this issue.
		*/


		// If at the first element
		if (i == 0) {

			// but first registered physics surface is more than 1, then we manually add SurfaceType1, 2 etc. until we reach whatever element the first actually registered phys surface is. 
			if (physSurfaceName.Type.GetValue() > 1) {

				for (int j = 1; j < physSurfaceName.Type.GetValue(); j++) {

					cachedPhysicalSurface = static_cast<EPhysicalSurface>(j);
					physicalSurfaces.Add(cachedPhysicalSurface);
				}


				// When finished filling in default physics surfaces, then we add the actually first registered physics surface
				physicalSurfaces.Add(physSurfaceName.Type);
			}


			else {

				physicalSurfaces.Add(physSurfaceName.Type);
			}
		}

		// If it has skipped a element, for example jumped from 12 to 15, indicating that 13 and 14 is None, then we add the missing surface
		else if (cachedPhysicalSurface.GetValue() + 1 != physSurfaceName.Type.GetValue()) {


			int different = physSurfaceName.Type.GetValue() - (cachedPhysicalSurface.GetValue() + 1);

			for (int j = 0; j < different; j++) {

				cachedPhysicalSurface = static_cast<EPhysicalSurface>(cachedPhysicalSurface.GetValue() + 1);
				physicalSurfaces.Add(cachedPhysicalSurface);
			}


			// When finished filling in physics surfaces in between where there where Nones, then we add the registered physics surface
			physicalSurfaces.Add(physSurfaceName.Type);
		}


		// If the next physics surface is the expected value then just adds it
		else if (cachedPhysicalSurface.GetValue() + 1 == physSurfaceName.Type.GetValue()) {

			physicalSurfaces.Add(physSurfaceName.Type);
		}


		// Caches the latest one so we can check against it the next loop if there is a diff to what we expect
		cachedPhysicalSurface = physSurfaceName.Type;
	}

	return physicalSurfaces;
}


//--------------------------------------------------------

// Reliable FLinear To FColor

FColor UVertexPaintFunctionLibrary::ReliableFLinearToFColor(FLinearColor LinearColor) {

	return FColor(static_cast<uint8>(UKismetMathLibrary::MapRangeClamped(LinearColor.R, 0, 1, 0, 255)),
		static_cast<uint8>(UKismetMathLibrary::MapRangeClamped(LinearColor.G, 0, 1, 0, 255)),
		static_cast<uint8>(UKismetMathLibrary::MapRangeClamped(LinearColor.B, 0, 1, 0, 255)),
		static_cast<uint8>(UKismetMathLibrary::MapRangeClamped(LinearColor.A, 0, 1, 0, 255)));
}


//--------------------------------------------------------

// Reliable FColor To FLinearColor

FLinearColor UVertexPaintFunctionLibrary::ReliableFColorToFLinearColor(FColor Color) {

	return FLinearColor(UKismetMathLibrary::MapRangeClamped(static_cast<float>(Color.R), 0, 255, 0, 1),
		UKismetMathLibrary::MapRangeClamped(static_cast<float>(Color.G), 0, 255, 0, 1),
		UKismetMathLibrary::MapRangeClamped(static_cast<float>(Color.B), 0, 255, 0, 1),
		UKismetMathLibrary::MapRangeClamped(static_cast<float>(Color.A), 0, 255, 0, 1));
}


//--------------------------------------------------------

// Get Component Bounds Top World Z

float UVertexPaintFunctionLibrary::GetComponentBoundsTopWorldZ(UPrimitiveComponent* Component) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetComponentBoundsTopWorldZ);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - GetComponentBoundsTopWorldZ");

	if (!IsValid(Component)) return 0;


	if (auto boxComp = Cast<UBoxComponent>(Component)) {

		return Component->GetComponentLocation().Z + boxComp->GetScaledBoxExtent().Z;
	}

	else if (auto sphereComp = Cast<USphereComponent>(Component)) {

		return Component->GetComponentLocation().Z + sphereComp->GetScaledSphereRadius();
	}

	else if (auto staticMeshComp = Cast<UStaticMeshComponent>(Component)) {

		FBoxSphereBounds bounds;

		Component->GetBodySetup()->AggGeom.CalcBoxSphereBounds(bounds, Component->GetComponentTransform());

		return bounds.GetBox().GetCenter().Z + (bounds.GetBox().GetExtent().Z * Component->GetComponentTransform().GetScale3D().Z);
	}

	else if (auto skeletalMeshComp = Cast<USkeletalMeshComponent>(Component)) {

		FBoxSphereBounds bounds;

		// The Engines own CalcAABB has a requirement that the scale has to be uniform for some reason, so it doesn't work if for instance the characters scale is 1, 1, 0.5. So we only run it if uniform, otherwise we fall back to a custom one where i've copied over the exact same logic but commented out the uniform check, which works as intended despite the scale not being uniform..
		if (skeletalMeshComp->GetComponentTransform().GetScale3D().IsUniform()) {

			bounds = skeletalMeshComp->GetPhysicsAsset()->CalcAABB(skeletalMeshComp, skeletalMeshComp->GetComponentTransform());
		}
		else {

			bounds = UVertexPaintFunctionLibrary::CalcAABBWithoutUniformCheck(skeletalMeshComp, skeletalMeshComp->GetComponentTransform());
		}

		return bounds.GetBox().GetCenter().Z + (bounds.GetBox().GetExtent().Z * Component->GetComponentTransform().GetScale3D().Z);
	}

	return 0;
}


//--------------------------------------------------------

// Get Component Bounds Bottom World Z

float UVertexPaintFunctionLibrary::GetComponentBoundsBottomWorldZ(UPrimitiveComponent* Component) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetComponentBoundsBottomWorldZ);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - GetComponentBoundsBottomWorldZ");

	if (!IsValid(Component)) return 0;


	if (auto boxComp = Cast<UBoxComponent>(Component)) {

		return Component->GetComponentLocation().Z - boxComp->GetScaledBoxExtent().Z;
	}

	else if (auto sphereComp = Cast<USphereComponent>(Component)) {

		return Component->GetComponentLocation().Z - sphereComp->GetScaledSphereRadius();
	}

	else if (auto staticMeshComp = Cast<UStaticMeshComponent>(Component)) {

		FBoxSphereBounds bounds;

		Component->GetBodySetup()->AggGeom.CalcBoxSphereBounds(bounds, Component->GetComponentTransform());

		return bounds.GetBox().GetCenter().Z - (bounds.GetBox().GetExtent().Z * Component->GetComponentTransform().GetScale3D().Z);
	}

	else if (auto skeletalMeshComp = Cast<USkeletalMeshComponent>(Component)) {

		FBoxSphereBounds bounds;

		// The Engines own CalcAABB has a requirement that the scale has to be uniform for some reason, so it doesn't work if for instance the characters scale is 1, 1, 0.5. So we only run it if uniform, otherwise we fall back to a custom one where i've copied over the exact same logic but commented out the uniform check, which works as intended despite the scale not being uniform..
		if (skeletalMeshComp->GetComponentTransform().GetScale3D().IsUniform()) {

			bounds = skeletalMeshComp->GetPhysicsAsset()->CalcAABB(skeletalMeshComp, skeletalMeshComp->GetComponentTransform());
		}
		else {

			bounds = UVertexPaintFunctionLibrary::CalcAABBWithoutUniformCheck(skeletalMeshComp, skeletalMeshComp->GetComponentTransform());
		}

		return bounds.GetBox().GetCenter().Z - (bounds.GetBox().GetExtent().Z * Component->GetComponentTransform().GetScale3D().Z);
	}

	return 0;
}


//--------------------------------------------------------

// Register Paint Task Callbacks To Object From Specified Mesh Component

void UVertexPaintFunctionLibrary::RegisterPaintTaskCallbacksToObjectFromSpecifiedMeshComponent_Wrapper(UPrimitiveComponent* MeshComponent, UObject* ObjectToRegisterForCallbacks) {

	if (!MeshComponent) return;
	if (!ObjectToRegisterForCallbacks) return;


	if (UVertexPaintDetectionGISubSystem* gameInstanceSubsystem = GetVertexPaintGameInstanceSubsystem(MeshComponent))
		gameInstanceSubsystem->RegisterPaintTaskCallbacksToObjectFromSpecifiedMeshComponent(MeshComponent, ObjectToRegisterForCallbacks);
}


//--------------------------------------------------------

// UnRegister Paint Task Callbacks To Object From Specified Mesh Component

void UVertexPaintFunctionLibrary::UnRegisterPaintTaskCallbacksToObjectFromSpecifiedMeshComponent_Wrapper(UPrimitiveComponent* MeshComponent, UObject* objectToUnregisterForCallbacks) {

	if (!MeshComponent) return;
	if (!objectToUnregisterForCallbacks) return;


	if (UVertexPaintDetectionGISubSystem* gameInstanceSubsystem = GetVertexPaintGameInstanceSubsystem(MeshComponent)) {

		gameInstanceSubsystem->UnRegisterPaintTaskCallbacksToObjectFromSpecifiedMeshComponent(MeshComponent, objectToUnregisterForCallbacks);
	}
}


//--------------------------------------------------------

// Register Paint Task Callbacks To Object From Specified Mesh Component

void UVertexPaintFunctionLibrary::RegisterDetectTaskCallbacksToObjectFromSpecifiedMeshComponent_Wrapper(UPrimitiveComponent* MeshComponent, UObject* ObjectToRegisterForCallbacks) {

	if (!MeshComponent) return;
	if (!ObjectToRegisterForCallbacks) return;


	if (UVertexPaintDetectionGISubSystem* gameInstanceSubsystem = GetVertexPaintGameInstanceSubsystem(MeshComponent))
		gameInstanceSubsystem->RegisterDetectTaskCallbacksToObjectFromSpecifiedMeshComponent(MeshComponent, ObjectToRegisterForCallbacks);
}


//--------------------------------------------------------

// UnRegister Paint Task Callbacks To Object From Specified Mesh Component

void UVertexPaintFunctionLibrary::UnRegisterDetectTaskCallbacksToObjectFromSpecifiedMeshComponent_Wrapper(UPrimitiveComponent* MeshComponent, UObject* ObjectToUnregisterForCallbacks) {

	if (!MeshComponent) return;
	if (!ObjectToUnregisterForCallbacks) return;


	if (UVertexPaintDetectionGISubSystem* gameInstanceSubsystem = GetVertexPaintGameInstanceSubsystem(MeshComponent)) {

		gameInstanceSubsystem->UnRegisterDetectTaskCallbacksToObjectFromSpecifiedMeshComponent(MeshComponent, ObjectToUnregisterForCallbacks);
	}
}


//-------------------------------------------------------

// Should Task Loop Through All Vertices On LOD

bool UVertexPaintFunctionLibrary::ShouldTaskLoopThroughAllVerticesOnLOD(const FRVPDPTaskCallbackSettings& CallbackSettings, UPrimitiveComponent* Component, const FRVPDPOverrideColorsToApplySettings& OverrideColorsToApplySettings) {

	// If set to include IncludePhysicsSurfaceResultOfEachChannel, then we also have to loop through the vertices and sections to check the phys surface of each section
	return ((OverrideColorsToApplySettings.OverrideVertexColorsToApply && OverrideColorsToApplySettings.ObjectToRunOverrideVertexColorsInterface) || CallbackSettings.IncludeAmountOfPaintedColorsOfEachChannel.IncludeVertexColorChannelResultOfEachChannel || CallbackSettings.IncludeAmountOfPaintedColorsOfEachChannel.IncludePhysicsSurfaceResultOfEachChannel || CallbackSettings.CompareMeshVertexColorsToColorArray.ColorArrayToCompareWith.Num() > 0 || CallbackSettings.CompareMeshVertexColorsToColorSnippets.CompareWithColorsSnippetDataAssetInfo.Num() > 0 || CallbackSettings.IncludeSerializedVertexColorData || CallbackSettings.IncludeVertexPositionData || CallbackSettings.IncludeVertexNormalData || (CallbackSettings.IncludeColorsOfEachBone && Cast<USkeletalMeshComponent>(Component)));
}


//-------------------------------------------------------

// Get Vertex Color Physics Surface Data From Material

FRVPDPPhysicsSurfaceDataInfo UVertexPaintFunctionLibrary::GetVertexColorPhysicsSurfaceDataFromMaterial(const UObject* WorldContextObject, const FColor& VertexColor, UMaterialInterface* Material) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetPhysicsSurfaceData);
	RVPDP_CPUPROFILER_STR("Vertex Paint Game Instance Subsystem - GetPhysicsSurfaceData");


	if (!WorldContextObject) return FRVPDPPhysicsSurfaceDataInfo();

	UVertexPaintMaterialDataAsset* materialDataAsset = GetVertexPaintMaterialDataAsset(WorldContextObject);
	if (!materialDataAsset) return FRVPDPPhysicsSurfaceDataInfo();

	Material = materialDataAsset->GetRegisteredMaterialFromMaterialInterface(Material);
	if (!Material) return FRVPDPPhysicsSurfaceDataInfo();


	const FRVPDPRegisteredMaterialSetting materialDataInfo = materialDataAsset->GetVertexPaintMaterialInterface().FindRef(Material);
	FRVPDPPhysicsSurfaceDataInfo physicsSurfaceData;

	// Used to make sure that not only did we get the color data, but we could get the surfaces as well, i.e. the Material was registered in the data asset. 
	physicsSurfaceData.PhysicsSurfaceSuccessfullyAcquired = true;
	physicsSurfaceData.MaterialRegisteredToIncludeDefaultChannel = materialDataInfo.IncludeDefaultChannelWhenDetecting;

	// Converts closest Color from Byte to float
	physicsSurfaceData.PhysicsSurface_AtRed.PhysicalSurfaceValueAtChannel = static_cast<float>(VertexColor.R);
	physicsSurfaceData.PhysicsSurface_AtGreen.PhysicalSurfaceValueAtChannel = static_cast<float>(VertexColor.G);
	physicsSurfaceData.PhysicsSurface_AtBlue.PhysicalSurfaceValueAtChannel = static_cast<float>(VertexColor.B);
	physicsSurfaceData.PhysicsSurface_AtAlpha.PhysicalSurfaceValueAtChannel = static_cast<float>(VertexColor.A);


	// Clamps it from 0-255 to 0-1
	physicsSurfaceData.PhysicsSurface_AtRed.PhysicalSurfaceValueAtChannel = UKismetMathLibrary::MapRangeClamped(physicsSurfaceData.PhysicsSurface_AtRed.PhysicalSurfaceValueAtChannel, 0, 255, 0, 1);
	physicsSurfaceData.PhysicsSurface_AtGreen.PhysicalSurfaceValueAtChannel = UKismetMathLibrary::MapRangeClamped(physicsSurfaceData.PhysicsSurface_AtGreen.PhysicalSurfaceValueAtChannel, 0, 255, 0, 1);
	physicsSurfaceData.PhysicsSurface_AtBlue.PhysicalSurfaceValueAtChannel = UKismetMathLibrary::MapRangeClamped(physicsSurfaceData.PhysicsSurface_AtBlue.PhysicalSurfaceValueAtChannel, 0, 255, 0, 1);
	physicsSurfaceData.PhysicsSurface_AtAlpha.PhysicalSurfaceValueAtChannel = UKismetMathLibrary::MapRangeClamped(physicsSurfaceData.PhysicsSurface_AtAlpha.PhysicalSurfaceValueAtChannel, 0, 255, 0, 1);


	physicsSurfaceData.PhysicsSurface_AtDefault.PhysicalSurfaceAtChannel = materialDataInfo.AtDefault;
	physicsSurfaceData.PhysicsSurface_AtDefault.PhysicalSurfaceAsStringAtChannel = GetPhysicsSurfaceAsString(physicsSurfaceData.PhysicsSurface_AtDefault.PhysicalSurfaceAtChannel);
	physicsSurfaceData.PhysicsSurface_AtDefault.PhysicalSurfaceValueAtChannel = 1;


	// Sets EPhysicalSurface - Red
	physicsSurfaceData.PhysicsSurface_AtRed.PhysicalSurfaceAtChannel = materialDataInfo.PaintedAtRed;
	physicsSurfaceData.PhysicsSurface_AtRed.PhysicalSurfaceAsStringAtChannel = GetPhysicsSurfaceAsString(physicsSurfaceData.PhysicsSurface_AtRed.PhysicalSurfaceAtChannel); // Sets String that can be used with third party software like wwise and their RTCPs

	if (materialDataInfo.PaintedAtRed == EPhysicalSurface::SurfaceType_Default)
		physicsSurfaceData.PhysicsSurface_AtRed.PhysicalSurfaceValueAtChannel = 0;


	// Green
	physicsSurfaceData.PhysicsSurface_AtGreen.PhysicalSurfaceAtChannel = materialDataInfo.PaintedAtGreen;
	physicsSurfaceData.PhysicsSurface_AtGreen.PhysicalSurfaceAsStringAtChannel = GetPhysicsSurfaceAsString(physicsSurfaceData.PhysicsSurface_AtGreen.PhysicalSurfaceAtChannel);

	if (physicsSurfaceData.PhysicsSurface_AtGreen.PhysicalSurfaceAtChannel == EPhysicalSurface::SurfaceType_Default)
		physicsSurfaceData.PhysicsSurface_AtGreen.PhysicalSurfaceValueAtChannel = 0;


	// Blue
	physicsSurfaceData.PhysicsSurface_AtBlue.PhysicalSurfaceAtChannel = materialDataInfo.PaintedAtBlue;
	physicsSurfaceData.PhysicsSurface_AtBlue.PhysicalSurfaceAsStringAtChannel = GetPhysicsSurfaceAsString(physicsSurfaceData.PhysicsSurface_AtBlue.PhysicalSurfaceAtChannel);

	if (physicsSurfaceData.PhysicsSurface_AtBlue.PhysicalSurfaceAtChannel == EPhysicalSurface::SurfaceType_Default)
		physicsSurfaceData.PhysicsSurface_AtBlue.PhysicalSurfaceValueAtChannel = 0;


	// Alpha
	physicsSurfaceData.PhysicsSurface_AtAlpha.PhysicalSurfaceAtChannel = materialDataInfo.PaintedAtAlpha;
	physicsSurfaceData.PhysicsSurface_AtAlpha.PhysicalSurfaceAsStringAtChannel = GetPhysicsSurfaceAsString(physicsSurfaceData.PhysicsSurface_AtAlpha.PhysicalSurfaceAtChannel);

	if (physicsSurfaceData.PhysicsSurface_AtAlpha.PhysicalSurfaceAtChannel == EPhysicalSurface::SurfaceType_Default)
		physicsSurfaceData.PhysicsSurface_AtAlpha.PhysicalSurfaceValueAtChannel = 0;


	// Fills the closest arrays which can be used to more comfortable loop through them without having to make arrays yourself. Note how even if a surface may be default it always get added to the array so it's always 5 in size if we've been able to get any close colors
	physicsSurfaceData.PhysicalSurfacesAsArray.Add(physicsSurfaceData.PhysicsSurface_AtDefault.PhysicalSurfaceAtChannel);
	physicsSurfaceData.PhysicalSurfacesAsArray.Add(physicsSurfaceData.PhysicsSurface_AtRed.PhysicalSurfaceAtChannel);
	physicsSurfaceData.PhysicalSurfacesAsArray.Add(physicsSurfaceData.PhysicsSurface_AtGreen.PhysicalSurfaceAtChannel);
	physicsSurfaceData.PhysicalSurfacesAsArray.Add(physicsSurfaceData.PhysicsSurface_AtBlue.PhysicalSurfaceAtChannel);
	physicsSurfaceData.PhysicalSurfacesAsArray.Add(physicsSurfaceData.PhysicsSurface_AtAlpha.PhysicalSurfaceAtChannel);

	physicsSurfaceData.SurfacesAsStringArray.Add(physicsSurfaceData.PhysicsSurface_AtDefault.PhysicalSurfaceAsStringAtChannel);
	physicsSurfaceData.SurfacesAsStringArray.Add(physicsSurfaceData.PhysicsSurface_AtRed.PhysicalSurfaceAsStringAtChannel);
	physicsSurfaceData.SurfacesAsStringArray.Add(physicsSurfaceData.PhysicsSurface_AtGreen.PhysicalSurfaceAsStringAtChannel);
	physicsSurfaceData.SurfacesAsStringArray.Add(physicsSurfaceData.PhysicsSurface_AtBlue.PhysicalSurfaceAsStringAtChannel);
	physicsSurfaceData.SurfacesAsStringArray.Add(physicsSurfaceData.PhysicsSurface_AtAlpha.PhysicalSurfaceAsStringAtChannel);

	physicsSurfaceData.SurfaceValuesArray.Add(physicsSurfaceData.PhysicsSurface_AtDefault.PhysicalSurfaceValueAtChannel);
	physicsSurfaceData.SurfaceValuesArray.Add(physicsSurfaceData.PhysicsSurface_AtRed.PhysicalSurfaceValueAtChannel);
	physicsSurfaceData.SurfaceValuesArray.Add(physicsSurfaceData.PhysicsSurface_AtGreen.PhysicalSurfaceValueAtChannel);
	physicsSurfaceData.SurfaceValuesArray.Add(physicsSurfaceData.PhysicsSurface_AtBlue.PhysicalSurfaceValueAtChannel);
	physicsSurfaceData.SurfaceValuesArray.Add(physicsSurfaceData.PhysicsSurface_AtAlpha.PhysicalSurfaceValueAtChannel);




	// If anything is painted then gets the most dominant one out of RGBA, otherwise just sets the default as the dominant one
	if (physicsSurfaceData.PhysicsSurface_AtRed.PhysicalSurfaceValueAtChannel > 0 || physicsSurfaceData.PhysicsSurface_AtGreen.PhysicalSurfaceValueAtChannel > 0 || physicsSurfaceData.PhysicsSurface_AtBlue.PhysicalSurfaceValueAtChannel > 0 || physicsSurfaceData.PhysicsSurface_AtAlpha.PhysicalSurfaceValueAtChannel > 0) {

		TArray<TEnumAsByte<EPhysicalSurface>> physiccalSurfacesToCheck = physicsSurfaceData.PhysicalSurfacesAsArray;
		TArray<float> physicalSurfaceValuesToCheck = physicsSurfaceData.SurfaceValuesArray;

		// Removes the Default if set Not to include it, so we only check RGBA
		if (!materialDataInfo.IncludeDefaultChannelWhenDetecting) {

			physiccalSurfacesToCheck.RemoveAt(0);
			physicalSurfaceValuesToCheck.RemoveAt(0);
		}

		TEnumAsByte<EPhysicalSurface> mostDominantSurface;
		float mostDominantSurfaceValue = 0;
		if (GetTheMostDominantPhysicsSurface_Wrapper(WorldContextObject, Material, physiccalSurfacesToCheck, physicalSurfaceValuesToCheck, mostDominantSurface, mostDominantSurfaceValue)) {

			physicsSurfaceData.MostDominantPhysicsSurfaceInfo.MostDominantPhysicsSurface = mostDominantSurface;
			physicsSurfaceData.MostDominantPhysicsSurfaceInfo.MostDominantPhysicsSurfaceAsString = GetPhysicsSurfaceAsString(mostDominantSurface);
			physicsSurfaceData.MostDominantPhysicsSurfaceInfo.MostDominantPhysicstSurfaceValue = mostDominantSurfaceValue;
		}
	}

	else {

		physicsSurfaceData.MostDominantPhysicsSurfaceInfo.MostDominantPhysicsSurface = physicsSurfaceData.PhysicsSurface_AtDefault.PhysicalSurfaceAtChannel;
		physicsSurfaceData.MostDominantPhysicsSurfaceInfo.MostDominantPhysicsSurfaceAsString = GetPhysicsSurfaceAsString(physicsSurfaceData.PhysicsSurface_AtDefault.PhysicalSurfaceAtChannel);
		physicsSurfaceData.MostDominantPhysicsSurfaceInfo.MostDominantPhysicstSurfaceValue = physicsSurfaceData.PhysicsSurface_AtDefault.PhysicalSurfaceValueAtChannel;
	}


	bool successfullyGotColorChannels = false;
	TArray<ESurfaceAtChannel> surfaceAtChannels = GetAllVertexColorChannelsPhysicsSurfaceIsRegisteredTo_Wrapper(WorldContextObject, Material, physicsSurfaceData.MostDominantPhysicsSurfaceInfo.MostDominantPhysicsSurface, successfullyGotColorChannels);

	if (successfullyGotColorChannels) {

		physicsSurfaceData.MostDominantPhysicsSurfaceInfo.MostDominantPhysicsSurfaceAtVertexColorChannels = surfaceAtChannels;
	}


	// Returns Struct Updated with Names of channels from Data Asset
	return physicsSurfaceData;
}


//-------------------------------------------------------

// Multi Cone Trace For Objects Using Cone Mesh

bool UVertexPaintFunctionLibrary::MultiConeTraceForObjectsUsingConeMesh(TArray<FHitResult>& OutHits, UPrimitiveComponent* ConeMesh, const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes, const TArray<AActor*>& ActorsToIgnore, bool DebugTrace, float DebugTraceDuration) {

	OutHits.Empty();

	if (!IsValid(ConeMesh)) return false;


	// Gets Cone Shape Info
	FTransform transformWithoutRotation;
	transformWithoutRotation.SetLocation(ConeMesh->GetComponentLocation());
	transformWithoutRotation.SetScale3D(FVector(1, 1, 1));

	FBoxSphereBounds cleanAggGeomBounds = FBoxSphereBounds();
	ConeMesh->GetBodySetup()->AggGeom.CalcBoxSphereBounds(cleanAggGeomBounds, transformWithoutRotation);
	const auto cleanBoundsBox = cleanAggGeomBounds.GetBox();
	const FVector componentScale = ConeMesh->GetComponentScale();

	const FVector coneOrigin = ConeMesh->GetComponentLocation();
	const FVector coneDirection = ConeMesh->GetUpVector() * -1;

	const float coneRadius = FMath::Max(cleanBoundsBox.GetExtent().Y, cleanBoundsBox.GetExtent().X) * componentScale.X;
	const float coneHeight = (cleanBoundsBox.GetExtent().Z * 2.0f) * componentScale.Z;

	return MultiConeTraceForObjects(OutHits, ConeMesh, coneOrigin, coneDirection, coneHeight, coneRadius, ObjectTypes, ActorsToIgnore, DebugTrace, DebugTraceDuration);
}


//-------------------------------------------------------

// Multi Cone Trace For Objects

bool UVertexPaintFunctionLibrary::MultiConeTraceForObjects(TArray<FHitResult>& OutHits, const UObject* WorldContextObject, const FVector& ConeOrigin, const FVector& ConeDirection, float ConeHeight, float ConeRadius, const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes, const TArray<AActor*>& ActorsToIgnore, bool DebugTrace, float DebugTraceDuration) {

	OutHits.Empty();

	if (!WorldContextObject) return false;

	UWorld* world = WorldContextObject->GetWorld();
	if (!IsWorldValid(world)) return false;


	const float coneSlantHeight = FMath::Sqrt(FMath::Square(ConeRadius) + FMath::Square(ConeHeight)); // Height to the bottom edges of the cone
	const float coneAngleInDegrees = FMath::RadiansToDegrees(FMath::Atan(ConeRadius / ConeHeight));


	// First does Sphere Trace for all Hits within the same cone height. 
	TArray<FHitResult> sphereHitResults;
	UKismetSystemLibrary::SphereTraceMultiForObjects(world, ConeOrigin, ConeOrigin * 1.0001f, coneSlantHeight, ObjectTypes,false, ActorsToIgnore, EDrawDebugTrace::None,sphereHitResults,true, FLinearColor::Red, FLinearColor::Green, DebugTraceDuration);


	if (DebugTrace) {

		const float coneAngleRad = FMath::DegreesToRadians(coneAngleInDegrees);

		DrawDebugCone(world, ConeOrigin, ConeDirection, coneSlantHeight, coneAngleRad, coneAngleRad, 12, FColor::Red, false, DebugTraceDuration, 0, 2);
	}


	// Then we filter the hits based on the cone's angle and distance, same as we do when we check if vertex is within cone if running Paint Within Area with Cone Shape
	for (const FHitResult& hitResult : sphereHitResults) {

		if (!hitResult.bBlockingHit) continue;


		bool hitAdded = false;
		USkeletalMeshComponent* skeletalMeshComponent = Cast<USkeletalMeshComponent>(hitResult.GetComponent());


		// If skel mesh then checks the bones bounds and each corner of it. This fixed the issue where if the sphere trace got a hit one a bone, where the bone was reached into the cone angle, but the hit location was just outside of the cones angle so the bone wasn't included
		if (skeletalMeshComponent && skeletalMeshComponent->GetPhysicsAsset()) {


			const FName boneName = hitResult.BoneName;
			UPhysicsAsset* physicsAsset = skeletalMeshComponent->GetPhysicsAsset();

			const int32 bodyIndex = physicsAsset->FindBodyIndex(boneName);
			if (bodyIndex == INDEX_NONE) continue;


			FKAggregateGeom aggGeom;

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5

			if (USkeletalBodySetup* skeletalBodySetup = physicsAsset->SkeletalBodySetups[bodyIndex].Get())
				aggGeom = skeletalBodySetup->AggGeom;
			else
				continue;

#else

			if (UBodySetup* bodySetup = physicsAsset->SkeletalBodySetups[bodyIndex])
				aggGeom = bodySetup->AggGeom;
			else
				continue;
#endif

			
			const int32 boneIndex = skeletalMeshComponent->GetBoneIndex(boneName);
			const FTransform boneTransform = skeletalMeshComponent->GetBoneTransform(boneIndex);


			// If capsule physics asset collision then check if it's bottom, center or top is within the angle / height
			if (aggGeom.SphylElems.Num() > 0) {

				const FKSphylElem& capsuleElem = aggGeom.SphylElems[0];
				const FVector capsuleCenter = boneTransform.TransformPosition(capsuleElem.Center);
				const FQuat capsuleRotation = boneTransform.GetRotation() * capsuleElem.Rotation.Quaternion();
				const float capsuleRadius = capsuleElem.Radius;
				const float capsuleHalfHeight = capsuleElem.Length;

				if (DebugTrace) {

					DrawDebugCapsule(world, capsuleCenter, capsuleHalfHeight, capsuleRadius, capsuleRotation, FColor::Blue, false, DebugTraceDuration);
				}


				const FVector capsuleTop = capsuleCenter + capsuleRotation.GetUpVector() * capsuleHalfHeight;
				const FVector capsuleBottom = capsuleCenter - capsuleRotation.GetUpVector() * capsuleHalfHeight;
				const TArray<FVector> capsulePointsToCheck = { capsuleTop, capsuleCenter, capsuleBottom };


				// If any point is within angle and height
				for (const FVector& capsulePoint : capsulePointsToCheck) {

					const FVector directionToCapsulePoint = (capsulePoint - ConeOrigin).GetSafeNormal();
					const float angleToCapsulePointDegrees = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(ConeDirection, directionToCapsulePoint)));
					const float distanceToCapsulePoint = (capsulePoint - ConeOrigin).Size();

					if (angleToCapsulePointDegrees <= coneAngleInDegrees && distanceToCapsulePoint <= coneSlantHeight) {

						OutHits.Add(hitResult);
						break;
					}
				}

				continue;
			}

			// If box physics asset collision then checks if any of its corners is within the angle and height
			else if (aggGeom.BoxElems.Num() > 0) {


				const FKBoxElem& boxElem = aggGeom.BoxElems[0];
				const FTransform boxLocalTransform(boxElem.Rotation.Quaternion(), boxElem.Center, FVector(boxElem.X * 0.5f, boxElem.Y * 0.5f, boxElem.Z * 0.5f));
				const FTransform boxWorldTransform = boxLocalTransform * boneTransform;
				const FVector boxExtent(boxElem.X * 0.5f, boxElem.Y * 0.5f, boxElem.Z * 0.5f);


				if (DebugTrace) {

					DrawDebugBox(world, boxWorldTransform.GetLocation(), boxExtent, boxWorldTransform.GetRotation(), FColor::Green, false, DebugTraceDuration);
				}


				TArray<FVector> boxCorners;
				boxCorners.Add(boxWorldTransform.TransformPosition(FVector(-boxExtent.X, -boxExtent.Y, -boxExtent.Z)));
				boxCorners.Add(boxWorldTransform.TransformPosition(FVector(-boxExtent.X, -boxExtent.Y, boxExtent.Z)));
				boxCorners.Add(boxWorldTransform.TransformPosition(FVector(-boxExtent.X, boxExtent.Y, -boxExtent.Z)));
				boxCorners.Add(boxWorldTransform.TransformPosition(FVector(-boxExtent.X, boxExtent.Y, boxExtent.Z)));
				boxCorners.Add(boxWorldTransform.TransformPosition(FVector(boxExtent.X, -boxExtent.Y, -boxExtent.Z)));
				boxCorners.Add(boxWorldTransform.TransformPosition(FVector(boxExtent.X, -boxExtent.Y, boxExtent.Z)));
				boxCorners.Add(boxWorldTransform.TransformPosition(FVector(boxExtent.X, boxExtent.Y, -boxExtent.Z)));
				boxCorners.Add(boxWorldTransform.TransformPosition(FVector(boxExtent.X, boxExtent.Y, boxExtent.Z)));


				for (const FVector& corner : boxCorners) {

					FVector directionToCorner = (corner - ConeOrigin).GetSafeNormal();
					float angleToCornerDegrees = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(ConeDirection, directionToCorner)));
					float distanceToCorner = (corner - ConeOrigin).Size();

					if (angleToCornerDegrees <= coneAngleInDegrees && distanceToCorner <= coneSlantHeight) {

						OutHits.Add(hitResult);
						break;
					}
				}

				continue;
			}
		}


		// If not a Skeletal Mesh with Box/Capsule then just does the simple check. 

		// Angle Check
		const FVector directionToHitLocation = hitResult.ImpactPoint - ConeOrigin;
		const float distanceToHitLocation = directionToHitLocation.Size();
		const float dotToHitLocation = FVector::DotProduct(ConeDirection, directionToHitLocation.GetSafeNormal());
		const float angleToHitLocation = FMath::Acos(dotToHitLocation);
		const float angleToHitLocationDegrees = FMath::RadiansToDegrees(angleToHitLocation);

		if (angleToHitLocationDegrees > coneAngleInDegrees) continue;


		// Height Check
		const float heightToCheck = UKismetMathLibrary::MapRangeClamped(angleToHitLocationDegrees, 0, coneAngleInDegrees, ConeHeight, coneSlantHeight);

		if (distanceToHitLocation < 0 || distanceToHitLocation > heightToCheck) continue;


		OutHits.Add(hitResult);
	}


	if (OutHits.Num() > 0)
		return true;


	return false;
}


//-------------------------------------------------------

// Get Mesh Required Callback Settings

bool UVertexPaintFunctionLibrary::GetMeshRequiredCallbackSettings(UPrimitiveComponent* MeshComponent, bool& MeshPaintedByAutoPaintComponent, FRVPDPTaskCallbackSettings& RequiredCallbackSettings) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetMeshRequiredCallbackSettings);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - GetMeshRequiredCallbackSettings");

	MeshPaintedByAutoPaintComponent = false;
	RequiredCallbackSettings = FRVPDPTaskCallbackSettings();
	
	if (!IsValid(MeshComponent)) return false;
	if (!IsValid(MeshComponent->GetOwner())) return false;


	// Checks if either actor or component implements the interface and if so gets the callback settings
	if (MeshComponent->GetOwner()->GetClass()->ImplementsInterface(UVertexPaintDetectionInterface::StaticClass())) {

		RequiredCallbackSettings = IVertexPaintDetectionInterface::Execute_RequiredCallbackSettings(MeshComponent->GetOwner(), MeshComponent);
	}

	else if (MeshComponent->GetClass()->ImplementsInterface(UVertexPaintDetectionInterface::StaticClass())) {

		RequiredCallbackSettings = IVertexPaintDetectionInterface::Execute_RequiredCallbackSettings(MeshComponent, MeshComponent);
	}


	if (!MeshPaintedByAutoPaintComponent) {

		// Checks if any Auto Paint Components on the Owner needs the Amount of Colors of Each Channel
		TArray<UActorComponent*> autoPaintComponents;
		MeshComponent->GetOwner()->GetComponents(UAutoAddColorComponent::StaticClass(), autoPaintComponents);
		for (UActorComponent* autoPaintComponent : autoPaintComponents) {

			if (UAutoAddColorComponent* autoPaintColorComponent = Cast<UAutoAddColorComponent>(autoPaintComponent)) {

				if (autoPaintColorComponent->IsMeshBeingAutoPainted(MeshComponent)) {

					// Makes sure we include both phys surface and RGBA just in case, with default min color amount.
					RequiredCallbackSettings.IncludeAmountOfPaintedColorsOfEachChannel.IncludeIfMinColorAmountIs = FRVPDPIncludeAmountOfPaintedColorsOfEachChannelSettings().IncludeIfMinColorAmountIs;
					RequiredCallbackSettings.IncludeAmountOfPaintedColorsOfEachChannel.IncludeVertexColorChannelResultOfEachChannel = true;
					RequiredCallbackSettings.IncludeAmountOfPaintedColorsOfEachChannel.IncludePhysicsSurfaceResultOfEachChannel = true;

					MeshPaintedByAutoPaintComponent = true;
					break;
				}
			}
		}
	}



	// Checks if something is registered to receive callbacks when a task is finished on the mesh
	FRVPDPCallbackFromSpecifiedMeshComponentsInfo callbacksFromSpecifiedMesh = GetRegisteredPaintTaskCallbacksObjectsForSpecificMeshComponent(MeshComponent, MeshComponent);
	if (callbacksFromSpecifiedMesh.RunCallbacksOnObjects.Num() > 0) {

		for (auto runCallbacksOnObjects : callbacksFromSpecifiedMesh.RunCallbacksOnObjects) {

			if (!runCallbacksOnObjects.IsValid()) continue;

			UObject* object = runCallbacksOnObjects.Get();

			if (!IsValid(object)) continue;


			FRVPDPTaskCallbackSettings callbackSettings;
			if (object->GetClass()->ImplementsInterface(UVertexPaintDetectionInterface::StaticClass()))
				callbackSettings = IVertexPaintDetectionInterface::Execute_RequiredCallbackSettings(object, MeshComponent);


			// Can append to default settings, so we don't override something that got set to true earlier here and reset it back to default
			if (callbackSettings != FRVPDPTaskCallbackSettings())
				RequiredCallbackSettings.AppendCallbackSettingsToDefaults(callbackSettings);


			// Also checks if the objects listening for callbacks has any auto paint components, and if the mesh is being auto painted by them. Can be a usecase if you have Managers like Weather Systems. 
			if (AActor* listenerActor = Cast<AActor>(object)) {

				if (MeshPaintedByAutoPaintComponent) continue;

				TArray<UActorComponent*> listenersAutoPaintComponents;
				listenerActor->GetComponents(UAutoAddColorComponent::StaticClass(), listenersAutoPaintComponents);
				for (UActorComponent* listenerAutoPaintComponent : listenersAutoPaintComponents) {

					if (UAutoAddColorComponent* listenerAutoPaintColorComponent = Cast<UAutoAddColorComponent>(listenerAutoPaintComponent)) {

						if (!listenerAutoPaintColorComponent->IsMeshBeingAutoPainted(MeshComponent)) continue;

						// Makes sure we include both phys surface and RGBA just in case, with default min color amount.
						RequiredCallbackSettings.IncludeAmountOfPaintedColorsOfEachChannel.IncludeIfMinColorAmountIs = FRVPDPIncludeAmountOfPaintedColorsOfEachChannelSettings().IncludeIfMinColorAmountIs;
						RequiredCallbackSettings.IncludeAmountOfPaintedColorsOfEachChannel.IncludeVertexColorChannelResultOfEachChannel = true;
						RequiredCallbackSettings.IncludeAmountOfPaintedColorsOfEachChannel.IncludePhysicsSurfaceResultOfEachChannel = true;

						MeshPaintedByAutoPaintComponent = true;
						break;
					}
				}
			}
		}
	}

	callbacksFromSpecifiedMesh = GetRegisteredDetectTaskCallbacksObjectsForSpecificMeshComponent(MeshComponent, MeshComponent);
	if (callbacksFromSpecifiedMesh.RunCallbacksOnObjects.Num() > 0) {

		for (auto runCallbacksOnObjects : callbacksFromSpecifiedMesh.RunCallbacksOnObjects) {

			if (!runCallbacksOnObjects.IsValid()) continue;

			UObject* object = runCallbacksOnObjects.Get();

			if (!IsValid(object)) continue;


			FRVPDPTaskCallbackSettings callbackSettings;
			if (object->GetClass()->ImplementsInterface(UVertexPaintDetectionInterface::StaticClass()))
				callbackSettings = IVertexPaintDetectionInterface::Execute_RequiredCallbackSettings(object, MeshComponent);


			if (callbackSettings != FRVPDPTaskCallbackSettings())
				RequiredCallbackSettings.AppendCallbackSettingsToDefaults(callbackSettings);


			if (AActor* listenerActor = Cast<AActor>(object)) {

				if (MeshPaintedByAutoPaintComponent) continue;

				TArray<UActorComponent*> listenersAutoPaintComponents;
				listenerActor->GetComponents(UAutoAddColorComponent::StaticClass(), listenersAutoPaintComponents);
				for (UActorComponent* listenerAutoPaintComponent : listenersAutoPaintComponents) {

					if (UAutoAddColorComponent* listenerAutoPaintColorComponent = Cast<UAutoAddColorComponent>(listenerAutoPaintComponent)) {

						if (!listenerAutoPaintColorComponent->IsMeshBeingAutoPainted(MeshComponent)) continue;

						// Makes sure we include both phys surface and RGBA just in case, with default min color amount.
						RequiredCallbackSettings.IncludeAmountOfPaintedColorsOfEachChannel.IncludeIfMinColorAmountIs = FRVPDPIncludeAmountOfPaintedColorsOfEachChannelSettings().IncludeIfMinColorAmountIs;
						RequiredCallbackSettings.IncludeAmountOfPaintedColorsOfEachChannel.IncludeVertexColorChannelResultOfEachChannel = true;
						RequiredCallbackSettings.IncludeAmountOfPaintedColorsOfEachChannel.IncludePhysicsSurfaceResultOfEachChannel = true;

						MeshPaintedByAutoPaintComponent = true;
						break;
					}
				}
			}
		}
	}


	// True if it got something other than the Default
	return (RequiredCallbackSettings != FRVPDPTaskCallbackSettings());
}


//-------------------------------------------------------

// Is There Any Paint Conditions

bool UVertexPaintFunctionLibrary::IsThereAnyPaintConditions(const FRVPDPApplyColorSettings& ApplyColorSettings, bool& IsOnPhysicsSurface, bool& IsOnRedChannel, bool& IsOnGreenChannel, bool& IsOnBlueChannel, bool& IsOnAlphaChannel, int& AmountOfVertexNormalWithinDotToDirectionConditions, int& AmountOfOnlyAffectVerticesWithDirectionToActorOrLocationConditions, int& AmountOfOnlyAffectVerticesWithinDirectionFromActorOrLocation, int& AmountOfIfVertexIsAboveOrBelowWorldZ, int& AmountOfIfVertexColorWithinRange, int& AmountOfIfVertexHasLineOfSightTo, int& AmountOfIfVertexIsOnBone, int& AmountOfIfVertexIsOnMaterial) {


	IsOnPhysicsSurface = false;
	IsOnRedChannel = false;
	IsOnGreenChannel = false;
	IsOnBlueChannel = false;
	IsOnAlphaChannel = false;

	AmountOfVertexNormalWithinDotToDirectionConditions = 0;
	AmountOfOnlyAffectVerticesWithDirectionToActorOrLocationConditions = 0;
	AmountOfOnlyAffectVerticesWithinDirectionFromActorOrLocation = 0;
	AmountOfIfVertexIsAboveOrBelowWorldZ = 0;
	AmountOfIfVertexColorWithinRange = 0;
	AmountOfIfVertexHasLineOfSightTo = 0;
	AmountOfIfVertexIsOnBone = 0;
	AmountOfIfVertexIsOnMaterial = 0;


	if (ApplyColorSettings.ApplyColorsUsingPhysicsSurface.ApplyVertexColorUsingPhysicsSurface) {

		for (const FRVPDPPhysicsSurfaceToApplySettings& physicalSurfaceToApply : ApplyColorSettings.ApplyColorsUsingPhysicsSurface.PhysicalSurfacesToApply) {


			AmountOfVertexNormalWithinDotToDirectionConditions = physicalSurfaceToApply.PaintConditions.IsVertexNormalWithinDotToDirection.Num();
			AmountOfOnlyAffectVerticesWithDirectionToActorOrLocationConditions = physicalSurfaceToApply.PaintConditions.OnlyAffectVerticesWithDirectionToActorOrLocation.Num();
			AmountOfOnlyAffectVerticesWithinDirectionFromActorOrLocation = physicalSurfaceToApply.PaintConditions.OnlyAffectVerticesWithinDirectionFromActorOrLocation.Num();
			AmountOfIfVertexIsAboveOrBelowWorldZ = physicalSurfaceToApply.PaintConditions.IfVertexIsAboveOrBelowWorldZ.Num();
			AmountOfIfVertexColorWithinRange = physicalSurfaceToApply.PaintConditions.IfVertexColorIsWithinRange.Num();
			AmountOfIfVertexHasLineOfSightTo = physicalSurfaceToApply.PaintConditions.IfVertexHasLineOfSightTo.Num();
			AmountOfIfVertexIsOnBone = physicalSurfaceToApply.PaintConditions.IfVertexIsOnBone.Num();
			AmountOfIfVertexIsOnMaterial = physicalSurfaceToApply.PaintConditions.IfVertexIsOnMaterial.Num();

			if (AmountOfVertexNormalWithinDotToDirectionConditions > 0 || AmountOfOnlyAffectVerticesWithDirectionToActorOrLocationConditions > 0 || AmountOfOnlyAffectVerticesWithinDirectionFromActorOrLocation > 0 || AmountOfIfVertexIsAboveOrBelowWorldZ > 0 || AmountOfIfVertexColorWithinRange > 0 || AmountOfIfVertexHasLineOfSightTo > 0 || AmountOfIfVertexIsOnBone > 0 || AmountOfIfVertexIsOnMaterial > 0) {

				IsOnPhysicsSurface = true;
			}
		}
	}

	else {


		AmountOfVertexNormalWithinDotToDirectionConditions += ApplyColorSettings.ApplyColorsOnRedChannel.PaintConditions.IsVertexNormalWithinDotToDirection.Num();
		AmountOfOnlyAffectVerticesWithDirectionToActorOrLocationConditions += ApplyColorSettings.ApplyColorsOnRedChannel.PaintConditions.OnlyAffectVerticesWithDirectionToActorOrLocation.Num();
		AmountOfOnlyAffectVerticesWithinDirectionFromActorOrLocation += ApplyColorSettings.ApplyColorsOnRedChannel.PaintConditions.OnlyAffectVerticesWithinDirectionFromActorOrLocation.Num();
		AmountOfIfVertexIsAboveOrBelowWorldZ += ApplyColorSettings.ApplyColorsOnRedChannel.PaintConditions.IfVertexIsAboveOrBelowWorldZ.Num();
		AmountOfIfVertexColorWithinRange += ApplyColorSettings.ApplyColorsOnRedChannel.PaintConditions.IfVertexColorIsWithinRange.Num();
		AmountOfIfVertexHasLineOfSightTo += ApplyColorSettings.ApplyColorsOnRedChannel.PaintConditions.IfVertexHasLineOfSightTo.Num();
		AmountOfIfVertexIsOnBone += ApplyColorSettings.ApplyColorsOnRedChannel.PaintConditions.IfVertexIsOnBone.Num();
		AmountOfIfVertexIsOnMaterial += ApplyColorSettings.ApplyColorsOnRedChannel.PaintConditions.IfVertexIsOnMaterial.Num();

		if (AmountOfVertexNormalWithinDotToDirectionConditions > 0 || AmountOfOnlyAffectVerticesWithDirectionToActorOrLocationConditions > 0 || AmountOfOnlyAffectVerticesWithinDirectionFromActorOrLocation > 0 || AmountOfIfVertexIsAboveOrBelowWorldZ > 0 || AmountOfIfVertexColorWithinRange > 0 || AmountOfIfVertexHasLineOfSightTo > 0 || AmountOfIfVertexIsOnBone > 0 || AmountOfIfVertexIsOnMaterial > 0) {

			IsOnRedChannel = true;
		}


		AmountOfVertexNormalWithinDotToDirectionConditions += ApplyColorSettings.ApplyColorsOnGreenChannel.PaintConditions.IsVertexNormalWithinDotToDirection.Num();
		AmountOfOnlyAffectVerticesWithDirectionToActorOrLocationConditions += ApplyColorSettings.ApplyColorsOnGreenChannel.PaintConditions.OnlyAffectVerticesWithDirectionToActorOrLocation.Num();
		AmountOfOnlyAffectVerticesWithinDirectionFromActorOrLocation += ApplyColorSettings.ApplyColorsOnGreenChannel.PaintConditions.OnlyAffectVerticesWithinDirectionFromActorOrLocation.Num();
		AmountOfIfVertexIsAboveOrBelowWorldZ += ApplyColorSettings.ApplyColorsOnGreenChannel.PaintConditions.IfVertexIsAboveOrBelowWorldZ.Num();
		AmountOfIfVertexColorWithinRange += ApplyColorSettings.ApplyColorsOnGreenChannel.PaintConditions.IfVertexColorIsWithinRange.Num();
		AmountOfIfVertexHasLineOfSightTo += ApplyColorSettings.ApplyColorsOnGreenChannel.PaintConditions.IfVertexHasLineOfSightTo.Num();
		AmountOfIfVertexIsOnBone += ApplyColorSettings.ApplyColorsOnGreenChannel.PaintConditions.IfVertexIsOnBone.Num();
		AmountOfIfVertexIsOnMaterial += ApplyColorSettings.ApplyColorsOnGreenChannel.PaintConditions.IfVertexIsOnMaterial.Num();

		if (AmountOfVertexNormalWithinDotToDirectionConditions > 0 || AmountOfOnlyAffectVerticesWithDirectionToActorOrLocationConditions > 0 || AmountOfOnlyAffectVerticesWithinDirectionFromActorOrLocation > 0 || AmountOfIfVertexIsAboveOrBelowWorldZ > 0 || AmountOfIfVertexColorWithinRange > 0 || AmountOfIfVertexHasLineOfSightTo > 0 || AmountOfIfVertexIsOnBone > 0 || AmountOfIfVertexIsOnMaterial > 0) {

			IsOnGreenChannel = true;
		}


		AmountOfVertexNormalWithinDotToDirectionConditions += ApplyColorSettings.ApplyColorsOnBlueChannel.PaintConditions.IsVertexNormalWithinDotToDirection.Num();
		AmountOfOnlyAffectVerticesWithDirectionToActorOrLocationConditions += ApplyColorSettings.ApplyColorsOnBlueChannel.PaintConditions.OnlyAffectVerticesWithDirectionToActorOrLocation.Num();
		AmountOfOnlyAffectVerticesWithinDirectionFromActorOrLocation += ApplyColorSettings.ApplyColorsOnBlueChannel.PaintConditions.OnlyAffectVerticesWithinDirectionFromActorOrLocation.Num();
		AmountOfIfVertexIsAboveOrBelowWorldZ += ApplyColorSettings.ApplyColorsOnBlueChannel.PaintConditions.IfVertexIsAboveOrBelowWorldZ.Num();
		AmountOfIfVertexColorWithinRange += ApplyColorSettings.ApplyColorsOnBlueChannel.PaintConditions.IfVertexColorIsWithinRange.Num();
		AmountOfIfVertexHasLineOfSightTo += ApplyColorSettings.ApplyColorsOnBlueChannel.PaintConditions.IfVertexHasLineOfSightTo.Num();
		AmountOfIfVertexIsOnBone += ApplyColorSettings.ApplyColorsOnBlueChannel.PaintConditions.IfVertexIsOnBone.Num();
		AmountOfIfVertexIsOnMaterial += ApplyColorSettings.ApplyColorsOnBlueChannel.PaintConditions.IfVertexIsOnMaterial.Num();

		if (AmountOfVertexNormalWithinDotToDirectionConditions > 0 || AmountOfOnlyAffectVerticesWithDirectionToActorOrLocationConditions > 0 || AmountOfOnlyAffectVerticesWithinDirectionFromActorOrLocation > 0 || AmountOfIfVertexIsAboveOrBelowWorldZ > 0 || AmountOfIfVertexColorWithinRange > 0 || AmountOfIfVertexHasLineOfSightTo > 0 || AmountOfIfVertexIsOnBone > 0 || AmountOfIfVertexIsOnMaterial > 0) {

			IsOnBlueChannel = true;
		}


		AmountOfVertexNormalWithinDotToDirectionConditions += ApplyColorSettings.ApplyColorsOnAlphaChannel.PaintConditions.IsVertexNormalWithinDotToDirection.Num();
		AmountOfOnlyAffectVerticesWithDirectionToActorOrLocationConditions += ApplyColorSettings.ApplyColorsOnAlphaChannel.PaintConditions.OnlyAffectVerticesWithDirectionToActorOrLocation.Num();
		AmountOfOnlyAffectVerticesWithinDirectionFromActorOrLocation += ApplyColorSettings.ApplyColorsOnAlphaChannel.PaintConditions.OnlyAffectVerticesWithinDirectionFromActorOrLocation.Num();
		AmountOfIfVertexIsAboveOrBelowWorldZ += ApplyColorSettings.ApplyColorsOnAlphaChannel.PaintConditions.IfVertexIsAboveOrBelowWorldZ.Num();
		AmountOfIfVertexColorWithinRange += ApplyColorSettings.ApplyColorsOnAlphaChannel.PaintConditions.IfVertexColorIsWithinRange.Num();
		AmountOfIfVertexHasLineOfSightTo += ApplyColorSettings.ApplyColorsOnAlphaChannel.PaintConditions.IfVertexHasLineOfSightTo.Num();
		AmountOfIfVertexIsOnBone += ApplyColorSettings.ApplyColorsOnAlphaChannel.PaintConditions.IfVertexIsOnBone.Num();
		AmountOfIfVertexIsOnMaterial += ApplyColorSettings.ApplyColorsOnAlphaChannel.PaintConditions.IfVertexIsOnMaterial.Num();

		if (AmountOfVertexNormalWithinDotToDirectionConditions > 0 || AmountOfOnlyAffectVerticesWithDirectionToActorOrLocationConditions > 0 || AmountOfOnlyAffectVerticesWithinDirectionFromActorOrLocation > 0 || AmountOfIfVertexIsAboveOrBelowWorldZ > 0 || AmountOfIfVertexColorWithinRange > 0 || AmountOfIfVertexHasLineOfSightTo > 0 || AmountOfIfVertexIsOnBone > 0 || AmountOfIfVertexIsOnMaterial > 0) {

			IsOnAlphaChannel = true;
		}
	}


	if (IsOnPhysicsSurface || IsOnRedChannel || IsOnGreenChannel || IsOnBlueChannel || IsOnAlphaChannel)
		return true;


	return false;
}


//--------------------------------------------------------

// Get Registered Paint Task Callbacks Objects For Specific Mesh Component

FRVPDPCallbackFromSpecifiedMeshComponentsInfo UVertexPaintFunctionLibrary::GetRegisteredPaintTaskCallbacksObjectsForSpecificMeshComponent(const UObject* WorldContextObject, const UPrimitiveComponent* MeshComponent) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetRegisteredPaintTaskCallbacksObjectsForSpecificMeshComponent);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - GetRegisteredPaintTaskCallbacksObjectsForSpecificMeshComponent");

	if (!WorldContextObject) return FRVPDPCallbackFromSpecifiedMeshComponentsInfo();
	if (!MeshComponent) return FRVPDPCallbackFromSpecifiedMeshComponentsInfo();

	if (UVertexPaintDetectionGISubSystem* gameInstanceSubsystem = GetVertexPaintGameInstanceSubsystem(WorldContextObject))
		return gameInstanceSubsystem->GetPaintTaskCallbacksFromSpecificMeshComponents().FindRef(MeshComponent);

	return FRVPDPCallbackFromSpecifiedMeshComponentsInfo();
}


//--------------------------------------------------------

// Get Registered Detect Task Callbacks Objects For Specific Mesh Component

FRVPDPCallbackFromSpecifiedMeshComponentsInfo UVertexPaintFunctionLibrary::GetRegisteredDetectTaskCallbacksObjectsForSpecificMeshComponent(const UObject* WorldContextObject, const UPrimitiveComponent* MeshComponent) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetRegisteredDetectTaskCallbacksObjectsForSpecificMeshComponent);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - GetRegisteredDetectTaskCallbacksObjectsForSpecificMeshComponent");

	if (!WorldContextObject) return FRVPDPCallbackFromSpecifiedMeshComponentsInfo();
	if (!MeshComponent) return FRVPDPCallbackFromSpecifiedMeshComponentsInfo();

	if (UVertexPaintDetectionGISubSystem* gameInstanceSubsystem = GetVertexPaintGameInstanceSubsystem(WorldContextObject))
		return gameInstanceSubsystem->GetDetectTaskCallbacksFromSpecificMeshComponents().FindRef(MeshComponent);

	return FRVPDPCallbackFromSpecifiedMeshComponentsInfo();
}


/** Helper struct for the mesh component vert position octree - Copied from MeshPaintHelpers.cpp ApplyVertexColorsToAllLODs */
struct FStaticMeshComponentVertPosOctreeSemantics {
	enum { MaxElementsPerLeaf = 16 };
	enum { MinInclusiveElementsPerNode = 7 };
	enum { MaxNodeDepth = 12 };

	typedef TInlineAllocator<MaxElementsPerLeaf> ElementAllocator;

	/**
	 * Get the bounding box of the provided octree element. In this case, the box
	 * is merely the point specified by the element.
	 *
	 * @param	Element	Octree element to get the bounding box for
	 *
	 * @return	Bounding box of the provided octree element
	 */
	FORCEINLINE static FBoxCenterAndExtent GetBoundingBox(const FExtendedPaintedVertex& Element)
	{
		return FBoxCenterAndExtent(Element.Position, FVector::ZeroVector);
	}

	/**
	 * Determine if two octree elements are equal
	 *
	 * @param	A	First octree element to check
	 * @param	B	Second octree element to check
	 *
	 * @return	true if both octree elements are equal, false if they are not
	 */
	FORCEINLINE static bool AreElementsEqual(const FExtendedPaintedVertex& A, const FExtendedPaintedVertex& B)
	{
		return (A.Position == B.Position && A.Normal == B.Normal && A.Color == B.Color);
	}

	/** Ignored for this implementation */
	FORCEINLINE static void SetElementId(const FExtendedPaintedVertex& Element, FOctreeElementId2 Id)
	{
	}
};

typedef TOctree2<FExtendedPaintedVertex, FStaticMeshComponentVertPosOctreeSemantics> TSMCVertPosOctree;


//-------------------------------------------------------

// Propogate LOD 0 Vertex Colors To LODs

void UVertexPaintFunctionLibrary::PropagateLOD0VertexColorsToLODs(const TArray<FExtendedPaintedVertex>& Lod0PaintedVerts, FRVPDPVertexDataInfo& VertexDetectMeshData, const TArray<FBox>& LodBaseBounds, const TArray<FColorVertexBuffer*>& ColorVertexBufferToUpdate, bool ReturnPropogateToLODInfo, TMap<int32, FRVPDPPropogateToLODsInfo>& PropogateToLODInfo) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_PropogateLOD0VertexColorsToLODs);
	RVPDP_CPUPROFILER_STR("Calculate Colors Task - PropogateLOD0VertexColorsToLODs");

	PropogateToLODInfo.Empty();

	if (VertexDetectMeshData.MeshDataPerLOD.Num() <= 1) return;
	if (VertexDetectMeshData.MeshDataPerLOD[0].MeshVertexPositionsInComponentSpacePerLODArray.Num() == 0) return;


	TArray<FExtendedPaintedVertex> pointsToConsider;
	const float distanceOverNormalThreshold = KINDA_SMALL_NUMBER;

	FBox combinedBounds = LodBaseBounds[0];
	for (uint32 i = 1; i < static_cast<uint32>(VertexDetectMeshData.MeshDataPerLOD.Num()); ++i)
		combinedBounds += LodBaseBounds[i];


	TSMCVertPosOctree vertPosOctree(combinedBounds.GetCenter(), combinedBounds.GetExtent().GetMax());
	for (int32 paintedVertexIndex = 0; paintedVertexIndex < Lod0PaintedVerts.Num(); ++paintedVertexIndex) {

		FExtendedPaintedVertex paintedVertex = Lod0PaintedVerts[paintedVertexIndex];
		paintedVertex.VertexIndex = paintedVertexIndex;
		vertPosOctree.AddElement(paintedVertex);
	}


	for (int32 i = 1; i < VertexDetectMeshData.MeshDataPerLOD.Num(); ++i) {

		if (!VertexDetectMeshData.MeshDataPerLOD.IsValidIndex(i)) continue;
		if (VertexDetectMeshData.MeshDataPerLOD[i].MeshVertexPositionsInComponentSpacePerLODArray.Num() == 0) continue;


		for (int32 currentLODVertex = 0; currentLODVertex < VertexDetectMeshData.MeshDataPerLOD[i].MeshVertexPositionsInComponentSpacePerLODArray.Num(); ++currentLODVertex) {


			pointsToConsider.Empty();
			const FVector curPosition = VertexDetectMeshData.MeshDataPerLOD[i].MeshVertexPositionsInComponentSpacePerLODArray[currentLODVertex];
			const FVector curNormal = VertexDetectMeshData.MeshDataPerLOD[i].MeshVertexNormalsPerLODArray[currentLODVertex];


			// Iterate through the octree to find the vertices closest to the current new point
			vertPosOctree.FindNearbyElements(curPosition, [&pointsToConsider](const FExtendedPaintedVertex& Vertex) {

				pointsToConsider.Add(Vertex); 
			});


			// Check that there are any points
			if (pointsToConsider.Num() == 0) continue;

			// If any points to consider were found, iterate over each and find which one is the closest to the new point 
			FExtendedPaintedVertex bestVertex = pointsToConsider[0];
			const FVector bestVertexNormal = bestVertex.Normal;
			
			float bestDistanceSquared = (bestVertex.Position - curPosition).SizeSquared();
			float bestNormalDot = bestVertexNormal | curNormal;
			

			for (int32 considerationIndex = 1; considerationIndex < pointsToConsider.Num(); ++considerationIndex) {

				const FExtendedPaintedVertex vertexToConsider = pointsToConsider[considerationIndex];
				const FVector vertexNormalToConsider = vertexToConsider.Normal;

				const float distSqrdToConsider = (vertexToConsider.Position - curPosition).SizeSquared();
				const float normalDotToConsider = vertexNormalToConsider | curNormal;

				if (distSqrdToConsider < bestDistanceSquared - distanceOverNormalThreshold) {

					bestVertex = vertexToConsider;
					bestDistanceSquared = distSqrdToConsider;
					bestNormalDot = normalDotToConsider;
				}

				else if (distSqrdToConsider < bestDistanceSquared + distanceOverNormalThreshold && normalDotToConsider > bestNormalDot) {

					bestVertex = vertexToConsider;
					bestDistanceSquared = distSqrdToConsider;
					bestNormalDot = normalDotToConsider;
				}
			}


			if (VertexDetectMeshData.MeshDataPerLOD.IsValidIndex(i) && VertexDetectMeshData.MeshDataPerLOD[i].MeshVertexColorsPerLODArray.IsValidIndex(currentLODVertex)) {

				VertexDetectMeshData.MeshDataPerLOD[i].MeshVertexColorsPerLODArray[currentLODVertex] = bestVertex.Color;
			}


			if (ColorVertexBufferToUpdate.IsValidIndex(i))
				ColorVertexBufferToUpdate[i]->VertexColor(currentLODVertex) = bestVertex.Color;


			// Can optionally return propogate to LOD info which is which LOD0 vertex that is the best one, for every vertex index at every lod above 0, so we can much easier get what LOD0 vertex color to apply to a vertex at a higher LOD. When stored in a data asset we don't have to run this function in runtime but can quickly get this data from the optimization data asset instead. 
			if (ReturnPropogateToLODInfo && i > 0) {

				FRVPDPPropogateToLODsInfo propogateToLODInfo;

				// If the LOD has been added then gets current info
				if (PropogateToLODInfo.Contains(i))
					propogateToLODInfo = PropogateToLODInfo.FindRef(i);

				// Adds the best LOD 0 vertex for this LODs Vertex
				propogateToLODInfo.LODVerticesAssociatedLOD0Vertex.Add(currentLODVertex, bestVertex.VertexIndex);

				PropogateToLODInfo.Add(i, propogateToLODInfo);
			}
		}
	}
}


//-------------------------------------------------------

// Run Task Job Callback Interfaces

void UVertexPaintFunctionLibrary::RunFinishedDetectTaskCallbacks(const FRVPDPTaskResults& DetectTaskResult, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_RunFinishedDetectTaskCallbacks);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - RunFinishedDetectTaskCallbacks");

	if (!TaskFundamentalSettings.TaskWorld.IsValid()) return;


	if (UVertexPaintDetectionGISubSystem* gameInstance = GetVertexPaintGameInstanceSubsystem(TaskFundamentalSettings.TaskWorld.Get())) {


		gameInstance->VertexDetectTaskFinished.Broadcast(DetectTaskResult, AdditionalDataToPassThrough);

		FRVPDPCallbackFromSpecifiedMeshComponentsInfo callbackOnObjectsInfo = TaskFundamentalSettings.CallbackSettings.CallbacksOnObjectsForMeshComponent;

		if (callbackOnObjectsInfo.RunCallbacksOnObjects.Num() > 0) {

			for (auto runCallbacksOnObjects : callbackOnObjectsInfo.RunCallbacksOnObjects) {

				UObject* callbackOnObject = runCallbacksOnObjects.Get();

				if (IsValid(callbackOnObject) && callbackOnObject->GetClass()->ImplementsInterface(UVertexPaintDetectionInterface::StaticClass())) {

					IVertexPaintDetectionInterface::Execute_DetectTaskFinishedOnRegisteredMeshComponent(callbackOnObject, DetectTaskResult, AdditionalDataToPassThrough);
				}
			}
		}
	}
}

void UVertexPaintFunctionLibrary::RunFinishedPaintTaskCallbacks(const FRVPDPTaskResults& TaskResult, const FRVPDPPaintTaskResultInfo& PaintTaskResult, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_RunFinishedPaintTaskCallbacks);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - RunFinishedPaintTaskCallbacks");

	if (!TaskFundamentalSettings.TaskWorld.IsValid()) return;


	if (UVertexPaintDetectionGISubSystem* gameInstance = GetVertexPaintGameInstanceSubsystem(TaskFundamentalSettings.TaskWorld.Get())) {


		gameInstance->VertexPaintTaskFinished.Broadcast(TaskResult, PaintTaskResult, AdditionalDataToPassThrough);


		FRVPDPCallbackFromSpecifiedMeshComponentsInfo callbackOnObjectsInfo = TaskFundamentalSettings.CallbackSettings.CallbacksOnObjectsForMeshComponent;

		if (callbackOnObjectsInfo.RunCallbacksOnObjects.Num() > 0) {

			for (auto runCallbacksOnObjects : callbackOnObjectsInfo.RunCallbacksOnObjects) {
				
				if (runCallbacksOnObjects.IsValid()) {

					UObject* callbackOnObject = runCallbacksOnObjects.Get();

					if (IsValid(callbackOnObject) && callbackOnObject->GetClass()->ImplementsInterface(UVertexPaintDetectionInterface::StaticClass())) {

						IVertexPaintDetectionInterface::Execute_PaintTaskFinishedOnRegisteredMeshComponent(callbackOnObject, TaskResult, PaintTaskResult, AdditionalDataToPassThrough);
					}
				}
			}
		}
	}
}

void UVertexPaintFunctionLibrary::RunGetClosestVertexDataCallbackInterfaces(const FRVPDPTaskResults& DetectTaskResult, const FRVPDPGetClosestVertexDataSettings& GetClosestVertexDataSettings, const FRVPDPClosestVertexDataResults& ClosestVertexColorResult, const FRVPDPEstimatedColorAtHitLocationInfo& EstimatedColorAtHitLocationResult, const FRVPDPAverageColorInAreaInfo& AverageColor, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_RunGetClosestVertexDataCallbackInterfaces);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - RunGetClosestVertexDataCallbackInterfaces");


	if (TaskFundamentalSettings.CallbackSettings.ObjectToRunInterfacesOn.IsValid()) {

		UObject* objectToRunInterfacesOn = TaskFundamentalSettings.CallbackSettings.ObjectToRunInterfacesOn.Get();


		if (TaskFundamentalSettings.CallbackSettings.RunCallbackInterfacesOnObject) {

			if (IsValid(objectToRunInterfacesOn) && objectToRunInterfacesOn->GetClass() && objectToRunInterfacesOn->GetClass()->ImplementsInterface(UVertexPaintDetectionInterface::StaticClass())) {

				IVertexPaintDetectionInterface::Execute_GetClosestVertexDataOnActor(objectToRunInterfacesOn, DetectTaskResult, GetClosestVertexDataSettings, ClosestVertexColorResult, EstimatedColorAtHitLocationResult, AverageColor, AdditionalDataToPassThrough);
			}
		}


		if (TaskFundamentalSettings.CallbackSettings.RunCallbackInterfacesOnObjectComponents && objectToRunInterfacesOn) {

			if (AActor* actorToRunInterface = Cast<AActor>(objectToRunInterfacesOn)) {

				// Runs the Interface on any components as well if they're owner got it run, so you can't make cleaner implementations
				for (UActorComponent* actorComp : actorToRunInterface->GetComponents()) {

					if (actorComp->GetClass()->ImplementsInterface(UVertexPaintDetectionInterface::StaticClass())) {

						IVertexPaintDetectionInterface::Execute_GetClosestVertexDataOnActor(actorComp, DetectTaskResult, GetClosestVertexDataSettings, ClosestVertexColorResult, EstimatedColorAtHitLocationResult, AverageColor, AdditionalDataToPassThrough);
					}
				}
			}
		}
	}
}

void UVertexPaintFunctionLibrary::RunGetAllColorsOnlyCallbackInterfaces(const FRVPDPTaskResults& DetectTaskResult, const FRVPDPGetColorsOnlySettings& GetAllVertexColorsSettings, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_RunGetAllColorsOnlyCallbackInterfaces);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - RunGetAllColorsOnlyCallbackInterfaces");


	if (TaskFundamentalSettings.CallbackSettings.ObjectToRunInterfacesOn.IsValid()) {

		UObject* objectToRunInterfacesOn = TaskFundamentalSettings.CallbackSettings.ObjectToRunInterfacesOn.Get();


		if (TaskFundamentalSettings.CallbackSettings.RunCallbackInterfacesOnObject) {

			if (IsValid(objectToRunInterfacesOn) && objectToRunInterfacesOn->GetClass()->ImplementsInterface(UVertexPaintDetectionInterface::StaticClass())) {

				IVertexPaintDetectionInterface::Execute_GetAllVertexColorsOnlyOnActor(objectToRunInterfacesOn, DetectTaskResult, GetAllVertexColorsSettings, AdditionalDataToPassThrough);
			}
		}


		if (TaskFundamentalSettings.CallbackSettings.RunCallbackInterfacesOnObjectComponents && objectToRunInterfacesOn) {

			if (AActor* actorToRunInterface = Cast<AActor>(objectToRunInterfacesOn)) {

				// Runs the Interface on any components as well if they're owner got it run, so you can't make cleaner implementations
				for (UActorComponent* actorComp : actorToRunInterface->GetComponents()) {

					if (actorComp->GetClass()->ImplementsInterface(UVertexPaintDetectionInterface::StaticClass())) {

						IVertexPaintDetectionInterface::Execute_GetAllVertexColorsOnlyOnActor(actorComp, DetectTaskResult, GetAllVertexColorsSettings, AdditionalDataToPassThrough);
					}
				}
			}
		}
	}
}

void UVertexPaintFunctionLibrary::RunGetColorsWithinAreaCallbackInterfaces(const FRVPDPTaskResults& DetectTaskResult, const FRVPDPGetColorsWithinAreaSettings& GetColorsWithinAreaSettings, const FRVPDPWithinAreaResults& WithinAreaResults, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_RunGetColorsWithinAreaCallbackInterfaces);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - RunGetColorsWithinAreaCallbackInterfaces");


	if (TaskFundamentalSettings.CallbackSettings.ObjectToRunInterfacesOn.IsValid()) {

		UObject* objectToRunInterfacesOn = TaskFundamentalSettings.CallbackSettings.ObjectToRunInterfacesOn.Get();


		if (TaskFundamentalSettings.CallbackSettings.RunCallbackInterfacesOnObject && IsValid(objectToRunInterfacesOn) && objectToRunInterfacesOn->GetClass()->ImplementsInterface(UVertexPaintDetectionInterface::StaticClass())) {

			IVertexPaintDetectionInterface::Execute_GetColorsWithinArea(objectToRunInterfacesOn, DetectTaskResult, GetColorsWithinAreaSettings, WithinAreaResults, AdditionalDataToPassThrough);
		}


		if (TaskFundamentalSettings.CallbackSettings.RunCallbackInterfacesOnObjectComponents && IsValid(objectToRunInterfacesOn)) {

			if (AActor* actorToRunInterface = Cast<AActor>(objectToRunInterfacesOn)) {

				// Runs the Interface on any components as well if they're owner got it run, so you can't make cleaner implementations
				for (UActorComponent* actorComp : actorToRunInterface->GetComponents()) {

					if (actorComp->GetClass()->ImplementsInterface(UVertexPaintDetectionInterface::StaticClass())) {

						IVertexPaintDetectionInterface::Execute_GetColorsWithinArea(actorComp, DetectTaskResult, GetColorsWithinAreaSettings, WithinAreaResults, AdditionalDataToPassThrough);
					}
				}
			}
		}
	}
}

void UVertexPaintFunctionLibrary::RunPaintAtLocationCallbackInterfaces(const FRVPDPTaskResults& TaskResult, const FRVPDPPaintTaskResultInfo& PaintTaskResult, const FRVPDPPaintAtLocationSettings& PaintAtLocationSettings, const FRVPDPClosestVertexDataResults& ClosestVertexColorResult, const FRVPDPEstimatedColorAtHitLocationInfo& EstimatedColorAtHitLocationResult, const FRVPDPAverageColorInAreaInfo& AverageColor, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_RunPaintAtLocationCallbackInterfaces);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - RunPaintAtLocationCallbackInterfaces");


	if (TaskFundamentalSettings.CallbackSettings.ObjectToRunInterfacesOn.IsValid()) {

		UObject* objectToRunInterfacesOn = TaskFundamentalSettings.CallbackSettings.ObjectToRunInterfacesOn.Get();


		if (TaskFundamentalSettings.CallbackSettings.RunCallbackInterfacesOnObject && IsValid(objectToRunInterfacesOn) && objectToRunInterfacesOn->GetClass()->ImplementsInterface(UVertexPaintDetectionInterface::StaticClass())) {

			IVertexPaintDetectionInterface::Execute_PaintedOnActor_AtLocation(objectToRunInterfacesOn, TaskResult, PaintTaskResult, PaintAtLocationSettings, ClosestVertexColorResult, EstimatedColorAtHitLocationResult, AverageColor, AdditionalDataToPassThrough);

			IVertexPaintDetectionInterface::Execute_ColorsAppliedOnActor(objectToRunInterfacesOn, TaskResult, PaintTaskResult, PaintAtLocationSettings, AdditionalDataToPassThrough);
		}


		if (TaskFundamentalSettings.CallbackSettings.RunCallbackInterfacesOnObjectComponents && IsValid(objectToRunInterfacesOn)) {

			if (AActor* actorToRunInterface = Cast<AActor>(objectToRunInterfacesOn)) {

				// Runs the Interface on any components as well if they're owner got it run, so you can't make cleaner implementations
				for (UActorComponent* actorComp : actorToRunInterface->GetComponents()) {

					if (actorComp->GetClass()->ImplementsInterface(UVertexPaintDetectionInterface::StaticClass())) {

						IVertexPaintDetectionInterface::Execute_PaintedOnActor_AtLocation(actorComp, TaskResult, PaintTaskResult, PaintAtLocationSettings, ClosestVertexColorResult, EstimatedColorAtHitLocationResult, AverageColor, AdditionalDataToPassThrough);

						IVertexPaintDetectionInterface::Execute_ColorsAppliedOnActor(actorComp, TaskResult, PaintTaskResult, PaintAtLocationSettings, AdditionalDataToPassThrough);
					}
				}
			}
		}
	}
}

void UVertexPaintFunctionLibrary::RunPaintWithinAreaCallbackInterfaces(const FRVPDPTaskResults& TaskResult, const FRVPDPPaintTaskResultInfo& PaintTaskResult, const FRVPDPPaintWithinAreaSettings& PaintWithinAreaSettings, const FRVPDPWithinAreaResults& WithinAreaResults, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_RunPaintWithinAreaCallbackInterfaces);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - RunPaintWithinAreaCallbackInterfaces");


	if (TaskFundamentalSettings.CallbackSettings.ObjectToRunInterfacesOn.IsValid()) {

		UObject* objectToRunInterfacesOn = TaskFundamentalSettings.CallbackSettings.ObjectToRunInterfacesOn.Get();


		if (TaskFundamentalSettings.CallbackSettings.RunCallbackInterfacesOnObject && IsValid(objectToRunInterfacesOn) && objectToRunInterfacesOn->GetClass()->ImplementsInterface(UVertexPaintDetectionInterface::StaticClass())) {

			IVertexPaintDetectionInterface::Execute_PaintedOnActor_WithinArea(objectToRunInterfacesOn, TaskResult, PaintTaskResult, PaintWithinAreaSettings, WithinAreaResults, AdditionalDataToPassThrough);

			IVertexPaintDetectionInterface::Execute_ColorsAppliedOnActor(objectToRunInterfacesOn, TaskResult, PaintTaskResult, PaintWithinAreaSettings, AdditionalDataToPassThrough);
		}


		if (TaskFundamentalSettings.CallbackSettings.RunCallbackInterfacesOnObjectComponents && IsValid(objectToRunInterfacesOn)) {

			if (AActor* actorToRunInterface = Cast<AActor>(objectToRunInterfacesOn)) {

				// Runs the Interface on any components as well if they're owner got it run, so you can't make cleaner implementations
				for (UActorComponent* actorComp : actorToRunInterface->GetComponents()) {

					if (actorComp->GetClass()->ImplementsInterface(UVertexPaintDetectionInterface::StaticClass())) {

						IVertexPaintDetectionInterface::Execute_PaintedOnActor_WithinArea(actorComp, TaskResult, PaintTaskResult, PaintWithinAreaSettings, WithinAreaResults, AdditionalDataToPassThrough);

						IVertexPaintDetectionInterface::Execute_ColorsAppliedOnActor(actorComp, TaskResult, PaintTaskResult, PaintWithinAreaSettings, AdditionalDataToPassThrough);
					}
				}
			}
		}
	}
}

void UVertexPaintFunctionLibrary::RunPaintEntireMeshCallbackInterfaces(const FRVPDPTaskResults& TaskResult, const FRVPDPPaintTaskResultInfo& PaintTaskResult, const FRVPDPPaintOnEntireMeshSettings& PaintOnEntireMeshSettings, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_RunPaintEntireMeshCallbackInterfaces);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - RunPaintEntireMeshCallbackInterfaces");


	if (TaskFundamentalSettings.CallbackSettings.ObjectToRunInterfacesOn.IsValid()) {

		UObject* objectToRunInterfacesOn = TaskFundamentalSettings.CallbackSettings.ObjectToRunInterfacesOn.Get();


		if (TaskFundamentalSettings.CallbackSettings.RunCallbackInterfacesOnObject && IsValid(objectToRunInterfacesOn) && objectToRunInterfacesOn->GetClass()->ImplementsInterface(UVertexPaintDetectionInterface::StaticClass())) {

			IVertexPaintDetectionInterface::Execute_PaintedOnActor_EntireMesh(objectToRunInterfacesOn, TaskResult, PaintTaskResult, PaintOnEntireMeshSettings, AdditionalDataToPassThrough);

			IVertexPaintDetectionInterface::Execute_ColorsAppliedOnActor(objectToRunInterfacesOn, TaskResult, PaintTaskResult, PaintOnEntireMeshSettings, AdditionalDataToPassThrough);
		}


		if (TaskFundamentalSettings.CallbackSettings.RunCallbackInterfacesOnObjectComponents && IsValid(objectToRunInterfacesOn)) {

			if (AActor* actorToRunInterface = Cast<AActor>(objectToRunInterfacesOn)) {

				// Runs the Interface on any components as well if they're owner got it run, so you can't make cleaner implementations
				for (UActorComponent* actorComp : actorToRunInterface->GetComponents()) {

					if (actorComp->GetClass()->ImplementsInterface(UVertexPaintDetectionInterface::StaticClass())) {

						IVertexPaintDetectionInterface::Execute_PaintedOnActor_EntireMesh(actorComp, TaskResult, PaintTaskResult, PaintOnEntireMeshSettings, AdditionalDataToPassThrough);

						IVertexPaintDetectionInterface::Execute_ColorsAppliedOnActor(actorComp, TaskResult, PaintTaskResult, PaintOnEntireMeshSettings, AdditionalDataToPassThrough);
					}
				}
			}
		}
	}
}

void UVertexPaintFunctionLibrary::RunPaintColorSnippetCallbackInterfaces(const FRVPDPTaskResults& TaskResult, const FRVPDPPaintTaskResultInfo& PaintTaskResult, const FRVPDPPaintColorSnippetSettings& PaintColorSnippetSettings, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_RunPaintColorSnippetCallbackInterfaces);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - RunPaintColorSnippetCallbackInterfaces");


	if (TaskFundamentalSettings.CallbackSettings.ObjectToRunInterfacesOn.IsValid()) {

		UObject* objectToRunInterfacesOn = TaskFundamentalSettings.CallbackSettings.ObjectToRunInterfacesOn.Get();


		// Calls interface on the painted Actor so they can run custom things if they get painted on
		if (TaskFundamentalSettings.CallbackSettings.RunCallbackInterfacesOnObject && IsValid(objectToRunInterfacesOn) && objectToRunInterfacesOn->GetClass()->ImplementsInterface(UVertexPaintDetectionInterface::StaticClass())) {

			IVertexPaintDetectionInterface::Execute_PaintedOnActor_PaintColorSnippet(objectToRunInterfacesOn, TaskResult, PaintTaskResult, PaintColorSnippetSettings, AdditionalDataToPassThrough);

			IVertexPaintDetectionInterface::Execute_ColorsAppliedOnActor(objectToRunInterfacesOn, TaskResult, PaintTaskResult, PaintColorSnippetSettings, AdditionalDataToPassThrough);
		}


		if (TaskFundamentalSettings.CallbackSettings.RunCallbackInterfacesOnObjectComponents && IsValid(objectToRunInterfacesOn)) {

			if (AActor* actorToRunInterface = Cast<AActor>(objectToRunInterfacesOn)) {

				// Runs the Interface on any components as well if they're owner got it run, so you can't make cleaner implementations
				for (UActorComponent* actorComp : actorToRunInterface->GetComponents()) {

					if (actorComp->GetClass()->ImplementsInterface(UVertexPaintDetectionInterface::StaticClass())) {

						IVertexPaintDetectionInterface::Execute_PaintedOnActor_PaintColorSnippet(actorComp, TaskResult, PaintTaskResult, PaintColorSnippetSettings, AdditionalDataToPassThrough);

						IVertexPaintDetectionInterface::Execute_ColorsAppliedOnActor(actorComp, TaskResult, PaintTaskResult, PaintColorSnippetSettings, AdditionalDataToPassThrough);
					}
				}
			}
		}
	}
}

void UVertexPaintFunctionLibrary::RunPaintSetMeshColorsCallbackInterfaces(const FRVPDPTaskResults& TaskResult, const FRVPDPPaintTaskResultInfo& PaintTaskResult, const FRVPDPSetVertexColorsSettings& SetVertexColorsSettings, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_RunPaintSetMeshColorsCallbackInterfaces);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - RunPaintSetMeshColorsCallbackInterfaces");


	if (TaskFundamentalSettings.CallbackSettings.ObjectToRunInterfacesOn.IsValid()) {

		UObject* objectToRunInterfacesOn = TaskFundamentalSettings.CallbackSettings.ObjectToRunInterfacesOn.Get();


		// Calls interface on the painted Actor so they can run custom things if they get painted on
		if (TaskFundamentalSettings.CallbackSettings.RunCallbackInterfacesOnObject && IsValid(objectToRunInterfacesOn) && objectToRunInterfacesOn->GetClass()->ImplementsInterface(UVertexPaintDetectionInterface::StaticClass())) {

			IVertexPaintDetectionInterface::Execute_PaintedOnActor_SetMeshComponentVertexColors(objectToRunInterfacesOn, TaskResult, PaintTaskResult, SetVertexColorsSettings, AdditionalDataToPassThrough);

			IVertexPaintDetectionInterface::Execute_ColorsAppliedOnActor(objectToRunInterfacesOn, TaskResult, PaintTaskResult, SetVertexColorsSettings, AdditionalDataToPassThrough);
		}


		if (TaskFundamentalSettings.CallbackSettings.RunCallbackInterfacesOnObjectComponents && IsValid(objectToRunInterfacesOn)) {

			if (AActor* actorToRunInterface = Cast<AActor>(objectToRunInterfacesOn)) {

				// Runs the Interface on any components as well if they're owner got it run, so you can't make cleaner implementations
				for (UActorComponent* actorComp : actorToRunInterface->GetComponents()) {

					if (actorComp->GetClass()->ImplementsInterface(UVertexPaintDetectionInterface::StaticClass())) {

						IVertexPaintDetectionInterface::Execute_PaintedOnActor_SetMeshComponentVertexColors(actorComp, TaskResult, PaintTaskResult, SetVertexColorsSettings, AdditionalDataToPassThrough);

						IVertexPaintDetectionInterface::Execute_ColorsAppliedOnActor(actorComp, TaskResult, PaintTaskResult, SetVertexColorsSettings, AdditionalDataToPassThrough);
					}
				}
			}
		}
	}
}

void UVertexPaintFunctionLibrary::RunPaintSetMeshColorsUsingSerializedStringCallbackInterfaces(const FRVPDPTaskResults& TaskResult, const FRVPDPPaintTaskResultInfo& PaintTaskResult, const FRVPDPSetVertexColorsUsingSerializedStringSettings& SetVertexColorsUsingSerializedStringSettings, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough) {

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_RunPaintSetMeshColorsUsingSerializedStringCallbackInterfaces);
	RVPDP_CPUPROFILER_STR("Vertex Paint Function Library - RunPaintSetMeshColorsUsingSerializedStringCallbackInterfaces");


	if (TaskFundamentalSettings.CallbackSettings.ObjectToRunInterfacesOn.IsValid()) {

		UObject* objectToRunInterfacesOn = TaskFundamentalSettings.CallbackSettings.ObjectToRunInterfacesOn.Get();


		// Calls interface on the painted Actor so they can run custom things if they get painted on
		if (TaskFundamentalSettings.CallbackSettings.RunCallbackInterfacesOnObject && IsValid(objectToRunInterfacesOn) && objectToRunInterfacesOn->GetClass()->ImplementsInterface(UVertexPaintDetectionInterface::StaticClass())) {

			IVertexPaintDetectionInterface::Execute_PaintedOnActor_SetMeshComponentVertexColorsUsingSerializedString(objectToRunInterfacesOn, TaskResult, PaintTaskResult, SetVertexColorsUsingSerializedStringSettings, AdditionalDataToPassThrough);

			IVertexPaintDetectionInterface::Execute_ColorsAppliedOnActor(objectToRunInterfacesOn, TaskResult, PaintTaskResult, SetVertexColorsUsingSerializedStringSettings, AdditionalDataToPassThrough);
		}


		if (TaskFundamentalSettings.CallbackSettings.RunCallbackInterfacesOnObjectComponents && IsValid(objectToRunInterfacesOn)) {

			if (AActor* actorToRunInterface = Cast<AActor>(objectToRunInterfacesOn)) {

				// Runs the Interface on any components as well if they're owner got it run, so you can't make cleaner implementations
				for (UActorComponent* actorComp : actorToRunInterface->GetComponents()) {

					if (actorComp->GetClass()->ImplementsInterface(UVertexPaintDetectionInterface::StaticClass())) {

						IVertexPaintDetectionInterface::Execute_PaintedOnActor_SetMeshComponentVertexColorsUsingSerializedString(actorComp, TaskResult, PaintTaskResult, SetVertexColorsUsingSerializedStringSettings, AdditionalDataToPassThrough);

						IVertexPaintDetectionInterface::Execute_ColorsAppliedOnActor(actorComp, TaskResult, PaintTaskResult, SetVertexColorsUsingSerializedStringSettings, AdditionalDataToPassThrough);
					}
				}
			}
		}
	}
}
