// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "CalculateColorsPrerequisites.h"
#include "CollisionQueryParams.h"
#include "Engine/EngineTypes.h"
#include "Runtime/Launch/Resources/Version.h"

#if ENGINE_MAJOR_VERSION == 5

#include "Components/DynamicMeshComponent.h"

#if ENGINE_MINOR_VERSION >= 1
#include "Engine/HitResult.h"
#endif

#if ENGINE_MINOR_VERSION >= 2
#include "Rendering/SkeletalMeshRenderData.h" // For 5.2 and up the forward decleration wasn't enough. 
#endif

#endif

#include "Async/AsyncWork.h"


class UBoxComponent;
class USphereComponent;
class USplineMeshComponent;

struct FPaintedVertex;
class FPositionVertexBuffer;
class FStaticMeshVertexBuffer;

struct FSkelMeshRenderSection;
class FSkeletalMeshRenderData;
class FSkinWeightVertexBuffer;
class FSkeletalMeshLODRenderData;
class UClothingAssetBase;


DECLARE_DELEGATE_OneParam(FVertexPaintAsyncTaskFinished, const FRVPDPCalculateColorsInfo&);



//-------------------------------------------------------

// Async Task Debug Messages

struct FRVPDPAsyncTaskDebugMessagesInfo {

	FRVPDPAsyncTaskDebugMessagesInfo(FString Message, FColor Color) {

		DebugMessage = Message;
		MessageColor = Color;
	}

	FString DebugMessage = "";

	FColor MessageColor = FColor(ForceInitToZero);
};


//-------------------------------------------------------

// Direction From Hit Location To Closest Vertices Info

struct FRVPDPDirectionFromHitLocationToClosestVerticesInfo {

	int LodVertexIndex = 0;

	FVector DirectionFromHitDirectionToVertex = FVector(ForceInitToZero);

	FVector VertexWorldPosition = FVector(ForceInitToZero);

	float DistanceFromHitLocation = 0;
};



class VERTEXPAINTDETECTIONPLUGIN_API FVertexPaintCalculateColorsTask : public FNonAbandonableTask {

	friend class FAsyncTask<FVertexPaintCalculateColorsTask>;
	friend class FAutoDeleteAsyncTask<FVertexPaintCalculateColorsTask>;


public:

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FVertexPaintCalculateColorsTask, STATGROUP_ThreadPoolAsyncTasks);
	}


	void DoWork();

	void SetCalculateColorsInfo(const FRVPDPCalculateColorsInfo& CalculateColorsInfo);

	FVertexPaintAsyncTaskFinished AsyncTaskFinishedDelegate;


private:


	bool CalculateColorToApply();



	//-----------  UTILITIES  ----------- //

	// Should take CalculateColors as param since we use it when back on game thread as well, so it can't use the global property
	inline bool IsMeshStillValid(const FRVPDPCalculateColorsInfo& CalculateColorsInfo) const;

	// Should take CalculateColors as param since we use it when back on game thread as well, so it can't use the global property
	inline bool IsTaskStillValid(const FRVPDPCalculateColorsInfo& CalculateColorsInfo) const;

	inline FLinearColor ClampLinearColor(FLinearColor LinearColor);

	inline bool IsValidSkeletalMesh(const USkeletalMesh* SkeletalMesh, const USkeletalMeshComponent* SkeletalMeshComponent, const FSkeletalMeshRenderData* SkeletalMeshRenderData, int Lod);

	bool ShouldPaintJobRunCalculateColorsToApply(const FRVPDPApplyColorSetting& PaintColorSettings, const FRVPDPPaintTaskSettings& PaintSettings, const FRVPDPPaintDirectlyTaskSettings& PaintColorsDirectlySettings);

	bool IsSetToOverrideVertexColorsAndImplementsInterface(const FRVPDPPaintTaskSettings& VertexPaintSettings);

	void SetAllColorChannelsToHaveBeenChanged();

	bool DoesPaintJobUseVertexColorBuffer() const;

	void PaintJobPrintDebugLogs(const FRVPDPApplyColorSetting& ApplyColorSettings, bool AnyVertexColorGotChanged, bool OverrideColorsMadeChange, const FRVPDPAmountOfColorsOfEachChannelResults& ColorsOfEachChannel_AboveZeroBeforeApplyingColor, const FRVPDPAmountOfColorsOfEachChannelResults& ColorsOfEachChannelResultsAtMinAmount_BeforeApplyingColor, const FRVPDPAmountOfColorsOfEachChannelResults& ColorsOfEachChannelResultsAtMinAmount_AfterApplyingColor);

	FRVPDPClosestVertexDataResults GetClosestVertexDataResult(const UPrimitiveComponent* MeshComponent, FRVPDPClosestVertexDataResults ClosestVertexDataResult, const int& ClosestVertexIndex, const FColor& ClosestVertexColor, const FVector& ClosestVertexPosition, const FVector& ClosestVertexNormal);

	void PrintClosestVertexColorResults(const FRVPDPClosestVertexDataResults& ClosestVertexDataResult);


	//-----------  ESSENTIALS BEFORE LOD LOOP  ----------- //

	bool GetWithinAreaInfo(const UPrimitiveComponent* MeshComponent, TArray<FRVPDPComponentToCheckIfIsWithinAreaInfo>& ComponentsToCheckIfWithin);

