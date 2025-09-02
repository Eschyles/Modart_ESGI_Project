// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "FundementalsPrerequisites.h"
#include "FallOffPrerequisites.h"
#include "CollisionQueryParams.h"
#include "WithinAreaPrerequisites.generated.h" 


//-------------------------------------------------------

UENUM(BlueprintType)
enum class EPaintWithinAreaShape : uint8 {

	IsSquareOrRectangleShape = 0 UMETA(DisplayName = "Is Square or Rectangle Shape", Tooltip = "Fast as they only check if the vertex is within Bounds"),
	IsSphereShape = 1 UMETA(DisplayName = "Is Sphere Shape", Tooltip = "Fast as they only check if the vertex is within Bounds"),
	IsComplexShape = 2 UMETA(DisplayName = "Is Complex Shape", Tooltip = "Complex Shapes are Skeletal Meshes, Spline Meshes, or any shapped Static Mesh. These are Slower as we run traces for each vertex to check if in the complex shape. "),
	IsCapsuleShape = 3 UMETA(DisplayName = "Is Capsule Shape", Tooltip = "Fast as we can use math to check if vertex is in shape. "),
	IsConeShape = 4 UMETA(DisplayName = "Is Cone Shape", Tooltip = "Fast as we can use math to check if vertex is in shape. "),
};


//-------------------------------------------------------

// Paint Within Area Falloff Type

UENUM(BlueprintType)
enum class EVertexPaintWithinAreaFallOffType : uint8 {

	SphericalFromCenter = 0 UMETA(DisplayName = "Spherical From Center", Tooltip = "Falloff from Center outward. "),
	GradiantUpward = 1 UMETA(DisplayName = "Gradiant Upward", Tooltip = "Falloff from Bottom Upward in Relative Space to how the component to check if within is rotated. "),
	GradiantDownward = 2 UMETA(DisplayName = "Gradiant Downward", Tooltip = "Falloff from Top Downward in Relative Space to how the component to check if within is rotated."),
	InverseGradiant = 3 UMETA(DisplayName = "Inverse Gradiant", Tooltip = "Falloff from Center Outward with a Gradiant in Relative Space to how the component to check if within is rotated. "),
};


//-------------------------------------------------------

// Paint Within Area FallOff Settings

USTRUCT(BlueprintType, meta = (DisplayName = "Paint Within Area Fall Off Settings"))
struct FRVPDPPaintWithinAreaFallOffSettings : public FRVPDPVertexPaintFallOffSettings {

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Whether we calculate the Falloff from the components center or from the components Z Height, so only the distance from the components Z will matter. This is great for large components like oceans if you want some falloff but the mesh being painted is very far from the components X and Y, but they won't matter since we only use the Z. "))
	EVertexPaintWithinAreaFallOffType PaintWithinAreaFallOffType = EVertexPaintWithinAreaFallOffType::SphericalFromCenter;
};


//-------------------------------------------------------

// Component To Check If Is Within Area Info

USTRUCT(BlueprintType, meta = (DisplayName = "Component To Check If Is Within Area Info"))
struct FRVPDPComponentToCheckIfIsWithinAreaInfo {

	GENERATED_BODY()

