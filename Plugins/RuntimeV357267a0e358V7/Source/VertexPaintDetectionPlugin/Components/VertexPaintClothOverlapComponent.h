// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Engine/EngineTypes.h"
#include "VertexPaintClothOverlapComponent.generated.h"


class USkeletalMeshComponent;
class UClothingAssetBase;
struct FTraceHandle;
struct FTraceDatum;


UCLASS(Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent), DisplayName = "Cloth Overlap Component", ClassGroup = (Custom))
class VERTEXPAINTDETECTIONPLUGIN_API UVertexPaintClothOverlapComponent : public USceneComponent
{
	GENERATED_BODY()


protected:

	UVertexPaintClothOverlapComponent();
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;

	UFUNCTION()
	virtual void OnRep_ClothOverlapTracingEnabled();

	void ActivateClothOverlapTracing();
	void DeActivateClothOverlapTracing();
	void OnSphereTraceComplete(const FTraceHandle& TraceHandle, FTraceDatum& TraceData);

public:

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Cloth Overlap", meta = (ToolTip = "Sets the Cloth Objects it should sphere trace to. "))
		void SetClothObjectsTypesToSphereTrace(TArray<TEnumAsByte<EObjectTypeQuery>> ClothObjectTypes) { ClothObjectsToSphereTrace = ClothObjectTypes; };

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Cloth Overlap", meta = (ToolTip = "Activates / DeActivates Cloth Overlap Tracing. Useful if you don't want to have it activated all the time but enable it when you want to have more attention to detail on the cloth, like in a cutscene or something similar. "))
		void SetClothOverlapTracingEnabled(bool EnableClothTracing);

	UFUNCTION(BlueprintPure, Category = "Runtime Vertex Paint and Detection Plugin|Cloth Overlap", meta = (ToolTip = "Returns the Cloth Objects currently set to sphere trace. "))
		TArray<TEnumAsByte<EObjectTypeQuery>> GetClothObjectsToSphereTrace() { return ClothObjectsToSphereTrace; };



	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_ClothOverlapTracingEnabled, Category = "Cloth Overlap|Trace Settings", meta = (ToolTip = "If we should run the traces at all"))
		bool ClothOverlapTracingEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cloth Overlap|Trace Settings", meta = (ToolTip = "Size of each sphere trace on the cloth vertex position. Can be larger if you opt to only trace every other vertex or similar. "))
		float ClothVertexTraceRadius = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cloth Overlap|Trace Settings", meta = (ToolTip = "0 or 1 = Traces every vertex, 2 every other vertex etc. \nCan be used to optimize the tracing so you don't run them on all simulated cloth positions!"))
		int ClothVertexTraceInterval = 7;

	UPROPERTY(EditDefaultsOnly, Category = "Cloth Overlap|Trace Settings", meta = (ToolTip = "Can set which channels it should overlap here, so in case you want it to overlap other channels other than the skeletal mesh component its attached then that is possible. "))
		TArray<TEnumAsByte<EObjectTypeQuery>> ClothObjectsToSphereTrace;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cloth Overlap|Debug Settings", meta = (ToolTip = "Show the sphere trace locations"))
		bool DebugClothSphereTraces = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cloth Overlap|Debug Settings", meta = (ToolTip = "Duration of the debug trace. "))
		float DebugClothSphereTracesDuration = 0.05f;


protected:

	UPROPERTY()
	USkeletalMeshComponent* AttachedSkeletalMeshComponent;

	UPROPERTY()
	TArray<UClothingAssetBase*> AttachedSkeletalMeshComponentClothingAssets;

	UPROPERTY()
	TMap<UPrimitiveComponent*, int32> ClothOverlappingComponentAndItemsCache;

	UPROPERTY()
	TMap<UClothingAssetBase*, FTransform> ClothBoneTransformsInComponentSpace;

	UPROPERTY()
	TMap<UClothingAssetBase*, FQuat> ClothBoneQuaternionsInComponentSpace;


private:

	UPROPERTY()
		TMap<UPrimitiveComponent*, int32> CurrentTraceOverlappingComponentAndItemsCache;

	int ClothTraceExpectedCallbacks = 0;
	int ClothTraceCurrentCallbacks = 0;
};
