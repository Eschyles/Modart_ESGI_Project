// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "VertexPaintDetectionLog.h"
#include "Chaos/ChaosEngineInterface.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"
#include "ColorSnippetPrerequisites.h"
#include "FundementalsPrerequisites.generated.h" 



//-------------------------------------------------------

// Additional Data To Pass Through

USTRUCT(BlueprintType, meta = (DisplayName = "Additional Data To Pass Through Info"))
struct FRVPDPAdditionalDataToPassThroughInfo {

	GENERATED_BODY()

	bool operator==(const FRVPDPAdditionalDataToPassThroughInfo& Other) const {

		if (PassThrough_Actor1 == Other.PassThrough_Actor1 &&
			PassThrough_Actor2 == Other.PassThrough_Actor2 &&
			PassThrough_PrimitiveComponent1 == Other.PassThrough_PrimitiveComponent1 &&
			PassThrough_PrimitiveComponent2 == Other.PassThrough_PrimitiveComponent2 &&
			PassThrough_Bool1 == Other.PassThrough_Bool1 &&
			PassThrough_Bool2 == Other.PassThrough_Bool2 &&
			PassThrough_Bool3 == Other.PassThrough_Bool3 &&
			PassThrough_Byte1 == Other.PassThrough_Byte1 &&
			PassThrough_Byte2 == Other.PassThrough_Byte2 &&
			PassThrough_Float1 == Other.PassThrough_Float1 &&
			PassThrough_Float2 == Other.PassThrough_Float2 &&
			PassThrough_Float3 == Other.PassThrough_Float3 &&
			PassThrough_Float4 == Other.PassThrough_Float4 &&
			PassThrough_Int1 == Other.PassThrough_Int1 &&
			PassThrough_Int2 == Other.PassThrough_Int2 &&
			PassThrough_Vector1 == Other.PassThrough_Vector1 &&
			PassThrough_Vector2 == Other.PassThrough_Vector2 &&
			PassThrough_Vector3 == Other.PassThrough_Vector3 &&
			PassThrough_Vector4 == Other.PassThrough_Vector4 &&
			PassThrough_Rotator1 == Other.PassThrough_Rotator1 &&
			PassThrough_Rotator2 == Other.PassThrough_Rotator2 &&
			PassThrough_String1 == Other.PassThrough_String1 &&
			PassThrough_String2 == Other.PassThrough_String2 &&
			PassThrough_Name1 == Other.PassThrough_Name1 &&
			PassThrough_Name2 == Other.PassThrough_Name2 &&
			PassThrough_PhysicsSurface1 == Other.PassThrough_PhysicsSurface1 &&
			PassThrough_PhysicsSurface2 == Other.PassThrough_PhysicsSurface2 &&
			PassThrough_PhysicalMaterial1 == Other.PassThrough_PhysicalMaterial1 &&
			PassThrough_ObjectCollisionChannel == Other.PassThrough_ObjectCollisionChannel &&
			PassThrough_ObjectType == Other.PassThrough_ObjectType &&
			PassThrough_RandomStream.GetCurrentSeed() == Other.PassThrough_RandomStream.GetCurrentSeed() &&
			PassThrough_UObject1 == Other.PassThrough_UObject1 &&
			PassThrough_UObject2 == Other.PassThrough_UObject2) {

			return true;
		}

		return false;
	}

