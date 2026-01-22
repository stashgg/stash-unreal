// Fill out your copyright notice in the Description page of Project Settings.

#include "MyPlayerController.h"
#include "Components/InputComponent.h" // Include for UInputComponent - must be early for iOS builds
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "MobileNativeCodeBlueprint.h"
#include "HAL/PlatformProperties.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Json.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "PurchaseCelebrationManager.h"
#include "NiagaraSystem.h" // Include for UNiagaraSystem to access GetName() and GetPathName()
#include "Misc/ConfigCacheIni.h"

AMyPlayerController::AMyPlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, PurchaseCelebrationManager(nullptr)
	, StashPayOpenCheckCount(0)
{
	// Create the purchase celebration manager
	PurchaseCelebrationManager = CreateDefaultSubobject<UPurchaseCelebrationManager>(TEXT("PurchaseCelebrationManager"));
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	
	// Try to load API key from config file (alternative to Blueprint property)
	FString ConfigApiKey;
	if (GConfig->GetString(TEXT("/Script/StashUnreal.MyPlayerController"), TEXT("ApiAuthToken"), ConfigApiKey, GGameIni))
	{
		if (!ConfigApiKey.IsEmpty())
		{
			ApiAuthToken = ConfigApiKey;
			UE_LOG(LogTemp, Warning, TEXT("Loaded ApiAuthToken from config file (length: %d)"), ApiAuthToken.Len());
		}
	}
	
	// Try to load API URL from config file
	FString ConfigApiUrl;
	if (GConfig->GetString(TEXT("/Script/StashUnreal.MyPlayerController"), TEXT("CheckoutApiUrl"), ConfigApiUrl, GGameIni))
	{
		if (!ConfigApiUrl.IsEmpty())
		{
			CheckoutApiUrl = ConfigApiUrl;
			UE_LOG(LogTemp, Warning, TEXT("Loaded CheckoutApiUrl from config file: %s"), *CheckoutApiUrl);
		}
	}
	
	// Log constructor to verify class is being instantiated
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("AMyPlayerController CONSTRUCTOR CALLED!"));
	UE_LOG(LogTemp, Warning, TEXT("Class Name: %s"), *GetClass()->GetName());
	UE_LOG(LogTemp, Warning, TEXT("ApiAuthToken length: %d"), ApiAuthToken.Len());
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	
	if (GEngine)
	{
		FString ClassName = GetClass()->GetName();
		FString Message = FString::Printf(TEXT("MyPlayerController CONSTRUCTOR! Class: %s"), *ClassName);
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Magenta, Message);
	}
}

void AMyPlayerController::VerifyPlayerController()
{
	UE_LOG(LogTemp, Error, TEXT("VerifyPlayerController called! Class: %s"), *GetClass()->GetName());
	if (GEngine)
	{
		FString ClassName = GetClass()->GetName();
		FString Message = FString::Printf(TEXT("VerifyPlayerController! Class: %s"), *ClassName);
		GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Yellow, Message);
	}
}

