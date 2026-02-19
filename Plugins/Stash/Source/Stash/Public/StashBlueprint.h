// Copyright Stash. All Rights Reserved.
// Stash Unreal Engine SDK - Blueprint Function Library

#pragma once

#include <Kismet/BlueprintFunctionLibrary.h>

#include "StashBlueprint.generated.h"

class UStashSubsystem;

// Stash payment and lifecycle callbacks
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStashPaymentSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStashPaymentFailure);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStashDialogDismissed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStashOptInResponse, FString, OptInType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStashPageLoaded, float, LoadTimeMs);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStashNetworkError);

/**
 * Configuration for Stash Native card presentation (openCard).
 *
 * Card slides up from bottom on phones; centered on tablets.
 * When force portrait is off, phone landscape uses configurable width/height ratios.
 */
USTRUCT(BlueprintType)
struct FStashCardConfig
{
	GENERATED_BODY()

	/** When true, phone card is portrait-only; when false, card appears in current orientation with landscape sizing. Default false. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stash")
	bool bForcePortrait = false;

	/** Phone card height ratio for portrait (0.1-1.0). Default 0.68. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stash", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float CardHeightRatioPortrait = 0.68f;

	/** Phone card width ratio for landscape (0.1-1.0). Used when bForcePortrait is false. Default 0.9. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stash", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float CardWidthRatioLandscape = 0.9f;

	/** Phone card height ratio for landscape (0.1-1.0). Used when bForcePortrait is false. Default 0.6. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stash", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float CardHeightRatioLandscape = 0.6f;

	/** Tablet width ratio for portrait (0.1-1.0). Default 0.6. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stash", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float TabletWidthRatioPortrait = 0.6f;

	/** Tablet height ratio for portrait (0.1-1.0). Default 0.8. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stash", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float TabletHeightRatioPortrait = 0.8f;

	/** Tablet width ratio for landscape (0.1-1.0). Default 0.8. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stash", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float TabletWidthRatioLandscape = 0.8f;

	/** Tablet height ratio for landscape (0.1-1.0). Default 0.65. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stash", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float TabletHeightRatioLandscape = 0.65f;
};

/**
 * Configuration for Stash Native modal presentation (openModal).
 *
 * Modal always appears centered on screen. Supports independent sizing for phone/tablet and portrait/landscape.
 */
USTRUCT(BlueprintType)
struct FStashModalConfig
{
	GENERATED_BODY()