	bool operator!=(const FRVPDPAdditionalDataToPassThroughInfo& Other) const {

		return !(*this == Other);
	}


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "These can be used effectively if several tasks is dependent on the results of eachother, so you don't have to cache global variables but can just pass through the ones you want when the task is finished. "))
	AActor* PassThrough_Actor1 = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "These can be used effectively if several tasks is dependent on the results of eachother, so you don't have to cache global variables but can just pass through the ones you want when the task is finished. "))
	AActor* PassThrough_Actor2 = nullptr;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "These can be used effectively if several tasks is dependent on the results of eachother, so you don't have to cache global variables but can just pass through the ones you want when the task is finished. "))
	UPrimitiveComponent* PassThrough_PrimitiveComponent1 = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "These can be used effectively if several tasks is dependent on the results of eachother, so you don't have to cache global variables but can just pass through the ones you want when the task is finished. "))
	UPrimitiveComponent* PassThrough_PrimitiveComponent2 = nullptr;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "These can be used effectively if several tasks is dependent on the results of eachother, so you don't have to cache global variables but can just pass through the ones you want when the task is finished. "))
	bool PassThrough_Bool1 = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "These can be used effectively if several tasks is dependent on the results of eachother, so you don't have to cache global variables but can just pass through the ones you want when the task is finished. "))
	bool PassThrough_Bool2 = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "These can be used effectively if several tasks is dependent on the results of eachother, so you don't have to cache global variables but can just pass through the ones you want when the task is finished. "))
	bool PassThrough_Bool3 = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "These can be used effectively if several tasks is dependent on the results of eachother, so you don't have to cache global variables but can just pass through the ones you want when the task is finished. "))
	uint8 PassThrough_Byte1 = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "These can be used effectively if several tasks is dependent on the results of eachother, so you don't have to cache global variables but can just pass through the ones you want when the task is finished. "))
	uint8 PassThrough_Byte2 = 0;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "These can be used effectively if several tasks is dependent on the results of eachother, so you don't have to cache global variables but can just pass through the ones you want when the task is finished. "))
	float PassThrough_Float1 = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "These can be used effectively if several tasks is dependent on the results of eachother, so you don't have to cache global variables but can just pass through the ones you want when the task is finished. "))
	float PassThrough_Float2 = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "These can be used effectively if several tasks is dependent on the results of eachother, so you don't have to cache global variables but can just pass through the ones you want when the task is finished. "))
	float PassThrough_Float3 = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "These can be used effectively if several tasks is dependent on the results of eachother, so you don't have to cache global variables but can just pass through the ones you want when the task is finished. "))
	float PassThrough_Float4 = 0;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "These can be used effectively if several tasks is dependent on the results of eachother, so you don't have to cache global variables but can just pass through the ones you want when the task is finished. "))
	int PassThrough_Int1 = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "These can be used effectively if several tasks is dependent on the results of eachother, so you don't have to cache global variables but can just pass through the ones you want when the task is finished. "))
	int PassThrough_Int2 = 0;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "These can be used effectively if several tasks is dependent on the results of eachother, so you don't have to cache global variables but can just pass through the ones you want when the task is finished. \nThis could for instance Location, Direction, Velocity, Normal etc. "))
	FVector PassThrough_Vector1 = FVector(ForceInitToZero);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "These can be used effectively if several tasks is dependent on the results of eachother, so you don't have to cache global variables but can just pass through the ones you want when the task is finished. \nThis could for instance Location, Direction, Velocity etc. "))
	FVector PassThrough_Vector2 = FVector(ForceInitToZero);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "These can be used effectively if several tasks is dependent on the results of eachother, so you don't have to cache global variables but can just pass through the ones you want when the task is finished. \nThis could for instance Location, Direction, Velocity etc. "))
	FVector PassThrough_Vector3 = FVector(ForceInitToZero);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "These can be used effectively if several tasks is dependent on the results of eachother, so you don't have to cache global variables but can just pass through the ones you want when the task is finished. \nThis could for instance Location, Direction, Velocity etc. "))
	FVector PassThrough_Vector4 = FVector(ForceInitToZero);


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "These can be used effectively if several tasks is dependent on the results of eachother, so you don't have to cache global variables but can just pass through the ones you want when the task is finished."))
	FRotator PassThrough_Rotator1 = FRotator(ForceInitToZero);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "These can be used effectively if several tasks is dependent on the results of eachother, so you don't have to cache global variables but can just pass through the ones you want when the task is finished."))
	FRotator PassThrough_Rotator2 = FRotator(ForceInitToZero);


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "These can be used effectively if several tasks is dependent on the results of eachother, so you don't have to cache global variables but can just pass through the ones you want when the task is finished. "))
	FString PassThrough_String1 = "";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "These can be used effectively if several tasks is dependent on the results of eachother, so you don't have to cache global variables but can just pass through the ones you want when the task is finished. "))
	FString PassThrough_String2 = "";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "These can be used effectively if several tasks is dependent on the results of eachother, so you don't have to cache global variables but can just pass through the ones you want when the task is finished. "))
	FName PassThrough_Name1 = "";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "These can be used effectively if several tasks is dependent on the results of eachother, so you don't have to cache global variables but can just pass through the ones you want when the task is finished. "))
	FName PassThrough_Name2 = "";


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "These can be used effectively if several tasks is dependent on the results of eachother, so you don't have to cache global variables but can just pass through the ones you want when the task is finished. "))
	TEnumAsByte<EPhysicalSurface> PassThrough_PhysicsSurface1 = EPhysicalSurface::SurfaceType_Default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "These can be used effectively if several tasks is dependent on the results of eachother, so you don't have to cache global variables but can just pass through the ones you want when the task is finished. "))
	TEnumAsByte<EPhysicalSurface> PassThrough_PhysicsSurface2 = EPhysicalSurface::SurfaceType_Default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "These can be used effectively if several tasks is dependent on the results of eachother, so you don't have to cache global variables but can just pass through the ones you want when the task is finished. "))
	UPhysicalMaterial* PassThrough_PhysicalMaterial1 = nullptr;



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "These can be used effectively if several tasks is dependent on the results of eachother, so you don't have to cache global variables but can just pass through the ones you want when the task is finished. "))
	TEnumAsByte<ECollisionChannel> PassThrough_ObjectCollisionChannel = ECollisionChannel::ECC_WorldStatic;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "These can be used effectively if several tasks is dependent on the results of eachother, so you don't have to cache global variables but can just pass through the ones you want when the task is finished. "))
	TEnumAsByte<EObjectTypeQuery> PassThrough_ObjectType = EObjectTypeQuery::ObjectTypeQuery1;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "These can be used effectively if several tasks is dependent on the results of eachother, so you don't have to cache global variables but can just pass through the ones you want when the task is finished. "))
	FRandomStream PassThrough_RandomStream = FRandomStream(ForceInitToZero);


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "These can be used effectively if several tasks is dependent on the results of eachother, so you don't have to cache global variables but can just pass through the ones you want when the task is finished. "))
	UObject* PassThrough_UObject1 = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "These can be used effectively if several tasks is dependent on the results of eachother, so you don't have to cache global variables but can just pass through the ones you want when the task is finished. "))
	UObject* PassThrough_UObject2 = nullptr;
};