void AMyPlayerController::BeginPlay()
{
	// Log BEFORE Super::BeginPlay to catch if it's called at all
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("AMyPlayerController::BeginPlay CALLED (BEFORE Super)"));
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Cyan, TEXT("BeginPlay CALLED (before Super)!"));
	}
	
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("AMyPlayerController::BeginPlay STARTED (AFTER Super)"));
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("World: %s"), GetWorld() ? TEXT("Valid") : TEXT("NULL"));
	UE_LOG(LogTemp, Warning, TEXT("PlayerController: %s"), *GetName());
	
	// Initialize celebration manager with properties from Player Controller
	if (PurchaseCelebrationManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("Initializing celebration manager..."));
		if (PurchaseCelebrationNiagara)
		{
			PurchaseCelebrationManager->SetCelebrationNiagara(PurchaseCelebrationNiagara);
			UE_LOG(LogTemp, Warning, TEXT("Set Niagara system on celebration manager"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("PurchaseCelebrationNiagara is NULL - set it in Blueprint!"));
		}
		if (PurchaseCelebrationWidgetClass)
		{
			PurchaseCelebrationManager->SetCelebrationWidgetClass(PurchaseCelebrationWidgetClass);
			UE_LOG(LogTemp, Warning, TEXT("Set Widget class on celebration manager"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("PurchaseCelebrationWidgetClass is NULL - set it in Blueprint!"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("PurchaseCelebrationManager is NULL!"));
	}
	
	// Bind to payment success delegate
	// Note: For dynamic multicast delegates, we use AddDynamic
	UMobileNativeCodeBlueprint::OnPaymentSuccess.AddDynamic(this, &AMyPlayerController::OnPaymentSuccessReceived);
	UE_LOG(LogTemp, Warning, TEXT("Bound to OnPaymentSuccess delegate"));
	
	// Add on-screen debug message
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("MyPlayerController::BeginPlay CALLED!"));
	}
	
	// Create and show the game menu using Widget Blueprint
	UE_LOG(LogTemp, Warning, TEXT("Creating GameMenuWidget from Widget Blueprint..."));
	
	// Use Widget Blueprint if set in editor, otherwise try to load WBP_GameMenu automatically
	if (GameMenuWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("Using Widget Blueprint from GameMenuWidgetClass: %s"), *GameMenuWidgetClass->GetName());
		GameMenuWidget = CreateWidget<UUserWidget>(this, GameMenuWidgetClass);
	}
	else
	{
		// Try to find WBP_GameMenu Widget Blueprint automatically
		UE_LOG(LogTemp, Warning, TEXT("GameMenuWidgetClass not set, trying to load WBP_GameMenu..."));
		
		FString WidgetBlueprintPath = TEXT("/Game/WBP_GameMenu.WBP_GameMenu_C");
		UClass* FoundWidgetClass = LoadClass<UUserWidget>(nullptr, *WidgetBlueprintPath);
		
		if (FoundWidgetClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("SUCCESS: Found WBP_GameMenu Widget Blueprint!"));
			GameMenuWidget = CreateWidget<UUserWidget>(this, FoundWidgetClass);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("ERROR: WBP_GameMenu Widget Blueprint not found!"));
			UE_LOG(LogTemp, Error, TEXT("Please create WBP_GameMenu Widget Blueprint or set GameMenuWidgetClass in Player Controller Blueprint."));
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("ERROR: WBP_GameMenu not found!"));
			}
			return; // Don't continue if we can't create the widget
		}
	}
	
	if (GameMenuWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("GameMenuWidget created successfully"));
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Blue, TEXT("GameMenuWidget Created!"));
		}
		
		// Add menu to viewport with high z-order
		UE_LOG(LogTemp, Warning, TEXT("Adding GameMenuWidget to viewport..."));
		GameMenuWidget->AddToViewport(9999); // Very high z-order
		GameMenuWidget->SetVisibility(ESlateVisibility::Visible);
		UE_LOG(LogTemp, Warning, TEXT("GameMenuWidget added to viewport, z-order 9999"));
		
		// Set input mode for UI interaction (mobile-friendly)
		UE_LOG(LogTemp, Warning, TEXT("Setting input mode for UI..."));
		FInputModeUIOnly InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		// Don't set widget to focus - it causes errors on mobile
		// The widget will still receive input without explicit focus
		SetInputMode(InputMode);
		UE_LOG(LogTemp, Warning, TEXT("Input mode set to UI Only"));
		
		// Show mouse cursor only on non-mobile platforms
		#if !PLATFORM_ANDROID && !PLATFORM_IOS
		SetShowMouseCursor(true);
		UE_LOG(LogTemp, Warning, TEXT("Mouse cursor shown (desktop platform)"));
		#else
		UE_LOG(LogTemp, Warning, TEXT("Mobile platform - no mouse cursor needed"));
		#endif
		
		// Verify widget is in viewport
		if (GameMenuWidget->IsInViewport())
		{
			UE_LOG(LogTemp, Warning, TEXT("CONFIRMED: GameMenuWidget is in viewport"));
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Green, TEXT("Widget is in viewport!"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("WARNING: GameMenuWidget is NOT in viewport!"));
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("Widget NOT in viewport!"));
			}
		}
		
		UE_LOG(LogTemp, Warning, TEXT("GameMenuWidget setup complete"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("========================================"));
		UE_LOG(LogTemp, Error, TEXT("CRITICAL: FAILED to create GameMenuWidget!"));
		UE_LOG(LogTemp, Error, TEXT("========================================"));
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("FAILED to create GameMenuWidget!"));
		}
	}
	
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("AMyPlayerController::BeginPlay COMPLETE"));
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
}

