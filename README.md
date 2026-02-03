# Stash SDK for Unreal Engine 4

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
    Stash Unreal Plugin (Wrapper)
         ↓
Pre-built Native SDKs (iOS/Android)
```

**Components:**

1. **Stash Plugin** - Unreal Engine plugin providing Blueprint and C++ access to the Stash Pay SDK. Located at `Plugins/Stash/`.
2. **Pre-built Native SDKs** - Pre-compiled plugins are from [stash-native](https://github.com/stashgg/stash-native) and include the iOS XCFramework and Android AAR bundled in `Plugins/Stash/Source/Stash/ThirdParty/`.

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
2. You should be able to call Stash Blueprint functions from the "Stash" category (Such as OpenCheckout)
3. Hook Stash functions into your components, Package for Android or iOS to test native integration

## Usage

The Stash Package provides two presentation methods:

- **Checkout** - For Stash Pay checkout URLs (payment flows)
- **Modal** - For Stash Pay opt-in modals (payment channel selection)

Both methods work on iOS and Android and can be opened with default settings or custom configuration.

---

### SetLandscapeLockWhenCheckoutClosed (iOS, landscape games)

Stash Pay **checkout on phone** is designed for portrait. If your game is **landscape-only** but you want the checkout overlay to rotate to portrait when opened, use this so the **game** stays landscape and only the **overlay** can go portrait.

#### When to use

- Your game is **landscape-only** and you show Stash Pay checkout on **iOS** (phone).
- You want the in-app checkout overlay to be allowed to rotate to portrait while the rest of the game stays in landscape.

Do **not** enable this if your game already supports portrait, or if you don’t need the game locked to landscape.

#### How to use

1. **Enable portrait in Unreal project settings**  
   **Edit → Project Settings → Platform → iOS** (or **Mobile**). Under **Supported Orientations**, enable **Portrait** (and **Upside Down** if you want). The plugin does **not** modify the plist; orientation support is controlled by your project. Enabling portrait here allows the Stash Pay overlay to rotate to portrait when checkout is open.

2. **Call SetLandscapeLockWhenCheckoutClosed once at startup**  
   In **Blueprint** (e.g. Level Blueprint or GameMode **Begin Play**) or **C++**:
   - **Blueprint:** Stash → **Set Landscape Lock When Checkout Closed** → set to **true**.
   - **C++:** `UStashBlueprint::SetLandscapeLockWhenCheckoutClosed(true);`

   This keeps the **game window** in landscape; only the **Stash Pay overlay** is allowed to rotate to portrait when checkout is open. When the user dismisses checkout, the game remains in landscape.

**Platform:** iOS only. No effect on Android.

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
    Config.bForcePortraitOnCheckout = false;    // Allow current orientation; use true for portrait-only checkout on phone
    Config.CardHeightRatioPortrait = 0.7f;      // Phone card height in portrait
    Config.CardWidthRatioLandscape = 0.9f;      // Phone card width in landscape (when force portrait off)
    Config.CardHeightRatioLandscape = 0.6f;     // Phone card height in landscape (when force portrait off)
    Config.TabletWidthRatioPortrait = 0.5f;     // Tablet width in portrait
    Config.TabletHeightRatioPortrait = 0.7f;    // Tablet height in portrait
    Config.TabletWidthRatioLandscape = 0.8f;     // Tablet width in landscape
    Config.TabletHeightRatioLandscape = 0.65f;  // Tablet height in landscape
    
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
void OnStashOptInResponse(const FString& OptInType);

UFUNCTION()
void OnStashNetworkError();

// In your PlayerController.cpp BeginPlay()
void AYourPlayerController::BeginPlay()
{
    Super::BeginPlay();
    
    UStashBlueprint::OnPaymentSuccess.AddDynamic(this, &AYourPlayerController::OnStashPaymentSuccess);
    UStashBlueprint::OnPaymentFailure.AddDynamic(this, &AYourPlayerController::OnStashPaymentFailure);
    UStashBlueprint::OnDialogDismissed.AddDynamic(this, &AYourPlayerController::OnStashDialogDismissed);
    UStashBlueprint::OnOptInResponse.AddDynamic(this, &AYourPlayerController::OnStashOptInResponse);
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

void AYourPlayerController::OnStashOptInResponse(const FString& OptInType)
{
    UE_LOG(LogTemp, Log, TEXT("Opt-in response: %s"), *OptInType);
    // Handle payment channel selection from modal
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
| `SetLandscapeLockWhenCheckoutClosed(bEnable)` | (iOS) Keep game in landscape when checkout is closed; allow portrait only for the Stash Pay overlay when checkout is open. Enable portrait in **Project Settings → iOS** if needed, then call once at startup (e.g. Begin Play). See [SetLandscapeLockWhenCheckoutClosed](#setlandscapelockwhencheckoutclosed-ios-landscape-games) for when and how to use. |

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
| `OnOptInResponse(OptInType)` | Called when an opt-in response is received (modal payment channel selection) |
| `OnPageLoaded(LoadTimeMs)` | Called when page finishes loading |
| `OnNetworkError` | Called when a network error occurs |

**FStashCheckoutConfig Properties:**

| Property | Default | Description |
|----------|---------|-------------|
| `bForcePortraitOnCheckout` | false | When true, phone checkout is portrait-only; when false, current orientation with landscape sizing (SDK 1.2.4+) |
| `CardHeightRatioPortrait` | 0.68 | Phone card height in portrait |
| `CardWidthRatioLandscape` | 0.9 | Phone card width in landscape (when force portrait off) |
| `CardHeightRatioLandscape` | 0.6 | Phone card height in landscape (when force portrait off) |
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

## Rebuilding the project

To regenerate IDE project files and build with your Unreal Engine 4.27+ installation:

### Option A: Open in Unreal Editor (simplest)

1. Double‑click **`StashUnreal4.uproject`** (or use **File → Open Project** in Unreal Editor).
2. If prompted, choose your Unreal Engine 4.27+ installation.
3. The editor will compile the Stash plugin and load the project. Use **File → Package Project** or the Play button as needed.

### Option B: Regenerate project files from the engine

If you use a custom engine or need to regenerate `.xcworkspace` / `.sln`:

**macOS (Xcode):**

```bash
UE_ROOT="/Users/ondrejrehacek/Git/UnrealEngine-421-plus"

"$UE_ROOT/Engine/Build/BatchFiles/Mac/GenerateProjectFiles.sh" \
  -project="$(pwd)/StashUnreal4.uproject" \
  -game
```

Then open **`StashUnreal4.xcworkspace`** in Xcode, or open **`StashUnreal4.uproject`** in Unreal Editor.

**Windows (Visual Studio):**

```bat
REM Set UE_ROOT to your Unreal Engine root (folder that contains Engine\)
"C:\Path\To\UnrealEngine\Engine\Build\BatchFiles\GenerateProjectFiles.bat" ^
  "C:\Path\To\stash-unreal-4\StashUnreal4.uproject" -game
```

Then open **`StashUnreal4.sln`** in Visual Studio, or open the `.uproject` in Unreal Editor.

### Clean rebuild

If you run into build issues, clean generated folders then reopen the project:

```bash
rm -rf Binaries Intermediate Saved/StagedBuilds DerivedDataCache
# On Windows also remove .vs, *.sln, *.xcworkspace if you want to regenerate them
```

Then open **`StashUnreal4.uproject`** in Unreal Editor again so it recompiles.

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
