// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Attachment.h"
#include "CoreMinimal.h"
#include "RailAttachment.generated.h"

class USplineComponent;

/**
 * @brief Specialized attachment that represents a rail (e.g., Picatinny,
 * M-LOK).
 *
 * - Provides slot-based placement (using spline + occupancy mask).
 * - Manages which attachments are mounted along the rail.
 */
UCLASS()
class ATTACHMENTSYSTEMPLUGIN_API ARailAttachment : public AAttachment {
  GENERATED_BODY()

public:
  ARailAttachment();

  /* =============================
   * Rail Configuration
   * ============================= */

  /** Total number of slots (bumps) available on this rail. */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rail")
  int32 NumSlots = 15;

  /** Distance between slots (cm). Defaults to ~1 inch (2.54 cm). */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rail")
  float SlotSpacing = 2.54f;

  /** Spline that defines the physical rail geometry in 3D space. */
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rail")
  TObjectPtr<USplineComponent> RailSpline;

  /* =============================
   * Runtime State
   * ============================= */

  /** Bitmask representing slot occupancy (1 = taken, 0 = free). */
  uint64 OccupancyMask = 0;

  /** Set of all attachments currently mounted to this rail. */
  UPROPERTY()
  TSet<AAttachment *> MountedAttachments;

  /* =============================
   * Rail Operations
   * ============================= */

  /**
   * Checks if an attachment can fit within rail limits and current occupancy.
   *
   * @param Attachment  The attachment to test.
   * @return true if placement is valid, false otherwise.
   */
  UFUNCTION(BlueprintCallable, Category = "Rail")
  bool CanPlaceAttachment(AAttachment *Attachment) const;

  /** @return Total spline length of this rail (in world units). */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Rail")
  float GetSplineLength() const;

  /**
   * Converts a world distance along the spline into a slot index.
   *
   * @param Distance  Distance along spline.
   * @return          Closest slot index at that distance.
   */
  UFUNCTION(BlueprintCallable, Category = "Rail")
  int32 GetSlotFromDistance(float Distance) const;

  /**
   * Places an attachment at its StartPosition (updates occupancy + attaches to
   * spline).
   *
   * @param Attachment  Attachment to place.
   * @return true if successfully placed, false if blocked or invalid.
   */
  UFUNCTION(BlueprintCallable, Category = "Rail")
  bool PlaceAttachment(AAttachment *Attachment);

  UFUNCTION(Server, Reliable)
  void Server_PlaceAttachment(AAttachment *Attachment);

  /**
   * Removes a previously mounted attachment from this rail.
   * Updates occupancy mask and detaches from spline.
   */
  UFUNCTION(BlueprintCallable, Category = "Rail")
  void RemoveAttachment(AAttachment *Attachment);

  UFUNCTION(Server, Reliable)
  void Server_RemoveAttachment(AAttachment *Attachment);

  /**
   * Gets the world transform of a given slot index along the spline.
   *
   * @param SlotIndex  Slot index (0..NumSlots-1).
   * @return           World transform at that slot.
   */
  FTransform GetSlotTransform(int32 SlotIndex) const;

private:
  /**
   * Utility to generate a bitmask for a contiguous slot range.
   *
   * @param StartSlot  Index of first slot.
   * @param ItemSize   Number of slots the item occupies.
   * @return           Bitmask with those slots marked as occupied.
   */
  uint64 MakeMask(int32 StartSlot, int32 ItemSize) const;
};