// Copyright 2023 Alexander Floden, Alias Alex River. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "AutoAddColorComponent.h"
#include "AutoAddColorPaintAtLocComponent.generated.h"


UCLASS(Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent), DisplayName = "Auto Paint At Location Component", ClassGroup = (Custom))
class VERTEXPAINTDETECTIONPLUGIN_API UAutoAddColorPaintAtLocComponent : public UAutoAddColorComponent
{
	GENERATED_BODY()


protected:

	UAutoAddColorPaintAtLocComponent();
	virtual void BeginPlay() override;

public:

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint|Paint At Location", meta = (ToolTip = "Adds a Mesh to be Auto painted with specific settings. If it's already been added then updated the settings with the new one. \nIn AutoAddColorSettings you can set a delay between tasks, which is useful if you for instance is drying a character but it's going to fast, then you add maybe 0.1 or so to make it slower. "))
	bool AddAutoPaintAtLocation(UPrimitiveComponent* MeshComponent, FRVPDPPaintAtLocationSettings PaintAtLocationSettings, FRVPDPAdditionalDataToPassThroughInfo AdditionalData, FRVPDPAutoAddColorSettings AutoAddColorSettings, bool ResumeIfPaused = false);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint|Paint At Location", meta = (ToolTip = "Update the Settings of a Mesh being Auto Painted AtLocation.. \nIn AutoAddColorSettings you can set a delay between tasks, which is useful if you for instance is drying a character but it's going to fast, then you add maybe 0.1 or so to make it slower. "))
	void UpdateAutoPaintedAtLocation(UPrimitiveComponent* MeshComponent, FRVPDPPaintAtLocationSettings PaintAtLocationSettings, FRVPDPAdditionalDataToPassThroughInfo AdditionalData, FRVPDPAutoAddColorSettings AutoAddColorSettings, bool ResumeIfPaused = false);

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint|Paint At Location")
	const TMap<UPrimitiveComponent*, FRVPDPPaintAtLocationSettings>& GetAutoPaintingAtLocationPaintSettings() { return AutoPaintingAtLocationWithSettings; }

	UFUNCTION(BlueprintCallable, Category = "Runtime Vertex Paint and Detection Plugin|Auto Paint|Paint At Location")
	const TMap<UPrimitiveComponent*, FRVPDPAdditionalDataToPassThroughInfo>& GetAutoPaintingAtLocationAdditionalDataSettings() { return AutoPaintingAtLocationAdditionalDataSettings; }


	virtual void StopAllAutoPainting() override;

	virtual void StopAutoPaintingMesh(UPrimitiveComponent* MeshComponent) override;


protected:

	virtual void StartAutoPaintTask(UPrimitiveComponent* MeshComponent) override;

	virtual bool GetApplyVertexColorSettings(UPrimitiveComponent* MeshComponent, FRVPDPApplyColorSettings& ApplyVertexColorSettings) override;

	virtual void VerifyAutoPaintingMeshComponents() override;


private:

	UPROPERTY()
	TMap<UPrimitiveComponent*, FRVPDPPaintAtLocationSettings> AutoPaintingAtLocationWithSettings;

	UPROPERTY()
	TMap<UPrimitiveComponent*, FRVPDPAdditionalDataToPassThroughInfo> AutoPaintingAtLocationAdditionalDataSettings;
};
