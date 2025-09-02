// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "CorePrerequisites.h"
#include "Chaos/ChaosEngineInterface.h"
#include "Engine/EngineTypes.h"
#include "CollisionQueryParams.h"
#include "Materials/MaterialInterface.h"
#include "PaintConditionsPrerequisites.generated.h" 


//-------------------------------------------------------

// Paint Condition Base

USTRUCT(BlueprintType, meta = (DisplayName = "Paint Condition Base"))
struct FRVPDPPaintConditionBase {

	GENERATED_BODY()


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Whether we should Apply a Different Color Strength if this specific Condition Failed. For instance if we want to Dry Mesh if in Line of Sight to a Sun, but it failed, the vertex is in a Shadow, then maybe we still want to Dry it, just by not as much. "))
	float ColorStrengthIfThisConditionIsNotMet = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Setting to Apply with if Failed to reach the Condition "))
	EApplyVertexColorSetting SettingIfConditionIsNotMet = EApplyVertexColorSetting::EAddVertexColor;
};


//-------------------------------------------------------

// Is Vertex Normal Within Dot To Direction Paint Condition

USTRUCT(BlueprintType, meta = (DisplayName = "Is Vertex Normal Within Dot To Direction Paint Condition Settings"))
struct FRVPDPIsVertexNormalWithinDotToDirectionPaintConditionSettings : public FRVPDPPaintConditionBase {

	GENERATED_BODY()


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The Direction/Normal we will get the Dot Product against with the Vertex Normal. "))
	FVector DirectionToCheckAgainst = FVector(ForceInitToZero);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If the Vertex Normal, and the Direction to Check Against's Dot Product is Min this the condition will pass. Great if you for instance have a Snow Storm toward a certain Direction, and you only want the side of a House or something that is in line with the storms Direction to get Painted. \nIf 1, then will only pass condition if the Vertex Normal and the Normal to check against is Exactly the same on the decimal, if -1 then all vertices will pass, but no point in using this condition if setting this to -1. If you're for instance painting a wall with Area of Effect, but you don't want to paint the Other side of the wall, but still be able to paint around corners, then something like -0.1 should work. "))
	float MinDotProductToDirectionRequired = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If set, then will get the actors rotation and set the Direction to check against to be that. "))
	AActor* GetDirectionFromActorRotation = nullptr;
};


//-------------------------------------------------------

// Is Vertex Direction Toward Location Paint Condition

USTRUCT(BlueprintType, meta = (DisplayName = "Is Vertex Direction Toward Location Paint Condition Settings"))
struct FRVPDPIsVertexDirectionTowardLocationPaintConditionSettings : public FRVPDPPaintConditionBase {

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If this is set then will use this Actors Location, otherwise the Location. "))
	AActor* Actor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If Actor is set then will use that, otherwise fall back to what is set here. "))
	FVector Location = FVector(ForceInitToZero);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If Vertex Normal to Actor or Location is at least above this Dot, only then the vertex will get included. It can be between -1 and 1, where -1 is the opposite (i.e. if set to -1 then all vertices will get affected), 0 is straight to the side, and 1 is straight toward the Location. Depending on what you're doing you may want to adjust this, for instance a fire hose spraying a wide amount of water over a large mesh, then you want all of the vertices within the AoE to get affected which means this may have to be at min 0.5"))
	float MinDotProductToActorOrLocation = 0.8;
};


//-------------------------------------------------------

// Is Vertex Within Direction From Location Paint Condition

USTRUCT(BlueprintType, meta = (DisplayName = "Is Vertex Within Direction From Location Paint Condition Settings"))
struct FRVPDPIsVertexWithinDirectionFromLocationPaintConditionSettings : public FRVPDPPaintConditionBase {

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If this is set then will use this Actors Location, otherwise the Location. "))
	AActor* Actor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Location we want to check against, could for example be the Base of a Cone. Will use the Actors Location if set, otherwise fallback to this one. "))
	FVector Location = FVector(ForceInitToZero);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The Direction we want to check against, if for example a cone, then the Direction the Cone is facing towards, i.e. the Direction it expands out to. "))
	FVector Direction = FVector(ForceInitToZero);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The Min Dot Product, of the Direction from the Location to the Vertex. If for example a Cone, that this should match the width of that cone, so if a very narrow cone then something closer to 1, if very wide then closer to 0. Should be between -1 and 1. "))
	float MinDotProductToDirection = 0.8;
};


//-------------------------------------------------------

// Is Vertex Above or Below World Z Paint Condition

USTRUCT(BlueprintType, meta = (DisplayName = "Is Vertex Above or Below World Z Paint Condition Settings"))
struct FRVPDPIsVertexAboveOrBelowWorldZPaintConditionSettings : public FRVPDPPaintConditionBase {

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The World Location Z value that we check if the Vertex Location Z is either equal/below, or if the bool is true, then it has to be equal/above.  "))
	float IfVertexIsAboveOrBelowWorldZ = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If True then checks if the Vertex is Equal or Above the World Z height, instead of Equal or Below. "))
	bool CheckIfAboveInsteadOfBelowZ = false;
};


//-------------------------------------------------------

