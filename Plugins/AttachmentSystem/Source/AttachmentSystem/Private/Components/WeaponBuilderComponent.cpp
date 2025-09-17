#include "Components/WeaponBuilderComponent.h"

#include "Actors/Attachment.h"
#include "Actors/Weapon.h"
#include "Net/UnrealNetwork.h"

UWeaponBuilderComponent::UWeaponBuilderComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	SetIsReplicatedByDefault(true);
}

void UWeaponBuilderComponent::BeginPlay()
{
	Super::BeginPlay();
	Weapom = Cast<AWeapon>(GetOwner());
}

void UWeaponBuilderComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearWeapon();
	Super::EndPlay(EndPlayReason);
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
	TSet<AAttachment*> Visited;

	for (AAttachment* Attachment : BaseAttachments)
	{
		BuildWeaponFromAttachmentGraph(Attachment, nullptr, Visited);
	}
}

void UWeaponBuilderComponent::BuildWeaponFromAttachmentGraph(AAttachment* Attachment, const AAttachment* ParentAttachment, TSet<AAttachment*>& Visited)
{
	if (!Attachment) return;
	if (Visited.Contains(Attachment)) return;

	Visited.Add(Attachment);

	USkeletalMeshComponent* ParentMesh = ParentAttachment ? ParentAttachment->GetMeshComponent() : nullptr;
	USkeletalMeshComponent* ChildMesh = Attachment->GetMeshComponent();

	// Only attach if it has a valid parent and the socket exists
	if (ParentMesh && ChildMesh && ParentMesh->DoesSocketExist(Attachment->GetSocketName()))
	{
		ChildMesh->AttachToComponent(
			ParentMesh,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			Attachment->GetSocketName()
		);
	}

	// Continue browsing the graph
	for (AAttachment* Connected : Attachment->GetConnectedAttachments())
	{
		if (Connected != ParentAttachment)
		{
			BuildWeaponFromAttachmentGraph(Connected, Attachment, Visited);
		}
	}
}

void UWeaponBuilderComponent::ClearWeapon()
{
	for (AAttachment* Attachment : BaseAttachments)
	{
		ClearAttachmentRecursive(Attachment, nullptr);
	}

	BaseAttachments.Empty();
}

void UWeaponBuilderComponent::ClearAttachmentRecursive(AAttachment* Attachment, AAttachment* Parent)
{
	if (!Attachment) return;

	for (AAttachment* Connected : Attachment->GetConnectedAttachments())
	{
		if (Connected != Parent)
		{
			ClearAttachmentRecursive(Connected, Attachment);
		}
	}

	if (Attachment->GetMeshComponent())
	{
		Attachment->GetMeshComponent()->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	}

	// Optional: Clean up connections
	// Attachment->ConnectedAttachments.Empty();
}

AAttachment* UWeaponBuilderComponent::GetAttachmentAtSocket(FName SocketName)
{
	for (AAttachment* Attachment : BaseAttachments)
	{
		AAttachment* Found = FindAttachmentRecursive(Attachment, nullptr, SocketName);
		if (Found) return Found;
	}
	return nullptr;
}

AAttachment* UWeaponBuilderComponent::FindAttachmentRecursive(AAttachment* Attachment, AAttachment* Parent, FName SocketName)
{
	if (!Attachment) return nullptr;

	if (Attachment->GetSocketName() == SocketName)
	{
		return Attachment;
	}

	for (AAttachment* Connected : Attachment->GetConnectedAttachments())
	{
		if (Connected != Parent)
		{
			AAttachment* Found = FindAttachmentRecursive(Connected, Attachment, SocketName);
			if (Found) return Found;
		}
	}

	return nullptr;
}

void UWeaponBuilderComponent::AddBehaviorComponent(AAttachment* Attachment)
{
	// Example:
	// if (Attachment->AttachmentInfo.Type == EAttachmentType::Scope)
	// {
	//     AddComponent<UZoomComponent>(...);
	// }
}