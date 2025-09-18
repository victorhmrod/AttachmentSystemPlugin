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

    TSet<AAttachment*> Visited;
    TQueue<AAttachment*> Queue;
    TArray<AAttachment*> RootAttachmentsInstances;

    // Spawn and setup base attachments
    for (const TSubclassOf<AAttachment>& AttachmentClass : BaseAttachments)
    {
        if (!*AttachmentClass) continue;

        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = GetOwner();
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        AAttachment* RootInstance = GetWorld()->SpawnActor<AAttachment>(
            AttachmentClass,
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            SpawnParams
        );

        if (!RootInstance) continue;

        RootInstance->BuildAttachment();

        if (Weapon && Weapon->GetRootComponentMesh() && RootInstance->MeshComponent)
        {
            FName RootSocket = NAME_None;

            // Derive socket from category
            if (RootInstance->GetAttachmentInfo().Category != EAttachmentCategory::MonolithicReceiver)
            {
                RootSocket = GetSocketFromCategory(RootInstance->GetAttachmentInfo().Category);
            }

            if (Weapon->GetRootComponentMesh()->DoesSocketExist(RootSocket))
            {
                RootInstance->MeshComponent->AttachToComponent(
                    Weapon->GetRootComponentMesh(),
                    FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                    RootSocket
                );
            }
        }

        Queue.Enqueue(RootInstance);
        Visited.Add(RootInstance);

        RootAttachmentsInstances.Add(RootInstance);
        SpawnedAttachments.Add(RootInstance);
        SpawnedMeshes.Add(RootInstance, RootInstance->MeshComponent);
    }

    // BFS traversal for children
    while (!Queue.IsEmpty())
    {
        AAttachment* Current;
        Queue.Dequeue(Current);
        if (!Current) continue;

        USkeletalMeshComponent* ParentMesh = Current->MeshComponent;

        for (FAttachmentLink& Link : Current->ChildrenLinks)
        {
            if (!*Link.ChildClass) continue;

            FActorSpawnParameters SpawnParams;
            SpawnParams.Owner = GetOwner();
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

            // Spawn child
            AAttachment* ChildInstance = GetWorld()->SpawnActor<AAttachment>(
                Link.ChildClass,
                FVector::ZeroVector,
                FRotator::ZeroRotator,
                SpawnParams
            );

            if (!ChildInstance) continue;

            ChildInstance->BuildAttachment();
            USkeletalMeshComponent* ChildMesh = ChildInstance->MeshComponent;

            FName TargetSocket = GetSocketFromCategory(ChildInstance->GetAttachmentInfo().Category);

            if (ParentMesh && ChildMesh && ParentMesh->DoesSocketExist(TargetSocket))
            {
                ChildMesh->AttachToComponent(
                    ParentMesh,
                    FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                    TargetSocket
                );
            }

            // Register instance
            Link.ChildInstance = ChildInstance;
            Queue.Enqueue(ChildInstance);
            Visited.Add(ChildInstance);

            SpawnedAttachments.Add(ChildInstance);
            SpawnedMeshes.Add(ChildInstance, ChildMesh);
        }
    }
}

void UWeaponBuilderComponent::BuildWeaponFromAttachmentGraph(AAttachment* ParentAttachment, TSet<AAttachment*>& Visited)
{
	if (!ParentAttachment || Visited.Contains(ParentAttachment)) return;
	Visited.Add(ParentAttachment);

	USkeletalMeshComponent* ParentMesh = ParentAttachment->MeshComponent;

	for (const FAttachmentLink& Link : ParentAttachment->ChildrenLinks)
	{
		AAttachment* ChildInstance = Link.ChildInstance;

		if (!ChildInstance && *Link.ChildClass)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = GetOwner();
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			ChildInstance = GetWorld()->SpawnActor<AAttachment>(
				Link.ChildClass,
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				SpawnParams
			);
		}

		if (!ChildInstance) continue;

		ChildInstance->BuildAttachment();
		USkeletalMeshComponent* ChildMesh = ChildInstance->MeshComponent;

		FName TargetSocket = Link.SocketName;
		if (ParentMesh && ChildMesh && ParentMesh->DoesSocketExist(TargetSocket))
		{
			ChildMesh->AttachToComponent(
				ParentMesh,
				FAttachmentTransformRules::SnapToTargetNotIncludingScale,
				TargetSocket
			);
		}

		BuildWeaponFromAttachmentGraph(ChildInstance, Visited);

		SpawnedAttachments.Add(ChildInstance);
		SpawnedMeshes.Add(ChildInstance, ChildMesh);
	}
}

// Clears weapon graph
void UWeaponBuilderComponent::ClearWeapon()
{
	TSet<AAttachment*> Visited;
	for (AAttachment* Root : SpawnedAttachments)
	{
		ClearAttachmentRecursive(Root, Visited);
	}

	for (auto& Pair : SpawnedMeshes)
	{
		if (Pair.Value)
		{
			Pair.Value->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		}
	}

	SpawnedAttachments.Empty();
	SpawnedMeshes.Empty();
}

void UWeaponBuilderComponent::ClearAttachmentRecursive(AAttachment* Attachment, TSet<AAttachment*>& Visited)
{
	if (!Attachment || Visited.Contains(Attachment)) return;
	Visited.Add(Attachment);

	for (const FAttachmentLink& Link : Attachment->ChildrenLinks)
	{
		AAttachment* Child = Link.ChildInstance;
		if (Child)
		{
			ClearAttachmentRecursive(Child, Visited);
		}
	}

	if (Attachment->MeshComponent)
	{
		Attachment->MeshComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	}
}

AAttachment* UWeaponBuilderComponent::GetAttachmentAtSocket(const EAttachmentCategory Category)
{
	TSet<AAttachment*> Visited;
	const FName TargetSocket = GetSocketFromCategory(Category);

	for (AAttachment* Attachment : SpawnedAttachments)
	{
		AAttachment* Found = FindAttachmentRecursive(Attachment, TargetSocket, Visited);
		if (Found) return Found;
	}
	return nullptr;
}

AAttachment* UWeaponBuilderComponent::FindAttachmentRecursive(AAttachment* Attachment, const FName SocketName, TSet<AAttachment*>& Visited)
{
	if (!Attachment || Visited.Contains(Attachment)) return nullptr;
	Visited.Add(Attachment);

	// Check if the attachment's category matches the target socket
	const FName AttachmentSocket = GetSocketFromCategory(Attachment->GetAttachmentInfo().Category);
	if (AttachmentSocket == SocketName)
	{
		return Attachment;
	}

	for (const FAttachmentLink& Link : Attachment->ChildrenLinks)
	{
		AAttachment* ChildInstance = Link.ChildInstance;
		if (!ChildInstance) continue;

		// Compare link socket (from parent to child) with target
		if (Link.SocketName == SocketName)
		{
			return ChildInstance;
		}

		AAttachment* Found = FindAttachmentRecursive(ChildInstance, SocketName, Visited);
		if (Found) return Found;
	}

	return nullptr;
}

FName UWeaponBuilderComponent::GetSocketFromCategory(EAttachmentCategory Category) const
{
	const UEnum* EnumPtr = StaticEnum<EAttachmentCategory>();
	if (!EnumPtr) return NAME_None;

	return FName(EnumPtr->GetNameStringByValue(static_cast<int64>(Category)));
}


void UWeaponBuilderComponent::AddBehaviorComponent(AAttachment* Attachment)
{
}