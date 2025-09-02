// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "CorePrerequisites.h"
#include "FundementalsPrerequisites.h"
#include "PaintConditionsPrerequisites.h"
#include "Chaos/ChaosEngineInterface.h"
#include "PaintFundementalsPrerequisites.generated.h" 



//-------------------------------------------------------

// Paint Limit Settings

USTRUCT(BlueprintType, meta = (DisplayName = "Paint Limit Settings"))
struct FRVPDPPaintLimitSettings {

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Paint Limits are useful if you have something like light Rain that should be able to make characters completely drenched but only a bit wet. Then you can use this to limit how much the Rain can paint on whatever Channel. "))
	bool UsePaintLimits = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If painting and going over this limit we're going to clamp it to that. If set to limit if already over the limit then will always clamp it to whatever this is set. "))
	float PaintLimit = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If the Color was already over the limit before we Applied any, then should we clamp that color or not. Should be false if you for instance have a water drop that paints with limit of 0.75, that goes over an already fully watered surface, then the drop shouldn't change and make the surface has less water. "))
	bool LimitColorIfTheColorWasAlreadyOverTheLimit = false;
};


//-------------------------------------------------------

// Lerp Vertex Color To Target Settings

USTRUCT(BlueprintType, meta = (DisplayName = "Lerp Vertex Color To Target Settings"))
struct FRVPDPLerpVertexColorToTargetSettings {

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If True then will Lerp the vertex color to the target value with the given lerp strength. For instance if the current vertex color value for a channel is 1, but you lerp toward 0.5 with lerp strength 0.1, then after 1 paint job the vertex channel should be 0.9. "))
	bool LerpToTarget = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The Value we will lerp towards. "))
	float TargetValue = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "What Alpha to Lerp with."))
	float LerpStrength = 0;
};


//-------------------------------------------------------

// Physics Surface To Apply Settings

USTRUCT(BlueprintType, meta = (DisplayName = "Physics Surface To Apply Settings"))
struct FRVPDPPhysicsSurfaceToApplySettings {

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Physics Surface to Apply. "))
	TEnumAsByte<EPhysicalSurface> SurfaceToApply = EPhysicalSurface::SurfaceType_Default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Strength to Apply on Physics Surface. "))
	float StrengthToApply = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Setting to Apply with Physics Surface. NOTE That if you've added several of the Same physics surface, i.e. that is registered on the same channel, then the last one setting of this will decide weather we will Add or Set on the channel the physics surface is registered to. "))
	EApplyVertexColorSetting SettingToApplyWith = EApplyVertexColorSetting::EAddVertexColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Color Condition to use on whatever Vertex Color Channels that was successfull in getting what colors to apply. "))
	FRVPDPPaintConditionSettings PaintConditions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If True then will Lerp the vertex color in the channel that had the physics surface registered, to the target value with the given lerp strength. For instance if the current vertex color value for a channel is 1, but you lerp toward 0.5 with lerp strength 0.1, then after 1 paint job the vertex channel should be 0.9.  "))
	FRVPDPLerpVertexColorToTargetSettings LerpPhysicsSurfaceToTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Whether to Limit Paint Color up to a certain amount on the vertex color channels that had the physics surface, useful for stuff like Light Rain that shouldn't make characters completely Drenched but just a bit wet. "))
	FRVPDPPaintLimitSettings SurfacePaintLimit;
};


//-------------------------------------------------------

// Apply Colors Using Physics Surface Settings

USTRUCT(BlueprintType, meta = (DisplayName = "Apply Colors Using Physics Surface Settings"))
struct FRVPDPApplyColorsUsingPhysicsSurfaceSettings {