	UPrimitiveComponent* GetComponentToCheckIfIsWithin() const {

		if (ComponentToCheckIfIsWithinWeak.IsValid())
			return ComponentToCheckIfIsWithinWeak.Get();

		return nullptr;
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The Primitive Component to check if within, for example a Cube, Rectangle, Sphere or even a Skeletal Mesh, Spline Mesh or Landscape if you opt for Complex Shape. "))
	UPrimitiveComponent* ComponentToCheckIfIsWithin = nullptr;

	UPROPERTY()
	TWeakObjectPtr<UPrimitiveComponent> ComponentToCheckIfIsWithinWeak = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Which shape to use, i.e. shape of the Component passed in. Cube/Rectangle and Sphere is the Cheapest. Complex does a line trace on each vertex to check if its within it, which makes it more expensive, but can support any kind of shape and is required for Skeletal, Spline Mesh and Landscapes. "))
	EPaintWithinAreaShape PaintWithinAreaShape = EPaintWithinAreaShape::IsSquareOrRectangleShape;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Paint Within Area supports Falloff as well, where you could have it so at the center of a Sphere/Cube you get more strength than at the edges. Can be useful if you have for instance a Torch that should Melt Snow with stronger effect at it's center, or some falloff on the topside of a lake so it looks like water is smoothly being soaked up to a characters clothes. \nNote that with Cube/Rectangle it will scale the falloff to the outmost corner of the rectangle if using Spherical From Center. If using Gradiant Upward/Downward then it will scale it just to the Z Extent of the Bounds (plus any Extra Extent of course). "))
	FRVPDPPaintWithinAreaFallOffSettings FallOffSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If Shape is set to Complex, then you can choose if the Trace to Check if in the Area should be run with Trace Complex. May be useful if you want to check if inside a skeletal mesh or something similar. "))
	bool TraceComplexIfComplexShape = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Extra Extent to Consider, useful if for instance a character is standing in a body of water, and you want them to get wet slightly above where the water is as well, as if their clothes is soaking up the water. \nNOTE Doesn't support Complex Shape. "))
	float ExtraExtentOutsideOfArea = 0;


	FTransform ComponentWorldTransform;

	FVector ComponentRelativeLocationToMeshBeingPainted = FVector(ForceInitToZero);
	FRotator ComponentRelativeRotationToMeshBeingPainted = FRotator(ForceInitToZero);
	FVector ComponentRelativeScaleToMeshBeingPainted = FVector(ForceInitToZero);

	// Caches these as well so i don't have to go through the transform->GetRotator->GetForwardVector as i want to reduce as many calculations etc. per vertex we're looping through
	FVector ComponentForwardVector = FVector(ForceInitToZero);
	FVector ComponentRightVector = FVector(ForceInitToZero);
	FVector ComponentUpVector = FVector(ForceInitToZero);

	FBoxSphereBounds CleanAggGeomBounds = FBoxSphereBounds();
	FBoxSphereBounds ComponentBounds = FBoxSphereBounds();
	FVector ActorBounds_Origin = FVector(ForceInitToZero);
	FVector ActorBounds_Extent = FVector(ForceInitToZero);

	// Fall Off
	float FallOff_WithinAreaOfEffect = 0;
	float FallOff_ScaleFrom = 0;

	// Square/Rectangle Shape
	float SquareOrRectangle_XHalfSize = 0;
	float SquareOrRectangle_YHalfSize = 0;
	float SquareOrRectangle_ZHalfSize = 0;
	FVector SquareOrRectangle_ForwardPosLocal = FVector(ForceInitToZero);
	FVector SquareOrRectangle_BackPosLocal = FVector(ForceInitToZero);
	FVector SquareOrRectangle_LeftPosLocal = FVector(ForceInitToZero);
	FVector SquareOrRectangle_RightPosLocal = FVector(ForceInitToZero);
	FVector SquareOrRectangle_DownPosLocal = FVector(ForceInitToZero);
	FVector SquareOrRectangle_UpPosLocal = FVector(ForceInitToZero);


	// Sphere Shape
	float Sphere_Radius = 0;

	// Complex Shape
	FCollisionQueryParams Complex_TraceParams;

	// Capsule Shape
	float Capsule_Radius = 0;
	float Capsule_HalfHeight = 0;
	FVector Capsule_TopPoint = FVector(ForceInitToZero);
	FVector Capsule_TopPoint_Local = FVector(ForceInitToZero);
	FVector Capsule_BottomPoint = FVector(ForceInitToZero);
	FVector Capsule_BottomPoint_Local = FVector(ForceInitToZero);
	FQuat Capsule_Orientation = FQuat(ForceInitToZero);

	// Cone Shape
	FVector Cone_Origin = FVector(ForceInitToZero);
	FVector Cone_Direction = FVector(ForceInitToZero);
	float Cone_AngleInDegrees = 0;
	float Cone_Height = 0;
	float Cone_SlantHeight = 0;
	float Cone_Radius = 0;
	FTransform Cone_CenterBottomTransform;
	FTransform Cone_CenterTransform;
};


//-------------------------------------------------------

// Within Area Settings

USTRUCT(BlueprintType, meta = (DisplayName = "Within Area Settings"))
struct FRVPDPWithinAreaSettings {

	GENERATED_BODY()

	FRVPDPWithinAreaSettings() {

		IncludeAmountOfColorsOfEachChannelWithinArea.IncludeVertexColorChannelResultOfEachChannel = false;
		IncludeAmountOfColorsOfEachChannelWithinArea.IncludePhysicsSurfaceResultOfEachChannel = false;
	}

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The Primitive Components to check if within, for example a Cube, Rectangle, Sphere or even a Skeletal Mesh if you opt for Complex Shape. "))
	TArray<FRVPDPComponentToCheckIfIsWithinAreaInfo> ComponentsToCheckIfIsWithin;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If we should include the vertex colors that is within the area. "))
	bool IncludeVertexColorsWithinArea = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If we should include the vertex positions in component space that is within the area. "))
	bool IncludeVertexPositionsWithinArea = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If we should include the vertex normals that is within the area. "))
	bool IncludeVertexNormalsWithinArea = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If we should include the vertex indexes that is within the area. NOTE That this doesn't match whatever index the vertex colors, position or normal arrays has as this is the true vertex index of those within the area. "))
	bool IncludeVertexIndexesWithinArea = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If we should include the amount of colors of each channel, such as amount, average strength and % of each channel and physics surface. "))
	FRVPDPIncludeAmountOfPaintedColorsOfEachChannelSettings IncludeAmountOfColorsOfEachChannelWithinArea;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If meshComponent is a Skeletal Mesh Component and is Registered in the Optimization Data Asset through the Plugins Editor Widget, then if the componentToCheckIfWithin is either Box/Rectangle or Sphere Shape and nothing in the callback settings require us to loop through all of the vertices, we will do a Multi Box or Sphere Trace on the Object Type of the Mesh Component we're trying to Paint/Detect, to get the actual Bones within the Area. If gotten some bones we will only loop through the vertices on those Bones which can make the task finish much faster. We will also include child and parent bones if they had no collision, since we can't get a hit on those parents/childs with a trace, so if for instance tracing and hitting a hand_r, then the fingers will be included as well. "))
	bool TraceForSpecificBonesWithinArea = false;

	TArray<FName> SpecificBonesWithinArea;
};