#include "Components/WeaponBuilderComponent.h"

#include "Engine/HitResult.h"
#include "Actors/Attachment.h"
#include "Actors/RailAttachment.h"
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
    // Clear old attachments
    ClearWeapon();

    TSet<AAttachment*> Visited;
    TQueue<AAttachment*> Queue;

    // Spawn and set up BaseAttachments (roots)
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
            RootInstance->MeshComponent->AttachToComponent(
                Weapon->GetRootComponentMesh(),
                FAttachmentTransformRules::SnapToTargetNotIncludingScale
            );
            RootInstance->MeshComponent->SetRelativeTransform(FTransform::Identity);
        }

        Queue.Enqueue(RootInstance);
        Visited.Add(RootInstance);

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

        // 🔑 Loop through each link of this parent
        for (FAttachmentLink& Link : Current->ChildrenLinks)
        {
            // Ensure arrays have the same size
            for (int32 i = 0; i < Link.ChildClasses.Num(); i++)
            {
                // Spawn child if needed
                if (!Link.ChildInstances.IsValidIndex(i) || !Link.ChildInstances[i])
                {
                    if (*Link.ChildClasses[i])
                    {
                        FActorSpawnParameters SpawnParams;
                        SpawnParams.Owner = GetOwner();
                        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

                        AAttachment* Spawned = GetWorld()->SpawnActor<AAttachment>(
                            Link.ChildClasses[i],
                            FVector::ZeroVector,
                            FRotator::ZeroRotator,
                            SpawnParams
                        );
                        if (Link.ChildInstances.IsValidIndex(i))
                            Link.ChildInstances[i] = Spawned;
                        else
                            Link.ChildInstances.Add(Spawned);
                    }
                }

                AAttachment* ChildInstance = Link.ChildInstances.IsValidIndex(i) ? Link.ChildInstances[i] : nullptr;
                if (!ChildInstance) continue;

                ChildInstance->BuildAttachment();
                USkeletalMeshComponent* ChildMesh = ChildInstance->MeshComponent;

                // Assign start slot if defined
                int32 DesiredSlot = Link.StartSlots.IsValidIndex(i) ? Link.StartSlots[i] : 0;
                ChildInstance->StartPosition = DesiredSlot;

                // Socket by category
                FAttachmentInfo ChildInfo = ChildInstance->GetAttachmentInfo();
                FName TargetSocket = GetSocketFromCategory(ChildInfo.Category);

                // --- Case 1: Parent is a rail ---
                if (ARailAttachment* Rail = Cast<ARailAttachment>(Current))
                {
                    if (ChildInfo.bUseRail)
                    {
                        // 2. Spline check
                        bool bSplineCheck = (DesiredSlot + ChildInstance->Size) <= Rail->NumSlots;

                        // 3. Bitmask check
                        bool bMaskCheck = Rail->CanPlaceAttachment(ChildInstance);

                        bool bSocketExists = ParentMesh->DoesSocketExist(TargetSocket);

                        if (bSplineCheck && bMaskCheck && bSocketExists)
                        {
                            Rail->PlaceAttachment(ChildInstance);

                            ChildMesh->AttachToComponent(
                                ParentMesh,
                                FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                                TargetSocket
                            );
                            ChildMesh->SetRelativeTransform(Link.Offset);

                            UE_LOG(LogTemp, Warning, TEXT("Attached %s using RAIL pipeline to %s (Slot=%d)"),
                                *ChildInstance->GetName(), *Rail->GetName(), DesiredSlot);
                        }
                        else
                        {
                            UE_LOG(LogTemp, Error, TEXT("FAILED: %s could not attach to Rail %s (Spline=%d Mask=%d Socket=%d Slot=%d)"),
                                *ChildInstance->GetName(),
                                *Rail->GetName(),
                                bSplineCheck, bMaskCheck, bSocketExists, DesiredSlot);
                        }
                    }
                    else
                    {
                        // Standard attach even if parent is a rail
                        if (ParentMesh && ChildMesh && ParentMesh->DoesSocketExist(TargetSocket))
                        {
                            ChildMesh->AttachToComponent(
                                ParentMesh,
                                FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                                TargetSocket
                            );
                            ChildMesh->SetRelativeTransform(Link.Offset);

                            UE_LOG(LogTemp, Warning, TEXT("Attached %s using STANDARD pipeline on rail %s"),
                                *ChildInstance->GetName(), *Rail->GetName());
                        }
                    }
                }
                // --- Case 2: Normal parent (non-rail) ---
                else if (ParentMesh && ChildMesh && ParentMesh->DoesSocketExist(TargetSocket))
                {
                    ChildMesh->AttachToComponent(
                        ParentMesh,
                        FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                        TargetSocket
                    );
                    ChildMesh->SetRelativeTransform(Link.Offset);

                    UE_LOG(LogTemp, Warning, TEXT("Attached %s to non-rail parent %s"),
                        *ChildInstance->GetName(), *Current->GetName());
                }

                // Continue BFS
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
}

void UWeaponBuilderComponent::BuildWeaponFromAttachmentGraph(AAttachment* ParentAttachment, TSet<AAttachment*>& Visited)
{
	if (!ParentAttachment || Visited.Contains(ParentAttachment)) return;
	Visited.Add(ParentAttachment);

	USkeletalMeshComponent* ParentMesh = ParentAttachment->MeshComponent;

	for (FAttachmentLink& Link : ParentAttachment->ChildrenLinks)
	{
		// Iterate over all defined child classes
		for (TSubclassOf<AAttachment> ChildClass : Link.ChildClasses)
		{
			if (!*ChildClass) continue;

			// Always spawn a new instance (multiple children of same class allowed)
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = GetOwner();
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			AAttachment* ChildInstance = GetWorld()->SpawnActor<AAttachment>(
				ChildClass,
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				SpawnParams
			);

			if (!ChildInstance) continue;

			ChildInstance->BuildAttachment();
			USkeletalMeshComponent* ChildMesh = ChildInstance->MeshComponent;

			// Use category to resolve target socket
			const FName TargetSocket = GetSocketFromCategory(ChildInstance->GetAttachmentInfo().Category);

			// Attach to parent if socket exists
			if (ParentMesh && ChildMesh && ParentMesh->DoesSocketExist(TargetSocket))
			{
				ChildMesh->AttachToComponent(
					ParentMesh,
					FAttachmentTransformRules::SnapToTargetNotIncludingScale,
					TargetSocket
				);
				ChildMesh->SetRelativeTransform(Link.Offset);
			}

			// Recursively build this child's subtree
			BuildWeaponFromAttachmentGraph(ChildInstance, Visited);

			// Register inside the link
			Link.ChildInstances.Add(ChildInstance);

			// Register globally
			SpawnedAttachments.Add(ChildInstance);
			SpawnedMeshes.Add(ChildInstance, ChildMesh);
		}
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

	// Recursively clear all children in this link
	for (const FAttachmentLink& Link : Attachment->ChildrenLinks)
	{
		for (AAttachment* Child : Link.ChildInstances)
		{
			if (Child)
			{
				ClearAttachmentRecursive(Child, Visited);
			}
		}
	}

	// Detach this attachment's mesh
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

AAttachment* UWeaponBuilderComponent::FindAttachmentRecursive(
	AAttachment* Attachment,
	const EAttachmentCategory TargetCategory,
	TSet<AAttachment*>& Visited)
{
	if (!Attachment || Visited.Contains(Attachment)) return nullptr;
	Visited.Add(Attachment);

	// Check if the current attachment's category matches the target
	if (Attachment->GetAttachmentInfo().Category == TargetCategory)
	{
		return Attachment;
	}

	// Traverse all children in all links
	for (const FAttachmentLink& Link : Attachment->ChildrenLinks)
	{
		for (AAttachment* Child : Link.ChildInstances)
		{
			if (!Child) continue;

			// Check if this child matches the category
			if (Child->GetAttachmentInfo().Category == TargetCategory)
			{
				return Child;
			}

			// Recursively search deeper
			if (AAttachment* Found = FindAttachmentRecursive(Child, TargetCategory, Visited))
			{
				return Found;
			}
		}
	}

	return nullptr; // Nothing found in this branch
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