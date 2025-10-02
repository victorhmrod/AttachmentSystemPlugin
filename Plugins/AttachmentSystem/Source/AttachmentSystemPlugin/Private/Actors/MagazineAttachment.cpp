#include "Actors/MagazineAttachment.h"
#include "Engine/Engine.h" // GEngine
#include "Misc/AttachmentSystemTypes.h" // for LogAttachmentSystem + enums
#include "Net/UnrealNetwork.h"

/* =============================
 * Lifecycle
 * ============================= */

AMagazineAttachment::AMagazineAttachment()
{
    PrimaryActorTick.bCanEverTick = false;
}
void AMagazineAttachment::GetLifetimeReplicatedProps(
    TArray<class FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ThisClass, ReplicatedAmmoCount);
}

void AMagazineAttachment::BeginPlay()
{
    Super::BeginPlay();
}

void AMagazineAttachment::RunPerformanceTest()
{
    constexpr int32 TestCount = 1'000'000;
    EBulletType Temp;

    UE_LOG(LogAttachmentSystem, Warning, TEXT("Performance Test: %d bullets"), TestCount);

    {
        Lomont::RingBuffer<TestCount, EBulletType, int32> PerfBuffer;

        const double Start = FPlatformTime::Seconds();

        // Insert
        for (int32 i = 0; i < TestCount; i++)
        {
            PerfBuffer.Put(EBulletType::Standard_FMJ);
        }

        // Remove
        for (int32 i = 0; i < TestCount; i++)
        {
            PerfBuffer.Get(Temp);
        }

        const double End = FPlatformTime::Seconds();
        const double DurationMs = (End - Start) * 1000.0;

        UE_LOG(LogAttachmentSystem, Warning, TEXT("RingBuffer → %d insertions + removals in %.3f ms"), TestCount, DurationMs);
    }

    {
        TQueue<EBulletType> PerfQueue;

        const double Start = FPlatformTime::Seconds();

        for (int32 i = 0; i < TestCount; i++)
        {
            PerfQueue.Enqueue(EBulletType::Standard_FMJ);
        }

        for (int32 i = 0; i < TestCount; i++)
        {
            PerfQueue.Dequeue(Temp);
        }

        const double End = FPlatformTime::Seconds();
        const double DurationMs = (End - Start) * 1000.0;

        UE_LOG(LogAttachmentSystem, Warning, TEXT("TQueue → %d insertions + removals in %.3f ms"),
               TestCount, DurationMs);
    }

    {
        TArray<EBulletType> PerfArray;

        const double Start = FPlatformTime::Seconds();

        // Insert (push)
        for (int32 i = 0; i < TestCount; i++)
        {
            PerfArray.Push(EBulletType::Standard_FMJ);
        }

        // Remove (pop)
        for (int32 i = 0; i < TestCount; i++)
        {
            Temp = PerfArray.Pop(EAllowShrinking::Yes);
        }

        const double End = FPlatformTime::Seconds();
        const double DurationMs = (End - Start) * 1000.0;

        UE_LOG(LogAttachmentSystem, Warning, TEXT("TArray → %d insertions + removals in %.3f ms"),
               TestCount, DurationMs);
    }

    UE_LOG(LogAttachmentSystem, Warning, TEXT("Performance Test End"));
}

/* =============================
 * Public API (Gameplay usage)
 * ============================= */

bool AMagazineAttachment::AddBullet(EBulletType BulletType) {
    if (!HasAuthority())
    {
        Server_AddBullet(BulletType);
        return false;
    }

    if (GetAmmoCount() >= MagazineCapacity)
    {
        UE_LOG(LogAttachmentSystem, Warning, TEXT("Magazine %s FULL! Capacity=%d"),
               *GetName(), MagazineCapacity);
        return false;
    }

    if (BulletBuffer.Put(BulletType))
    {
        UE_LOG(LogAttachmentSystem, Warning, TEXT("Magazine %s added bullet: %s (AmmoCount=%d)"),
               *GetName(), *BulletTypeToString(BulletType), GetAmmoCount());
        // Update replicate if using
        ReplicatedAmmoCount = GetAmmoCount();
        return true;
    }
    return false;
}

void AMagazineAttachment::Server_AddBullet_Implementation(EBulletType BulletType) {
    if (!HasAuthority()) return;
    AddBullet(BulletType);
}

