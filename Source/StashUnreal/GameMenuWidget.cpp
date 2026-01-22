// Fill out your copyright notice in the Description page of Project Settings.

#include "GameMenuWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/Engine.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateTypes.h" // For FButtonStyle
#include "Styling/AppStyle.h" // For FAppStyle (UE 5.7)

void UGameMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("GameMenuWidget::NativeConstruct STARTED"));
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	
	// Ensure we have a WidgetTree
	if (!WidgetTree)
	{
		UE_LOG(LogTemp, Error, TEXT("CRITICAL: WidgetTree is NULL!"));
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("WidgetTree is NULL!"));
		}
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("WidgetTree is valid"));
	
	// Get or create root CanvasPanel
	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(GetRootWidget());
	if (!RootCanvas)
	{
		UE_LOG(LogTemp, Warning, TEXT("Creating new root CanvasPanel"));
		RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
		if (!RootCanvas)
		{
			UE_LOG(LogTemp, Error, TEXT("CRITICAL: Failed to create RootCanvas!"));
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("Failed to create RootCanvas!"));
			}
			return;
		}
		WidgetTree->RootWidget = RootCanvas;
		RootCanvas->SetVisibility(ESlateVisibility::Visible);
		UE_LOG(LogTemp, Warning, TEXT("RootCanvas created and set as root"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("RootCanvas already exists"));
		RootCanvas->SetVisibility(ESlateVisibility::Visible);
	}
	
	// Create the TEST button
	UE_LOG(LogTemp, Warning, TEXT("Creating TestButton..."));
	TestButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	if (!TestButton)
	{
		UE_LOG(LogTemp, Error, TEXT("CRITICAL: Failed to create TestButton!"));
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("Failed to create TestButton!"));
		}
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("TestButton created successfully"));
	
	// Set up button click event
	TestButton->OnClicked.AddDynamic(this, &UGameMenuWidget::OnTestButtonClicked);
	TestButton->SetVisibility(ESlateVisibility::Visible); // Make button visible (not hit-test invisible)
	
	// Ensure button is touch-enabled on mobile
	TestButton->SetIsEnabled(true);
	TestButton->SetTouchMethod(EButtonTouchMethod::Down);
	
	// Set a default style for the button to ensure it has a visible background
	TestButton->SetStyle(FAppStyle::Get().GetWidgetStyle<FButtonStyle>("Button"));
	
	UE_LOG(LogTemp, Warning, TEXT("TestButton click event and visibility set (touch-enabled, visible, style set)"));
	
	// Add button to canvas and center it
	UE_LOG(LogTemp, Warning, TEXT("Adding TestButton to canvas..."));
	UCanvasPanelSlot* ButtonSlot = RootCanvas->AddChildToCanvas(TestButton);
	if (!ButtonSlot)
	{
		UE_LOG(LogTemp, Error, TEXT("CRITICAL: Failed to get ButtonSlot!"));
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("Failed to get ButtonSlot!"));
		}
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("ButtonSlot obtained"));
	
	// Center the button on screen with explicit large size
	ButtonSlot->SetAnchors(FAnchors(0.5f, 0.5f));
	ButtonSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	ButtonSlot->SetOffsets(FMargin(-200.0f, -50.0f, 200.0f, 50.0f)); // 400x100 button - LARGE
	ButtonSlot->SetAutoSize(false); // Explicitly disable auto-size to use offsets
	ButtonSlot->SetZOrder(1000); // High z-order
	UE_LOG(LogTemp, Warning, TEXT("ButtonSlot configured: 400x100, centered, z-order 1000, AutoSize=false"));
	
	// Create text for the button - add it DIRECTLY to canvas (more reliable on mobile)
	UE_LOG(LogTemp, Warning, TEXT("Creating TestButtonText..."));
	TestButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	if (!TestButtonText)
	{
		UE_LOG(LogTemp, Error, TEXT("CRITICAL: Failed to create TestButtonText!"));
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("Failed to create TestButtonText!"));
		}
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("TestButtonText created successfully"));
	
	// Set button text properties - make it VERY visible
	TestButtonText->SetText(FText::FromString(TEXT("TEST")));
	TestButtonText->SetColorAndOpacity(FSlateColor(FLinearColor::Yellow)); // YELLOW for maximum visibility
	TestButtonText->SetJustification(ETextJustify::Center);
	
	// Make text MUCH larger
	FSlateFontInfo FontInfo = TestButtonText->GetFont();
	FontInfo.Size = 72; // VERY LARGE
	TestButtonText->SetFont(FontInfo);
	TestButtonText->SetVisibility(ESlateVisibility::Visible);
	
	UE_LOG(LogTemp, Warning, TEXT("TestButtonText configured: Yellow, Size 72, Visible"));
	
	// Add text DIRECTLY to canvas (not as child of button) - this is more reliable on mobile
	UCanvasPanelSlot* TextSlot = RootCanvas->AddChildToCanvas(TestButtonText);
	if (TextSlot)
	{
		// Center the text on screen - use explicit offsets to ensure it's centered
		TextSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		TextSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		// Use explicit offsets centered around anchor point (negative half-width, negative half-height)
		TextSlot->SetOffsets(FMargin(-150.0f, -36.0f, 150.0f, 36.0f)); // Approximate size for "TEST" at 72pt
		TextSlot->SetAutoSize(false); // Use explicit offsets for reliable positioning
		TextSlot->SetZOrder(1001); // Above button
		UE_LOG(LogTemp, Warning, TEXT("TestButtonText added directly to canvas (above button, explicit offsets)"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get TextSlot for TestButtonText!"));
		// Fallback: add as child of button
		TestButton->AddChild(TestButtonText);
		UE_LOG(LogTemp, Warning, TEXT("Fallback: TestButtonText added to TestButton"));
	}
	
	// Force layout update
	InvalidateLayoutAndVolatility();
	if (RootCanvas)
	{
		RootCanvas->InvalidateLayoutAndVolatility();
	}
	
	// Ensure widget is set to handle touch input on mobile
	SetIsEnabled(true);
	
	// Track if we've done initial layout (needed for mobile)
	bInitialLayoutDone = false;
	
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("GameMenuWidget::NativeConstruct COMPLETE"));
	UE_LOG(LogTemp, Warning, TEXT("Widget enabled for touch input"));
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	
	// Add on-screen confirmation
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Green, TEXT("GameMenuWidget Created! Button should be visible."));
	}
}

void UGameMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	
	// On mobile, widget might not have size until after first tick
	// Force layout update once we have valid geometry
	if (!bInitialLayoutDone && MyGeometry.GetLocalSize().X > 0 && MyGeometry.GetLocalSize().Y > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Widget has valid geometry: %.2f x %.2f - Forcing layout update"), 
			MyGeometry.GetLocalSize().X, MyGeometry.GetLocalSize().Y);
		
		// Ensure root canvas fills the widget
		if (UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(GetRootWidget()))
		{
			RootCanvas->SetVisibility(ESlateVisibility::Visible);
			RootCanvas->InvalidateLayoutAndVolatility();
			
			// Get canvas geometry
			FGeometry CanvasGeometry = RootCanvas->GetCachedGeometry();
			UE_LOG(LogTemp, Warning, TEXT("RootCanvas geometry: %.2f x %.2f"), 
				CanvasGeometry.GetLocalSize().X, CanvasGeometry.GetLocalSize().Y);
		}
		
		// Ensure button is visible and properly sized
		if (TestButton)
		{
			TestButton->SetVisibility(ESlateVisibility::Visible);
			TestButton->InvalidateLayoutAndVolatility();
			
			// Get button geometry
			FGeometry ButtonGeometry = TestButton->GetCachedGeometry();
			UE_LOG(LogTemp, Warning, TEXT("TestButton geometry: %.2f x %.2f, DesiredSize: %.2f x %.2f"), 
				ButtonGeometry.GetLocalSize().X, ButtonGeometry.GetLocalSize().Y,
				TestButton->GetDesiredSize().X, TestButton->GetDesiredSize().Y);
			
			// Check button slot
			if (UCanvasPanelSlot* ButtonSlot = Cast<UCanvasPanelSlot>(TestButton->Slot))
			{
				FMargin Offsets = ButtonSlot->GetOffsets();
				UE_LOG(LogTemp, Warning, TEXT("ButtonSlot offsets: L=%.2f T=%.2f R=%.2f B=%.2f"), 
					Offsets.Left, Offsets.Top, Offsets.Right, Offsets.Bottom);
			}
		}
		
		// Ensure text is visible
		if (TestButtonText)
		{
			TestButtonText->SetVisibility(ESlateVisibility::Visible);
			TestButtonText->InvalidateLayoutAndVolatility();
			
			// Get text geometry
			FGeometry TextGeometry = TestButtonText->GetCachedGeometry();
			UE_LOG(LogTemp, Warning, TEXT("TestButtonText geometry: %.2f x %.2f, DesiredSize: %.2f x %.2f"), 
				TextGeometry.GetLocalSize().X, TextGeometry.GetLocalSize().Y,
				TestButtonText->GetDesiredSize().X, TestButtonText->GetDesiredSize().Y);
		}
		
		// Force widget layout update
		InvalidateLayoutAndVolatility();
		
		bInitialLayoutDone = true;
		
		UE_LOG(LogTemp, Warning, TEXT("Layout update forced - button should now be visible"));
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, TEXT("Layout updated - button should be visible!"));
		}
	}
}

void UGameMenuWidget::OnTestButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("=== TEST BUTTON CLICKED! ==="));
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("TEST BUTTON CLICKED!"));
	}
}
