// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 


#include "VertexPaintOptimizationDataAsset.h"

// Engine
#include "Rendering/SkeletalMeshRenderData.h"
#include "Animation/Skeleton.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/BodySetup.h"

// Plugin
#include "VertexPaintDetectionProfiling.h"

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
#include "PhysicsEngine/SkeletalBodySetup.h"
#endif


#if WITH_EDITOR


//-------------------------------------------------------

// Register Skeletal Mesh Bone Info

bool UVertexPaintOptimizationDataAsset::RegisterSkeletalMeshBoneInfo(USkeletalMesh* SkeletalMesh) {

	if (!SkeletalMesh) return false;


	FRVPDPRegisteredSkeletalMeshInfo registeredSkeletalMeshInfo;
	if (RegisteredSkeletalMeshInfo.Contains(SkeletalMesh)) {


		registeredSkeletalMeshInfo = RegisteredSkeletalMeshInfo.FindRef(SkeletalMesh);

		if (registeredSkeletalMeshInfo.SkeletalMeshBonesToInclude.Num() > 0) {

			registeredSkeletalMeshInfo.SkeletalMeshBoneInfoPerLOD.Empty();
			registeredSkeletalMeshInfo.SkeletalMeshBonesToInclude.Empty();
			registeredSkeletalMeshInfo.TotalAmountsOfPaintableBonesWithCollision = 0;

			// If already exists, but for some reason render data or another vital check fails we will store the bone info as null since it wouldn't be safe to use whatever was already there. 
			RegisteredSkeletalMeshInfo.Add(SkeletalMesh, registeredSkeletalMeshInfo);
		}
	}


	FSkeletalMeshRenderData* skeletalMeshRenderData = SkeletalMesh->GetResourceForRendering();
	if (!skeletalMeshRenderData) return false;

	USkeleton* skeleton = SkeletalMesh->GetSkeleton();
	if (!skeleton) return false;


	const FReferenceSkeleton& RefSkeleton = skeleton->GetReferenceSkeleton();
	TArray<FName> skeletalMeshBoneNames;
	for (int32 i = 0; i < RefSkeleton.GetNum(); i++)
		skeletalMeshBoneNames.Add(RefSkeleton.GetBoneName(i));


	for (int i = 0; i < skeletalMeshRenderData->LODRenderData.Num(); i++) {

		FSkeletalMeshLODRenderData* skeletalMeshLODRenderData = &skeletalMeshRenderData->LODRenderData[i];
		if (!skeletalMeshLODRenderData) continue;
		
		FSkinWeightVertexBuffer* skinWeightBuffer = skeletalMeshLODRenderData->GetSkinWeightVertexBuffer();
		if (!skinWeightBuffer) continue;


		FRVPDPSkeletalMeshBoneInfoPerLOD skeletalMeshBoneInfoPerLOD;
		skeletalMeshBoneInfoPerLOD.Lod = i;
		int32 lastBoneIndex = -1;
		int lastSectionIndex = -1;
		int lodVertex = 0;


		for (int j = 0; j < skeletalMeshLODRenderData->RenderSections.Num(); j++) {


			FRVPDPSkeletalMeshSectionInfo sectionBoneInfoTest;
			sectionBoneInfoTest.AtSection = j;

			FSkelMeshRenderSection& skeletalMeshRenderSection = skeletalMeshLODRenderData->RenderSections[j];
			const int32 amountOfVerticesOnSection = skeletalMeshRenderSection.GetNumVertices();

			for (int32 sectionVertex = 0; sectionVertex < amountOfVerticesOnSection; sectionVertex++) {

				// Only interested in influence index 0. 
				int32 InfluenceIndex = 0;

				const uint32 boneIndex = skinWeightBuffer->GetBoneIndex(lodVertex, InfluenceIndex);
				const int32 currentBoneIndex = skeletalMeshRenderSection.BoneMap[boneIndex];
				const FName currentBone = skeletalMeshBoneNames[currentBoneIndex];

				// If on a new bone
				if (lastBoneIndex != currentBoneIndex || lastSectionIndex != j) {

					lastBoneIndex = currentBoneIndex;
					lastSectionIndex = j;

					FRVPDPSkeletalMeshBoneVertexInfo boneVertexInfo;
					boneVertexInfo.BoneFirstVertex = lodVertex;
					boneVertexInfo.BoneFirstSectionVertex = sectionVertex;
					boneVertexInfo.BoneName = currentBone;
					// boneVertexInfo.AtSection = j;
					// boneVertexInfo.boneIndex = currentBoneIndex;

					sectionBoneInfoTest.SkeletalMeshBoneVertexInfo.Add(lodVertex, boneVertexInfo);
				}

				lodVertex++;
			}

			skeletalMeshBoneInfoPerLOD.SkeletalMeshSectionInfo.Add(j, sectionBoneInfoTest);
		}

		registeredSkeletalMeshInfo.SkeletalMeshBoneInfoPerLOD.Add(skeletalMeshBoneInfoPerLOD);
	}


	int totalAmountOfPaintableBonesWithCollision = 0;

	if (UPhysicsAsset* physicsAsset = SkeletalMesh->GetPhysicsAsset()) {

		for (const FName& boneName : skeletalMeshBoneNames) {

			// Doesn't store ik bones since they don't have collision
			if (boneName.ToString().StartsWith(TEXT("ik"), ESearchCase::IgnoreCase) || boneName.ToString().EndsWith(TEXT("_ik"), ESearchCase::IgnoreCase)) continue;

			const int32 bodyIndex = physicsAsset->FindBodyIndex(boneName);
			if (bodyIndex == INDEX_NONE) continue;


			FKAggregateGeom aggGeom;

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5

			if (USkeletalBodySetup* skeletalBodySetup = physicsAsset->SkeletalBodySetups[bodyIndex].Get())
				aggGeom = skeletalBodySetup->AggGeom;
			else
				continue;

#else

			if (UBodySetup* bodySetup = physicsAsset->SkeletalBodySetups[bodyIndex])
				aggGeom = bodySetup->AggGeom;
			else
				continue;

#endif


			// Requires the parent bone to have collision, since it is the bones that we can hit with traces etc. and if hit with trace we want to very fast be able to look up it's children that doesn't have collision that can't hit with trace and include them.
			if (aggGeom.GetElementCount() > 0) {

				totalAmountOfPaintableBonesWithCollision++;
				const int32 boneIndex = RefSkeleton.FindBoneIndex(boneName);
				TArray<FName> childBoneNamesWithoutCollision;
				TArray<FName> parentBoneNamesWithoutCollision;
				TArray<int32> childBoneIndicesWithoutCollision;
				TArray<int32> parentBoneIndicesWithoutCollision;


				GetSkeletalMeshBonesToInclude(SkeletalMesh, boneIndex, false, childBoneNamesWithoutCollision, childBoneIndicesWithoutCollision);
				GetSkeletalMeshBonesToInclude(SkeletalMesh, boneIndex, true, parentBoneNamesWithoutCollision, parentBoneIndicesWithoutCollision);


				if (childBoneIndicesWithoutCollision.Num() > 0 || parentBoneIndicesWithoutCollision.Num() > 0) {

					FRVPDPSkeletalMeshBonesToIncludeInfo boneChildsToInclude;
					boneChildsToInclude.ParentBoneName = boneName;
					boneChildsToInclude.BoneChildsToInclude = childBoneNamesWithoutCollision;
					boneChildsToInclude.BoneParentsToInclude = parentBoneNamesWithoutCollision;

					registeredSkeletalMeshInfo.SkeletalMeshBonesToInclude.Add(boneName, boneChildsToInclude);
				}
			}
		}
	}


	registeredSkeletalMeshInfo.TotalAmountsOfPaintableBonesWithCollision = totalAmountOfPaintableBonesWithCollision;

	RegisteredSkeletalMeshInfo.Add(SkeletalMesh, registeredSkeletalMeshInfo);

	return true;
}