void AMyPlayerController::OpenStoreWebView()
{
	UE_LOG(LogTemp, Warning, TEXT("OpenStoreWebView called"));
	
	// Pause Unreal's input so WebView can receive all touch events
	// Disable the InputComponent to block all input
	if (InputComponent)
	{
		InputComponent->Deactivate();
	}
	
	// Remove the game menu widget from viewport entirely to prevent it from receiving input
	// This is necessary because Unreal processes input events before they reach the native Android layer
	if (GameMenuWidget && GameMenuWidget->IsInViewport())
	{
		GameMenuWidget->RemoveFromParent();
		UE_LOG(LogTemp, Warning, TEXT("GameMenuWidget removed from viewport to prevent input interference"));
	}
	
	// Set input mode to UI-only but don't set any widget to focus
	// This prevents Unreal from processing UI input events, allowing them to reach the native WebView
	// Since we removed the widget from viewport, no UI widgets can receive input anyway
	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	// Don't set widget to focus - this prevents any UI from receiving input
	SetInputMode(InputMode);
	
	UE_LOG(LogTemp, Warning, TEXT("Unreal input paused - WebView will receive all input"));
	
	// Create a store mockup HTML
	// The Java code will handle loading this as HTML content
	// Users can close the WebView using the Android back button
	FString StoreHTML = TEXT("<!DOCTYPE html><html><head><title>Game Store</title><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"><style>body{font-family:Arial;margin:0;padding:20px;background:#f5f5f5}.header{background:#fff;padding:20px;text-align:center;box-shadow:0 2px 4px rgba(0,0,0,0.1);margin-bottom:20px}.header h1{margin:0;color:#333}.products{display:grid;grid-template-columns:repeat(auto-fill,minmax(200px,1fr));gap:20px}.product{background:#fff;border-radius:8px;padding:15px;box-shadow:0 2px 4px rgba(0,0,0,0.1)}.product h3{margin-top:0;color:#333}.product p{color:#666}.product .price{color:#e74c3c;font-size:18px;font-weight:bold;margin:10px 0}.product button{background:#27ae60;color:white;border:none;padding:10px 20px;border-radius:5px;cursor:pointer;width:100%;font-size:16px}</style></head><body><div class=\"header\"><h1>Game Store</h1><p style=\"font-size:12px;color:#666;margin:5px 0\">Press back button to close</p></div><div class=\"products\"><div class=\"product\"><h3>Power Boost</h3><p>Increase power by 50%</p><p class=\"price\">$2.99</p><button onclick=\"alert('Purchased!')\">Buy Now</button></div><div class=\"product\"><h3>Health Pack</h3><p>Restore full health</p><p class=\"price\">$1.99</p><button onclick=\"alert('Purchased!')\">Buy Now</button></div><div class=\"product\"><h3>Speed Boost</h3><p>Move 2x faster</p><p class=\"price\">$3.99</p><button onclick=\"alert('Purchased!')\">Buy Now</button></div><div class=\"product\"><h3>Premium Pack</h3><p>All items + bonus</p><p class=\"price\">$9.99</p><button onclick=\"alert('Purchased!')\">Buy Now</button></div></div></body></html>");
	
	// Create data URL - the Java code will detect this and use loadDataWithBaseURL
	FString DataURL = FString::Printf(TEXT("data:text/html;charset=utf-8,%s"), *StoreHTML);
	
	// Use platform-specific WebView functions
#if PLATFORM_ANDROID
	UE_LOG(LogTemp, Warning, TEXT("Opening Android WebView with store mockup"));
	UMobileNativeCodeBlueprint::OpenAndroidWebView(DataURL);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Opening Store WebView..."));
	}
	
	// Start a timer to periodically check if WebView is still open
	// If it's closed (e.g., by back button), resume Unreal input
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(WebViewCheckTimer, this, &AMyPlayerController::CheckWebViewStatus, 0.5f, true);
	}
#elif PLATFORM_IOS
	UE_LOG(LogTemp, Warning, TEXT("Opening iOS WebView with store mockup"));
	UMobileNativeCodeBlueprint::OpenIOSWebView(DataURL);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Opening Store WebView..."));
	}
	
	// Start a timer to periodically check if WebView is still open
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(WebViewCheckTimer, this, &AMyPlayerController::CheckWebViewStatus, 0.5f, true);
	}
