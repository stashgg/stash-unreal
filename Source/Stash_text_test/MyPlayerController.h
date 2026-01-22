// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Templates/SharedPointer.h"
#include "MyPlayerController.generated.h"

// Forward declarations for HTTP types
class IHttpRequest;
class IHttpResponse;

// Forward declarations for celebration
class UPurchaseCelebrationManager;
class UNiagaraSystem;

// Struct for store item data
USTRUCT(BlueprintType)
struct FStoreItem
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Store")
	FString Id;

	UPROPERTY(BlueprintReadWrite, Category = "Store")
	FString Name;

	UPROPERTY(BlueprintReadWrite, Category = "Store")
	FString PricePerItem;

	UPROPERTY(BlueprintReadWrite, Category = "Store")
	int32 Quantity;

	UPROPERTY(BlueprintReadWrite, Category = "Store")
	FString ImageUrl;

	UPROPERTY(BlueprintReadWrite, Category = "Store")
	FString Description;

	FStoreItem()
		: Quantity(1)
	{
	}
};

// Struct for user data
USTRUCT(BlueprintType)
struct FStoreUser
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Store")
	FString Id;

	UPROPERTY(BlueprintReadWrite, Category = "Store")
	FString ValidatedEmail;

	UPROPERTY(BlueprintReadWrite, Category = "Store")
	FString ProfileImageUrl;

	UPROPERTY(BlueprintReadWrite, Category = "Store")
	FString DisplayName;

	UPROPERTY(BlueprintReadWrite, Category = "Store")
	FString RegionCode;

	UPROPERTY(BlueprintReadWrite, Category = "Store")
	FString Currency;

	UPROPERTY(BlueprintReadWrite, Category = "Store")
	FString Platform;

	UPROPERTY(BlueprintReadWrite, Category = "Store")
	FString PayerIp;
};

/**
 * Player Controller that displays the game menu on BeginPlay
 */
UCLASS(Blueprintable, BlueprintType)
class STASH_PAY_UNREAL_DEMO_API AMyPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMyPlayerController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;

	// Blueprint-callable function to verify this class is being used
	UFUNCTION(BlueprintCallable, Category = "Debug")
	void VerifyPlayerController();

	// Blueprint-callable function to open the store WebView
	UFUNCTION(BlueprintCallable, Category = "Store")
	void OpenStoreWebView();

	// Blueprint-callable function to close the store WebView
	UFUNCTION(BlueprintCallable, Category = "Store")
	void CloseStoreWebView();

	// Blueprint-callable function to request checkout URL from API and open WebView
	// This is the main function buttons should call
	UFUNCTION(BlueprintCallable, Category = "Store")
	void RequestCheckoutAndOpenWebView(const FStoreItem& Item, const FStoreUser& User);

	// Blueprint-callable function to open WebView with a specific URL
	UFUNCTION(BlueprintCallable, Category = "Store")
	void OpenWebViewWithURL(const FString& URL);

	// Blueprint-callable function to show purchase celebration effect
	// Call this when payment succeeds to show particle effect and text
	UFUNCTION(BlueprintCallable, Category = "Store")
	void ShowPurchaseCelebration(const FString& ItemName);
	
	// Test function to manually trigger celebration (for debugging)
	UFUNCTION(BlueprintCallable, Category = "Store|Debug")
	void TestPurchaseCelebration();
	
	// Gets the purchase celebration manager (for Blueprint access)
	UFUNCTION(BlueprintPure, Category = "Store")
	UPurchaseCelebrationManager* GetPurchaseCelebrationManager() const { return PurchaseCelebrationManager; }

protected:
	// The Widget Blueprint class to use for the game menu (e.g., WBP_GameMenu)
	// Set this in a Blueprint based on MyPlayerController, or it will try to load WBP_GameMenu automatically
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<class UUserWidget> GameMenuWidgetClass;

	// Server API URL (can be configured in Blueprint)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Store")
	FString CheckoutApiUrl = TEXT("http://localhost:3000/api/checkout");

	// Niagara system for purchase celebration (set in Blueprint)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Store|Celebration")
	class UNiagaraSystem* PurchaseCelebrationNiagara;

	// Widget class for purchase celebration (set in Blueprint)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Store|Celebration")
	TSubclassOf<class UUserWidget> PurchaseCelebrationWidgetClass;

private:
	// Purchase celebration manager (handles all celebration effects)
	// This is created automatically and doesn't need to be set in Blueprint
	UPROPERTY()
	class UPurchaseCelebrationManager* PurchaseCelebrationManager;

private:
	// The game menu widget instance
	UPROPERTY()
	class UUserWidget* GameMenuWidget;
	
	// Timer handle for checking if WebView is still open
	FTimerHandle WebViewCheckTimer;
	
	// Counter to track how long StashPay has been reporting as "open"
	// Used to detect stale state when StashPay is closed but still reports as open
	int32 StashPayOpenCheckCount;
	
	// Store the current item being purchased (for celebration when payment succeeds)
	FString CurrentPurchaseItemName;
	
	// Function to check if WebView is closed and resume input if needed
	void CheckWebViewStatus();

	// HTTP request callback handlers
	void OnCheckoutRequestComplete(TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> Request, TSharedPtr<IHttpResponse, ESPMode::ThreadSafe> Response, bool bWasSuccessful);
	
	// Delegate callback for payment success
	UFUNCTION()
	void OnPaymentSuccessReceived(const FString& ItemName);
};
