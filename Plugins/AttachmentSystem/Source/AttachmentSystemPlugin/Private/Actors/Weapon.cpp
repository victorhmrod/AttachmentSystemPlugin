#include "Actors/Weapon.h"

#include "Components/WeaponBuilderComponent.h"

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = true;
	
	Root = CreateDefaultSubobject<USceneComponent>(FName{TEXTVIEW("Root")});
	
	SetRootComponent(Root);

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

