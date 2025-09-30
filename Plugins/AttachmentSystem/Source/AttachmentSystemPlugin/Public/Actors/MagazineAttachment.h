#pragma once

#include "CoreMinimal.h"
#include "Attachment.h"
#include "ThirdParty/RingBuffer.h"
#include "MagazineAttachment.generated.h"

UCLASS()
class ATTACHMENTSYSTEMPLUGIN_API AMagazineAttachment : public AAttachment
{
    GENERATED_BODY()

public:
    AMagazineAttachment();

    /** Add a bullet of the given type to the magazine (returns true if added). */
    UFUNCTION(BlueprintCallable, Category="Magazine")
    bool AddBullet(EBulletType BulletType);

    /** Removes one bullet from the magazine and returns its type. */
    UFUNCTION(BlueprintCallable, Category="Magazine")
    EBulletType RemoveBullet();

    /** Gets the current number of bullets in the magazine. */
    UFUNCTION(BlueprintCallable, Category="Magazine")
    int32 GetAmmoCount() const;

    /** Gets the maximum capacity of the magazine. */
    UFUNCTION(BlueprintCallable, Category="Magazine")
    int32 GetCapacity() const { return MagazineCapacity; }

    /** Checks if the magazine is empty. */
    UFUNCTION(BlueprintCallable, Category="Magazine")
    bool IsEmpty() const;

    /** Utility: convert bullet type to string for logs. */
    static FString BulletTypeToString(EBulletType Type);

protected:
    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, CallInEditor, Category="Magazine|Debug")
    static void RunPerformanceTest();

private:
    /** Capacity of the magazine (compile-time constant). */
    static constexpr int32 MagazineCapacity = 30;

    /** Internal buffer storing bullet types. */
    Lomont::RingBuffer<MagazineCapacity, EBulletType, int32> BulletBuffer;
    
    /** Utility: log buffer contents without destroying data. */
    void LogBufferNonDestructive(const FString& Context);

    /** Helper: enqueue N bullets of same type. */
    void EnqueueNTimes(EBulletType Type, int32 Count);

    /** Helper: dequeue N bullets and log results. */
    void DequeueNTimes(int32 Count);
};