//-------------------------------------------------------

// Multi Thread Settings 

USTRUCT(BlueprintType, meta = (DisplayName = "Multi Thread Settings"))
struct FRVPDPMultiThreadSettings {

	GENERATED_BODY()

	bool operator==(const FRVPDPMultiThreadSettings& Other) const {

		if (UseMultithreadingForCalculations == Other.UseMultithreadingForCalculations) {

			return true;
		}

		return false;
	}

	bool operator!=(const FRVPDPMultiThreadSettings& Other) const {

		return !(*this == Other);
	}


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Whether to use Multithreading for Calculations instead of using Game Thread. Is recommended since you get will get better FPS. Each Mesh will have their own Calculation Task queue where each paint job is based off of the result of the previous. Detection Jobs will be set to be first in the queue so they will run the fastest. "))
	bool UseMultithreadingForCalculations = true;
};


//-------------------------------------------------------

// Include Amount Of Painted Colors Of Each Channel Settings

USTRUCT(BlueprintType, meta = (DisplayName = "Include Amount Of Painted Colors Of Each Channel Settings"))
struct FRVPDPIncludeAmountOfPaintedColorsOfEachChannelSettings {

	GENERATED_BODY()

	bool operator==(const FRVPDPIncludeAmountOfPaintedColorsOfEachChannelSettings& Other) const {

		if (IncludeVertexColorChannelResultOfEachChannel == Other.IncludeVertexColorChannelResultOfEachChannel &&
			IncludePhysicsSurfaceResultOfEachChannel == Other.IncludePhysicsSurfaceResultOfEachChannel &&
			IncludeIfMinColorAmountIs == Other.IncludeIfMinColorAmountIs &&
			IncludeOnlyIfPhysicsSurfacesIsRegisteredToAnyVertexChannel == Other.IncludeOnlyIfPhysicsSurfacesIsRegisteredToAnyVertexChannel) {

			return true;
		}

		return false;
	}

	bool operator!=(const FRVPDPIncludeAmountOfPaintedColorsOfEachChannelSettings& Other) const {

		return !(*this == Other);
	}


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If we should get the Percent, Average Amount, and How many vertices on each RGBA Vertex Color Channel, so you can for instance check if the Red Channel has is over a certain %. \n This is really useful if you have something like splaboon, where you want to know, how much in Percent of Red and Blue is on this Mesh above this minimum amount. \nNOTE That if True and if we're running a paint/detect job that usually doesn't have to loop through the vertices, like Get All Colors Only, SetMeshComponentColors, or Paint Color Snippet, then this will make it loop through the vertices so we can get the data, meaning with this included those task types may take a tiny bit longer to finish. Not as long as a regular task like Paint at Location since we're not looping through Sections etc. though. "))
		bool IncludeVertexColorChannelResultOfEachChannel = false;
		
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If True then we will get the Percent, Amount and Average Amount for every Physics Surface that was registered to each color channel on the Mesh Materials, so you can for instance get that Sand had X %, and X amount of Vertices etc. In the callback results the first in the TMap will be the one with the highest %. \nNOTE that if this is true, then the task may take roughly 25% longer time to finish since it requires a few more loops for every vertex, and paint/detect jobs that usually don't need to loop through sections of the mesh like Get All Colors Only, Paint Color Snippet or Paint Entire Mesh with Set, will have to do so, meaning with this true the task will take just as long as a regular Paint at Location that has this to true for instance. "))
		bool IncludePhysicsSurfaceResultOfEachChannel = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If Min this Amount of color value on R, G, B or A then they will be included. So if you for instance want to check if something is fully painted on a channel, then you can set this to like 0.999"))
		float IncludeIfMinColorAmountIs = 0.001;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If we should optionally Only include results on channels that has either of these surfaces registered, or is a part of their family. For instance if you have a Physics Surface Family Parent Registered as Wet, and a Cobble-Puddle as child, then you can add Wet here and even if the Material has Cobble-Puddle on it that will be included. \nUseful if you're looking for the results of a specific physics surface and don't care about anything else. "))
		TArray<TEnumAsByte<EPhysicalSurface>> IncludeOnlyIfPhysicsSurfacesIsRegisteredToAnyVertexChannel;
};


