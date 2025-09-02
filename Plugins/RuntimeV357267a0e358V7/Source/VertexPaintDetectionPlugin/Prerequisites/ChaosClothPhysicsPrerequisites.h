// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "ChaosClothPhysicsPrerequisites.generated.h" 


//-------------------------------------------------------

// Cloth Physics - UChaosClothingInteractor::SetDamping()

USTRUCT(BlueprintType, meta = (DisplayName = "Cloth Physics Damping Settings"))
struct FRVPDPClothPhysicsDampingSettings {

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The amount of global damping applied to the cloth velocities, also known as point damping. Point damping improves simulation stability, but can also cause an overall slow-down effect and therefore is best left to very small percentage amounts."))
	float SetDamping_dampingCoefficient = 0.01;
};


//-------------------------------------------------------

// Cloth Physics - UChaosClothingInteractor::SetGravity()

USTRUCT(BlueprintType, meta = (DisplayName = "Cloth Physics Gravity Settings"))
struct FRVPDPClothPhysicsGravitySettings {

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Scale factor applied to the world gravity and also to the clothing simulation interactor gravity. Does not affect the gravity if set using the override below."))
	float SetGravity_gravityScale = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Use the config gravity value instead of world gravity."))
	bool SetGravity_overrideGravity = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The gravitational acceleration vector [cm/s^2]"))
	FVector SetGravity_gravityOverride = FVector(0, 0, -980.664978);
};


//-------------------------------------------------------

// Cloth Physics - UChaosClothingInteractor::SetWind()

USTRUCT(BlueprintType, meta = (DisplayName = "Cloth Physics Wind Settings"))
struct FRVPDPClothPhysicsWindSettings {

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The aerodynamic coefficient of drag applying on each particle. If an enabled Weight Map (Mask with values in the range [0;1]) targeting the 'Drag' is added to the cloth, then both the Low and High values will be used in conjunction with the per particle Weight stored in the Weight Map to interpolate the final value from them. Otherwise only the Low value is meaningful and sufficient to set the aerodynamic drag."))
	FVector2D SetWind_drag = FVector2D(0.07, 0.5);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The aerodynamic coefficient of lift applying on each particle. If an enabled Weight Map (Mask with values in the range [0;1]) targeting the 'Lift' is added to the cloth, then both the Low and High values will be used in conjunction with the per particle Weight stored in the Weight Map to interpolate the final value from them. Otherwise only the Low value is meaningful and sufficient to set the aerodynamic lift."))
	FVector2D SetWind_lift = FVector2D(0.07, 0.5);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	float SetWind_airDensity = 0.000001225f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	FVector SetWind_windVelocity = FVector(0, 0, 0);
};


//-------------------------------------------------------

// Cloth Physics - UChaosClothingInteractor::SetAnimDrive()

USTRUCT(BlueprintType, meta = (DisplayName = "Cloth Physics Anim Drive Settings"))
struct FRVPDPClothPhysicsAnimDriveSettings {

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The strength of the constraint driving the cloth towards the animated goal mesh. If an enabled Weight Map (Mask with values in the range [0;1]) targeting the 'Anim Drive Stiffness' is added to the cloth, then both the Low and High values will be used in conjunction with the per particle Weight stored in the Weight Map to interpolate the final value from them. Otherwise only the Low value is meaningful and sufficient to enable this constraint."))
	FVector2D SetAnimDrive_Stiffness = FVector2D(0, 1);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The damping amount of the anim drive. If an enabled Weight Map (Mask with values in the range [0;1]) targeting the 'Anim Drive Damping' is added to the cloth, then both the Low and High values will be used in conjunction with the per particle Weight stored in the Weight Map to interpolate the final value from them. Otherwise only the Low value is sufficient to work on this constraint."))
	FVector2D SetAnimDrive_Damping = FVector2D(0, 1);
};


//-------------------------------------------------------

// Cloth Physics - UChaosClothingInteractor::SetCollision()

USTRUCT(BlueprintType, meta = (DisplayName = "Cloth Physics Collision Settings"))
struct FRVPDPClothPhysicsCollisionSettings {

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The added thickness of collision shapes."))
	float SetCollision_CollisionThickness = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Friction coefficient for cloth - collider interaction."))
	float SetCollision_FrictionCoefficient = 0.8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Use continuous collision detection (CCD) to prevent any missed collisions between fast moving particles and colliders. This has a negative effect on performance compared to when resolving collision without using CCD."))
	bool SetCollision_UseCCD = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Enable self collision."))
	float SetCollision_SelfCollisionThickness = 2;
};


