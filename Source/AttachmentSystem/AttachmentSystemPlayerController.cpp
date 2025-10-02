// Copyright Epic Games, Inc. All Rights Reserved.

#include "AttachmentSystemPlayerController.h"
#include "AttachmentSystem.h"
#include "AttachmentSystemCameraManager.h"
#include "Blueprint/UserWidget.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Widgets/Input/SVirtualJoystick.h"

AAttachmentSystemPlayerController::AAttachmentSystemPlayerController() {
  // set the player camera manager class
  PlayerCameraManagerClass = AAttachmentSystemCameraManager::StaticClass();
}

void AAttachmentSystemPlayerController::BeginPlay() {
  Super::BeginPlay();

  // only spawn touch controls on local player controllers
  if (SVirtualJoystick::ShouldDisplayTouchInterface() &&
      IsLocalPlayerController()) {
    // spawn the mobile controls widget
    MobileControlsWidget =
        CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

    if (MobileControlsWidget) {
      // add the controls to the player screen
      MobileControlsWidget->AddToPlayerScreen(0);

    } else {

      UE_LOG(LogAttachmentSystem, Error,
             TEXT("Could not spawn mobile controls widget."));
    }
  }
}

void AAttachmentSystemPlayerController::SetupInputComponent() {
  Super::SetupInputComponent();

  // only add IMCs for local player controllers
  if (IsLocalPlayerController()) {
    // Add Input Mapping Context
    if (UEnhancedInputLocalPlayerSubsystem *Subsystem =
            ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(
                GetLocalPlayer())) {
      for (UInputMappingContext *CurrentContext : DefaultMappingContexts) {
        Subsystem->AddMappingContext(CurrentContext, 0);
      }

      // only add these IMCs if we're not using mobile touch input
      if (!SVirtualJoystick::ShouldDisplayTouchInterface()) {
        for (UInputMappingContext *CurrentContext :
             MobileExcludedMappingContexts) {
          Subsystem->AddMappingContext(CurrentContext, 0);
        }
      }
    }
  }
}
