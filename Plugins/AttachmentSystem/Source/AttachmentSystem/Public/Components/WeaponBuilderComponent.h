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

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Parts")
	TArray<AAttachment*> BaseAttachments;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Weapon|Builder")
	void BuildWeapon();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Weapon|Builder")
	void ClearWeapon();

	UFUNCTION(BlueprintPure, Category = "Weapon|Builder")
	AAttachment* GetAttachmentAtSocket(FName SocketName);

protected:
	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY()
	TMap<AAttachment*, USceneComponent*> SpawnedMeshes;

	UPROPERTY()
	TMap<AAttachment*, UActorComponent*> SpawnedBehaviors;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Weapon")
	TObjectPtr<AWeapon> Weapom;

private:
	void BuildFromAttachmentData(AAttachment* Attachment, USceneComponent* ParentComponent);

	void AddBehaviorComponent(AAttachment* Attachment);

};
