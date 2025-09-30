// Weapon.h
// Core weapon actor class for the Attachment System Plugin.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Misc/AttachmentSystemTypes.h"
#include "Weapon.generated.h"

// Forward declarations
class AMagazineAttachment;
class UWeaponBuilderComponent;

/**
 * @brief Core weapon actor class.
 * 
 * Responsibilities:
 * - Serves as the root container for all attachments.
 * - Contains a WeaponBuilderComponent that assembles attachments from DataTables.
 * - Provides durability calculation and ammo handling (via magazine).
 * - Can be extended for different weapon types (rifles, pistols, etc.).
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponFire);

UCLASS()
class ATTACHMENTSYSTEMPLUGIN_API AWeapon : public AActor
{
    GENERATED_BODY()

public:
    /* =============================
     * Lifecycle
     * ============================= */
    
    /** Default constructor. */
    AWeapon();

    /** Called every frame if ticking is enabled. */
    virtual void Tick(float DeltaTime) override;

protected:
    /** Called when the weapon is spawned or the game starts. */
    virtual void BeginPlay() override;


    /* =============================
     * Components
     * ============================= */

    /** Root scene component of the weapon actor. 
     *  Provides the base transform for all attachments.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Components")
    TObjectPtr<USceneComponent> Root;

    /** Weapon builder component responsible for spawning and managing attachments. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Components")
    TObjectPtr<UWeaponBuilderComponent> WeaponBuilderComponent;


public:
    /* =============================
     * Weapon Data
     * ============================= */

    /** Called when attachments are built by the WeaponBuilderComponent. */
    UFUNCTION()
    void HandleWeaponBuilt(const TArray<AAttachment*>& SpawnedAttachments);

    /** Weapon configuration (static info, may come from DataTable). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon|Config")
    FWeaponInfo WeaponInfo;

    /** Runtime state (durability, active attachments, dynamic values). */
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Weapon|Stats", meta=(AllowPrivateAccess="true"))
    FWeaponCurrentState WeaponCurrentState;

    /** Calculates weapon durability (uses WeaponInfo.DefaultDurabilityMode if not specified). */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="Weapon|Stats")
    float GetWeaponDurability(EWeaponDurabilityMode Mode = EWeaponDurabilityMode::Average);

    /** Attempts to fire the weapon. Returns the bullet type used (or None if no shot fired). */
    UFUNCTION(BlueprintCallable, Category="Weapon|Ammo")
    EBulletType FireWeapon();

    /* =============================
    * Weapon Stats
    * ============================= */
    
    /** Modify weapon durability by a delta (negative to decrease). Clamped [0..100]. */
    UFUNCTION(BlueprintCallable, Category="Weapon|Stats")
    void ModifyDurability(float Delta);

    /* =============================
     * Ammo / Magazine
     * ============================= */

    /** Current magazine attached to the weapon. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon|Ammo")
    AMagazineAttachment* CurrentMagazine;

    /** Fired whenever the weapon shoots successfully (after ConsumeBullet). */
    UPROPERTY(BlueprintAssignable, Category="Weapon|Ammo")
    FOnWeaponFire OnWeaponFired;

    /** Returns current number of bullets in the magazine. */
    UFUNCTION(BlueprintCallable, Category="Weapon|Ammo")
    int32 GetAmmoCount() const;

    /** Returns true if weapon has ammo available in the magazine. */
    UFUNCTION(BlueprintCallable, Category="Weapon|Ammo")
    bool HasAmmo() const;

    /** Removes one bullet from the magazine and returns its type. */
    UFUNCTION(BlueprintCallable, Category="Weapon|Ammo")
    EBulletType ConsumeBullet() const;

    /** Replaces the current magazine with a new one. */
    UFUNCTION(BlueprintCallable, Category="Weapon|Ammo")
    void ReloadMagazine(AMagazineAttachment* NewMag);

    /** Returns true if the bullet type is valid (not None). */
    UFUNCTION(BlueprintCallable, Category="Weapon|Ammo")
    static bool IsValidBullet(EBulletType BulletType);


    /* =============================
     * Getters
     * ============================= */

    /** Getter for the root component (base transform of the weapon). */
    UFUNCTION(BlueprintCallable, Category="Weapon")
    FORCEINLINE USceneComponent* GetRoot() const { return Root; }

    /** Getter for the current magazine. */
    UFUNCTION(BlueprintCallable, Category="Weapon")
    FORCEINLINE AMagazineAttachment* GetCurrentMagazine() const { return CurrentMagazine; }

    /** Getter for the WeaponBuilderComponent (handles attachment assembly). */
    UFUNCTION(BlueprintCallable, Category="Weapon")
    FORCEINLINE UWeaponBuilderComponent* GetWeaponBuilder() const { return WeaponBuilderComponent; }
};