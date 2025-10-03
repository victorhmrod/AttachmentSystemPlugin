// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "AttachmentSystemCameraManager.generated.h"

/**
 *  Basic First Person camera manager.
 *  Limits min/max look pitch.
 */
UCLASS()
class AAttachmentSystemCameraManager : public APlayerCameraManager {
  GENERATED_BODY()

public:
  /** Constructor */
  AAttachmentSystemCameraManager();
};
