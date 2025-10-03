#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Misc/AttachmentSystemTypes.h"
#include "Weapon.generated.h"

class AMagazineAttachment;
class ABarrelAttachment;
class UWeaponBuilderComponent;

/** Reload stages for staged reload logic. */
UENUM(BlueprintType)
enum class EReloadStage : uint8
{
    None            UMETA(DisplayName="None"),
    RemoveMagazine  UMETA(DisplayName="Remove Magazine"),
    InsertMagazine  UMETA(DisplayName="Insert Magazine"),
    RackHandle      UMETA(DisplayName="Rack Charging Handle")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponFire, const TArray<EBulletType>&, FiredRounds);

UCLASS()
class ATTACHMENTSYSTEMPLUGIN_API AWeapon : public AActor
{
    GENERATED_BODY()

public:
    AWeapon();
    virtual void Tick(float DeltaTime) override;

protected:
    virtual void BeginPlay() override;

    virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

    /** Root component */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Components")
    TObjectPtr<USceneComponent> Root;

    /** Builder that spawns and manages attachments */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Components")
    TObjectPtr<UWeaponBuilderComponent> WeaponBuilderComponent;

public:
    /* =============================
     * Weapon Data
     * ============================= */
    UFUNCTION()
    void HandleWeaponBuilt(const TArray<AAttachment*>& SpawnedAttachments);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon|Config")
    FWeaponInfo WeaponInfo;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Weapon|Stats")
    FWeaponCurrentState WeaponCurrentState;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="Weapon|Stats")
    float GetWeaponDurability(EWeaponDurabilityMode Mode = EWeaponDurabilityMode::Average);

    UFUNCTION(BlueprintCallable, Category="Weapon|Stats")
    void ModifyDurability(float Delta);

    /* =============================
     * Ammo / Attachments
     * ============================= */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon|Ammo")
    AMagazineAttachment* CurrentMagazine = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon|Ammo")
    ABarrelAttachment* CurrentBarrel = nullptr;

    UPROPERTY(BlueprintAssignable, Category="Weapon|Ammo")
    FOnWeaponFire OnWeaponFired;

    /** Debug function to test weapon logic with logs. */
    UFUNCTION(BlueprintCallable, CallInEditor, Category="Weapon|Debug")
    void RunWeaponTest();

    /* =============================
     * Fire
     * ============================= */
    UFUNCTION(BlueprintCallable, Category="Weapon|Ammo")
    TArray<EBulletType> FireWeapon();


    UFUNCTION(BlueprintCallable, Category="Weapon|Ammo")
    TArray<EBulletType> FireFromChamber();

    UFUNCTION(BlueprintCallable, Category="Weapon|Ammo")
    bool TryChamberFromMagazine();
    
    /* =============================
     * Reload
     * ============================= */
    
    UFUNCTION(BlueprintCallable, Category="Weapon|Ammo")
    void ReloadWeapon(AMagazineAttachment* NewMag);
    
    UFUNCTION(BlueprintCallable, Category="Weapon|Ammo")
    void ReloadMagazine(AMagazineAttachment* NewMag);
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon|Reload")
    EReloadStage CurrentReloadStage = EReloadStage::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon|Reload")
    bool bHasMagazineAttached = false;

    UFUNCTION(BlueprintCallable, Category="Weapon|Reload")
    void BeginStagedReload();

    UFUNCTION(BlueprintCallable, Category="Weapon|Reload")
    void ProcessReloadStage(EReloadStage Stage, AMagazineAttachment* NewMag = nullptr);

    UFUNCTION(BlueprintCallable, Category="Weapon|Reload")
    void CancelReload();

    /* =============================
     * Helpers
     * ============================= */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="Weapon|Ammo")
    int32 GetAmmoCount() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="Weapon|Ammo")
    EBulletType GetChamberedRound() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="Weapon|Ammo")
    bool HasRoundChambered() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="Weapon|Ammo")
    FORCEINLINE USceneComponent* GetRoot() const
    {
        return Root;
    }

    UFUNCTION(BlueprintCallable, Category="Weapon|Ammo")
    FORCEINLINE bool HasAmmo() const
    {
        return GetAmmoCount() > 0;
    }
};