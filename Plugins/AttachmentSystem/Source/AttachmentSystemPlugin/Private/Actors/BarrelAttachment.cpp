#include "Actors/BarrelAttachment.h"
#include "Net/UnrealNetwork.h"

ABarrelAttachment::ABarrelAttachment() {
  PrimaryActorTick.bCanEverTick = false;
  bReplicates = true;
  UE_LOG(LogTemp, Warning, TEXT("ABarrelAttachment initialized"));
}

void ABarrelAttachment::SetChamberedRounds(const TArray<EBulletType>& NewRounds) {
  if (!HasAuthority()) {
    Server_SetChamberedRounds(NewRounds);
    return;
  }

  if (NewRounds.Num() > 0) {
    ChamberedRounds = NewRounds;
    UE_LOG(LogTemp, Warning, TEXT("SetChamberedRounds: %d rounds chambered"), NewRounds.Num());
  } else {
    ChamberedRounds.Empty();
    UE_LOG(LogTemp, Warning, TEXT("SetChamberedRounds: chamber cleared"));
  }
}

void ABarrelAttachment::Server_SetChamberedRounds_Implementation(const TArray<EBulletType>& NewRounds) {
  if (!HasAuthority()) return;
  SetChamberedRounds(NewRounds);
}

void ABarrelAttachment::ClearChamber() {
  if (!HasAuthority()) {
    Server_ClearChamber();
    return;
  }

  ChamberedRounds.Empty();
  UE_LOG(LogTemp, Warning, TEXT("ClearChamber called"));
}

void ABarrelAttachment::Server_ClearChamber_Implementation() {
  if (!HasAuthority()) return;
  ClearChamber();
}

void ABarrelAttachment::OnRep_ChamberedRounds() {
  if (ChamberedRounds.Num() == 0) {
    UE_LOG(LogTemp, Warning, TEXT("OnRep_ChamberedRounds: chamber is empty"));
  } else {
    UE_LOG(LogTemp, Warning, TEXT("OnRep_ChamberedRounds: %d rounds replicated"), ChamberedRounds.Num());
  }
}

void ABarrelAttachment::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
  Super::GetLifetimeReplicatedProps(OutLifetimeProps);

  DOREPLIFETIME(ABarrelAttachment, ChamberedRounds);
}