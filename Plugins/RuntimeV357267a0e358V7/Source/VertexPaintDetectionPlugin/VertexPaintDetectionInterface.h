// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Chaos/ChaosEngineInterface.h"

// Prerequisites
#include "TaskResultsPrerequisites.h"
#include "ChaosClothPhysicsPrerequisites.h"

// Paint Prerequisites
#include "SetColorsWithStringPrerequisites.h"
#include "SetMeshColorsPrerequisites.h"
#include "PaintSnippetsPrerequisites.h"
#include "PaintAtLocationPrerequisites.h"
#include "PaintEntireMeshPrerequisites.h"
#include "PaintWithinAreaPrerequisites.h"

//  Detect Prerequisites
#include "GetClosestVertexDataPrerequisites.h"
#include "GetColorsOnlyPrerequisites.h"
#include "GetColorsWithinAreaPrerequisites.h"

#include "VertexPaintDetectionInterface.generated.h"



// This class does not need to be modified.
UINTERFACE(Blueprintable, BlueprintType)
class UVertexPaintDetectionInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 *
 */
class VERTEXPAINTDETECTIONPLUGIN_API IVertexPaintDetectionInterface
{
	GENERATED_BODY()

		// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	//---------- PAINT / DETECTION ----------//

	UFUNCTION(BlueprintNativeEvent, Category = "Runtime Vertex Paint and Detection Plugin|Detection", meta = (ToolTip = "Runs on the Actor that we ran GetClosestVertexDataOn, on if the actor in the callback settings is the default null, otherwise it will run for whatever the user has specified."))
		void GetClosestVertexDataOnActor(const FRVPDPTaskResults& TaskResultInfo, const FRVPDPGetClosestVertexDataSettings& GetClosestVertexDataSettings, const FRVPDPClosestVertexDataResults& ClosestVertexColorResult, const FRVPDPEstimatedColorAtHitLocationInfo& EstimatedColorAtHitLocationResult, const FRVPDPAverageColorInAreaInfo& AverageColorInAreaInfo, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalData);

	UFUNCTION(BlueprintNativeEvent, Category = "Runtime Vertex Paint and Detection Plugin|Detection", meta = (ToolTip = "Runs on the Actor that we ran GetAllColorsOnly on, if the actor in the callback settings is the default null, otherwise it will run for whatever the user has specified."))
		void GetAllVertexColorsOnlyOnActor(const FRVPDPTaskResults& TaskResultInfo, const FRVPDPGetColorsOnlySettings& GotAllVertexColorsWithSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalData);

	UFUNCTION(BlueprintNativeEvent, Category = "Runtime Vertex Paint and Detection Plugin|Detection", meta = (ToolTip = "Runs on the Actor that we ran Get Colors Within Area on, if the actor in the callback settings is the default null, otherwise it will run for whatever the user has specified."))
		void GetColorsWithinArea(const FRVPDPTaskResults& TaskResultInfo, const FRVPDPGetColorsWithinAreaSettings& GetColorsWithinAreaSettings, const FRVPDPWithinAreaResults& WithinAreaResults, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalData);

	UFUNCTION(BlueprintNativeEvent, Category = "Runtime Vertex Paint and Detection Plugin|Detection", meta = (ToolTip = "Runs on Objects that has Registered themselves to get Paint Task callbacks from specific Mesh Components. "))
		void DetectTaskFinishedOnRegisteredMeshComponent(const FRVPDPTaskResults& TaskResultInfo, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalData);
		

	UFUNCTION(BlueprintNativeEvent, Category = "Runtime Vertex Paint and Detection Plugin|Painting", meta = (ToolTip = "Runs on the Actor that got Painted At Location on, if the actor in the callback settings is the default null, otherwise it will run for whatever the user has specified."))
		void PaintedOnActor_AtLocation(const FRVPDPTaskResults& TaskResultInfo, const FRVPDPPaintTaskResultInfo& PaintTaskResultInfo, const FRVPDPPaintAtLocationSettings& PaintedAtLocationSettings, const FRVPDPClosestVertexDataResults& ClosestVertexColorResult, const FRVPDPEstimatedColorAtHitLocationInfo& EstimatedColorAtHitLocationResult, const FRVPDPAverageColorInAreaInfo& AverageColorInAreaInfo, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalData);