//-------------------------------------------------------

// Callback From Specified Mesh Components Info

USTRUCT()
struct FRVPDPCallbackFromSpecifiedMeshComponentsInfo {

	GENERATED_BODY()

	bool operator==(const FRVPDPCallbackFromSpecifiedMeshComponentsInfo& Other) const {

		if (RunCallbacksOnObjects == Other.RunCallbacksOnObjects) {

			return true;
		}

		return false;
	}

	bool operator!=(const FRVPDPCallbackFromSpecifiedMeshComponentsInfo& Other) const {

		return !(*this == Other);
	}


	UPROPERTY()
	TArray<TWeakObjectPtr<UObject>> RunCallbacksOnObjects;
};


//-------------------------------------------------------

// Compare Mesh Vertex Colors 

USTRUCT(BlueprintType, meta = (DisplayName = "Compare Mesh Vertex Colors Settings Base"))
struct FRVPDPCompareMeshVertexColorsSettings {

	GENERATED_BODY()

	bool operator==(const FRVPDPCompareMeshVertexColorsSettings& Other) const {

		if (SkipEmptyVertices == Other.SkipEmptyVertices &&
			EmptyVertexColor == Other.EmptyVertexColor &&
			ComparisonErrorTolerance == Other.ComparisonErrorTolerance &&
			CompareRedChannel == Other.CompareRedChannel &&
			CompareGreenChannel == Other.CompareGreenChannel &&
			CompareBlueChannel == Other.CompareBlueChannel &&
			CompareAlphaChannel == Other.CompareAlphaChannel &&
			TransformedErrorTolerance == Other.TransformedErrorTolerance &&
			TransformedEmptyVertexColor == Other.TransformedEmptyVertexColor) {

			return true;
		}

		return false;
	}

	bool operator!=(const FRVPDPCompareMeshVertexColorsSettings& Other) const {

		return !(*this == Other);
	}


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If True then will only Compare against those Vertices in the compare array that actually has any colors instead of All of them. For instance if you have a floor and comparing to an array where its a circle in the middle, then only painting/removing colors at the circle will actually make the % go up and down. Otherwise if this is false, then all vertices around the circle will affect the % as well. "))
	bool SkipEmptyVertices = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If a Vertex Colors has this color it will be considered as Empty. Some use cases may benedift from having this as white. "))
	FLinearColor EmptyVertexColor = FLinearColor(0, 0, 0, 0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Error Tolerance between 0-1 how Close the Mesh Vertex Color and the Compare Vertex Color has to be to be considered matched. So if 0 then the colors has to be Exactly the same for it to pass the comparison check, if 1 they can be completely different. \n\nIf your snippets was painted with high falloff strength, i.e. there may be vertices stored with very low paint strength, then it may be good to have a very low error tolerance, since otherwise even vertices on the mesh that are empty may pass the check. For instance if a vertex color from the snippet has value 10 (0-255), and the color from the mesh is 0, then it may pass easily if the tolerance is to high, which may lead to unexpected results. Snippets that was painted without falloff removes this possibility completely. "))
	float ComparisonErrorTolerance = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "You can check just the channels you want to compare if you don't want to compare on all. "))
	bool CompareRedChannel = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "You can check just the channels you want to compare if you don't want to compare on all. "))
	bool CompareGreenChannel = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "You can check just the channels you want to compare if you don't want to compare on all. "))
	bool CompareBlueChannel = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "You can check just the channels you want to compare if you don't want to compare on all. "))
	bool CompareAlphaChannel = true;

	int32 TransformedErrorTolerance = 0;

	FColor TransformedEmptyVertexColor = FColor(0, 0, 0, 0);
};


//-------------------------------------------------------

// Compare Mesh Vertex Colors To Color Array Settings

USTRUCT(BlueprintType, meta = (DisplayName = "Compare Mesh Vertex Colors To Color Array Settings"))
struct FRVPDPCompareMeshVertexColorsToColorArraySettings : public FRVPDPCompareMeshVertexColorsSettings {

	GENERATED_BODY()

	bool operator==(const FRVPDPCompareMeshVertexColorsToColorArraySettings& Other) const {

		if (ColorArrayToCompareWith == Other.ColorArrayToCompareWith) {

			return true;
		}

		return false;
	}