	GENERATED_BODY()


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Applies color using Physics surface, if it's registered on a Vertex Channel, or if it's not registered, but it is a Parent / Child of the one we set, then we can apply to that. It's also useful if your Materials Vertex Color Channels isn't synched up, where for instance a Puddle could be on different Channels on different materials. It's also useful if you don't want to apply any colors at all if a surface isn't registered to the Material. It requires that the Material and what Physics Surface is on each channel is registered in the Editor Widget. Make sure to setup the Physics Surface Families as well, for example if you have Sand as a Parent, and Cobble-Sand as a Child, and a character that has Sand on their Red Vertex Channel, if they're walking over a floor with Cobble-Sand, it can Apply Colors on the Channel with Sand. "))
	bool ApplyVertexColorUsingPhysicsSurface = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The Physics Surfaces you want to Paint/Remove Colors on and with how much Strength and if any Paint Condition should be used. Since you can set it per physics surface, you can Add on one and Remove on another. This can be a Parent or Child of a Parent Physics Surface as well if properly registered in the Editor Widget, for example if you have Sand as a Parent, and Cobble-Sand as a Child, and a character that has Sand on their Red Vertex Channel. Then if they're walking over a floor with Cobble-Sand, it will Apply Colors on the Channel with Sand, i.e. the Red Channel. \nShould be between -1 - 1. \nNOTE that if less than 0.005 (or -0.005 if minus), then we will clamp to that as long as it's not 0, as that is the lowest amount needed to paint at the lowest strength when converted to FColor."))
	TArray<FRVPDPPhysicsSurfaceToApplySettings> PhysicalSurfacesToApply;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "With this you can Apply a Strength to the Channels that we where unsuccessfull in getting colors for, i.e. channels that didn't have the surface registered etc. Useful if for Example if you want to paint Puddle, and Remove on the other Channels since water should clean away stuff on them, then you could set this to a minus value. Should be between -1 - 1. If painting with Add and not Set, then this and paintStrengthToApply can't both be 0, since then no difference will occur so no reason to start the task. \nNOTE that if less than 0.005 (or -0.005 if minus), then we will clamp to that as long as it's not 0, as that is the lowest amount needed to paint at the lowest strength since when converted to FColor."))
	float StrengtOnChannelsWithoutThePhysicsSurfaces = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "What setting we should use if we want to apply a strength on channels without any of the physics surfaces.  "))
	EApplyVertexColorSetting ApplyWithSettingOnChannelsWithoutThePhysicsSurface = EApplyVertexColorSetting::EAddVertexColor;
};


//-------------------------------------------------------

// Apply Colors Using Vertex Channel Settings

USTRUCT(BlueprintType, meta = (DisplayName = "Apply Colors Using Vertex Channel Settings"))
struct FRVPDPApplyColorsUsingVertexChannelSettings {

	GENERATED_BODY()


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Color to Apply on Channel. Should be between -1.0 and 1.0 if Adding, or between 0-1 if Setting. With Set, you actually Set the vertex color to be that. "))
	float AmountToApply = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If True then will Lerp the vertex color to the target value with the given lerp strength. For instance if the current vertex color value for a channel is 1, but you lerp toward 0.5 with lerp strength 0.1, then after 1 paint job the vertex channel should be 0.9. "))
	FRVPDPLerpVertexColorToTargetSettings LerpVertexColorToTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Add will Add onto the existing colors, meaning if you set higher than 0 in strength, it will increase the vertex colors up to maximum 1, if less than 0, it decrease down to 0. If Set to Set Vertex Color, it will set the colors to the strength, if 0 it will set the vertex colors to the lowest 0 for instance. "))
	EApplyVertexColorSetting ApplyWithSetting = EApplyVertexColorSetting::EAddVertexColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Apply Color on Channel if Specified Paint Conditions are met."))
	FRVPDPPaintConditionSettings PaintConditions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "When not painting with physics surface but is using the vertex channels values, this is the Paint Limit up to a certain amount for this channel. Useful for stuff like light Rain that shouldn't make characters completely Drenched but just a bit wet. "))
	FRVPDPPaintLimitSettings PaintLimit;
};


//-------------------------------------------------------

//  Adjust Paint Strength To Delta Time Settings

USTRUCT(BlueprintType, meta = (DisplayName = "Adjust Paint Strength Settings"))
struct FRVPDPAdjustPaintStrengthToDeltaTimeSettings {

	GENERATED_BODY()


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Adjusts the Paint Strength depending on the Delta time, so even if the Delta time, i.e. the FPS changes, something can get somewhat painted equally fast. \nThis is useful in mechanics where you may paint something every frame, and you don't want it to get painted too fast if the FPS is high. \nIt does this by using a simple calculation the PaintStrength = PaintStrength * DeltaTime * TargetFPS. "))
	bool AdjustPaintStrengthsToDeltaTime = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The Current Delta Seconds you want to adjust for. "))
	float DeltaTimeToAdjustTo = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The Base Target FPS of your game. Will most likely be 60 FPS. "))
	float TargetFPS = 60;
};