#else
	UE_LOG(LogTemp, Warning, TEXT("WebView not available on this platform"));
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("WebView only available on mobile platforms"));
	}
#endif
}

void AMyPlayerController::CheckWebViewStatus()
{
	// Check if StashPay checkout is still open
	bool bWebViewOpen = false;
	
#if PLATFORM_ANDROID
	bWebViewOpen = UMobileNativeCodeBlueprint::IsStashPayCheckoutOpen();
	UE_LOG(LogTemp, Warning, TEXT("CheckWebViewStatus: StashPay checkout open = %s (check count: %d)"), bWebViewOpen ? TEXT("true") : TEXT("false"), StashPayOpenCheckCount);
#elif PLATFORM_IOS
	bWebViewOpen = UMobileNativeCodeBlueprint::IsStashPayCheckoutOpenIOS();
	UE_LOG(LogTemp, Warning, TEXT("CheckWebViewStatus: StashPay checkout open = %s (check count: %d)"), bWebViewOpen ? TEXT("true") : TEXT("false"), StashPayOpenCheckCount);
#endif
	
	// If WebView reports as open, increment the check count
	if (bWebViewOpen)
	{
		StashPayOpenCheckCount++;
		
		// Safety timeout: Only force-close if StashPay has been reporting as open for a VERY long time
		// (60 checks = 30 seconds). This is a last-resort fallback in case the dismissal callback
		// doesn't fire. We use a long timeout to avoid closing StashPay while the player is legitimately using it.
		// The primary mechanism for detecting closure is the Java isDismissed flag set by onDialogDismissed().
		if (StashPayOpenCheckCount > 60)
		{
			UE_LOG(LogTemp, Warning, TEXT("StashPay has been reporting as open for 30+ seconds - forcing close and restoring UI (safety timeout)"));
			bWebViewOpen = false; // Force treat as closed
		}
	}
	else
	{
		// Reset counter when StashPay reports as closed
		StashPayOpenCheckCount = 0;
	}
	
	// If WebView is closed, resume Unreal input
	if (!bWebViewOpen)
	{
		UE_LOG(LogTemp, Warning, TEXT("WebView closed - resuming Unreal input"));
		
		// Reset the check count
		StashPayOpenCheckCount = 0;
		
		// Stop the timer
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(WebViewCheckTimer);
		}
		
		// Resume Unreal's input
		if (InputComponent)
		{
			InputComponent->Activate();
		}
		
		// Restore UI input mode
		FInputModeUIOnly InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
		
		// Re-add the game menu widget to viewport
		if (GameMenuWidget && !GameMenuWidget->IsInViewport())
		{
			GameMenuWidget->AddToViewport(9999); // Restore with high z-order
			GameMenuWidget->SetVisibility(ESlateVisibility::Visible);
			UE_LOG(LogTemp, Warning, TEXT("GameMenuWidget restored to viewport"));
		}
	}
}

