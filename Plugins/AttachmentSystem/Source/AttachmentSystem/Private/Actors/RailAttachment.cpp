#include "Actors/RailAttachment.h"
#include "Components/SplineComponent.h"

ARailAttachment::ARailAttachment()
{
    // Create spline component
    RailSpline = CreateDefaultSubobject<USplineComponent>(TEXT("RailSpline"));
    RailSpline->SetupAttachment(RootComponent);

    NumSlots = 15;
    SlotSpacing = 2.54f; // default spacing
    OccupancyMask = 0;
}

uint64 ARailAttachment::MakeMask(const int32 StartSlot, const int32 ItemSize) const
{
    // Avoid variable name 'Size' to not shadow AAttachment::Size
    if (ItemSize <= 0) return 0ull;
    return ((ItemSize >= 64) ? ~0ull : ((1ULL << ItemSize) - 1ULL)) << StartSlot;
}

bool ARailAttachment::CanPlaceAttachment(AAttachment* Attachment) const
{
    if (!Attachment || NumSlots <= 0) return false;

    const int32 Start    = Attachment->StartPosition;
    const int32 ItemSize = Attachment->Size; // renamed local

    // --- Spline bounds check ---
    if (Start < 0 || Start + ItemSize > NumSlots)
        return false;

    // --- Bitmask occupancy check ---
    const uint64 Mask = MakeMask(Start, ItemSize);
    return (OccupancyMask & Mask) == 0ull;
}

bool ARailAttachment::PlaceAttachment(AAttachment* Attachment)
{
    if (!CanPlaceAttachment(Attachment)) return false;

    const int32 Start    = Attachment->StartPosition;
    const int32 ItemSize = Attachment->Size; // renamed local

    const uint64 Mask = MakeMask(Start, ItemSize);
    OccupancyMask |= Mask;
    MountedAttachments.Add(Attachment);

    // Snap the attachment to the spline slot
    if (RailSpline && Attachment->GetMeshComponent())
    {
        const FTransform SlotTransform = GetSlotTransform(Start);
        Attachment->GetMeshComponent()->SetWorldTransform(SlotTransform);
        Attachment->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
    }

    return true;
}

void ARailAttachment::RemoveAttachment(AAttachment* Attachment)
{
    if (!Attachment || !MountedAttachments.Contains(Attachment)) return;

    const int32 Start    = Attachment->StartPosition;
    const int32 ItemSize = Attachment->Size; // renamed local

    const uint64 Mask = MakeMask(Start, ItemSize);
    OccupancyMask &= ~Mask;

    MountedAttachments.Remove(Attachment);
}

FTransform ARailAttachment::GetSlotTransform(const int32 SlotIndex) const
{
    // If the spline is missing, just return identity
    if (!IsValid(RailSpline))
    {
        return FTransform::Identity;
    }

    // Clamp the index so it never goes beyond NumSlots
    const int32 ClampedIndex = FMath::Clamp(SlotIndex, 0, NumSlots - 1);

    // Distance along the spline = index * spacing
    const float Distance = ClampedIndex * SlotSpacing;

    // Get the world transform at this distance along the spline
    return RailSpline->GetTransformAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
}