//-------------------------------------------------------

//  Apply Color Settings

USTRUCT(BlueprintType, meta = (DisplayName = "Apply Color Settings"))
struct FRVPDPApplyColorSettings {

	GENERATED_BODY()


	bool IsAnyColorChannelSetToAddColorAndIsNotZero() const {

		if (ApplyColorsOnRedChannel.ApplyWithSetting == EApplyVertexColorSetting::EAddVertexColor && ApplyColorsOnRedChannel.AmountToApply != 0)
			return true;

		if (ApplyColorsOnGreenChannel.ApplyWithSetting == EApplyVertexColorSetting::EAddVertexColor && ApplyColorsOnGreenChannel.AmountToApply != 0)
			return true;

		if (ApplyColorsOnBlueChannel.ApplyWithSetting == EApplyVertexColorSetting::EAddVertexColor && ApplyColorsOnBlueChannel.AmountToApply != 0)
			return true;

		if (ApplyColorsOnAlphaChannel.ApplyWithSetting == EApplyVertexColorSetting::EAddVertexColor && ApplyColorsOnAlphaChannel.AmountToApply != 0)
			return true;

		return false;
	}


	bool IsAnyColorChannelSetToSetColor() const {

		if (ApplyColorsOnRedChannel.ApplyWithSetting == EApplyVertexColorSetting::ESetVertexColor)
			return true;

		if (ApplyColorsOnGreenChannel.ApplyWithSetting == EApplyVertexColorSetting::ESetVertexColor)
			return true;

		if (ApplyColorsOnBlueChannel.ApplyWithSetting == EApplyVertexColorSetting::ESetVertexColor)
			return true;

		if (ApplyColorsOnAlphaChannel.ApplyWithSetting == EApplyVertexColorSetting::ESetVertexColor)
			return true;

		return false;
	}

	bool IsAnyColorChannelSetToSetToLerpToTarget() const {

		if (ApplyColorsOnRedChannel.LerpVertexColorToTarget.LerpToTarget && ApplyColorsOnRedChannel.LerpVertexColorToTarget.LerpStrength > 0)
			return true;

		if (ApplyColorsOnGreenChannel.LerpVertexColorToTarget.LerpToTarget && ApplyColorsOnGreenChannel.LerpVertexColorToTarget.LerpStrength > 0)
			return true;

		if (ApplyColorsOnBlueChannel.LerpVertexColorToTarget.LerpToTarget && ApplyColorsOnBlueChannel.LerpVertexColorToTarget.LerpStrength > 0)
			return true;

		if (ApplyColorsOnAlphaChannel.LerpVertexColorToTarget.LerpToTarget && ApplyColorsOnAlphaChannel.LerpVertexColorToTarget.LerpStrength > 0)
			return true;

		return false;
	}


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "This is useful so you don't have to hardcode what to paint on which Vertex Color Channel, but just want to paint a physics surface, if it exists. It's also useful if your Materials Vertex Color Channels isn't synched up, where for instance a Puddle could be on different Channels on different materials. It's also useful if you don't want to apply any colors at all if a surface isn't registered to the Material. It requires that the Material and what Physics Surface is on each channel is registered in the Editor Widget. Make sure to setup the Parent and Child Surfaces as well, for example if you have Mud as a Parent, and Cobble-Mud as a Child, and a character that has Mud on their Red Vertex Channel, if they're walking over a floor with Cobble-Mud, it can Apply Colors on the Channel with Mud, i.e. the Red Channel."))
	FRVPDPApplyColorsUsingPhysicsSurfaceSettings ApplyColorsUsingPhysicsSurface;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Colors to Apply on Channel Settings, such as Amount to Apply, if should Add or Set, and if any conditions has to be met. "))
	FRVPDPApplyColorsUsingVertexChannelSettings ApplyColorsOnRedChannel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Colors to Apply on Channel Settings, such as Amount to Apply, if should Add or Set, and if any conditions has to be met. "))
	FRVPDPApplyColorsUsingVertexChannelSettings ApplyColorsOnGreenChannel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Colors to Apply on Channel Settings, such as Amount to Apply, if should Add or Set, and if any conditions has to be met. "))
	FRVPDPApplyColorsUsingVertexChannelSettings ApplyColorsOnBlueChannel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Colors to Apply on Channel Settings, such as Amount to Apply, if should Add or Set, and if any conditions has to be met. "))
	FRVPDPApplyColorsUsingVertexChannelSettings ApplyColorsOnAlphaChannel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Adjusts the Paint Strength depending on the Delta time, so even if the Delta time, i.e. the FPS changes, something can get somewhat painted equally fast. \nThis is useful in mechanics where you may paint something every frame, and you don't want it to get painted too fast if the FPS is high. \It does this by using a simple calculation the PaintStrength = PaintStrength * DeltaTime * TargetFPS. "))
	FRVPDPAdjustPaintStrengthToDeltaTimeSettings AdjustPaintStrengthToDeltaTimeSettings;
};