	bool operator!=(const FRVPDPCompareMeshVertexColorsToColorArraySettings& Other) const {

		return !(*this == Other);
	}


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Color Array we will compare it to. Have to match the Vertex Color Array Amount of the Mesh we're going to Paint/Detect. You can get both the vertex color array from any paint or detect job if you set to include them in the callback settings, so you can get them from a mesh and store it for later use. "))
	TArray<FColor> ColorArrayToCompareWith;
};


//-------------------------------------------------------

// Compare Mesh Vertex Colors To Color Snippets Settings

USTRUCT(BlueprintType, meta = (DisplayName = "Compare Mesh Vertex Colors To Color Snippets Settings"))
struct FRVPDPCompareMeshVertexColorsToColorSnippetsSettings : public FRVPDPCompareMeshVertexColorsSettings {

	GENERATED_BODY()

	bool operator==(const FRVPDPCompareMeshVertexColorsToColorSnippetsSettings& Other) const {

		if (CompareWithColorsSnippetDataAssetInfo == Other.CompareWithColorsSnippetDataAssetInfo) {

			return true;
		}

		return false;
	}

	bool operator!=(const FRVPDPCompareMeshVertexColorsToColorSnippetsSettings& Other) const {

		return !(*this == Other);
	}


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Color Snippet we will compare to. Useful if you've for instance have a Color Snippet of a Tatoo or a Pattern, and you want to check how close to it the player has painted. "))
	TArray<FGameplayTag> CompareWithColorsSnippetTag;

	UPROPERTY(BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The Color Snippet Tag but in FString Format and the Data Asset the snippet is stored on. "))
	TArray<FRVPDPColorSnippetDataAssetInfo> CompareWithColorsSnippetDataAssetInfo;
};


//-------------------------------------------------------

// Task Callback Settings

USTRUCT(BlueprintType, meta = (DisplayName = "Task Callback Settings"))
struct FRVPDPTaskCallbackSettings {

	GENERATED_BODY()

	bool operator==(const FRVPDPTaskCallbackSettings& Other) const {

		// Bool Checks
		if (RunCallbackDelegate == Other.RunCallbackDelegate &&
			RunCallbackInterfacesOnObject == Other.RunCallbackInterfacesOnObject &&
			ObjectToRunInterfacesOn == Other.ObjectToRunInterfacesOn &&
			RunCallbackInterfacesOnObjectComponents == Other.RunCallbackInterfacesOnObjectComponents &&
			IncludeVertexDataOnlyForLOD0 == Other.IncludeVertexDataOnlyForLOD0 &&
			IncludeColorsOfEachBone == Other.IncludeColorsOfEachBone &&
			IncludeVertexColorData == Other.IncludeVertexColorData &&
			IncludeSerializedVertexColorData == Other.IncludeSerializedVertexColorData &&
			IncludeVertexPositionData == Other.IncludeVertexPositionData &&
			IncludeVertexNormalData == Other.IncludeVertexNormalData &&
			IncludeVertexIndexes == Other.IncludeVertexIndexes) {

			// Struct Checks
			if (IncludeAmountOfPaintedColorsOfEachChannel == Other.IncludeAmountOfPaintedColorsOfEachChannel &&
				CompareMeshVertexColorsToColorArray == Other.CompareMeshVertexColorsToColorArray &&
				CompareMeshVertexColorsToColorSnippets == Other.CompareMeshVertexColorsToColorSnippets &&
				CallbacksOnObjectsForMeshComponent == Other.CallbacksOnObjectsForMeshComponent) {

				return true;
			}
		}

		return false;
	}

	bool operator!=(const FRVPDPTaskCallbackSettings& Other) const {

		return !(*this == Other);
	}


