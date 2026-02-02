# Stash SDK for Unreal Engine 4 (Legacy Support)

> **For Unreal Engine 5:**  
> This is a sample for Unreal Engine 4.27+. For new projects, we recommend using Unreal Engine 5 with our actively maintained SDK. See the [main branch](https://github.com/stashgg/stash-unreal) for UE5 support.

Seamlessly integrate the Stash Pay native in-app purchase dialog with Unreal Engine 4 using the Stash plugin.

This repository contains the Stash plugin configured for Stash Pay with **pre-built native SDKs** for iOS and Android. Follow the setup guide below to add Stash Pay integration to your Unreal Engine 4.27+ project.


## Prerequisites

Before using the Stash Pay popup in your Unreal Engine game, **you must set up your game server** to create Stash Pay checkout URLs using the Stash API. 

If you haven't already configured checkout URL generation on your backend, see our [Stash Pay Integration Guide](https://docs.stash.gg/guides/stash-pay/integration) for complete instructions.

### Integration Flow

The Stash Pay integration follows this workflow:

1. **Server generates checkout URL** - Your game server calls the Stash API to create a checkout link
2. **URL sent to game client** - The checkout URL is transmitted to the Unreal Engine client
3. **Game client displays checkout** - Client opens the Stash Pay dialog using this SDK
4. **Client listens to callbacks** - Game receives payment success/failure events
5. **Server verifies purchase** - Your backend validates the purchase via webhooks before granting items

This SDK handles steps 3-4 (client-side display and callbacks). You are responsible for implementing steps 1, 2, and 5 (server-side URL generation, delivery, and verification).

## Architecture

Integration uses a layered architecture to integrate native Stash Pay dialog functionality:

```
Unreal Engine (C++/Blueprints)
         ↓
    Stash Plugin (Wrapper)
         ↓
Pre-built Native SDKs (iOS/Android)
```

**Components:**

1. **Stash Plugin** - Unreal Engine plugin providing Blueprint and C++ access to the Stash Pay SDK. Located at `Plugins/Stash/`.
2. **Pre-built Native SDKs** - Pre-compiled iOS XCFramework and Android AAR bundled in `Plugins/Stash/Source/Stash/ThirdParty/`.

**Native SDK Version:** 1.2.1 ([stash-native releases](https://github.com/stashgg/stash-native/releases))

## Quick Setup

This repository includes pre-built native SDKs - no additional setup required. Simply clone and open in Unreal Engine 4.27+.

```bash
git clone https://github.com/stashgg/stash-unreal.git
cd stash-unreal
# Open StashUnreal4.uproject in Unreal Engine
```


## Setup in Clean / Existing Unreal Project

Follow these steps to integrate Stash Pay into your own Unreal Engine 4.27+ project:

### 1. Add Stash Plugin

Copy the `Plugins/Stash/` folder from this repository to your project's `Plugins/` directory (create it if needed). This includes:
- All wrapper code for iOS and Android
- Pre-built native SDKs in `ThirdParty/` directory

### 2. Enable Plugin

1. Open your project in Unreal Engine
2. Go to **Edit → Plugins → Installed → Mobile → Stash**
3. Enable the plugin and restart the editor

### 3. Verify Setup

1. Open the Level Blueprint
2. You should be able to call Stash Blueprint functions from the "Stash" category
3. Package for Android or iOS to test native integration

### 4. Using the SDK - Show Checkout & Listen to Callbacks

Once the plugin is set up, you can integrate Stash Pay checkout into your game using either C++ or Blueprints.

#### C++ Implementation

**Opening the Checkout:**

```cpp
#include "StashBlueprint.h"

void AYourPlayerController::OpenStashCheckout(const FString& CheckoutURL)
{
    // Cross-platform - works on both iOS and Android
    UStashBlueprint::OpenCheckout(CheckoutURL);
}
```

**Listening to Payment Callbacks:**

Bind to the payment delegates in your controller's `BeginPlay()` or initialization code:

```cpp
// In your PlayerController.h
UFUNCTION()
void OnStashPaymentSuccess();

UFUNCTION()
void OnStashPaymentFailure();

UFUNCTION()
void OnStashDialogDismissed();

// In your PlayerController.cpp BeginPlay()
void AYourPlayerController::BeginPlay()
{
    Super::BeginPlay();
    
    // Bind to Stash delegates
    UStashBlueprint::OnPaymentSuccess.AddDynamic(this, &AYourPlayerController::OnStashPaymentSuccess);
    UStashBlueprint::OnPaymentFailure.AddDynamic(this, &AYourPlayerController::OnStashPaymentFailure);
    UStashBlueprint::OnDialogDismissed.AddDynamic(this, &AYourPlayerController::OnStashDialogDismissed);
}

void AYourPlayerController::OnStashPaymentSuccess()
{
    UE_LOG(LogTemp, Log, TEXT("Stash payment succeeded!"));
    // Grant items to the player, show success UI, etc.
}

void AYourPlayerController::OnStashPaymentFailure()
{
    UE_LOG(LogTemp, Warning, TEXT("Stash payment failed"));
    // Handle payment failure
}

void AYourPlayerController::OnStashDialogDismissed()
{
    UE_LOG(LogTemp, Log, TEXT("Stash checkout dialog was dismissed"));
    // Handle dialog dismissal
}
```

**Using Modal Presentation (SDK 1.2.0+):**

```cpp
#include "StashBlueprint.h"

void AYourPlayerController::OpenStashModal(const FString& URL)
{
    // Open with default configuration
    UStashBlueprint::OpenModal(URL);
    
    // Or with custom configuration
    FStashModalConfig Config;
    Config.bShowDragBar = true;
    Config.bAllowDismiss = true;
    Config.PhoneWidthRatioPortrait = 0.95f;
    Config.PhoneHeightRatioPortrait = 0.8f;
    
    UStashBlueprint::OpenModalWithConfig(URL, Config);
}
```

**Opening Checkout with Custom Sizing (SDK 1.2.0+):**

```cpp
#include "StashBlueprint.h"

void AYourPlayerController::OpenCustomCheckout(const FString& CheckoutURL)
{
    // Open checkout with custom sizing in a single call
    FStashCheckoutConfig Config;
    Config.CardHeightRatioPortrait = 0.7f;       // Phone card height
    Config.TabletWidthRatioPortrait = 0.5f;      // Tablet width in portrait
    Config.TabletHeightRatioPortrait = 0.7f;     // Tablet height in portrait
    Config.TabletWidthRatioLandscape = 0.8f;     // Tablet width in landscape
    Config.TabletHeightRatioLandscape = 0.65f;   // Tablet height in landscape
    
    UStashBlueprint::OpenCheckoutWithConfig(CheckoutURL, Config);
}

// Use web-based checkout instead of in-app UI
UStashBlueprint::SetForceWebBasedCheckout(true);
```

**Handling Network Errors (SDK 1.2.0+):**

```cpp
// In BeginPlay(), add network error binding
UStashBlueprint::OnNetworkError.AddDynamic(this, &AYourPlayerController::OnStashNetworkError);

void AYourPlayerController::OnStashNetworkError()
{
    UE_LOG(LogTemp, Warning, TEXT("Network error during checkout load"));
    // Show offline message, retry option, etc.
}
```

#### Blueprint Implementation

**Opening the Checkout:**

1. Get the checkout URL from your server
2. Use the **Open Checkout** node from the Stash category
3. Connect the checkout URL string to the node

![Checkout Blueprint Example](.github/blueprint_checkout.png)

**Opening the Modal:**

1. Get the URL for the modal content
2. Use the **Open Modal** node from the Stash category
3. Connect the URL string to the node

![Modal Blueprint Example](.github/blueprint_modal.png)

**Listening to Payment Callbacks:**

The payment delegates are static and require C++ binding. To use them in Blueprints:

1. Create a C++ PlayerController that binds to the delegates (as shown above)
2. Create Blueprint implementable events that can be called from C++
3. Call your Blueprint events from the C++ callback handlers

**Example pattern:**

```cpp
// In your C++ PlayerController header
UFUNCTION(BlueprintImplementableEvent, Category = "Stash")
void OnPaymentSucceeded();

UFUNCTION(BlueprintImplementableEvent, Category = "Stash")
void OnPaymentFailed();

// In your C++ callback handlers
void AYourPlayerController::OnStashPaymentSuccess()
{
    OnPaymentSucceeded();  // Calls Blueprint event
}

void AYourPlayerController::OnStashPaymentFailure()
{
    OnPaymentFailed();  // Calls Blueprint event
}
```

Then implement `OnPaymentSucceeded` and `OnPaymentFailed` as events in your Blueprint.

**Checking Checkout Status:**

Use the **Is Checkout Open** node to check if the checkout dialog is currently displayed.

**Closing the Checkout:**

Use the **Dismiss Checkout** node to programmatically close the checkout dialog.

#### API Reference

**Core Functions:**

| Function | Description |
|----------|-------------|
| `OpenCheckout(CheckoutURL)` | Opens the Stash Pay checkout dialog with default sizing |
| `OpenCheckoutWithConfig(CheckoutURL, Config)` | Opens checkout with custom sizing configuration |
| `IsCheckoutOpen()` | Returns true if checkout is currently displayed |
| `DismissCheckout()` | Closes the checkout dialog |
| `SetForceWebBasedCheckout(bForce)` | Use Safari/Chrome instead of in-app UI |

**Modal Presentation (SDK 1.2.0+):**

| Function | Description |
|----------|-------------|
| `OpenModal(URL)` | Opens a URL in a centered modal dialog with default settings |
| `OpenModalWithConfig(URL, Config)` | Opens a URL in a centered modal with custom configuration |

Unlike `OpenCheckout` which uses different presentations on phones vs tablets, `OpenModal` always shows a centered modal on all devices.

**Delegates:**

| Delegate | Description |
|----------|-------------|
| `OnPaymentSuccess` | Called when payment completes successfully |
| `OnPaymentFailure` | Called when payment fails |
| `OnDialogDismissed` | Called when user dismisses the checkout |
| `OnPageLoaded(LoadTimeMs)` | Called when checkout page finishes loading |
| `OnNetworkError` | Called when a network error occurs during page load (SDK 1.2.0+) |

**FStashCheckoutConfig Properties:**

| Property | Default | Description |
|----------|---------|-------------|
| `CardHeightRatioPortrait` | 0.68 | Phone card height in portrait (68%) |
| `TabletWidthRatioPortrait` | 0.6 | Tablet card width in portrait (60%) |
| `TabletHeightRatioPortrait` | 0.8 | Tablet card height in portrait (80%) |
| `TabletWidthRatioLandscape` | 0.8 | Tablet card width in landscape (80%) |
| `TabletHeightRatioLandscape` | 0.65 | Tablet card height in landscape (65%) |

**FStashModalConfig Properties:**

| Property | Default | Description |
|----------|---------|-------------|
| `bShowDragBar` | true | Show visual drag bar at top of modal |
| `bAllowDismiss` | true | Allow tap-outside and drag gestures to dismiss |
| `PhoneWidthRatioPortrait` | 0.9 | Phone width in portrait (90%) |
| `PhoneHeightRatioPortrait` | 0.7 | Phone height in portrait (70%) |
| `PhoneWidthRatioLandscape` | 0.7 | Phone width in landscape (70%) |
| `PhoneHeightRatioLandscape` | 0.85 | Phone height in landscape (85%) |
| `TabletWidthRatioPortrait` | 0.6 | Tablet width in portrait (60%) |
| `TabletHeightRatioPortrait` | 0.7 | Tablet height in portrait (70%) |
| `TabletWidthRatioLandscape` | 0.5 | Tablet width in landscape (50%) |
| `TabletHeightRatioLandscape` | 0.8 | Tablet height in landscape (80%) |

## Requirements

- Unreal Engine 4.27+
- Visual Studio (for Windows/Android development)
- Xcode with iOS SDK (for iOS development)
- Android SDK (for Android builds)

## Key Files

**Public API:**
- `Plugins/Stash/Source/Stash/Public/StashBlueprint.h` - Blueprint function library with all Stash functions and delegates

**Wrapper Implementation:**
- `Plugins/Stash/Source/Stash/Private/IOS/ObjC/StashPayCardWrapper.mm` - iOS wrapper
- `Plugins/Stash/Source/Stash/Private/Android/Java/StashHelper.java` - Android wrapper

**Pre-built Native SDKs:**
- `Plugins/Stash/Source/Stash/ThirdParty/Android/StashPay.aar` - Android library
- `Plugins/Stash/Source/Stash/ThirdParty/iOS/StashPay.xcframework/` - iOS framework

## Updating the Native SDK

To update the pre-built native SDKs to a newer version:

1. Download the latest release from [stash-native releases](https://github.com/stashgg/stash-native/releases)
2. Replace the files in `Plugins/Stash/Source/Stash/ThirdParty/`:
   - Android: `StashPay.aar`
   - iOS: Extract `StashPay.xcframework.zip` to `iOS/StashPay.xcframework/`
3. Perform a clean rebuild of your project

## Documentation

- [Stash Native SDK](https://github.com/stashgg/stash-native) - Native SDK documentation
- [Stash Pay Docs](https://docs.stash.gg) - Official Stash Pay documentation

## Troubleshooting

If you encounter issues with the plugin, ensure:
1. The pre-built SDKs exist in `Plugins/Stash/Source/Stash/ThirdParty/`
2. You've done a clean rebuild after adding the plugin (`rm -rf Intermediate Binaries Saved/StagedBuilds`)
3. Your build targets the correct platform (Android arm64-v8a or iOS arm64)

### Common Issues

**Android build fails with missing class:**
- Ensure `StashPay.aar` exists in `ThirdParty/Android/`
- Check that ProGuard rules include `com.stash.**` classes

**iOS build fails with framework not found:**
- Ensure `StashPay.xcframework` directory exists in `ThirdParty/iOS/`
- Verify the framework contains the `ios-arm64` architecture

## Support

For Stash Pay integration issues, contact: developers@stash.gg
