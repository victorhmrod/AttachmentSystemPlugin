// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawn_Smith.h"

#include "Attachment_Base.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
APawn_Smith::APawn_Smith()
{
    PrimaryActorTick.bCanEverTick = true;
	//
	SceneComp = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComp"));
	RootComponent = SceneComp;
	
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->bDoCollisionTest = false;
	
	// //
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);

}


void APawn_Smith::BeginPlay()
{
	Super::BeginPlay();
	
	// Get Player Controller + Show Mouse Cursor
	PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	PlayerController->SetShowMouseCursor(true);

	if(PlayerController)
	{
		if(UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	// Spawn First Attachment (Magni)
	CurrentAttachment = GetWorld()->SpawnActor<AAttachment_Base>(FirstAttachmentClass);

	// Initialize Timeline
	FOnTimelineFloat UpdateFocusEvent;
	UpdateFocusEvent.BindUFunction(this, FName("UpdateFocus"));

	FocusTimeline.AddInterpFloat(FocusCurve, UpdateFocusEvent);
	
	OriginLoc = this->GetActorLocation();
}


void APawn_Smith::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Set Timeline Tick
	FocusTimeline.TickTimeline(DeltaTime);

	// Gather Mouse Position in Local & World Space

	PlayerController->GetMousePosition(LocalMouseLocation.X, LocalMouseLocation.Y);
	
	PlayerController->DeprojectMousePositionToWorld(WorldSpaceMouseLocation, WorldSpaceMouseDirection);

	if (CurrentAttachment) CheckForSnapping();

	// Check for matching railings tags to place Attachment, if not matching, set Attachment materials to denied
	if (HitRailing && bIsSnapping && CurrentAttachment && !CurrentAttachment->IsCollidingAttachment() && !CurrentAttachment->IsCollidingRailing())
	{
		bCanAttachmentBePlaced = false;
		CurrentAttachment->ToggleDeniedMat(true);
		bDoOnceMatAttachment = false;
		
		for (FGameplayTag Tag : CurrentAttachment->AttachmentTags)
		{
			if (HitRailing->RailingTags.HasTag(Tag))
			{
				bCanAttachmentBePlaced = true;
				CurrentAttachment->ToggleDeniedMat(false);

				break;
			}
			
		}
	}
	else if (bDoOnceMatAttachment && !CurrentAttachment->IsCollidingAttachment() && !CurrentAttachment->IsCollidingRailing())
	{
		CurrentAttachment->ToggleDeniedMat(false);
		bDoOnceMatAttachment = false;
	}


	if(CurrentAttachment)
	{
		if (CurrentAttachment->IsCollidingRailing())
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, TEXT("Colliding Railing"));
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Green, TEXT("Not Colliding Railing"));
		}
	
		if (CurrentAttachment->IsCollidingAttachment())
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, TEXT("Colliding Attachment"));
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Green, TEXT("Not Colliding Attachment"));
		}
	}
}


void APawn_Smith::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &APawn_Smith::OnInteract);

		EnhancedInputComponent->BindAction(ActivateRotateAction, ETriggerEvent::Started, this, &APawn_Smith::OnPressedCamRotation);
		EnhancedInputComponent->BindAction(ActivateRotateAction, ETriggerEvent::Completed, this, &APawn_Smith::OnReleasedCamRotation);

		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APawn_Smith::OnLook);

		EnhancedInputComponent->BindAction(ScrollAction, ETriggerEvent::Triggered, this, &APawn_Smith::OnScroll);

		EnhancedInputComponent->BindAction(ActionAction, ETriggerEvent::Started, this, &APawn_Smith::OnAction);

		EnhancedInputComponent->BindAction(MirrorAttachmentAction, ETriggerEvent::Started, this, &APawn_Smith::OnMirrorAttachment);
	}

}