bool UVertexPaintOptimizationDataAsset::UnRegisterSkeletalMeshBoneInfo(USkeletalMesh* SkeletalMesh) {

	if (!SkeletalMesh) return false;
	if (!RegisteredSkeletalMeshInfo.Contains(SkeletalMesh)) return false;


	if (RegisteredSkeletalMeshInfo.Contains(SkeletalMesh)) {


		FRVPDPRegisteredSkeletalMeshInfo registeredSkeletalMeshInfo = RegisteredSkeletalMeshInfo.FindRef(SkeletalMesh);

		// If has bone info registered
		if (registeredSkeletalMeshInfo.SkeletalMeshBonesToInclude.Num() > 0) {

			registeredSkeletalMeshInfo.SkeletalMeshBoneInfoPerLOD.Empty();
			registeredSkeletalMeshInfo.SkeletalMeshBonesToInclude.Empty();
			registeredSkeletalMeshInfo.TotalAmountsOfPaintableBonesWithCollision = 0;


			if (registeredSkeletalMeshInfo.SkeletalMeshRequiredToStayRegistered()) {

				RegisteredSkeletalMeshInfo.Add(SkeletalMesh, registeredSkeletalMeshInfo);
				return true;
			}


			RegisteredSkeletalMeshInfo.Remove(SkeletalMesh);
			return true;
		}
	}

	return false;
}


