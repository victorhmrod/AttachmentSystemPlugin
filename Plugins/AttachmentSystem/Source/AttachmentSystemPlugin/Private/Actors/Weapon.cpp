#include "Actors/Weapon.h"

#include "Actors/Attachment.h"
#include "Actors/MagazineAttachment.h"
#include "Components/WeaponBuilderComponent.h"
#include "Misc/AttachmentSystemTypes.h"

/* =============================
 * Lifecycle
 * ============================= */

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = true;
	
	// Root scene component to serve as the base transform
	Root = CreateDefaultSubobject<USceneComponent>(FName(TEXTVIEW("Root")));
	SetRootComponent(Root);

	// Component that manages attachments (spawn/assembly)
	WeaponBuilderComponent = CreateDefaultSubobject<UWeaponBuilderComponent>(FName(TEXTVIEW("WeaponBuilderComponent")));

	// Enable network replication
	bReplicates = true;

	// Subscribe to weapon builder events
	WeaponBuilderComponent->OnWeaponBuilt.AddDynamic(this, &AWeapon::HandleWeaponBuilt);
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	// Initialize runtime durability
	WeaponCurrentState.Durability = 100.f;

	for (int i = 0; i < 5; i++)
	{
		(void)FireWeapon();
	}
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// Optional: update durability in real-time if attachments degrade dynamically
}


/* =============================
 * Weapon Builder
 * ============================= */

void AWeapon::HandleWeaponBuilt(const TArray<AAttachment*>& SpawnedAttachments)
{
	// Reset runtime arrays to avoid duplicates from rebuilds
	WeaponCurrentState.ActiveAttachments.Reset();
	WeaponCurrentState.ActiveAttachmentMeshes.Reset();

	// Save references into runtime state
	for (AAttachment* Attachment : SpawnedAttachments)
	{
		if (!Attachment) continue;

		WeaponCurrentState.ActiveAttachments.Add(Attachment);

		if (USkeletalMeshComponent* Mesh = Attachment->GetMeshComponent())
		{
			WeaponCurrentState.ActiveAttachmentMeshes.Add(Mesh);
		}

		// Detect if attachment is a magazine
		if (AMagazineAttachment* Mag = Cast<AMagazineAttachment>(Attachment))
		{
			CurrentMagazine = Mag;
			UE_LOG(LogAttachmentSystem, Log, TEXT("Weapon %s found magazine: %s"),
				   *GetName(), *GetNameSafe(Mag));
		}
	}

	UE_LOG(LogAttachmentSystem, Log, TEXT("Weapon %s registered %d active attachments (%d meshes)."),
		*GetName(),
		WeaponCurrentState.ActiveAttachments.Num(),
		WeaponCurrentState.ActiveAttachmentMeshes.Num());
}


/* =============================
 * Weapon Stats
 * ============================= */

float AWeapon::GetWeaponDurability(EWeaponDurabilityMode Mode)
{
	// If not explicitly provided, use the weapon's default config
	if (Mode == EWeaponDurabilityMode::Average && WeaponInfo.DefaultDurabilityMode != EWeaponDurabilityMode::Average)
	{
		Mode = WeaponInfo.DefaultDurabilityMode;
	}

	// No builder component → durability cannot be calculated
	if (!IsValid(WeaponBuilderComponent)) return 0.f;

	// Get all currently spawned attachments
	const TArray<AAttachment*>& SpawnedAttachments = WeaponBuilderComponent->GetSpawnedAttachments();
	if (SpawnedAttachments.Num() == 0) return 0.f;

	// Local variables for calculation
	float Total = 0.f;
	float MinDurability = FLT_MAX;
	float MaxDurability = -FLT_MAX;
	int32 ValidCount = 0;

	// Traverse all attachments and gather durability data
	for (const AAttachment* Attachment : SpawnedAttachments)
	{
		if (!Attachment) continue;

		const float CurrentDurability = Attachment->GetDurability();

		Total += CurrentDurability;
		MinDurability = FMath::Min(MinDurability, CurrentDurability);
		MaxDurability = FMath::Max(MaxDurability, CurrentDurability);

		++ValidCount;
	}

	// Safety check: no valid attachments
	if (ValidCount == 0) return 0.f;

	// Select calculation mode
	switch (Mode)
	{
	case EWeaponDurabilityMode::Average:
		WeaponCurrentState.Durability = Total / ValidCount;
		break;
	case EWeaponDurabilityMode::Minimum:
		WeaponCurrentState.Durability = MinDurability;
		break;
	case EWeaponDurabilityMode::Maximum:
		WeaponCurrentState.Durability = MaxDurability;
		break;
	}

	// Return the final durability value
	return WeaponCurrentState.Durability;
}