#if ENGINE_MAJOR_VERSION == 4

	void GetSkeletalMeshClothInfo(USkeletalMeshComponent* SkeletalMeshComponent, USkeletalMesh* SkeletalMesh, TArray<FName> BoneNames, TArray<UClothingAssetBase*>& ClothingAssets, TMap<int32, TArray<FVector>>& ClothvertexPositions, TMap<int32, TArray<FVector>>& ClothVertexNormals, TArray<FVector>& ClothBoneLocationInComponentSpace, TArray<FQuat>& ClothBoneQuaternionsInComponentSpace) const;

#elif ENGINE_MAJOR_VERSION == 5

	void GetSkeletalMeshClothInfo(USkeletalMeshComponent* SkeletalMeshComponent, USkeletalMesh* SkeletalMesh, TArray<FName> BoneNames, TArray<UClothingAssetBase*>& ClothingAssets, TMap<int32, TArray<FVector3f>>& ClothvertexPositions, TMap<int32, TArray<FVector3f>>& ClothVertexNormals, TArray<FVector>& ClothBoneLocationInComponentSpace, TArray<FQuat>& ClothBoneQuaternionsInComponentSpace) const;

#endif

	FRVPDPDoesVertexHasLineOfSightPaintConditionSettings SetLineOfSightPaintConditionParams(FRVPDPDoesVertexHasLineOfSightPaintConditionSettings LineOfSightPaintConditionSettings);


	//-----------  LOD LOOP  ----------- //

	bool GetStaticMeshLODInfo(UStaticMeshComponent* StaticMeshComponent, bool LoopThroughAllVerticesOnLOD, int Lod, TArray<int32>& SectionsToLoopThrough, int& TotalAmountOfSections, FPositionVertexBuffer*& PositionVertexBuffer, FStaticMeshVertexBuffer*& MeshVertexBuffer, int& TotalAmountOfVerticesAtLOD, FRVPDPVertexDataInfo& MeshVertexData, TArray<FColor>& ColorsAtLOD);

	bool GetSkeletalMeshLODInfo(bool LoopThroughAllVerticesOnLOD, FSkeletalMeshRenderData* SkeletalMeshRenderData, FSkeletalMeshLODRenderData*& SkeletalMeshLODRenderData, int Lod, FSkinWeightVertexBuffer*& SkinWeightBuffer, TArray<int32>& SectionsToLoopThrough, int& TotalAmountOfSections, int& TotalAmountOfVerticesAtLOD, FRVPDPVertexDataInfo& MeshVertexData, TArray<FColor>& ColorsAtLOD, int& VertexIndexToStartAt, TArray<TArray<FColor>>& BoneIndexColors);

#if ENGINE_MAJOR_VERSION == 5

	bool GetDynamicMeshLODInfo(UDynamicMesh* DynamicMesh, UE::Geometry::FDynamicMesh3*& DynamicMesh3, TArray<int32>& SectionsToLoopThrough, int& TotalAmountOfVerticesAtLOD, TArray<FColor>& ColorsAtLOD, TArray<FColor>& DefaultColorsFromLOD, TArray<FColor>& DynamicMeshCompVertexColors);

	bool GetGeometryCollectionMeshLODInfo(FGeometryCollection* GeometryCollectionData, TArray<int32>& SectionsToLoopThrough, int& TotalAmountOfVerticesAtLOD, TArray<FColor>& ColorsAtLOD, TArray<FColor>& DefaultColorsFromLOD, TArray<FLinearColor>& GeometryCollectionVertexColorsFromLOD);

