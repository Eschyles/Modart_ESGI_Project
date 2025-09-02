// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#include "VertexPaintMaterialDataAsset.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceDynamic.h"


//-------------------------------------------------------

// Is Material Added To Paint On Material Data Asset

bool UVertexPaintMaterialDataAsset::IsMaterialAddedToPaintOnMaterialDataAsset(TSoftObjectPtr<UMaterialInterface> Material) {

	if (!Material) return false;

	TArray<TSoftObjectPtr<UMaterialInterface>> storedMaterials;
	GetVertexPaintMaterialInterface().GenerateKeyArray(storedMaterials);

	// When moving materials around to different folder, we couldn't do a simple .Contains() since it didn't return true eventhough the data asset had updated the Material to the latest path. So we had to restrt the editor. This fixes that issue without having to load the Material. 
	for (TSoftObjectPtr<UMaterialInterface> materials : storedMaterials) {

		if (materials.ToSoftObjectPath().ToString() == Material.ToSoftObjectPath().ToString())
			return true;
	}

	return false;
}



//-------------------------------------------------------

// Get Parents Of Physics Surface

TArray<TEnumAsByte<EPhysicalSurface>> UVertexPaintMaterialDataAsset::GetParentsOfPhysicsSurface(TEnumAsByte<EPhysicalSurface> PhysicalSurface) const {


	TArray<TEnumAsByte<EPhysicalSurface>> parentOfPhysicsSurface;

	if (PhysicalSurface == EPhysicalSurface::SurfaceType_Default) return parentOfPhysicsSurface;


	TArray<TEnumAsByte<EPhysicalSurface>> physicsSurfacesParents;
	PhysicsSurfaceFamilies.GetKeys(physicsSurfacesParents);

	TArray<FRVPDPRegisteredPhysicsSurfacesSettings> childPhysicsSurfaces;
	PhysicsSurfaceFamilies.GenerateValueArray(childPhysicsSurfaces);


	for (int i = 0; i < physicsSurfacesParents.Num(); i++) {

		// If registered children contain the surface, or the Parent itself Is the Physical Surface we're checking
		if (childPhysicsSurfaces[i].ChildSurfaces.Contains(PhysicalSurface)) {

			parentOfPhysicsSurface.Add(physicsSurfacesParents[i]);
		}

		else if (physicsSurfacesParents[i] == PhysicalSurface) {

			parentOfPhysicsSurface.Add(physicsSurfacesParents[i]);
		}
	}

	return parentOfPhysicsSurface;
}


//-------------------------------------------------------

// Get Registered Material From Material Interface - Checks if the material interface is instance, instance dynamic etc. and looks up the parent and checks if registered. 

UMaterialInterface* UVertexPaintMaterialDataAsset::GetRegisteredMaterialFromMaterialInterface(UMaterialInterface* Material) {

	if (!Material) return nullptr;


	if (GetVertexPaintMaterialInterface().Contains(Material)) {

		return Material;
	}

	// If not registered then checks if instance, or dynamic instance and gets parents through that. 
	else {

		UMaterialInterface* materialInstanceParent = nullptr;

		if (UMaterialInstanceDynamic* materialInstanceDynamic = Cast<UMaterialInstanceDynamic>(Material)) {

			if (UMaterialInstance* materialInstance = Cast<UMaterialInstance>(materialInstanceDynamic->Parent))
				materialInstanceParent = materialInstance->Parent;
			else 
				materialInstanceParent = materialInstanceDynamic->Parent;
		}

		else if (UMaterialInstance* materialInstance = Cast<UMaterialInstance>(Material)) {

			materialInstanceParent = materialInstance->Parent;
		}


		if (materialInstanceParent) {

			if (GetVertexPaintMaterialInterface().Contains(materialInstanceParent))
				return materialInstanceParent;
		}
	}

	return nullptr;
}


#if WITH_EDITOR

//-------------------------------------------------------

// Add Child To Parent Physics Surfaces

void UVertexPaintMaterialDataAsset::AddChildToPhysicsSurfaceFamily(TEnumAsByte<EPhysicalSurface> ParentPhysicsSurface, TEnumAsByte<EPhysicalSurface> ChildPhysicsSurface) {

	FRVPDPRegisteredPhysicsSurfacesSettings parentPhysicsSurfaces;

	if (PhysicsSurfaceFamilies.Contains(ParentPhysicsSurface)) {

		parentPhysicsSurfaces = PhysicsSurfaceFamilies.FindRef(ParentPhysicsSurface);
		parentPhysicsSurfaces.ChildSurfaces.Add(ChildPhysicsSurface);

		PhysicsSurfaceFamilies.Add(ParentPhysicsSurface, parentPhysicsSurfaces);
	}

	else {

		parentPhysicsSurfaces.ChildSurfaces.Add(ChildPhysicsSurface);

		PhysicsSurfaceFamilies.Add(ParentPhysicsSurface, parentPhysicsSurfaces);
	}
}


//-------------------------------------------------------

// Remove Child From Parent Physics Surface

void UVertexPaintMaterialDataAsset::RemoveChildFromPhysicsSurfaceFamily(TEnumAsByte<EPhysicalSurface> ParentPhysicsSurface, TEnumAsByte<EPhysicalSurface> ChildPhysicsSurface) {


	if (PhysicsSurfaceFamilies.Contains(ParentPhysicsSurface)) {

		if (PhysicsSurfaceFamilies.FindRef(ParentPhysicsSurface).ChildSurfaces.Contains(ChildPhysicsSurface)) {

			FRVPDPRegisteredPhysicsSurfacesSettings parentPhysicsSurfaces = PhysicsSurfaceFamilies.FindRef(ParentPhysicsSurface);
			parentPhysicsSurfaces.ChildSurfaces.Remove(ChildPhysicsSurface);

			if (parentPhysicsSurfaces.ChildSurfaces.Num() > 0)
				PhysicsSurfaceFamilies.Add(ParentPhysicsSurface, parentPhysicsSurfaces);
			else
				PhysicsSurfaceFamilies.Remove(ParentPhysicsSurface);
		}
	}
}


//-------------------------------------------------------

// Remove Parent Physics Surface

void UVertexPaintMaterialDataAsset::RemovePhysicsSurfaceFamily(TEnumAsByte<EPhysicalSurface> ParentPhysicsSurface) {

	if (PhysicsSurfaceFamilies.Contains(ParentPhysicsSurface)) {

		PhysicsSurfaceFamilies.Remove(ParentPhysicsSurface);
	}
}


#endif