// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Misc/AttachmentSystemTypes.h"
#include "Attachment.generated.h"

UCLASS()
class ATTACHMENTSYSTEM_API AAttachment : public AActor
{
	GENERATED_BODY()

public:
	AAttachment();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	FName ID;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	FAttachmentInfo AttachmentInfo;
};
