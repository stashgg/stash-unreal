// Copyright Stash. All Rights Reserved.
// Stash Pay Unreal Engine SDK - Blueprint Function Library

#pragma once

#include <Kismet/BlueprintFunctionLibrary.h>

#include "StashBlueprint.generated.h"

// Stash Pay Payment Callbacks
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStashPaymentSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStashPaymentFailure);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStashDialogDismissed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStashOptInResponse, FString, OptInType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStashPageLoaded, float, LoadTimeMs);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStashNetworkError);

/**
 * Configuration for Stash Pay checkout presentation.
 * 
 * Checkout uses card presentation on phones (full width, configurable height)
 * and centered cards on tablets (configurable width and height).
 */
USTRUCT(BlueprintType)
struct FStashCheckoutConfig
{
	GENERATED_BODY()

	/** Phone card height ratio for portrait (0.1-1.0). Default 0.68. */
	UPROPERTY(BlueprintReadWrite, Category = "Stash", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float CardHeightRatioPortrait = 0.68f;

	/** Tablet width ratio for portrait (0.1-1.0). Default 0.6. */
	UPROPERTY(BlueprintReadWrite, Category = "Stash", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float TabletWidthRatioPortrait = 0.6f;

	/** Tablet height ratio for portrait (0.1-1.0). Default 0.8. */
	UPROPERTY(BlueprintReadWrite, Category = "Stash", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float TabletHeightRatioPortrait = 0.8f;

	/** Tablet width ratio for landscape (0.1-1.0). Default 0.8. */
	UPROPERTY(BlueprintReadWrite, Category = "Stash", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float TabletWidthRatioLandscape = 0.8f;

	/** Tablet height ratio for landscape (0.1-1.0). Default 0.65. */
	UPROPERTY(BlueprintReadWrite, Category = "Stash", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float TabletHeightRatioLandscape = 0.65f;
};

/**
 * Configuration for Stash Pay modal presentation.
 * 
 * Modal always appears centered on screen (unlike checkout which uses cards on phones).
 * Supports independent sizing for phone/tablet and portrait/landscape orientations.
 */
USTRUCT(BlueprintType)
struct FStashModalConfig
{
	GENERATED_BODY()

	/** Whether to show drag bar at top of modal. Default true. */
	UPROPERTY(BlueprintReadWrite, Category = "Stash")
	bool bShowDragBar = true;

	/** Whether tap outside and drag gestures can dismiss the modal. Default true. */
	UPROPERTY(BlueprintReadWrite, Category = "Stash")
	bool bAllowDismiss = true;

	/** Phone width ratio for portrait (0.1-1.0). Default 0.9. */
	UPROPERTY(BlueprintReadWrite, Category = "Stash", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float PhoneWidthRatioPortrait = 0.9f;

	/** Phone height ratio for portrait (0.1-1.0). Default 0.7. */
	UPROPERTY(BlueprintReadWrite, Category = "Stash", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float PhoneHeightRatioPortrait = 0.7f;

	/** Phone width ratio for landscape (0.1-1.0). Default 0.7. */
	UPROPERTY(BlueprintReadWrite, Category = "Stash", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float PhoneWidthRatioLandscape = 0.7f;

	/** Phone height ratio for landscape (0.1-1.0). Default 0.85. */
	UPROPERTY(BlueprintReadWrite, Category = "Stash", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float PhoneHeightRatioLandscape = 0.85f;

	/** Tablet width ratio for portrait (0.1-1.0). Default 0.6. */
	UPROPERTY(BlueprintReadWrite, Category = "Stash", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float TabletWidthRatioPortrait = 0.6f;

	/** Tablet height ratio for portrait (0.1-1.0). Default 0.7. */
	UPROPERTY(BlueprintReadWrite, Category = "Stash", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float TabletHeightRatioPortrait = 0.7f;

	/** Tablet width ratio for landscape (0.1-1.0). Default 0.5. */
	UPROPERTY(BlueprintReadWrite, Category = "Stash", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float TabletWidthRatioLandscape = 0.5f;

	/** Tablet height ratio for landscape (0.1-1.0). Default 0.8. */
	UPROPERTY(BlueprintReadWrite, Category = "Stash", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float TabletHeightRatioLandscape = 0.8f;
};

/**
 * Stash Pay Blueprint Function Library
 * 
 * Provides cross-platform functions for integrating Stash Pay checkout
 * into Unreal Engine projects on iOS and Android.
 */
UCLASS()
class STASH_API UStashBlueprint : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UStashBlueprint(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {};

	/**
	 * Opens the Stash Pay checkout dialog with default sizing.
	 * Works on both iOS and Android platforms.
	 * 
	 * @param CheckoutURL The URL to load in the checkout dialog
	 */
	UFUNCTION(BlueprintCallable, Category = "Stash")
	static void OpenCheckout(const FString& CheckoutURL);

	/**
	 * Opens the Stash Pay checkout dialog with custom sizing configuration.
	 * Works on both iOS and Android platforms.
	 * 
	 * @param CheckoutURL The URL to load in the checkout dialog
	 * @param Config Configuration for card/tablet sizing
	 */
	UFUNCTION(BlueprintCallable, Category = "Stash")
	static void OpenCheckoutWithConfig(const FString& CheckoutURL, const FStashCheckoutConfig& Config);

	/**
	 * Checks if the Stash Pay checkout dialog is currently open.
	 * Works on both iOS and Android platforms.
	 * 
	 * @return true if the checkout dialog is displayed
	 */
	UFUNCTION(BlueprintCallable, Category = "Stash")
	static bool IsCheckoutOpen();

	/**
	 * Dismisses the Stash Pay checkout dialog.
	 * Works on both iOS and Android platforms.
	 */
	UFUNCTION(BlueprintCallable, Category = "Stash")
	static void DismissCheckout();

	// ========================================================================
	// Modal Presentation (SDK 1.2.0+)
	// ========================================================================

	/**
	 * Opens a URL in a centered modal dialog with default configuration.
	 * Unlike OpenCheckout which uses different presentations on phones vs tablets,
	 * OpenModal always shows a centered modal on all devices.
	 * 
	 * @param URL The URL to load in the modal
	 */
	UFUNCTION(BlueprintCallable, Category = "Stash")
	static void OpenModal(const FString& URL);

	/**
	 * Opens a URL in a centered modal dialog with custom configuration.
	 * Unlike OpenCheckout which uses different presentations on phones vs tablets,
	 * OpenModal always shows a centered modal on all devices.
	 * 
	 * @param URL The URL to load in the modal
	 * @param Config Configuration for sizing, drag bar, and dismiss behavior
	 */
	UFUNCTION(BlueprintCallable, Category = "Stash")
	static void OpenModalWithConfig(const FString& URL, const FStashModalConfig& Config);

	// ========================================================================
	// Configuration (SDK 1.2.0+)
	// ========================================================================

	/**
	 * Sets whether to use web-based checkout (Safari/Chrome) instead of in-app UI.
	 * When enabled, checkout URLs open in SFSafariViewController (iOS) or Chrome Custom Tabs (Android).
	 * 
	 * @param bForce true to use web-based checkout, false for in-app UI
	 */
	UFUNCTION(BlueprintCallable, Category = "Stash")
	static void SetForceWebBasedCheckout(bool bForce);

	/**
	 * (iOS) When enabled, the app stays in landscape when checkout is not open; portrait is allowed only while Stash Pay checkout is displayed (required for phone checkout). Call at game startup for landscape-only games. No effect on Android.
	 * @param bEnable true to lock to landscape when checkout closed, false to use default orientations
	 */
	UFUNCTION(BlueprintCallable, Category = "Stash")
	static void SetLandscapeLockWhenCheckoutClosed(bool bEnable);

	// ========================================================================
	// Delegates - Bind to these to receive payment callbacks
	// ========================================================================
	
	/** Called when a payment completes successfully */
	static FOnStashPaymentSuccess OnPaymentSuccess;
	
	/** Called when a payment fails */
	static FOnStashPaymentFailure OnPaymentFailure;
	
	/** Called when the checkout dialog is dismissed by the user */
	static FOnStashDialogDismissed OnDialogDismissed;
	
	/** Called when an opt-in response is received (modal payment channel selection) */
	static FOnStashOptInResponse OnOptInResponse;
	
	/** Called when the checkout page finishes loading */
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
