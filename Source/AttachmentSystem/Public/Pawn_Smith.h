// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Weapon_Core.h"
#include "Weapon_Railing.h"
#include "Camera/CameraComponent.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/SpringArmComponent.h"
#include "Pawn_Smith.generated.h"

class AAttachment_Base;

UCLASS()
class ATTACHMENTSYSTEM_API APawn_Smith : public APawn {
  GENERATED_BODY()

public:
  //==================================================
  // PROPERTIES & VARIABLES
  //==================================================
  UPROPERTY(EditDefaultsOnly, Category = "Parameters")
  TObjectPtr<UClass> FirstAttachmentClass;

  UPROPERTY(EditDefaultsOnly, Category = "Parameters")
  TObjectPtr<UClass> SecondAttachmentClass;

  UPROPERTY(EditDefaultsOnly, Category = "Parameters")
  TObjectPtr<UCurveFloat> FocusCurve;
  //==================================================
  // FUNCTIONS
  //==================================================

protected:
  //==================================================
  // PROPERTIES & VARIABLES
  //==================================================
  UPROPERTY(EditAnywhere, Category = "Mesh", meta = (AllowPrivateAccess = true))
  TObjectPtr<USceneComponent> SceneComp;

  UPROPERTY(EditAnywhere, Category = "Mesh", meta = (AllowPrivateAccess = true))
  TObjectPtr<UCameraComponent> Camera;

  UPROPERTY(EditAnywhere, Category = "Mesh", meta = (AllowPrivateAccess = true))
  TObjectPtr<USpringArmComponent> SpringArm;

  UPROPERTY(EditAnywhere, Category = "Mesh", meta = (AllowPrivateAccess = true))
  TObjectPtr<UChildActorComponent> Weapon;
  //==================================================
  // FUNCTIONS
  //==================================================
  APawn_Smith();

  virtual void BeginPlay() override;

  virtual void Tick(float DeltaTime) override;

  virtual void SetupPlayerInputComponent(
      class UInputComponent *PlayerInputComponent) override;

  void CheckForSnapping();

  void CursorSnapping();

  void CursorRoaming();

  void OnInteract(const FInputActionValue &Value);

  void OnPressedCamRotation(const FInputActionValue &Value);
  void OnReleasedCamRotation(const FInputActionValue &Value);

  void OnLook(const FInputActionValue &Value);
  void OnScroll(const FInputActionValue &Value);

  void OnAction(const FInputActionValue &Value);

  void OnMirrorAttachment(const FInputActionValue &Value);

private:
  //==================================================
  // PROPERTIES & VARIABLES
  //==================================================
  FVector2D LocalMouseLocation{FVector2D::ZeroVector};
  FVector WorldSpaceMouseLocation{FVector::ZeroVector};
  FVector WorldSpaceMouseDirection{FVector::ZeroVector};

  FVector ValidMousePos;
  FVector SnappingMousePos;
  FVector AttachmentLocation;
  FRotator AttachmentRotation;

  UPROPERTY()
  TObjectPtr<AAttachment_Base> CurrentAttachment{nullptr};
  UPROPERTY()
  TObjectPtr<AAttachment_Base> FocusedAttachment{nullptr};
  UPROPERTY()
  TObjectPtr<AWeapon_Railing> HitRailing{nullptr};

  UPROPERTY()
  TObjectPtr<APlayerController> PlayerController{nullptr};

  UPROPERTY(EditDefaultsOnly, Category = "Input",
            meta = (AllowPrivateAccess = true))
  TObjectPtr<UInputAction> InteractAction;
  UPROPERTY(EditDefaultsOnly, Category = "Input",
            meta = (AllowPrivateAccess = true))
  TObjectPtr<UInputAction> LookAction;
  UPROPERTY(EditDefaultsOnly, Category = "Input",
            meta = (AllowPrivateAccess = true))
  TObjectPtr<UInputAction> ActivateRotateAction;
  UPROPERTY(EditDefaultsOnly, Category = "Input",
            meta = (AllowPrivateAccess = true))
  TObjectPtr<UInputAction> ScrollAction;
  UPROPERTY(EditDefaultsOnly, Category = "Input",
            meta = (AllowPrivateAccess = true))
  TObjectPtr<UInputAction> ActionAction;
  UPROPERTY(EditDefaultsOnly, Category = "Input",
            meta = (AllowPrivateAccess = true))
  TObjectPtr<UInputAction> MirrorAttachmentAction;

  UPROPERTY(EditDefaultsOnly, Category = "Input",
            meta = (AllowPrivateAccess = true))
  TObjectPtr<UInputMappingContext> DefaultMappingContext{nullptr};

  bool DoOnceChangeAttachment{false};

  bool bEnableCamRotation{false};

  bool bIsSnapping{false};

  FVector OriginLoc{FVector::ZeroVector};
  FVector FocusStartLoc{FVector::ZeroVector};
  FVector FocusEndLoc{FVector::ZeroVector};

  bool bCanAttachmentBePlaced{false};
  bool bDoOnceMatAttachment{false};

  //==================================================
  // FUNCTIONS
  //==================================================

  FTimeline FocusTimeline;

  UFUNCTION()
  void UpdateFocus(const float Alpha);
};
