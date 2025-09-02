// Copyright 2022-2024 Alexander Floden, Alias Alex River. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AutoAddColorComponent.h"
#include "AutoAddColorEntireMeshComponent.generated.h"


UCLASS(Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent), DisplayName = "Auto Paint Entire Mesh Component", ClassGroup = (Custom))
class VERTEXPAINTDETECTIONPLUGIN_API UAutoAddColorEntireMeshComponent : public UAutoAddColorComponent
{
	GENERATED_BODY()

protected:

	virtual void BeginPlay() override;

public:

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint|Entire Mesh", meta = (ToolTip = "Adds a Mesh to be Auto painted with specific settings. If it's already been added then updated the settings with the new one. \nIn AutoAddColorSettings you can set a delay between tasks, which is useful if you for instance is drying a character but it's going to fast, then you add maybe 0.1 or so to make it slower. "))
	bool AddAutoPaintEntireMesh(UPrimitiveComponent* MeshComponent, FRVPDPPaintOnEntireMeshSettings PaintEntireMeshSettings, FRVPDPAdditionalDataToPassThroughInfo AdditionalData, FRVPDPAutoAddColorSettings AutoAddColorSettings, bool ResumeIfPaused = false);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint|Entire Mesh", meta = (ToolTip = "Update the Settings of a Mesh being Auto Painted with Entire Mesh. \nIn AutoAddColorSettings you can set a delay between tasks, which is useful if you for instance is drying a character but it's going to fast, then you add maybe 0.1 or so to make it slower. "))
	void UpdateAutoPaintedEntireMesh(UPrimitiveComponent* MeshComponent, FRVPDPPaintOnEntireMeshSettings PaintEntireMeshSettings, FRVPDPAdditionalDataToPassThroughInfo AdditionalData, FRVPDPAutoAddColorSettings AutoAddColorSettings, bool ResumeIfPaused = false);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint|Entire Mesh")
	const TMap<UPrimitiveComponent*, FRVPDPPaintOnEntireMeshSettings>& GetAutoPaintingEntireMeshesPaintSettings() { return AutoPaintingEntireMeshesWithSettings; }

		UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint|Entire Mesh")
	const TMap<UPrimitiveComponent*, FRVPDPAdditionalDataToPassThroughInfo>& GetAutoPaintingEntireMeshesAdditionalDataSettings() { return AutoPaintingEntireMeshesAdditionalDataSettings; }


	virtual void StopAllAutoPainting() override;

	virtual void StopAutoPaintingMesh(UPrimitiveComponent* MeshComponent) override;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint|Auto Paint Root Mesh at Begin Play", meta = (ToolTip = "The Paint Entire Mesh Settings to paint the root mesh with at begin play. "))
	FRVPDPPaintOnEntireMeshSettings AutoPaintRootMeshAtBeginPlay_PaintEntireMeshSettings;


protected:

	virtual void StartAutoPaintTask(UPrimitiveComponent* MeshComponent) override;

	virtual bool GetApplyVertexColorSettings(UPrimitiveComponent* MeshComponent, FRVPDPApplyColorSettings& ApplyVertexColorSettings) override;

	virtual void VerifyAutoPaintingMeshComponents() override;

	UPROPERTY()
	TMap<UPrimitiveComponent*, FRVPDPPaintOnEntireMeshSettings> AutoPaintingEntireMeshesWithSettings;

	UPROPERTY()
	TMap<UPrimitiveComponent*, FRVPDPAdditionalDataToPassThroughInfo> AutoPaintingEntireMeshesAdditionalDataSettings;
};