//-------------------------------------------------------

// Override Paint Up To LOD

USTRUCT(BlueprintType, meta = (DisplayName = "Override LOD To Paint Up To Settings"))
struct FRVPDPOverrideLODToPaintUpToSettings {

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If we override the amount of LODs to Paint so only Paint up to a Certain Amount. Can save performance by alot. You can also set in the Editor Widget the Standard Amount of LODs to Paint on Skeletal and Static Meshes. "))
	bool OverrideLODToPaintUpTo = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	int AmountOfLODsToPaint = 1;
};


//-------------------------------------------------------

// Override Colors To Apply

USTRUCT(BlueprintType, meta = (DisplayName = "Override Colors To Apply Settings"))
struct FRVPDPOverrideColorsToApplySettings {

	GENERATED_BODY()


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If we want to run an Override Vertex Colors To Apply Interface call on an actor for every single vertex, with information such as the current vertex color, position, normal etc., then the user can themselves decide what amount they want to return. This means that the user can create their own paint conditions for their specific needs. \nThe Actor has to implement the VertexPaintDetectionInterface interface. \nNOTE That if the task is started using multithreading, then the interface will execute on another thread, so recommend avoiding things like debug symbols."))
	bool OverrideVertexColorsToApply = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The Object to run the Interface on. The Object has to implement the VertexPaintDetectionInterface interface. \nNOTE That if the task is started using multithreading, then the interface will execute on another thread, so recommend avoiding things like debug symbols as they will cause crashes. "))
	UObject* ObjectToRunOverrideVertexColorsInterface = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "This will be sent along with the Override Vertex Colors Interface so you can do different overrides depending on which Task is calling to do so. Useful if you have several tasks running overrides on the same actor, and you want them to do different things. "))
	int OverrideVertexColorsToApplyID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If True this can greatly optimize this feature since you're not running the interface for every single vertex but just those that we're trying to change, for instance those within the Area of Effect if running a Paint at Location. "))
	bool OnlyRunOverrideInterfaceIfTryingToApplyColorToVertex = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If we should Include the Most Dominant Physics Surface and Value for each Vertex when we run the Interface. NOTE That this makes the Task roughly 5x slower! A Task with 23k vert character took 0.03 sec to finish with this set to False, and 0.15 sec to finish if True. But it may be useful if you want to get what physics surface is on the vertex and how much of it, and the speed of the task doesn't matter. \nFor instance if you want to know if Water in case you have some electrical gameplay thing going on. "))
	bool IncludeMostDominantPhysicsSurface = false;
};



//-------------------------------------------------------

// Paint Task Callback Settings

USTRUCT(BlueprintType, meta = (DisplayName = "Paint Task Callback Settings"))
struct FRVPDPPaintTaskCallbackSettings {

	GENERATED_BODY()


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If True then we will fill an Array of the bones that got vertex colors applied on them, which you can get in the paint task callback settings.  "))
	bool IncludeBonesThatGotPainted = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If True, then we will fill an Array of the vertex indexes that got vertex colors applied on them on LOD0, which you can get in the paint task callback settings. "))
	bool IncludePaintedVertexIndicesAtLOD0 = false;
};


