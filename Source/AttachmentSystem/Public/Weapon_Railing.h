// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "Weapon_Railing.generated.h"

UCLASS()
class ATTACHMENTSYSTEM_API AWeapon_Railing : public AActor {
  GENERATED_BODY()

public:
  //==================================================
  // PROPERTIES & VARIABLES
  //==================================================
  UPROPERTY(EditAnywhere, Category = "Mesh", meta = (AllowPrivateAccess = true))
  TObjectPtr<UStaticMeshComponent> RailMesh;

  UPROPERTY(EditAnywhere, Category = "Mesh", meta = (AllowPrivateAccess = true))
  TObjectPtr<USceneComponent> StartPoint;
  UPROPERTY(EditAnywhere, Category = "Mesh", meta = (AllowPrivateAccess = true))
  TObjectPtr<USceneComponent> EndPoint;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters")
  FGameplayTagContainer RailingTags;

protected:
  AWeapon_Railing();

  virtual void BeginPlay() override;
};