#endif

	void GetPaintEntireMeshRandomLODInfo(const FRVPDPPaintOnEntireMeshAtRandomVerticesSettings& PaintEntireMeshRandomVertices, bool PropogateLOD0ToAll, int Lod, int AmountOfVerticesAtLOD, FRandomStream& RandomSeedToUse, float& AmountOfVerticesToRandomize);

	void GetEstimatedColorToHitLocationValues(int VertexToLerpToward, const FColor& ClosestVertexColor, const FVector& ClosestVertexInWorldSpace, const FVector& HitLocationInWorldSpace, const FColor& VertexToLerpTowardColor, const FVector& VertexToLerpTowardPositionInWorldSpace, FLinearColor& EstimatedColorAtHitLocation, FVector& EstimatedHitLocation);

	int GetEstimatedColorToHitLocationVertexToLerpColorToward(const TMap<int, FRVPDPDirectionFromHitLocationToClosestVerticesInfo>& DirectionFromHitToClosestVertices, TArray<FVector>& ClosestVerticesToTheHitLocationInWorldLocation, const FVector& DirectionToClosestVertex, int ClosestVertexBased, float DotGraceRange = 0.2f);

	UMaterialInterface* GetMaterialAtSection(int Section, FSkeletalMeshLODRenderData* SkeletalMeshLODRenderData);

	void ResolveWithinAreaResults(int Lod, int AmountOfVerticesWithinArea, const TArray<FColor>& VertexColorsWithinArea_BeforeApplyingColors, const TArray<FColor>& VertexColorsWithinArea_AfterApplyingColors, const TArray<FVector>& VertexPositionsInComponentSpaceWithinArea, const TArray<FVector>& VertexNormalsWithinArea, const TArray<int>& VertexIndexesWithinArea, FRVPDPAmountOfColorsOfEachChannelResults& ColorsOfEachChannel_WithinArea_BeforeApplyingColors, FRVPDPAmountOfColorsOfEachChannelResults& ColorsOfEachChannel_WithinArea_AfterApplyingColors, FRVPDPWithinAreaResults& WithinAreaResultsBeforeApplyingColors, FRVPDPWithinAreaResults& WithinAreaResultsAfterApplyingColors);

	void ResolveSkeletalMeshBoneColors(int Lod, const TArray<TArray<FColor>>& BoneVertexColors, TArray<FRVPDPBoneVertexColorsInfo>& SkeletalMeshColorsOfEachBone, bool& SuccessfullyGoBoneColors);

	void ResolveChaosClothPhysics(USkeletalMeshComponent* SkelComponent, bool AffectClothPhysics, bool VertexColorsChanged, const FSkeletalMeshLODRenderData* SkeletalMeshLODRenderData, int Lod, const TMap<UClothingAssetBase*, FRVPDPVertexChannelsChaosClothPhysicsSettings>& ChaosClothPhysicsSettings, const TMap<int16, FLinearColor>& ClothAverageColor, TMap<UClothingAssetBase*, FRVPDPChaosClothPhysicsSettings>& ClothPhysicsSettingsBasedOnAverageColor);

	FRVPDPCompareMeshVertexColorsToColorArrayResult GetMatchingColorArrayResults(int AmountOfVerticesConsidered, float MatchingPercent);

	TArray<FVector2D> GetClosestVertexUVs(UPrimitiveComponent* MeshComponent, int32 ClosestVertex);


	//-----------  SECTION LOOP  ----------- //

	void GetStaticMeshComponentSectionInfo(UStaticMeshComponent* StaticMeshComponent, int Lod, int Section, int& SectionMaxAmountOfVertices, int& CurrentLODVertex, UMaterialInterface*& MaterialSection) const;

#if ENGINE_MAJOR_VERSION == 4

	bool GetSkeletalMeshComponentSectionInfo(USkeletalMeshComponent* SkelComponent, bool AffectClothPhysics, int Lod, int Section, FSkeletalMeshRenderData* SkelMeshRenderData, FSkeletalMeshLODRenderData* SkelMeshLODRenderData, const TMap<int32, TArray<FVector>>& ClothVertexPositions, const TMap<int32, TArray<FVector>>& ClothVertexNormals, const TMap<UClothingAssetBase*, FRVPDPVertexChannelsChaosClothPhysicsSettings>& ClothsPhysicsSettings, const TArray<UClothingAssetBase*>& ClothingAssets, int& SectionMaxAmountOfVertices, int& CurrentLODVertex, UMaterialInterface*& MaterialSection, FSkelMeshRenderSection*& SkelMeshRenderSection, bool& ShouldAffectClothPhysics, TArray<FVector>& ClothSectionVertexPositions, TArray<FVector>& ClothSectionVertexNormals);

