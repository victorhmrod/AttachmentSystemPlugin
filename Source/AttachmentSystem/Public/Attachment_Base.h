// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "Attachment_Base.generated.h"

DECLARE_DELEGATE_OneParam(FOnMaterialChanged, const bool /* bActivate */)

UCLASS()
class ATTACHMENTSYSTEM_API AAttachment_Base : public AActor
{
	GENERATED_BODY()
	
public:
	//==================================================
	// PROPERTIES & VARIABLES
	//==================================================
	
	UPROPERTY(EditAnywhere, Category = "Mesh")
	TObjectPtr<USceneComponent> SceneRootPoint = nullptr;
	UPROPERTY(EditAnywhere, Category = "Mesh")
	TObjectPtr<UStaticMeshComponent> RailCollision = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Mesh")
	TObjectPtr<UStaticMeshComponent> AttachmentMesh = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Parameters")
	FGameplayTagContainer AttachmentTags;
	
	UPROPERTY(EditAnywhere, Category = "Parameters", meta = (ToolTip = "Radius before snapping take effect"))
	float Radius;

	UPROPERTY(EditAnywhere, Category = "Parameters", meta = (Tooltip = "Length/2 of the base to avoid standing in air"))
	float BaseOffset;

	UPROPERTY(EditAnywhere, Category = "Parameters", meta = (Tooltip = "Length/2 of the base to avoid standing in air"))
	FVector CamPositionOffset;

	UPROPERTY(EditAnywhere, Category = "Parameters")
	TObjectPtr<UMaterialInterface> DeniedMaterial;

	UPROPERTY()
	bool bIsPlaced{false};
	//==================================================
	// FUNCTIONS
	//==================================================
	bool IsCollidingAttachment() const {return bIsCollidingAttachment;}
	bool IsCollidingRailing() const {return bIsCollidingRailing;}
	
	virtual void DoAction();

	void ToggleDeniedMat(const bool bActivate);

	virtual void OnPlacedEvent();
	
protected:
	//==================================================
	// PROPERTIES & VARIABLES
	//==================================================
	UPROPERTY()
	TObjectPtr<UMaterialInterface> SavedMaterial;

	bool bIsCollidingAttachment {false};
	bool bIsCollidingRailing {false};


	FOnMaterialChanged OnMaterialChanged;
	
	//==================================================
	// FUNCTIONS
	//==================================================
	AAttachment_Base();

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;



	
private:
	UFUNCTION()
	virtual void OnCollRailBeginOverlap(UPrimitiveComponent* OverlappedComponent,
									AActor* OtherActor, UPrimitiveComponent* OtherComp,
									int32 OtherBodyIndex, bool bFromSweep,
									const FHitResult& SweepResult);

	UFUNCTION()
	void OnCollRailEndOverlap(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor, UPrimitiveComponent* OtherComp, 
		int32 OtherBodyIndex);
	
	UFUNCTION()
	virtual void OnMeshBeginOverlap(UPrimitiveComponent* OverlappedComponent,
	                                AActor* OtherActor, UPrimitiveComponent* OtherComp,
	                                int32 OtherBodyIndex, bool bFromSweep,
	                                const FHitResult& SweepResult);

	UFUNCTION()
	void OnMeshEndOverlap(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor, UPrimitiveComponent* OtherComp, 
		int32 OtherBodyIndex);


	// Array of colliding railings
	TArray<TObjectPtr<AActor>> CollidingRails;
	
	// Array of colliding Objects
	TArray<TObjectPtr<UPrimitiveComponent>> CollidingActors;
};
