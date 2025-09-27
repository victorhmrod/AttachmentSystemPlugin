#include "Actors/Weapon.h"

#include "Actors/Attachment.h"
#include "Components/WeaponBuilderComponent.h"

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = true;
	
	// Root scene component to serve as the base transform
	Root = CreateDefaultSubobject<USceneComponent>(FName{TEXTVIEW("Root")});
	SetRootComponent(Root);

	// Component that manages attachments (spawn/assembly)
	WeaponBuilderComponent = CreateDefaultSubobject<UWeaponBuilderComponent>(FName{TEXTVIEW("WeaponBuilderComponent")});

	// Enable network replication
	bReplicates = true;

	WeaponBuilderComponent->OnWeaponBuilt.AddDynamic(this, &AWeapon::HandleWeaponBuilt);
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	// Initialize runtime durability
	WeaponCurrentState.Durability = 100.f;
}

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
	}

	UE_LOG(LogTemp, Log, TEXT("Weapon %s registered %d active attachments (%d meshes)."),
		*GetName(),
		WeaponCurrentState.ActiveAttachments.Num(),
		WeaponCurrentState.ActiveAttachmentMeshes.Num());
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// Optional: could update durability in real-time if attachments degrade dynamically
}

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