#elif ENGINE_MAJOR_VERSION == 5

	bool GetSkeletalMeshComponentSectionInfo(USkeletalMeshComponent* SkelComponent, bool AffectClothPhysics, int Lod, int Section, FSkeletalMeshRenderData* SkelMeshRenderData, FSkeletalMeshLODRenderData* SkelMeshLODRenderData, const TMap<int32, TArray<FVector3f>>& ClothVertexPositions, const TMap<int32, TArray<FVector3f>>& ClothVertexNormals, const TMap<UClothingAssetBase*, FRVPDPVertexChannelsChaosClothPhysicsSettings>& ClothsPhysicsSettings, const TArray<UClothingAssetBase*>& ClothingAssets, int& SectionMaxAmountOfVertices, int& CurrentLODVertex, UMaterialInterface*& MaterialSection, FSkelMeshRenderSection*& SkelMeshRenderSection, bool& ShouldAffectClothPhysics, TArray<FVector3f>& ClothSectionVertexPositions, TArray<FVector3f>& ClothSectionVertexNormals);

	void GetDynamicMeshComponentSectionInfo(UDynamicMeshComponent* DynamicMeshComponent, int LodAmountOfVertices, int Section, int& SectionMaxAmountOfVertices, int& CurrentLODVertex, UMaterialInterface*& MaterialSection);

	void GetGeometryCollectionMeshComponentSectionInfo(FGeometryCollection* GeometryCollectionData, UGeometryCollectionComponent* GeometryCollectionComponent, int Section, int& SectionMaxAmountOfVertices, int& CurrentLODVertex, UMaterialInterface*& MaterialSection);

