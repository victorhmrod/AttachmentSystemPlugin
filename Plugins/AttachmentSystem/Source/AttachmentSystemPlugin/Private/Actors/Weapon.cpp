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
    WeaponCurrentState.ActiveAttachments.Reset();
    WeaponCurrentState.ActiveAttachmentMeshes.Reset();
    CurrentMagazine = nullptr;
    CurrentBarrel = nullptr;

    for (AAttachment* Attachment : SpawnedAttachments)
    {
        if (!Attachment) continue;
        WeaponCurrentState.ActiveAttachments.Add(Attachment);

        if (USkeletalMeshComponent* Mesh = Attachment->GetMeshComponent())
        {
            WeaponCurrentState.ActiveAttachmentMeshes.Add(Mesh);
        }

        if (AMagazineAttachment* Mag = Cast<AMagazineAttachment>(Attachment))
            CurrentMagazine = Mag;

        if (ABarrelAttachment* Barrel = Cast<ABarrelAttachment>(Attachment))
            CurrentBarrel = Barrel;
    }
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

    // Reset state
    if (CurrentMagazine)
    {
        CurrentMagazine->Empty();
    }
    if (CurrentBarrel)
    {
        CurrentBarrel->ClearChamber();
    }
    bHasMagazineAttached = false;
    CurrentReloadStage = EReloadStage::RemoveMagazine;

    // 1. Criar um mag dummy e encher
    AMagazineAttachment* DummyMag = GetWorld()->SpawnActor<AMagazineAttachment>();
    if (!DummyMag)
    {
        UE_LOG(LogAttachmentSystem, Error, TEXT("%s → Could not spawn DummyMag!"), *GetName());
        return;
    }

    for (int i = 0; i < 35; i++) // tenta passar da capacidade
    {
        DummyMag->AddBullet(EBulletType::Standard_FMJ);
    }

    UE_LOG(LogAttachmentSystem, Warning, TEXT("Initial MagCount=%d | Chamber=%s"),
           DummyMag->GetAmmoCount(),
           HasRoundChambered() ? *UEnum::GetValueAsString(GetChamberedRound()) : TEXT("EMPTY"));
    UE_LOG(LogAttachmentSystem, Warning, TEXT("Initial Reload Stage = %d"), (uint8)CurrentReloadStage);

    // 2. Staged reload
    ProcessReloadStage(EReloadStage::RemoveMagazine);
    UE_LOG(LogAttachmentSystem, Warning, TEXT("After RemoveMag → Stage=%d | MagAttached=%s"),
           (uint8)CurrentReloadStage, bHasMagazineAttached ? TEXT("true") : TEXT("false"));

    ProcessReloadStage(EReloadStage::InsertMagazine, DummyMag);
    UE_LOG(LogAttachmentSystem, Warning, TEXT("After InsertMag → Stage=%d | MagAttached=%s | MagCount=%d"),
           (uint8)CurrentReloadStage,
           bHasMagazineAttached ? TEXT("true") : TEXT("false"),
           CurrentMagazine ? CurrentMagazine->GetAmmoCount() : -1);

    ProcessReloadStage(EReloadStage::RackHandle);
    UE_LOG(LogAttachmentSystem, Warning, TEXT("After RackHandle → Stage=%d | Chamber=%s"),
           (uint8)CurrentReloadStage,
           HasRoundChambered() ? *UEnum::GetValueAsString(GetChamberedRound()) : TEXT("EMPTY"));

    // 3. Disparos de teste (limite 5)
    for (int i = 1; i <= 5; i++)
    {
        EBulletType Shot = FireWeapon();
        UE_LOG(LogAttachmentSystem, Warning, TEXT("Shot %d → %s"), i, *UEnum::GetValueAsString(Shot));
    }

    // 4. Reload direto
    ReloadWeapon(DummyMag);
    UE_LOG(LogAttachmentSystem, Warning, TEXT("After ReloadWeapon → Chamber=%s | MagCount=%d"),
           HasRoundChambered() ? *UEnum::GetValueAsString(GetChamberedRound()) : TEXT("EMPTY"),
           CurrentMagazine ? CurrentMagazine->GetAmmoCount() : -1);

    // 5. Cancel reload test
    BeginStagedReload();
    UE_LOG(LogAttachmentSystem, Warning, TEXT("Before Cancel → Stage=%d"), (uint8)CurrentReloadStage);
    CancelReload();
    UE_LOG(LogAttachmentSystem, Warning, TEXT("After Cancel → Stage=%d | Chamber=%s | MagCount=%d"),
           (uint8)CurrentReloadStage,
           HasRoundChambered() ? *UEnum::GetValueAsString(GetChamberedRound()) : TEXT("EMPTY"),
           CurrentMagazine ? CurrentMagazine->GetAmmoCount() : -1);

    // 6. Loop de disparos até esvaziar o mag
    int32 ExtraShot = 1;
    while (HasRoundChambered() || (CurrentMagazine && !CurrentMagazine->IsEmpty()))
    {
        EBulletType Shot = FireWeapon();
        UE_LOG(LogAttachmentSystem, Warning, TEXT("Extra Shot %d → %s"),
               ExtraShot++, *UEnum::GetValueAsString(Shot));
    }

    // 7. Teste de dry fire
    EBulletType Dry = FireWeapon();
    UE_LOG(LogAttachmentSystem, Warning, TEXT("Dry Fire → %s"), *UEnum::GetValueAsString(Dry));

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