void AMyPlayerController::CloseStoreWebView()
{
	UE_LOG(LogTemp, Warning, TEXT("CloseStoreWebView called"));
	
	// Stop the timer
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(WebViewCheckTimer);
	}
	
	// Resume Unreal's input
	if (InputComponent)
	{
		InputComponent->Activate();
	}
	
	// Restore UI input mode
	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	
	// Re-add the game menu widget to viewport
	if (GameMenuWidget && !GameMenuWidget->IsInViewport())
	{
		GameMenuWidget->AddToViewport(9999); // Restore with high z-order
		GameMenuWidget->SetVisibility(ESlateVisibility::Visible);
		UE_LOG(LogTemp, Warning, TEXT("GameMenuWidget restored to viewport"));
	}
	
	UE_LOG(LogTemp, Warning, TEXT("Unreal input resumed"));
	
	// Close the StashPay checkout
#if PLATFORM_ANDROID
	UMobileNativeCodeBlueprint::DismissStashPayCheckout();
#elif PLATFORM_IOS
	UMobileNativeCodeBlueprint::DismissStashPayCheckoutIOS();
#endif
}

void AMyPlayerController::RequestCheckoutAndOpenWebView(const FStoreItem& Item, const FStoreUser& User)
{
	UE_LOG(LogTemp, Warning, TEXT("RequestCheckoutAndOpenWebView called for item: %s"), *Item.Name);

	// Store the item name for celebration when payment succeeds
	CurrentPurchaseItemName = Item.Name;

	// Create JSON request body
	TSharedPtr<FJsonObject> RequestObj = MakeShareable(new FJsonObject);
	
	// Item object
	TSharedPtr<FJsonObject> ItemObj = MakeShareable(new FJsonObject);
	ItemObj->SetStringField(TEXT("id"), Item.Id);
	ItemObj->SetStringField(TEXT("pricePerItem"), Item.PricePerItem);
	ItemObj->SetNumberField(TEXT("quantity"), Item.Quantity);
	ItemObj->SetStringField(TEXT("imageUrl"), Item.ImageUrl);
	ItemObj->SetStringField(TEXT("name"), Item.Name);
	ItemObj->SetStringField(TEXT("description"), Item.Description);
	RequestObj->SetObjectField(TEXT("item"), ItemObj);

	// User object
	TSharedPtr<FJsonObject> UserObj = MakeShareable(new FJsonObject);
	UserObj->SetStringField(TEXT("id"), User.Id);
	UserObj->SetStringField(TEXT("validatedEmail"), User.ValidatedEmail);
	
	// ProfileImageUrl must be an absolute URL - use placeholder if empty
	FString ProfileImageUrl = User.ProfileImageUrl;
	if (ProfileImageUrl.IsEmpty())
	{
		ProfileImageUrl = TEXT("https://via.placeholder.com/150"); // Default placeholder image URL
	}
	UserObj->SetStringField(TEXT("profileImageUrl"), ProfileImageUrl);
	
	UserObj->SetStringField(TEXT("displayName"), User.DisplayName);
	UserObj->SetStringField(TEXT("regionCode"), User.RegionCode);
	UserObj->SetStringField(TEXT("currency"), User.Currency);
	UserObj->SetStringField(TEXT("platform"), User.Platform);
	UserObj->SetStringField(TEXT("payerIp"), User.PayerIp);
	RequestObj->SetObjectField(TEXT("user"), UserObj);

	// Add optional top-level fields for Stash API
	// regionCode - optional, use from user if available (can be at top level or in user object)
	if (!User.RegionCode.IsEmpty())
	{
		RequestObj->SetStringField(TEXT("regionCode"), User.RegionCode);
	}
	
	// currency - optional, use from user if available (can be at top level or in user object)
	if (!User.Currency.IsEmpty())
	{
		RequestObj->SetStringField(TEXT("currency"), User.Currency);
	}
	
	// transactionId - optional, can be empty string (not currently in FStoreUser struct)
	// If you need transactionId, add it to FStoreUser struct

	// Convert to JSON string
	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(RequestObj.ToSharedRef(), Writer);

	UE_LOG(LogTemp, Warning, TEXT("Sending checkout request to: %s"), *CheckoutApiUrl);
	UE_LOG(LogTemp, Warning, TEXT("Request body: %s"), *OutputString);
	
	// Log API key status (first 10 chars only for security)
	FString ApiKeyPreview = ApiAuthToken.Len() > 10 ? ApiAuthToken.Left(10) + TEXT("...") : ApiAuthToken;
	UE_LOG(LogTemp, Warning, TEXT("Using Stash API Token: %s (length: %d)"), *ApiKeyPreview, ApiAuthToken.Len());
	
	if (ApiAuthToken.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("ERROR: ApiAuthToken is empty! Please set it in Blueprint Class Defaults."));
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("ERROR: Stash API Token is not set!"));
		}
		return;
	}

	// Create HTTP request
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->OnProcessRequestComplete().BindUObject(this, &AMyPlayerController::OnCheckoutRequestComplete);
	Request->SetURL(CheckoutApiUrl);
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetHeader(TEXT("X-Stash-Api-Key"), ApiAuthToken);
	Request->SetContentAsString(OutputString);
	Request->ProcessRequest();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, TEXT("Requesting checkout URL..."));
	}
}

