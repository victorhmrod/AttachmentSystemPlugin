// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Misc/AttachmentSystemTypes.h"
#include "Attachment.generated.h"

class UBoxComponent;

UCLASS(Blueprintable)
class ATTACHMENTSYSTEMPLUGIN_API AAttachment : public AActor
{
	GENERATED_BODY()

public:

	/** Default constructor */
	AAttachment();

	/* =============================
	 * Unreal Lifecycle Overrides
	 * ============================= */

	/** Called after all components have been initialized. 
	 *  Good place to finalize setup that depends on subcomponents.
	 */
	virtual void PostInitializeComponents() override;

	/** Called every frame if ticking is enabled. 
	 *  Used for per-frame updates (rare in attachments, but available).
	 *
	 * @param DeltaTime  Time elapsed since last frame.
	 */
	virtual void Tick(float DeltaTime) override;

protected:

	/** Called when the game starts or when this actor is spawned. 
	 *  Ideal for initializing runtime state or binding events.
	 */
	virtual void BeginPlay() override;

public:

    /* =============================
     * Visual Representation
     * ============================= */

    /** Skeletal mesh component that visually represents this attachment. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attachment")
    TObjectPtr<USkeletalMeshComponent> MeshComponent;


    /* =============================
     * Graph / Hierarchy
     * ============================= */

    /** Outgoing links to child attachments in the graph. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attachment")
    TArray<FAttachmentLink> ChildrenLinks;

    /** Starting position index in graph traversal (used for rails/layout). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attachment")
    int32 StartPosition = 0;

    /** Logical "size" of this attachment in graph traversal (e.g., slots it occupies). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attachment")
    int32 Size = 1;


    /* =============================
     * Identity / Data
     * ============================= */

    /** Unique identifier for this attachment instance. 
     *  - Exposed on spawn for runtime instancing.
     *  - Used to match rows in DataTables or inventory references.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attachment", meta=(AllowPrivateAccess="true", ExposeOnSpawn="true"))
    FName ID;

    /** Static attachment definition (display name, description, stats, mesh, etc.). 
     *  Usually loaded from a DataTable row.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attachment", meta=(AllowPrivateAccess="true"))
    FAttachmentInfo AttachmentInfo;

    /** DataTable reference that provides attachment definitions (FAttachmentInfo rows). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attachment")
    TObjectPtr<UDataTable> AttachmentDataTable;

    /** 
     * Initializes this attachment using its DataTable definition. 
     * Spawns mesh, applies stats, and prepares runtime state. 
     */
    UFUNCTION(BlueprintCallable, Category="Attachment")
    void BuildAttachment();


    /* =============================
     * Getters
     * ============================= */

    /** Returns the skeletal mesh component that visually represents this attachment. */
    UFUNCTION(BlueprintPure, Category="Attachment")
    FORCEINLINE USkeletalMeshComponent* GetMeshComponent() const { return MeshComponent; }

	// Getter da durabilidade (pega do Info)
	UFUNCTION(BlueprintPure, BlueprintCallable, Category="Attachment|Stats")
	FORCEINLINE float GetDurability() const { return AttachmentInfo.Durability; }

    /** Returns the full attachment info struct (DataTable row). */
    UFUNCTION(BlueprintPure, Category="Attachment")
    FORCEINLINE FAttachmentInfo GetAttachmentInfo() const { return AttachmentInfo; }

    /** Returns this attachment's category (e.g., Optic, Stock, Barrel). */
    UFUNCTION(BlueprintPure, Category="Attachment")
    FORCEINLINE EAttachmentCategory GetAttachmentCategory() const { return AttachmentInfo.Category; }

    /** Returns how many rail slots this attachment occupies. */
    UFUNCTION(BlueprintPure, Category="Attachment")
    FORCEINLINE int32 GetSize() const { return AttachmentInfo.Size; }

    /** Returns the starting rail slot index for this attachment. */
    UFUNCTION(BlueprintPure, Category="Attachment")
    FORCEINLINE int32 GetAttachmentStartSlot() const { return AttachmentInfo.StartSlot; }

    /** Returns the short display name of this attachment (used in UI/tooltips). */
    UFUNCTION(BlueprintPure, Category="Attachment")
    FORCEINLINE FName GetAttachmentDisplayName() const { return AttachmentInfo.Display_Name; }

    /** Returns the full display description of this attachment (used in UI/tooltips). */
    UFUNCTION(BlueprintPure, Category="Attachment")
    FORCEINLINE FText GetAttachmentDisplayDescription() const { return AttachmentInfo.Display_Description; }

    /** Returns the stat modifiers this attachment applies to the weapon. */
    UFUNCTION(BlueprintPure, Category="Attachment")
    FORCEINLINE TArray<FStatModifier> GetStatModifiers() const { return AttachmentInfo.Modifiers; }
};