//-------------------------------------------------------

// Skeletal Mesh Max Num Of LODs To Paint

void UVertexPaintOptimizationDataAsset::RegisterSkeletalMeshMaxNumOfLODsToPaint(USkeletalMesh* SkeletalMesh, int32 MaxAmountOfLODsToPaint) {

	if (MaxAmountOfLODsToPaint <= 0) return;


	FRVPDPRegisteredSkeletalMeshInfo registeredSkeletalMeshInfo;

	if (RegisteredSkeletalMeshInfo.Contains(SkeletalMesh))
		registeredSkeletalMeshInfo = RegisteredSkeletalMeshInfo.FindRef(SkeletalMesh);

	registeredSkeletalMeshInfo.MaxAmountOfLODsToPaint = MaxAmountOfLODsToPaint;
	RegisteredSkeletalMeshInfo.Add(SkeletalMesh, registeredSkeletalMeshInfo);
}

void UVertexPaintOptimizationDataAsset::UnRegisterFromSkeletalMeshMaxNumOfLODsToPaint(USkeletalMesh* SkeletalMesh) {

	if (!RegisteredSkeletalMeshInfo.Contains(SkeletalMesh)) return;


	FRVPDPRegisteredSkeletalMeshInfo registeredSkeletalMeshInfo;
	registeredSkeletalMeshInfo = RegisteredSkeletalMeshInfo.FindRef(SkeletalMesh);
	registeredSkeletalMeshInfo.MaxAmountOfLODsToPaint = 0;

	if (registeredSkeletalMeshInfo.SkeletalMeshRequiredToStayRegistered()) {

		RegisteredSkeletalMeshInfo.Add(SkeletalMesh, registeredSkeletalMeshInfo);
		return;
	}

	RegisteredSkeletalMeshInfo.Remove(SkeletalMesh);
}


//-------------------------------------------------------

// Register Mesh Propogate LOD Info