void AMyPlayerController::OnCheckoutRequestComplete(TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> Request, TSharedPtr<IHttpResponse, ESPMode::ThreadSafe> Response, bool bWasSuccessful)
{
	if (!bWasSuccessful || !Response.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Checkout request failed!"));
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Failed to get checkout URL"));
		}
		return;
	}

	int32 ResponseCode = Response->GetResponseCode();
	FString ResponseContent = Response->GetContentAsString();

	UE_LOG(LogTemp, Warning, TEXT("Checkout response code: %d"), ResponseCode);
	UE_LOG(LogTemp, Warning, TEXT("Checkout response: %s"), *ResponseContent);

	if (ResponseCode != 200)
	{
		UE_LOG(LogTemp, Error, TEXT("Checkout request returned error code: %d"), ResponseCode);
		if (GEngine)
		{
			FString ErrorMsg = FString::Printf(TEXT("Checkout failed: %d"), ResponseCode);
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, ErrorMsg);
		}
		return;
	}

	// Parse JSON response
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON response"));
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Failed to parse server response"));
		}
		return;
	}

	// Extract URL from response
	FString CheckoutUrl;
	if (JsonObject->TryGetStringField(TEXT("url"), CheckoutUrl))
	{
		UE_LOG(LogTemp, Warning, TEXT("Got checkout URL: %s"), *CheckoutUrl);
		OpenWebViewWithURL(CheckoutUrl);
	}
	else if (JsonObject->TryGetStringField(TEXT("checkoutUrl"), CheckoutUrl))
	{
		UE_LOG(LogTemp, Warning, TEXT("Got checkout URL (checkoutUrl field): %s"), *CheckoutUrl);
		OpenWebViewWithURL(CheckoutUrl);
	}
	else
	{
		// Try to get the URL if the response is just a string
		if (ResponseContent.StartsWith(TEXT("http://")) || ResponseContent.StartsWith(TEXT("https://")))
		{
			CheckoutUrl = ResponseContent.TrimStartAndEnd();
			UE_LOG(LogTemp, Warning, TEXT("Got checkout URL from response string: %s"), *CheckoutUrl);
			OpenWebViewWithURL(CheckoutUrl);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("No URL found in response"));
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("No checkout URL in response"));
			}
		}
	}
}