//-------------------------------------------------------

// Cloth Physics - UChaosClothingInteractor::SetLongRangeAttachment()

USTRUCT(BlueprintType, meta = (DisplayName = "Cloth Physics Long Range Attachment Settings"))
struct FRVPDPClothPhysicsLongRangeAttachmentSettings {

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The tethers' stiffness of the long range attachment constraints. The long range attachment connects each of the cloth particles to its closest fixed point with a spring constraint. This can be used to compensate for a lack of stretch resistance when the iterations count is kept low for performance reasons. Can lead to an unnatural pull string puppet like behavior. If an enabled Weight Map (Mask with values in the range [0;1]) targeting the 'Tether Stiffness' is added to the cloth, then both the Low and High values will be used in conjunction with the per particle Weight stored in the Weight Map to interpolate the final value from them. Otherwise only the Low value is meaningful and sufficient to enable this constraint. Use 0, 0 to disable."))
	FVector2D LongRangeAttachment_TetherThickness = FVector2D(1, 1);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The limit scale of the long range attachment constraints (aka tether limit). If an enabled Weight Map (Mask with values in the range [0;1]) targeting the 'Tether Scale' is added to the cloth, then both the Low and High values will be used in conjunction with the per particle Weight stored in the Weight Map to interpolate the final value from them. Otherwise only the Low value is meaningful and sufficient to set the tethers' scale."))
	FVector2D LongRangeAttachment_TetherScale = FVector2D(1, 1);
};


//-------------------------------------------------------

// Cloth Physics - UChaosClothingInteractor::SetMaterial()

USTRUCT(BlueprintType, meta = (DisplayName = "Cloth Physics Material Settings"))
struct FRVPDPClothPhysicsMaterialSettings {

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The Stiffness of segments constraints. Increase the iteration count for stiffer materials. If an enabled Weight Map (Mask with values in the range [0;1]) targeting the 'Edge Stiffness' is added to the cloth, then both the Low and High values will be used in conjunction with the per particle Weight stored in the Weight Map to interpolate the final value from them. Otherwise only the Low value is meaningful and sufficient to enable this constraint."))
	FVector2D Material_EdgeStiffness = FVector2D(1, 1);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The Stiffness of cross segments and bending elements constraints. Increase the iteration count for stiffer materials. If an enabled Weight Map (Mask with values in the range [0;1]) targeting the 'Bend Stiffness' is added to the cloth, then both the Low and High values will be used in conjunction with the per particle Weight stored in the Weight Map to interpolate the final value from them. Otherwise only the Low value is meaningful and sufficient to enable this constraint."))
	FVector2D Material_BendingStiffness = FVector2D(1, 1);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The stiffness of the surface area preservation constraints. Increase the iteration count for stiffer materials. If an enabled Weight Map (Mask with values in the range [0;1]) targeting the 'Bend Stiffness' is added to the cloth, then both the Low and High values will be used in conjunction with the per particle Weight stored in the Weight Map to interpolate the final value from them. Otherwise only the Low value is meaningful and sufficient to enable this constraint."))
	FVector2D Material_AreaStiffness = FVector2D(1, 1);
};


//-------------------------------------------------------

// Cloth Physics - UChaosClothingInteractor::SetVelocityScale()

USTRUCT(BlueprintType, meta = (DisplayName = "Cloth Physics Velocity Scale Settings"))
struct FRVPDPClothPhysicsVelocityScaleSettings {

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The amount of linear velocities sent to the local cloth space from the reference bone (the closest bone to the root on which the cloth section has been skinned, or the root itself if the cloth isn't skinned)."))
	FVector PhysicsVelocityScale_LinearVelocityScale = FVector(0.75, 0.75, 0.75);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The amount of angular velocities sent to the local cloth space from the reference bone the closest bone to the root on which the cloth section has been skinned, or the root itself if the cloth isn't skinned)."))
	float PhysicVelocityScale_AngularVelocityScale = 0.75;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The portion of the angular velocity that is used to calculate the strength of all fictitious forces (e.g. centrifugal force). This parameter is only having an effect on the portion of the reference bone's angular velocity that has been removed from the simulation via the Angular Velocity Scale parameter. This means it has no effect when AngularVelocityScale is set to 1 in which case the cloth is simulated with full world space angular velocitiesand subjected to the true physical world inertial forces. Values range from 0 to 2, with 0 showing no centrifugal effect, 1 full centrifugal effect, and 2 an overdriven centrifugal effect."))
	float PhysicsVelocityScale_FictitiousAngularVelocityScale = 1;
};


//-------------------------------------------------------

// Cloth Physics - UChaosClothingInteractor::SetPressure()

USTRUCT(BlueprintType, meta = (DisplayName = "Cloth Physics Set Air Pressure Settings"))
struct FRVPDPClothPhysicsSetAirPressureSettings {

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Pressure force strength applied in the normal direction(use negative value to push toward backface) If an enabled Weight Map (Mask with values in the range [0;1]) targeting the Pressure is added to the cloth, then both the Low and High values will be used in conjunction with the per particle Weight stored in the Weight Map to interpolate the final value from them. Otherwise only the Low value is meaningful and sufficient to set the pressure. "))
	FVector2D SetPressure_Pressure = FVector2D(ForceInitToZero);
};


//-------------------------------------------------------

// Cloth Physics Settings At Vertex Color Struct

USTRUCT(BlueprintType, meta = (DisplayName = "Chaos Cloth Physics At Vertex Color Channel Settings"))
struct FRVPDPChaosClothPhysicsAtVertexColorChannelSettings {

