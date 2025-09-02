// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Chaos/ChaosEngineInterface.h"
#include "VertexPaintMaterialDataAsset.generated.h"


class UMaterialInterface;


//-------------------------------------------------------

// Physics Surface Blend Settings

USTRUCT(BlueprintType, meta = (DisplayName = "Physics Surface Blend Settings"))
struct FRVPDPPhysicsSurfaceBlendSettings {

	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	TSoftObjectPtr<UMaterialInterface> AssociatedMaterial = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	TArray<TEnumAsByte<EPhysicalSurface>> PhysicsSurfacesThatCanBlend;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	float MinAmountOnEachSurfaceToBeAbleToBlend = 0.5f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	TEnumAsByte<EPhysicalSurface> PhysicsSurfaceToResultIn = EPhysicalSurface::SurfaceType_Default;
};


//-------------------------------------------------------

// Registered Material Setting

USTRUCT(BlueprintType, meta = (DisplayName = "Registered Material Setting"))
struct FRVPDPRegisteredMaterialSetting {

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Will include the default channel when running detection where it will return 1 AtDefault value in the VertexPaintDetectionComponent if nothing is painted over it. 0.5 if one thing is painted, 0.33 if two things is fully painted over it etc. \nCan be set to false if you have a material setup where whatever you're painting completely hides what is default."))
	bool IncludeDefaultChannelWhenDetecting = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	TEnumAsByte<EPhysicalSurface> AtDefault = EPhysicalSurface::SurfaceType_Default;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	TEnumAsByte<EPhysicalSurface> PaintedAtRed = EPhysicalSurface::SurfaceType_Default;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	TEnumAsByte<EPhysicalSurface> PaintedAtGreen = EPhysicalSurface::SurfaceType_Default;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	TEnumAsByte<EPhysicalSurface> PaintedAtBlue = EPhysicalSurface::SurfaceType_Default;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	TEnumAsByte<EPhysicalSurface> PaintedAtAlpha = EPhysicalSurface::SurfaceType_Default;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	TMap<TEnumAsByte<EPhysicalSurface>, FRVPDPPhysicsSurfaceBlendSettings> PhysicsSurfaceBlendingSettings;
};


//-------------------------------------------------------

// Registered Physics Surfaces Settings

USTRUCT(BlueprintType, meta = (DisplayName = "Registered Physics Surfaces Settings"))
struct FRVPDPRegisteredPhysicsSurfacesSettings {

	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin")
	TArray <TEnumAsByte<EPhysicalSurface>> ChildSurfaces;
};



/**
 *
 */
UCLASS(Blueprintable, BlueprintType)
class VERTEXPAINTDETECTIONPLUGIN_API UVertexPaintMaterialDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:


	//---------- MATERIALS ----------//

	bool IsMaterialAddedToPaintOnMaterialDataAsset(TSoftObjectPtr<UMaterialInterface> Material);

	TMap<TSoftObjectPtr<UMaterialInterface>, FRVPDPRegisteredMaterialSetting> GetVertexPaintMaterialInterface() { return VertexPaintMaterialInterfaces; }

	UMaterialInterface* GetRegisteredMaterialFromMaterialInterface(UMaterialInterface* Material);



	//---------- PARENT & CHILD PHYSICS SURFACES ----------//

	TMap<TEnumAsByte<EPhysicalSurface>, FRVPDPRegisteredPhysicsSurfacesSettings> GetPhysicsSurfaceFamilies() { return PhysicsSurfaceFamilies; }

	TArray<TEnumAsByte<EPhysicalSurface>> GetParentsOfPhysicsSurface(TEnumAsByte<EPhysicalSurface> PhysicalSurface) const;


#if WITH_EDITOR

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin")
	void AddToVertexPaintMaterialInterface(TSoftObjectPtr<UMaterialInterface> Material, FRVPDPRegisteredMaterialSetting MaterialDataAssetStruct) { VertexPaintMaterialInterfaces.Add(Material, MaterialDataAssetStruct); }

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin")
	void RemoveFromVertexPaintMaterialInterface(TSoftObjectPtr<UMaterialInterface> Material) { VertexPaintMaterialInterfaces.Remove(Material); } // NOTE Doesn't have a .Contains check here because then we can't remove it if the Material was Forced deleted and the TMap had null

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin")
		void AddPhysicsSurfaceFamily(TEnumAsByte<EPhysicalSurface> ParentPhysicsSurface, FRVPDPRegisteredPhysicsSurfacesSettings parentPhysicsSurfaceStruct) { PhysicsSurfaceFamilies.Add(ParentPhysicsSurface, parentPhysicsSurfaceStruct); }

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin")
		void RemovePhysicsSurfaceFamily(TEnumAsByte<EPhysicalSurface> ParentPhysicsSurface);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin")
		void AddChildToPhysicsSurfaceFamily(TEnumAsByte<EPhysicalSurface> ParentPhysicsSurface, TEnumAsByte<EPhysicalSurface> ChildPhysicsSurface);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin")
		void RemoveChildFromPhysicsSurfaceFamily(TEnumAsByte<EPhysicalSurface> ParentPhysicsSurface, TEnumAsByte<EPhysicalSurface> ChildPhysicsSurface);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin")
		void ClearAllPhysicsSurfaceFamilies() { PhysicsSurfaceFamilies.Empty(); }

#endif
	

private:

	// TMap of All Materials that uses Vertex Colors
	UPROPERTY(VisibleAnywhere, Category = "Runtime Vertex Paint and Detection Plugin")
		TMap<TSoftObjectPtr<UMaterialInterface>, FRVPDPRegisteredMaterialSetting> VertexPaintMaterialInterfaces;


	UPROPERTY(VisibleAnywhere, Category = "Runtime Vertex Paint and Detection Plugin")
		TMap<TEnumAsByte<EPhysicalSurface>, FRVPDPRegisteredPhysicsSurfacesSettings> PhysicsSurfaceFamilies;
};
