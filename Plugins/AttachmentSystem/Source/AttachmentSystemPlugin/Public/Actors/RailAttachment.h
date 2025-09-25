// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Attachment.h"
#include "RailAttachment.generated.h"

class USplineComponent;

UCLASS()
class ATTACHMENTSYSTEMPLUGIN_API ARailAttachment : public AAttachment
{
	GENERATED_BODY()

public:
	ARailAttachment();

	/** Number of slots (bumps) in this rail */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rail")
	int32 NumSlots = 15;

	/** Distance between slots (in cm, adjust for your Picatinny scale) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rail")
	float SlotSpacing = 2.54f; // ~1 inch by default

	/** Spline that defines the rail geometry */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Rail")
	TObjectPtr<USplineComponent> RailSpline;

	/** Occupancy bitmask */
	uint64 OccupancyMask = 0;

	/** Set of attachments mounted to this rail */
	UPROPERTY()
	TSet<AAttachment*> MountedAttachments;

	/** Checks if an attachment fits inside rail limits + occupancy */
	UFUNCTION(BlueprintCallable, Category="Rail")
	bool CanPlaceAttachment(AAttachment* Attachment) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Rail")
	float GetSplineLength() const;

	UFUNCTION(BlueprintCallable, Category="Rail")
	int32 GetSlotFromDistance(float Distance) const;

	/** Places attachment at its StartPosition (updates mask + attaches to spline) */
	UFUNCTION(BlueprintCallable, Category="Rail")
	bool PlaceAttachment(AAttachment* Attachment);

	/** Removes attachment from this rail */
	UFUNCTION(BlueprintCallable, Category="Rail")
	void RemoveAttachment(AAttachment* Attachment);

	/** Returns world transform of a given slot index along the spline */
	FTransform GetSlotTransform(int32 SlotIndex) const;

private:
	// Utility to build a mask for the given size and start slot
	uint64 MakeMask(int32 StartSlot, int32 ItemSize) const; // was: Size
};