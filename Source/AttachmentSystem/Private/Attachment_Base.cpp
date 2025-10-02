// Fill out your copyright notice in the Description page of Project Settings.

#include "Attachment_Base.h"

// Sets default values
AAttachment_Base::AAttachment_Base() {
  PrimaryActorTick.bCanEverTick = true;

  SceneRootPoint =
      CreateDefaultSubobject<USceneComponent>(TEXT("SceneRootPoint"));
  RootComponent = SceneRootPoint;
  //
  RailCollision =
      CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RailCollision"));
  RailCollision->SetupAttachment(SceneRootPoint);
  RailCollision->bHiddenInGame = true;

  // Mesh with collision testing
  AttachmentMesh =
      CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AttachmentMesh"));
  AttachmentMesh->SetupAttachment(SceneRootPoint);
}

void AAttachment_Base::BeginPlay() {
  Super::BeginPlay();

  SavedMaterial = AttachmentMesh->GetMaterial(0);

  if (RailCollision) {
    RailCollision->OnComponentBeginOverlap.AddDynamic(
        this, &AAttachment_Base::OnCollRailBeginOverlap);
    RailCollision->OnComponentEndOverlap.AddDynamic(
        this, &AAttachment_Base::OnCollRailEndOverlap);
  }

  // Delegate handling collision
  if (AttachmentMesh) {
    AttachmentMesh->OnComponentBeginOverlap.AddDynamic(
        this, &AAttachment_Base::OnMeshBeginOverlap);
    AttachmentMesh->OnComponentEndOverlap.AddDynamic(
        this, &AAttachment_Base::OnMeshEndOverlap);
  }
}

void AAttachment_Base::Tick(float DeltaTime) {
  Super::Tick(DeltaTime);

  // bIsColliding ? GEngine->AddOnScreenDebugMessage(-1, 0.f , FColor::Yellow,
  // "Colliding") : GEngine->AddOnScreenDebugMessage(-1, 0.f , FColor::Yellow,
  // "Not Colliding"); bIsPlaced ?GEngine->AddOnScreenDebugMessage(-1, 0.f ,
  // FColor::Yellow, "Placed") : GEngine->AddOnScreenDebugMessage(-1, 0.f ,
  // FColor::Yellow, "Not Placed");
  //  for (UPrimitiveComponent* Actor : CollidingActors)
  //  {
  //  	GEngine->AddOnScreenDebugMessage(-1, 0.f , FColor::Yellow,
  //  *Actor->GetName());
  //  }
}

void AAttachment_Base::OnCollRailBeginOverlap(
    UPrimitiveComponent *OverlappedComponent, AActor *OtherActor,
    UPrimitiveComponent *OtherComp, int32 OtherBodyIndex, bool bFromSweep,
    const FHitResult &SweepResult) {
  if (!bIsPlaced) {
    if (OtherActor != this) {
      bIsCollidingRailing = true;

      CollidingRails.AddUnique(OtherActor);

      if (CollidingRails.Num() == 1) {
        ToggleDeniedMat(true);
      }
    }
  }
}

void AAttachment_Base::OnCollRailEndOverlap(
    UPrimitiveComponent *OverlappedComponent, AActor *OtherActor,
    UPrimitiveComponent *OtherComp, int32 OtherBodyIndex) {
  if (OtherActor != this) {
    CollidingRails.Remove(OtherActor);

    if (CollidingRails.Num() == 0) {
      bIsCollidingRailing = false;
      if (CollidingActors.Num() == 0)
        ToggleDeniedMat(false);
    }
  }
}

void AAttachment_Base::OnMeshBeginOverlap(
    UPrimitiveComponent *OverlappedComponent, AActor *OtherActor,
    UPrimitiveComponent *OtherComp, int32 OtherBodyIndex, bool bFromSweep,
    const FHitResult &SweepResult) {
  if (!bIsPlaced) {
    if (OtherActor != this) {
      bIsCollidingAttachment = true;

      CollidingActors.AddUnique(OtherComp);

      if (CollidingActors.Num() == 1) {
        ToggleDeniedMat(true);
      }
    }
  }
}

void AAttachment_Base::OnMeshEndOverlap(
    UPrimitiveComponent *OverlappedComponent, AActor *OtherActor,
    UPrimitiveComponent *OtherComp, int32 OtherBodyIndex) {
  if (OtherActor != this) {
    CollidingActors.Remove(OtherComp);

    if (CollidingActors.Num() == 0) {
      bIsCollidingAttachment = false;

      if (CollidingRails.Num() == 0)
        ToggleDeniedMat(false);
    }
  }
}

void AAttachment_Base::DoAction() {}

void AAttachment_Base::ToggleDeniedMat(const bool bActivate) {
  bActivate ? AttachmentMesh->SetMaterial(0, DeniedMaterial)
            : AttachmentMesh->SetMaterial(0, SavedMaterial);

  OnMaterialChanged.ExecuteIfBound(bActivate);
}

void AAttachment_Base::OnPlacedEvent() {}
