#include "Components/WeaponBuilderComponent.h"

#include "Actors/Attachment.h"
#include "Actors/RailAttachment.h"
#include "Actors/Weapon.h"
#include "Components/SplineComponent.h"
#include "Engine/HitResult.h"
#include "Engine/OverlapResult.h"
#include "Net/UnrealNetwork.h"

UWeaponBuilderComponent::UWeaponBuilderComponent() {
  PrimaryComponentTick.bCanEverTick = true;
  SetIsReplicatedByDefault(true);
}

void UWeaponBuilderComponent::BeginPlay() {
  Super::BeginPlay();
  Weapon = Cast<AWeapon>(GetOwner());
}

void UWeaponBuilderComponent::EndPlay(
    const EEndPlayReason::Type EndPlayReason) {
  ClearWeapon();
  Super::EndPlay(EndPlayReason);
}

void UWeaponBuilderComponent::TickComponent(
    const float DeltaTime, const ELevelTick TickType,
    FActorComponentTickFunction *ThisTickFunction) {
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UWeaponBuilderComponent::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty> &OutLifetimeProps) const {
  Super::GetLifetimeReplicatedProps(OutLifetimeProps);
  DOREPLIFETIME(ThisClass, Weapon);
}

void UWeaponBuilderComponent::BuildWeapon() {
  // if it's not a server, ask the server to execute
  if (!GetOwner() || !GetOwner()->HasAuthority()) {
    Server_BuildWeapon();
    return;
  }

  // Clear old attachments
  ClearWeapon();

  TSet<AAttachment *> Visited;
  TQueue<AAttachment *> Queue;

  // Spawn and set up BaseAttachments (roots)
  for (const TSubclassOf<AAttachment> &AttachmentClass : BaseAttachments) {
    if (!*AttachmentClass)
      continue;

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = GetOwner();
    SpawnParams.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AAttachment *RootInstance = GetWorld()->SpawnActor<AAttachment>(
        AttachmentClass, FVector::ZeroVector, FRotator::ZeroRotator,
        SpawnParams);
    if (!RootInstance)
      continue;

    RootInstance->LoadAttachmentInfo();

    if (Weapon && Weapon->GetRoot() && RootInstance->MeshComponent) {
      RootInstance->MeshComponent->AttachToComponent(
          Weapon->GetRoot(),
          FAttachmentTransformRules::SnapToTargetNotIncludingScale);
      RootInstance->MeshComponent->SetRelativeTransform(FTransform::Identity);
    }

    Queue.Enqueue(RootInstance);
    Visited.Add(RootInstance);

    SpawnedAttachments.Add(RootInstance);
    SpawnedMeshes.Add(RootInstance, RootInstance->MeshComponent);
  }

  // BFS traversal for children
  while (!Queue.IsEmpty()) {
    AAttachment *Current;
    Queue.Dequeue(Current);
    if (!Current)
      continue;

    USkeletalMeshComponent *ParentMesh = Current->MeshComponent;

    for (FAttachmentLink &Link : Current->ChildrenLinks) {
      // Spawn child instances if needed
      if (Link.ChildInstances.Num() == 0 && Link.ChildClasses.Num() > 0) {
        for (const TSubclassOf<AAttachment> &ChildClass : Link.ChildClasses) {
          if (!*ChildClass)
            continue;

          FActorSpawnParameters SpawnParams;
          SpawnParams.Owner = GetOwner();
          SpawnParams.SpawnCollisionHandlingOverride =
              ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

          AAttachment *NewChild = GetWorld()->SpawnActor<AAttachment>(
              ChildClass, FVector::ZeroVector, FRotator::ZeroRotator,
              SpawnParams);

          if (NewChild) {
            Link.ChildInstances.Add(NewChild);
          }
        }
      }

      // NOTE: switched to index-based loop so we can null-out failed spawns
      // safely
      for (int32 i = 0; i < Link.ChildInstances.Num(); ++i) {
        AAttachment *ChildInstance = Link.ChildInstances[i];
        if (!ChildInstance)
          continue;

        ChildInstance->LoadAttachmentInfo();
        USkeletalMeshComponent *ChildMesh = ChildInstance->MeshComponent;

        // Socket by category
        FAttachmentInfo ChildInfo = ChildInstance->GetAttachmentInfo();
        FName TargetSocket = GetSocketFromCategory(ChildInfo.Category);

        bool bShouldRegister =
            false; // NEW: only register/enqueue if successfully placed/attached

        // --- Case 1: Parent is a rail ---
        if (ARailAttachment *Rail = Cast<ARailAttachment>(Current)) {
          if (ChildInfo.bUseRail) {
            const float RailLength = Rail->GetSplineLength();
            constexpr float Step = 2.5f;

            FVector SocketLoc = ParentMesh->GetSocketLocation(TargetSocket);
            float SocketZ = SocketLoc.Z;

            bool bPlaced = false;
            float CurrentDist = 0.f;
            FTransform TestTransform;
            TestTransform.SetRotation(FQuat::Identity);

            // Sweep along spline: Start -> End
            while (CurrentDist <= RailLength) {
              FVector SplineLoc =
                  Rail->RailSpline->GetLocationAtDistanceAlongSpline(
                      CurrentDist, ESplineCoordinateSpace::World);

              // Force Z from socket
              SplineLoc.Z = SocketZ;
              TestTransform.SetLocation(SplineLoc);

              // Map dist -> slot
              int32 MappedSlot = Rail->GetSlotFromDistance(CurrentDist);
              ChildInstance->StartPosition = MappedSlot;

              // Checks
              bool bSplineCheck =
                  (MappedSlot + ChildInstance->Size) <= Rail->NumSlots;
              bool bMaskCheck = Rail->CanPlaceAttachment(ChildInstance);
              bool bSocketExists = ParentMesh->DoesSocketExist(TargetSocket);
              bool bCollisionFree =
                  !DoesCollideWithRail(TestTransform, ChildMesh, Rail);

              UE_LOG(LogTemp, Warning,
                     TEXT("Checks for %s -> Spline=%d | Mask=%d | Socket=%d | "
                          "Collision=%d | Slot=%d/%d"),
                     *ChildInstance->GetName(), bSplineCheck, bMaskCheck,
                     bSocketExists, bCollisionFree, MappedSlot,
                     Rail->NumSlots - 1);

              if (bSplineCheck && bMaskCheck && bSocketExists &&
                  bCollisionFree) {
                Rail->PlaceAttachment(ChildInstance);

                ChildMesh->AttachToComponent(
                    ParentMesh,
                    FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                    TargetSocket);
                ChildMesh->SetWorldLocation(SplineLoc);

                UE_LOG(LogTemp, Log,
                       TEXT("Attached %s at slot %d (dist=%.2f) on rail %s | "
                            "Z=%.2f"),
                       *ChildInstance->GetName(), MappedSlot, CurrentDist,
                       *Rail->GetName(), SocketZ);

                bPlaced = true;
                bShouldRegister = true; // NEW
                break;
              }

              CurrentDist += Step;
            }

            if (!bPlaced) {
              UE_LOG(LogTemp, Warning,
                     TEXT("Rejected %s -> did not pass checks (rail)"),
                     *ChildInstance->GetName());

              // NEW: destroy failed piece and clear the slot in the array
              ChildInstance->Destroy();
              Link.ChildInstances[i] = nullptr;
            }
          } else {
            // ---- Standard pipeline, even though parent is a rail ----
            if (ParentMesh && ChildMesh &&
                ParentMesh->DoesSocketExist(TargetSocket)) {
              ChildMesh->AttachToComponent(
                  ParentMesh,
                  FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                  TargetSocket);
              ChildMesh->SetRelativeTransform(Link.Offset);

              UE_LOG(LogTemp, Log,
                     TEXT("Attached %s using STANDARD pipeline on rail %s"),
                     *ChildInstance->GetName(), *Rail->GetName());

              bShouldRegister = true; // NEW: standard attached ok
            } else {
              // NEW: destroy if we cannot attach even in standard case
              UE_LOG(
                  LogTemp, Warning,
                  TEXT("Rejected %s -> no valid socket for STANDARD pipeline"),
                  *ChildInstance->GetName());
              ChildInstance->Destroy();
              Link.ChildInstances[i] = nullptr;
            }
          }
        }
        // --- Case 2: Normal parent (non-rail) ---
        else if (ParentMesh && ChildMesh &&
                 ParentMesh->DoesSocketExist(TargetSocket)) {
          ChildMesh->AttachToComponent(
              ParentMesh,
              FAttachmentTransformRules::SnapToTargetNotIncludingScale,
              TargetSocket);
          ChildMesh->SetRelativeTransform(Link.Offset);

          UE_LOG(LogTemp, Log, TEXT("Attached %s to non-rail parent %s"),
                 *ChildInstance->GetName(), *Current->GetName());

          bShouldRegister = true; // NEW
        } else {
          // NEW: couldn't attach to non-rail either → destroy
          UE_LOG(LogTemp, Warning,
                 TEXT("Rejected %s -> no valid socket on non-rail parent"),
                 *ChildInstance->GetName());
          ChildInstance->Destroy();
          Link.ChildInstances[i] = nullptr;
        }

        // --- NEW: only enqueue/register if we actually attached/placed it ---
        if (bShouldRegister) {
          if (!Visited.Contains(ChildInstance)) {
            Queue.Enqueue(ChildInstance);
            Visited.Add(ChildInstance);
          }

          if (!SpawnedAttachments.Contains(ChildInstance)) {
            SpawnedAttachments.Add(ChildInstance);
            SpawnedMeshes.Add(ChildInstance, ChildMesh);
          }
        }
      } // end for i
    } // end for Link
  } // end BFS

  // --- Broadcast to listeners (e.g. Weapon) that build is complete ---
  OnWeaponBuilt.Broadcast(SpawnedAttachments);
}