// Is Vertex Color Within Color Range Paint Condition

USTRUCT(BlueprintType, meta = (DisplayName = "Is Vertex Color Within Color Range Paint Condition Settings"))
struct FRVPDPIsVertexColorWithinColorRangePaintConditionSettings : public FRVPDPPaintConditionBase {

	GENERATED_BODY()


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If this is set to something other then Default, then we will for each Section/Material the Mesh is using, check if it's registered in the Editor Widget and what Physics Surface is set on each channel, then do the Within Color Range for whatever Channel that is. Very useful if several Materials might not be synched with eachother and have something like Puddle on different channels, so you don't have to remember which channel it's on. "))
	TEnumAsByte<EPhysicalSurface> IfChannelWithPhysicsSurfaceIsWithinColorRange = EPhysicalSurface::SurfaceType_Default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If physicsSurfaceWithinColorRange is Default, then we will use this Color Channel is a certain Range. Gives the Developer more control of when paint is applied. Can for instance be used to Mask out Areas where you don't want it to be applied, like if it's Raining and you don't want the underside of Rooftops to get wet, then you can paint them with a channel the material isn't using, and have a condition that the channel has to be at 0 for the Wetness to be applied. "))
	ESurfaceAtChannel IfVertexColorChannelWithinColorRange = ESurfaceAtChannel::Default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If Current Vertex Color is >= than this. Gives the Developer more control of when paint is applied. Can for instance be used to Mask out Areas where you don't want it to be applied, like if it's Raining and you don't want the underside of Rooftops to get wet, then you can paint them with a channel the material isn't using, and have a condition that the channel has to be at 0 for the Wetness to be applied. "))
	float IfCurrentVertexColorValueIsHigherOrEqualThan = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If Current Vertex Color is <= than this. Gives the Developer more control of when paint is applied. Can for instance be used to Mask out Areas where you don't want it to be applied, like if it's Raining and you don't want the underside of Rooftops to get wet, then you can paint them with a channel the material isn't using, and have a condition that the channel has to be at 0 for the Wetness to be applied. "))
	float IfCurrentVertexColorValueIsLessOrEqualThan = 0;


	uint8 IfHigherOrEqualThanInt = 0;
	uint8 IfLessOrEqualThanInt = 0;
};


//-------------------------------------------------------

// Is Vertex On Bone Paint Condition

USTRUCT(BlueprintType, meta = (DisplayName = "Is Vertex On Bone Paint Condition Settings"))
struct FRVPDPIsVertexOnBonePaintConditionSettings : public FRVPDPPaintConditionBase {

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If Painting on Skeletal Mesh then can set to only Paint on specific Bone. If Static Mesh then we won't run this condition. Useful if you for instance have a Boss Fight and you should be able to Freeze exposed parts which could be just an Arm or a Leg etc. "))
	FName IfVertexIsAtBone = "None";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The Min Amount of Bone Weight Required. If 0 then will always apply, but can be increase to for instance 0.5 if you don't want to affect any below that. "))
	float MinBoneWeight = 0;
};


//-------------------------------------------------------

// Does Vertex Has Line Of Sight Paint Condition

USTRUCT(BlueprintType, meta = (DisplayName = "Does Vertex Has Line Of Sight Paint Condition Settings"))
struct FRVPDPDoesVertexHasLineOfSightPaintConditionSettings : public FRVPDPPaintConditionBase {

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If Set to an Actor then will use this Actors Location at Task Creation. "))
	AActor* IfVertexHasLineOfSightToActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If not set to use Line of Sight to Actor, then will use this. "))
	FVector IfVertexHasLineOfSightToPosition = FVector(ForceInitToZero);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "How far out from the vertex position we start the Trace toward the Position / Actor Location. If not ignoring the Mesh Component you're trying to paint on, and it has collision that completely covers the Mesh and the Vertices, then this can be increased so you start tracing outside of that collision and doesn't hit itself. \nYou can Add to Ignore the Mesh Component or Actor you're trying to paint. "))
	float DistanceFromVertexPositionToStartTrace = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Object Types to Trace if we have Line of Sight. "))
	TArray<TEnumAsByte<EObjectTypeQuery>> CheckLineOfSightAgainstObjectTypes = { EObjectTypeQuery::ObjectTypeQuery1, EObjectTypeQuery::ObjectTypeQuery2, EObjectTypeQuery::ObjectTypeQuery3, EObjectTypeQuery::ObjectTypeQuery4, EObjectTypeQuery::ObjectTypeQuery5 };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "NOTE If True then May affect the FPS if the Mesh has alot of vertices from 5.2 and up! Unsure why but for 5.0 i didn't notice any difference on heavy examples but did after 5.2. \n\nThis can be very useful in case you have something in which it's simple collision isn't 100%, for example a if you only want to paint on something that has line of sight to a Light, and you run this to a Location inside of a Lamp Shade, but the Lamp Shades collision has small gaps in it (which can occur easily if trying to build the collision using the editors solution). Then if this is true it will trace to the actual mesh and not the simple collision in it. "))
	bool TraceForComplex = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	TArray<AActor*> LineOfSightTraceActorsToIgnore;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	TArray<UPrimitiveComponent*> LineOfSightTraceMeshComponentsToIgnore;

	FCollisionObjectQueryParams LineOfSightTraceObjectQueryParams = FCollisionObjectQueryParams();

	FCollisionQueryParams LineOfSightTraceParams = FCollisionQueryParams();
};



