# Stash SDK for Unreal Engine 4 (Legacy Support)

> **For Unreal Engine 5:**  
> This is a sample for Unreal Engine 4.27+. For new projects, we recommend using Unreal Engine 5 with our actively maintained SDK. See the [main branch](https://github.com/stashgg/stash-unreal) for UE5 support.

Seamlessly integrate the Stash Pay native in-app purchase dialog with Unreal Engine 4 using the Stash plugin and the Stash Native SDK.

This repository contains the Stash plugin configured for Stash Pay. Follow the setup guide below to add Stash Pay integration to your Unreal Engine 4.27+ project.


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
Stash Native SDK (iOS/Android)
```

**Components:**

1. **Stash Plugin** - Unreal Engine plugin providing Blueprint and C++ access to the Stash Native SDK. Located at `Plugins/Stash/`.
2. **[Stash Native SDK](https://github.com/stashgg/stash-native)** - Native iOS/Android implementation of the Stash Pay checkout dialog.

## Quick Setup

After cloning this repository, initialize the Git submodules:

```bash
git submodule update --init --recursive
```

This command will download the Stash Native SDK into the `Plugins/stash-native-main/` directory. The repository includes a minimal UE 4.27+ project configured with the Stash plugin ready to use.


## Setup in Clean / Existing Unreal Project

Follow these steps to integrate Stash Pay into your own Unreal Engine 4.27+ project:

### 1. Add Stash Plugin

Copy the `Plugins/Stash/` folder from this repository to your project's `Plugins/` directory (create it if needed). This includes all necessary wrappers for the Stash Native SDK.

### 2. Add Stash Native SDK

Add the Stash Native SDK as a Git submodule / folder in your project:

```bash
cd YourProject
git submodule add https://github.com/stashgg/stash-native.git Plugins/stash-native-main
```

> **Keep the folder structure as shown in this sample project.**  
> The Stash plugin wrappers are configured to call into the Stash SDK located at `Plugins/stash-native-main`. If you change the folder name or location, you will need to update wrapper references accordingly.

### 3. Enable Plugin

1. Open your project in Unreal Engine
2. Go to **Edit → Plugins → Installed → Mobile → Stash**
3. Enable the plugin and restart the editor

### 4. Verify Setup

1. Open the Level Blueprint
2. You should be able to call Stash Blueprint functions from the "Stash" category
3. Package for Android or iOS to test native integration

### 5. Using the SDK - Show Checkout & Listen to Callbacks

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

#### Blueprint Implementation

**Opening the Checkout:**

1. Get the checkout URL from your server
2. Use the **Open Checkout** node from the Stash category
3. Connect the checkout URL string to the node

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

| Function | Description |
|----------|-------------|
| `OpenCheckout(CheckoutURL)` | Opens the Stash Pay checkout dialog (cross-platform) |
| `IsCheckoutOpen()` | Returns true if checkout is currently displayed |
| `DismissCheckout()` | Closes the checkout dialog |

| Delegate | Description |
|----------|-------------|
| `OnPaymentSuccess` | Called when payment completes successfully |
| `OnPaymentFailure` | Called when payment fails |
| `OnDialogDismissed` | Called when user dismisses the checkout |
| `OnPageLoaded(LoadTimeMs)` | Called when checkout page finishes loading |

## Requirements

- Unreal Engine 4.27-plus
- Visual Studio (for Windows/Android development)
- Xcode with iOS SDK (for iOS development)
- Android SDK (for Android builds)

## Key Files

**Public API:**
- `Plugins/Stash/Source/Stash/Public/StashBlueprint.h` - Blueprint function library with all Stash functions and delegates

**Wrapper Implementation:**
- `Plugins/Stash/Source/Stash/Private/IOS/ObjC/StashPayCardWrapper.mm` - iOS wrapper
- `Plugins/Stash/Source/Stash/Private/Android/Java/StashHelper.java` - Android wrapper

**Native SDK:**
- `Plugins/stash-native-main/iOS/` - iOS implementation
- `Plugins/stash-native-main/Android/` - Android implementation

## Documentation

- [Stash Native SDK](https://github.com/stashgg/stash-native) - Native SDK documentation
- [Stash Pay Docs](https://docs.stash.gg) - Official Stash Pay documentation

## Troubleshooting

If you encounter issues with the plugin, ensure:
1. The `Plugins/stash-native-main/` submodule is properly initialized
2. You've done a clean rebuild after adding the plugin
3. Your build targets the correct platform (Android arm64-v8a or iOS)

## Support

For Stash Pay integration issues, contact: developers@stash.gg