	// Can only change a property if it's its default value, so we don't change something that has already been changed
	void AppendCallbackSettingsToDefaults(const FRVPDPTaskCallbackSettings& CallbackSettingsToAppend) {

		if (RunCallbackDelegate == FRVPDPTaskCallbackSettings().RunCallbackDelegate)
			RunCallbackDelegate = CallbackSettingsToAppend.RunCallbackDelegate;

		if (RunCallbackInterfacesOnObject == FRVPDPTaskCallbackSettings().RunCallbackInterfacesOnObject)
			RunCallbackInterfacesOnObject = CallbackSettingsToAppend.RunCallbackInterfacesOnObject;

		if (ObjectToRunInterfacesOn == FRVPDPTaskCallbackSettings().ObjectToRunInterfacesOn)
			ObjectToRunInterfacesOn = CallbackSettingsToAppend.ObjectToRunInterfacesOn;

		if (RunCallbackInterfacesOnObjectComponents == FRVPDPTaskCallbackSettings().RunCallbackInterfacesOnObjectComponents)
			RunCallbackInterfacesOnObjectComponents = CallbackSettingsToAppend.RunCallbackInterfacesOnObjectComponents;

		if (IncludeVertexDataOnlyForLOD0 == FRVPDPTaskCallbackSettings().IncludeVertexDataOnlyForLOD0)
			IncludeVertexDataOnlyForLOD0 = CallbackSettingsToAppend.IncludeVertexDataOnlyForLOD0;

		if (IncludeColorsOfEachBone == FRVPDPTaskCallbackSettings().IncludeColorsOfEachBone)
			IncludeColorsOfEachBone = CallbackSettingsToAppend.IncludeColorsOfEachBone;

		if (IncludeAmountOfPaintedColorsOfEachChannel == FRVPDPTaskCallbackSettings().IncludeAmountOfPaintedColorsOfEachChannel)
			IncludeAmountOfPaintedColorsOfEachChannel = CallbackSettingsToAppend.IncludeAmountOfPaintedColorsOfEachChannel;

		if (CompareMeshVertexColorsToColorArray == FRVPDPTaskCallbackSettings().CompareMeshVertexColorsToColorArray)
			CompareMeshVertexColorsToColorArray = CallbackSettingsToAppend.CompareMeshVertexColorsToColorArray;

		if (CompareMeshVertexColorsToColorSnippets == FRVPDPTaskCallbackSettings().CompareMeshVertexColorsToColorSnippets)
			CompareMeshVertexColorsToColorSnippets = CallbackSettingsToAppend.CompareMeshVertexColorsToColorSnippets;

		if (IncludeVertexColorData == FRVPDPTaskCallbackSettings().IncludeVertexColorData)
			IncludeVertexColorData = CallbackSettingsToAppend.IncludeVertexColorData;

		if (IncludeSerializedVertexColorData == FRVPDPTaskCallbackSettings().IncludeSerializedVertexColorData)
			IncludeSerializedVertexColorData = CallbackSettingsToAppend.IncludeSerializedVertexColorData;

		if (IncludeVertexPositionData == FRVPDPTaskCallbackSettings().IncludeVertexPositionData)
			IncludeVertexPositionData = CallbackSettingsToAppend.IncludeVertexPositionData;

		if (IncludeVertexNormalData == FRVPDPTaskCallbackSettings().IncludeVertexNormalData)
			IncludeVertexNormalData = CallbackSettingsToAppend.IncludeVertexNormalData;

		if (IncludeVertexIndexes == FRVPDPTaskCallbackSettings().IncludeVertexIndexes)
			IncludeVertexIndexes = CallbackSettingsToAppend.IncludeVertexIndexes;

		if (CallbacksOnObjectsForMeshComponent == FRVPDPTaskCallbackSettings().CallbacksOnObjectsForMeshComponent)
			CallbacksOnObjectsForMeshComponent = CallbackSettingsToAppend.CallbacksOnObjectsForMeshComponent;
	}


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If we should run the Paint Components Callback Delegate whether the Task was a Success / Fail. "))
	bool RunCallbackDelegate = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If we should run the interface on the Object. If ObjectToRunInterfacesOn is null then default it to run the interface on the Actor we Paint/Detect On. "))
	bool RunCallbackInterfacesOnObject = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If Null then will default to run Interfaces on the Actor we Painted/Detected, but you can change if it should run on another object. Useful if you for instance have some sort of manager in the level that you want to callbacks to run on when you paint regular static meshes in the level. "))
	TWeakObjectPtr<UObject> ObjectToRunInterfacesOn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If ObjectToRunInterfacesOn is an Actor which can have Components, then if we should run the Paint/Detect Interfaces on the Components of that Actor as well. If it's null then it will default to the actor painted/detect on. "))
	bool RunCallbackInterfacesOnObjectComponents = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If we only return data for LOD0. This can save performance if the mesh is extremely heavy. There was a noticable difference on a mesh that had 980k vertices if only returned LOD0 and not the other LODs. Should only be false if you actually plan on using the data for the other LODs. "))
	bool IncludeVertexDataOnlyForLOD0 = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If looping through all vertices then will get the colors of each bone at LOD0 as well and include it in the callback settings. \nIf True and it is a Task that by default don't loop through vertices like Paint Color Snippet, Paint Entire Mesh with Set, or Detect with GetColorsOnly, then this will still make it loop through the vertices afterwards so it can get data for this. So those tasks that are usually extremely quick will take a bit longer to calculate if this is true, so only recommend setting to true if you actually need the data. "))
	bool IncludeColorsOfEachBone = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If we set amount of vertices that had color for each channel, and amount of percent each channel has as well. Really useful if you have something like splaboon, where you want to know, how much in Percent of Red and Blue is on this Mesh. \nNote that if we're running a paint job that doesn't have to loop through the vertices, like Detect with Get Colors Only, or Paint Color Snippet, then this will make it loop through the vertices after so it can correctly fill the struct. This means that a task like Paint Color Snippet that is usually extremely quick can take a bit longer to finish, unlike tasks like Paint at Location which won't get their calculation speed affected since they're already looping through the Vertices. "))
	FRVPDPIncludeAmountOfPaintedColorsOfEachChannelSettings IncludeAmountOfPaintedColorsOfEachChannel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "With this you can get the Matching Percent between the Meshes Current Vertex Colors (If paint job then colors after paint was applied) and the ones we send in here. \n\nThis is useful if you for instance have a Drawing Game, or a Tutorial where the player has to fill in a pattern that has been pre-filled, or just Mimick the pattern an AI is painting. "))
	FRVPDPCompareMeshVertexColorsToColorArraySettings CompareMeshVertexColorsToColorArray;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "With this you can get the Matching Percent between the Meshes Current Vertex Colors (If paint job then colors after paint was applied) and the Color Snippets set here. \n\nThis is useful if you for instance have a Drawing Game, or a Tutorial where the player has to fill in a pattern that has been pre-filled, or just Mimick the pattern an AI is painting. "))
	FRVPDPCompareMeshVertexColorsToColorSnippetsSettings CompareMeshVertexColorsToColorSnippets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If we include the color data, if false, we can save some performance if the mesh is extremely heavy. There was a noticable difference on a mesh that had 980k vert, but if it's just 50k or so there's not much difference.  "))
	bool IncludeVertexColorData = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If true then we will Serialize the Color Array for each LOD and return an array of strings, each representing color data for each LOD that can be De-Serialized to get the Color Data in Array format again. "))
	bool IncludeSerializedVertexColorData = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If we include the position data, if false, we can save some performance if the mesh is extremely heavy. There was a noticable difference on a mesh that had 980k vertices if we didn't include everything. \nIf True and it is a Task that by default don't loop through all vertices, like Paint Color Snippet, Paint Entire Mesh with Set, or Detect with GetColorsOnly, then this will still make it loop through the vertices afterwards so it can get data for this. Only recommend setting to true if you actually need the data. "))
	bool IncludeVertexPositionData = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If we include the Normal data, if false, we can save some performance if the mesh is extremely heavy. There was a noticable difference on a mesh that had 980k vertices if we didn't include everything. \nIf True and it is a Task that by default don't loop through vertices like Paint Color Snippet, Paint Entire Mesh with Set, or Detect with GetColorsOnly, then this will still make it loop through the vertices afterwards so it can get data for this. Only recommend setting to true if you actually need the data. "))
	bool IncludeVertexNormalData = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If we include the vertex indexes, so you can get them without having to get the color, position and normal data. "))
	bool IncludeVertexIndexes = false;

	UPROPERTY()
	FRVPDPCallbackFromSpecifiedMeshComponentsInfo CallbacksOnObjectsForMeshComponent;
};


