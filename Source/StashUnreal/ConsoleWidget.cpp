// Fill out your copyright notice in the Description page of Project Settings.

#include "ConsoleWidget.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/Engine.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/FileHelper.h"
#include "Misc/DateTime.h"

void UConsoleWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	CreateWidgets();
	
	LastLogCount = 0;
	AddLogMessage(TEXT("Console window opened. Logs will appear here."));
}

void UConsoleWidget::CreateWidgets()
{
	// Get or create root canvas using WidgetTree
	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(GetRootWidget());
	if (!RootCanvas)
	{
		RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
		WidgetTree->RootWidget = RootCanvas;
	}

	// Create main vertical container
	MainContainer = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	if (MainContainer && RootCanvas)
	{
		UCanvasPanelSlot* ContainerSlot = RootCanvas->AddChildToCanvas(MainContainer);
		if (ContainerSlot)
		{
			ContainerSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			ContainerSlot->SetOffsets(FMargin(50.0f, 50.0f, 50.0f, 50.0f));
		}
	}

	// Create title bar with title and close button
	UHorizontalBox* TitleBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (TitleBar && MainContainer)
	{
		// Create title text
		ConsoleTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		if (ConsoleTitle)
		{
			ConsoleTitle->SetText(FText::FromString(TEXT("Unreal Console Log")));
			ConsoleTitle->SetColorAndOpacity(FSlateColor(FLinearColor::White));
			TitleBar->AddChildToHorizontalBox(ConsoleTitle);
		}

		// Create close button
		CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		if (CloseButton)
		{
			CloseButton->OnClicked.AddDynamic(this, &UConsoleWidget::OnCloseButtonClicked);
			
			// Add text to close button
			UTextBlock* CloseButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			if (CloseButtonText)
			{
				CloseButtonText->SetText(FText::FromString(TEXT("Close")));
				CloseButtonText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
				CloseButton->AddChild(CloseButtonText);
			}
			
			// Add button to horizontal box and set size via slot
			UHorizontalBoxSlot* ButtonSlot = Cast<UHorizontalBoxSlot>(TitleBar->AddChildToHorizontalBox(CloseButton));
			if (ButtonSlot)
			{
				ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
				ButtonSlot->SetVerticalAlignment(VAlign_Center);
			}
		}

		// Add title bar to main container
		UVerticalBoxSlot* TitleBarSlot = Cast<UVerticalBoxSlot>(MainContainer->AddChildToVerticalBox(TitleBar));
		if (TitleBarSlot)
		{
			TitleBarSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}
	}

	// Create scroll box for log messages
	ConsoleScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
	if (ConsoleScrollBox && MainContainer)
	{
		UVerticalBoxSlot* ScrollBoxSlot = Cast<UVerticalBoxSlot>(MainContainer->AddChildToVerticalBox(ConsoleScrollBox));
		if (ScrollBoxSlot)
		{
			ScrollBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
	}
}

void UConsoleWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	
	// Update console logs periodically
	UpdateConsoleLogs();
}

void UConsoleWidget::OnCloseButtonClicked()
{
	UE_LOG(LogTemp, Log, TEXT("Console window closed"));
	RemoveFromParent();
}

void UConsoleWidget::AddLogMessage(const FString& Message)
{
	if (!ConsoleScrollBox)
	{
		return;
	}

	// Create text block for the log message
	UTextBlock* LogTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	if (LogTextBlock)
	{
		FString Timestamp = FDateTime::Now().ToString(TEXT("%H:%M:%S"));
		FString LogMessage = FString::Printf(TEXT("[%s] %s"), *Timestamp, *Message);
		LogTextBlock->SetText(FText::FromString(LogMessage));
		LogTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		
		// Set text wrapping
		LogTextBlock->SetAutoWrapText(true);
		
		// Add to scroll box
		ConsoleScrollBox->AddChild(LogTextBlock);
		
		// Limit the number of entries
		if (ConsoleScrollBox->GetChildrenCount() > MaxLogEntries)
		{
			ConsoleScrollBox->RemoveChildAt(0);
		}
		
		// Scroll to bottom
		ConsoleScrollBox->ScrollToEnd();
	}
}

void UConsoleWidget::UpdateConsoleLogs()
{
	// This is a simplified version - in a real implementation, you might want to
	// hook into the logging system more directly
	// For now, we'll just add a message when the widget is first shown
	// The actual log messages will come from the game's logging system
}