EBulletType AMagazineAttachment::RemoveBullet()
{
    if (!HasAuthority())
    {
        Server_RemoveBullet();
        return EBulletType::None;
    }

    EBulletType Out{};
    if (BulletBuffer.Get(Out))
    {
        UE_LOG(LogAttachmentSystem, Warning, TEXT("Magazine %s removed bullet: %s (AmmoCount=%d)"),
               *GetName(), *BulletTypeToString(Out), GetAmmoCount());
        ReplicatedAmmoCount = GetAmmoCount();
        return Out;
    }

    UE_LOG(LogAttachmentSystem, Warning, TEXT("Magazine %s is EMPTY!"), *GetName());
    return EBulletType::None;
}

void AMagazineAttachment::Server_RemoveBullet_Implementation()
{
    if (!HasAuthority()) return;
    RemoveBullet();
}

int32 AMagazineAttachment::GetAmmoCount() const
{
    return static_cast<int32>(BulletBuffer.AvailableToRead());
}

bool AMagazineAttachment::IsEmpty() const
{
    return BulletBuffer.IsEmpty();
}


/* =============================
 * Debug / Test Helpers
 * ============================= */

FString AMagazineAttachment::BulletTypeToString(EBulletType Type)
{
    switch (Type)
    {
    case EBulletType::None:           return TEXT("None (Empty)");
    case EBulletType::Standard_FMJ:   return TEXT("Standard FMJ (Ball)");
    case EBulletType::ArmorPiercing:  return TEXT("Armor Piercing (Black Tip)");
    case EBulletType::HollowPoint_SP: return TEXT("Hollow/Soft Point");
    case EBulletType::Tracer:         return TEXT("Tracer (Green Tip)");
    case EBulletType::Subsonic:       return TEXT("Subsonic");
    case EBulletType::Hunting_JSP:    return TEXT("Jacketed Soft Point");
    default:                          return TEXT("Unknown");
    }
}

void AMagazineAttachment::RemoveBullets(int32 n)
{
    if (!HasAuthority())
    {
        Server_RemoveBullets(n);
        return;
    }

    if (n < 0) return;

    EBulletType Temp{};
    if (n > 0 && BulletBuffer.Get(&Temp, n)) // pass pointer to Temp
    {
        UE_LOG(LogAttachmentSystem, Warning,
            TEXT("Magazine %s fully emptied (%d rounds removed)."),
            *GetName(), n);
    }
    else if (BulletBuffer.IsEmpty())
    {
        UE_LOG(LogAttachmentSystem, Warning,
            TEXT("Magazine %s already EMPTY."), *GetName());
    }

    ReplicatedAmmoCount = GetAmmoCount(); // maintains synchronized state
}

void AMagazineAttachment::Server_RemoveBullets_Implementation(int32 n)
{
    if (!HasAuthority()) return;
    RemoveBullets(n);
}

void AMagazineAttachment::OnRep_AmmoCount()
{
    UE_LOG(LogAttachmentSystem, Log, TEXT("Magazine %s ammo synced: %d"),
           *GetName(), ReplicatedAmmoCount);
}

void AMagazineAttachment::LogBufferNonDestructive(const FString& Context)
{
    UE_LOG(LogAttachmentSystem, Warning, TEXT("--- %s ---"), *Context);

    TArray<EBulletType> Temp;
    Temp.Reserve(BulletBuffer.AvailableToRead());

    // Drain buffer into temp
    EBulletType Item{};
    while (BulletBuffer.Get(Item))
    {
        Temp.Add(Item);
    }

    if (Temp.Num() == 0)
    {
        UE_LOG(LogAttachmentSystem, Warning, TEXT("Buffer is EMPTY"));
    }
    else
    {
        for (int32 i = 0; i < Temp.Num(); i++)
        {
            UE_LOG(LogAttachmentSystem, Warning, TEXT("[%02d] %s"), i, *BulletTypeToString(Temp[i]));
        }
    }

    // Restore buffer
    for (const EBulletType& Val : Temp)
    {
        BulletBuffer.Put(Val);
    }
}

void AMagazineAttachment::EnqueueNTimes(EBulletType Type, int32 Count)
{
    for (int32 i = 0; i < Count; i++)
    {
        if (!BulletBuffer.Put(Type))
        {
            UE_LOG(LogAttachmentSystem, Warning, TEXT("Buffer FULL at i=%d while adding %s"), i, *BulletTypeToString(Type));
            break;
        }
    }
}

void AMagazineAttachment::DequeueNTimes(int32 Count)
{
    for (int32 i = 0; i < Count; i++)
    {
        EBulletType Out{};
        if (BulletBuffer.Get(Out))
        {
            UE_LOG(LogAttachmentSystem, Warning, TEXT("Removed: %s"), *BulletTypeToString(Out));
        }
        else
        {
            UE_LOG(LogAttachmentSystem, Warning, TEXT("Tried to remove but buffer is EMPTY at i=%d"), i);
            break;
        }
    }
}