#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "StashGameMode.generated.h"

/**
 * StashGameMode
 * 
 * A simple GameMode that uses StashPlayerController.
 * Use this GameMode in your levels to enable Stash Pay checkout functionality.
 */
UCLASS()
class STASHUNREAL4_API AStashGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AStashGameMode();
};
