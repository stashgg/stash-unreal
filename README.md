# Stash Pay Checkout Demo for Unreal Engine 4.21+

A minimal Unreal Engine project demonstrating Stash Pay native checkout integration on iOS and Android.

## Quick Start

### For Source Engine Builds (Required for UE 4.21 source builds)

If you see the error "Building would modify the following engine files", you need to build from Xcode:

1. Generate Xcode project files:
   ```bash
   cd /path/to/UnrealEngine
   ./GenerateProjectFiles.sh -project="/path/to/StashUnreal4/StashUnreal4.uproject" -game
   ```

2. Open the generated `.xcworkspace` file in Xcode

3. Select the `StashUnreal4Editor - Mac` scheme and build (Cmd+B)

4. Once built, you can launch the editor from Xcode or by double-clicking the .uproject file

### For Binary Engine Installs

### 1. Open the Project

Open `StashUnreal4.uproject` in Unreal Engine 4.21+.

### 2. Enable the Plugin

Go to **Edit > Plugins > Mobile > MobileNativeCode** and ensure it's enabled.

### 3. Create the Demo UI

Since Widget Blueprints cannot be created programmatically, follow these steps in the Editor:

#### Create Checkout Button Widget

1. In the Content Browser, right-click and select **User Interface > Widget Blueprint**
2. Name it `WBP_CheckoutButton`
3. Open the widget and add:
   - A **Button** widget in the center
   - A **Text** widget inside the button with text "Open Checkout"
   - A **Text** widget below the button for status display

4. In the Button's **OnClicked** event:
   ```
   Get Player Controller (cast to StashPlayerController)
   → Call "Open Checkout" with URL: "https://checkout.stash.gg/demo"
   ```

#### Create HUD Blueprint

1. Right-click in Content Browser > **Blueprint Class > HUD**
2. Select **StashHUD** as the parent class
3. Name it `BP_StashHUD`
4. Open and set **CheckoutWidgetClass** to `WBP_CheckoutButton`

#### Create GameMode Blueprint

1. Right-click in Content Browser > **Blueprint Class > Game Mode Base**
2. Select **StashGameMode** as the parent class
3. Name it `BP_StashGameMode`

#### Configure the Level

1. Open `Content/MobileStarterContent/Maps/Minimal_Default`
2. Go to **World Settings**
3. Set **GameMode Override** to `BP_StashGameMode`

### 4. Test in Editor

Press **Play** to see the checkout button. On mobile platforms (iOS/Android), clicking the button will open the native Stash Pay checkout dialog.

## Architecture

```
Unreal Engine (C++/Blueprints)
         ↓
MobileNativeCode Plugin (Wrapper)
         ↓
Stash Native SDK (iOS/Android)
```

### Key Classes

| Class | Purpose |
|-------|---------|
| `StashPlayerController` | Handles checkout callbacks with Blueprint-implementable events |
| `StashGameMode` | GameMode that uses StashPlayerController |
| `StashHUD` | HUD that displays the checkout widget |
| `UMobileNativeCodeBlueprint` | Blueprint function library with Stash Pay functions |

### Blueprint Functions

Available in the **StashPay** category:

- `OpenStashPayCheckoutIOS(CheckoutURL)` - Open checkout on iOS
- `OpenStashPayCheckout(CheckoutURL)` - Open checkout on Android
- `IsStashPayCheckoutOpenIOS()` - Check if checkout is open (iOS)
- `IsStashPayCheckoutOpen()` - Check if checkout is open (Android)
- `DismissStashPayCheckoutIOS()` - Dismiss checkout (iOS)
- `DismissStashPayCheckout()` - Dismiss checkout (Android)

### C++ Usage

```cpp
#include "StashPlayerController.h"

// In your controller/actor:
AStashPlayerController* PC = Cast<AStashPlayerController>(GetController());
if (PC)
{
    PC->OpenCheckout(TEXT("https://checkout.stash.gg/demo"));
}

// Callbacks - override in Blueprint or C++:
void AMyPlayerController::OnPaymentSucceeded()
{
    // Grant items to player
}
```

### Callbacks

Override these events in your PlayerController Blueprint:

- `OnPaymentSucceeded` - Payment completed successfully
- `OnPaymentFailed` - Payment failed
- `OnCheckoutDismissed` - User dismissed the dialog
- `OnCheckoutPageLoaded(LoadTimeMs)` - Page finished loading

## Building for Mobile

### iOS

1. Configure iOS signing in **Project Settings > Platforms > iOS**
2. Package for iOS: **File > Package Project > iOS**
3. The Stash Pay SDK will be included automatically

### Android

1. Configure Android SDK in **Project Settings > Platforms > Android**
2. Package for Android: **File > Package Project > Android**
3. The Stash Pay SDK will be included via Gradle

## File Structure

```
StashUnreal4/
├── Plugins/
│   ├── MobileNativeCode/           # Native mobile code plugin
│   │   └── Source/MobileNativeCode/
│   │       ├── Private/
│   │       │   ├── Android/Java/
│   │       │   │   └── StashPayHelper.java
│   │       │   ├── IOS/ObjC/
│   │       │   │   ├── StashPayCardWrapper.h
│   │       │   │   └── StashPayCardWrapper.mm
│   │       │   └── MobileNativeCodeBlueprint.cpp
│   │       └── Public/
│   │           └── MobileNativeCodeBlueprint.h
│   └── stash-native-main/          # Stash Pay native SDK (git submodule)
│       ├── iOS/
│       └── Android/
├── Source/StashUnreal4/
│   ├── StashPlayerController.h/.cpp
│   ├── StashGameMode.h/.cpp
│   ├── StashHUD.h/.cpp
│   └── StashUnreal4.Build.cs
└── StashUnreal4.uproject
```

## Support

For Stash Pay integration issues, contact: developers@stash.gg

## Documentation

- [Stash Pay Docs](https://docs.stash.gg)
- [MobileNativeCode Plugin](https://github.com/Sovahero/PluginMobileNativeCode)
- [Stash Native SDK](https://github.com/stashgg/stash-native)