bool UVertexPaintOptimizationDataAsset::RegisterMeshPropogateLODInfo(UObject* MeshAsset) {

	if (!MeshAsset) return false;


	TArray<FExtendedPaintedVertex> paintedVerticesAtLOD0;
	FRVPDPVertexDataInfo meshVertexData;
	TArray<FBox> completeLOD0BaseBounds;
	if (GetPropogateLODFundementals(MeshAsset, paintedVerticesAtLOD0, meshVertexData, completeLOD0BaseBounds)) {


		if (UStaticMesh* staticMesh = Cast<UStaticMesh>(MeshAsset)) {

			FRVPDPRegisteredStaticMeshInfo registeredStaticMeshInfo;

			if (RegisteredStaticMeshInfo.Contains(staticMesh))
				registeredStaticMeshInfo = RegisteredStaticMeshInfo.FindRef(staticMesh);


			TArray<FColorVertexBuffer*> vertexBufferTemp;
			UVertexPaintFunctionLibrary::PropagateLOD0VertexColorsToLODs(paintedVerticesAtLOD0, meshVertexData, completeLOD0BaseBounds, vertexBufferTemp, true, registeredStaticMeshInfo.PropogateToLODsInfo);

			RegisteredStaticMeshInfo.Add(staticMesh, registeredStaticMeshInfo);
		}

		else if (USkeletalMesh* skeletalMesh = Cast<USkeletalMesh>(MeshAsset)) {

			FRVPDPRegisteredSkeletalMeshInfo registeredSkeletalMeshInfo;

			if (RegisteredSkeletalMeshInfo.Contains(skeletalMesh))
				registeredSkeletalMeshInfo = RegisteredSkeletalMeshInfo.FindRef(skeletalMesh);


			TArray<FColorVertexBuffer*> vertexBufferTemp;
			UVertexPaintFunctionLibrary::PropagateLOD0VertexColorsToLODs(paintedVerticesAtLOD0, meshVertexData, completeLOD0BaseBounds, vertexBufferTemp, true, registeredSkeletalMeshInfo.PropogateToLODsInfo);

			RegisteredSkeletalMeshInfo.Add(skeletalMesh, registeredSkeletalMeshInfo);
		}


		return true;
	}

	return false;
}

bool UVertexPaintOptimizationDataAsset::UnRegisterMeshPropogateLODInfo(UObject* MeshAsset) {

	if (!MeshAsset) return false;


	if (UStaticMesh* staticMesh = Cast<UStaticMesh>(MeshAsset)) {

		FRVPDPRegisteredStaticMeshInfo registeredStaticMeshInfo;

		if (RegisteredStaticMeshInfo.Contains(staticMesh))
			registeredStaticMeshInfo = RegisteredStaticMeshInfo.FindRef(staticMesh);

		registeredStaticMeshInfo.PropogateToLODsInfo.Empty();


		if (registeredStaticMeshInfo.StaticMeshRequiredToStayRegistered()) {

			RegisteredStaticMeshInfo.Add(staticMesh, registeredStaticMeshInfo);
			return true;
		}

		RegisteredStaticMeshInfo.Remove(staticMesh);
		return true;
	}

	else if (USkeletalMesh* skeletalMesh = Cast<USkeletalMesh>(MeshAsset)) {


		FRVPDPRegisteredSkeletalMeshInfo registeredSkeletalMeshInfo;

		if (RegisteredSkeletalMeshInfo.Contains(skeletalMesh))
			registeredSkeletalMeshInfo = RegisteredSkeletalMeshInfo.FindRef(skeletalMesh);

		registeredSkeletalMeshInfo.PropogateToLODsInfo.Empty();


		if (registeredSkeletalMeshInfo.SkeletalMeshRequiredToStayRegistered()) {

			RegisteredSkeletalMeshInfo.Add(skeletalMesh, registeredSkeletalMeshInfo);
			return true;
		}

		RegisteredSkeletalMeshInfo.Remove(skeletalMesh);

		return true;
	}

	return false;
}


//-------------------------------------------------------

// Get Propogate LOD Fundementals

