#include "Actors/Weapon.h"
#include "Actors/Attachment.h"
#include "Actors/MagazineAttachment.h"
#include "Actors/BarrelAttachment.h"
#include "Components/WeaponBuilderComponent.h"
#include "Misc/AttachmentSystemTypes.h"
#include "Net/UnrealNetwork.h"

AWeapon::AWeapon()
{
    PrimaryActorTick.bCanEverTick = true;

    Root = CreateDefaultSubobject<USceneComponent>(FName(TEXT("Root")));
    SetRootComponent(Root);

    WeaponBuilderComponent = CreateDefaultSubobject<UWeaponBuilderComponent>(FName(TEXT("WeaponBuilderComponent")));
    bReplicates = true;

    WeaponBuilderComponent->OnWeaponBuilt.AddDynamic(this, &AWeapon::HandleWeaponBuilt);
}

void AWeapon::BeginPlay()
{
    Super::BeginPlay();
    WeaponCurrentState.Durability = 100.f;
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AWeapon, CurrentReloadStage);
    DOREPLIFETIME(AWeapon, bHasMagazineAttached);
}

void AWeapon::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AWeapon::HandleWeaponBuilt(const TArray<AAttachment*>& SpawnedAttachments)
{
    // Reset runtime arrays to avoid duplicates from rebuilds
    WeaponCurrentState.ActiveAttachments.Reset();
    WeaponCurrentState.ActiveAttachmentMeshes.Reset();

    // Reset runtime references
    CurrentMagazine = nullptr;
    CurrentBarrel = nullptr;

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
        }

        // Detect if attachment is a barrel
        if (ABarrelAttachment* Barrel = Cast<ABarrelAttachment>(Attachment))
        {
            CurrentBarrel = Barrel;
        }
    }

    UE_LOG(LogAttachmentSystem, Log, TEXT("Weapon %s registered %d active attachments (%d Meshes)."), *GetName(), WeaponCurrentState.ActiveAttachments.Num(), WeaponCurrentState.ActiveAttachmentMeshes.Num())
}

/* =============================
 * Weapon Stats
 * ============================= */

float AWeapon::GetWeaponDurability(EWeaponDurabilityMode Mode)
{
    if (Mode == EWeaponDurabilityMode::Average &&
        WeaponInfo.DefaultDurabilityMode != EWeaponDurabilityMode::Average)
    {
        Mode = WeaponInfo.DefaultDurabilityMode;
    }

    if (!IsValid(WeaponBuilderComponent)) return 0.f;
    const TArray<AAttachment*>& SpawnedAttachments = WeaponBuilderComponent->GetSpawnedAttachments();
    if (SpawnedAttachments.Num() == 0) return 0.f;

    float Total = 0.f;
    float MinDurability = FLT_MAX;
    float MaxDurability = -FLT_MAX;
    int32 ValidCount = 0;

    for (const AAttachment* Attachment : SpawnedAttachments)
    {
        if (!Attachment) continue;
        const float CurrentDurability = Attachment->GetDurability();
        Total += CurrentDurability;
        MinDurability = FMath::Min(MinDurability, CurrentDurability);
        MaxDurability = FMath::Max(MaxDurability, CurrentDurability);
        ++ValidCount;
    }

    if (ValidCount == 0) return 0.f;

    switch (Mode)
    {
    case EWeaponDurabilityMode::Average:
        WeaponCurrentState.Durability = Total / ValidCount; break;
    case EWeaponDurabilityMode::Minimum:
        WeaponCurrentState.Durability = MinDurability; break;
    case EWeaponDurabilityMode::Maximum:
        WeaponCurrentState.Durability = MaxDurability; break;
    }

    return WeaponCurrentState.Durability;
}

void AWeapon::ModifyDurability(float Delta)
{
    WeaponCurrentState.Durability = FMath::Clamp(
        WeaponCurrentState.Durability + Delta, 0.f, 100.f
    );
}

