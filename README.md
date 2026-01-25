# Stash SDK for Unreal Engine

> **Unreal Engine 4 Warning:**  
> We actively maintain Unreal 5 support, we have a sample usage for legacy Unreal Engine 4 (4.27-plus). For Unreal Engine 4 sample please use the `4.27-plus` branch of this repository as the wrapper implementation differs.

> **Warning:**  
> This is a preview version currently under development. Features, APIs, and workflow may change, and the integration process or plugin behavior is subject to updates.
>  

Seamlessly integrate the Stash Pay native in-app purchase dialog with Unreal Engine using the MobileNativeCode plugin and the Stash Pay Native SDK.
You can either explore the included Unreal Engine 5.0+ sample project in this repository, or follow the "Setup in Clean Unreal Project" guide below to add Stash Pay integration to your own Unreal 5 project.


## Demo

[Watch iOS Demo Video](.github/video/video_ios.webm)

*Click to view: Stash Pay checkout integration running on iOS in Unreal Engine*

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
MobileNativeCode Plugin (Wrapper)
         ↓
Stash Native SDK (iOS/Android)
```

**Components:**

1. **[MobileNativeCode](https://github.com/Sovahero/PluginMobileNativeCode)** - Base plugin providing native mobile functionality access to Stash SDK for Unreal Engine projects. Includes wrapper for Stash native SDK.
2. **[Stash Native SDK](https://github.com/stashgg/stash-native)** - Native iOS/Android implementation of the Stash Pay checkout dialog.

## Quick Setup (This Sample Project)

This sample project is UE 5.7. After cloning this repository, initialize the Git submodules:

```bash
git submodule update --init --recursive
```

This command will download the Stash Native SDK into the `Plugins/stash-native-main/` directory. Once complete, you can build for iOS or Android, provided you have a properly configured Unreal Engine iOS or Android development environment.

## Setup in Clean / Existing Unreal Project

Follow these steps to integrate Stash Pay into your own Unreal Engine 5.0+ project:

### 1. Add MobileNativeCode Plugin

Copy the `Plugins/MobileNativeCode/` folder from this repository to your project's `Plugins/` directory (create it if needed). This includes all necessary wrappers for native Stash Pay SDK.

### 2. Add Stash Native SDK

Add the Stash Native SDK as a Git submodule / folder in your project:

```bash
cd YourProject
git submodule add https://github.com/stashgg/stash-native.git Plugins/stash-native-main
```

> **Keep the folder structure as shown in this sample project.**  
> The MobileNativeCode plugin wrappers are configured to call into the Stash SDK located at `Plugins/stash-native-main`. If you change the folder name or location, you will need to update wrapper references accordingly in your C++ and Blueprint integration code.


### 3. Enable Plugin

1. Open your project in Unreal Engine
2. Go to **Edit → Plugins → Installed → Mobile → MobileNativeCode**
3. Enable the plugin and restart the editor

### 4. Verify Setup

1. Open the Level Blueprint
2. You should be able to call MobileNativeCode Blueprint functions
3. Package for Android or iOS to test native integration

### 5. Using the SDK - Show Checkout & Listen to Callbacks

Once the plugin is set up, you can integrate Stash Pay checkout into your game using either C++ or Blueprints.

#### C++ Implementation

**Opening the Checkout:**

```cpp
#include "MobileNativeCodeBlueprint.h"

void AYourPlayerController::OpenCheckout(const FString& CheckoutURL)
{
    #if PLATFORM_IOS
        UMobileNativeCodeBlueprint::OpenStashPayCheckoutIOS(CheckoutURL);
    #elif PLATFORM_ANDROID
        UMobileNativeCodeBlueprint::OpenStashPayCheckout(CheckoutURL);
    #endif
}
```

**Listening to Payment Callbacks:**

Bind to the payment success delegate in your controller's `BeginPlay()` or initialization code:

```cpp
// In your PlayerController.h
UFUNCTION()
void OnPaymentSuccessReceived(const FString& ItemName);

// In your PlayerController.cpp BeginPlay()
void AYourPlayerController::BeginPlay()
{
    Super::BeginPlay();
    
    // Bind to payment success delegate
    UMobileNativeCodeBlueprint::OnPaymentSuccess.AddDynamic(this, &AYourPlayerController::OnPaymentSuccessReceived);
}

// Handle payment success
void AYourPlayerController::OnPaymentSuccessReceived(const FString& ItemName)
{
    UE_LOG(LogTemp, Warning, TEXT("Payment succeeded for: %s"), *ItemName);
    
    // Grant items to the player
    // Show success UI
    // etc.
}
```

#### Blueprint Implementation

**Opening the Checkout:**

1. Get the checkout URL from your server
2. Use the **Open Stash Pay Checkout iOS** (iOS) or **Open Stash Pay Checkout** (Android) node
3. Connect the checkout URL string to the node


**Listening to Payment Callbacks:**

The `OnPaymentSuccess` delegate is static and requires C++ binding. To use it in Blueprints:

1. Create a C++ PlayerController that binds to the delegate (as shown in the C++ example above)
2. Create a Blueprint event or function that can be called from C++
3. Call your Blueprint event from the C++ callback handler

**Example pattern:**

```cpp
// In your C++ PlayerController
UFUNCTION(BlueprintImplementableEvent, Category = "Store")
void OnPaymentSucceeded(const FString& ItemName);

void AYourPlayerController::OnPaymentSuccessReceived(const FString& ItemName)
{
    // Call Blueprint event
    OnPaymentSucceeded(ItemName);
}
```

Then implement `OnPaymentSucceeded` as an event in your Blueprint to handle the purchase.

**Checking Checkout Status:**

Use the **Is Stash Pay Checkout Open iOS** (iOS) or **Is Stash Pay Checkout Open** (Android) node to check if the checkout dialog is currently displayed.

**Closing the Checkout:**

Use the **Dismiss Stash Pay Checkout iOS** (iOS) or **Dismiss Stash Pay Checkout** (Android) node to programmatically close the checkout dialog.

#### Complete Reference

For a complete implementation example, see:
- **C++ Example**: `Source/StashUnreal/MyPlayerController.cpp` (lines 784-799 for callbacks)
- **Blueprint Setup**: Blueprint instances of `BP_MyPlayerController` in the sample project

## Requirements

- Unreal Engine 5.0+
- Visual Studio (for Windows/Android development)
- Xcode with iOS SDK (for iOS development)
- Android SDK (for Android builds)

## Key Files

**Wrapper Implementation:**
- `Plugins/MobileNativeCode/Source/MobileNativeCode/Private/IOS/ObjC/StashPayCardWrapper.mm`
- `Plugins/MobileNativeCode/Source/MobileNativeCode/Private/Android/Java/` (Android wrappers)

**Native SDK:**
- `Plugins/stash-native-main/iOS/` - iOS implementation
- `Plugins/stash-native-main/Android/` - Android implementation

## Documentation

- [MobileNativeCode Plugin](https://github.com/Sovahero/PluginMobileNativeCode) - Base plugin documentation
- [Stash Native SDK](https://github.com/stashgg/stash-native) - Native SDK documentation
- [Stash Pay Docs](https://docs.stash.gg) - Official Stash Pay documentation

## Troubleshooting

If you encounter issues specifically with the **MobileNativeCode plugin** (e.g., plugin not loading, native calls failing, Android/iOS compilation errors), please refer to the [MobileNativeCode repository README](https://github.com/Sovahero/PluginMobileNativeCode) for detailed setup instructions and troubleshooting guidance.

## Support

For Stash Pay integration issues, contact: developers@stash.gg