//-------------------------------------------------------

// Is Vertex On Bone Paint Condition

USTRUCT(BlueprintType, meta = (DisplayName = "Is Vertex On Material Condition Settings"))
struct FRVPDPIsVertexOnMaterialPaintConditionSettings : public FRVPDPPaintConditionBase {

	GENERATED_BODY()


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Will require that a vertex is on a section, that has this material specific material on it, or an instance/dynamic instance of it. "))
	UMaterialInterface* IfVertexIsOnMaterial = nullptr;

	UPROPERTY()
	UMaterialInterface* IfVertexIsOnMaterialParent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If IfVertexIsOnMaterial didn't directly match the material a vertex had, then we can check if both of their parents is a material instance, or dynamic instance, and if they match instead. "))
	bool CheckMaterialParents = true;
};



//-------------------------------------------------------

// Paint Condition Settings

USTRUCT(BlueprintType, meta = (DisplayName = "Paint Condition Settings"))
struct FRVPDPPaintConditionSettings {

	GENERATED_BODY()


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If the Vertex Normal, and the Direction to Check Against's Dot Product is Min this the condition will pass. Great if you for instance have a Snow Storm toward a certain Direction, and you only want the side of a House or something that is in line with the storms Direction to get Painted. \nIf 1, then will only pass condition if the Vertex Normal and the Normal to check against is Exactly the same on the decimal, if -1 then all vertices will pass, but no point in using this condition if setting this to -1. If you're for instance painting a wall with Area of Effect, but you don't want to paint the Other side of the wall, but still be able to paint around corners, then something like -0.1 should work. "))
	TArray<FRVPDPIsVertexNormalWithinDotToDirectionPaintConditionSettings> IsVertexNormalWithinDotToDirection;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Vertices with their Normal above a Min Dot toward an Actor or Location. \nMay be useful if you need a fast way to get control over on what side of a mesh to paint, like if you have a hose that is spraying water on a character, but you only want the part of the character that is facing the hose to get affected and not the backside. There is also an a Paint Condition with Line of Sight to an Actor and Location, the difference between that and this, is that it will trace and see if something is blocking, which will make the task take longer, where as this only does the dot check and is much faster. "))
	TArray<FRVPDPIsVertexDirectionTowardLocationPaintConditionSettings> OnlyAffectVerticesWithDirectionToActorOrLocation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Vertices within a certain Direction's Dot. I.e. we get the Direction from the Location/Actor we set here, to each vertex Location, and check that, with the Direction we set here, which could for instance be the Direction of a Mesh like a downward Direction of a cone, if you want to check if a vertex is within the cone Direction. \nDepending on the use case you should tweak the dot product as well which should be from -1 to 1. With the cone example, the wider the cone, the closer to 0 we would want the min dot product, if it's very thin then closer to 1. \nUsed for instance in the Example Project to make a cheap Paint Within Area with Cone Example using Paint Within Area with Sphere Shape, and by utilizing this, we get the affect as if it was a cone. "))
	TArray<FRVPDPIsVertexWithinDirectionFromLocationPaintConditionSettings> OnlyAffectVerticesWithinDirectionFromActorOrLocation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If Vertex Location Z has to be Equal or Above, or Equal and below the given Z height for it to be painted. "))
	TArray<FRVPDPIsVertexAboveOrBelowWorldZPaintConditionSettings> IfVertexIsAboveOrBelowWorldZ;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If Physics Surface or Color Channel is a certain Range. Gives the Developer more control of when paint is applied. Can for instance be used to Mask out Areas where you don't want it to be applied, like if it's Raining and you don't want the underside of Rooftops to get wet, then you can paint them with a channel the material isn't using, and have a condition that the channel has to be at 0 for the Wetness to be applied. "))
	TArray<FRVPDPIsVertexColorWithinColorRangePaintConditionSettings> IfVertexColorIsWithinRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If Painting on Skeletal Mesh then can set to only Paint on specific Bone. Useful if you for instance have a Boss Fight and you should be able to Freeze exposed parts which could be just an Arm or a Leg etc. "))
	TArray<FRVPDPIsVertexOnBonePaintConditionSettings> IfVertexIsOnBone;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Will require that a vertex is on a section, that has this material specific material on it, or an instance/dynamic instance of it. "))
	TArray<FRVPDPIsVertexOnMaterialPaintConditionSettings> IfVertexIsOnMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If Vertex has Line of Sight to something, for instance the Sun, then only Apply/Remove Colors then. Useful if you for instance only want to Dry a Mesh where the Sun is actually hitting the Mesh. \nThis will Trace to check if it has line of sight, so may make the Task take longer, especially if playing in the Editor. "))
	TArray<FRVPDPDoesVertexHasLineOfSightPaintConditionSettings> IfVertexHasLineOfSightTo;
};