void UWeaponBuilderComponent::Server_BuildWeapon_Implementation() {
  if (!GetOwner() || !GetOwner()->HasAuthority())
    return;

  BuildWeapon();
}

bool UWeaponBuilderComponent::DoesCollideWithRail(
    const FTransform &TestTransform, USkeletalMeshComponent *ChildMesh,
    AActor *IgnoredActor) const {
  if (!ChildMesh)
    return true;

  // Slightly inflate extents so “almost touching” still counts
  FVector Extents = ChildMesh->Bounds.BoxExtent * 1.2f;

  TArray<FOverlapResult> Overlaps;

  FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(DoesCollideWithRail),
                                    /*bTraceComplex=*/false);
  QueryParams.bFindInitialOverlaps = true;
  QueryParams.AddIgnoredActor(IgnoredActor);  // the rail
  QueryParams.AddIgnoredActor(GetOwner());    // weapon
  QueryParams.AddIgnoredComponent(ChildMesh); // ignore the candidate itself

  // Look for WorldDynamic objects (what we set on attachment meshes)
  FCollisionObjectQueryParams ObjParams;
  ObjParams.AddObjectTypesToQuery(ECC_WorldDynamic);

  const bool bHit = GetWorld()->OverlapMultiByObjectType(
      Overlaps, TestTransform.GetLocation(), TestTransform.GetRotation(),
      ObjParams, FCollisionShape::MakeBox(Extents), QueryParams);

  UE_LOG(LogTemp, Warning,
         TEXT("Collision test at %s | Overlaps=%d | Result=%d"),
         *TestTransform.GetLocation().ToString(), Overlaps.Num(), bHit);

