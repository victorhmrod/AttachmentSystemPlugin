// Fill out your copyright notice in the Description page of Project Settings.


#include "Attachment_Magni.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Kismet/KismetRenderingLibrary.h"

AAttachment_Magni::AAttachment_Magni()
{
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(AttachmentMesh);

	RenderTarget2D = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("RenderTarget2D"));
	RenderTarget2D->SetupAttachment(Mesh);
}

void AAttachment_Magni::BeginPlay()
{
	Super::BeginPlay();

	// Initialize Timeline
	FOnTimelineFloat UpdateMovement;
	UpdateMovement.BindUFunction(this, FName("UpdateMagniMovement"));

	MagniMovement.AddInterpFloat(MovementCurve, UpdateMovement);
	//

	MagniBodyMaterial = Mesh->CreateDynamicMaterialInstance(0);
	MagniLensMaterial = Mesh->CreateDynamicMaterialInstance(1);
	
	RenderTarget2D->TextureTarget = UKismetRenderingLibrary::CreateRenderTarget2D(GetWorld(), 512, 512);
	MagniLensMaterial->SetTextureParameterValue(FName("RenderTarget"), RenderTarget2D->TextureTarget);

	OnMaterialChanged.BindUObject(this, &AAttachment_Magni::SetMagniMat);
	
}

void AAttachment_Magni::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Set Timeline Tick
	MagniMovement.TickTimeline(DeltaTime);
}

void AAttachment_Magni::DoAction()
{
	Super::DoAction();

	// Alternate between On / Off
	if (bFlipFlop)
	{
		bFlipFlop = false;
		MagniMovement.Reverse();
	}
	else
	{
		bFlipFlop = true;
		MagniMovement.Play();
	}
}

void AAttachment_Magni::UpdateMagniMovement(const float Alpha)
{
	Mesh->SetRelativeRotation(FMath::Lerp(OriginalRotation, FinalRotation, Alpha));
}

// Function Bound to parent delegate
void AAttachment_Magni::SetMagniMat(const bool bActivate)
{
	bActivate ? Mesh->SetMaterial(0, DeniedMaterial) : Mesh->SetMaterial(0, MagniBodyMaterial);
}

