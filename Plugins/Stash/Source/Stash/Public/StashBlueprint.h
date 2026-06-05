// Copyright Stash. All Rights Reserved.
// Stash Unreal Engine SDK - Blueprint Function Library

#pragma once

#include <Kismet/BlueprintFunctionLibrary.h>

#include "StashBlueprint.generated.h"

class UStashSubsystem;
struct FLatentActionInfo;

// Stash payment and lifecycle callbacks
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStashPaymentSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStashPaymentFailure);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStashDialogDismissed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStashOptInResponse, FString, OptInType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStashPageLoaded, float, LoadTimeMs);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStashNetworkError);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStashExternalPayment, FString, URL);

/** Fired after end-of-frame viewport read; JPEG bytes suitable for Android checkout backdrop (assign to Card Config or Set Android Checkout Backdrop Bytes). */
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnStashViewportCaptureComplete, TArray<uint8>, ImageBytes);

/**
 * Optional Android-only configuration for the Stash Native keep-alive foreground service (Chrome Custom Tabs / low-memory devices).
 */
USTRUCT(BlueprintType)
struct FStashKeepAliveConfig
{
	GENERATED_BODY()

	/** Notification title shown while the user is outside the app. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stash")
	FString NotificationTitle;

	/** Notification body text. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stash")
	FString NotificationText;

	/**
	 * Android only. Base name of a drawable in the game's merged APK (no @drawable/, no extension).
	 * Example: "stash_payment_icon" for res/drawable/stash_payment_icon.xml.
	 * Leave empty to use the Stash SDK default notification icon.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stash",
		meta = (DisplayName = "Notification Icon Drawable Name (Android)"))
	FString NotificationIconDrawableName;
};

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

	/** Optional shell background color as HTML hex (e.g. "#RRGGBB"). Leave empty for SDK default light/dark. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stash")
	FString BackgroundColor;

	/**
	 * Android only: optional PNG or JPEG bytes shown behind the dim overlay during force-portrait checkout
	 * (stash-native `setBackdropBitmap`). When non-empty, Open Card With Config applies this on the UI thread
	 * immediately before `openCard` in the same runnable (preferred over separate Set + Open calls).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stash")
	TArray<uint8> AndroidCheckoutBackdrop;
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

	/** Optional shell background color as HTML hex (e.g. "#RRGGBB"). Leave empty for SDK default light/dark. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stash")
	FString BackgroundColor;
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
	 * @param BackgroundColor Optional HTML hex shell color (e.g. "#RRGGBB"); leave empty for SDK default.
	 * Assign **Android Checkout Backdrop** (byte array) on the returned struct in Blueprint (Set members / Break-Make) after viewport capture.
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
		float TabletHeightRatioLandscape = 0.65f,
		FString BackgroundColor = TEXT("")
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
	 * Builds a keep-alive config for Set Android Keep Alive Config (Android only).
	 *
	 * @param NotificationTitle Notification title while the user is outside the app.
	 * @param NotificationText Notification body text.
	 * @param NotificationIconDrawableName Drawable base name in the game APK (no @drawable/, no extension); leave empty for SDK default.
	 */
	UFUNCTION(BlueprintCallable, Category = "Stash", meta = (DisplayName = "Make Stash Keep Alive Config"))
	static FStashKeepAliveConfig MakeStashKeepAliveConfig(
		FString NotificationTitle,
		FString NotificationText,
		FString NotificationIconDrawableName = TEXT("")
	);

	/**
	 * (iOS) When enabled, the app stays in landscape when card/modal is not open; portrait is allowed only while Stash UI is displayed. Call at game startup for landscape-only games. No effect on Android.
	 *
	 * @param bEnable true to lock to landscape when card closed, false to use default orientations
	 */
	UFUNCTION(BlueprintCallable, Category = "Stash", meta = (DisplayName = "Set Landscape Lock When Card Closed"))
	static void SetLandscapeLockWhenCardClosed(bool bEnable);

