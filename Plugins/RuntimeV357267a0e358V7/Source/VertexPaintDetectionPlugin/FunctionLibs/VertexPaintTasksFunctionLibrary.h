// Copyright 2022-2024 Alexander Floden, Alias Alex River. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

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

#include "VertexPaintTasksFunctionLibrary.generated.h"



UCLASS()
class VERTEXPAINTDETECTIONPLUGIN_API UVertexPaintTasksFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()


public:

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Detect", meta = (BlueprintInternalUseOnly = "true", ToolTip = "Gets the Closest Vertex Data on  Static, Skeletal, or Spline Meshes such as Colors, Position and Normal. It can also return All of the Colors, Positions and Normals if set to do so in the callback settings, as well as Get Average Color within an Area, and the Estimated Color at Actual Hit Location. \nTrace Complex has to be True if doing a trace onto the mesh so we can get the faceIndex for Static Meshes! \ndetectWith is the source of the hit, for example Shoe, Bullet etc. and will just be returned in the Detected Delegate. \nIf Area of Effect is higher than 0, then you can get the Average Color and Physics Surface of that Area! \nGetEstimatedColorAtHitLocationSettings will get the vertex colors as close to the hit location as possible, which is useful if meshes have few vertices. \n\nDetection Tasks will always get moved to be the first in a Task Queue, so they run as quickly as possible since they're often used for SFX and/or VFX. "))
	static void GetClosestVertexDataOnMesh_Wrapper(UVertexPaintDetectionComponent* RuntimeVertexPaintAndDetectionComponent, UPrimitiveComponent* MeshComponent, FRVPDPGetClosestVertexDataSettings GetClosestVertexDataStruct, FRVPDPAdditionalDataToPassThroughInfo AdditionalDataToPassThrough);


	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Detect", meta = (BlueprintInternalUseOnly = "true", ToolTip = "Get All Vertex Colors Only on Static, Skeletal or Spline Meshes. This is faster than GetClosestVertexDataOnMesh since it doesn't require the task to loop through the vertices, but only gets the total colors. If you change the callback settings to return the positions and/or normal, the bones and the colors (if skeletal mesh comp), or get amount of color for each channel, then it will also loop through the vertices in order to get those, but will make the task take just as long as GetClosestVertexDataOnMesh. \n\nNOTE Make sure includeVertexColorData is True in the Callback Settings if you intend to make use of the Vertex Colors when the Callback is Run!"))
	static void GetAllVertexColorsOnly_Wrapper(UVertexPaintDetectionComponent* RuntimeVertexPaintAndDetectionComponent, UPrimitiveComponent* MeshComponent, FRVPDPGetColorsOnlySettings GetAllVertexColorsStruct, FRVPDPAdditionalDataToPassThroughInfo AdditionalDataToPassThrough);


	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Detect", meta = (BlueprintInternalUseOnly = "true", ToolTip = "Get Colors Within Area on Static, Skeletal or Spline Meshes. This is similar to the Paint Within Area, except it doesn't apply any paint jobs but can Instead just Get Colors, Position, Normals and amount of color of each channel such as % etc. of the vertices just within the Area. \nThese settings can be made in the Within Area Settings. "))
	static void GetColorsWithinArea_Wrapper(UVertexPaintDetectionComponent* RuntimeVertexPaintAndDetectionComponent, UPrimitiveComponent* MeshComponent, FRVPDPGetColorsWithinAreaSettings GetColorsWithinAreaStruct, FRVPDPAdditionalDataToPassThroughInfo AdditionalDataToPassThrough);


	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Paint", meta = (BlueprintInternalUseOnly = "true", ToolTip = "Paint Vertex Colors in Runtime on Static, Skeletal or Spline Meshes. \n\nStrength to Apply should be between -1 - 1, Fall Off Strength 0-1, Area of Effect > 0, Area of Effect with Full Color > 0 if you want to have an area around the paint location to be fully painted. \n\nPaint on Color Limit can be used if you don't want to paint over a certain amount on a channel, for instance if Rain should only be able to make you wet up to a certain amount. \n\nTrace Complex has to be True if Line Tracing as it is required to get the faceIndex! \n\nAffect Cloth Physics is a UE5 Exclusive Feature!"))
	static void PaintOnMeshAtLocation_Wrapper(UVertexPaintDetectionComponent* RuntimeVertexPaintAndDetectionComponent, UPrimitiveComponent* MeshComponent, FRVPDPPaintAtLocationSettings PaintAtLocationStruct, FRVPDPAdditionalDataToPassThroughInfo AdditionalDataToPassThrough);


	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Paint", meta = (BlueprintInternalUseOnly = "true", ToolTip = "Paint Vertex Colors on a Static, Skeletal or Spline Meshes on what is Within the componentToCheckIfIsWithins Area. \n\nStrength to Apply should be between -1 - 1, Fall Off Strength 0-1, Area of Effect with Full Color > 0 if you want an area in the component location to have full color. \n\nRecommended to use isSphereShape, or isSquareOrRectangleShape such as a big rectangle for for example a body of water, as they are the faster Tasks. \n\nisComplexShape is slower since for every vertex on the component that is within the bounds of componentToCheckIfIsWithin, we line trace to see if it hits componentToCheckIfIsWithin collison. Should only be used if you really need a mesh with complex shapes, like if you paint a pattern or want to paint how a skeletal mesh is posed. \n\nNOTE Extra Extent to Apply Paint does Not work for isComplexShape. \n\nAffect Cloth Physics is a UE5 Exclusive Feature!"))
	static void PaintOnMeshWithinArea_Wrapper(UVertexPaintDetectionComponent* RuntimeVertexPaintAndDetectionComponent, UPrimitiveComponent* MeshComponent, FRVPDPPaintWithinAreaSettings PaintWithinAreaStruct, FRVPDPAdditionalDataToPassThroughInfo AdditionalDataToPassThrough);


	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Paint", meta = (BlueprintInternalUseOnly = "true", ToolTip = "Paint Vertex Colors on the Entire Mesh on Static, Skeletal or Spline Meshes. \n\n In the Apply Vertex Color Settings you can choose to Set the colors, which is useful if the Mesh's default colors is undesirable and you want to 'Clean' the Mesh by painting it with 0, 0, 0, 0. Remember that with UEs Mesh Painting Tool you can propogate colors to the source mesh permanently, so you can set the source to always be 0, 0, 0, 0. \nPaint Entire Mesh can be combined effectively with Paint Color Conditions under Apply Vertex Color Settings, where you can for instance Paint Entire Mesh, but only if a Channel is between certain values, so for instance under Rooftops won't get painted. \nYou can opt to Paint on Random Vertices over the Entire Mesh as well, which is great for Rain Effects or similar. \n\nAffect Cloth Physics is a UE5 Exclusive Feature!"))
	static void PaintOnEntireMesh_Wrapper(UVertexPaintDetectionComponent* RuntimeVertexPaintAndDetectionComponent, UPrimitiveComponent* MeshComponent, FRVPDPPaintOnEntireMeshSettings PaintOnEntireMeshStruct, FRVPDPAdditionalDataToPassThroughInfo AdditionalDataToPassThrough);


	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Paint", meta = (BlueprintInternalUseOnly = "true", ToolTip = "Paints a Color Snippet on Mesh with the Color Snippet Tag Provided, if the Tag is registered for the Mesh. If it isn't registered for the mesh, or set to default, but we've set to Randomize a Tag under a category, then we will do that instead, if there any tags under that category that is registered to the Mesh. \n\nColor Snippets can be very useful if you want to be able to store snippets of exactly how a Mesh should look for later use, for example variations of Weapon Skins, or if you want your character to look exactly in a certain way for a level, cutscene, or just want to store different variations of zombies and randomly choose a snippet when spawning one. \n\nAffect Cloth Physics is a UE5 Exclusive Feature!"))
	static void PaintColorSnippetOnMesh_Wrappers(UVertexPaintDetectionComponent* RuntimeVertexPaintAndDetectionComponent, UPrimitiveComponent* MeshComponent, FRVPDPPaintColorSnippetSettings PaintColorSnippetStruct, FRVPDPAdditionalDataToPassThroughInfo AdditionalDataToPassThrough);


	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Paint", meta = (BlueprintInternalUseOnly = "true", ToolTip = "Paints a Group Snippet on Mesh with the Color Snippet Tag Provided, if it is a Group Snippet Tag. Group Snippets can be very useful if you want Store and Paint Snippets that should run across several mesh components, such as if you have a character with several split up skeletal mesh components, or Canvas that's been split up into several small static mesh components so you get high vertex density but with low task time. \n\nAffect Cloth Physics is a UE5 Exclusive Feature!"))
	static void PaintGroupSnippetOnMesh_Wrapper(UVertexPaintDetectionComponent* RuntimeVertexPaintAndDetectionComponent, TArray<UPrimitiveComponent*> GroupSnippetMeshes, FRVPDPPaintGroupSnippetSettings PaintGroupSnippetStruct, FRVPDPAdditionalDataToPassThroughInfo AdditionalDataToPassThrough);


	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Paint", meta = (BlueprintInternalUseOnly = "true", ToolTip = "Sets Vertex Colors to Component Using a Vertex COlor Array. Array has to match the amount of vertices the MeshComponent has. This is useful for Save/Load Systems in case you've Saved the colors before and want to Load them later. "))
	static void SetMeshComponentVertexColors_Wrapper(UVertexPaintDetectionComponent* RuntimeVertexPaintAndDetectionComponent, UPrimitiveComponent* MeshComponent, FRVPDPSetVertexColorsSettings SetMeshComponentVertexColorsSettings, FRVPDPAdditionalDataToPassThroughInfo AdditionalDataToPassThrough);


	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Paint", meta = (BlueprintInternalUseOnly = "true", ToolTip = "Sets Color Data to a Component using Serialized String. This is useful for Save/Load Systems in case you've Saved the colors before and want to Load them later."))
	static void SetMeshComponentVertexColorsUsingSerializedString_Wrapper(UVertexPaintDetectionComponent* RuntimeVertexPaintAndDetectionComponent, UPrimitiveComponent* MeshComponent, FRVPDPSetVertexColorsUsingSerializedStringSettings SetMeshComponentVertexColorsUsingSerializedStringSettings, FRVPDPAdditionalDataToPassThroughInfo AdditionalDataToPassThrough);
};
