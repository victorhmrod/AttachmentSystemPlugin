// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Attachment_Base.h"
#include "Components/TimelineComponent.h"
#include "Attachment_Magni.generated.h"

/**
 * 
 */
UCLASS()
class ATTACHMENTSYSTEM_API AAttachment_Magni : public AAttachment_Base
{
	GENERATED_BODY()
	
public:
	//==================================================
	// PROPERTIES & VARIABLES
	//==================================================
	UPROPERTY(EditAnywhere, Category = "Mesh")
	TObjectPtr<UStaticMeshComponent> Mesh {nullptr};
	
	UPROPERTY(EditAnywhere, Category = "Mesh")
	TObjectPtr<USceneCaptureComponent2D> RenderTarget2D {nullptr};

	UPROPERTY(EditAnywhere, Category = "Parameters")
	TObjectPtr<UCurveFloat> MovementCurve;

	UPROPERTY(EditAnywhere, Category = "Parameters")
	FRotator OriginalRotation {FRotator::ZeroRotator};
	UPROPERTY(EditAnywhere, Category = "Parameters")
	FRotator FinalRotation {FRotator::ZeroRotator};

	
	
protected:
	//==================================================
	// PROPERTIES & VARIABLES
	//==================================================
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> MagniBodyMaterial {nullptr};
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> MagniLensMaterial {nullptr};
	
	//==================================================
	// FUNCTIONS
	//==================================================
	AAttachment_Magni();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	virtual void DoAction() override;

private:
	//==================================================
	// PROPERTIES & VARIABLES
	//==================================================
	bool bFlipFlop {false};
	//==================================================
	// FUNCTIONS
	//==================================================
	FTimeline MagniMovement;

	UFUNCTION()
	void UpdateMagniMovement(const float Alpha);

	
	void SetMagniMat(const bool bActivate);
};
