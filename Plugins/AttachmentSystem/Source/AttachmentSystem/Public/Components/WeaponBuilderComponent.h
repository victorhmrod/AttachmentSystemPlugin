#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WeaponBuilderComponent.generated.h"

class AWeapon;
class AAttachment;

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
	AAttachment* GetAttachmentAtSocket(FName SocketName);

	/** Adds extra behavior based on the attachment type */
	void AddBehaviorComponent(AAttachment* Attachment);

protected:
	UPROPERTY(Replicated)
	TObjectPtr<AWeapon> Weapom;
	
	UPROPERTY(EditAnywhere, Category = "Weapon|Parts")
	TArray<AAttachment*> BaseAttachments;
	
	UPROPERTY()
	TMap<AAttachment*, USceneComponent*> SpawnedMeshes;

	UPROPERTY()
	TMap<AAttachment*, UActorComponent*> SpawnedBehaviors;

private:
	/** Recursive function that navigates the graph and assembles the attachments */
	void BuildWeaponFromAttachmentGraph(AAttachment* Attachment, const AAttachment* ParentAttachment, TSet<AAttachment*>& Visited);

	/** Recursive function for search */
	AAttachment* FindAttachmentRecursive(AAttachment* Attachment, AAttachment* Parent, FName SocketName);

	/** Recursive function for cleaning */
	void ClearAttachmentRecursive(AAttachment* Attachment, AAttachment* Parent);

	/** Replication */
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
};
