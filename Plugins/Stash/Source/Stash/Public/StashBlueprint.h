// Copyright Stash. All Rights Reserved.
// Stash Pay Unreal Engine SDK - Blueprint Function Library

#pragma once

#include <Kismet/BlueprintFunctionLibrary.h>
#include <Runtime/Launch/Resources/Version.h>
#include <Async/Async.h>
#include <Engine.h>

#include "StashBlueprint.generated.h"

// Stash Pay Payment Callbacks
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStashPaymentSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStashPaymentFailure);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStashDialogDismissed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStashPageLoaded, float, LoadTimeMs);

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
	 * Opens the Stash Pay checkout dialog.
	 * Works on both iOS and Android platforms.
	 * 
	 * @param CheckoutURL The URL to load in the checkout dialog
	 */
	UFUNCTION(BlueprintCallable, Category = "Stash")
	static void OpenCheckout(const FString& CheckoutURL);

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

	// Stash Pay Delegates - Bind to these to receive payment callbacks
	
	/** Called when a payment completes successfully */
	static FOnStashPaymentSuccess OnPaymentSuccess;
	
	/** Called when a payment fails */
	static FOnStashPaymentFailure OnPaymentFailure;
	
	/** Called when the checkout dialog is dismissed by the user */
	static FOnStashDialogDismissed OnDialogDismissed;
	
	/** Called when the checkout page finishes loading */
	static FOnStashPageLoaded OnPageLoaded;
	
	// Internal callback functions called from native code
	static void HandlePaymentSuccess();
	static void HandlePaymentFailure();
	static void HandleDialogDismissed();
	static void HandlePageLoaded(float LoadTimeMs);
};
