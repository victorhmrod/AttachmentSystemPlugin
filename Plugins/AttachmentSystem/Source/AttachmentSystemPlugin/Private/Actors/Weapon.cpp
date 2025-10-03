#include "Actors/Weapon.h"
#include "Actors/Attachment.h"
#include "Actors/MagazineAttachment.h"
#include "Actors/BarrelAttachment.h"
#include "Components/WeaponBuilderComponent.h"
#include "Misc/AttachmentSystemTypes.h"
#include "Net/UnrealNetwork.h"

AWeapon::AWeapon() {
  PrimaryActorTick.bCanEverTick = true;

  Root = CreateDefaultSubobject<USceneComponent>(FName(TEXT("Root")));
  SetRootComponent(Root);

  WeaponBuilderComponent = CreateDefaultSubobject<UWeaponBuilderComponent>(
      FName(TEXT("WeaponBuilderComponent")));
  bReplicates = true;

  WeaponBuilderComponent->OnWeaponBuilt.AddDynamic(this,
                                                   &AWeapon::HandleWeaponBuilt);

  BatchSendInterval = 0.15f;
  PendingShots = 0;
}

void AWeapon::BeginPlay() {
  Super::BeginPlay();
  WeaponCurrentState.Durability = 100.f;
}

void AWeapon::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty> &OutLifetimeProps) const {
  Super::GetLifetimeReplicatedProps(OutLifetimeProps);

  DOREPLIFETIME(AWeapon, CurrentReloadStage);
  DOREPLIFETIME(AWeapon, bHasMagazineAttached);
}

void AWeapon::Tick(float DeltaTime) { Super::Tick(DeltaTime); }

void AWeapon::HandleWeaponBuilt(const TArray<AAttachment*>& SpawnedAttachments) {
  WeaponCurrentState.ActiveAttachments.Reset();
  WeaponCurrentState.ActiveAttachmentMeshes.Reset();

  CurrentMagazine = nullptr;
  CurrentBarrel = nullptr;

  for (AAttachment* Attachment : SpawnedAttachments) {
    if (!Attachment) continue;

    WeaponCurrentState.ActiveAttachments.Add(Attachment);

    if (USkeletalMeshComponent* Mesh = Attachment->GetMeshComponent()) {
      WeaponCurrentState.ActiveAttachmentMeshes.Add(Mesh);
    }

    if (AMagazineAttachment* Mag = Cast<AMagazineAttachment>(Attachment)) {
      CurrentMagazine = Mag;
    }

    if (ABarrelAttachment* Barrel = Cast<ABarrelAttachment>(Attachment)) {
      CurrentBarrel = Barrel;
    }
  }

  UE_LOG(LogAttachmentSystem, Log,
         TEXT("Weapon %s registered %d active attachments (%d Meshes)."),
         *GetName(), WeaponCurrentState.ActiveAttachments.Num(),
         WeaponCurrentState.ActiveAttachmentMeshes.Num())
}

/* =============================
 * Durability
 * ============================= */

