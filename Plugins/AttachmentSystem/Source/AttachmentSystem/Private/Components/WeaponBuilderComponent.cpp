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
	Weapon = Cast<AWeapon>(GetOwner());
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
	DOREPLIFETIME(ThisClass, Weapon);
}

void UWeaponBuilderComponent::BuildWeapon()
{
	ClearWeapon();
	BuildWeaponBFS();
}

void UWeaponBuilderComponent::BuildWeaponBFS()
{
    if (!IsValid(Weapon) || !Weapon->GetRootComponentMesh())
    {
	    return;
    }

    TSet<AAttachment*> Visited;
    TQueue<AAttachment*> Queue;

    // Spawn e setup das instâncias base
    for (TSubclassOf AttachmentClass : BaseAttachments)
    {
        if (!AttachmentClass) continue;

        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = GetOwner();
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        AAttachment* AttachmentInstance = GetWorld()->SpawnActor<AAttachment>(
            AttachmentClass,
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            SpawnParams
        );

        if (!AttachmentInstance) continue;

        AttachmentInstance->BuildAttachment();
        SpawnedAttachments.Add(AttachmentInstance);

        if (AttachmentInstance->MeshComponent)
        {
            SpawnedMeshes.Add(AttachmentInstance, AttachmentInstance->MeshComponent);

            AttachmentInstance->MeshComponent->AttachToComponent(
                Weapon->GetRootComponentMesh(),
                FAttachmentTransformRules::SnapToTargetNotIncludingScale
            );
        }

        Queue.Enqueue(AttachmentInstance);
        Visited.Add(AttachmentInstance);
    }

    while (!Queue.IsEmpty())
    {
        AAttachment* Current;
        Queue.Dequeue(Current);

        USceneComponent* ParentMesh = Current->MeshComponent;
        const EAttachmentCategory ParentType = Current->GetAttachmentInfo().Category;

        for (const FAttachmentLink& Link : Current->ChildrenLinks)
        {
            if (!Link.Child || Visited.Contains(Link.Child)) continue;

            EAttachmentCategory ChildType = Link.Child->GetAttachmentInfo().Category;
            bool bValid = true;

            if (ParentType == EAttachmentCategory::LowerReceiver && ChildType != EAttachmentCategory::Barrel)
            {
                UE_LOG(LogTemp, Warning, TEXT("Cannot attach %s to LowerReceiver: type mismatch!"), *Link.Child->GetName());
                bValid = false;
            }

            if (!bValid) continue;

            USceneComponent* ChildMesh = Link.Child->MeshComponent;

            if (ParentMesh && ChildMesh && ParentMesh->DoesSocketExist(Link.SocketName))
            {
                ChildMesh->AttachToComponent(
                    ParentMesh,
                    FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                    Link.SocketName
                );
            }

            Queue.Enqueue(Link.Child);
            Visited.Add(Link.Child);
        }
    }
}

void UWeaponBuilderComponent::BuildWeaponFromAttachmentGraph(AAttachment* ParentAttachment, TSet<AAttachment*>& Visited)
{
	if (!ParentAttachment || Visited.Contains(ParentAttachment)) return;
	Visited.Add(ParentAttachment);

	USceneComponent* ParentMesh = ParentAttachment->MeshComponent;

	for (const FAttachmentLink& Link : ParentAttachment->ChildrenLinks)
	{
		if (!Link.Child) continue;

		USceneComponent* ChildMesh = Link.Child->MeshComponent;

		if (ParentMesh && ChildMesh && ParentMesh->DoesSocketExist(Link.SocketName))
		{
			ChildMesh->AttachToComponent(
				ParentMesh,
				FAttachmentTransformRules::SnapToTargetNotIncludingScale,
				Link.SocketName
			);
		}

		BuildWeaponFromAttachmentGraph(Link.Child, Visited);
	}
}

void UWeaponBuilderComponent::ClearWeapon()
{
	TSet<AAttachment*> Visited;

	for (AAttachment* Attachment : SpawnedAttachments)
	{
		ClearAttachmentRecursive(Attachment, Visited);

		SpawnedMeshes.Remove(Attachment);

		if (Attachment && !Attachment->IsPendingKillPending())
		{
			Attachment->Destroy();
		}
	}

	SpawnedAttachments.Empty();
}

void UWeaponBuilderComponent::ClearAttachmentRecursive(AAttachment* Attachment, TSet<AAttachment*>& Visited)
{
	if (!Attachment || Visited.Contains(Attachment)) return;
	Visited.Add(Attachment);

	for (const FAttachmentLink& Link : Attachment->ChildrenLinks)
	{
		if (Link.Child)
		{
			ClearAttachmentRecursive(Link.Child, Visited);
		}
	}

	if (Attachment->MeshComponent)
	{
		Attachment->MeshComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	}
}

AAttachment* UWeaponBuilderComponent::GetAttachmentAtSocket(const FName SocketName)
{
	TSet<AAttachment*> Visited;
	for (AAttachment* Attachment : SpawnedAttachments)
	{
		AAttachment* Found = FindAttachmentRecursive(Attachment, SocketName, Visited);
		if (Found) return Found;
	}
	return nullptr;
}

AAttachment* UWeaponBuilderComponent::FindAttachmentRecursive(AAttachment* Attachment, FName SocketName, TSet<AAttachment*>& Visited)
{
	if (!Attachment || Visited.Contains(Attachment)) return nullptr;
	Visited.Add(Attachment);

	for (const FAttachmentLink& Link : Attachment->ChildrenLinks)
	{
		if (Link.SocketName == SocketName)
		{
			return Link.Child;
		}

		AAttachment* Found = FindAttachmentRecursive(Link.Child, SocketName, Visited);
		if (Found) return Found;
	}

	return nullptr;
}

void UWeaponBuilderComponent::AddBehaviorComponent(AAttachment* Attachment)
{
}