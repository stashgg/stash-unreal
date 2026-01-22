// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MobileNativeCodeBlueprint.generated.h"

// Delegate for payment success events
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPaymentSuccess, const FString&, ItemName);

UCLASS()
class MOBILENATIVECODE_API UMobileNativeCodeBlueprint : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Delegate that fires when payment succeeds
	// Note: Static delegates can't use UPROPERTY, so Blueprint binding must be done in C++
	static FOnPaymentSuccess OnPaymentSuccess;
	
	// Android WebView Functions
	UFUNCTION(BlueprintCallable, Category = "Mobile Native Code|Android")
	static void OpenAndroidWebView(const FString& URL);

	UFUNCTION(BlueprintCallable, Category = "Mobile Native Code|Android")
	static void CloseAndroidWebView();

	UFUNCTION(BlueprintCallable, Category = "Mobile Native Code|Android")
	static bool IsAndroidWebViewOpen();

	// iOS WebView Functions
	UFUNCTION(BlueprintCallable, Category = "Mobile Native Code|iOS")
	static void OpenIOSWebView(const FString& URL);

	UFUNCTION(BlueprintCallable, Category = "Mobile Native Code|iOS")
	static void CloseIOSWebView();

	UFUNCTION(BlueprintCallable, Category = "Mobile Native Code|iOS")
	static bool IsIOSWebViewOpen();

	// StashPay Functions (Android)
	UFUNCTION(BlueprintCallable, Category = "Mobile Native Code|StashPay|Android")
	static void OpenStashPayCheckout(const FString& URL);

	UFUNCTION(BlueprintCallable, Category = "Mobile Native Code|StashPay|Android")
	static void DismissStashPayCheckout();

	UFUNCTION(BlueprintCallable, Category = "Mobile Native Code|StashPay|Android")
	static bool IsStashPayCheckoutOpen();

	// StashPay Functions (iOS)
	UFUNCTION(BlueprintCallable, Category = "Mobile Native Code|StashPay|iOS")
	static void OpenStashPayCheckoutIOS(const FString& URL);

	UFUNCTION(BlueprintCallable, Category = "Mobile Native Code|StashPay|iOS")
	static void DismissStashPayCheckoutIOS();

	UFUNCTION(BlueprintCallable, Category = "Mobile Native Code|StashPay|iOS")
	static bool IsStashPayCheckoutOpenIOS();

	// Internal function called from Java when payment succeeds
	// This is called via JNI from StashPayHelper.java
	UFUNCTION(Category = "Mobile Native Code|StashPay")
	static void NotifyPaymentSuccess(const FString& ItemName);
};
