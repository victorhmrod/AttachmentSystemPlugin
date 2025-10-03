#include "Actors/Attachment.h"

void AAttachment::PostInitializeComponents() {
  Super::PostInitializeComponents();

  // Load DataTable definition after components are ready
  LoadAttachmentInfo();
}

AAttachment::AAttachment() {
  PrimaryActorTick.bCanEverTick = true;

  // Create skeletal mesh component for the attachment
  MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(
      FName{TEXTVIEW("MeshComponent")});

  // Set mesh as root component
  SetRootComponent(MeshComponent);
}

void AAttachment::BeginPlay() {
  Super::BeginPlay();
  // Runtime initialization or event bindings go here
}

void AAttachment::Tick(const float DeltaTime) {
  Super::Tick(DeltaTime);
  // Per-frame logic (not needed for static attachments)
}

void AAttachment::LoadAttachmentInfo() {
  // Validate DataTable reference
  if (!IsValid(AttachmentDataTable)) {
    UE_LOG(LogTemp, Warning,
           TEXT("AttachmentDataTable is invalid for Attachment %s"),
           *GetName());
    return;
  }

  const FString ContextString(TEXT("Finding Row in Attachments Data Table"));

  // Try to fetch row by ID
  const FAttachmentInfo *FoundRow =
      AttachmentDataTable->FindRow<FAttachmentInfo>(ID, ContextString);
  if (!FoundRow) {
    UE_LOG(LogTemp, Warning, TEXT("Attachment ID '%s' not found in DataTable!"),
           *ID.ToString());
    return;
  }

  // Store DataTable row into local struct
  AttachmentInfo = *FoundRow;
  UE_LOG(LogTemp, Log, TEXT("Attachment '%s' built successfully."),
         *ID.ToString());

  // Apply mesh from DataTable definition
  MeshComponent->SetSkeletalMeshAsset(AttachmentInfo.Mesh.LoadSynchronous());

  // Configure collision to allow overlap detection
  if (MeshComponent) {
    MeshComponent->SetCollisionEnabled(
        ECollisionEnabled::QueryOnly); // No physics, only queries
    MeshComponent->SetCollisionObjectType(
        ECC_WorldDynamic); // Mark as dynamic world object
    MeshComponent->SetGenerateOverlapEvents(true); // Enable overlap events

    MeshComponent->SetCollisionResponseToAllChannels(
        ECR_Ignore); // Ignore everything by default
    MeshComponent->SetCollisionResponseToChannel(
        ECC_WorldDynamic, ECR_Overlap); // Only overlap with other attachments
  }

  // Initialize runtime durability with static value from DataTable
  AttachmentCurrentState.Durability = AttachmentInfo.Durability;
}