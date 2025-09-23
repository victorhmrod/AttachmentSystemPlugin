#include "Components/WeaponBuilderComponent.h"

#include "Engine/HitResult.h"
#include "Engine/OverlapResult.h"
#include "Actors/Attachment.h"
#include "Actors/RailAttachment.h"
#include "Actors/Weapon.h"
#include "Components/BoxComponent.h"
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

        for (FAttachmentLink& Link : Current->ChildrenLinks)
        {
            // Spawn child if needed
            if (!Link.ChildInstance && *Link.ChildClass)
            {
                FActorSpawnParameters SpawnParams;
                SpawnParams.Owner = GetOwner();
                SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

                Link.ChildInstance = GetWorld()->SpawnActor<AAttachment>(
                    Link.ChildClass,
                    FVector::ZeroVector,
                    FRotator::ZeroRotator,
                    SpawnParams
                );
            }

            AAttachment* ChildInstance = Link.ChildInstance;
            if (!ChildInstance) continue;

            ChildInstance->BuildAttachment();
            USkeletalMeshComponent* ChildMesh = ChildInstance->MeshComponent;

            // Socket by category
            FAttachmentInfo ChildInfo = ChildInstance->GetAttachmentInfo();
            FName TargetSocket = GetSocketFromCategory(ChildInfo.Category);

            // --- Case 1: Parent is a rail ---
            if (ARailAttachment* Rail = Cast<ARailAttachment>(Current))
            {
                if (ChildInfo.bUseRail)
                {
                    // ---- Full Rail pipeline ----
                    int32 DesiredSlot = ChildInstance->StartPosition;

                    // 1. UI check (collision preview at socket position)
                    FTransform DesiredTransform = ParentMesh->GetSocketTransform(TargetSocket);
                    bool bUICheck = CanPlaceAttachmentUI(ChildInstance, DesiredTransform);

                    // 2. Spline check (rail limits)
                    bool bSplineCheck = (DesiredSlot + ChildInstance->Size) <= Rail->NumSlots;

                    // 3. Bitmask check (slot occupancy)
                    bool bMaskCheck = Rail->CanPlaceAttachment(ChildInstance);

                    bool bSocketExists = ParentMesh->DoesSocketExist(TargetSocket);

                    if (bUICheck && bSplineCheck && bMaskCheck && bSocketExists)
                    {
                        Rail->PlaceAttachment(ChildInstance);

                        ChildMesh->AttachToComponent(
                            ParentMesh,
                            FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                            TargetSocket
                        );
                        ChildMesh->SetRelativeTransform(Link.Offset);

                        UE_LOG(LogTemp, Warning, TEXT("Attached %s using RAIL pipeline to %s"),
                            *ChildInstance->GetName(), *Rail->GetName());
                    }
                    else
                    {
                        UE_LOG(LogTemp, Error, TEXT("FAILED: %s could not attach to Rail %s (UI=%d Spline=%d Mask=%d Socket=%d)"),
                            *ChildInstance->GetName(),
                            *Rail->GetName(),
                            bUICheck, bSplineCheck, bMaskCheck, bSocketExists);
                    }
                }
                else
                {
                    // ---- Standard pipeline, even though parent is a rail ----
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


bool UWeaponBuilderComponent::CanPlaceAttachmentUI(AAttachment* Attachment, const FTransform& DesiredTransform) const
{
	if (!Attachment) return false;

	UBoxComponent* Box = Attachment->GetBoxComponent();
	if (!Box) return true; // no box = no collision

	FVector Extent = Box->GetUnscaledBoxExtent();

	// Overlap test
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Attachment);
	Params.AddIgnoredActor(GetOwner());

	bool bHit = GetWorld()->OverlapMultiByChannel(
		Overlaps,
		DesiredTransform.GetLocation(),
		DesiredTransform.GetRotation(),
		ECC_GameTraceChannel1, // custom channel for attachments
		FCollisionShape::MakeBox(Extent),
		Params
	);

#if WITH_EDITOR
	// Draw preview box (green = valid, red = invalid)
	FColor DebugColor = bHit ? FColor::Red : FColor::Green;
	DrawDebugBox(
		GetWorld(),
		DesiredTransform.GetLocation(),
		Extent,
		DesiredTransform.GetRotation(),
		DebugColor,
		false,
		0.1f // duration
	);
#endif

	return !bHit;
}