	UFUNCTION(BlueprintNativeEvent, Category = "Runtime Vertex Paint and Detection Plugin|Painting", meta = (ToolTip = "Runs on the Actor that got Painted Within Area on, if the actor in the callback settings is the default null, otherwise it will run for whatever the user has specified."))
		void PaintedOnActor_WithinArea(const FRVPDPTaskResults& TaskResultInfo, const FRVPDPPaintTaskResultInfo& PaintTaskResultInfo, const FRVPDPPaintWithinAreaSettings& PaintedWithinAreaSettings, const FRVPDPWithinAreaResults& WithinAreaResults, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalData);

	UFUNCTION(BlueprintNativeEvent, Category = "Runtime Vertex Paint and Detection Plugin|Painting", meta = (ToolTip = "Runs on the Actor that got Painted Entire Mesh on, if the actor in the callback settings is the default null, otherwise it will run for whatever the user has specified."))
		void PaintedOnActor_EntireMesh(const FRVPDPTaskResults& TaskResultInfo, const FRVPDPPaintTaskResultInfo& PaintTaskResultInfo, const FRVPDPPaintOnEntireMeshSettings& PaintedEntireMeshSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalData);

	UFUNCTION(BlueprintNativeEvent, Category = "Runtime Vertex Paint and Detection Plugin|Painting", meta = (ToolTip = "Runs on the Actor that got Painted Color Snippet on, if the actor in the callback settings is the default null, otherwise it will run for whatever the user has specified."))
		void PaintedOnActor_PaintColorSnippet(const FRVPDPTaskResults& TaskResultInfo, const FRVPDPPaintTaskResultInfo& PaintTaskResultInfo, const FRVPDPPaintColorSnippetSettings& PaintedColorSnippetSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalData);

	UFUNCTION(BlueprintNativeEvent, Category = "Runtime Vertex Paint and Detection Plugin|Painting", meta = (ToolTip = "Runs on the Actor that got run with Set Mesh Component Vertex Colors, if the actor in the callback settings is the default null, otherwise it will run for whatever the user has specified."))
		void PaintedOnActor_SetMeshComponentVertexColors(const FRVPDPTaskResults& TaskResultInfo, const FRVPDPPaintTaskResultInfo& PaintTaskResultInfo, const FRVPDPSetVertexColorsSettings& SetMeshComponentVertexColorSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalData);

	UFUNCTION(BlueprintNativeEvent, Category = "Runtime Vertex Paint and Detection Plugin|Painting", meta = (ToolTip = "Runs on the Actor that got run with Set Mesh Component Vertex Colors Using Serialized String, if the actor in the callback settings is the default null, otherwise it will run for whatever the user has specified."))
		void PaintedOnActor_SetMeshComponentVertexColorsUsingSerializedString(const FRVPDPTaskResults& TaskResultInfo, const FRVPDPPaintTaskResultInfo& PaintTaskResultInfo, const FRVPDPSetVertexColorsUsingSerializedStringSettings& SetMeshComponentVertexColorUsingSerializedStringSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalData);


	UFUNCTION(BlueprintNativeEvent, Category = "Runtime Vertex Paint and Detection Plugin|Painting", meta = (ToolTip = "Runs on the Actor that got Colors Applied on it, if the actor in the callback settings is the default null, otherwise it will run for whatever the user has specified."))
		void ColorsAppliedOnActor(const FRVPDPTaskResults& TaskResultInfo, const FRVPDPPaintTaskResultInfo& PaintTaskResultInfo, const FRVPDPPaintTaskSettings& PaintedOnActorSettings, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalData);

