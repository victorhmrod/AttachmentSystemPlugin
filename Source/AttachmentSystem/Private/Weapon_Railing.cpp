// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon_Railing.h"

// Sets default values
AWeapon_Railing::AWeapon_Railing() {
  RailMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RailMesh"));
  RootComponent = RailMesh;

  StartPoint = CreateDefaultSubobject<USceneComponent>(TEXT("StartPoint"));
  StartPoint->SetupAttachment(RootComponent);

  EndPoint = CreateDefaultSubobject<USceneComponent>(TEXT("EndPoint"));
  EndPoint->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void AWeapon_Railing::BeginPlay() { Super::BeginPlay(); }