EBulletType AWeapon::FireWeapon()
{
	// Ensure magazine exists
	if (!CurrentMagazine)
	{
		UE_LOG(LogAttachmentSystem, Warning, TEXT("Weapon %s has NO magazine! Cannot fire."), *GetName());
		return EBulletType::None;
	}

	// Ensure ammo available
	if (!HasAmmo())
	{
		UE_LOG(LogAttachmentSystem, Warning, TEXT("Weapon %s magazine is EMPTY! Cannot fire."), *GetName());
		return EBulletType::None;
	}

	// Consume one bullet
	const EBulletType FiredBullet = ConsumeBullet();

	// andle successful fire
	if (IsValidBullet(FiredBullet))
	{
		UE_LOG(LogAttachmentSystem, Log, TEXT("Weapon %s successfully FIRED! Bullet: %s | Remaining: %d"),
			   *GetName(),
			   *AMagazineAttachment::BulletTypeToString(FiredBullet),
			   GetAmmoCount());

		// Decrease durability slightly per shot
		ModifyDurability(-0.5f);

		// Broadcast delegate for external systems (UI, FX, recoil, etc.)
		OnWeaponFired.Broadcast();

		return FiredBullet;
	}

	// Fallback (invalid bullet consumed)
	UE_LOG(LogAttachmentSystem, Warning, TEXT("Weapon %s failed to fire (invalid bullet)."), *GetName());
	return EBulletType::None;
}

	/* =============================
	* Weapon Stats
	* ============================= */

void AWeapon::ModifyDurability(float Delta)
{
	// Clamp durability in [0..100] range
	WeaponCurrentState.Durability = FMath::Clamp(
		WeaponCurrentState.Durability + Delta,
		0.f,
		100.f
	);

	UE_LOG(LogAttachmentSystem, Log, TEXT("Weapon %s durability changed by %.2f → Current: %.2f"),
		   *GetName(), Delta, WeaponCurrentState.Durability);
}

/* =============================
 * Ammo / Magazine
 * ============================= */

int32 AWeapon::GetAmmoCount() const
{
	return (CurrentMagazine ? CurrentMagazine->GetAmmoCount() : 0);
}

bool AWeapon::HasAmmo() const
{
	return GetAmmoCount() > 0;
}

EBulletType AWeapon::ConsumeBullet() const
{
	if (CurrentMagazine)
	{
		EBulletType Type = CurrentMagazine->RemoveBullet();
		if (Type == EBulletType::None)
		{
			UE_LOG(LogAttachmentSystem, Warning, TEXT("Weapon %s tried to fire but magazine is EMPTY!"), *GetName());
		}
		else
		{
			UE_LOG(LogAttachmentSystem, Warning, TEXT("Weapon %s fired bullet: %s"),
				   *GetName(),
				   *AMagazineAttachment::BulletTypeToString(Type));
		}
		return Type;
	}

	UE_LOG(LogAttachmentSystem, Warning, TEXT("Weapon %s has NO magazine!"), *GetName());
	return EBulletType::None;
}

void AWeapon::ReloadMagazine(AMagazineAttachment* NewMag)
{
	CurrentMagazine = NewMag;

	if (NewMag)
	{
		UE_LOG(LogAttachmentSystem, Warning, TEXT("Weapon %s reloaded with magazine: %s (%d bullets)"),
			   *GetName(),
			   *GetNameSafe(NewMag),
			   NewMag->GetAmmoCount());
	}
	else
	{
		UE_LOG(LogAttachmentSystem, Warning, TEXT("Weapon %s magazine removed!"), *GetName());
	}
}

bool AWeapon::IsValidBullet(EBulletType BulletType)
{
	return BulletType != EBulletType::None;
}