#endif

	void GetMaterialToGetSurfacesFrom(UVertexPaintMaterialDataAsset* MaterialDataAsset, const FRVPDPTaskFundamentalSettings& TaskFundementalSettings, UMaterialInterface* MaterialAtSection, UMaterialInterface*& MaterialToGetSurfacesFrom, TArray<TEnumAsByte<EPhysicalSurface>>& RegisteredPhysicsSurfacesAtMaterial);

	void GetShouldApplyColorsOnChannels(const FRVPDPApplyColorSetting& PaintOnMeshColorSettingsForSection, bool& ShouldApplyOnRedChannel, bool& ShouldApplyOnGreenChannel, bool& ShouldApplyOnBlueChannel, bool& ShouldApplyOnAlphaChannel);

	bool GetPaintOnMeshColorSettingsFromPhysicsSurface(const FRVPDPTaskFundamentalSettings& TaskFundementalSettings, UMaterialInterface* MaterialToGetSurfacesFrom, const FRVPDPApplyColorSetting& PaintOnMeshColorSettingsForSection, TMap<int, TArray<FRVPDPPhysicsSurfaceToApplySettings>>& PhysicsSurfaceToApplySettingsPerVertexChannel);

	void GetChannelsThatContainsPhysicsSurface(const FRVPDPTaskFundamentalSettings& TaskFundementalSettings, const FRVPDPWithinAreaSettings& WithinAreaSettings, const TArray<TEnumAsByte<EPhysicalSurface>>& RegisteredPhysicsSurfacesAtMaterial, bool& ContainsPhysicsSurfaceOnRedChannel, bool& ContainsPhysicsSurfaceOnGreenChannel, bool& ContainsPhysicsSurfaceOnBlueChannel, bool& ContainsPhysicsSurfaceOnAlphaChannel);

	void GetIfPhysicsSurfacesIsRegisteredToVertexColorChannels(const UWorld* World, bool IncludeVertexColorChannelResultOfEachChannel, bool IncludePhysicsSurfaceResultOfEachChannel, const TArray<TEnumAsByte<EPhysicalSurface>>& IncludeOnlyIfPhysicsSurfaceIsRegisteredToAnyChannel, const TArray<TEnumAsByte<EPhysicalSurface>>& PhysicsSurfacesAtMaterial, bool& ContainsPhysicsSurfaceOnRedChannel, bool& ContainsPhysicsSurfaceOnGreenChannel, bool& ContainsPhysicsSurfaceOnBlueChannel, bool& ContainsPhysicsSurfaceOnAlphaChannel);

	void SetWithinColorRangePaintConditionChannelsIfUsingItWithPhysicsSurface(FRVPDPPaintConditionSettings& ChannelsPaintCondition, UMaterialInterface* Material) const;

	void GetClothSectionAverageColor(FSkelMeshRenderSection* SkelMeshRenderSection, bool ShouldAffectPhysicsOnClothSection, int SectionMaxAmountOfVertices, float ClothTotalRedVertexColorsOnSection, float ClothTotalGreenVertexColorsOnSection, float ClothTotalBlueVertexColorsOnSection, float ClothTotalAlphaVertexColorsOnSection, TMap<int16, FLinearColor>& ClothsSectionAvarageColor);

	void ResolveAmountOfColorsOfEachPhysicsSurfaceForSection(int Section, const TArray<TEnumAsByte<EPhysicalSurface>>& RegisteredPhysicsSurfacesAtMaterial, TArray<FRVPDPAmountOfColorsOfEachChannelPhysicsResults>& PhysicsSurfaceResultForSection_BeforeApplyingColors, FRVPDPAmountOfColorsOfEachChannelResults& AmountOfPaintedColorsOfEachChannel_BeforeApplyingColors, TArray<FRVPDPAmountOfColorsOfEachChannelPhysicsResults>& PhysicsSurfaceResultForSection_AfterApplyingColors, FRVPDPAmountOfColorsOfEachChannelResults& AmountOfPaintedColorsOfEachChannel_AfterApplyingColors, TArray<FRVPDPAmountOfColorsOfEachChannelPhysicsResults>& PhysicsSurfaceResultForSection_WithinArea_BeforeApplyingColors, FRVPDPAmountOfColorsOfEachChannelResults& AmountOfPaintedColorsOfEachChannel_WithinArea_BeforeApplyingColors, TArray<FRVPDPAmountOfColorsOfEachChannelPhysicsResults>& PhysicsSurfaceResultForSection_WithinArea_AfterApplyingColors, FRVPDPAmountOfColorsOfEachChannelResults& AmountOfPaintedColorsOfEachChannel_WithinArea_AfterApplyingColors);



	//-----------  VERTEX LOOP  ----------- //

	inline bool GetStaticMeshComponentVertexInfo(FPositionVertexBuffer* PositionVertBuffer, const FStaticMeshVertexBuffer* StaticMeshVertexBuffer, int LodVertexIndex, FVector& VertexPositionInComponentSpace, FVector& VertexPositionInActorSpace, FVector& VertexNormal, bool& GotVertexNormal);


#if ENGINE_MAJOR_VERSION == 4

	inline bool GetSkeletalMeshComponentVertexInfo(const TArray<FMatrix>& SkeletalMeshRefToLocals, const TArray<FName>& SkeletalMeshBoneNames, bool DrawClothVertexDebugPoint, float DrawClothVertexDebugPointDuration, FSkeletalMeshRenderData* SkeletalMeshRenderData, FSkeletalMeshLODRenderData* SkeletalMeshLODRenderData, FSkelMeshRenderSection* SkeletalMeshRenderSection, const FSkinWeightVertexBuffer* SkeletalMeshSkinWeightBuffer, const TArray<FVector>& ClothSectionVertexPositions, const TArray<FVector>& ClothSectionVertexNormals, const TArray<FQuat>& ClothBoneQuaternionsInComponentSpace, const TArray<FVector>& ClothBoneLocationInComponentSpace, int Lod, int Section, int SectionVertexIndex, int LodVertexIndex, int PredictedLOD, FVector& VertexPositionInComponentSpace, FVector& VertexPositionInActorSpace, FVector& VertexNormal, bool& GotVertexNormal, bool& IsVertexOnCloth, uint32& BoneIndex, FName& BoneName, float& BoneWeight);

	inline FVector Modified_GetTypedSkinnedVertexPosition(FSkelMeshRenderSection* Section, const FPositionVertexBuffer& PositionVertexBuffer, const FSkinWeightVertexBuffer* SkinWeightVertexBuffer, const int32 VertIndex, const TArray<FMatrix>& RefToLocals, uint32& BoneIndex, float& BoneWeight);

