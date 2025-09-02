// Copyright 2022-2024 Alexander Floden, Alias Alex River. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "VertexPaintDetectionComponent.h"
#include "AutoPaintWithinAreaUpToMaxSize.generated.h"


class USphereComponent;
class UBoxComponent;
class UCapsuleComponent;


USTRUCT(BlueprintType, meta = (DisplayName = "Auto Paint Up To Base Settings"))
struct FRVPDPAutoPaintUpToBaseSettings {

	GENERATED_BODY();

	UPROPERTY(BlueprintReadWrite, Category = "Auto Paint Up To Base Settings", meta = (ToolTip = "How fast we increase the Shapes Size.  "))
	float ShapeIncreaseSpeed = 10;

	UPROPERTY(BlueprintReadWrite, Category = "Auto Paint Up To Base Settings")
	FRVPDPPaintWithinAreaFallOffSettings FallOffSettings;

	UPROPERTY(BlueprintReadWrite, Category = "Auto Paint Up To Base Settings", meta = (ToolTip = "If True, then the size of the shape will only increase if a Paint Task is Started, which is recommended because it means even if it runs on a Heavy or a Light Mesh (that takes differently long amount of times to finish) the final result should look the same. Also useful in case you only want to run a fixed number of paint jobs, then you can simply increase the speed so after those number of paint jobs it should be over the size and destroy itself. "))
	bool OnlyIncreaseShapeSizeWhenTaskIsRun = true;

	UPROPERTY(BlueprintReadWrite, Category = "Auto Paint Up To Base Settings", meta = (ToolTip = "Can be useful if you want further control over how slow it should paint, since even with the lowest paint strength it will only take a few seconds to fully paint something (FColor goes from 0-255, lowest you can paint is 1 so 255 frames to fully paint something). But with this you can add a small delay between tasks if you want to paint even slower.  "))
	float DelayBetweenPaintTasks = 0;

	float DelayBetweenPaintTaskCurrentCounter = 0;
};

USTRUCT(BlueprintType, meta = (DisplayName = "Auto Paint Up To Sphere Settings"))
struct FRVPDPAutoPaintUpToSphereSettings : public FRVPDPAutoPaintUpToBaseSettings {

	GENERATED_BODY();

	UPROPERTY(BlueprintReadWrite, Category = "Auto Paint Up To Sphere Settings", meta = (ToolTip = "Radius the Paint Within Area with Sphere Shape should Start at. "))
	float StartRadius = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Auto Paint Up To Sphere Settings", meta = (ToolTip = "Radius the component will remove this Auto Paint Sphere Settings. If there are no other Auto Paint Settings the component will destroy itself. "))
	float MaxRadius = 30;


	UPROPERTY()
	USphereComponent* AutoPaintSphereComponent = nullptr;

	float CurrentSpreadRadius = 0;
};


USTRUCT(BlueprintType, meta = (DisplayName = "Auto Paint Up To Rectangle Settings"))
struct FRVPDPAutoPaintUpToRectangleSettings : public FRVPDPAutoPaintUpToBaseSettings {

	GENERATED_BODY();

	UPROPERTY(BlueprintReadWrite, Category = "Auto Paint Up To Rectangle Settings", meta = (ToolTip = "Extent the Paint Within Area with Rectangle Shape should Start at. "))
	FVector StartExtent = FVector(ForceInitToZero);

	UPROPERTY(BlueprintReadWrite, Category = "Auto Paint Up To Rectangle Settings", meta = (ToolTip = "If the Extent has reached the Starting Extent + this, then the component will remove this Auto Paint Rectangle Settings. If there are no other Auto Paint Settings the component will destroy itself. "))
	float MaxExtentFromStart = 10;


	UPROPERTY()
	UBoxComponent* AutoPaintBoxComponent = nullptr;

	float CurrentExtent = 0;
};


USTRUCT(BlueprintType, meta = (DisplayName = "Auto Paint Up To Capsule Settings"))
struct FRVPDPAutoPaintUpToCapsuleSettings : public FRVPDPAutoPaintUpToBaseSettings {

	GENERATED_BODY();

	UPROPERTY(BlueprintReadWrite, Category = "Auto Paint Up To Capsule Settings", meta = (ToolTip = "Start Radius Paint Within Area with Capsule Shape should Start at. "))
	float StartRadius = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Auto Paint Up To Capsule Settings", meta = (ToolTip = "If the Capsule has reached this Max Radius and the Max Half Height, then the component will remove this Auto Paint Capsule Settings. If there are no other Auto Paint Settings the component will destroy itself. "))
	float MaxRadius = 20;