bool UVertexPaintOptimizationDataAsset::GetPropogateLODFundementals(UObject* Mesh, TArray<FExtendedPaintedVertex>& PaintedVerticesAtLOD0, FRVPDPVertexDataInfo& MeshVertexData, TArray<FBox>& CompleteLOD0BaseBounds) {

	if (!Mesh) return false;


	int lodsToLoopThrough = 1;


	if (UStaticMesh* staticMesh = Cast<UStaticMesh>(Mesh)) {

		if (FStaticMeshRenderData* staticMeshRenderData = staticMesh->GetRenderData())
			lodsToLoopThrough = staticMeshRenderData->LODResources.Num();
	}

	else if (USkeletalMesh* skeletalMesh = Cast<USkeletalMesh>(Mesh)) {

		if (FSkeletalMeshRenderData* skelMeshRenderData = skeletalMesh->GetResourceForRendering())
			lodsToLoopThrough = skelMeshRenderData->LODRenderData.Num();
	}

	else {

		return false;
	}


	// Requires min 2 LODs
	if (lodsToLoopThrough < 2) return false;


	MeshVertexData.MeshDataPerLOD.SetNum(lodsToLoopThrough);
	CompleteLOD0BaseBounds.SetNum(lodsToLoopThrough);

	for (int currentLOD = 0; currentLOD < lodsToLoopThrough; currentLOD++) {

		int32 totalAmountOfVerticesAtLOD = 0;
		FPositionVertexBuffer* positionVertexBuffer = nullptr;
		FStaticMeshVertexBuffer* meshVertexBuffer = nullptr;
		FBox LOD0BaseBounds;


		if (UStaticMesh* staticMesh = Cast<UStaticMesh>(Mesh)) {

			if (FStaticMeshRenderData* staticMeshRenderData = staticMesh->GetRenderData()) {

				if (!staticMeshRenderData->LODResources.IsValidIndex(currentLOD)) return false;


				positionVertexBuffer = &staticMeshRenderData->LODResources[currentLOD].VertexBuffers.PositionVertexBuffer;
				meshVertexBuffer = &staticMeshRenderData->LODResources[currentLOD].VertexBuffers.StaticMeshVertexBuffer;
			}
		}

		else if (USkeletalMesh* skeletalMesh = Cast<USkeletalMesh>(Mesh)) {

			if (FSkeletalMeshRenderData* skelMeshRenderData = skeletalMesh->GetResourceForRendering()) {

				if (!skelMeshRenderData->LODRenderData.IsValidIndex(currentLOD)) return false;


				if (FSkeletalMeshLODRenderData* SkeletalMeshLODRenderData = &skelMeshRenderData->LODRenderData[currentLOD]) {

					positionVertexBuffer = &SkeletalMeshLODRenderData->StaticVertexBuffers.PositionVertexBuffer;
					meshVertexBuffer = &SkeletalMeshLODRenderData->StaticVertexBuffers.StaticMeshVertexBuffer;
				}
			}
		}

		if (!positionVertexBuffer || !meshVertexBuffer) return false;


		totalAmountOfVerticesAtLOD = positionVertexBuffer->GetNumVertices();


		for (int32 currentVertex = 0; currentVertex < totalAmountOfVerticesAtLOD; currentVertex++) {


#if ENGINE_MAJOR_VERSION == 4

			FVector vertexPositionInComponentSpace = positionVertexBuffer->VertexPosition(currentVertex);

#elif ENGINE_MAJOR_VERSION == 5

			FVector vertexPositionInComponentSpace = static_cast<FVector>(positionVertexBuffer->VertexPosition(currentVertex));

#endif

			FVector vertexNormal = FVector(meshVertexBuffer->VertexTangentZ(currentVertex).X, meshVertexBuffer->VertexTangentZ(currentVertex).Y, meshVertexBuffer->VertexTangentZ(currentVertex).Z);


			LOD0BaseBounds += vertexPositionInComponentSpace;
			MeshVertexData.MeshDataPerLOD[currentLOD].MeshVertexPositionsInComponentSpacePerLODArray.Add(vertexPositionInComponentSpace);
			MeshVertexData.MeshDataPerLOD[currentLOD].MeshVertexNormalsPerLODArray.Add(vertexNormal);
			MeshVertexData.MeshDataPerLOD[currentLOD].MeshVertexColorsPerLODArray.Add(FColor::White); 


			if (currentLOD == 0) {

				FExtendedPaintedVertex paintedVertex;
				paintedVertex.Position = vertexPositionInComponentSpace;
				paintedVertex.Normal = vertexNormal;
				paintedVertex.Color = FColor::White; // We only care about associated vertex indexes to LOD0 indexes here
				PaintedVerticesAtLOD0.Add(paintedVertex);
			}
		}

		CompleteLOD0BaseBounds[currentLOD] = LOD0BaseBounds;
	}

	return true;
}