float AWeapon::GetWeaponDurability(EWeaponDurabilityMode Mode) {
  if (Mode == EWeaponDurabilityMode::Average &&
      WeaponInfo.DefaultDurabilityMode != EWeaponDurabilityMode::Average) {
    Mode = WeaponInfo.DefaultDurabilityMode;
  }

  if (!IsValid(WeaponBuilderComponent))
    return 0.f;
  const TArray<AAttachment*>& SpawnedAttachments =
      WeaponBuilderComponent->GetSpawnedAttachments();
  if (SpawnedAttachments.Num() == 0)
    return 0.f;

  float Total = 0.f;
  float MinDurability = FLT_MAX;
  float MaxDurability = -FLT_MAX;
  int32 ValidCount = 0;

  for (const AAttachment* Attachment : SpawnedAttachments) {
    if (!Attachment) continue;
    const float CurrentDurability = Attachment->GetDurability();
    Total += CurrentDurability;
    MinDurability = FMath::Min(MinDurability, CurrentDurability);
    MaxDurability = FMath::Max(MaxDurability, CurrentDurability);
    ++ValidCount;
  }

  if (ValidCount == 0)
    return 0.f;

  switch (Mode) {
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

  return WeaponCurrentState.Durability;
}

void AWeapon::ModifyDurability(float Delta) {
  WeaponCurrentState.Durability =
      FMath::Clamp(WeaponCurrentState.Durability + Delta, 0.f, 100.f);
}

/* =============================
 * Batch Firing
 * ============================= */

void AWeapon::StartFiring() {
  PendingShots = 0;
  if (!GetWorldTimerManager().IsTimerActive(BatchTimerHandle)) {
    GetWorldTimerManager().SetTimer(BatchTimerHandle, this,
                                    &AWeapon::ProcessBatch,
                                    BatchSendInterval, true);
    UE_LOG(LogAttachmentSystem, Log, TEXT("Batch firing started for %s"), *GetName());
  }
}

void AWeapon::StopFiring() {
  ProcessBatch();
  GetWorldTimerManager().ClearTimer(BatchTimerHandle);
  UE_LOG(LogAttachmentSystem, Log, TEXT("Batch firing stopped for %s"), *GetName());
}

void AWeapon::OnShotFired() {
  PendingShots++;
}

void AWeapon::ProcessBatch() {
  if (PendingShots > 0 && CurrentMagazine) {
    CurrentMagazine->RemoveBullets(PendingShots);
    UE_LOG(LogAttachmentSystem, Log, TEXT("Processed batch of %d shots on %s"),
           PendingShots, *GetName());
    PendingShots = 0;
  }
}

/* =============================
 * Run Weapon Test (with batches)
 * ============================= */

void AWeapon::RunWeaponTest() {
  UE_LOG(LogAttachmentSystem, Warning, TEXT("==== WEAPON TEST START: %s ===="), *GetName());

  // Reset state
  if (CurrentMagazine)
    CurrentMagazine->RemoveBullets(CurrentMagazine->GetAmmoCount());
  if (CurrentBarrel)
    CurrentBarrel->ClearChamber();
  bHasMagazineAttached = false;
  CurrentReloadStage = EReloadStage::RemoveMagazine;

  // 1. SHOTGUN TEST
  UE_LOG(LogAttachmentSystem, Warning, TEXT("[SHOTGUN TEST]"));
  if (CurrentBarrel) {
    TArray<EBulletType> Pellets;
    for (int32 i = 0; i < 5; ++i)
      Pellets.Add(EBulletType::Standard_FMJ);
    CurrentBarrel->SetChamberedRounds(Pellets);
    UE_LOG(LogAttachmentSystem, Warning, TEXT("Chambered shotgun shell with %d pellets"), Pellets.Num());

    if (HasRoundChambered()) {
      TArray<EBulletType> Fired = FireWeapon();
      UE_LOG(LogAttachmentSystem, Warning, TEXT("Fired shotgun → %d pellets"), Fired.Num());
      for (int32 i = 0; i < Fired.Num(); i++)
        UE_LOG(LogAttachmentSystem, Warning, TEXT("  Pellet %d → %s"), i, *UEnum::GetValueAsString(Fired[i]));
    }
    UE_LOG(LogAttachmentSystem, Warning, TEXT("After shotgun shot → Chambered? %s"),
           HasRoundChambered() ? TEXT("YES") : TEXT("NO"));
  }

  // 2. SINGLE SHOT TEST
  UE_LOG(LogAttachmentSystem, Warning, TEXT("[SINGLE SHOT TEST]"));
  if (CurrentBarrel) {
    TArray<EBulletType> SingleRound;
    SingleRound.Add(EBulletType::ArmorPiercing);
    CurrentBarrel->SetChamberedRounds(SingleRound);
    UE_LOG(LogAttachmentSystem, Warning, TEXT("Chambered single round: %s"),
           *UEnum::GetValueAsString(EBulletType::ArmorPiercing));

    if (HasRoundChambered()) {
      TArray<EBulletType> Fired = FireWeapon();
      UE_LOG(LogAttachmentSystem, Warning, TEXT("Fired single round → %d bullet(s)"), Fired.Num());
      for (int32 i = 0; i < Fired.Num(); i++)
        UE_LOG(LogAttachmentSystem, Warning, TEXT("  Bullet %d → %s"), i, *UEnum::GetValueAsString(Fired[i]));
    }
    UE_LOG(LogAttachmentSystem, Warning, TEXT("After single shot → Chambered? %s"),
           HasRoundChambered() ? TEXT("YES") : TEXT("NO"));
  }

  // 3. BURST TEST
  UE_LOG(LogAttachmentSystem, Warning, TEXT("[BURST TEST]"));
  if (CurrentBarrel) {
    CurrentMagazine->RemoveBullets(CurrentMagazine->GetAmmoCount());
    for (int32 i = 0; i < 3; ++i)
      CurrentMagazine->AddBullet(EBulletType::Tracer);

    for (int32 burst = 0; burst < 3; ++burst) {
      if (!HasRoundChambered())
        TryChamberFromMagazine();
      if (HasRoundChambered()) {
        TArray<EBulletType> Fired = FireWeapon();
        UE_LOG(LogAttachmentSystem, Warning, TEXT("Burst shot %d → %d bullet(s)"), burst + 1, Fired.Num());
        for (int32 i = 0; i < Fired.Num(); i++)
          UE_LOG(LogAttachmentSystem, Warning, TEXT("  Bullet %d → %s"), i, *UEnum::GetValueAsString(Fired[i]));
      }
    }
    UE_LOG(LogAttachmentSystem, Warning, TEXT("After burst → Chambered? %s | MagCount: %d"),
           HasRoundChambered() ? TEXT("YES") : TEXT("NO"),
           CurrentMagazine ? CurrentMagazine->GetAmmoCount() : -1);
  }

  // 4. FULL AUTO TEST (batch firing)
  UE_LOG(LogAttachmentSystem, Warning, TEXT("[FULL AUTO TEST - BATCH]"));
  if (CurrentBarrel) {
    CurrentMagazine->RemoveBullets(CurrentMagazine->GetAmmoCount());
    for (int32 i = 0; i < 10; ++i)
      CurrentMagazine->AddBullet(EBulletType::HollowPoint_SP);

    StartFiring();

    // Simula 10 disparos → processados em lotes de 3 + 3 + 3 + 1
    for (int32 i = 0; i < 10; ++i) {
      OnShotFired();

      // força flush a cada 3 disparos
      if ((i + 1) % 3 == 0) {
        ProcessBatch();
      }
    }

    // garante envio do que sobrou (último 1 tiro)
    StopFiring();

    UE_LOG(LogAttachmentSystem, Warning,
           TEXT("After full-auto batch test → Chambered? %s | MagCount: %d"),
           HasRoundChambered() ? TEXT("YES") : TEXT("NO"),
           CurrentMagazine ? CurrentMagazine->GetAmmoCount() : -1);
  }

  // 5. EMPTY CHAMBER TEST
  UE_LOG(LogAttachmentSystem, Warning, TEXT("[EMPTY CHAMBER TEST]"));
  if (CurrentBarrel) {
    CurrentBarrel->ClearChamber();
    UE_LOG(LogAttachmentSystem, Warning, TEXT("Cleared chamber, should be empty now."));
    if (!HasRoundChambered()) {
      TArray<EBulletType> Fired = FireWeapon();
      UE_LOG(LogAttachmentSystem, Warning, TEXT("Fired with EMPTY chamber → %d pellets"), Fired.Num());
      if (Fired.Num() == 0)
        UE_LOG(LogAttachmentSystem, Warning, TEXT("No pellets fired, correct behavior."));
    }
    UE_LOG(LogAttachmentSystem, Warning, TEXT("After empty shot → Chambered? %s"),
           HasRoundChambered() ? TEXT("YES") : TEXT("NO"));
  }

  UE_LOG(LogAttachmentSystem, Warning, TEXT("==== WEAPON TEST END: %s ===="), *GetName());
}

/* =============================
 * Fire Logic
 * ============================= */

TArray<EBulletType> AWeapon::FireWeapon() {
  if (!HasAuthority())
    return {};
  if (!CurrentBarrel)
    return {};
  if (CurrentBarrel->HasRoundChambered())
    return FireFromChamber();
  if (TryChamberFromMagazine())
    return FireFromChamber();
  return {};
}

TArray<EBulletType> AWeapon::FireFromChamber() {
  if (!HasAuthority())
    return {};
  if (!CurrentBarrel || !CurrentBarrel->HasRoundChambered())
    return {};
  TArray<EBulletType> FiredRounds = CurrentBarrel->GetChamberedRounds();
  CurrentBarrel->ClearChamber();
  TryChamberFromMagazine();
  ModifyDurability(-0.5f);
  OnWeaponFired.Broadcast(FiredRounds);
  UE_LOG(LogAttachmentSystem, Log, TEXT("Weapon %s fired %d rounds"),
         *GetName(), FiredRounds.Num());
  return FiredRounds;
}

bool AWeapon::TryChamberFromMagazine() {
  if (!HasAuthority())
    return false;
  if (!CurrentBarrel || !CurrentMagazine)
    return false;
  if (CurrentBarrel->HasRoundChambered())
    return true;
  EBulletType NextRound = CurrentMagazine->RemoveBullet();
  if (NextRound != EBulletType::None) {
    TArray<EBulletType> Rounds;
    Rounds.Add(NextRound);
    CurrentBarrel->SetChamberedRounds(Rounds);
    UE_LOG(LogAttachmentSystem, Log, TEXT("Weapon %s chambered round %s"),
           *GetName(), *UEnum::GetValueAsString(NextRound));
    return true;
  }
  return false;
}

/* =============================
 * Reload
 * ============================= */

void AWeapon::ReloadWeapon(AMagazineAttachment* NewMag) {
  if (!HasAuthority())
    return;
  ReloadMagazine(NewMag);
  if (!CurrentBarrel)
    return;
  if (!CurrentBarrel->HasRoundChambered())
    TryChamberFromMagazine();
}

void AWeapon::ReloadMagazine(AMagazineAttachment* NewMag) {
  if (!HasAuthority())
    return;
  CurrentMagazine = NewMag;
  bHasMagazineAttached = (NewMag != nullptr);
  UE_LOG(LogAttachmentSystem, Log,
         TEXT("Weapon %s reload magazine: %s"),
         *GetName(), NewMag ? *NewMag->GetName() : TEXT("None"));
}

void AWeapon::BeginStagedReload() {
  if (!HasAuthority())
    return;
  bHasMagazineAttached = (CurrentMagazine != nullptr);
  if (HasRoundChambered() && !bHasMagazineAttached)
    CurrentReloadStage = EReloadStage::InsertMagazine;
  else if (HasRoundChambered() && bHasMagazineAttached)
    CurrentReloadStage = EReloadStage::RemoveMagazine;
  else if (!HasRoundChambered() && !bHasMagazineAttached)
    CurrentReloadStage = EReloadStage::InsertMagazine;
  else if (!HasRoundChambered() && bHasMagazineAttached)
    CurrentReloadStage = EReloadStage::RemoveMagazine;
  UE_LOG(LogAttachmentSystem, Log,
         TEXT("Weapon %s staged reload start, stage: %d"),
         *GetName(), (int32)CurrentReloadStage);
}

void AWeapon::ProcessReloadStage(EReloadStage Stage,
                                 AMagazineAttachment* NewMag) {
  if (!HasAuthority())
    return;
  if (!CurrentBarrel && Stage == EReloadStage::RackHandle)
    return;
  switch (Stage) {
  case EReloadStage::RemoveMagazine:
    bHasMagazineAttached = false;
    CurrentMagazine = nullptr;
    CurrentReloadStage = EReloadStage::InsertMagazine;
    break;
  case EReloadStage::InsertMagazine:
    if (NewMag) {
      ReloadMagazine(NewMag);
      bHasMagazineAttached = true;
    }
    CurrentReloadStage =
        HasRoundChambered() ? EReloadStage::None : EReloadStage::RackHandle;
    break;
  case EReloadStage::RackHandle:
    TryChamberFromMagazine();
    CurrentReloadStage = EReloadStage::None;
    break;
  default:
    CurrentReloadStage = EReloadStage::None;
    break;
  }
  UE_LOG(LogAttachmentSystem, Log,
         TEXT("Weapon %s reload stage processed: %d"),
         *GetName(), (int32)CurrentReloadStage);
}

void AWeapon::CancelReload() {
  if (!HasAuthority())
    return;
  CurrentReloadStage = EReloadStage::None;
  UE_LOG(LogAttachmentSystem, Log, TEXT("Weapon %s reload cancelled"),
         *GetName());
}

/* =============================
 * Helpers
 * ============================= */

int32 AWeapon::GetAmmoCount() const {
  return (CurrentMagazine ? CurrentMagazine->GetAmmoCount() : 0);
}

EBulletType AWeapon::GetChamberedRound() const {
  if (CurrentBarrel && CurrentBarrel->HasRoundChambered()) {
    TArray<EBulletType> Rounds = CurrentBarrel->GetChamberedRounds();
    return Rounds.Num() > 0 ? Rounds[0] : EBulletType::None;
  }
  return EBulletType::None;
}

bool AWeapon::HasRoundChambered() const {
  return (CurrentBarrel && CurrentBarrel->HasRoundChambered());
}