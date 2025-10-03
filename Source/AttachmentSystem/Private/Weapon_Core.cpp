// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon_Core.h"

// Sets default values
AWeapon_Core::AWeapon_Core() {
  WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
  RootComponent = WeaponMesh;
}