void AWeapon::RunWeaponTest()
{
    UE_LOG(LogAttachmentSystem, Warning, TEXT("==== WEAPON TEST START: %s ===="), *GetName());

    AMagazineAttachment* DummyMag = nullptr;

    // Reset weapon state
    if (CurrentMagazine)
    {
        CurrentMagazine->Empty();
    }
    else
    {
        DummyMag = GetWorld()->SpawnActor<AMagazineAttachment>();
        if (!DummyMag)
        {
            UE_LOG(LogAttachmentSystem, Error, TEXT("%s → Failed to spawn DummyMag!"), *GetName());
            return;
        }
    }

    if (CurrentBarrel)
    {
        CurrentBarrel->ClearChamber();
    }

    bHasMagazineAttached = false;
    CurrentReloadStage   = EReloadStage::RemoveMagazine;

    // Fill dummy mag with bullets
    for (int32 i = 0; i < 35; i++)
    {
        DummyMag->AddBullet(EBulletType::Standard_FMJ);
    }

    UE_LOG(LogAttachmentSystem, Warning, TEXT("Initial MagCount=%d | Chamber=%s"),
        DummyMag->GetAmmoCount(),
        HasRoundChambered() ? *UEnum::GetValueAsString(GetChamberedRound()) : TEXT("EMPTY"));

    UE_LOG(LogAttachmentSystem, Warning, TEXT("Initial Reload Stage = %d"), (uint8)CurrentReloadStage);

    // Remove magazine
    ProcessReloadStage(EReloadStage::RemoveMagazine);
    UE_LOG(LogAttachmentSystem, Warning, TEXT("After RemoveMag → Stage=%d | MagAttached=%s"),
        (uint8)CurrentReloadStage,
        bHasMagazineAttached ? TEXT("true") : TEXT("false"));

    // Insert magazine
    ProcessReloadStage(EReloadStage::InsertMagazine, DummyMag);
    UE_LOG(LogAttachmentSystem, Warning, TEXT("After InsertMag → Stage=%d | MagAttached=%s | MagCount=%d"),
        (uint8)CurrentReloadStage,
        bHasMagazineAttached ? TEXT("true") : TEXT("false"),
        CurrentMagazine ? CurrentMagazine->GetAmmoCount() : -1);

    // Rack handle
    ProcessReloadStage(EReloadStage::RackHandle);
    UE_LOG(LogAttachmentSystem, Warning, TEXT("After RackHandle → Stage=%d | Chamber=%s"),
        (uint8)CurrentReloadStage,
        HasRoundChambered() ? *UEnum::GetValueAsString(GetChamberedRound()) : TEXT("EMPTY"));

    // Fire 5 shots
    for (int32 i = 1; i <= 5; i++)
    {
        EBulletType Shot = FireWeapon();
        UE_LOG(LogAttachmentSystem, Warning, TEXT("Shot %d → %s"), i, *UEnum::GetValueAsString(Shot));
    }

    // Reload
    ReloadWeapon(DummyMag);
    UE_LOG(LogAttachmentSystem, Warning, TEXT("After ReloadWeapon → Chamber=%s | MagCount=%d"),
        HasRoundChambered() ? *UEnum::GetValueAsString(GetChamberedRound()) : TEXT("EMPTY"),
        CurrentMagazine ? CurrentMagazine->GetAmmoCount() : -1);

    // Begin staged reload and cancel
    BeginStagedReload();
    UE_LOG(LogAttachmentSystem, Warning, TEXT("Before Cancel → Stage=%d"), (uint8)CurrentReloadStage);
    CancelReload();
    UE_LOG(LogAttachmentSystem, Warning, TEXT("After Cancel → Stage=%d | Chamber=%s | MagCount=%d"),
        (uint8)CurrentReloadStage,
        HasRoundChambered() ? *UEnum::GetValueAsString(GetChamberedRound()) : TEXT("EMPTY"),
        CurrentMagazine ? CurrentMagazine->GetAmmoCount() : -1);

    // Keep firing until empty
    int32 ExtraShot = 1;
    while (HasRoundChambered() || (CurrentMagazine && !CurrentMagazine->IsEmpty()))
    {
        EBulletType Shot = FireWeapon();
        UE_LOG(LogAttachmentSystem, Warning, TEXT("Extra Shot %d → %s"),
            ExtraShot++, *UEnum::GetValueAsString(Shot));
    }

    // Dry fire
    EBulletType Dry = FireWeapon();
    UE_LOG(LogAttachmentSystem, Warning, TEXT("Dry Fire → %s"), *UEnum::GetValueAsString(Dry));

    // Final status
    UE_LOG(LogAttachmentSystem, Warning, TEXT("Final Chamber=%s | MagCount=%d"),
        HasRoundChambered() ? *UEnum::GetValueAsString(GetChamberedRound()) : TEXT("EMPTY"),
        CurrentMagazine ? CurrentMagazine->GetAmmoCount() : -1);

    UE_LOG(LogAttachmentSystem, Warning, TEXT("==== WEAPON TEST END: %s ===="), *GetName());
}

/* =============================
 * Fire Logic
 * ============================= */

EBulletType AWeapon::FireWeapon()
{
    if (!CurrentBarrel) return EBulletType::None;

    if (CurrentBarrel->HasRoundChambered())
        return FireFromChamber();

    if (TryChamberFromMagazine())
        return FireFromChamber();

    return EBulletType::None;
}

