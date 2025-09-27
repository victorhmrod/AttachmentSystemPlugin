// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Misc/AttachmentSystemTypes.h"
#include "Weapon.generated.h"

class UWeaponBuilderComponent;

/**
 * @brief Core weapon actor class.
 * 
 * - Serves as the root container for all attachments.
 * - Contains a WeaponBuilderComponent that assembles attachments from DataTables.
 * - Can be extended for different weapon types (rifles, pistols, etc.).
 */
UCLASS()
class ATTACHMENTSYSTEMPLUGIN_API AWeapon : public AActor
{
	GENERATED_BODY()

public:
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
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> Root;

	/** Weapon builder component responsible for spawning and managing attachments. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Components")
	TObjectPtr<UWeaponBuilderComponent> WeaponBuilderComponent;


public:
	/* =============================
	* Weapon Data
	* ============================= */

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

	
	/* =============================
	 * Getters
	 * ============================= */

	/** Getter for the root component (base transform of the weapon). */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	FORCEINLINE USceneComponent* GetRoot() const { return Root; }

	/** Getter for the WeaponBuilderComponent (handles attachment assembly). */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	FORCEINLINE UWeaponBuilderComponent* GetWeaponBuilder() const { return WeaponBuilderComponent; }
};