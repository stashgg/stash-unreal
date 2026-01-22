// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/VerticalBox.h"
#include "Components/CanvasPanel.h"
#include "ConsoleWidget.generated.h"

/**
 * Console widget to display logged messages - Pure C++ implementation
 */
UCLASS()
class STASH_PAY_UNREAL_DEMO_API UConsoleWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION()
	void OnCloseButtonClicked();

	// Add a log message to the console
	UFUNCTION(BlueprintCallable, Category = "Console")
	void AddLogMessage(const FString& Message);

private:
	// UI elements created programmatically
	UPROPERTY()
	class UScrollBox* ConsoleScrollBox;

	UPROPERTY()
	class UButton* CloseButton;

	UPROPERTY()
	class UTextBlock* ConsoleTitle;

	UPROPERTY()
	class UVerticalBox* MainContainer;

	// Store last log count to detect new messages
	int32 LastLogCount;
	
	// Maximum number of log entries to display
	UPROPERTY(EditAnywhere, Category = "Console")
	int32 MaxLogEntries = 100;

	// Update console with latest log messages
	void UpdateConsoleLogs();
	
	// Create all UI elements programmatically
	void CreateWidgets();
};