//-------------------------------------------------------

// Get Skeletal Mesh Child Bone Without Collision

void UVertexPaintOptimizationDataAsset::GetSkeletalMeshBonesToInclude(USkeletalMesh* SkeletalMesh, int32 BoneIndex, bool CheckParentBone, TArray<FName>& ChildBoneNames, TArray<int32>& ChildBoneIndices) {

	if (!SkeletalMesh) return;

	UPhysicsAsset* physicsAsset = SkeletalMesh->GetPhysicsAsset();
	if (!physicsAsset) return;

	USkeleton* skeleton = SkeletalMesh->GetSkeleton();
	if (!skeleton) return;


	const FReferenceSkeleton& referenceSkeleton = skeleton->GetReferenceSkeleton();

	TArray<int32> boneIndicesToCheck;
	
	if (CheckParentBone) {

		int32 parentBoneIndex = referenceSkeleton.GetParentIndex(BoneIndex);
		boneIndicesToCheck.Add(parentBoneIndex);
	}
	else {

		skeleton->GetChildBones(BoneIndex, boneIndicesToCheck);
	}

	if (boneIndicesToCheck.Num() == 0) return;


	// Checks immediate bone children if they have collision or not. If not, then we add them to a list and check if their children have collision etc. 
	for (int32 boneIndexToCheck : boneIndicesToCheck) {

		if (boneIndexToCheck == INDEX_NONE) continue;


		const FName boneNameToCheck = referenceSkeleton.GetBoneName(boneIndexToCheck);

		if (boneNameToCheck.ToString().StartsWith(TEXT("ik"), ESearchCase::IgnoreCase) || boneNameToCheck.ToString().EndsWith(TEXT("_ik"), ESearchCase::IgnoreCase)) continue;


		const int32 bodyIndex = physicsAsset->FindBodyIndex(boneNameToCheck);

		// If there's a body index for the child
		if (bodyIndex != INDEX_NONE) {


			int32 aggGeomElemCount = 0;

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5

			if (USkeletalBodySetup* skeletalBodySetup = physicsAsset->SkeletalBodySetups[bodyIndex].Get())
				aggGeomElemCount = skeletalBodySetup->AggGeom.GetElementCount();

#else

			if (UBodySetup* bodySetup = physicsAsset->SkeletalBodySetups[bodyIndex])
				aggGeomElemCount = bodySetup->AggGeom.GetElementCount();

#endif


			if (aggGeomElemCount > 0) {

				// This child bone has collision
			}

			// If no collision then adds and also gets childs from this that doesn't have collision
			else {

				ChildBoneNames.Add(boneNameToCheck);
				ChildBoneIndices.Add(boneIndexToCheck);

				GetSkeletalMeshBonesToInclude(SkeletalMesh, boneIndexToCheck, CheckParentBone, ChildBoneNames, ChildBoneIndices);
			}
		}

		// If no collision then adds and also gets childs from this that doesn't have collision
		else {

			ChildBoneNames.Add(boneNameToCheck);
			ChildBoneIndices.Add(boneIndexToCheck);

			GetSkeletalMeshBonesToInclude(SkeletalMesh, boneIndexToCheck, CheckParentBone, ChildBoneNames, ChildBoneIndices);
		}
	}
}


//-------------------------------------------------------

// Validate Registered Skeletal Meshes

