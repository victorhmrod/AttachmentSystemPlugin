// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Attachment_Base.h"
#include "Weapon_Railing.h"
#include "Attachment_45Degree.generated.h"

/**
 *
 */
UCLASS()
class ATTACHMENTSYSTEM_API AAttachment_45Degree : public AAttachment_Base {
  GENERATED_BODY()

public:
  //==================================================
  // PROPERTIES & VARIABLES
  //==================================================
  UPROPERTY(EditAnywhere, Category = "Mesh")
  TObjectPtr<UChildActorComponent> Railing{nullptr};

  //==================================================
  // FUNCTIONS
  //==================================================

protected:
  AAttachment_45Degree();

  virtual void BeginPlay() override;

  virtual void OnPlacedEvent() override;
};