void AMyPlayerController::OpenWebViewWithURL(const FString& URL)
{
	if (URL.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot open WebView: URL is empty"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("OpenWebViewWithURL called with URL: %s"), *URL);

	// Reset the check count when opening StashPay
	StashPayOpenCheckCount = 0;

	// Pause Unreal's input so WebView can receive all touch events
	if (InputComponent)
	{
		InputComponent->Deactivate();
	}

	// Remove the game menu widget from viewport entirely to prevent it from receiving input
	if (GameMenuWidget && GameMenuWidget->IsInViewport())
	{
		GameMenuWidget->RemoveFromParent();
		UE_LOG(LogTemp, Warning, TEXT("GameMenuWidget removed from viewport to prevent input interference"));
	}

	// Set input mode to UI-only but don't set any widget to focus
	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);

	UE_LOG(LogTemp, Warning, TEXT("Unreal input paused - WebView will receive all input"));

	// Open StashPay Checkout with the URL
#if PLATFORM_ANDROID
	UE_LOG(LogTemp, Warning, TEXT("Opening StashPay Checkout with URL: %s"), *URL);
	UMobileNativeCodeBlueprint::OpenStashPayCheckout(URL);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Opening StashPay Checkout..."));
	}

	// Start a timer to periodically check if checkout is still open
	// Delay the first check by 2 seconds to give StashPay time to open and initialize
	if (UWorld* World = GetWorld())
	{
		// Clear any existing timer first
		World->GetTimerManager().ClearTimer(WebViewCheckTimer);
		// Start checking after 2 seconds, then every 0.5 seconds
		World->GetTimerManager().SetTimer(WebViewCheckTimer, this, &AMyPlayerController::CheckWebViewStatus, 0.5f, true, 2.0f);
	}
#elif PLATFORM_IOS
	UE_LOG(LogTemp, Warning, TEXT("Opening StashPay Checkout iOS with URL: %s"), *URL);
	UMobileNativeCodeBlueprint::OpenStashPayCheckoutIOS(URL);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Opening StashPay Checkout..."));
	}

	// Start a timer to periodically check if checkout is still open
	// Delay the first check by 2 seconds to give StashPay time to open and initialize
	if (UWorld* World = GetWorld())
	{
		// Clear any existing timer first
		World->GetTimerManager().ClearTimer(WebViewCheckTimer);
		// Start checking after 2 seconds, then every 0.5 seconds
		World->GetTimerManager().SetTimer(WebViewCheckTimer, this, &AMyPlayerController::CheckWebViewStatus, 0.5f, true, 2.0f);
	}
#else
	UE_LOG(LogTemp, Warning, TEXT("StashPay not available on this platform (PLATFORM_ANDROID=%d, PLATFORM_IOS=%d)"), 
		PLATFORM_ANDROID ? 1 : 0, PLATFORM_IOS ? 1 : 0);
	if (GEngine)
	{
		FString PlatformMsg = FString::Printf(TEXT("StashPay only works on Android/iOS devices. Current platform detection: Android=%d, iOS=%d"), 
			PLATFORM_ANDROID ? 1 : 0, PLATFORM_IOS ? 1 : 0);
		GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Yellow, PlatformMsg);
	}
	
	// Restore the widget since we can't open StashPay
	if (GameMenuWidget && !GameMenuWidget->IsInViewport())
	{
		GameMenuWidget->AddToViewport(9999);
		GameMenuWidget->SetVisibility(ESlateVisibility::Visible);
		UE_LOG(LogTemp, Warning, TEXT("GameMenuWidget restored - StashPay not available in editor"));
	}
	
	// Resume input
	if (InputComponent)
	{
		InputComponent->Activate();
	}
#endif
}