//-------------------------------------------------------

// Get Average Color Settings

USTRUCT(BlueprintType, meta = (DisplayName = "Get Average Color Settings"))
struct FRVPDPGetAverageColorSettings {

	GENERATED_BODY()

	bool operator==(const FRVPDPGetAverageColorSettings& Other) const {

		if (GetAverageColor == Other.GetAverageColor &&
			AreaRangeToGetAvarageColorFrom == Other.AreaRangeToGetAvarageColorFrom &&
			VertexNormalToHitNormal_MinimumDotProductToBeAccountedFor == Other.VertexNormalToHitNormal_MinimumDotProductToBeAccountedFor) {

			return true;
		}

		return false;
	}

	bool operator!=(const FRVPDPGetAverageColorSettings& Other) const {

		return !(*this == Other);
	}


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If we should get the average color within the area of effect. "))
	bool GetAverageColor = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The Range we will get the average colors from. Has to be above 0 to get the average color. "))
	float AreaRangeToGetAvarageColorFrom = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If the Vertex Normal and Hit Normal Dot is Min this then takes that vertex colors into account. -1 will take all vertices in the area into account, 1 only the vertices that has the same normal as the Hit Normal. So if you for instance want to get the average of one side of a wall, then you could have it to be 1, otherwise if it's -1 then it will take the vertices on the other side of the wall as well. "))
	float VertexNormalToHitNormal_MinimumDotProductToBeAccountedFor = -1;
};


//-------------------------------------------------------

// Task Fundamental Settings

USTRUCT(BlueprintType, meta = (DisplayName = "Task Fundamental Settings"))
struct FRVPDPTaskFundamentalSettings {

	GENERATED_BODY()