	UPROPERTY(BlueprintReadWrite, Category = "Auto Paint Up To Capsule Settings", meta = (ToolTip = "Start Half Height Paint Within Area with Capsule Shape should Start at. "))
	float StartHalfHeight = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Auto Paint Up To Capsule Settings", meta = (ToolTip = "If the Capsule has reached this Max Half Height and the Max Radius, then the component will remove this Auto Paint Capsule Settings. If there are no other Auto Paint Settings the component will destroy itself. "))
	float MaxHalfHeight = 40;


	UPROPERTY()
	UCapsuleComponent* AutoPaintCapsuleComponent = nullptr;

	float CurrentRadius = 0;

	float CurrentHalfHeight = 0;
};


USTRUCT()
struct FRVPDPAutoPaintUpToMaxSizeSettings {

	GENERATED_BODY();

	UPROPERTY()
	TArray<FRVPDPAutoPaintUpToSphereSettings> AutoPaintSphereSettings;

	UPROPERTY()
	TArray<FRVPDPAutoPaintUpToRectangleSettings> AutoPaintRectangleSettings;

	UPROPERTY()
	TArray<FRVPDPAutoPaintUpToCapsuleSettings> AutoPaintCapsuleSettings;
};



UCLASS( ClassGroup=(Custom), Blueprintable, BlueprintType, meta=(BlueprintSpawnableComponent), DisplayName = "Auto Paint Within Area Up To Max Size")
class VERTEXPAINTDETECTIONPLUGIN_API UAutoPaintWithinAreaUpToMaxSize : public UVertexPaintDetectionComponent
{
	GENERATED_BODY()

protected:

	UAutoPaintWithinAreaUpToMaxSize();
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void VerifyComponent();

public:

	UFUNCTION(BlueprintCallable, Category = "Auto Paint Within Area Up To Max Size", meta = (ToolTip = "Starts Auto Painting in Sphere Shape up to a Certain Size. "))
	void AddAutoPaintWithinSphere(UPrimitiveComponent* MeshComponent, FVector Location, FName Bone, FRVPDPAutoPaintUpToSphereSettings AutoPaintUpToSphereSettings);

	UFUNCTION(BlueprintCallable, Category = "Auto Paint Within Area Up To Max Size", meta = (ToolTip = "Starts Auto Painting in Rectangle Shape up to a Certain Size. "))
	void AddAutoPaintWithinRectangle(UPrimitiveComponent* MeshComponent, FVector Location, FRotator Rotation, FName Bone, FRVPDPAutoPaintUpToRectangleSettings AutoPaintUpToRectangleSettings);

	UFUNCTION(BlueprintCallable, Category = "Auto Paint Within Area Up To Max Size", meta = (ToolTip = "Starts Auto Painting in Capsule Shape up to a Certain Size. "))
	void AddAutoPaintWithinCapsule(UPrimitiveComponent* MeshComponent, FVector Location, FRotator Rotation, FName Bone, FRVPDPAutoPaintUpToCapsuleSettings AutoPaintUpToCapsuleSettings);

	UFUNCTION(BlueprintCallable, Category = "Auto Paint Within Area Up To Max Size", meta = (ToolTip = "Destroys itself and therefor stops all auto painting, same thing that happens when Finished Auto Painting. "))
	void StopAllAutoPainting();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Auto Paint Within Area Up To Max Size", meta = (ExposeOnSpawn = true))
	FRVPDPApplyColorSettings ApplyVertexColorSettings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Auto Paint Within Area Up To Max Size", meta = (ExposeOnSpawn = true))
	FRVPDPDebugSettings TaskDebugSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto Paint Within Area Up To Max Size", meta = (ExposeOnSpawn = true))
	bool DebugAutoPaintAreaUpToLogsToScreen = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto Paint Within Area Up To Max Size", meta = (ExposeOnSpawn = true))
	float DebugAutoPaintAreaUpToScreen_Duration = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto Paint Within Area Up To Max Size", meta = (ExposeOnSpawn = true))
	bool DebugAutoPaintAreaUpToOutputLog = false;


protected:
	
	UPROPERTY()
	TMap<UPrimitiveComponent*, FRVPDPAutoPaintUpToMaxSizeSettings> AutoPaintingUpToMaxSizeOnMeshComponents;
};