	UFUNCTION(BlueprintNativeEvent, Category = "Runtime Vertex Paint and Detection Plugin|Painting", meta = (ToolTip = "Runs on the Actor that Owns the Associated Paint Component, i.e. the one that instigated the paint job. NOTE That if the task is started using multithreading, then this will execute on another thread, so recommend avoiding things like debug symbols. \nIf you return shouldApplyUpdatedVertexColor false then we won't use the UpdatedVertexColorToApply from code, or the vertexColorsToApply we return here. However if it's true then it will update it with UpdatedVertexColorToApply from code, but if OverrideVertexColorsToApply is True, then VertexColorsToOverrideWith will be used instead. "))
		void OverrideVertexColorToApply(int OverrideID, UVertexPaintDetectionComponent* AssociatedPaintComponent, UPrimitiveComponent* MeshApplyingColorsTo, int CurrentLOD, int CurrentVertexIndex, UMaterialInterface* MaterialVertexIsOn, bool IsVertexOnCloth, const FName& BoneVertexIsOn, const FVector& VertexPositionInWorldSpace, const FVector& VertexNormal, const FColor& CurrentVertexColor, EPhysicalSurface CurrentVertexMostDominantPhysicsSurface, float CurrentVertexMostDominantPhysicsSurfaceValue, const FColor& UpdatedVertexColorToApply, bool WantsToApplyUpdatedVertexColor, bool& ApplyUpdatedOrOverridenVertexColors, bool& OverrideVertexColorsToApply, FColor& VertexColorsToOverrideWith);


	UFUNCTION(BlueprintNativeEvent, Category = "Runtime Vertex Paint and Detection Plugin|Painting", meta = (ToolTip = "Runs on Objects that has Registered themselves to get Paint Task callbacks from specific Mesh Components. "))
		void PaintTaskFinishedOnRegisteredMeshComponent(const FRVPDPTaskResults& TaskResultInfo, const FRVPDPPaintTaskResultInfo& PaintTaskResultInfo, const FRVPDPAdditionalDataToPassThroughInfo& AdditionalData);


	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Utilities", meta = (ToolTip = "Highly useful so you can tell things that may paint the actor what specific callback settings it may need. For example if this is a Boss or something that does checks if any of it's Limbs have been painted with ice or something, so it needs the Colors of Each Bone, then you can use this interface to return true on that. So when we do our paint jobs and the Tasks AutoSetRequiredCallbackSettings is True, we will be able to get whatever settings the Target Mesh may need. You can ofc. also manually call this to get the settings, or call GetMeshRequiredCallbackSettings which will run this. "))
	FRVPDPTaskCallbackSettings RequiredCallbackSettings(UPrimitiveComponent* MeshComponent);


		//---------- CLOTH ----------//

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Runtime Vertex Paint and Detection Plugin|Cloth", meta = (ToolTip = "Returns Cloths and whatever Physics settings they should have depending on RGBA Vertex Colors. For instance if Blue is painted 1, what the physics settings should be for the cloth then. "))
		TMap<UClothingAssetBase*, FRVPDPVertexChannelsChaosClothPhysicsSettings> GetSkeletalMeshClothPhysicsSettings(USkeletalMeshComponent* SkeletalMeshComponentTryingToGetClothSettingsFor);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Runtime Vertex Paint and Detection Plugin|Cloth", meta = (ToolTip = "Runs on the Actor that Begin Overlapped Cloth. "))
		void ClothBeginOverlappingMesh(class UPrimitiveComponent* OverlappedComponent, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 item);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Runtime Vertex Paint and Detection Plugin|Cloth", meta = (ToolTip = "Runs on the Actor that End Overlapped Cloth."))
		void ClothEndOverlappingMesh(class UPrimitiveComponent* OverlappedComponent, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 item);
};
