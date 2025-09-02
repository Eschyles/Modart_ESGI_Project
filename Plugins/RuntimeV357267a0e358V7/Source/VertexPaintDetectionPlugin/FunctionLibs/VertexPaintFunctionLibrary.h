// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

// Engine
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Runtime/Launch/Resources/Version.h"
#include "GameplayTagContainer.h"
#include "Components/StaticMeshComponent.h"

// Prerequisites
#include "CalculateColorsPrerequisites.h"
#include "TaskResultsPrerequisites.h"
#include "ChaosClothPhysicsPrerequisites.h"
#include "VertexPaintMaterialDataAsset.h"
#include "ColorSnippetPrerequisites.h"
#include "FundementalsPrerequisites.h"

#if ENGINE_MAJOR_VERSION == 5

#include "GeometryScript/GeometryScriptTypes.h"
#include "GeometryScript/MeshVertexColorFunctions.h"

#if ENGINE_MINOR_VERSION >= 4
#include "Engine/HitResult.h"
#endif

#endif

#include "VertexPaintFunctionLibrary.generated.h"


class UVertexPaintDetectionTaskQueue;
class UVertexPaintDetectionGISubSystem;
class UDynamicMeshComponent;
class UGeometryCollectionComponent;



//-------------------------------------------------------

// Propogate To LODs Info

USTRUCT(BlueprintType)
struct FRVPDPPropogateToLODsInfo {

	GENERATED_BODY()

	// So key is LOD Vertex, value is the LOD0 Vertex it thinks it's best for it, i.e the color the LOD Vertex wants  
	UPROPERTY()
	TMap<int32, int32> LODVerticesAssociatedLOD0Vertex;
};

struct FExtendedPaintedVertex : public FPaintedVertex {

	int32 VertexIndex = -1;
};


//-------------------------------------------------------

// Trace For Closest Unique Meshes And Bones Prerequisite

USTRUCT(BlueprintType, meta = (DisplayName = "Trace For Closest Unique Meshes And Bones Prerequisite"))
struct FRVPDPTraceForClosestUniqueMeshesAndBonesPrerequisite {

	GENERATED_BODY()


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	TArray<FName> MeshBonesWithinTrace;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	FHitResult ClosestHitResultOfMesh;
};


UCLASS()
class VERTEXPAINTDETECTIONPLUGIN_API UVertexPaintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()