void UVertexPaintOptimizationDataAsset::ValidateRegisteredSkeletalMeshes() {


	TMap<USkeletalMesh*, FRVPDPRegisteredSkeletalMeshInfo> registeredSkeletalMeshInfoTemp = RegisteredSkeletalMeshInfo;
	RegisteredSkeletalMeshInfo.Empty();

	// Rebuilds the TMap with only Valid Meshes
	for (auto& registeredSkelMeshInf : registeredSkeletalMeshInfoTemp) {

		if (registeredSkelMeshInf.Key) 
			RegisteredSkeletalMeshInfo.Add(registeredSkelMeshInf.Key, registeredSkelMeshInf.Value);
	}
}


//-------------------------------------------------------

// Validate Registered Static Meshes

void UVertexPaintOptimizationDataAsset::ValidateRegisteredStaticMeshes() {


	TMap<UStaticMesh*, FRVPDPRegisteredStaticMeshInfo> registeredStaticMeshInfoTemp = RegisteredStaticMeshInfo;
	RegisteredStaticMeshInfo.Empty();

	// Rebuilds the TMap with only Valid Meshes
	for (auto& registeredStaticMeshInf : registeredStaticMeshInfoTemp) {

		if (registeredStaticMeshInf.Key)
			RegisteredStaticMeshInfo.Add(registeredStaticMeshInf.Key, registeredStaticMeshInf.Value);
	}
}


//-------------------------------------------------------

// Static Mesh Max Num Of LODs To Paint

void UVertexPaintOptimizationDataAsset::RegisterStaticMeshMaxNumOfLODsToPaint(UStaticMesh* StaticMesh, int32 MaxAmountOfLODsToPaint) {

	if (MaxAmountOfLODsToPaint <= 0) return;


	FRVPDPRegisteredStaticMeshInfo registeredStaticMeshInfo;

	if (RegisteredStaticMeshInfo.Contains(StaticMesh))
		registeredStaticMeshInfo = RegisteredStaticMeshInfo.FindRef(StaticMesh);

	registeredStaticMeshInfo.MaxAmountOfLODsToPaint = MaxAmountOfLODsToPaint;
	RegisteredStaticMeshInfo.Add(StaticMesh, registeredStaticMeshInfo);
}

void UVertexPaintOptimizationDataAsset::UnRegisterFromStaticMeshMaxNumOfLODsToPaint(UStaticMesh* StaticMesh) {

	if (!RegisteredStaticMeshInfo.Contains(StaticMesh)) return;


	FRVPDPRegisteredStaticMeshInfo registeredStaticMeshInfo;
	registeredStaticMeshInfo = RegisteredStaticMeshInfo.FindRef(StaticMesh);
	registeredStaticMeshInfo.MaxAmountOfLODsToPaint = 0;


	if (registeredStaticMeshInfo.StaticMeshRequiredToStayRegistered()) {

		RegisteredStaticMeshInfo.Add(StaticMesh, registeredStaticMeshInfo);
		return;
	}

	RegisteredStaticMeshInfo.Remove(StaticMesh);
}

#endif


//-------------------------------------------------------

// Get Mesh Max Num Of LODs To Paint

int32 UVertexPaintOptimizationDataAsset::GetStaticMeshMaxNumOfLODsToPaint(UStaticMesh* StaticMesh) const {


	if (RegisteredStaticMeshInfo.Contains(StaticMesh))
		return RegisteredStaticMeshInfo.FindRef(StaticMesh).MaxAmountOfLODsToPaint;

	return 0;
}

int32 UVertexPaintOptimizationDataAsset::GetSkeletalMeshMaxNumOfLODsToPaint(USkeletalMesh* SkeletalMesh) const {


	if (RegisteredSkeletalMeshInfo.Contains(SkeletalMesh))
		return RegisteredSkeletalMeshInfo.FindRef(SkeletalMesh).MaxAmountOfLODsToPaint;

	return 0;
}


//-------------------------------------------------------

// Has Mesh Propogate To LOD Info Registered