	/**
	 * (Android) Enables the Stash Native keep-alive foreground service so the app is less likely to be killed when the user leaves for Chrome Custom Tabs. No effect on iOS.
	 */
	UFUNCTION(BlueprintCallable, Category = "Stash", meta = (DisplayName = "Set Android Keep Alive Enabled"))
	static void SetAndroidKeepAliveEnabled(bool bEnabled);

	/**
	 * (Android) Sets notification title, text, and optional notification icon (drawable name) for the keep-alive service. Call after Set Android Keep Alive Enabled if needed. No effect on iOS.
	 */
	UFUNCTION(BlueprintCallable, Category = "Stash", meta = (DisplayName = "Set Android Keep Alive Config"))
	static void SetAndroidKeepAliveConfig(const FStashKeepAliveConfig& Config);

	/**
	 * (Android) Passes encoded image bytes (PNG or JPEG) to Stash Native before Open Card / Open Modal to reduce flash during
	 * landscape-to-portrait transitions. Uses stash-native `setBackdropBitmap` (decoded PNG/JPEG). Cleared automatically when the card is dismissed; you may also call Clear Android Checkout Backdrop. Prefer filling **Android Checkout Backdrop** on **Stash Card Config** so open runs in one UI step with **Open Card With Config**. No effect on iOS or other platforms.
	 */
	UFUNCTION(BlueprintCallable, Category = "Stash", meta = (DisplayName = "Set Android Checkout Backdrop Bytes"))
	static void SetAndroidCheckoutBackdropBytes(const TArray<uint8>& ImageBytes);

	/**
	 * (Android) Clears any checkout backdrop set via Set Android Checkout Backdrop Bytes. No effect on iOS or other platforms.
	 */
	UFUNCTION(BlueprintCallable, Category = "Stash", meta = (DisplayName = "Clear Android Checkout Backdrop"))
	static void ClearAndroidCheckoutBackdrop();

	/**
	 * (Android) Captures the game viewport after the current frame finishes rendering, compresses to JPEG, and returns bytes via the delegate.
	 * Prefer **Capture Viewport For Android Checkout Backdrop (Latent)** in Widget Blueprints (white exec chain, no delegate binding).
	 *
	 * @param WorldContextObject World, Actor, Player Controller, or Game Instance used to schedule the capture
	 * @param OnComplete Called on the game thread with JPEG bytes or an empty array on failure
	 */
	UFUNCTION(BlueprintCallable, Category = "Stash", meta = (DisplayName = "Capture Viewport For Android Checkout Backdrop (Delegate)", WorldContext = "WorldContextObject"))
	static void CaptureViewportForAndroidCheckoutBackdrop(UObject* WorldContextObject, FOnStashViewportCaptureComplete OnComplete);

	/**
	 * (Android) Same capture as the delegate version; use the white **Completed** exec pin and **Out Image Bytes** in Blueprint.
	 * Wire Completed → Set members (Android Checkout Backdrop) → Open Card With Config.
	 */
	UFUNCTION(BlueprintCallable, Category = "Stash", meta = (DisplayName = "Capture Viewport For Android Checkout Backdrop", WorldContext = "WorldContextObject", Latent, LatentInfo = "LatentInfo"))
	static void CaptureViewportForAndroidCheckoutBackdropLatent(UObject* WorldContextObject, TArray<uint8>& OutImageBytes, FLatentActionInfo LatentInfo);

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

	/** Called when checkout opens an external URL (e.g. Google Pay, Klarna); payment may complete in browser or another app. */
	static FOnStashExternalPayment OnExternalPayment;
	
	// ========================================================================
	// Internal callback functions called from native code
	// ========================================================================
	static void HandlePaymentSuccess();
	static void HandlePaymentFailure();
	static void HandleDialogDismissed();
	static void HandleOptInResponse(const FString& OptInType);
	static void HandlePageLoaded(float LoadTimeMs);
	static void HandleNetworkError();
	static void HandleExternalPayment(const FString& URL);
};