	/** Whether to show drag bar at top of modal. Default true. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stash")
	bool bShowDragBar = true;

	/** Whether tap outside and drag gestures can dismiss the modal. Default true. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stash")
	bool bAllowDismiss = true;

	/** Phone width ratio for portrait (0.1-1.0). Default 0.9. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stash", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float PhoneWidthRatioPortrait = 0.9f;

	/** Phone height ratio for portrait (0.1-1.0). Default 0.7. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stash", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float PhoneHeightRatioPortrait = 0.7f;

	/** Phone width ratio for landscape (0.1-1.0). Default 0.7. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stash", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float PhoneWidthRatioLandscape = 0.7f;

	/** Phone height ratio for landscape (0.1-1.0). Default 0.85. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stash", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float PhoneHeightRatioLandscape = 0.85f;

	/** Tablet width ratio for portrait (0.1-1.0). Default 0.6. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stash", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float TabletWidthRatioPortrait = 0.6f;

	/** Tablet height ratio for portrait (0.1-1.0). Default 0.7. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stash", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float TabletHeightRatioPortrait = 0.7f;

	/** Tablet width ratio for landscape (0.1-1.0). Default 0.5. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stash", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float TabletWidthRatioLandscape = 0.5f;

	/** Tablet height ratio for landscape (0.1-1.0). Default 0.8. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stash", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float TabletHeightRatioLandscape = 0.8f;
};

/**
 * Stash Blueprint Function Library
 *
 * Provides cross-platform functions for integrating Stash Native (card, modal, browser)
 * into Unreal Engine projects on iOS and Android.
 */
UCLASS()
class STASH_API UStashBlueprint : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UStashBlueprint(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {};

	/**
	 * Builds a card config with all options. Use in Blueprint with Open Card With Config.
	 *
	 * @param bForcePortrait When true, phone card is portrait-only; when false, uses current orientation and landscape ratios.
	 * @param CardHeightRatioPortrait Phone card height in portrait (0.1-1.0).
	 * @param CardWidthRatioLandscape Phone card width in landscape when force portrait off (0.1-1.0).
	 * @param CardHeightRatioLandscape Phone card height in landscape when force portrait off (0.1-1.0).
	 * @param TabletWidthRatioPortrait Tablet width in portrait (0.1-1.0).
	 * @param TabletHeightRatioPortrait Tablet height in portrait (0.1-1.0).
	 * @param TabletWidthRatioLandscape Tablet width in landscape (0.1-1.0).
	 * @param TabletHeightRatioLandscape Tablet height in landscape (0.1-1.0).
	 */
	UFUNCTION(BlueprintCallable, Category = "Stash", meta = (DisplayName = "Make Stash Card Config"))
	static FStashCardConfig MakeStashCardConfig(
		bool bForcePortrait,
		float CardHeightRatioPortrait,
		float CardWidthRatioLandscape,
		float CardHeightRatioLandscape,
		float TabletWidthRatioPortrait = 0.6f,
		float TabletHeightRatioPortrait = 0.8f,
		float TabletWidthRatioLandscape = 0.8f,
		float TabletHeightRatioLandscape = 0.65f
	);

	/**
	 * Opens the Stash card with default sizing (drawer on phones, centered on tablets).
	 *
	 * @param URL The URL to load in the card
	 */
	UFUNCTION(BlueprintCallable, Category = "Stash", meta = (DisplayName = "Open Card"))
	static void OpenCard(const FString& URL);

	/**
	 * Opens the Stash card with custom sizing configuration.
	 *
	 * @param URL The URL to load in the card
	 * @param Config Configuration for card/tablet sizing (use Make Stash Card Config)
	 */
	UFUNCTION(BlueprintCallable, Category = "Stash", meta = (DisplayName = "Open Card With Config"))
	static void OpenCardWithConfig(const FString& URL, const FStashCardConfig& Config);

	/**
	 * Checks if the Stash card or modal is currently open.
	 *
	 * @return true if card or modal is displayed
	 */
	UFUNCTION(BlueprintCallable, Category = "Stash", meta = (DisplayName = "Is Card Open"))
	static bool IsCardOpen();

	/**
	 * Checks if a purchase is currently being processed.
	 * When true, the checkout UI cannot be dismissed by the user.
	 *
	 * @return true if a purchase is in progress
	 */
	UFUNCTION(BlueprintCallable, Category = "Stash", meta = (DisplayName = "Is Purchase Processing"))
	static bool IsPurchaseProcessing();

	/**
	 * Dismisses the currently displayed Stash card or modal.
	 */
	UFUNCTION(BlueprintCallable, Category = "Stash", meta = (DisplayName = "Dismiss Card"))
	static void DismissCard();

	/**
	 * Opens the URL in the platform browser (SFSafariViewController on iOS, Chrome Custom Tabs on Android).
	 * No in-app UI, no config, no callbacks.
	 *
	 * @param URL The URL to open
	 */
	UFUNCTION(BlueprintCallable, Category = "Stash", meta = (DisplayName = "Open Browser"))
	static void OpenBrowser(const FString& URL);

	/**
	 * Dismisses the in-app browser view. iOS only; no-op on Android (Chrome Custom Tabs cannot be closed by the app).
	 */
	UFUNCTION(BlueprintCallable, Category = "Stash", meta = (DisplayName = "Close Browser"))
	static void CloseBrowser();

	// ========================================================================
	// Modal Presentation (Stash Native 2.0)
	// ========================================================================

	/**
	 * Opens a URL in a centered modal dialog with default configuration.
	 * Unlike OpenCard which uses drawer/centered card, OpenModal always shows a centered modal on all devices.
	 *
	 * @param URL The URL to load in the modal
	 */
	UFUNCTION(BlueprintCallable, Category = "Stash", meta = (DisplayName = "Open Modal"))
	static void OpenModal(const FString& URL);

	/**
	 * Opens a URL in a centered modal dialog with custom configuration.
	 *
	 * @param URL The URL to load in the modal
	 * @param Config Configuration for sizing, drag bar, and dismiss behavior
	 */
	UFUNCTION(BlueprintCallable, Category = "Stash", meta = (DisplayName = "Open Modal With Config"))
	static void OpenModalWithConfig(const FString& URL, const FStashModalConfig& Config);

	// ========================================================================
	// Configuration
	// ========================================================================

	/**
	 * (iOS) When enabled, the app stays in landscape when card/modal is not open; portrait is allowed only while Stash UI is displayed. Call at game startup for landscape-only games. No effect on Android.
	 *
	 * @param bEnable true to lock to landscape when card closed, false to use default orientations
	 */
	UFUNCTION(BlueprintCallable, Category = "Stash", meta = (DisplayName = "Set Landscape Lock When Card Closed"))
	static void SetLandscapeLockWhenCardClosed(bool bEnable);

	/**
	 * Returns the Stash Subsystem so you can bind to On Payment Success, On Dialog Dismissed, etc. in Blueprint.
	 * In Blueprint: call "Get Stash Subsystem", then use "Assign [event]" or "Add [event]" on the returned object.
	 * World context can be a World, Actor, Player Controller, or Game Instance; pass Self from Level Blueprint or an Actor in a running game.
	 * @return The subsystem, or null if no valid world is available (e.g. in editor before Play-in-Editor, or invalid context).
	 */
	UFUNCTION(BlueprintCallable, Category = "Stash", meta = (DisplayName = "Get Stash Subsystem", WorldContext = "WorldContextObject"))
	static UStashSubsystem* GetStashSubsystem(UObject* WorldContextObject);

	// ========================================================================
	// Delegates - Bind to these to receive payment callbacks (C++). For Blueprint, use Get Stash Subsystem then bind to its events.
	// ========================================================================
	
	/** Called when a payment completes successfully */
	static FOnStashPaymentSuccess OnPaymentSuccess;
	
	/** Called when a payment fails */
	static FOnStashPaymentFailure OnPaymentFailure;
	
	/** Called when the card or modal is dismissed by the user */
	static FOnStashDialogDismissed OnDialogDismissed;
	
	/** Called when an opt-in response is received (modal payment channel selection) */
	static FOnStashOptInResponse OnOptInResponse;
	
	/** Called when the card/modal page finishes loading */
	static FOnStashPageLoaded OnPageLoaded;

	/** Called when a network error occurs during initial page load */
	static FOnStashNetworkError OnNetworkError;
	
	// ========================================================================
	// Internal callback functions called from native code
	// ========================================================================
	static void HandlePaymentSuccess();
	static void HandlePaymentFailure();
	static void HandleDialogDismissed();
	static void HandleOptInResponse(const FString& OptInType);
	static void HandlePageLoaded(float LoadTimeMs);
	static void HandleNetworkError();
};