EBulletType AWeapon::FireFromChamber()
{
    if (!CurrentBarrel || !CurrentBarrel->HasRoundChambered())
        return EBulletType::None;

    EBulletType Fired = CurrentBarrel->GetChamberedRound().value();
    CurrentBarrel->ClearChamber();
    TryChamberFromMagazine();
    ModifyDurability(-0.5f);
    OnWeaponFired.Broadcast();
    return Fired;
}

bool AWeapon::TryChamberFromMagazine()
{
    if (!CurrentBarrel || !CurrentMagazine) return false;
    if (CurrentBarrel->HasRoundChambered()) return true;

    EBulletType NextRound = CurrentMagazine->RemoveBullet();
    if (NextRound != EBulletType::None)
    {
        CurrentBarrel->SetChamberedRound(NextRound);
        return true;
    }
    return false;
}

/* =============================
 * Reload
 * ============================= */
void AWeapon::ReloadWeapon(AMagazineAttachment* NewMag)
{
    ReloadMagazine(NewMag);

    if (!CurrentBarrel) return;

    if (!CurrentBarrel->HasRoundChambered())
        TryChamberFromMagazine();
}

void AWeapon::ReloadMagazine(AMagazineAttachment* NewMag)
{
    CurrentMagazine = NewMag;
    bHasMagazineAttached = (NewMag != nullptr);
}

void AWeapon::BeginStagedReload()
{
    bHasMagazineAttached = (CurrentMagazine != nullptr);

    if (HasRoundChambered() && !bHasMagazineAttached)
        CurrentReloadStage = EReloadStage::InsertMagazine;
    else if (HasRoundChambered() && bHasMagazineAttached)
        CurrentReloadStage = EReloadStage::RemoveMagazine;
    else if (!HasRoundChambered() && !bHasMagazineAttached)
        CurrentReloadStage = EReloadStage::InsertMagazine;
    else if (!HasRoundChambered() && bHasMagazineAttached)
        CurrentReloadStage = EReloadStage::RemoveMagazine;
}

void AWeapon::ProcessReloadStage(EReloadStage Stage, AMagazineAttachment* NewMag)
{
    if (!CurrentBarrel && Stage == EReloadStage::RackHandle)
        return;

    switch (Stage)
    {
    case EReloadStage::RemoveMagazine:
        bHasMagazineAttached = false;
        CurrentMagazine = nullptr;
        CurrentReloadStage = EReloadStage::InsertMagazine;
        UE_LOG(LogAttachmentSystem, Log, TEXT("%s → Stage: Remove Magazine"), *GetName());
        break;

    case EReloadStage::InsertMagazine:
        if (NewMag)
        {
            ReloadMagazine(NewMag);
            bHasMagazineAttached = true;
            UE_LOG(LogAttachmentSystem, Log, TEXT("%s → Stage: Insert Magazine (%d rounds)"),
                   *GetName(), NewMag->GetAmmoCount());
        }
        else
        {
            UE_LOG(LogAttachmentSystem, Warning, TEXT("%s tried to insert magazine but none provided"), *GetName());
        }

        CurrentReloadStage = HasRoundChambered() ? EReloadStage::None : EReloadStage::RackHandle;
        break;

    case EReloadStage::RackHandle:
        if (TryChamberFromMagazine())
        {
            UE_LOG(LogAttachmentSystem, Log, TEXT("%s → Stage: Rack Handle (chambered round: %s)"),
                   *GetName(),
                   *UEnum::GetValueAsString(GetChamberedRound()));
        }
        else
        {
            UE_LOG(LogAttachmentSystem, Warning, TEXT("%s → Rack Handle FAILED (mag empty)"), *GetName());
        }
        CurrentReloadStage = EReloadStage::None;
        break;

    default:
        CurrentReloadStage = EReloadStage::None;
        break;
    }
}



void AWeapon::CancelReload()
{
    CurrentReloadStage = EReloadStage::None;
}

/* =============================
 * Helpers
 * ============================= */
int32 AWeapon::GetAmmoCount() const
{
    return (CurrentMagazine ? CurrentMagazine->GetAmmoCount() : 0);
}

EBulletType AWeapon::GetChamberedRound() const
{
    return (CurrentBarrel && CurrentBarrel->HasRoundChambered())
        ? CurrentBarrel->GetChamberedRound().value()
        : EBulletType::None;
}

bool AWeapon::HasRoundChambered() const
{
    return (CurrentBarrel && CurrentBarrel->HasRoundChambered());
}