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
    // Clear previous attachments
    ClearWeapon();

    TSet<AAttachment*> Visited;
    TQueue<AAttachment*> Queue;

	// Temporary array of root instances
    TArray<AAttachment*> RootAttachmentsInstances;

	// Spawn and setup of BaseAttachments
    for (const TSubclassOf<AAttachment>& AttachmentClass : BaseAttachments)
    {
        if (!*AttachmentClass) continue;

    	// Avoid duplicate spawn
        bool bAlreadySpawned = SpawnedAttachments.ContainsByPredicate([&](AAttachment* A) { return A->IsA(AttachmentClass); });
        if (bAlreadySpawned) continue;

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

    	// Connect the root directly to the weapon using the category socket
        if (Weapon && Weapon->GetRootComponentMesh() && RootInstance->MeshComponent)
        {
	        RootInstance->MeshComponent->AttachToComponent(
				Weapon->GetRootComponent(),
				FAttachmentTransformRules::SnapToTargetNotIncludingScale);
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
        	// If an instance already exists, just connect
            if (!Link.ChildInstance && *Link.ChildClass)
            {
            	// Avoid duplicate spawn
                bool bAlreadySpawned = SpawnedAttachments.ContainsByPredicate([&](AAttachment* A) { return A->IsA(Link.ChildClass); });
                if (bAlreadySpawned) continue;

                FActorSpawnParameters SpawnParams;
                SpawnParams.Owner = GetOwner();
                SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

            	// Spawn child instance
                Link.ChildInstance = GetWorld()->SpawnActor<AAttachment>(
                    Link.ChildClass,
                    FVector::ZeroVector,
                    FRotator::ZeroRotator,
                    SpawnParams
                );
            }

            AAttachment* ChildInstance = Link.ChildInstance;
            if (!ChildInstance) continue;

            USkeletalMeshComponent* ChildMesh = ChildInstance->MeshComponent;

        	// Connect via the child category socket
            FName TargetSocket = GetSocketFromCategory(ChildInstance->GetAttachmentInfo().Category);
            if (ParentMesh && ChildMesh && ParentMesh->DoesSocketExist(TargetSocket))
            {
                ChildMesh->AttachToComponent(
                    ParentMesh,
                    FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                    TargetSocket
                );
            }

            // BFS enqueue
            if (!Visited.Contains(ChildInstance))
            {
                Queue.Enqueue(ChildInstance);
                Visited.Add(ChildInstance);
            }

        	// Register globally
            if (!SpawnedAttachments.Contains(ChildInstance))
            {
                SpawnedAttachments.Add(ChildInstance);
                SpawnedMeshes.Add(ChildInstance, ChildMesh);
            }
        }
    }
}

void UWeaponBuilderComponent::BuildWeaponFromAttachmentGraph(AAttachment* ParentAttachment, TSet<AAttachment*>& Visited)
{
	if (!ParentAttachment || Visited.Contains(ParentAttachment)) return;
	Visited.Add(ParentAttachment);

	USkeletalMeshComponent* ParentMesh = ParentAttachment->MeshComponent;

	for (FAttachmentLink& Link : ParentAttachment->ChildrenLinks)
	{
		AAttachment* ChildInstance = Link.ChildInstance;

		// Spawn child if it doesn't exist yet
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

		// Use attachment category as socket
		const FName TargetSocket = GetSocketFromCategory(ChildInstance->GetAttachmentInfo().Category);

		if (ParentMesh && ChildMesh && ParentMesh->DoesSocketExist(TargetSocket))
		{
			ChildMesh->AttachToComponent(
				ParentMesh,
				FAttachmentTransformRules::SnapToTargetNotIncludingScale,
				TargetSocket
			);
		}

		// Recurse into children
		BuildWeaponFromAttachmentGraph(ChildInstance, Visited);

		// Register spawned instance
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

	for (AAttachment* Attachment : SpawnedAttachments)
	{
		AAttachment* Found = FindAttachmentRecursive(Attachment, Category, Visited);
		if (Found) return Found;
	}

	return nullptr;
}

AAttachment* UWeaponBuilderComponent::FindAttachmentRecursive(AAttachment* Attachment, const EAttachmentCategory TargetCategory, TSet<AAttachment*>& Visited)
{
	if (!Attachment || Visited.Contains(Attachment)) return nullptr;
	Visited.Add(Attachment);

	// Check if the current attachment's category matches the target
	if (Attachment->GetAttachmentInfo().Category == TargetCategory)
	{
		return Attachment;
	}

	// Traverse all children links
	for (const FAttachmentLink& Link : Attachment->ChildrenLinks)
	{
		AAttachment* ChildInstance = Link.ChildInstance;
		if (!ChildInstance) continue;

		// Check if the child's category matches
		if (ChildInstance->GetAttachmentInfo().Category == TargetCategory)
		{
			return ChildInstance;
		}

		// Recursively search in the child branch
		AAttachment* Found = FindAttachmentRecursive(ChildInstance, TargetCategory, Visited);
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