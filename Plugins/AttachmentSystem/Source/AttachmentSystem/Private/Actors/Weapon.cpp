#include "Actors/Weapon.h"

#include "Components/WeaponBuilderComponent.h"

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = true;

	WeaponBuilderComponent = CreateDefaultSubobject<UWeaponBuilderComponent>(FName{TEXTVIEW("WeaponBuilderComponent")});
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();
	
}

void AWeapon::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);
}

