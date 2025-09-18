// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Misc/AttachmentSystemTypes.h"
#include "Attachment.generated.h"

UCLASS()
class ATTACHMENTSYSTEM_API AAttachment : public AActor
{
	GENERATED_BODY()

#pragma region Unreal Defaults
public:
	AAttachment();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

private:
#pragma endregion

#pragma region Info
public:
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
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> MeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Graph")
	TArray<FAttachmentLink> ChildrenLinks;

	// Edges: all attachments connected to this

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
