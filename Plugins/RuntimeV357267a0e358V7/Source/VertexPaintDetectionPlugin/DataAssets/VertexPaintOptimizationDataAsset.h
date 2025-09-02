// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "VertexPaintFunctionLibrary.h"
#include "VertexPaintOptimizationDataAsset.generated.h"


class UStaticMesh;
class USkeletalMesh;


//-------------------------------------------------------

// Skeletal Mesh Bone Vertex Info

USTRUCT(BlueprintType, meta = (DisplayName = "Skeletal Mesh Bone Vertex Info"))
struct FRVPDPSkeletalMeshBoneVertexInfo {

	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	int32 BoneFirstVertex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	int32 BoneFirstSectionVertex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	FName BoneName = "";
};


//-------------------------------------------------------

// Skeletal Mesh Section Info

USTRUCT(BlueprintType, meta = (DisplayName = "Skeletal Mesh Section Info"))
struct FRVPDPSkeletalMeshSectionInfo {

	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	int32 AtSection = 0;

	// First Vertex as Key with Info on the Bone etc. on that vertex. Not visible as it's so heavy to open the data asset if all is visible
	UPROPERTY(/*VisibleAnywhere, */BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Bone info such as first vertex, first section vertex etc. which is used when we want to skip bones to another bone further down in the section. "))
	TMap<int32, FRVPDPSkeletalMeshBoneVertexInfo> SkeletalMeshBoneVertexInfo;
};


//-------------------------------------------------------

// Skeletal Mesh Bone Info Per LOD

USTRUCT(BlueprintType, meta = (DisplayName = "Skeletal Mesh Bone Info Per LOD"))
struct FRVPDPSkeletalMeshBoneInfoPerLOD {

	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	int32 Lod = 0;

	// Section as Key with all the bone info on that section
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin", meta = (ToolTip = "Section index as key. Has info about the bones first vertex etc. on the section so we can quickly look it up in case we need to skip over bones. "))
	TMap<int32, FRVPDPSkeletalMeshSectionInfo> SkeletalMeshSectionInfo;
};


//-------------------------------------------------------

// Skeletal Mesh Bone Childs To Include Info

USTRUCT(BlueprintType, meta = (DisplayName = "Skeletal Mesh Bone Childs To Include Info"))
struct FRVPDPSkeletalMeshBonesToIncludeInfo {

	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	FName ParentBoneName = "";

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	TArray<FName> BoneParentsToInclude;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin")
	TArray<FName> BoneChildsToInclude;
};


//-------------------------------------------------------

// Registered Mesh Info

USTRUCT(BlueprintType, meta = (DisplayName = "Registered Mesh Info"))
struct FRVPDPRegisteredMeshInfo {

	GENERATED_BODY()

	bool HasMaxAmountOfLODInfoRegistered() {

		return (MaxAmountOfLODsToPaint > 0);
	}

	bool HasPropogateToLODInfoRegistered() {

		return (PropogateToLODsInfo.Num() > 0);
	}


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin|Optimization", meta = (ToolTip = "How many LODs to Paint on Static Meshes that are Added here, given that it has that many LODs. Also if the amount set is the max amount that should be stored. Meshes that aren't added here will be painted on all their LODs. "))
	int32 MaxAmountOfLODsToPaint = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin|Optimization", meta = (ToolTip = "Stored Propogate to LOD Info, i.e. what Best LOD0 Vertex the vertices prefer on LODs above LOD0. So when looping through those LODs if set to propogate LOD0 colors to other LODs, then we can quicky look up for every vertex which LOD0 vertex color to use. The Struct is hidden since it gets heavy if visible. "))
	TMap<int32, FRVPDPPropogateToLODsInfo> PropogateToLODsInfo;
};

USTRUCT(BlueprintType, meta = (DisplayName = "Registered Skeletal Mesh Info"))
struct FRVPDPRegisteredSkeletalMeshInfo : public FRVPDPRegisteredMeshInfo {

	GENERATED_BODY()

	bool HasSkeletalMeshBonesToInclude() {

		return (SkeletalMeshBonesToInclude.Num() > 0);
	}

	bool SkeletalMeshRequiredToStayRegistered() {

		if (HasMaxAmountOfLODInfoRegistered())
			return true;

		if (HasPropogateToLODInfoRegistered())
			return true;

		if (HasSkeletalMeshBonesToInclude())
			return true;

		return false;
	}


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin|Bone Info", meta = (ToolTip = "Total amount of Paintable Bones, i.e. not IK stuff that has physics asset and can get hit by traces. Used to determine if the specific bones you're trying to paint is All of these, then we don't even have to bother doing the optimization logic since you have to loop through all bones anyway. "))
	int32 TotalAmountsOfPaintableBonesWithCollision = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin|Bone Info", meta = (ToolTip = "Stores things such as the First Vertex of each bone on each section, which can be used if detecting with Get Closest Vertex Data or Painting with Paint at Location or Paint Within Area so if the paint settings doesn't require us to loop through all vertices, then we can make sure we only loop on the relevant bones. "))
	TArray<FRVPDPSkeletalMeshBoneInfoPerLOD> SkeletalMeshBoneInfoPerLOD;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime Vertex Paint and Detection Plugin|Bone Info", meta = (ToolTip = "Also stores child bones to include so if for instance tracing and hitting hand_r then you can get the additional bones under it such as the fingers which doesn't have their own collision so we also loop through those if painting with paint at location. Can also store parent bones in case the parent doesn't have any collision. "))
	TMap<FName, FRVPDPSkeletalMeshBonesToIncludeInfo> SkeletalMeshBonesToInclude;
};

USTRUCT(BlueprintType, meta = (DisplayName = "Registered Static Mesh Info"))
struct FRVPDPRegisteredStaticMeshInfo : public FRVPDPRegisteredMeshInfo {

