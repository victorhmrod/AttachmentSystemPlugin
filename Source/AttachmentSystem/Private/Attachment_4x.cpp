// Fill out your copyright notice in the Description page of Project Settings.

#include "Attachment_4x.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Kismet/KismetRenderingLibrary.h"

AAttachment_4x::AAttachment_4x() {
  RenderTarget2D =
      CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("RenderTarget2D"));
  RenderTarget2D->SetupAttachment(AttachmentMesh);
}

void AAttachment_4x::BeginPlay() {
  Super::BeginPlay();

  DotMaterial = AttachmentMesh->CreateDynamicMaterialInstance(1);

  RenderTarget2D->TextureTarget =
      UKismetRenderingLibrary::CreateRenderTarget2D(GetWorld(), 512, 512);
  DotMaterial->SetTextureParameterValue(FName("RenderTarget"),
                                        RenderTarget2D->TextureTarget);
}

void AAttachment_4x::DoAction() {
  Super::DoAction();

  ArrayIndex += 1;

  if (ArrayIndex == DotTextures.Num())
    ArrayIndex = 0;

  DotMaterial->SetTextureParameterValue(FName("DotTexture"),
                                        DotTextures[ArrayIndex]);
}
