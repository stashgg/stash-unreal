// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "PurchaseCelebrationManager.generated.h"

class UNiagaraSystem;
class UUserWidget;

/**
 * Manages purchase celebration effects (Niagara systems and widgets)
 */
UCLASS(BlueprintType, Blueprintable)
class STASH_PAY_UNREAL_DEMO_API UPurchaseCelebrationManager : public UObject
{
	GENERATED_BODY()

public:
	UPurchaseCelebrationManager(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	 * Shows the celebration effect for a purchased item
	 * @param World The world context
	 * @param ItemName The name of the purchased item
	 * @param SpawnLocation The location to spawn the effect (if empty, uses player/camera location)
	 */
	UFUNCTION(BlueprintCallable, Category = "Purchase Celebration")
	void ShowCelebration(UWorld* World, const FString& ItemName, const FVector& SpawnLocation = FVector::ZeroVector);

	/**
	 * Sets the Niagara system to use for celebrations
	 */
	UFUNCTION(BlueprintCallable, Category = "Purchase Celebration")
	void SetCelebrationNiagara(UNiagaraSystem* NiagaraSystem);

	/**
	 * Sets the widget class to use for celebrations
	 */
	UFUNCTION(BlueprintCallable, Category = "Purchase Celebration")
	void SetCelebrationWidgetClass(TSubclassOf<UUserWidget> WidgetClass);

	/**
	 * Gets the current item name (for Blueprint access)
	 */
	UFUNCTION(BlueprintPure, Category = "Purchase Celebration")
	FString GetCurrentItemName() const { return CurrentItemName; }

protected:
	// Niagara system template for purchase celebration (set via SetCelebrationNiagara)
	UPROPERTY()
	UNiagaraSystem* CelebrationNiagara;

	// Widget class for purchase celebration text (set via SetCelebrationWidgetClass)
	UPROPERTY()
	TSubclassOf<UUserWidget> CelebrationWidgetClass;

public:

private:
	// Current item name being celebrated
	UPROPERTY()
	FString CurrentItemName;

	// Helper function to get spawn location
	FVector GetSpawnLocation(UWorld* World, const FVector& ProvidedLocation) const;
};
