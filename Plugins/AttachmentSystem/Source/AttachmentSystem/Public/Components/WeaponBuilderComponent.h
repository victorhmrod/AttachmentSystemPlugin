#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Misc/AttachmentSystemTypes.h"
#include "WeaponBuilderComponent.generated.h"

class AWeapon;
class AAttachment;

/**
* Component responsible for mounting/dismounting weapons using an attachment graph.
* Each edge of the graph contains the parent's socket information.
*/
UCLASS(meta = (BlueprintSpawnableComponent))
class ATTACHMENTSYSTEM_API UWeaponBuilderComponent : public UActorComponent
{
	GENERATED_BODY()

public:    
	UWeaponBuilderComponent();
	
protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Assembles a weapon from the base list of attachments */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Weapon|Builder")
	void BuildWeapon();

	/** Disassembles the weapon (Detaches all attachments) */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Weapon|Builder")
	void ClearWeapon();

	/** Searches for a specific attachment via socket */
	UFUNCTION(BlueprintPure, Category = "Weapon|Builder")
	AAttachment* GetAttachmentAtSocket(EAttachmentCategory Category);

	FName GetSocketFromCategory(EAttachmentCategory Category) const;

	/** Adds extra behavior based on the attachment type */
	void AddBehaviorComponent(AAttachment* Attachment);

protected:
	UPROPERTY(Replicated)
	TObjectPtr<AWeapon> Weapon;
	
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Parts")
	TArray<TSubclassOf<AAttachment>> BaseAttachments;

	UPROPERTY(Transient)
	TArray<AAttachment*> SpawnedAttachments;
	
	UPROPERTY()
	TMap<AAttachment*, USceneComponent*> SpawnedMeshes;

	UPROPERTY()
	TMap<AAttachment*, UActorComponent*> SpawnedBehaviors;

private:
	/** Recursive function that navigates the graph and assembles the attachments */
	void BuildWeaponFromAttachmentGraph(AAttachment* ParentAttachment, TSet<AAttachment*>& Visited);

	/** Recursive function for search */
	AAttachment* FindAttachmentRecursive(AAttachment* Attachment, EAttachmentCategory TargetCategory, TSet<AAttachment*>& Visited);

	/** Recursive function for cleaning */
	void ClearAttachmentRecursive(AAttachment* Attachment, TSet<AAttachment*>& Visited);

	/** Replication */
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	UStoredAttachmentData* BuildStoredDataFromAttachment(AAttachment* Attachment);
	AAttachment* SpawnAttachmentFromStoredData(UStoredAttachmentData* Data, USkeletalMeshComponent* ParentMesh,
	                                           FName ParentSocket);
};