	bool operator==(const FRVPDPTaskFundamentalSettings& Other) const {

		if (TaskWorld == Other.TaskWorld &&
			MeshComponent == Other.MeshComponent &&
			ComponentItem == Other.ComponentItem &&
			CallbackSettings == Other.CallbackSettings &&
			MultiThreadSettings == Other.MultiThreadSettings &&
			OnlyAllowOneTaskForMeshPerPaintComponent == Other.OnlyAllowOneTaskForMeshPerPaintComponent &&
			IgnoreTaskQueueLimit == Other.IgnoreTaskQueueLimit &&
			DebugSettings == Other.DebugSettings) {

			return true;
		}

		return false;
	}

	bool operator!=(const FRVPDPTaskFundamentalSettings& Other) const {

		return !(*this == Other);
	}


	void SetPaintDetectionFundementals(FRVPDPTaskFundamentalSettings fundementals) {

		TaskWorld = fundementals.TaskWorld;
		WeakMeshComponent = fundementals.WeakMeshComponent;
		MeshComponent = fundementals.MeshComponent;
		ComponentItem = fundementals.ComponentItem;
		CallbackSettings = fundementals.CallbackSettings;
		MultiThreadSettings = fundementals.MultiThreadSettings;
		OnlyAllowOneTaskForMeshPerPaintComponent = fundementals.OnlyAllowOneTaskForMeshPerPaintComponent;
		IgnoreTaskQueueLimit = fundementals.IgnoreTaskQueueLimit;
		DebugSettings = fundementals.DebugSettings;
	}

	UPrimitiveComponent* GetMeshComponent() const {

		if (WeakMeshComponent.IsValid()) {
			return WeakMeshComponent.Get();
		}

		return nullptr;
	}

	UWorld* GetTaskWorld() const {

		if (TaskWorld.IsValid())
			return TaskWorld.Get();

		return nullptr;
	}


	UPROPERTY()
	TWeakObjectPtr<UWorld> TaskWorld = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Skeletal or Static Mesh Component to Paint / Detect"))
	UPrimitiveComponent* MeshComponent = nullptr;

	UPROPERTY()
	TWeakObjectPtr<UPrimitiveComponent> WeakMeshComponent = nullptr; 

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "This is used if it was an Instanced Mesh, for example Foliage, so we can get the specific instance and check the correct location from it. You can get this from Trace Hit or Overlap Events. \nIf you're not using Instanced Meshes then you don't have to bother with this at all. "))
	int ComponentItem = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "You can set what you can to return. If you're Painting / Detecting on extremely large meshes like 1 million vertices big, then it can make a bit difference if you don't return any colors, position and normals. "))
	FRVPDPTaskCallbackSettings CallbackSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Recommend running with Multithreading True so the game thread doesn't get affected by the Paint/Detect Job. "))
	FRVPDPMultiThreadSettings MultiThreadSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If True then will automatically run GetMeshRequiredCallbackSettings() and Append them to whatever is already set. This is highly useful since it runs RequiredCallbackSettings Interface on the Mesh Owner we're trying to Paint, as well as checks if it has any Auto Paint Components that are actively painting the Mesh and if so makes sure that the Amount of Colors to Include is set Accordingly. It even checks if some Other objects has binded to receive callbacks to paint jobs on the Mesh, and runs RequiredCallbackSettings() and auto paint component checks on that as well, which may be a common use case for managers like a Weather System. "))
	bool AutoSetRequiredCallbackSettings = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If True, then we will only queue up the Task if the vertex paint compoennt doesn't already have it queued up for the type. So only 1 Paint and 1 Detection Task is allowed at one time per Mesh per vertex paint component if this is true. However, if the Paint Task we want to add is something that will make previous Tasks irrelevant for the Mesh and clear its queue, for example Paint Entire Mesh with Set, Color Snippet or Set Mesh Component Colors, then we will still allow the newly added paint job. \nThis is recommended to ensure that Task Queues cannot grow to large which can easily happen if painting in Tick, and if that happens then it may feel unresponsive because it may take a few frames for the latest added paint job to actually start and show it's effect. "))
	bool OnlyAllowOneTaskForMeshPerPaintComponent = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "If True, we ignore the task queue limit that's set in the Project Settings. It's not recommended to change this, if it grows large then it may take a while before it gets to the latest added paint task, which may make it feel unresponsive. \n\nThis setting is mostly for things like Save/Load Systems, where if you want to Load a Saved Game and have saved vertex colors on a bunch of Meshes, and need to queue up possibly several hundreds of paint tasks at once. "))
	bool IgnoreTaskQueueLimit = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "The Debug Settings for the Task, you can hover over each one to read more about it! \nNOTE Some may require you to turn off multithreading for the task job, since we can't draw debug symbols on an async thread.  "))
	FRVPDPDebugSettings DebugSettings;
};