	GENERATED_BODY()

	bool StaticMeshRequiredToStayRegistered() {

		if (HasMaxAmountOfLODInfoRegistered())
			return true;

		if (HasPropogateToLODInfoRegistered())
			return true;

		return false;
	}
};

//-------------------------------------------------------


UCLASS(Blueprintable, BlueprintType)
class VERTEXPAINTDETECTIONPLUGIN_API UVertexPaintOptimizationDataAsset : public UDataAsset {

	GENERATED_BODY()


public:

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin|Optimization")
	const TMap<USkeletalMesh*, FRVPDPRegisteredSkeletalMeshInfo>& GetRegisteredSkeletalMeshInfo() const { return RegisteredSkeletalMeshInfo; }

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin|Optimization")
	const TMap<UStaticMesh*, FRVPDPRegisteredStaticMeshInfo>& GetRegisteredStaticMeshInfo() const { return RegisteredStaticMeshInfo; }

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin|Optimization|Paint on LODs", meta = (ToolTip = "How many LODs to Paint on the Mesh, given that it has that many LODs. "))
	int32 GetStaticMeshMaxNumOfLODsToPaint(UStaticMesh* StaticMesh) const;

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin|Optimization|Paint on LODs", meta = (ToolTip = "How many LODs to Paint on the Mesh, given that it has that many LODs. "))
	int32 GetSkeletalMeshMaxNumOfLODsToPaint(USkeletalMesh* SkeletalMesh) const;

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin|Optimization")
	bool HasMeshPropogateToLODInfoRegistered(const UObject* MeshAsset);

	bool GetRegisteredMeshPropogateToLODInfo(const UObject* MeshAsset, TMap<int32, FRVPDPPropogateToLODsInfo>& PropogateToLODInfo);

	bool GetRegisteredMeshPropogateToLODInfoAtLOD(const UObject* MeshAsset, int32 LOD, FRVPDPPropogateToLODsInfo& PropogateToLODInfo);


#if WITH_EDITOR

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Optimization")
	void ValidateRegisteredSkeletalMeshes();

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Optimization")
	void ValidateRegisteredStaticMeshes();

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Optimization|Paint on LODs")
	void UnRegisterFromStaticMeshMaxNumOfLODsToPaint(UStaticMesh* StaticMesh);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Optimization|Paint on LODs")
	void UnRegisterFromSkeletalMeshMaxNumOfLODsToPaint(USkeletalMesh* SkeletalMesh);


	bool RegisterSkeletalMeshBoneInfo(USkeletalMesh* SkeletalMesh);

	bool UnRegisterSkeletalMeshBoneInfo(USkeletalMesh* SkeletalMesh);

	void RegisterSkeletalMeshMaxNumOfLODsToPaint(USkeletalMesh* SkeletalMesh, int32 MaxAmountOfLODsToPaint);

	bool RegisterMeshPropogateLODInfo(UObject* MeshAsset);

	bool UnRegisterMeshPropogateLODInfo(UObject* MeshAsset);

	void RegisterStaticMeshMaxNumOfLODsToPaint(UStaticMesh* StaticMesh, int32 MaxAmountOfLODsToPaint);


private:

	bool GetPropogateLODFundementals(UObject* Mesh, TArray<FExtendedPaintedVertex>& PaintedVerticesAtLOD0, FRVPDPVertexDataInfo& MeshVertexData, TArray<FBox>& CompleteLOD0BaseBounds);

	void GetSkeletalMeshBonesToInclude(USkeletalMesh* SkeletalMesh, int32 BoneIndex, bool CheckParentBone, TArray<FName>& ChildBoneNames, TArray<int32>& ChildBoneIndices);

#endif


private:

	UPROPERTY(VisibleAnywhere, Category = "Runtime Vertex Paint and Detection Plugin|Optimization", meta = (ToolTip = "Registered Skeletal Mesh info such as Max Amount of LODs to Paint and Propogate to LOD Info and Bone Info per LOD such as the First Vertex of each bone on each section, which can be used if detecting with Get Closest Vertex Data or Painting with Paint at Location or Paint Within Area so if the paint settings doesn't require us to loop through all vertices, then we can make sure we only loop on the relevant bones. \nAlso stores child bones to include for that so if for instance tracing and hitting hand_r then you can get the additional bones under it such as the fingers which doesn't have their own collision so we also loop through those. "))
	TMap<USkeletalMesh*, FRVPDPRegisteredSkeletalMeshInfo> RegisteredSkeletalMeshInfo;

	UPROPERTY(VisibleAnywhere, Category = "Runtime Vertex Paint and Detection Plugin|Optimization", meta = (ToolTip = "Registered Static Mesh info such as Max Amount of LODs to Paint and Propogate to LOD Info"))
	TMap<UStaticMesh*, FRVPDPRegisteredStaticMeshInfo> RegisteredStaticMeshInfo;
};
