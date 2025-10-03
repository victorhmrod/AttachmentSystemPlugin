// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Attachment_Base.h"
#include "Attachment_4x.generated.h"

/**
 *
 */
UCLASS()
class ATTACHMENTSYSTEM_API AAttachment_4x : public AAttachment_Base {
  GENERATED_BODY()

public:
  UPROPERTY(EditAnywhere, Category = "Mesh")
  TObjectPtr<USceneCaptureComponent2D> RenderTarget2D = nullptr;

protected:
  //==================================================
  // PROPERTIES & VARIABLES
  //==================================================
  UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Parameters",
            meta = (AllowPrivateAccess = true))
  TArray<TObjectPtr<UTexture2D>> DotTextures;

  UPROPERTY()
  TObjectPtr<UMaterialInstanceDynamic> DotMaterial{nullptr};

  //==================================================
  // FUNCTIONS
  //==================================================
  AAttachment_4x();
  virtual void BeginPlay() override;

  virtual void DoAction() override;

private:
  int ArrayIndex{0};
};
