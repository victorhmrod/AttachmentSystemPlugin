// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Misc/AttachmentSystemTypes.h"
#include "Attachment.generated.h"

class UBoxComponent;

UCLASS()
class ATTACHMENTSYSTEM_API AAttachment : public AActor
{
	GENERATED_BODY()

#pragma region Unreal Defaults
public:
	virtual void PostInitializeComponents() override;
	
	AAttachment();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

private:
#pragma endregion

#pragma region Info
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 StartPosition = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Size = 1;
	
	UFUNCTION()
	void BuildAttachment();
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info", meta = (AllowPrivateAccess = "true", ExposeOnSpawn = "true"))
	FName ID;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info", meta = (AllowPrivateAccess = "true"))
	FAttachmentInfo AttachmentInfo;

	UPROPERTY(EditDefaultsOnly, Category = "Info")
	TObjectPtr<UDataTable> AttachmentDataTable;

private:
#pragma endregion

#pragma region Components
public:
	UFUNCTION(BlueprintImplementableEvent, BlueprintPure, Category = "Components")
	UBoxComponent* GetBoxComponent();
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> MeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Graph")
	TArray<FAttachmentLink> ChildrenLinks;

	USkeletalMeshComponent* GetMeshComponent() const
	{
		return MeshComponent;
	}

	FAttachmentInfo GetAttachmentInfo() const
	{
		return AttachmentInfo;
	}
	
protected:
	
private:

#pragma endregion 
};
