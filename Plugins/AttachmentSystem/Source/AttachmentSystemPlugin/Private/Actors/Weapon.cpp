#include "Actors/Weapon.h"

#include "Actors/Attachment.h"
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

float AWeapon::GetWeaponDurability() const
{
	if (!IsValid(WeaponBuilderComponent)) return 0.f;

	const TArray<AAttachment*>& SpawnedAttachments = WeaponBuilderComponent->GetSpawnedAttachments();
	if (SpawnedAttachments.Num() == 0)
	{
		return 0.f;
	}

	float TotalDurability = 0.f;
	int32 ValidCount = 0;

	for (const AAttachment* Attachment : SpawnedAttachments)
	{
		if (Attachment)
		{
			TotalDurability += Attachment->GetAttachmentInfo().Durability;
			++ValidCount;
		}
	}

	return ValidCount > 0 ? TotalDurability / ValidCount : 0.f;
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}