# Stash for Unreal Engine


<p align="left">
  <img src="https://github.com/stashgg/stash-native/raw/main/.github/assets/stash_unreal.png" width="128" height="128" alt="Stash Unreal Logo"/>
</p>


Seamlessly integrate the Stash Pay native in-app purchase dialog with Unreal Engine using the Stash plugin and the Stash Pay Native SDK.
You can either explore the included Unreal Engine 5.0+ sample project in this repository, or follow the "Setup in Clean Unreal Project" guide below to add Stash Pay integration to your own Unreal 5 project.

## Requirements

- Unreal Engine 5+
- iOS 12.0+ / Android API 21+

> **Unreal Engine 4 Warning:**  
> We actively maintain Unreal 5 support, we have a sample usage for legacy Unreal Engine 4 (4.27-plus). For Unreal Engine 4 sample please use the [`4.27-plus` branch](https://github.com/stashgg/stash-unreal/tree/4.27-plus) of this repository as the wrapper implementation differs.

## Sample Apps

[Watch iOS Demo Video](.github/video/video_ios.webm)

This repository include a sample scene with bluperints to call stash-native methods. Build locally and try on both platforms. Due to
restrictions we currently dont have online-hosted emulator with Unreal demo.

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

1. **Stash Plugin** (`Plugins/Stash/`) - Unreal plugin in this repository. Provides Blueprint and C++ API and wraps the Stash Pay native SDK for iOS and Android.
2. **[Stash Native SDK](https://github.com/stashgg/stash-native)** - Native iOS/Android implementation of the Stash Pay checkout dialog (bundled in the plugin’s `ThirdParty` folder; source in `Plugins/stash-native-main` when added as submodule).


## Quick Start

Follow these steps to integrate Stash Pay into your own Unreal Engine 5.0+ project:

### 1. Add Stash Plugin

Copy the `Plugins/Stash/` folder from this repository into your project’s `Plugins/` directory (create it if needed). The plugin includes the Blueprint/C++ wrapper and the Stash Pay native SDK (iOS/Android) in its `ThirdParty` folder.

### 2. (Optional) Add Stash Native SDK source

To work with or rebuild the native SDK from source, add it as a Git submodule:

```bash
cd YourProject
git submodule add https://github.com/stashgg/stash-native.git Plugins/stash-native-main
```

> **Folder layout.**  
> This sample project uses `Plugins/Stash/` for the Unreal plugin and optionally `Plugins/stash-native-main` for the native SDK source. The Stash plugin ships with pre-built SDKs in `Plugins/Stash/Source/Stash/ThirdParty/`; the submodule is only needed if you modify the native SDK.

### 3. Enable Plugin

1. Open your project in Unreal Engine
2. Go to **Edit → Plugins**, search for **Stash**
3. Enable the **Stash** plugin and restart the editor

### 4. Verify Setup

1. Open a Level Blueprint or any Blueprint
2. You should see **Stash** category and nodes (e.g. **Open Checkout**, **Is Checkout Open**, **Dismiss Checkout**)
3. Package for Android or iOS to test native integration

### 5. Using the SDK - Show Checkout & Listen to Callbacks

Once the plugin is set up, you can integrate Stash Pay checkout into your game using either C++ or Blueprints.

#### C++ Implementation

Add the **Stash** module to your module’s `Build.cs` (e.g. `PublicDependencyModuleNames.Add("Stash");`), then:

**Opening the Checkout:**

```cpp
#include "StashBlueprint.h"

void AYourPlayerController::OpenCheckout(const FString& CheckoutURL)
{
    UStashBlueprint::OpenCheckout(CheckoutURL);  // Works on both iOS and Android
}
```

**Listening to Payment Callbacks:**

Bind to the payment success delegate in your controller’s `BeginPlay()` or initialization code. The delegate has no parameters; use your server or session state to know what was purchased.

```cpp
// In your PlayerController.h
UFUNCTION()
void OnPaymentSuccessReceived();

// In your PlayerController.cpp BeginPlay()
void AYourPlayerController::BeginPlay()
{
    Super::BeginPlay();
    
    UStashBlueprint::OnPaymentSuccess.AddDynamic(this, &AYourPlayerController::OnPaymentSuccessReceived);
}

void AYourPlayerController::OnPaymentSuccessReceived()
{
    UE_LOG(LogTemp, Warning, TEXT("Payment succeeded"));
    // Grant items, show success UI, etc. (verify purchase on your server via webhooks)
}
```

#### Blueprint Implementation

**Opening the Checkout:**

1. Get the checkout URL from your server
2. Use the **Open Checkout** node (Stash category) and pass the checkout URL string. Same node works on iOS and Android.
3. Optionally use **Open Checkout With Config** to customize card/modal sizing.

**Listening to Payment Callbacks:**

The `OnPaymentSuccess` delegate is static. To use it in Blueprints:

1. Create a C++ class (e.g. PlayerController) that binds to `UStashBlueprint::OnPaymentSuccess` in `BeginPlay()` (as in the C++ example above)
2. In that C++ class, declare a `BlueprintImplementableEvent` and call it from the delegate handler
3. Implement that event in your Blueprint to handle the purchase

**Example pattern:**

```cpp
// In your C++ PlayerController
UFUNCTION(BlueprintImplementableEvent, Category = "Store")
void OnPaymentSucceeded();

void AYourPlayerController::OnPaymentSuccessReceived()
{
    OnPaymentSucceeded();
}
```

Then implement `OnPaymentSucceeded` in your Blueprint to grant items or show UI.

**Checking Checkout Status:** Use the **Is Checkout Open** node (Stash category).

**Closing the Checkout:** Use the **Dismiss Checkout** node (Stash category).

#### Complete Reference

For a full sample, see this repository:
- **Game module**: `Source/StashUnreal5/`
- **Stash plugin**: `Plugins/Stash/Source/Stash/` (C++ and Blueprint API in `Public/StashBlueprint.h`)
- **Content**: Blueprints and sample UI in `Content/MobileStarterContent/` (e.g. `WBP_Checkout`, level Blueprints)

## Requirements

- Unreal Engine 5.0+
- Visual Studio (for Windows/Android development)
- Xcode with iOS SDK (for iOS development)
- Android SDK (for Android builds)

## Key Files

**Stash plugin (wrapper and API):**
- `Plugins/Stash/Source/Stash/Public/StashBlueprint.h` - Blueprint library and delegates
- `Plugins/Stash/Source/Stash/Private/IOS/ObjC/StashPayCardWrapper.mm` - iOS native bridge
- `Plugins/Stash/Source/Stash/Private/Android/Java/` - Android native bridge (StashHelper, StashInit)

**Native SDK (pre-built in plugin):**
- `Plugins/Stash/Source/Stash/ThirdParty/iOS/StashPay.xcframework/` - iOS SDK
- `Plugins/Stash/Source/Stash/ThirdParty/Android/StashPay.aar` - Android SDK

**Native SDK source (when using submodule):**
- `Plugins/stash-native-main/iOS/` - iOS implementation
- `Plugins/stash-native-main/Android/` - Android implementation

## Documentation

- [Stash Native SDK](https://github.com/stashgg/stash-native) - Native SDK documentation
- [Stash Pay Docs](https://docs.stash.gg) - Official Stash Pay documentation

## Troubleshooting

If the **Stash** plugin does not load, or you see native build errors on Android/iOS, ensure the plugin is enabled (Edit → Plugins → Stash), your project has the correct iOS/Android SDKs installed, and that you have copied the full `Plugins/Stash/` folder including the `ThirdParty` binaries.

## Support

For Stash Pay integration issues, contact: developers@stash.gg
