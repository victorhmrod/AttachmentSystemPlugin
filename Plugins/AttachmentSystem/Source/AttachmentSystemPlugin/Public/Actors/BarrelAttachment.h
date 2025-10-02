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
	void SetChamberedRound(EBulletType NewRound);

	/** Clear chamber (after firing). */
	UFUNCTION(BlueprintCallable, Category="Barrel|Ammo")
	void ClearChamber();

	/** Get current chambered round (optional). */
	std::optional<EBulletType> GetChamberedRound() const { return ChamberedRound; }

	/** Quick check if chamber is occupied. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Barrel|Ammo")
	bool HasRoundChambered() const { return ChamberedRound.has_value(); }

protected:
	/** Replicated value of the chambered round (None means no round). */
	UPROPERTY(ReplicatedUsing=OnRep_ChamberedRound)
	EBulletType ReplicatedChamberedRound = EBulletType::None;

	/** Local state (optional, not directly replicated). */
	std::optional<EBulletType> ChamberedRound;

	/** Called when ReplicatedChamberedRound updates from server. */
	UFUNCTION()
	void OnRep_ChamberedRound();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};