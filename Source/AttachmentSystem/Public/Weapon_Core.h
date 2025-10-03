// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon_Core.generated.h"

UCLASS()
class ATTACHMENTSYSTEM_API AWeapon_Core : public AActor
{
	GENERATED_BODY()
	
public:	

	AWeapon_Core();

	UPROPERTY(EditAnywhere, Category = "Mesh", meta = (AllowPrivateAccess = true))
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

protected:

public:	


};
