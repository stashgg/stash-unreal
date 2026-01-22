// Fill out your copyright notice in the Description page of Project Settings.

#include "MyGameMode.h"
#include "MyPlayerController.h"
#include "Engine/Engine.h"

AMyGameMode::AMyGameMode()
{
	// Set the default player controller class
	PlayerControllerClass = AMyPlayerController::StaticClass();
	
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("AMyGameMode Constructor Called!"));
	UE_LOG(LogTemp, Warning, TEXT("PlayerControllerClass set to: MyPlayerController"));
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Magenta, TEXT("MyGameMode Constructor Called!"));
	}
}
