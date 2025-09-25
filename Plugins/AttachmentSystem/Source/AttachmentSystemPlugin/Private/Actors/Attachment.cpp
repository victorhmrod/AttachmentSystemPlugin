#include "Actors/Attachment.h"

void AAttachment::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	BuildAttachment();
}

AAttachment::AAttachment()
{
	PrimaryActorTick.bCanEverTick = true;

	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(FName{TEXTVIEW("MeshComponent")});
	SetRootComponent(MeshComponent);
}

void AAttachment::BeginPlay()
{
	Super::BeginPlay();
}

void AAttachment::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);
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

	AttachmentInfo = *FoundRow;
	UE_LOG(LogTemp, Log, TEXT("Attachment '%s' build successfully."), *ID.ToString());

	MeshComponent->SetSkeletalMeshAsset(AttachmentInfo.Mesh.LoadSynchronous());

	// Enable query-only overlaps on a common channel (WorldDynamic)
	if (MeshComponent)
	{
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		MeshComponent->SetCollisionObjectType(ECC_WorldDynamic);
		MeshComponent->SetGenerateOverlapEvents(true);            // not required for OverlapMulti*, but helpful

		MeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
		MeshComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap); // attachments see each other
	}
}
