// Fill out your copyright notice in the Description page of Project Settings.

#include "Attachment_45Degree.h"

AAttachment_45Degree::AAttachment_45Degree() {
  Railing = CreateDefaultSubobject<UChildActorComponent>(TEXT("Railing"));
  Railing->SetupAttachment(AttachmentMesh);
}

void AAttachment_45Degree::BeginPlay() {
  Super::BeginPlay();

  Railing->GetChildActor()->SetActorEnableCollision(false);
}
void AAttachment_45Degree::OnPlacedEvent() {
  Super::OnPlacedEvent();

  Railing->GetChildActor()->SetActorEnableCollision(true);
}
