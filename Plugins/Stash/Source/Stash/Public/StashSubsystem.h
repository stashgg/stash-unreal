// Copyright Stash. All Rights Reserved.
// Stash Unreal Engine SDK - Game Instance Subsystem for Blueprint-bindable delegates

#pragma once

#include <Subsystems/GameInstanceSubsystem.h>

#include "StashBlueprint.h"
#include "StashSubsystem.generated.h"

/**
 * Game Instance Subsystem that exposes Stash callbacks as BlueprintAssignable delegates.
 * This is the recommended callback surface for Blueprint and C++ (via GetStashSubsystem).
 *
 * In Blueprint: Get Stash Subsystem -> Assign / Add to the event you want.
 */
UCLASS()
class STASH_API UStashSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** Called when a payment completes successfully. Bind in Blueprint via Add On Payment Success. */
	UPROPERTY(BlueprintAssignable, Category = "Stash")
	FOnStashPaymentSuccess OnPaymentSuccess;

	/** Called when a payment fails. Bind in Blueprint via Add On Payment Failure. */
	UPROPERTY(BlueprintAssignable, Category = "Stash")
	FOnStashPaymentFailure OnPaymentFailure;

	/** Called when the card or modal is dismissed. Bind in Blueprint via Add On Dialog Dismissed. */
	UPROPERTY(BlueprintAssignable, Category = "Stash")
	FOnStashDialogDismissed OnDialogDismissed;

	/** Called when an opt-in response is received. Bind in Blueprint via Add On Opt In Response. */
	UPROPERTY(BlueprintAssignable, Category = "Stash")
	FOnStashOptInResponse OnOptInResponse;

	/** Called when the card/modal page finishes loading. Bind in Blueprint via Add On Page Loaded. */
	UPROPERTY(BlueprintAssignable, Category = "Stash")
	FOnStashPageLoaded OnPageLoaded;

	/** Called when a network error occurs. Bind in Blueprint via Add On Network Error. */
	UPROPERTY(BlueprintAssignable, Category = "Stash")
	FOnStashNetworkError OnNetworkError;

	/** Called when checkout requests an external payment URL. Bind in Blueprint via Add On External Payment. */
	UPROPERTY(BlueprintAssignable, Category = "Stash")
	FOnStashExternalPayment OnExternalPayment;
};
