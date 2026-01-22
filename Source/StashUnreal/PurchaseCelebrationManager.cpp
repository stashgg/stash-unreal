// Fill out your copyright notice in the Description page of Project Settings.

#include "PurchaseCelebrationManager.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

UPurchaseCelebrationManager::UPurchaseCelebrationManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, CelebrationNiagara(nullptr)
	, CelebrationWidgetClass(nullptr)
{
}

void UPurchaseCelebrationManager::ShowCelebration(UWorld* World, const FString& ItemName, const FVector& SpawnLocation)
{
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("PurchaseCelebrationManager::ShowCelebration - World is NULL!"));
		return;
	}

	if (ItemName.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("PurchaseCelebrationManager::ShowCelebration - ItemName is empty!"));
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("ERROR: Item name is empty!"));
		}
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("PurchaseCelebrationManager::ShowCelebration"));
	UE_LOG(LogTemp, Warning, TEXT("Item: %s"), *ItemName);
	UE_LOG(LogTemp, Warning, TEXT("========================================"));

	// Store the item name
	CurrentItemName = ItemName;

	// Get spawn location
	FVector FinalSpawnLocation = GetSpawnLocation(World, SpawnLocation);

	// Spawn Niagara system if set
	if (CelebrationNiagara)
	{
		UE_LOG(LogTemp, Warning, TEXT("Spawning Niagara system: %s"), *CelebrationNiagara->GetName());
		UE_LOG(LogTemp, Warning, TEXT("Spawn location: X=%.2f, Y=%.2f, Z=%.2f"), 
			FinalSpawnLocation.X, FinalSpawnLocation.Y, FinalSpawnLocation.Z);

		UNiagaraComponent* NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World,
			CelebrationNiagara,
			FinalSpawnLocation,
			FRotator::ZeroRotator,
			FVector::OneVector,
			true,
			true
		);
		
		if (NiagaraComponent)
		{
			UE_LOG(LogTemp, Warning, TEXT("SUCCESS: Spawned purchase celebration Niagara effect (Component created)"));
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Niagara effect spawned!"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("ERROR: NiagaraComponent is NULL after spawn!"));
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("ERROR: Failed to create Niagara component!"));
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("CelebrationNiagara is NULL - cannot spawn effect!"));
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("ERROR: Niagara system is NULL! Set it in Blueprint!"));
		}
	}

	// Create and show celebration widget if set
	if (CelebrationWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("Creating widget from class: %s"), *CelebrationWidgetClass->GetName());

		// Get player controller for widget creation
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController)
		{
			UE_LOG(LogTemp, Warning, TEXT("PlayerController found: %s"), *PlayerController->GetName());
			UUserWidget* CelebrationWidget = CreateWidget<UUserWidget>(PlayerController, CelebrationWidgetClass);
			if (CelebrationWidget)
			{
				UE_LOG(LogTemp, Warning, TEXT("Widget created successfully: %s"), *CelebrationWidget->GetName());
				
				// Find and update the TextBlock to replace [ItemName] with the actual item name
				if (CelebrationWidget->WidgetTree)
				{
					// Try to find the TextBlock by name (TextBlock_38 from the screenshot)
					UTextBlock* ItemNameTextBlock = Cast<UTextBlock>(CelebrationWidget->WidgetTree->FindWidget(FName("TextBlock_38")));
					
					// If not found by name, try to find any TextBlock in the widget
					if (!ItemNameTextBlock)
					{
						TArray<UWidget*> AllWidgets;
						CelebrationWidget->WidgetTree->GetAllWidgets(AllWidgets);
						for (UWidget* Widget : AllWidgets)
						{
							if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
							{
								ItemNameTextBlock = TextBlock;
								UE_LOG(LogTemp, Warning, TEXT("Found TextBlock: %s"), *TextBlock->GetName());
								break;
							}
						}
					}
					
					if (ItemNameTextBlock)
					{
						// Get the current text
						FString CurrentText = ItemNameTextBlock->GetText().ToString();
						UE_LOG(LogTemp, Warning, TEXT("Current TextBlock text: %s"), *CurrentText);
						
						// Replace [ItemName] with the actual item name
						FString UpdatedText = CurrentText.Replace(TEXT("[ItemName]"), *ItemName);
						ItemNameTextBlock->SetText(FText::FromString(UpdatedText));
						
						UE_LOG(LogTemp, Warning, TEXT("Updated TextBlock text to: %s"), *UpdatedText);
					}
					else
					{
						UE_LOG(LogTemp, Warning, TEXT("Could not find TextBlock in widget - [ItemName] will not be replaced"));
					}
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("WidgetTree is NULL - cannot update TextBlock"));
				}
				
				CelebrationWidget->AddToViewport(10000); // High z-order to appear on top
				CelebrationWidget->SetVisibility(ESlateVisibility::Visible);
				UE_LOG(LogTemp, Warning, TEXT("SUCCESS: Widget added to viewport with z-order 10000"));
				UE_LOG(LogTemp, Warning, TEXT("Widget visibility: %d"), (int32)CelebrationWidget->GetVisibility());
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Celebration widget displayed!"));
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("ERROR: Failed to create widget from class!"));
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("ERROR: Failed to create widget!"));
				}
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("ERROR: No PlayerController found - cannot create widget!"));
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("ERROR: No PlayerController!"));
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ERROR: CelebrationWidgetClass is NULL - cannot create widget!"));
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("ERROR: Widget class is NULL! Set it in Blueprint!"));
		}
	}

	// Always show on-screen debug message as fallback
	if (GEngine)
	{
		FString Message = FString::Printf(TEXT("Congratulations! You purchased: %s"), *ItemName);
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, Message);
	}
}

void UPurchaseCelebrationManager::SetCelebrationNiagara(UNiagaraSystem* NiagaraSystem)
{
	CelebrationNiagara = NiagaraSystem;
	UE_LOG(LogTemp, Warning, TEXT("PurchaseCelebrationManager: Set Niagara system to %s"), 
		NiagaraSystem ? *NiagaraSystem->GetName() : TEXT("NULL"));
}

void UPurchaseCelebrationManager::SetCelebrationWidgetClass(TSubclassOf<UUserWidget> WidgetClass)
{
	CelebrationWidgetClass = WidgetClass;
	UE_LOG(LogTemp, Warning, TEXT("PurchaseCelebrationManager: Set widget class to %s"), 
		WidgetClass ? *WidgetClass->GetName() : TEXT("NULL"));
}

FVector UPurchaseCelebrationManager::GetSpawnLocation(UWorld* World, const FVector& ProvidedLocation) const
{
	// If a location was provided and it's not zero, use it
	if (!ProvidedLocation.IsNearlyZero())
	{
		return ProvidedLocation;
	}

	// Otherwise, try to get player/camera location
	if (!World)
	{
		return FVector::ZeroVector;
	}

	// Try to get player pawn location
	APawn* PlayerPawn = World->GetFirstPlayerController() ? World->GetFirstPlayerController()->GetPawn() : nullptr;
	if (PlayerPawn)
	{
		return PlayerPawn->GetActorLocation();
	}

	// Fallback to camera location
	APlayerController* PlayerController = World->GetFirstPlayerController();
	if (PlayerController && PlayerController->PlayerCameraManager)
	{
		return PlayerController->PlayerCameraManager->GetCameraLocation();
	}

	// Last resort: return zero vector
	return FVector::ZeroVector;
}
