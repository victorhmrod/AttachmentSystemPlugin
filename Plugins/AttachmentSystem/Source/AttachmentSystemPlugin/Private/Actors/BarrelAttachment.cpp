#include "Actors/BarrelAttachment.h"
#include "Net/UnrealNetwork.h"

ABarrelAttachment::ABarrelAttachment() {
  PrimaryActorTick.bCanEverTick = false;
  bReplicates = true;
}

void ABarrelAttachment::SetChamberedRounds(
    const TArray<EBulletType> &NewRounds) {
  if (!HasAuthority()) {
    Server_SetChamberedRounds(NewRounds);
    return;
  }

  if (NewRounds.Num() > 0) {
    ChamberedRounds = NewRounds;
    ReplicatedChamberedRounds = NewRounds;
  } else {
    ChamberedRounds.reset();
    ReplicatedChamberedRounds.Empty();
  }
}

void ABarrelAttachment::Server_SetChamberedRounds_Implementation(
    const TArray<EBulletType> &NewRounds) {
  if (!HasAuthority())
    return;
  SetChamberedRounds(NewRounds);
}

void ABarrelAttachment::ClearChamber() {
  if (!HasAuthority()) {
    Server_ClearChamber();
    return;
  }

  ChamberedRounds.reset();
  ReplicatedChamberedRounds.Empty();
}

void ABarrelAttachment::Server_ClearChamber_Implementation() {
  if (!HasAuthority())
    return;
  ClearChamber();
}

void ABarrelAttachment::OnRep_ChamberedRounds() {
  if (ReplicatedChamberedRounds.Num() == 0) {
    ChamberedRounds.reset();
  } else {
    ChamberedRounds = ReplicatedChamberedRounds;
  }
}

void ABarrelAttachment::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty> &OutLifetimeProps) const {
  Super::GetLifetimeReplicatedProps(OutLifetimeProps);

  DOREPLIFETIME(ABarrelAttachment, ReplicatedChamberedRounds);
}