	GENERATED_BODY()


	// Damping
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	bool SetDamping = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	FRVPDPClothPhysicsDampingSettings ClothDampingSettingsWithNoColorPaintedAtChannel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	FRVPDPClothPhysicsDampingSettings ClothDampingSettingsWithFullColorPaintedAtChannel;


	// Gravity
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	bool SetGravity = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	FRVPDPClothPhysicsGravitySettings ClothGravitySettingsWithNoColorPaintedAtChannel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	FRVPDPClothPhysicsGravitySettings ClothGravitySettingsWithFullColorPaintedAtChannel;


	// Wind
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	bool SetWind = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	FRVPDPClothPhysicsWindSettings ClothWindSettingsWithNoColorPaintedAtChannel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	FRVPDPClothPhysicsWindSettings ClothWindSettingsWithFullColorPaintedAtChannel;


	// Anim Drive
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	bool SetAnimDrive = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	FRVPDPClothPhysicsAnimDriveSettings ClothAnimDriveSettingsWithNoColorPaintedAtChannel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	FRVPDPClothPhysicsAnimDriveSettings ClothAnimDriveSettingsWithFullColorPaintedAtChannel;


	// Collision
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	bool SetCollision = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	FRVPDPClothPhysicsCollisionSettings ClothCollisionSettingsWithNoColorPaintedAtChannel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	FRVPDPClothPhysicsCollisionSettings ClothCollisionSettingsWithFullColorPaintedAtChannel;


	// Long Range Attachment
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	bool SetLongRangeAttachment = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	FRVPDPClothPhysicsLongRangeAttachmentSettings ClothLongRangeAttachmentSettingsWithNoColorPaintedAtChannel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	FRVPDPClothPhysicsLongRangeAttachmentSettings ClothLongRangeAttachmentSettingsWithFullColorPaintedAtChannel;


	// Material
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	bool SetMaterial = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	FRVPDPClothPhysicsMaterialSettings ClothMaterialSettingsWithNoColorPaintedAtChannel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	FRVPDPClothPhysicsMaterialSettings ClothMaterialSettingsWithFullColorPaintedAtChannel;


	// Physics Velocity Scale
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	bool SetPhysicsVelocityScale = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	FRVPDPClothPhysicsVelocityScaleSettings ClothPhysicsVelocityScaleSettingsWithNoColorPaintedAtChannel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	FRVPDPClothPhysicsVelocityScaleSettings ClothPhysicsVelocityScaleSettingsWithFullColorPaintedAtChannel;


	// Set Air Pressure
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	bool SetAirPressure = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	FRVPDPClothPhysicsSetAirPressureSettings ClothPhysicssAirPressureWithNoColorPaintedAtChannel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	FRVPDPClothPhysicsSetAirPressureSettings ClothPhysicssAirPressureWithFullColorPaintedAtChannel;
};


//-------------------------------------------------------

// Cloth Settings Struct

USTRUCT(BlueprintType, meta = (DisplayName = "Vertex Channels Chaos Cloth Physics Settings"))
struct FRVPDPVertexChannelsChaosClothPhysicsSettings {

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "What the Cloth Physics Settings should be on the Red Channel. If Red is 0-1, how a cloths physics should get affected. "))
	FRVPDPChaosClothPhysicsAtVertexColorChannelSettings ClothPhysicsSettingsAtRedChannel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "What the Cloth Physics Settings should be on the Green Channel. If Red is 0-1, how a cloths physics should get affected. "))
	FRVPDPChaosClothPhysicsAtVertexColorChannelSettings ClothPhysicsSettingsAtGreenChannel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "What the Cloth Physics Settings should be on the Blue Channel. If Red is 0-1, how a cloths physics should get affected. "))
	FRVPDPChaosClothPhysicsAtVertexColorChannelSettings ClothPhysicsSettingsAtBlueChannel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "What the Cloth Physics Settings should be on the Alpha Channel. If Red is 0-1, how a cloths physics should get affected. "))
	FRVPDPChaosClothPhysicsAtVertexColorChannelSettings ClothPhysicsSettingsAtAlphaChannel;
};


