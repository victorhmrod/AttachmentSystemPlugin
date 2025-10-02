// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AttachmentSystemGameMode.generated.h"
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"

/**
 *  Simple GameMode for a first person game
 */
UCLASS(abstract)
class AAttachmentSystemGameMode : public AGameModeBase {
  GENERATED_BODY()

public:
  AAttachmentSystemGameMode();
};
