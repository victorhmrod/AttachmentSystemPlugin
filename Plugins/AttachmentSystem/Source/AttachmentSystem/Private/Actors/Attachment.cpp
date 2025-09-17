#include "Actors/Attachment.h"

AAttachment::AAttachment()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create the root mesh component;
	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(FName{TEXTVIEW("MeshComponent")});

	// Set the mesh component to be the root of this class.
	SetRootComponent(MeshComponent);
}

void AAttachment::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	
}

void AAttachment::BeginPlay()
{
	Super::BeginPlay();
}

void AAttachment::BuildAttachment()
{
	// Check if the DataTable is valid
	if (!IsValid(AttachmentDataTable))
	{
		UE_LOG(LogTemp, Warning, TEXT("AttachmentDataTable is invalid for Attachment %s"), *GetName());
		return;
	}

	const FString ContextString(TEXT("Finding Row in Attachments Data Table"));

	// Search for the row in the DataTable by ID
	const FAttachmentInfo* FoundRow = AttachmentDataTable->FindRow<FAttachmentInfo>(ID, ContextString);
	if (!FoundRow)
	{
		UE_LOG(LogTemp, Warning, TEXT("Attachment ID '%s' not found in DataTable!"), *ID.ToString());
		return;
	}

	// Copy the found struct to the local variable
	AttachmentInfo = *FoundRow;

	// Optional debugging
	UE_LOG(LogTemp, Log, TEXT("Attachment '%s' built successfully."), *ID.ToString());
}

void AAttachment::ConnectTo(AAttachment* Other)
{
	{
		if (!Other || Other == this) return;

		if (!ConnectedAttachments.Contains(Other))
		{
			ConnectedAttachments.Add(Other);
		}

		if (!Other->ConnectedAttachments.Contains(this))
		{
			Other->ConnectedAttachments.Add(this);
		}
	}
}