//-------------------------------------------------------

// Paint Task Settings

USTRUCT(BlueprintType, meta = (DisplayName = "Paint Task Settings"))
struct FRVPDPPaintTaskSettings : public FRVPDPTaskFundamentalSettings {

	GENERATED_BODY()


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Whether we should Apply the change in Vertex Colors from the Paint Job to the Mesh. You can set this to false if you want to go through the entire paint job and get information such as Amount of Painted Colors of Each Channel, Compare Mesh Colors % or Task Duration etc. to know what they would've been IF the paint job had applied the colors. "))
	bool ApplyPaintJobToVertexColors = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If we override the amount of LODs to Paint so only Paint up to a Certain Amount. Can save performance by alot. You can also set in the Editor Widget the Standard Amount of LODs to Paint on Skeletal and Static Meshes. "))
	FRVPDPOverrideLODToPaintUpToSettings OverrideLOD;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Whether Cloth Physics should get affected at all. "))
	bool AffectClothPhysics = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If you only want to be able to paint on just certain actors then add them here. \nUseful if you for instance have a faucet that drips and paints out puddles of water etc. with sphere traces but doesn't want the player to get painted as well if they walk into the sphere but only the floor. "))
	TArray<AActor*> CanOnlyApplyPaintOnTheseActors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If we want to run an Override Vertex Colors To Apply Interface call on an actor for every single vertex, with information such as the current vertex color, position, normal etc., then the user can themselves decide what amount they want to return. This means that the user can create their own paint conditions for their specific needs. \nThe Actor has to implement the VertexPaintDetectionInterface interface. \nNOTE That if the task is started using multithreading, then the interface will execute on another thread, so recommend avoiding things like debug symbols."))
	FRVPDPOverrideColorsToApplySettings OverrideVertexColorsToApplySettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Paint Task Specific Callback Settings such as which bones and vertex indices got painted. "))
	FRVPDPPaintTaskCallbackSettings PaintTaskCallbackSettings;

	bool CanClearOutPreviousMeshTasksInQueue = false;
};


//-------------------------------------------------------

// Apply Color Setting

USTRUCT(BlueprintType, meta = (DisplayName = "Apply Color Setting"))
struct FRVPDPApplyColorSetting : public FRVPDPPaintTaskSettings {

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If we should Add or Set Vertex Colrs to be what we set here. The Strength to Apply to Colors should be between -1.0 to 1.0. If you Set vertex colors then 0 to 1 is what matters. "))
	FRVPDPApplyColorSettings ApplyVertexColorSettings;
};


//-------------------------------------------------------

// Paint Directly Task Settings

USTRUCT(BlueprintType, meta = (DisplayName = "Paint Directly Task Settings"))
struct FRVPDPPaintDirectlyTaskSettings : public FRVPDPPaintTaskSettings {

	GENERATED_BODY()

	// The reason why this has its own limit colors is because if we move this Up to the parent struct, then it may be confusing when running a paint job that applies with RGBA since they have one under each channel, and if applying with physics surface you specify it per surface. 

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Limit up to a certain amount, useful if for example Paints a Color Snippet or SetMeshComponentColors that is a pattern, but you don't want it to be fully colored but just a bit, to hint to the players that they should do something with the pattern. "))
	FRVPDPPaintLimitSettings VertexColorRedChannelsLimit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Limit up to a certain amount, useful if for example Paints a Color Snippet or SetMeshComponentColors that is a pattern, but you don't want it to be fully colored but just a bit, to hint to the players that they should do something with the pattern. "))
	FRVPDPPaintLimitSettings VertexColorGreenChannelsLimit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Limit up to a certain amount, useful if for example Paints a Color Snippet or SetMeshComponentColors that is a pattern, but you don't want it to be fully colored but just a bit, to hint to the players that they should do something with the pattern. "))
	FRVPDPPaintLimitSettings VertexColorBlueChannelsLimit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Limit up to a certain amount, useful if for example Paints a Color Snippet or SetMeshComponentColors that is a pattern, but you don't want it to be fully colored but just a bit, to hint to the players that they should do something with the pattern. "))
	FRVPDPPaintLimitSettings VertexColorAlphaChannelsLimit;
};