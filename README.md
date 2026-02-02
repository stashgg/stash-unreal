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

## Usage

The Stash SDK provides two presentation methods:

- **Checkout** - For Stash Pay checkout URLs (payment flows)
- **Modal** - For Stash Pay opt-in modals (payment channel selection)

Both methods work on iOS and Android and can be opened with default settings or custom configuration.

---

### How to Use Checkout

**Checkout** is used for displaying Stash Pay checkout URLs where users complete their purchase.

#### C++ Implementation

**Open with default settings:**

```cpp
#include "StashBlueprint.h"

void AYourPlayerController::OpenStashCheckout(const FString& CheckoutURL)
{
    UStashBlueprint::OpenCheckout(CheckoutURL);
}
```

**Open with custom settings:**

```cpp
void AYourPlayerController::OpenCustomCheckout(const FString& CheckoutURL)
{
    FStashCheckoutConfig Config;
    Config.CardHeightRatioPortrait = 0.7f;       // Phone card height
    Config.TabletWidthRatioPortrait = 0.5f;      // Tablet width in portrait
    Config.TabletHeightRatioPortrait = 0.7f;     // Tablet height in portrait
    Config.TabletWidthRatioLandscape = 0.8f;     // Tablet width in landscape
    Config.TabletHeightRatioLandscape = 0.65f;   // Tablet height in landscape
    
    UStashBlueprint::OpenCheckoutWithConfig(CheckoutURL, Config);
}
```

#### Blueprint Implementation

Use the **Open Checkout** or **Open Checkout With Config** nodes from the Stash category:

![Checkout Blueprint Example](.github/blueprint_checkout.png)

---

### How to Use Modal

**Modal** is used for displaying Stash Pay opt-in modals where users select their preferred payment channel.

#### C++ Implementation

**Open with default settings:**

```cpp
#include "StashBlueprint.h"

void AYourPlayerController::OpenStashModal(const FString& URL)
{
    UStashBlueprint::OpenModal(URL);
}
```

**Open with custom settings:**

```cpp
void AYourPlayerController::OpenCustomModal(const FString& URL)
{
    FStashModalConfig Config;
    Config.bShowDragBar = true;
    Config.bAllowDismiss = true;
    Config.PhoneWidthRatioPortrait = 0.95f;
    Config.PhoneHeightRatioPortrait = 0.8f;
    
    UStashBlueprint::OpenModalWithConfig(URL, Config);
}
```

#### Blueprint Implementation

Use the **Open Modal** or **Open Modal With Config** nodes from the Stash category:

![Modal Blueprint Example](.github/blueprint_modal.png)

---

### Listening to Callbacks

Bind to payment delegates to handle success, failure, and dismissal events.

#### C++ Implementation

```cpp
// In your PlayerController.h
UFUNCTION()
void OnStashPaymentSuccess();

UFUNCTION()
void OnStashPaymentFailure();

UFUNCTION()
void OnStashDialogDismissed();

UFUNCTION()
void OnStashNetworkError();

// In your PlayerController.cpp BeginPlay()
void AYourPlayerController::BeginPlay()
{
    Super::BeginPlay();
    
    UStashBlueprint::OnPaymentSuccess.AddDynamic(this, &AYourPlayerController::OnStashPaymentSuccess);
    UStashBlueprint::OnPaymentFailure.AddDynamic(this, &AYourPlayerController::OnStashPaymentFailure);
    UStashBlueprint::OnDialogDismissed.AddDynamic(this, &AYourPlayerController::OnStashDialogDismissed);
    UStashBlueprint::OnNetworkError.AddDynamic(this, &AYourPlayerController::OnStashNetworkError);
}

void AYourPlayerController::OnStashPaymentSuccess()
{
    UE_LOG(LogTemp, Log, TEXT("Payment succeeded!"));
    // Grant items to the player
}

void AYourPlayerController::OnStashPaymentFailure()
{
    UE_LOG(LogTemp, Warning, TEXT("Payment failed"));
    // Handle payment failure
}

void AYourPlayerController::OnStashDialogDismissed()
{
    UE_LOG(LogTemp, Log, TEXT("Dialog dismissed"));
    // Handle dialog dismissal
}

void AYourPlayerController::OnStashNetworkError()
{
    UE_LOG(LogTemp, Warning, TEXT("Network error"));
    // Show offline message
}
```

#### Blueprint Implementation

Delegates require C++ binding. To use them in Blueprints:

1. Create a C++ PlayerController that binds to the delegates (as shown above)
2. Create Blueprint implementable events in your C++ class
3. Call Blueprint events from C++ callback handlers

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

---

### API Reference

**Checkout Functions:**

| Function | Description |
|----------|-------------|
| `OpenCheckout(CheckoutURL)` | Opens checkout with default settings |
| `OpenCheckoutWithConfig(CheckoutURL, Config)` | Opens checkout with custom settings |
| `IsCheckoutOpen()` | Returns true if checkout is displayed |
| `DismissCheckout()` | Closes the checkout dialog |
| `SetForceWebBasedCheckout(bForce)` | Use Safari/Chrome instead of in-app UI |

**Modal Functions:**

| Function | Description |
|----------|-------------|
| `OpenModal(URL)` | Opens modal with default settings |
| `OpenModalWithConfig(URL, Config)` | Opens modal with custom settings |

**Delegates:**

| Delegate | Description |
|----------|-------------|
| `OnPaymentSuccess` | Called when payment completes successfully |
| `OnPaymentFailure` | Called when payment fails |
| `OnDialogDismissed` | Called when user dismisses the dialog |
| `OnPageLoaded(LoadTimeMs)` | Called when page finishes loading |
| `OnNetworkError` | Called when a network error occurs |

**FStashCheckoutConfig Properties:**

| Property | Default | Description |
|----------|---------|-------------|
| `CardHeightRatioPortrait` | 0.68 | Phone card height in portrait |
| `TabletWidthRatioPortrait` | 0.6 | Tablet width in portrait |
| `TabletHeightRatioPortrait` | 0.8 | Tablet height in portrait |
| `TabletWidthRatioLandscape` | 0.8 | Tablet width in landscape |
| `TabletHeightRatioLandscape` | 0.65 | Tablet height in landscape |

**FStashModalConfig Properties:**

| Property | Default | Description |
|----------|---------|-------------|
| `bShowDragBar` | true | Show visual drag bar at top |
| `bAllowDismiss` | true | Allow tap-outside to dismiss |
| `PhoneWidthRatioPortrait` | 0.9 | Phone width in portrait |
| `PhoneHeightRatioPortrait` | 0.7 | Phone height in portrait |
| `PhoneWidthRatioLandscape` | 0.7 | Phone width in landscape |
| `PhoneHeightRatioLandscape` | 0.85 | Phone height in landscape |
| `TabletWidthRatioPortrait` | 0.6 | Tablet width in portrait |
| `TabletHeightRatioPortrait` | 0.7 | Tablet height in portrait |
| `TabletWidthRatioLandscape` | 0.5 | Tablet width in landscape |
| `TabletHeightRatioLandscape` | 0.8 | Tablet height in landscape |

---

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