//-------------------------------------------------------

// Cloth Physics Settings At Vertex Color Struct

USTRUCT(BlueprintType, meta = (DisplayName = "Chaos Cloth Physics Settings"))
struct FRVPDPChaosClothPhysicsSettings {

	GENERATED_BODY()


	// Damping
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	bool SetDamping = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	FRVPDPClothPhysicsDampingSettings ClothDampingSettings;


	// Gravity
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	bool SetGravity = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	FRVPDPClothPhysicsGravitySettings ClothGravitySettings;


	// Wind
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	bool SetWind = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	FRVPDPClothPhysicsWindSettings ClothWindSettings;


	// Anim Drive
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	bool SetAnimDrive = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	FRVPDPClothPhysicsAnimDriveSettings ClothAnimDriveSettings;


	// Collision
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	bool SetCollision = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	FRVPDPClothPhysicsCollisionSettings ClothCollisionSettings;


	// Long Range Attachment
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	bool SetLongRangeAttachment = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	FRVPDPClothPhysicsLongRangeAttachmentSettings ClothLongRangeAttachmentSettings;


	// Material
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	bool SetMaterial = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	FRVPDPClothPhysicsMaterialSettings ClothMaterialSettings;


	// Physics Velocity Scale
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	bool SetPhysicsVelocityScale = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	FRVPDPClothPhysicsVelocityScaleSettings ClothPhysicsVelocityScaleSettings;


	// Air Pressure
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	bool SetAirPressure = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	FRVPDPClothPhysicsSetAirPressureSettings ClothPhysicssAirPressureSettings;

	// Reset function so we can have default values when the users are creating the structs, but when we're actually constructing the settings with all the combined vertex color channels settings etc. we can easily reset the struct we create so it's all based from 0, so we're not accidentically adding on to the default values. 
	void ResetAllClothSettings() {
		ClothDampingSettings.SetDamping_dampingCoefficient = 0;

		ClothGravitySettings.SetGravity_gravityOverride = FVector(ForceInitToZero);
		ClothGravitySettings.SetGravity_gravityScale = 0;
		ClothGravitySettings.SetGravity_overrideGravity = false;

		ClothWindSettings.SetWind_airDensity = 0;
		ClothWindSettings.SetWind_drag = FVector2D(ForceInitToZero);
		ClothWindSettings.SetWind_lift = FVector2D(ForceInitToZero);
		ClothWindSettings.SetWind_windVelocity = FVector(ForceInitToZero);

		ClothAnimDriveSettings.SetAnimDrive_Damping = FVector2D(ForceInitToZero);
		ClothAnimDriveSettings.SetAnimDrive_Stiffness = FVector2D(ForceInitToZero);

		ClothCollisionSettings.SetCollision_CollisionThickness = 0;
		ClothCollisionSettings.SetCollision_FrictionCoefficient = 0;
		ClothCollisionSettings.SetCollision_SelfCollisionThickness = 0;
		ClothCollisionSettings.SetCollision_UseCCD = false;

		ClothLongRangeAttachmentSettings.LongRangeAttachment_TetherScale = FVector2D(ForceInitToZero);
		ClothLongRangeAttachmentSettings.LongRangeAttachment_TetherThickness = FVector2D(ForceInitToZero);

		ClothMaterialSettings.Material_AreaStiffness = FVector2D(ForceInitToZero);
		ClothMaterialSettings.Material_BendingStiffness = FVector2D(ForceInitToZero);
		ClothMaterialSettings.Material_EdgeStiffness = FVector2D(ForceInitToZero);

		ClothPhysicsVelocityScaleSettings.PhysicsVelocityScale_FictitiousAngularVelocityScale = 0;
		ClothPhysicsVelocityScaleSettings.PhysicsVelocityScale_LinearVelocityScale = FVector(ForceInitToZero);
		ClothPhysicsVelocityScaleSettings.PhysicVelocityScale_AngularVelocityScale = 0;

		ClothPhysicssAirPressureSettings.SetPressure_Pressure = FVector2D(ForceInitToZero);
	}
};