void AMyPlayerController::ShowPurchaseCelebration(const FString& ItemName)
{
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("ShowPurchaseCelebration CALLED"));
	UE_LOG(LogTemp, Warning, TEXT("ItemName: %s"), *ItemName);
	UE_LOG(LogTemp, Warning, TEXT("PurchaseCelebrationManager: %s"), PurchaseCelebrationManager ? TEXT("VALID") : TEXT("NULL"));
	
	// Log property values BEFORE checking manager
	UE_LOG(LogTemp, Warning, TEXT("PurchaseCelebrationNiagara pointer: %p"), PurchaseCelebrationNiagara);
	if (PurchaseCelebrationNiagara)
	{
		UE_LOG(LogTemp, Warning, TEXT("PurchaseCelebrationNiagara: %s"), *PurchaseCelebrationNiagara->GetName());
		UE_LOG(LogTemp, Warning, TEXT("PurchaseCelebrationNiagara path: %s"), *PurchaseCelebrationNiagara->GetPathName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("PurchaseCelebrationNiagara is NULL!"));
		// Try to get from CDO as fallback
		if (AMyPlayerController* CDO = GetClass()->GetDefaultObject<AMyPlayerController>())
		{
			if (CDO->PurchaseCelebrationNiagara)
			{
				UE_LOG(LogTemp, Warning, TEXT("Found Niagara in CDO, using it: %s"), *CDO->PurchaseCelebrationNiagara->GetName());
				PurchaseCelebrationNiagara = CDO->PurchaseCelebrationNiagara;
			}
		}
	}
	
	UE_LOG(LogTemp, Warning, TEXT("PurchaseCelebrationWidgetClass pointer: %p"), PurchaseCelebrationWidgetClass.Get());
	if (PurchaseCelebrationWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("PurchaseCelebrationWidgetClass: %s"), *PurchaseCelebrationWidgetClass->GetName());
		UE_LOG(LogTemp, Warning, TEXT("PurchaseCelebrationWidgetClass path: %s"), *PurchaseCelebrationWidgetClass->GetPathName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("PurchaseCelebrationWidgetClass is NULL!"));
		// Try to get from CDO as fallback
		if (AMyPlayerController* CDO = GetClass()->GetDefaultObject<AMyPlayerController>())
		{
			if (CDO->PurchaseCelebrationWidgetClass)
			{
				UE_LOG(LogTemp, Warning, TEXT("Found Widget Class in CDO, using it: %s"), *CDO->PurchaseCelebrationWidgetClass->GetName());
				PurchaseCelebrationWidgetClass = CDO->PurchaseCelebrationWidgetClass;
			}
		}
	}
	
	if (!PurchaseCelebrationManager)
	{
		UE_LOG(LogTemp, Error, TEXT("ShowPurchaseCelebration: PurchaseCelebrationManager is NULL!"));
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("ERROR: Celebration manager not initialized!"));
		}
		return;
	}

	// Store the item name for Blueprint access
	CurrentPurchaseItemName = ItemName;

	// Update manager with current properties (in case they were changed in Blueprint)
	if (PurchaseCelebrationManager)
	{
		if (PurchaseCelebrationNiagara)
		{
			PurchaseCelebrationManager->SetCelebrationNiagara(PurchaseCelebrationNiagara);
			UE_LOG(LogTemp, Warning, TEXT("Updated manager with Niagara system: %s"), *PurchaseCelebrationNiagara->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("PurchaseCelebrationNiagara is NULL on Player Controller!"));
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("ERROR: Niagara not set in Blueprint!"));
			}
		}

		if (PurchaseCelebrationWidgetClass)
		{
			PurchaseCelebrationManager->SetCelebrationWidgetClass(PurchaseCelebrationWidgetClass);
			UE_LOG(LogTemp, Warning, TEXT("Updated manager with Widget class"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("PurchaseCelebrationWidgetClass is NULL on Player Controller!"));
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("ERROR: Widget class not set in Blueprint!"));
			}
		}
	}

	// Delegate to the celebration manager
	PurchaseCelebrationManager->ShowCelebration(GetWorld(), ItemName);
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
}

void AMyPlayerController::OnPaymentSuccessReceived(const FString& ItemName)
{
	UE_LOG(LogTemp, Warning, TEXT("OnPaymentSuccessReceived: Payment succeeded for item: %s"), *ItemName);
	
	// Use the provided item name, or fall back to current purchase item name
	FString ItemNameToUse = ItemName.IsEmpty() ? CurrentPurchaseItemName : ItemName;
	
	if (ItemNameToUse.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("OnPaymentSuccessReceived: Item name is empty, using default"));
		ItemNameToUse = TEXT("Item");
	}
	
	// Show the celebration
	ShowPurchaseCelebration(ItemNameToUse);
}

void AMyPlayerController::TestPurchaseCelebration()
{
	UE_LOG(LogTemp, Warning, TEXT("TestPurchaseCelebration called - manually testing celebration"));
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, TEXT("Testing Purchase Celebration..."));
	}
	ShowPurchaseCelebration(TEXT("Test Item"));
}