#pragma region AttachmentLocation
void APawn_Smith::CheckForSnapping()
{
	FHitResult HitResult;
	TArray<TObjectPtr<AActor>> ActorsToIgnore;
	ActorsToIgnore.Add(this);

	const FVector StartLoc = Camera->GetComponentLocation();
	
	//GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, StartLoc.ToString());
	ValidMousePos = WorldSpaceMouseLocation + (WorldSpaceMouseDirection * SpringArm->TargetArmLength * 2.f);
	//GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Green, ValidMousePos.ToString());

	// Line trace to check possible collision with railings
	UKismetSystemLibrary::SphereTraceSingle(GetWorld(), StartLoc, ValidMousePos, CurrentAttachment->Radius, TraceTypeQuery4,false, ActorsToIgnore,
		EDrawDebugTrace::ForOneFrame, HitResult, true);
	

	// Check if Hit is Detected
	if (HitResult.bBlockingHit)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Cyan, FString("HitObject : %s").Append(*HitResult.GetActor()->GetName()));
		if (!CurrentAttachment->IsCollidingRailing())	
		{
			// Get Hit Railing
			if (HitRailing = Cast<AWeapon_Railing>(HitResult.GetActor()))
			{
				SnappingMousePos = HitResult.Location;
				bIsSnapping = true;
				CursorSnapping();
			}
		}
		else
		{
			FHitResult NewHitResult;

			TArray<TObjectPtr<AActor>> NewActorsToIgnore;
			NewActorsToIgnore.Add(HitResult.GetActor());
			
			UKismetSystemLibrary::SphereTraceSingle(GetWorld(), HitResult.Location, HitResult.Location, 10.f, TraceTypeQuery4,false, NewActorsToIgnore,
			EDrawDebugTrace::ForOneFrame, NewHitResult, true);
			
			if (NewHitResult.bBlockingHit && NewHitResult.GetActor() != HitResult.GetActor())
			{
				if (HitRailing = Cast<AWeapon_Railing>(NewHitResult.GetActor()))
				{
					SnappingMousePos = NewHitResult.Location;
					bIsSnapping = true;

					FVector2D NewMouseLoc;
					if (UGameplayStatics::ProjectWorldToScreen(PlayerController,NewHitResult.ImpactPoint + NewHitResult.GetActor()->GetActorUpVector() * 5.f, NewMouseLoc) && !CurrentAttachment->IsCollidingRailing())
					{
						PlayerController->SetMouseLocation(NewMouseLoc.X, NewMouseLoc.Y);
						CursorSnapping();
					}
				}
			}
			else
			{
				SnappingMousePos = HitResult.Location;
				bIsSnapping = true;
				CursorSnapping();
			}
		}
	}
	// Is not close enough to a rail, keep free roam movement
	else
	{
		bIsSnapping = false;
		CursorRoaming();
	}
}

void APawn_Smith::CursorSnapping()
{
	// Init variables
	FVector Min;
	FVector Max;
	HitRailing->RailMesh->GetLocalBounds(Min, Max);
	
	FVector NewAttachmentLoc = HitRailing->StartPoint->GetComponentLocation();
	const float StartY = HitRailing->StartPoint->GetComponentLocation().Y < HitRailing->EndPoint->GetComponentLocation().Y ? HitRailing->StartPoint->GetComponentLocation().Y : HitRailing->EndPoint->GetComponentLocation().Y;
	const float EndY = HitRailing->StartPoint->GetComponentLocation().Y < HitRailing->EndPoint->GetComponentLocation().Y ? HitRailing->EndPoint->GetComponentLocation().Y : HitRailing->StartPoint->GetComponentLocation().Y;
	//

	// Create Step Movement
	float MouseY = FMath::RoundToInt(SnappingMousePos.Y / 10.f) * 10.f;

	// Get Normalize Value between Rail's Points
	const float tempY = FMath::GetMappedRangeValueClamped(UE::Math::TVector2(StartY, EndY), UE::Math::TVector2(0.f, 1.f), MouseY);

	// Get World Value between Rail's Points
	NewAttachmentLoc.Y = FMath::Lerp(StartY, EndY, tempY);
	
	NewAttachmentLoc.Y -= 2.5f;
	// Apply Final Location
	const float FinalY = FMath::Clamp(EndY - CurrentAttachment->BaseOffset, StartY + CurrentAttachment->BaseOffset, NewAttachmentLoc.Y);
	NewAttachmentLoc.Y = FinalY;

	FRotator NewAttachmentRot = FRotator(HitRailing->GetActorRotation().Pitch, HitRailing->GetActorRotation().Yaw, HitRailing->GetActorRotation().Roll);

	//GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, NewAttachmentRot.ToString());

	//GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString("Rail Vector : ").Append(HitRailing->GetActorUpVector().ToString()));
	//GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString("Default Up Vector : ").Append(FVector::UpVector.ToString()));
	//GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString("DotProduct : ").Append(FString::SanitizeFloat(FVector::DotProduct(HitRailing->GetActorUpVector(), FVector::UpVector))));

	// Force Rotation due to Rotator Limitation if Railing is Upside Down
	if (FMath::IsNearlyEqual(FVector::DotProduct(HitRailing->GetActorUpVector(), FVector::UpVector), -1.f, 0.1f))
	{
		NewAttachmentRot = FRotator(0.f, 180.f, 180.f);
	}


	
	
	AttachmentLocation = NewAttachmentLoc;
	AttachmentRotation = NewAttachmentRot;
	CurrentAttachment->SetActorLocation(NewAttachmentLoc);
	CurrentAttachment->SetActorRotation(NewAttachmentRot);

	//
}

// Set Attachment Location on Cursor
void APawn_Smith::CursorRoaming()
{
	CurrentAttachment->SetActorLocation(ValidMousePos);
}
#pragma endregion 

