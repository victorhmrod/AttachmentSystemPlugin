// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Attachment.h"
#include "BarrelAttachment.generated.h"

UCLASS()
class ATTACHMENTSYSTEMPLUGIN_API ABarrelAttachment : public AAttachment
{
	GENERATED_BODY()

public:
	ABarrelAttachment();

	/** Set chambered round when racking or reloading. */
	UFUNCTION(BlueprintCallable, Category="Barrel|Ammo")
	void SetChamberedRounds(const TArray<EBulletType>& NewRounds);

	UFUNCTION(Server, Reliable)
	void Server_SetChamberedRounds(const TArray<EBulletType>& NewRounds);
	
	UFUNCTION(BlueprintCallable, Category="Barrel|Ammo")
	void ClearChamber();

	UFUNCTION(Server, Reliable)
	void Server_ClearChamber();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Barrel|Ammo")
	TArray<EBulletType> GetChamberedRounds() const { return ChamberedRounds.value_or(TArray<EBulletType>{}); }

	/** Quick check if chamber is occupied. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Barrel|Ammo")
	bool HasRoundChambered() const { return ChamberedRounds && ChamberedRounds->Num() > 0; }

protected:
	/** Replicated chambered rounds (empty if none). */
	UPROPERTY(ReplicatedUsing=OnRep_ChamberedRounds)
	TArray<EBulletType> ReplicatedChamberedRounds;

	/** Local optional array of chambered rounds (e.g. shotgun pellets). */
	std::optional<TArray<EBulletType>> ChamberedRounds;

	UFUNCTION()
	void OnRep_ChamberedRounds();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;	
};