#elif ENGINE_MAJOR_VERSION == 5

	inline bool GetSkeletalMeshComponentVertexInfo(const TArray<FMatrix44f>& SkeletalMeshRefToLocals, const TArray<FName>& SkeletalMeshBoneNames, bool DrawClothVertexDebugPoint, float DrawClothVertexDebugPointDuration, FSkeletalMeshRenderData* SkeletalMeshRenderData, FSkeletalMeshLODRenderData* SkeletalMeshLODRenderData, FSkelMeshRenderSection* SkeletalMeshRenderSection, const FSkinWeightVertexBuffer* SkeletalMeshSkinWeightBuffer, const TArray<FVector3f>& ClothSectionVertexPositions, const TArray<FVector3f>& ClothSectionVertexNormals, const TArray<FQuat>& ClothBoneQuaternionsInComponentSpace, const TArray<FVector>& ClothBoneLocationInComponentSpace, int Lod, int Section, int SectionVertexIndex, int LodVertexIndex, int PredictedLOD, FVector& VertexPositionInComponentSpace, FVector& VertexPositionInActorSpace, FVector& VertexNormal, bool& GotVertexNormal, bool& IsVertexOnCloth, uint32& BoneIndex, FName& BoneName, float& BoneWeight);

	inline bool GetDynamicMeshComponentVertexInfo(const UE::Geometry::FDynamicMesh3* DynamicMesh3, int TotalAmountOfVerticesAtLOD, int LodVertexIndex, FColor& VertexColor, FVector& VertexPositionInComponentSpace, FVector& VertexPositionInActorSpace, FVector& VertexNormal, bool& GotVertexNormal, UE::Geometry::FVertexInfo& DynamicMeshVertexInfo, int& DynamicMeshMaxVertexID, int& DynamicMeshVerticesBufferMax, bool& SkipDynamicMeshVertex);

	inline FVector Modified_GetTypedSkinnedVertexPosition(FSkelMeshRenderSection* Section, const FPositionVertexBuffer& PositionVertexBuffer, const FSkinWeightVertexBuffer* SkinWeightVertexBuffer, const int32 VertIndex, const TArray<FMatrix44f>& RefToLocals, uint32& BoneIndex, float& BoneWeight);

#if ENGINE_MINOR_VERSION >= 3

	inline bool GetGeometryCollectionComponentVertexInfo(FGeometryCollection* GeometryCollectionData, int LodVertexIndex, FColor& VertexColor, FVector& VertexPositionInComponentSpace, FVector& VertexPositionInActorSpace, FVector& VertexNormal, bool& GotVertexNormal);