#pragma region Inputs
void APawn_Smith::OnInteract(const FInputActionValue& Value)
{
	
	//GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, FString("Clicked"));
	
	if (CurrentAttachment && bCanAttachmentBePlaced && bIsSnapping && !CurrentAttachment->IsCollidingAttachment() && !CurrentAttachment->IsCollidingRailing())
	{
		// Spawn New Actor to attach it to Railing
		const TObjectPtr<AAttachment_Base> tempNewAttachment = GetWorld()->SpawnActor<AAttachment_Base>(CurrentAttachment->GetClass());
		tempNewAttachment->SetActorLocationAndRotation(AttachmentLocation, AttachmentRotation);
		tempNewAttachment->SetActorScale3D(CurrentAttachment->GetActorScale3D());
		tempNewAttachment->AttachToComponent(HitRailing->RailMesh, FAttachmentTransformRules::KeepWorldTransform);
		tempNewAttachment->OnPlacedEvent();
		tempNewAttachment->bIsPlaced = true;

		// Delete Unused Reference
		CurrentAttachment->Destroy();
		CurrentAttachment = nullptr;

		// Swap to 4x Attachment
		if (!DoOnceChangeAttachment || true)
		{
			DoOnceChangeAttachment = true;
			
			// Spawn First Attachment (Magni)
			CurrentAttachment = GetWorld()->SpawnActor<AAttachment_Base>(SecondAttachmentClass);
		}
	}
	else if (!CurrentAttachment)
	{
		// Check for an attachment to focus
		FHitResult HitResult;
		TArray<TObjectPtr<AActor>> ActorsToIgnore;
		ActorsToIgnore.Add(this);

		const FVector StartLoc = Camera->GetComponentLocation();
	
		ValidMousePos = WorldSpaceMouseLocation + (WorldSpaceMouseDirection * SpringArm->TargetArmLength * 2.f);

		// Line trace to check possible collision with Attachment
		UKismetSystemLibrary::LineTraceSingle(GetWorld(), StartLoc, ValidMousePos, TraceTypeQuery3,false, ActorsToIgnore,
			EDrawDebugTrace::None, HitResult, true);

		// Check for Hit Result, then move camera to hit attachment, return to origin otherwise
		if (HitResult.bBlockingHit)
		{
			if (FocusedAttachment = Cast<AAttachment_Base>(HitResult.GetActor()))
			{
				FocusStartLoc = this->GetActorLocation();
				FocusEndLoc = FocusedAttachment->GetActorLocation() + FocusedAttachment->CamPositionOffset;
				FocusTimeline.PlayFromStart();
			}
			else
			{
				FocusStartLoc = this->GetActorLocation();
				FocusEndLoc = OriginLoc;
				FocusTimeline.PlayFromStart();
			}
		}
		
	}
	
	
}

void APawn_Smith::OnPressedCamRotation(const FInputActionValue& Value)
{
	bEnableCamRotation = true;

}

void APawn_Smith::OnReleasedCamRotation(const FInputActionValue& Value)
{
	bEnableCamRotation = false;

}

// Move Camera around Origin
void APawn_Smith::OnLook(const FInputActionValue& Value)
{
	if(bEnableCamRotation)
	{
		const FVector2D LookVector = Value.Get<FVector2D>();

		this->AddActorLocalRotation(FRotator(0.f, LookVector.X , 0.f));

		//GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Blue, FRotator(LookVector.Y * 100, LookVector.X * 100, 0.f).ToString());
	}
}

// Change Camera Arm Length
void APawn_Smith::OnScroll(const FInputActionValue& Value)
{
	float input = Value.Get<float>() * 5.f;

	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Blue, FString::SanitizeFloat(input));

	float NewArmLength = SpringArm->TargetArmLength;

	NewArmLength -= input;
	
	SpringArm->TargetArmLength = FMath::Clamp(NewArmLength, 200.f, 500.f);
}

void APawn_Smith::OnAction(const FInputActionValue& Value)
{
	if (FocusedAttachment)
	{
		FocusedAttachment->DoAction();
	}
}

void APawn_Smith::OnMirrorAttachment(const FInputActionValue& Value)
{
	if (CurrentAttachment)
	{
		const FVector CurrentAttachmentScale = CurrentAttachment->GetActorScale3D();
		CurrentAttachment->SetActorScale3D(FVector(CurrentAttachmentScale.X * -1.f, CurrentAttachmentScale.Y, CurrentAttachmentScale.Z));
	}
}
#pragma endregion

// Timeline Update
void APawn_Smith::UpdateFocus(const float Alpha)
{
	this->SetActorLocation(FMath::Lerp(FocusStartLoc, FocusEndLoc, Alpha));
}