#if WITH_EDITOR
  // Draw the test box
  DrawDebugBox(GetWorld(), TestTransform.GetLocation(), Extents,
               TestTransform.GetRotation(), bHit ? FColor::Red : FColor::Green,
               false, 5.f);
#endif

  // Print who we overlapped
  if (bHit) {
    for (const FOverlapResult &Res : Overlaps) {
      if (const AActor *HitActor = Res.GetActor()) {
        UE_LOG(LogTemp, Warning, TEXT("  -> overlap: %s"),
               *HitActor->GetName());
      }
    }
  }

  return bHit;
}

void UWeaponBuilderComponent::BuildWeaponFromAttachmentGraph(
    AAttachment *ParentAttachment, TSet<AAttachment *> &Visited) {
  if (!ParentAttachment || Visited.Contains(ParentAttachment))
    return;
  Visited.Add(ParentAttachment);

  USkeletalMeshComponent *ParentMesh = ParentAttachment->MeshComponent;

  for (FAttachmentLink &Link : ParentAttachment->ChildrenLinks) {
    // Iterate over all defined child classes
    for (TSubclassOf<AAttachment> ChildClass : Link.ChildClasses) {
      if (!*ChildClass)
        continue;

      // Always spawn a new instance (multiple children of same class allowed)
      FActorSpawnParameters SpawnParams;
      SpawnParams.Owner = GetOwner();
      SpawnParams.SpawnCollisionHandlingOverride =
          ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

      AAttachment *ChildInstance = GetWorld()->SpawnActor<AAttachment>(
          ChildClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

      if (!ChildInstance)
        continue;

      ChildInstance->LoadAttachmentInfo();
      USkeletalMeshComponent *ChildMesh = ChildInstance->MeshComponent;

      // Use category to resolve target socket
      const FName TargetSocket =
          GetSocketFromCategory(ChildInstance->GetAttachmentInfo().Category);

      // Attach to parent if socket exists
      if (ParentMesh && ChildMesh &&
          ParentMesh->DoesSocketExist(TargetSocket)) {
        ChildMesh->AttachToComponent(
            ParentMesh,
            FAttachmentTransformRules::SnapToTargetNotIncludingScale,
            TargetSocket);
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
void UWeaponBuilderComponent::ClearWeapon() {
  if (!GetOwner() || !GetOwner()->HasAuthority()) {
    Server_ClearWeapon();
    return;
  }

  TSet<AAttachment *> Visited;
  for (AAttachment *Root : SpawnedAttachments) {
    ClearAttachmentRecursive(Root, Visited);
  }

  for (auto &Pair : SpawnedMeshes) {
    if (Pair.Value) {
      Pair.Value->DetachFromComponent(
          FDetachmentTransformRules::KeepWorldTransform);
    }
  }

  SpawnedAttachments.Empty();
  SpawnedMeshes.Empty();
}

void UWeaponBuilderComponent::Server_ClearWeapon_Implementation() {
  if (!GetOwner() || !GetOwner()->HasAuthority())
    return;

  ClearWeapon();
}

void UWeaponBuilderComponent::ClearAttachmentRecursive(
    AAttachment *Attachment, TSet<AAttachment *> &Visited) {
  if (!Attachment || Visited.Contains(Attachment))
    return;
  Visited.Add(Attachment);

  // Recursively clear all children in this link
  for (const FAttachmentLink &Link : Attachment->ChildrenLinks) {
    for (AAttachment *Child : Link.ChildInstances) {
      if (Child) {
        ClearAttachmentRecursive(Child, Visited);
      }
    }
  }

  // Detach this attachment's mesh
  if (Attachment->MeshComponent) {
    Attachment->MeshComponent->DetachFromComponent(
        FDetachmentTransformRules::KeepWorldTransform);
  }
}

AAttachment *UWeaponBuilderComponent::GetAttachmentAtSocket(
    const EAttachmentCategory Category) {
  TSet<AAttachment *> Visited;

  for (AAttachment *Attachment : SpawnedAttachments) {
    AAttachment *Found = FindAttachmentRecursive(Attachment, Category, Visited);
    if (Found)
      return Found;
  }

  return nullptr;
}

AAttachment *UWeaponBuilderComponent::FindAttachmentRecursive(
    AAttachment *Attachment, const EAttachmentCategory TargetCategory,
    TSet<AAttachment *> &Visited) {
  if (!Attachment || Visited.Contains(Attachment))
    return nullptr;
  Visited.Add(Attachment);

  // Check if the current attachment's category matches the target
  if (Attachment->GetAttachmentInfo().Category == TargetCategory) {
    return Attachment;
  }

  // Traverse all children in all links
  for (const FAttachmentLink &Link : Attachment->ChildrenLinks) {
    for (AAttachment *Child : Link.ChildInstances) {
      if (!Child)
        continue;

      // Check if this child matches the category
      if (Child->GetAttachmentInfo().Category == TargetCategory) {
        return Child;
      }

      // Recursively search deeper
      if (AAttachment *Found =
              FindAttachmentRecursive(Child, TargetCategory, Visited)) {
        return Found;
      }
    }
  }

  return nullptr; // Nothing found in this branch
}

FName UWeaponBuilderComponent::GetSocketFromCategory(
    EAttachmentCategory Category) const {
  const UEnum *EnumPtr = StaticEnum<EAttachmentCategory>();
  if (!EnumPtr)
    return NAME_None;

  return FName(EnumPtr->GetNameStringByValue(static_cast<int64>(Category)));
}

void UWeaponBuilderComponent::AddBehaviorComponent(AAttachment *Attachment) {}