bool UVertexPaintOptimizationDataAsset::HasMeshPropogateToLODInfoRegistered(const UObject* MeshAsset) {

	if (!MeshAsset) return false;

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_HasMeshPropogateToLODInfoRegistered);
	RVPDP_CPUPROFILER_STR("Optimization Data Asset - HasMeshPropogateToLODInfoRegistered");


	if (const UStaticMesh* staticMesh = Cast<UStaticMesh>(MeshAsset)) {

		if (RegisteredStaticMeshInfo.Contains(staticMesh)) {

			if (RegisteredStaticMeshInfo.FindRef(staticMesh).HasPropogateToLODInfoRegistered())
				return true;
		}
	}

	else if (const USkeletalMesh* skeletalMesh = Cast<USkeletalMesh>(MeshAsset)) {

		if (RegisteredSkeletalMeshInfo.Contains(skeletalMesh)) {

			if (RegisteredSkeletalMeshInfo.FindRef(skeletalMesh).HasPropogateToLODInfoRegistered())
				return true;
		}
	}

	return false;
}


//-------------------------------------------------------

// Get Registered Mesh Propogate To LOD Info

bool UVertexPaintOptimizationDataAsset::GetRegisteredMeshPropogateToLODInfo(const UObject* MeshAsset, TMap<int32, FRVPDPPropogateToLODsInfo>& PropogateToLODInfo) {

	PropogateToLODInfo.Empty();

	if (!MeshAsset) return false;

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetRegisteredMeshPropogateToLODInfo);
	RVPDP_CPUPROFILER_STR("Optimization Data Asset - GetRegisteredMeshPropogateToLODInfo");


	if (const UStaticMesh* staticMesh = Cast<UStaticMesh>(MeshAsset)) {

		if (RegisteredStaticMeshInfo.Contains(staticMesh)) {

			PropogateToLODInfo = RegisteredStaticMeshInfo.FindRef(staticMesh).PropogateToLODsInfo;

			if (PropogateToLODInfo.Num() > 0)
				return true;
		}
	}

	else if (const USkeletalMesh* skeletalMesh = Cast<USkeletalMesh>(MeshAsset)) {

		if (RegisteredSkeletalMeshInfo.Contains(skeletalMesh)) {

			PropogateToLODInfo = RegisteredSkeletalMeshInfo.FindRef(skeletalMesh).PropogateToLODsInfo;

			if (PropogateToLODInfo.Num() > 0)
				return true;
		}
	}

	return false;
}


//-------------------------------------------------------

// Get Registered Mesh Propogate To LOD Info At LOD

bool UVertexPaintOptimizationDataAsset::GetRegisteredMeshPropogateToLODInfoAtLOD(const UObject* MeshAsset, int32 LOD, FRVPDPPropogateToLODsInfo& PropogateToLODInfo) {

	PropogateToLODInfo = FRVPDPPropogateToLODsInfo();

	if (!MeshAsset) return false;
	if (LOD <= 0) return false;

	RVPDP_SCOPE_CYCLE_COUNTER(STAT_GetRegisteredMeshPropogateToLODInfoAtLOD);
	RVPDP_CPUPROFILER_STR("Optimization Data Asset - GetRegisteredMeshPropogateToLODInfoAtLOD");


	if (const UStaticMesh* staticMesh = Cast<UStaticMesh>(MeshAsset)) {

		if (RegisteredStaticMeshInfo.Contains(staticMesh)) {

			PropogateToLODInfo = RegisteredStaticMeshInfo.FindRef(staticMesh).PropogateToLODsInfo.FindRef(LOD);
			
			if (PropogateToLODInfo.LODVerticesAssociatedLOD0Vertex.Num() > 0)
				return true;
		}
	}

	else if (const USkeletalMesh* skeletalMesh = Cast<USkeletalMesh>(MeshAsset)) {

		if (RegisteredSkeletalMeshInfo.Contains(skeletalMesh)) {

			PropogateToLODInfo = RegisteredSkeletalMeshInfo.FindRef(skeletalMesh).PropogateToLODsInfo.FindRef(LOD);

			if (PropogateToLODInfo.LODVerticesAssociatedLOD0Vertex.Num() > 0)
				return true;
		}
	}

	return false;
}
