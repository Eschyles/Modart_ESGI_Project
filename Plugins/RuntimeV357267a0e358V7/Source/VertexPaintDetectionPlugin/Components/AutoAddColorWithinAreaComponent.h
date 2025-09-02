// Copyright 2022-2024 Alexander Floden, Alias Alex River. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AutoAddColorComponent.h"
#include "AutoAddColorWithinAreaComponent.generated.h"



UCLASS(Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent), DisplayName = "Auto Paint Within Area Component", ClassGroup = (Custom))
class VERTEXPAINTDETECTIONPLUGIN_API UAutoAddColorWithinAreaComponent : public UAutoAddColorComponent
{
	GENERATED_BODY()


protected:

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


public:

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint|Within Area", meta = (ToolTip = "Adds a Mesh to be Auto painted with specific settings. If it's already been added then updated the settings with the new one. \nIn AutoAddColorSettings you can set a delay between tasks, which is useful if you for instance is drying a character but it's going to fast, then you add maybe 0.1 or so to make it slower. "))
	bool AddAutoPaintWithinArea(UPrimitiveComponent* MeshComponent, FRVPDPPaintWithinAreaSettings PaintWithinAreaSettings, FRVPDPAdditionalDataToPassThroughInfo AdditionalData, FRVPDPAutoAddColorSettings AutoAddColorSettings, bool ResumeIfPaused = false);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint|Within Area", meta = (ToolTip = "Update the Settings of a Mesh being Auto Painted with Within Area. \nIn AutoAddColorSettings you can set a delay between tasks, which is useful if you for instance is drying a character but it's going to fast, then you add maybe 0.1 or so to make it slower. "))
	void UpdateAutoPaintedWithinArea(UPrimitiveComponent* MeshComponent, FRVPDPPaintWithinAreaSettings PaintWithinAreaSettings, FRVPDPAdditionalDataToPassThroughInfo AdditionalData, FRVPDPAutoAddColorSettings AutoAddColorSettings, bool ResumeIfPaused = false);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint|Within Area")
	const TMap<UPrimitiveComponent*, FRVPDPPaintWithinAreaSettings>& GetAutoPaintingWithinAreaPaintSettings() { return AutoPaintingWithinAreaWithSettings; }

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint|Within Area")
	const TMap<UPrimitiveComponent*, FRVPDPAdditionalDataToPassThroughInfo>& GetAutoPaintingWithinAreaAdditionalDataSettings() { return AutoPaintingWithinAreaAdditionalDataSettings; }


	virtual void StopAllAutoPainting() override;

	virtual void StopAutoPaintingMesh(UPrimitiveComponent* MeshComponent) override;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint|Within Area|Optimizations", meta = (ToolTip = "Optimization so we start another auto paint within area if any change has occured on the Mesh Location Z. Can be combined with the other options so if Any of them passes their check, we start a new task. \nFor instance if a character actively walking down into a lake and not just standing in it. The draw back is that this component will have to tick to check if the change has happened. \nIf not checking any of the location, rotation or scale then the parent will decide if it should start a new one when the paint job was finished based on its colors then. But it will not be able to start any new colors automatically because the mesh has moved etc. "))
	bool OnlyPaintOnMovingMeshZ = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint|Within Area|Optimizations", meta = (ToolTip = "If set to start new Task if Mesh has moved up/down on Z, then this is the difference in Z that has to occur for it to start a new task. "))
	float OnlyPaintOnMovingMeshZDifferenceRequired = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint|Within Area|Optimizations", meta = (ToolTip = "Optimization so we start another auto paint within area if any change has occured on the Mesh Location XY. Can be combined with the other options so if Any of them passes their check, we start a new task.  For instance if a character is swimming in a lake but is just floating at the surface and not swimming in any direction. The draw back is that this component will have to tick to check if the change has happened. \nIf not checking any of the location, rotation or scale then the parent will decide if it should start a new one when the paint job was finished based on its colors then. But it will not be able to start any new colors automatically because the mesh has moved etc."))
	bool OnlyPaintOnMovingMeshXY = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint|Within Area|Optimizations", meta = (ToolTip = "If set to start new Task if Mesh has moved on XY, then this is the difference in XY that has to occur for it to start a new task. "))
	float OnlyPaintOnMovingMeshXYDifferenceRequired = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint|Within Area|Optimizations", meta = (ToolTip = "Optimization so we start another auto paint within area if any change has occured on the Mesh Rotation. Can be combined with the other options so if Any of them passes their check, we start a new task. The draw back is that this component will have to tick to check if the change has happened. \nIf not checking any of the location, rotation or scale then the parent will decide if it should start a new one when the paint job was finished based on its colors then. But it will not be able to start any new colors automatically because the mesh has moved etc."))
	bool OnlyPaintOnRotatedMesh = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint|Within Area|Optimizations", meta = (ToolTip = "If set to start new Task if Mesh has Rotated, then this is the difference in Rotation that has to occur for it to start a new task. "))
	float OnlyPaintOnRotatedMeshDifferenceRequired = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint|Within Area|Optimizations", meta = (ToolTip = "Optimization so we start another auto paint within area if any change has occured on the Mesh Scale. Can be combined with the other options so if Any of them passes their check, we start a new task. The draw back is that this component will have to tick to check if the change has happened. \nIf not checking any of the location, rotation or scale then the parent will decide if it should start a new one when the paint job was finished based on its colors then. But it will not be able to start any new colors automatically because the mesh has moved etc."))
	bool OnlyPaintOnReScaledMesh = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint|Within Area|Optimizations", meta = (ToolTip = "If set to start new Task if Mesh has has been rescaled, then this is the difference in in scale that has to occur for it to start a new task. "))
	float OnlyPaintOnReScaledMeshDifferenceRequired = 0.01f;


protected:

	virtual bool ShouldAlwaysTick() const override;

	virtual void StartAutoPaintTask(UPrimitiveComponent* meshComponent) override;

	virtual bool CanStartNewTaskBasedOnTaskResult(const FRVPDPTaskResults& TaskResultInfo, const FRVPDPPaintTaskResultInfo& PaintTaskResultInfo, bool& IsFullyPainted, bool& IsCompletelyEmpty) override;

	virtual bool GetApplyVertexColorSettings(UPrimitiveComponent* MeshComponent, FRVPDPApplyColorSettings& ApplyVertexColorSettings) override;

	virtual void VerifyAutoPaintingMeshComponents() override;


private:

	bool HasMeshMovedSinceLastPaintJob(const UPrimitiveComponent* MeshComponent, const FVector& LastMeshLocation) const;

	bool HasMeshRotatedSinceLastPaintJob(const UPrimitiveComponent* MeshComponent, const FRotator& LastMeshRotation) const;

	bool HasMeshReScaledSinceLastPaintJob(const UPrimitiveComponent* MeshComponent, const FVector& LastMeshScale) const ;


protected:

	UPROPERTY()
	TMap<UPrimitiveComponent*, FRVPDPPaintWithinAreaSettings> AutoPaintingWithinAreaWithSettings;

	UPROPERTY()
	TMap<UPrimitiveComponent*, FRVPDPAdditionalDataToPassThroughInfo> AutoPaintingWithinAreaAdditionalDataSettings;

	UPROPERTY()
	TMap<UPrimitiveComponent*, FTransform> MeshComponentsToCheckIfMoved;
};
