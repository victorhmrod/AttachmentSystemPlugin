#include "Actors/BarrelAttachment.h"
#include "Net/UnrealNetwork.h"

ABarrelAttachment::ABarrelAttachment()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
}

void ABarrelAttachment::SetChamberedRound(EBulletType NewRound)
{
	ChamberedRound = (NewRound != EBulletType::None) ? std::make_optional(NewRound) : std::nullopt;
	ReplicatedChamberedRound = NewRound;
}

void ABarrelAttachment::ClearChamber()
{
	ChamberedRound.reset();
	ReplicatedChamberedRound = EBulletType::None;
}

void ABarrelAttachment::OnRep_ChamberedRound()
{
	if (ReplicatedChamberedRound == EBulletType::None)
	{
		ChamberedRound.reset();
	}
	else
	{
		ChamberedRound = ReplicatedChamberedRound;
	}
}

void ABarrelAttachment::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABarrelAttachment, ReplicatedChamberedRound);
}

