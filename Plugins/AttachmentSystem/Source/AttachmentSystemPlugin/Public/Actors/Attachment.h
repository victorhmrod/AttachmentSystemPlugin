// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Misc/AttachmentSystemTypes.h"
#include "Attachment.generated.h"

class UBoxComponent;

/**
 * @brief Base class for all weapon attachments.
 *
 * - Represents modular parts (scopes, stocks, barrels, etc.).
 * - Handles DataTable loading, mesh setup, and runtime state like durability.
 * - Provides getters for UI/tooltips and stat modifiers.
 */
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

	/** Called after all components have been initialized. */
	virtual void PostInitializeComponents() override;

	/** Called every frame if ticking is enabled. */
	virtual void Tick(float DeltaTime) override;

protected:

	/** Called when the game starts or when this actor is spawned. */
	virtual void BeginPlay() override;

public:

	/* =============================
	 * Visual Representation
	 * ============================= */

	/** Mesh component representing this attachment visually. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attachment")
	TObjectPtr<USkeletalMeshComponent> MeshComponent;


	/* =============================
	 * Graph / Hierarchy
	 * ============================= */

	/** Links to child attachments in the hierarchy. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attachment")
	TArray<FAttachmentLink> ChildrenLinks;

	/** Starting index in graph traversal (e.g., rail offset). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attachment")
	int32 StartPosition = 0;

	/** Number of slots this attachment occupies on a rail. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attachment")
	int32 Size = 1;


	/* =============================
	 * Identity / Data
	 * ============================= */

	/** Unique ID used for DataTable lookup and inventory reference. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attachment", meta=(AllowPrivateAccess="true", ExposeOnSpawn="true"))
	FName ID;

	/** Static definition loaded from DataTable (mesh, stats, description). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attachment", meta=(AllowPrivateAccess="true"))
	FAttachmentInfo AttachmentInfo;

	/** Runtime state (e.g., current durability, dynamic values). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attachment", meta=(AllowPrivateAccess="true"))
	FAttachmentCurrentState AttachmentCurrentState;

	/** DataTable containing attachment definitions. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attachment")
	TObjectPtr<UDataTable> AttachmentDataTable;

	/** Load and apply DataTable info into this attachment (mesh, stats, etc.). */
	void LoadAttachmentInfo();


	/* =============================
	 * Getters
	 * ============================= */

	/** Returns this attachment's mesh component. */
	UFUNCTION(BlueprintPure, Category="Attachment")
	FORCEINLINE USkeletalMeshComponent* GetMeshComponent() const { return MeshComponent; }

	/** Returns current durability (runtime, not static DataTable value). */
	UFUNCTION(BlueprintPure, BlueprintCallable, Category="Attachment|Stats")
	FORCEINLINE float GetDurability() const { return AttachmentCurrentState.Durability; }

	/** Returns full static definition (from DataTable). */
	UFUNCTION(BlueprintPure, Category="Attachment")
	FORCEINLINE FAttachmentInfo GetAttachmentInfo() const { return AttachmentInfo; }

	/** Returns this attachment's category (optic, stock, barrel, etc.). */
	UFUNCTION(BlueprintPure, Category="Attachment")
	FORCEINLINE EAttachmentCategory GetAttachmentCategory() const { return AttachmentInfo.Category; }

	/** Returns how many slots this attachment occupies. */
	UFUNCTION(BlueprintPure, Category="Attachment")
	FORCEINLINE int32 GetSize() const { return AttachmentInfo.Size; }

	/** Returns the starting slot index. */
	UFUNCTION(BlueprintPure, Category="Attachment")
	FORCEINLINE int32 GetAttachmentStartSlot() const { return AttachmentInfo.StartSlot; }

	/** Returns short display name (used in UI). */
	UFUNCTION(BlueprintPure, Category="Attachment")
	FORCEINLINE FName GetAttachmentDisplayName() const { return AttachmentInfo.Display_Name; }

	/** Returns description text (used in UI/tooltips). */
	UFUNCTION(BlueprintPure, Category="Attachment")
	FORCEINLINE FText GetAttachmentDisplayDescription() const { return AttachmentInfo.Display_Description; }

	/** Returns stat modifiers this attachment applies. */
	UFUNCTION(BlueprintPure, Category="Attachment")
	FORCEINLINE TArray<FStatModifier> GetStatModifiers() const { return AttachmentInfo.Modifiers; }
};