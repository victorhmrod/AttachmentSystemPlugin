#include "Components/WeaponBuilderComponent.h"

#include "Actors/Weapon.h"
#include "Net/UnrealNetwork.h"

UWeaponBuilderComponent::UWeaponBuilderComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UWeaponBuilderComponent::BeginPlay()
{
	Super::BeginPlay();

	Weapom = Cast<AWeapon>(GetOwner());
}

void UWeaponBuilderComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UWeaponBuilderComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, Weapom);
}

void UWeaponBuilderComponent::BuildWeapon()
{
}

void UWeaponBuilderComponent::ClearWeapon()
{
}

AAttachment* UWeaponBuilderComponent::GetAttachmentAtSocket(FName SocketName)
{
	return nullptr;
}

void UWeaponBuilderComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void UWeaponBuilderComponent::BuildFromAttachmentData(AAttachment* Attachment, USceneComponent* ParentComponent)
{
	for (auto ArrayElement : BaseAttachments)
	{
		
	}
}

void UWeaponBuilderComponent::AddBehaviorComponent(AAttachment* Attachment)
{
}