#endif
#endif


	inline void GetPaintEntireMeshVertexInfo(const FRVPDPPaintOnEntireMeshAtRandomVerticesSettings& PaintEntireMeshRandomVerticesSettings, const FVector& VertexPositionInWorldSpace, const FVector& VertexPositionInComponentSpace, int TotalAmountOfVerticesAtLOD, int Lod, int LodVertexIndex, const TMap<FVector, FColor>& VertexDublettesPaintedAtLOD0, const FRandomStream& RandomStream, bool& ShouldPaintCurrentVertex, float& AmountOfVerticesLeftToRandomize);

	inline void GetVerticesCloseToEstimatedColorToHitLocation(float& CurrentLongestDistance, int& CurrentLongestDistanceIndex, TMap<int, FRVPDPDirectionFromHitLocationToClosestVerticesInfo>& DirectionFromHitToClosestVertices, const FVector& HitLocationInWorldSpace, const FVector& VertexInWorldSpace, float HitNormalToVertexNormalDot, int VertexLODIndex, int AmountOfLODVertices) const;

	inline bool GetWithinAreaVertexInfoAtLODZeroBeforeApplyingColors(const FRVPDPWithinAreaSettings& WithinAreaSettings, int Lod, const FVector& VertexPositionInWorldSpace, bool& VertexIsWithinArea);

	inline void UpdateColorsOfEachChannelAbove0BeforeApplyingColors(const FLinearColor& VertexColorAsLinear, int Lod, FRVPDPAmountOfColorsOfEachChannelResults& ColorsOfEachChannelAbove0);

	inline void GetWithinAreaVertexInfoAfterApplyingColorsAtLODZero(const FRVPDPWithinAreaSettings& WithinAreaSettings, const TArray<TEnumAsByte<EPhysicalSurface>>& RegisteredPhysicsSurfacesAtMaterial, const FLinearColor& VertexColorAsLinearBeforeApplyingColor, const FLinearColor& VertexColorAsLinearAfterApplyingColor, int Lod, bool VertexWasWithinArea, bool ContainsPhysicsSurfaceOnRedChannel, bool ContainsPhysicsSurfaceOnGreenChannel, bool ContainsPhysicsSurfaceOnBlueChannel, bool ContainsPhysicsSurfaceOnAlphaChannel, FRVPDPAmountOfColorsOfEachChannelResults& AmountOfPaintedColorsOfEachChannelWithinArea_BeforeApplyingColors, TArray<FRVPDPAmountOfColorsOfEachChannelPhysicsResults>& PhysicsSurfaceResultForSection_BeforeApplyingColors, FRVPDPAmountOfColorsOfEachChannelResults& AmountOfPaintedColorsOfEachChannelWithinArea_AfterApplyingColors, TArray<FRVPDPAmountOfColorsOfEachChannelPhysicsResults>& PhysicsSurfaceResultForSection_AfterApplyingColors);

	inline void GetColorsOfEachChannelForVertex(const FLinearColor& CurrentVertexColor, const FRVPDPIncludeAmountOfPaintedColorsOfEachChannelSettings& IncludeAmountOfPaintedColorsOfEachValueSetting, bool HasRequiredPhysSurfaceOnRedChannel, bool HasRequiredPhysSurfaceOnGreenChannel, bool HasRequiredPhysSurfaceOnBlueChannel, bool HasRequiredPhysSurfaceOnAlphaChannel, FRVPDPAmountOfColorsOfEachChannelChannelResults& RedChannelResult, FRVPDPAmountOfColorsOfEachChannelChannelResults& GreenChannelResult, FRVPDPAmountOfColorsOfEachChannelChannelResults& BlueChannelResult, FRVPDPAmountOfColorsOfEachChannelChannelResults& AlphaChannelResult);

	inline void GetColorsOfEachPhysicsSurfaceForVertex(const TArray<TEnumAsByte<EPhysicalSurface>>& RegisteredPhysicsSurfaceAtMaterial, const FLinearColor& CurrentVertexColor, const FRVPDPIncludeAmountOfPaintedColorsOfEachChannelSettings& IncludeAmountOfPaintedColorsOfEachValueSetting, bool HasRequiredPhysSurfaceOnRedChannel, bool HasRequiredPhysSurfaceOnGreenChannel, bool HasRequiredPhysSurfaceOnBlueChannel, bool HasRequiredPhysSurfaceOnAlphaChannel, TArray<FRVPDPAmountOfColorsOfEachChannelPhysicsResults>& PhysicsSurfaceResultToFill);

	

	inline bool DoesVertexColorsMatch(const FColor& VertexColor, const FColor& CompareColor, int32 ErrorTolerance, bool CompareOnRedChannel, bool CompareOnGreenChannel, bool CompareOnBlueChannel, bool CompareOnAlphaChannel);

	inline FColor GetColorToApplyOnVertex(UVertexPaintDetectionComponent* AssociatedPaintComponent, UObject* ObjectToOverrideVertexColorsWith, int CurrentLOD, int CurrentVertexIndex, UMaterialInterface* MaterialVertexIsOn, const TArray<TEnumAsByte<EPhysicalSurface>>& RegisteredPhysicsSurfacesAtMaterialVertexIsOn, const TMap<int, TArray<FRVPDPPhysicsSurfaceToApplySettings>>& physicsSurfacePaintSettingsPerChannel, bool isVertexOnCloth, const FLinearColor& currentLinearVertexColor, const FColor& currentVertexColor, const FVector& currentVertexWorldPosition, const FVector& CurrentVertexNormal, const FName& CurrentBoneName, float BoneWeight, const FRVPDPVertexPaintFallOffSettings& FallOffSettings, float FallOffRange, float ScaleFallOffFrom, float DistanceFromFalloffBaseToVertexLocation, const FRVPDPApplyColorSetting& PaintOnMeshColorSetting, bool ApplyColorOnRedChannel, bool ApplyColorOnGreenChannel, bool ApplyColorOnBlueChannel, bool ApplyColorOnAlphaChannel, bool& VertexGotColorChanged, bool& HaveRunOverrideInterface, bool& ShouldApplyColorOnAnyChannel, bool hasAnyPaintCondition, bool hasPhysicsSurfacePaintCondition, bool hasPaintConditionOnRedChannel, bool hasPaintConditionOnGreenChannel, bool hasPaintConditionOnBlueChannel, bool hasPaintConditionOnAlphaChannel);

	inline void GetVertexColorAfterAnyLimitation(const FColor& CurrentVertexColor, const FRVPDPPaintLimitSettings& PaintLimitRedChannel, const FRVPDPPaintLimitSettings& PaintLimitGreenChannel, const FRVPDPPaintLimitSettings& PaintLimitBlueChannel, const FRVPDPPaintLimitSettings& PaintLimitAlphaChannel, float NewRedVertexColorToApply, float NewGreenVertexColorToApply, float NewBlueVertexColorToApply, float NewAlphaVertexColorToApply, float& FinalRedVertexColorToApply, float& FinalGreenVertexColorToApply, float& FinalBlueVertexColorToApply, float& FinalAlphaVertexColorToApply);

	inline bool WillAppliedColorMakeChangeOnVertex(const FColor& CurrentColorOnVertex, const FRVPDPApplyColorSetting& PaintColorSettings, const TMap<int, TArray<FRVPDPPhysicsSurfaceToApplySettings>>& PhysicsSurfacePaintSettingsPerChannel);

	inline bool IsCurrentVertexWithinPaintColorConditions(UMaterialInterface* VertexMaterial, const FColor& CurrentVertexColor, const FVector& CurrentVertexPosition, const FVector& CurrentVertexNormal, const FName& CurrentBoneName, float BoneWeight, const FRVPDPPaintConditionSettings& PaintConditions, float& ColorStrengthIfConditionsIsNotMet, EApplyVertexColorSetting& ColorSettingIfConditionIsNotMet);

	inline FVector GetSplineMeshVertexPositionInMeshSpace(const FVector& VertexPos, const USplineMeshComponent* SplineMeshComponent);

	inline FVector GetSplineMeshVertexNormalInMeshSpace(const FVector& Normal, const USplineMeshComponent* SplineMeshComponent);

	inline bool IsVertexWithinArea(UPrimitiveComponent* ComponentToCheckIfWithin, const FVector& VertexWorldPosition, const FRVPDPComponentToCheckIfIsWithinAreaInfo& ComponentToCheckIfIsWithinInfo) const;

	inline void GetSerializedVertexColor(int AmountOfLODVertices, const FColor& VertexColor, int LodVertexIndex, FString& SerializedColorData);

	inline float GetPaintAtLocationDistanceFromFallOffBase(const FRVPDPPaintAtLocationFallOffSettings& PaintAtLocationFallOffSettings, float DistanceFromVertexToHitLocation) const;

	inline float GetPaintWithinAreaDistanceFromFalloffBase(const FRVPDPPaintWithinAreaFallOffSettings& PaintWithinAreaFallOffSettings, const FRVPDPComponentToCheckIfIsWithinAreaInfo& ComponentToCheckIfIsWithinInfo, const FVector& CurrentVertexInWorldSpace) const;

	inline void CompareColorsSnippetToVertex(const FRVPDPCompareMeshVertexColorsToColorSnippetsSettings& CompareColorSnippetSettings, const TArray<TArray<FColor>>& CompareSnippetsColorsArray, int CurrentLODVertex, const FColor& VertexColor, TArray<int>& AmountOfVerticesConsidered, TArray<float>& MatchingPercent);

	inline FRVPDPCompareMeshVertexColorsToColorSnippetResult GetMatchingColorSnippetsResults(const FRVPDPCompareMeshVertexColorsToColorSnippetsSettings& CompareColorSnippetSettings, const TArray<TArray<FColor>>& CompareSnippetsColorsArray, const TArray<int>& AmountOfVerticesConsidered, TArray<float>& MatchingPercent);

	inline void CompareColorArrayToVertex(const FRVPDPCompareMeshVertexColorsToColorArraySettings& CompareColorArraySettings, int CurrentLODVertex, const FColor& VertexColor, int& AmountOfVerticesConsidered, float& MatchingPercent);



	bool InGameThread = false;
	FRVPDPCalculateColorsInfo CalculateColorsInfoGlobal;
	TArray<FRVPDPAsyncTaskDebugMessagesInfo> CalculateColorsResultMessage;

	TArray<FColor> ColorSnippetColors;
	TArray<FColor> DeserializedVertexColors;

	// Caches these so the checks we do throughout the code is a bit cleaner
	bool GetClosestVertexData = false;
	bool GetColorsWithinArea = false;
};
