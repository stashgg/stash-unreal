// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MyGameMode.generated.h"

/**
 * Custom Game Mode that uses MyPlayerController
 */
UCLASS(Blueprintable)
class STASH_PAY_UNREAL_DEMO_API AMyGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMyGameMode();
};