public:

	//---------- MATERIAL DATA ASSET WRAPPERS ----------//

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin|Material", meta = (WorldContext = "WorldContextObject", DisplayName = "Get Vertex Paint Material Interface", ToolTip = "Gets All Physics Surfaces that are Registered to the Material, or it's Parent Material if it's an instance that isn't registered but the Parent is. "))
	static TMap<TSoftObjectPtr<UMaterialInterface>, FRVPDPRegisteredMaterialSetting> GetVertexPaintMaterialInterface_Wrapper(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin|Material", meta = (WorldContext = "WorldContextObject", DisplayName = "Is Material Added To Paint On Material Data Asset", ToolTip = "If the Material is added in the paint on Material TMap. Soft UMaterialInterface Ptr so it can be used with UMaterialInterface directly gotten from GetSoftPointerObjectsOfClass without resolving and casting, i.e. bringing them into memory. "))
	static bool IsMaterialAddedToPaintOnMaterialDataAsset_Wrapper(const UObject* WorldContextObject, TSoftObjectPtr<UMaterialInterface> Material);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Material", meta = (WorldContext = "WorldContextObject", DisplayName = "Get Parents Of Physics Surface", ToolTip = "Checks if PhysicalSurface is either a Parent itself, or a Child of a Family of Physics Surfaces. PhysicalSurface to check can't be Default. "))
	static TArray<TEnumAsByte<EPhysicalSurface>> GetParentsOfPhysicsSurface_Wrapper(const UObject* WorldContextObject, TEnumAsByte<EPhysicalSurface> PhysicalSurface);

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin|Material", meta = (WorldContext = "WorldContextObject", DisplayName = "Get Physics Surface Families", ToolTip = "Gets the Physics Surfaces Family Tree, the Parents and which children that 'inherit' from them. "))
	static TMap<TEnumAsByte<EPhysicalSurface>, FRVPDPRegisteredPhysicsSurfacesSettings> GetPhysicsSurfaceFamilies_Wrapper(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin|Material", meta = (WorldContext = "WorldContextObject", ToolTip = "Checks if Physics Surface we passes in, has been registered under a physics surface family, or if it is the parent of the family itself. "))
	static bool DoesPhysicsSurfaceBelongToPhysicsSurfaceFamily(const UObject* WorldContextObject, TEnumAsByte<EPhysicalSurface> PhysicsSurface, TEnumAsByte<EPhysicalSurface> ParentOfPhysicsSurfaceFamily);

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin|Material", meta = (WorldContext = "WorldContextObject", ToolTip = "If Physics Surface sent in is a Registered Physics Surface Child, then returns the first parent it can find (optimally they should only be registered to 1 parent). If it's not a child then it will just return whatever was sent in. "))
	static TEnumAsByte<EPhysicalSurface> GetFirstParentOfPhysicsSurface(const UObject* WorldContextObject, TEnumAsByte<EPhysicalSurface> PhysicsSurface);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Material", meta = (WorldContext = "WorldContextObject", ToolTip = "Gets the Channel the Physics Surface, or it's Parent or It's child is registered to. So if you for instance use this with Cobble-Sand, which isn't registered, but it's parent Sand is, then it can still return whatever channel that is on. This is useful when you only know a Physical Surface and Material, and want to apply vertex colors to a mesh but are unsure of on which channels. "))
	static void GetChannelsPhysicsSurfaceIsRegisteredTo(const UObject* WorldContextObject, UMaterialInterface* MaterialToApplyColorsTo, const TEnumAsByte<EPhysicalSurface>& PhysicalSurface, bool& AtRedChannel, bool& AtGreenChannel, bool& AtBlueChannel, bool& AtAlphaChannel);



	//---------- COLOR SNIPPETS ----------//

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Color Snippets", meta = (BlueprintInternalUseOnly = "true", ToolTip = "Gets all the color snippets on a stored mesh and the associated color snippet data asset they are stored on. Can be used if you for example want to get all available snippets of a zombie and randomize which to use when spawning or similar."))
	static void GetAllMeshColorSnippetsAsString_Wrapper(UPrimitiveComponent* MeshComponent, TMap<FString, FRVPDPStoredColorSnippetInfo>& AvailableColorSnippetsAndDataAssets);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Color Snippets", meta = (BlueprintInternalUseOnly = "true", ToolTip = "Gets all the color snippets tags on a stored mesh and the associated color snippet data asset they are stored on. Can be used if you for example want to get all available snippets of a zombie and randomize which to use when spawning so you can always spawn a unique one or similar."))
	static void GetAllMeshColorSnippetsAsTags_Wrapper(UPrimitiveComponent* MeshComponent, TMap<FGameplayTag, TSoftObjectPtr<UVertexPaintColorSnippetDataAsset>>& AvailableColorSnippetTagsAndDataAssets);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Color Snippets", meta = (BlueprintInternalUseOnly = "true", ToolTip = "Gets all the color snippets tags on a stored mesh Under a Tag Category, and the associated Color Snippet Data Asset they are stored on. Can be used if you for example want to get All Available Snippets of a Zombie for a Certain Area"))
	static void GetAllMeshColorSnippetsTagsUnderTagCategory_Wrapper(UPrimitiveComponent* MeshComponent, FGameplayTag TagCategory, TMap<FGameplayTag, TSoftObjectPtr<UVertexPaintColorSnippetDataAsset>>& AvailableColorSnippetTagsAndDataAssetsUnderTagCategory);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Color Snippets", meta = (BlueprintInternalUseOnly = "true", ToolTip = "Gets all the color snippets tags on a stored mesh In a Tag Category, i.e. the direct children of the category and not tags under those, and the associated Color Snippet Data Asset they are stored on. \nFor instance if you have Enemy tags like ColorSnippet.Zombie, ColorSnippet.Zombie.Bloody, ColorSnippet.Zombie.Sewers.Filthy. If you would then run this with ColorSnippet.Zombie, you would get ColorSnippet.Zombie.Bloody and others directly under ColorSnippet.Zombie., and not .Sewers.Filthy. "))
	static void GetMeshColorSnippetsTagsInTagCategory_Wrapper(UPrimitiveComponent* MeshComponent, FGameplayTag TagCategory, TMap<FGameplayTag, TSoftObjectPtr<UVertexPaintColorSnippetDataAsset>>& AvailableColorSnippetTagsAndDataAssetsUnderTagCategory);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Color Snippets", meta = (WorldContext = "WorldContextObject", ToolTip = "Gets all Color Snippet under the Tag Group Category. For instance if you have ColorSnippets.Test.TestSnippet1. Then if you run this with Test, then you would get TestSnipped1 and any other under Test.. "))
	static TArray<FString> GetAllColorSnippetsUnderGroupSnippetAsString(const UObject* WorldContextObject, const FString& GroupSnippetID);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Color Snippets", meta = (ToolTip = "Finds the Mesh's Specific Child Tag from the Group Snippet, if the Mesh has correct source mesh, relative location and rotation to the group so we can find it. Requires us to send in all of the Group Snippet Meshes so we can accurately get the Tag for the specific MeshComponent. "))
	static FGameplayTag GetMeshColorSnippetChildFromGroupSnippet(UPrimitiveComponent* MeshComponent, const FGameplayTag& GroupSnippet, TArray<UPrimitiveComponent*> GroupSnippetMeshes, bool& Success);

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin|Color Snippets", meta = (WorldContext = "WorldContextObject", ToolTip = "Gets the Amount of Child snippets that is a part of a group snippet. Useful if you're running Compare against child snippets of a group then want to divide the total result with the amount of child snippets that is a part of a group. "))
	static int GetAmountOfColorSnippetChildsFromGroupSnippet(const UObject* WorldContextObject, FGameplayTag GroupSnippet);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Color Snippets", meta = (WorldContext = "WorldContextObject", ToolTip = "Will look up which group snippet the child snippet belongs to and return that. "))
	static bool GetTheGroupSnippetAChildSnippetBelongsTo(const UObject* WorldContextObject, FGameplayTag childSnippet, FGameplayTag& groupSnippetChildBelongsTo);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Color Snippets", meta = (Latent, LatentInfo = "LatentInfo", WorldContext = "WorldContextObject", ToolTip = "Getter if you just want to get the Vertex Colors of a Color Snippet. "))
	static void GetColorSnippetVertexColorsAsync(UObject* WorldContextObject, FGameplayTag ColorSnippetTag, FLatentActionInfo LatentInfo, TSoftObjectPtr<UObject>& ObjectSnippetIsAssociatedWith, TArray<FColor>& ColorSnippetVertexColors, bool& Success);

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin|Color Snippets", meta = (WorldContext = "OptionalWorldContextObject", ToolTip = "If async thread then needs a valid world context object so we can get the cached one from game instance subsystems, otherwise if in game thread it can be null since we can also load it from the settings"))
	static UVertexPaintColorSnippetRefs* GetColorSnippetReferenceDataAsset(const UObject* OptionalWorldContextObject);



	//---------- UTILITIES ----------//

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin|Utilities", meta = (ToolTip = "This is useful if you're running Paint at Locations with very quick Motions, where the distance between the locations can be greater than the Area of Effect, for example if Painting a Canvas, then you can Fill in the Area between those locations using Paint Within Area using a Box Collision. With this you can get the information you want for that box collision, or if you just want to run a trace to get what's in between. "))
	static void GetFillAreaBetweenTwoLocationsInfo(const FVector& FromLocation, const FVector& ToLocation, float DesiredBoxThickness, FVector& BoxExtent, FVector& MiddlePointBetweenLocations, FRotator& RotationFromAndToLocation);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Utilities", meta = (ToolTip = "Blueprint Getter so you can get information about the added collisions of a static mesh component, may be useful if you have for instance a Sword and you want to get specifically where the Collision of the blade is and it's extents. "))
	static bool GetStaticMeshBodyCollisionInfo(UPrimitiveComponent* StaticMeshComponent, TArray<FVector>& BoxCenters, TArray<FVector>& BoxExtents, TArray<FRotator>& BoxRotations, TArray<FVector>& SphereCenters, TArray<float>& SphereRadiuses, TArray<FVector>& CapsuleCenters, TArray<FRotator>& CapsuleRotations, TArray<float>& CapsuleRadiuses, TArray<float>& CapsuleLengths);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Utilities", meta = (Latent, LatentInfo = "LatentInfo", WorldContext = "WorldContextObject", ToolTip = "Takes in a array of soft objects and loads them async. \nAsset Load Priority 0 is the Default Priority, 100 is High Priority. "))
	static void AsyncLoadAssets(UObject* WorldContextObject, FLatentActionInfo LatentInfo, TArray<TSoftObjectPtr<UObject>> AssetsToLoad, bool PrintResultsToScreen, bool PrintResultsToLog, TArray<UObject*>& LoadedAssets, bool& Success, int32 AssetLoadPriority = 100);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Utilities", meta = ( WorldContext = "WorldContextObject", ToolTip = "Simply checks if it can resolve all of the objects sent in that has a valid soft ptr. "))
	static bool IsAssetsLoaded(UObject* WorldContextObject, TArray<TSoftObjectPtr<UObject>> AssetsToCheck);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Utilities", meta = (Latent, LatentInfo = "LatentInfo", WorldContext = "WorldContextObject", ToolTip = "Recommend Async Version of GetAmountOfPaintedColorsForEachChannelAsync(), which won't affect the FPS if there's alot of vertex colors to Loop Through! Calling it again before it finishes won't do anything. \nGets the Amount of vertices with colors above the min amount, gets them in Percent from 0-100, as well as the average color value of the ones above the minimum. Useful if you want to check that you've painted the majority of something. \nNOTE You can get this by running a Paint or Detect Job as well, but in some cases you may want to just run this, for instance if you've gotten colors of each bone and you want to check the % on just one bone. \nNOTE This does NOT return the amount for each physics surface, since that requires us to loop through the mesh sections etc.. If you want that, then you can simply run GetAllColorsOnly instead where you will get that in it's callback. "))
	static void GetAmountOfPaintedColorsForEachChannelAsync(UObject* WorldContextObject, FLatentActionInfo LatentInfo, const TArray<FColor>& VertexColors, FRVPDPAmountOfColorsOfEachChannelResults& AmountOfColorsOfEachChannel, float MinColorAmountToBeConsidered = 0.01f);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Utilities", meta = (DisplayName = "Get Amount of Painted Colors for Each Channel", ToolTip = "Gets the Amount of vertices with colors above the min amount, gets them in Percent from 0-100, as well as the average color value of the ones above the minimum. Useful if you want to check that you've painted the majority of something. \nNOTE You can get this by running a Paint or Detect Job as well, which is more performant as well since they can run on async thread! "))
	static FRVPDPAmountOfColorsOfEachChannelResults GetAmountOfPaintedColorsForEachChannel(const TArray<FColor>& VertexColors, float MinColorAmountToBeConsidered = 0.01f);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Utilities", meta = (ToolTip = "Just running SetMeshConstantVertexColors on Dynamic Mesh won't actually Enable Vertex Colors. This will run the SetMeshConstantVertexColors, but Enable the Vertex Colors as well with what you put in, so if you try to detect or paint afterwards, things will work as intended since we can get the correct result from FDynamicMesh3 Vertex Info. "))
	static UDynamicMesh* SetMeshConstantVertexColorsAndEnablesThem(UDynamicMesh* TargetMesh, FLinearColor Color, FGeometryScriptColorFlags Flags, bool bClearExisting, UGeometryScriptDebug* Debug);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Utilities", meta = (WorldContext = "WorldContextObject", ToolTip = "Gets All Physics Surfaces that are Registered to the Material, or it's Parent Material if it's an instance that isn't registered but the Parent is. "))
	static TArray<TEnumAsByte<EPhysicalSurface>> GetPhysicsSurfacesRegisteredToMaterial(const UObject* WorldContextObject, UMaterialInterface* Material);

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin|Utilities", meta = (ToolTip = "Checks if the World is valid, that it is a game World and is not being teared down. "))
	static bool IsWorldValid(const UWorld* World);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Utilities", meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "ActorsToIgnore, ObjectTypesToTraceFor", BlueprintInternalUseOnly = "true", ToolTip = "This function Multi Sphere Traces for Meshes Within the Radius, and make sure we get only get the Closest Hit Result of each Mesh, but also with all of the Bones within the sphere for each mesh. So what's returned is essentially ready to be used with Paint at Location. \nIf ObjectTypesToTraceFor is > 0 then we will trace with that, otherwise use the TraceChannel. \nThis is very useful since we can paint on Multiple Meshes, but without any visible Seams between them, very useful for for instance Floors! "))
	static bool MultiSphereTraceForClosestUniqueMeshesAndBones_Wrapper(const UObject* WorldContextObject, FVector Location, float Radius, ETraceTypeQuery TraceChannel, const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypesToTraceFor, const TArray<AActor*>& ActorsToIgnore, bool TraceComplex, bool IgnoreSelf, EDrawDebugTrace::Type DrawDebugType, TArray<FRVPDPTraceForClosestUniqueMeshesAndBonesPrerequisite>& ClosestUniqueMeshesWithBones, float DebugDrawTime = 0.2f);

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin|Utilities", meta = (WorldContext = "OptionalWorldContextObject", ToolTip = "If async thread then needs a valid world context object so we can get the cached one from game instance subsystems, otherwise if in game thread it can be null since we can also load it from the settings"))
	static UVertexPaintOptimizationDataAsset* GetOptimizationDataAsset(const UObject* OptionalWorldContextObject);

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin|Utilities", meta = (WorldContext = "OptionalWorldContextObject", ToolTip = "If async thread then needs a valid world context object so we can get the cached one from game instance subsystems, otherwise if in game thread it can be null since we can also load it from the settings"))
	static UVertexPaintMaterialDataAsset* GetVertexPaintMaterialDataAsset(const UObject* OptionalWorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Utilities")
	static int GetAmountOfLODsToPaintOn(UPrimitiveComponent* MeshComp, bool OverrideLODToPaintUpOn, int OverrideUpToLOD);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Utilities", meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "true", ToolTip = "Gets the Amount of Calculate Colors Paint Tasks for Mesh Components in the Queue. Mainly useful to present it on widgets etc. "))
	static TMap<UPrimitiveComponent*, int> GetCalculateColorsPaintTasksAmount_Wrapper(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Utilities", meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "true", ToolTip = "Gets the Amount of Calculate Colors Detection Tasks for Mesh Components in the Queue. Mainly useful to present it on widgets etc. "))
	static TMap<UPrimitiveComponent*, int> GetCalculateColorsDetectionTasksAmount_Wrapper(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Utilities", meta = (WorldContext = "WorldContextObject"))
	static int GetAmountOfOnGoingPaintTasks(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Utilities", meta = (WorldContext = "WorldContextObject"))
	static int GetAmountOfOnGoingDetectTasks(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Utilities", meta = (WorldContext = "WorldContextObject", ToolTip = "Gets all Tags under a Tag Category, for instance if you have a Category Zombie.SewerLevel and several tags under SewerLevel, then you will get all of those if you select SewerLevel. "))
	static TArray<FGameplayTag> GetAllTagsUnderTagCategory(const UObject* WorldContextObject, FGameplayTag TagCategory);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Utilities", meta = (BlueprintInternalUseOnly = "true", ToolTip = "Gets the colors of the component either for All LODs if that is True, or up to a specified LOD. You can use GetAllVertexColorsOnly if you want to be more specific in what you get, for instance get the colors of each bones etc. "))
	static FRVPDPVertexDataInfo GetMeshComponentVertexColors_Wrapper(UPrimitiveComponent* meshComponent, bool& Success, bool GetColorsForAllLODs, int GetColorsUpToLOD = 0);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Utilities", meta = (ToolTip = "Gets the Amount of Vertices on LOD0 for the Mesh. "))
	static int GetMeshComponentAmountOfVerticesOnLOD(UPrimitiveComponent* MeshComponent, int Lod = 0);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Utilities", meta = (BlueprintInternalUseOnly = "true", ToolTip = "Gets the colors of the component at Specified LOD, if it exist. "))
	static TArray<FColor> GetMeshComponentVertexColorsAtLOD_Wrapper(UPrimitiveComponent* MeshComponent, int Lod = 0);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Utilities", meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "true", ToolTip = "OptionalMaterialPhysicsSurfaceWasDetectedOn is used to check if the surfaces can blend  This will get loop through and get the Most Dominant Surface from the Physics Surface Array, so it and the physics Sruface Color Value array has to be in sync. This combines well with the Physics Result Structs you get from GetClosestVertexData or PaintAtLocation Callback Events. \nIf you provide the Material that the physics surfaces was on, it will also check the Blendable Surfaces that has been Registered in the Editor Widget, and check if any sent in here can Blend and become another Surface, if so, that surface could be the Most dominant one. For example if Red Channel is Sand, Blue Channel is Puddle, they can be set to blend into Mud in the Editor Widget. \n\nThis is very useful when you want to run SFX or VFX based on detected surfaces. "))
	static bool GetTheMostDominantPhysicsSurface_Wrapper(const UObject* WorldContextObject, UMaterialInterface* OptionalMaterialPhysicsSurfaceWasDetectedOn, TArray<TEnumAsByte<EPhysicalSurface>> PhysicsSurfaces, TArray<float> PhysicsSurfaceValues, TEnumAsByte<EPhysicalSurface>& MostDominantPhysicsSurfaceFromArray, float& MostDominantPhysicsSurfaceColorValue);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Utilities", meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "true", ToolTip = "Gets the Red, Green, Blue or Alpha Channel that the Physics Surface is registered to in the Editor Widget. Useful if you have a physics surface and you want to know what color channel that you should apply paint on just based on that. It can also return Default if it's set to that (i.e. what is on the Material when nothing is on it), but if it can't get the info it will not be Successful. "))
	static TArray<ESurfaceAtChannel> GetAllVertexColorChannelsPhysicsSurfaceIsRegisteredTo_Wrapper(const UObject* WorldContextObject, UMaterialInterface* Material, TEnumAsByte<EPhysicalSurface> PhysicsSurface, bool& Successful);

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin|Utilities")
	static TArray<TEnumAsByte<EPhysicalSurface>> GetAllPhysicsSurfaces();

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Utilities", meta = (ToolTip = "Draws Debug Box of the primitive components Bounds. "))
	static void DrawPrimitiveComponentBoundsBox(UPrimitiveComponent* PrimitiveComponent, float Lifetime = 5.0f, float Thickness = 3.0f, FLinearColor ColorToDraw = FLinearColor::Red);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Utilities", meta = (ToolTip = "Releases and Blocks Vertex Colors before switching Static Mesh. Can choose to Clear the Vertex Colors of the newly switched mesh.  "))
	static void SetStaticMeshAndReleaseResources(UStaticMeshComponent* StaticMeshComponent, UStaticMesh* NewMesh, bool ClearVertexColorsOfChangedMesh = true);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Utilities", meta = (DisplayName = "Set Skeletal Mesh And Release Resources", ToolTip = "Unlike just running SetSkeletalMesh(), this doesn't cause a crash when switching meshes if applying vertex colors on them since it runs ReleasesResurces first and BeginOverrideVertexColors etc., then calls the SetSkeletalMesh(). "))
	static void SetSkeletalMeshAndReleaseResources(USkeletalMeshComponent* SkeletalMeshComponent, USkeletalMesh* NewMesh, bool ClearVertexColorsOfChangedMesh = true);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Utilities", meta = (DisplayName = "Sort String Array Alphabetically", ToolTip = "SortStringArrayAlphabetically"))
	static TArray<FString> VertexPaintDetectionPlugin_SortStringArrayAlphabetically(TArray<FString> Strings);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Utilities", meta = (DisplayName = "SortAssetsNamesAlphabetically", ToolTip = "Sorts the TMap Alphabetically while matching the index. So you can send in a TMap of AssetNames and Indexes, and get a TMap back but with them in alphabetical order. Intended to be used with the Asset Registry->GetAssets...() functions, see example in the Vertex Paint Editor Utility Widget"))
	static TMap<int, FString> VertexPaintDetectionPlugin_SortAssetsNamesAlphabetically(TMap<int, FString> AssetIndexAndName);

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin|Utilities", meta = (DisplayName = "GetSkeletalMesh", ToolTip = "Utility function since 5.1 don't use the same get skeletal mesh in BP as 4.27 and 5.0. By using this we don't have to update the examples or Widget BP"))
	static USkeletalMesh* VertexPaintDetectionPlugin_GetSkeletalMesh(USkeletalMeshComponent* SkeletalMeshComp);

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin|Utilities")
	static EObjectTypeQuery CollisionChannelToObjectType(ECollisionChannel CollisionChannel);

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin|Utilities")
	static ECollisionChannel ObjectTypeToCollisionChannel(EObjectTypeQuery ObjectType);

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin|Utilities", meta = (WorldContext = "WorldContextObject"))
	static bool IsPlayInEditor(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Utilities", meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "true", ToolTip = "Checks if any of the Cached Physical Materials Assets in the Game Instance Subsystem uses the physics surface we pass through as paramater here, then returns the one it finds. We cache all of them since it's very expensive to run GetAssetsOfClass. \nIf PhysicsMaterialClasses is set to be something other than the Base PhysicsMaterialClass, like a custom class, then we will make a check to make sure it is that class. \nMake sure only one physics material uses a physics surface, otherwise you might get the wrong physics material. \nAlso make sure that you add the folder that has all the physics materials to Additional Directories to be Cooked in the Project Settings, in case there's a risk any of them isn't referenced by anything and won't get cooked, but has a physics surface that is registered in the editor widget. ", DeterminesOutputType = "PhysicalMaterialClass"))
	static UPhysicalMaterial* GetPhysicalMaterialUsingPhysicsSurface_Wrapper(const UObject* WorldContextObject, TSubclassOf<UPhysicalMaterial> PhysicalMaterialClass, TEnumAsByte<EPhysicalSurface> PhysicsSurface);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Utilities", meta = (ToolTip = "Removes a Mesh Component from Queue if it has any tasks queued up. Current and Future Task will fail so use with caution. "))
	static void RemoveComponentFromPaintTaskQueue(UPrimitiveComponent* Component);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Utilities", meta = (ToolTip = "Removes a Mesh Component from Queue if it has any tasks queued up. Current and Future Task will fail so use with caution. "))
	static void RemoveComponentFromDetectTaskQueue(UPrimitiveComponent* Component);

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin|Utilities")
	static const UObject* GetMeshComponentSourceMesh(UPrimitiveComponent* MeshComponent);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Utilities", meta = (DisplayName = "CalcAABBWithoutUniformCheck", ToolTip = "The Engines own CalcAABB has a requirement that the scale has to be uniform for some reason, so it doesn't work if for instance the characters scale is 1, 1, 0.5. This is the same logic but without the uniform check, and works as intended with our Paint Within Area checks. "))
	static FBox CalcAABBWithoutUniformCheck(const class USkinnedMeshComponent* MeshComponent, const FTransform& LocalToWorld);

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin|Utilities")
	static FString GetSubstringAfterLastCharacter(const FString& String, const FString& Character);

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin|Utilities", meta = (ToolTip = "The Engines own ToFColor and ToFLinear functions isn't 100%, i've had issues where it hadn't returned the correct amount, for instance a FColor Red channel that was 191, which should convert to 0.75 but got converted to 0.5 etc. So made own wrappers for Linear to FColor and vice versa. "))
	static FColor ReliableFLinearToFColor(FLinearColor LinearColor);

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin|Utilities", meta = (ToolTip = "The Engines own ToFColor and ToFLinear functions isn't 100%, i've had issues where it hadn't returned the correct amount, for instance a FColor Red channel that was 191, which should convert to 0.75 but got converted to 0.5 etc. So made own wrappers for Linear to FColor and vice versa. "))
	static FLinearColor ReliableFColorToFLinearColor(FColor Color);

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin|Utilities", meta = (ToolTip = "Returns a Box, Sphere, Static or Skeletal Mesh's Component Bounds Top World Z.  "))
	static float GetComponentBoundsTopWorldZ(UPrimitiveComponent* Component);

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin|Utilities", meta = (ToolTip = "Returns a Box, Sphere, Static or Skeletal Mesh's Component Bounds Bottom World Z.  "))
	static float GetComponentBoundsBottomWorldZ(UPrimitiveComponent* Component);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Utilities", meta = (ToolTip = "So whenever a Paint Task is finished for the MeshComponent, the objectToRegister can get an interface callback of it. Useful if you for instance has some kind of centralized weather manager or game mode manager that needs to get the latest data of mesh components. ", DisplayName = "Register Paint Task Callbacks To Object From Specified Mesh Component", DefaultToSelf = "ObjectToRegisterForCallbacks"))
	static void RegisterPaintTaskCallbacksToObjectFromSpecifiedMeshComponent_Wrapper(UPrimitiveComponent* MeshComponent, UObject* ObjectToRegisterForCallbacks);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Utilities", meta = (ToolTip = "So whenever a Paint Task is finished for the MeshComponent, the objectToRegister can get an interface callback of it. Useful if you for instance has some kind of centralized weather manager or game mode manager that needs to get the latest data of mesh components. ", DisplayName = "Un-Register Paint Task Callbacks To Object From Specified Mesh Component", DefaultToSelf = "objectToUnregisterForCallbacks"))
	static void UnRegisterPaintTaskCallbacksToObjectFromSpecifiedMeshComponent_Wrapper(UPrimitiveComponent* MeshComponent, UObject* objectToUnregisterForCallbacks);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Utilities", meta = (ToolTip = "So whenever a Paint Task is finished for the MeshComponent, the objectToRegister can get an interface callback of it. Useful if you for instance has some kind of centralized weather manager or game mode manager that needs to get the latest data of mesh components. ", DisplayName = "Register Detect Task Callbacks To Object From Specified Mesh Component", DefaultToSelf = "ObjectToRegisterForCallbacks"))
	static void RegisterDetectTaskCallbacksToObjectFromSpecifiedMeshComponent_Wrapper(UPrimitiveComponent* MeshComponent, UObject* ObjectToRegisterForCallbacks);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Utilities", meta = (ToolTip = "So whenever a Paint Task is finished for the MeshComponent, the objectToRegister can get an interface callback of it. Useful if you for instance has some kind of centralized weather manager or game mode manager that needs to get the latest data of mesh components. ", DisplayName = "Un-Register Detect Task Callbacks To Object From Specified Mesh Component", DefaultToSelf = "ObjectToUnregisterForCallbacks"))
	static void UnRegisterDetectTaskCallbacksToObjectFromSpecifiedMeshComponent_Wrapper(UPrimitiveComponent* MeshComponent, UObject* ObjectToUnregisterForCallbacks);

	static bool ShouldTaskLoopThroughAllVerticesOnLOD(const FRVPDPTaskCallbackSettings& CallbackSettings, UPrimitiveComponent* Component, const FRVPDPOverrideColorsToApplySettings& OverrideColorsToApplySettings);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Utilities", meta = (WorldContext = "WorldContextObject"))
	static FRVPDPPhysicsSurfaceDataInfo GetVertexColorPhysicsSurfaceDataFromMaterial(const UObject* WorldContextObject, const FColor& VertexColor, UMaterialInterface* Material);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Utilities", meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "ActorsToIgnore", ToolTip = "Wrapper for MultiConeTraceForObjects() that gets the trace information it needs from a ConeMesh reference, then runs the MultiConeTraceForObjects, which first performs a Sphere Trace, then checks the Hits it got if they're within the correct angle and height. \nRequires that the Cone Mesh is a perfectly shaped Cone!"))
	static bool MultiConeTraceForObjectsUsingConeMesh(TArray<FHitResult>& OutHits, UPrimitiveComponent* ConeMesh, const TArray<TEnumAsByte<EObjectTypeQuery> >& ObjectTypes, const TArray<AActor*>& ActorsToIgnore, bool DebugTrace = false, float DebugTraceDuration = 2);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Utilities", meta = (AutoCreateRefTerm = "ActorsToIgnore", ToolTip = "First performs a Sphere Trace, then checks the Hits it got if they're within the correct angle and height. "))
	static bool MultiConeTraceForObjects(TArray<FHitResult>& OutHits, const UObject* WorldContextObject, const FVector& ConeOrigin, const FVector& ConeDirection, float ConeHeight, float ConeRadius, const TArray<TEnumAsByte<EObjectTypeQuery> >& ObjectTypes, const TArray<AActor*>& ActorsToIgnore, bool DebugTrace = false, float DebugTraceDuration = 2);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Utilities", meta = (ToolTip = "Checks if the Mesh is Being Painted by an Auto Paint Component, or if it's Owner returns something with RequiredCallbackSettings interface, or if something Else binds to callback events for the Mesh and they have an auto paint comp etc. that is painting the mesh. We automatically call this if a Tasks AutoSetRequiredCallbackSettings is True, as it's very useful if users are unsure what the Actor/Mesh you're painting on may require as callback settings. \nReturns True if it got some callback settings that isn't the default one. "))
	static bool GetMeshRequiredCallbackSettings(UPrimitiveComponent* MeshComponent, bool& MeshPaintedByAutoPaintComponent, FRVPDPTaskCallbackSettings& RequiredCallbackSettings);

	static bool IsThereAnyPaintConditions(const FRVPDPApplyColorSettings& ApplyColorSettings, bool& IsOnPhysicsSurface, bool& IsOnRedChannel, bool& IsOnGreenChannel, bool& IsOnBlueChannel, bool& IsOnAlphaChannel, int& AmountOfVertexNormalWithinDotToDirectionConditions, int& AmountOfOnlyAffectVerticesWithDirectionToActorOrLocationConditions, int& AmountOfOnlyAffectVerticesWithinDirectionFromActorOrLocation, int& AmountOfIfVertexIsAboveOrBelowWorldZ, int& AmountOfIfVertexColorWithinRange, int& AmountOfIfVertexHasLineOfSightTo, int& AmountOfIfVertexIsOnBone, int& AmountOfIfVertexIsOnMaterial);

	static FString GetPhysicsSurfaceAsString(TEnumAsByte<EPhysicalSurface> PhysicsSurface) { return *StaticEnum<EPhysicalSurface>()->GetDisplayNameTextByValue(PhysicsSurface.GetValue()).ToString(); }

	static FRVPDPCallbackFromSpecifiedMeshComponentsInfo GetRegisteredPaintTaskCallbacksObjectsForSpecificMeshComponent(const UObject* WorldContextObject, const UPrimitiveComponent* MeshComponent);

	static FRVPDPCallbackFromSpecifiedMeshComponentsInfo GetRegisteredDetectTaskCallbacksObjectsForSpecificMeshComponent(const UObject* WorldContextObject, const UPrimitiveComponent* MeshComponent);

	static void PropagateLOD0VertexColorsToLODs(const TArray<FExtendedPaintedVertex>& Lod0PaintedVerts, FRVPDPVertexDataInfo& VertexDetectMeshData, const TArray<FBox>& LodBaseBounds, const TArray<FColorVertexBuffer*>& ColorVertexBufferToUpdate, bool ReturnPropogateToLODInfo, TMap<int32, FRVPDPPropogateToLODsInfo>& PropogateToLODInfo);

	static UVertexPaintDetectionGISubSystem* GetVertexPaintGameInstanceSubsystem(const UObject* WorldContextObject);

	static TArray<FColor> GetSkeletalMeshVertexColorsAtLOD(USkeletalMeshComponent* SkeletalMeshComponent, int Lod);

	static TArray<FColor> GetStaticMeshVertexColorsAtLOD(UStaticMeshComponent* StaticMeshComponent, int Lod);

	static UVertexPaintDetectionTaskQueue* GetVertexPaintTaskQueue(const UObject* WorldContextObject);

	static FRVPDPAmountOfColorsOfEachChannelResults ConsolidateColorsOfEachChannel(FRVPDPAmountOfColorsOfEachChannelResults AmountOfColorsOfEachChannel, int AmountOfVertices);

	static FRVPDPAmountOfColorsOfEachChannelResults ConsolidatePhysicsSurfaceResult(FRVPDPAmountOfColorsOfEachChannelResults AmountOfColorsOfEachChannel, int AmountOfVertices);

	static bool DoesPaintTaskQueueContainID(const UObject* WorldContextObject, int32 TaskID);

	static bool DoesDetectTaskQueueContainID(const UObject* WorldContextObject, int32 TaskID);


	static void RunFinishedPaintTaskCallbacks(const FRVPDPTaskResults& TaskResult, const FRVPDPPaintTaskResultInfo& PaintTaskResult, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough);

	static void RunFinishedDetectTaskCallbacks(const FRVPDPTaskResults& DetectTaskResult, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough);

	static void RunGetClosestVertexDataCallbackInterfaces(const FRVPDPTaskResults& DetectTaskResult, const FRVPDPGetClosestVertexDataSettings& GetClosestVertexDataSettings, const FRVPDPClosestVertexDataResults& ClosestVertexColorResult, const FRVPDPEstimatedColorAtHitLocationInfo& EstimatedColorAtHitLocationResult, const FRVPDPAverageColorInAreaInfo& AverageColor, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough);

	static void RunGetAllColorsOnlyCallbackInterfaces(const FRVPDPTaskResults& DetectTaskResult, const FRVPDPGetColorsOnlySettings& GetAllVertexColorsSettings, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough);

	static void RunGetColorsWithinAreaCallbackInterfaces(const FRVPDPTaskResults& DetectTaskResult, const FRVPDPGetColorsWithinAreaSettings& GetColorsWithinAreaSettings, const FRVPDPWithinAreaResults& WithinAreaResults, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough);

	static void RunPaintAtLocationCallbackInterfaces(const FRVPDPTaskResults& TaskResult, const FRVPDPPaintTaskResultInfo& PaintTaskResult, const FRVPDPPaintAtLocationSettings& PaintAtLocationSettings, const FRVPDPClosestVertexDataResults& ClosestVertexColorResult, const FRVPDPEstimatedColorAtHitLocationInfo& EstimatedColorAtHitLocationResult, const FRVPDPAverageColorInAreaInfo& AverageColor, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough);

	static void RunPaintWithinAreaCallbackInterfaces(const FRVPDPTaskResults& TaskResult, const FRVPDPPaintTaskResultInfo& PaintTaskResult, const FRVPDPPaintWithinAreaSettings& PaintWithinAreaSettings, const FRVPDPWithinAreaResults& WithinAreaResults, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough);

	static void RunPaintEntireMeshCallbackInterfaces(const FRVPDPTaskResults& TaskResult, const FRVPDPPaintTaskResultInfo& PaintTaskResult, const FRVPDPPaintOnEntireMeshSettings& PaintOnEntireMeshSettings, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough);

	static void RunPaintColorSnippetCallbackInterfaces(const FRVPDPTaskResults& TaskResult, const FRVPDPPaintTaskResultInfo& PaintTaskResult, const FRVPDPPaintColorSnippetSettings& PaintColorSnippetSettings, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough);

	static void RunPaintSetMeshColorsCallbackInterfaces(const FRVPDPTaskResults& TaskResult, const FRVPDPPaintTaskResultInfo& PaintTaskResult, const FRVPDPSetVertexColorsSettings& SetVertexColorsSettings, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough);

	static void RunPaintSetMeshColorsUsingSerializedStringCallbackInterfaces(const FRVPDPTaskResults& TaskResult, const FRVPDPPaintTaskResultInfo& PaintTaskResult, const FRVPDPSetVertexColorsUsingSerializedStringSettings& SetVertexColorsUsingSerializedStringSettings, const FRVPDPTaskFundamentalSettings& TaskFundamentalSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalDataToPassThrough);


#if ENGINE_MAJOR_VERSION == 5

	static TArray<FColor> GetDynamicMeshVertexColors(UDynamicMeshComponent* DynamicMeshComponent);

	static TArray<FColor> GetGeometryCollectionVertexColors(UGeometryCollectionComponent* GeometryCollectionComponent);

#endif

public:

	//---------- CHAOS CLOTH ----------//

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin|Cloth", meta = (ToolTip = "Returns Skeletal Mesh Cloths if it ha any. Can be used in conjunction with UVertexPaintFunctionLibrary::SetChaosClothPhysics if you want to directly set physics on a cloth. Also useful when implementing interface GetSkeletalMeshClothPhysicsSettings on a BP and has to return physics setting for each Cloth the Mesh has. \NOTE Affecting Cloth Physics is a UE5 exclusive feature, and Painting on Skeletal Meshes with Proxy Simulated Cloth is not supported, for example the Wind Walker Echo Character. Only Cloth created the regular way through the editor! "))
	static TArray<UClothingAssetBase*> GetClothAssets(USkeletalMesh* SkeletalMesh);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Cloth", meta = (ToolTip = "Set Cloth Physics on the Cloth sent in for the Skeletal Mesh. Can use UVertexPaintFunctionLibrary::VertexPaintDetectionPlugin_GetClothingAssets() to get a Skeletal Meshes Clothing Assets in the order they're set in the Skeletal Mesh, so you have to be aware of which cloth is at which index. \n\NOTE Affecting Cloth Physics is a UE5 exclusive feature, and Painting on Skeletal Meshes with Proxy Simulated Cloth might not be supported, for example the Wind Walker Echo Character. Only Cloth created the regular way through the editor is Supported! "))
	static void SetChaosClothPhysics(USkeletalMeshComponent* SkeletalMeshComponent, UClothingAssetBase* ClothingAsset, const FRVPDPChaosClothPhysicsSettings& ClothPhysicsSettings);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Cloth", meta = (ToolTip = "Updates Cloth Physics with the exisisting Vertex Colors. This is run on the game thread, and if the cloths have a lot of vertice the FPS may get affected. Recommend using the Async Variant to avoid this. \nThis function is useful if Vertex Colors has been changed by something else other than a paint job, or was pre-painetd in the editor and you want them to behave correctly. \n\NOTE Affecting Cloth Physics is a UE5 exclusive feature, and Painting on Skeletal Meshes with Proxy Simulated Cloth is not supported, for example the Wind Walker Echo Character. Only Cloth created the regular way through the editor! "))
	static void UpdateChaosClothPhysicsWithExistingColors(USkeletalMeshComponent* SkeletalMeshComponent);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Cloth", meta = (Latent, LatentInfo = "LatentInfo", WorldContext = "WorldContextObject", ToolTip = "Updates Cloth Physics with the exisisting Vertex Colors. This will run on async when it is looping through the vertices of the cloths to get the average color and get the cloth physics settings for them, which is then uses when back at game thread to update the cloth physics. \nThis function is useful if Vertex Colors has been changed by something else other than a paint job, or was pre-painetd in the editor and you want them to behave correctly. \n\NOTE Affecting Cloth Physics is a UE5 exclusive feature, and Painting on Skeletal Meshes with Proxy Simulated Cloth is not supported, for example the Wind Walker Echo Character. Only Cloth created the regular way through the editor!"))
	static void UpdateChaosClothPhysicsWithExistingColorsAsync(UObject* WorldContextObject, FLatentActionInfo LatentInfo, USkeletalMeshComponent* SkeletalMesh);

#if ENGINE_MAJOR_VERSION == 5

	static TMap<UClothingAssetBase*, FRVPDPChaosClothPhysicsSettings> GetChaosClothPhysicsSettingsBasedOnExistingColors(USkeletalMeshComponent* SkeletalMeshComponent);

	static FRVPDPChaosClothPhysicsSettings GetChaosClothPhysicsSettingsBasedOnAverageVertexColor(const UClothingAssetBase* ClothingAsset, FLinearColor AverageColorOnClothingAsset, const FRVPDPVertexChannelsChaosClothPhysicsSettings& ClothPhysicsSettings);

#endif

private:

	static TArray<ESurfaceAtChannel> GetVertexColorChannelsPhysicsSurfaceIsRegisteredTo(const UObject* WorldContextObject, UMaterialInterface* Material, TEnumAsByte<EPhysicalSurface> PhysicsSurface, bool& Successful);
};
