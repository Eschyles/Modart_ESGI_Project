// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "VertexPaintDetectionSettings.generated.h"

class UVertexPaintOptimizationDataAsset;
class UVertexPaintMaterialDataAsset;
class UVertexPaintColorSnippetRefs;


//-------------------------------------------------------

// Since the engines own EThreadPriority isn't blueprint type i had to make our own 

UENUM(BlueprintType)
enum class ETVertexPaintThreadPriority : uint8 {

	TimeCritical = 0 UMETA(DisplayName = "Time Critical"),
	Highest = 1 UMETA(DisplayName = "Highest"),
	Normal = 2 UMETA(DisplayName = "Normal"),
	Slowest = 3 UMETA(DisplayName = "Slow")
};


/**
 *
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Runtime Vertex Paint and Detection Plugin Settings"))
class VERTEXPAINTDETECTIONPLUGIN_API UVertexPaintDetectionSettings : public UDeveloperSettings {

	GENERATED_BODY()

public:

	UVertexPaintDetectionSettings();

	UPROPERTY(config, VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin|Data Assets", meta = (ToolTip = "Which Optimization Data Asset we should use when painting. Meshes that are registered in it are only painted on those amounts of LODs which can save alot of performance. \nThis can be changed in the Editor Widget. "))
		TSoftObjectPtr<UVertexPaintOptimizationDataAsset> OptimizationDataAssetToUse = nullptr;

	UPROPERTY(config, VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin|Data Assets", meta = (ToolTip = "Which Vertex Paint Material Data Asset to use. This is the data asset you register your materials that use vertex colors that you want to detect what is on what channel etc. \nIf there is none set then you can't detect vertex data, you can only paint. \nThis can be changed in the Editor Widget. "))
		TSoftObjectPtr<UVertexPaintMaterialDataAsset> MaterialsDataAssetToUse = nullptr;

	UPROPERTY(config, VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin|Data Assets", meta = (ToolTip = "Which Data Asset to store references to Color Snippet Data Assets. Used so we can more optimally get which color snippet data asset to bring into memory so we don't have to go through them all in order to find a stored snippet. \nThis can be changed in the Editor Widget. "))
		TSoftObjectPtr<UVertexPaintColorSnippetRefs> ColorSnippetReferencesDataAssetToUse = nullptr;

	UPROPERTY(config, EditDefaultsOnly, Category = "Runtime Vertex Paint and Detection Plugin|Editor Widget", meta = (ToolTip = "If we should Show Notifications when making changes in the Editor Widget. "))
		bool EditorWidgetNotificationEnabled = true;

	UPROPERTY(config, EditDefaultsOnly, Category = "Runtime Vertex Paint and Detection Plugin|Editor Widget", meta = (ToolTip = "Duration of the Notifications that pop up when making certain changes in  the editor widget. "))
		float EditorWidgetNotificationDuration = 3;

		UPROPERTY(config, EditDefaultsOnly, Category = "Runtime Vertex Paint and Detection Plugin|Editor Widget", meta = (ToolTip = "When the Editor Widget looks up Data Assets, Skeletal and Static Meshes etc. it by default only gets those from the Game Folder, i.e. Content Browser. But you can here add additional paths, in case you have something in a plugin or something.", LongPackageName))
		TArray<FDirectoryPath> EditorWidgetAdditionalPathsToLookUpAssets;

	UPROPERTY(config, EditDefaultsOnly, Category = "Runtime Vertex Paint and Detection Plugin|Tasks", meta = (ToolTip = "This is the Max Amount of Tasks a Mesh can have queued up, any new Tasks will Declined. Don't recommend changing this too much since if the Task Queue grows to large, performance can be affected. \nRecommend making sure you don't add more Tasks than you need, for instance adding the new task when the current task is finished instead of on tick, or check in tick if the runtime paint component doesn't have any active ones and only add one then. If the queue grows too large, paint jobs will feel unresponsive as well since it may take a while before it reaches the latest added task. "))
		int MaxAmountOfAllowedTasksPerMesh = 15;

	UPROPERTY(config, EditDefaultsOnly, Category = "Runtime Vertex Paint and Detection Plugin|Tasks", meta = (ToolTip = "How many Tasks that can be Active at one point for Paint and Detections. So if set to for instance 25, then 25 Paint tasks and 25 Detect Tasks can be Active at the same Time. \n0 = Infinite Tasks can be Active at one point, only recommended if you really know what you're doing and won't accidentally start 1000 tasks on one frame or something. \n\nSince you may get different performance in Editor vs Development and Shipping Builds you can specify each one. Shipping builds are greatly optimized and can handle Multithreading much more effectively than in Editor. "))
		int MaxAmountOfActiveTasksInEditor = 15;

	UPROPERTY(config, EditDefaultsOnly, Category = "Runtime Vertex Paint and Detection Plugin|Tasks", meta = (ToolTip = "How many Tasks that can be Active at one point for Paint and Detections. So if set to for instance 25, then 25 Paint tasks and 25 Detect Tasks can be Active at the same Time. \n0 = Infinite Tasks can be Active at one point, only recommended if you really know what you're doing and won't accidentally start 1000 tasks on one frame or something. \n\nSince you may get different performance in Editor vs Development and Shipping Builds you can specify each one. Shipping builds are greatly optimized and can handle Multithreading much more effectively than in Editor. "))
		int MaxAmountOfActiveTasksInDevelopmentBuild = 30;

	UPROPERTY(config, EditDefaultsOnly, Category = "Runtime Vertex Paint and Detection Plugin|Tasks", meta = (ToolTip = "How many Tasks that can be Active at one point for Paint and Detections. So if set to for instance 25, then 25 Paint tasks and 25 Detect Tasks can be Active at the same Time. \n0 = Infinite Tasks can be Active at one point, only recommended if you really know what you're doing and won't accidentally start 1000 tasks on one frame or something. \n\nSince you may get different performance in Editor vs Development and Shipping Builds you can specify each one. Shipping builds are greatly optimized and can handle Multithreading much more effectively than in Editor. "))
		int MaxAmountOfActiveTasksInShippingBuild = 50;

	UPROPERTY(config, EditDefaultsOnly, Category = "Runtime Vertex Paint and Detection Plugin|Multi Threading", meta = (ToolTip = "The Priority of the Thread Pool we're creating for Painting/Detecting. "))
	ETVertexPaintThreadPriority MultithreadPriority = ETVertexPaintThreadPriority::Highest;

	UPROPERTY(config, EditDefaultsOnly, Category = "Runtime Vertex Paint and Detection Plugin|Multi Threading", meta = (ToolTip = "Will get the Max amount of cores using FGenericPlatformMisc::NumberOfCoresIncludingHyperthreads() instead of the regular FGenericPlatformMisc::NumberOfWorkerThreadsToSpawn(). This may be useful if your game is all about the vertex painting/detection like a Tatoo game or something and you don't have any other heavy things that may require use of the threads. "))
	bool UseMaximumAmountOfCoresForMultithreading = false;


private:

#if WITH_EDITOR

	void VertexPaintDetectionSettingsChanged(UObject* Settings, FPropertyChangedEvent& PropertyChangedEvent) const;

#endif
};
