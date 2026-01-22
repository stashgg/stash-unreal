// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "GameMenuWidget.generated.h"

/**
 * Game Menu Widget - Main menu with TEST button
 * This is a pure C++ widget that creates its UI programmatically
 */
UCLASS()
class STASH_PAY_UNREAL_DEMO_API UGameMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION()
	void OnTestButtonClicked();

private:
	// UI Components created programmatically
	UPROPERTY()
	class UButton* TestButton;

	UPROPERTY()
	class UTextBlock* TestButtonText;
	
	// Track if initial layout has been done (needed for mobile)
	bool bInitialLayoutDone;
};
