# MobileNativeCode Plugin

A lightweight Unreal Engine plugin that provides a bridge to native mobile functionality, specifically designed to work with **stash-native** for mobile payment integration.

## Architecture

```
MobileNativeCode (thin bridge)
       ↓
stash-native (external, self-contained SDK)
```

This plugin is a **minimal wrapper** that:
- ✅ Exposes Blueprint-callable functions
- ✅ Bridges Unreal C++ ↔ Native iOS/Android code
- ✅ References stash-native externally (not copied)
- ✅ Remains clean and maintainable

## File Structure

```
MobileNativeCode/
├── Source/
│   └── MobileNativeCode/
│       ├── Public/
│       │   ├── MobileNativeCode.h
│       │   └── MobileNativeCodeBlueprint.h          # Blueprint API
│       ├── Private/
│       │   ├── MobileNativeCode.cpp
│       │   ├── MobileNativeCodeBlueprint.cpp        # Blueprint implementation
│       │   ├── IOS/ObjC/
│       │   │   ├── StashPayHelper.h/mm              # iOS bridge to stash-native
│       │   │   └── WebViewHelper.h/mm
│       │   └── Android/Java/
│       │       ├── StashPayHelper.java              # Android bridge to stash-native
│       │       └── WebViewHelper.java
│       └── MobileNativeCode.Build.cs                # Build configuration
├── MobileNativeCode_UPL_iOS.xml                     # iOS build settings
├── MobileNativeCode_UPL_Android.xml                 # Android build settings
├── MobileNativeCode.uplugin                         # Plugin descriptor
├── add_stashpay_activity.sh                         # Android helper script
└── README_STASH_INTEGRATION.md                      # Integration guide
```

## What This Plugin Does

### iOS Bridge
- **StashPayHelper.mm** - Calls stash-native iOS SDK
  - Initializes StashPayCard delegate
  - Opens checkout
  - Handles callbacks
  - Bridges to Unreal C++

### Android Bridge
- **StashPayHelper.java** - Calls stash-native Android SDK
  - Initializes StashPayCard
  - Opens checkout
  - Handles callbacks
  - Bridges to Unreal C++ via JNI

### Blueprint API
- **MobileNativeCodeBlueprint.h/cpp**
  - `OpenStashPayCheckoutIOS(URL)`
  - `OpenStashPayCheckout(URL)` - Android
  - `OnPaymentSuccess` delegate
  - Status check functions

## What This Plugin Does NOT Do

❌ Does not contain stash-native code  
❌ Does not duplicate SDK functionality  
❌ Does not handle payment logic (that's in stash-native)  
❌ Does not need patching or preprocessing  

## Dependencies

- **stash-native** - External package
  - Location: `../stash-native-main/`
  - iOS: Swift Package / Objective-C SDK
  - Android: Gradle library / Java SDK

## Usage in Game Code

### C++
```cpp
#include "MobileNativeCodeBlueprint.h"

// Open checkout
UMobileNativeCodeBlueprint::OpenStashPayCheckoutIOS(CheckoutURL);

// Listen for success
UMobileNativeCodeBlueprint::OnPaymentSuccess.AddDynamic(
    this, 
    &AMyPlayerController::OnPaymentSuccessReceived
);
```

### Blueprint
1. Call `Open Stash Pay Checkout IOS` (or Android)
2. Bind to `On Payment Success` event

## Build Requirements

### iOS
- stash-native at `../stash-native-main/iOS/`
- Xcode with iOS SDK
- SafariServices.framework
- WebKit.framework

### Android
- stash-native at `../stash-native-main/Android/`
- Android SDK API 27+
- Gradle build system

## Installation

1. Copy `MobileNativeCode/` to `YourProject/Plugins/`
2. Ensure `stash-native-main/` is at `YourProject/Plugins/stash-native-main/`
3. Regenerate project files
4. Build

## Integration Guide

See [README_STASH_INTEGRATION.md](./README_STASH_INTEGRATION.md) for detailed integration instructions.

## Credits

- Based on [PluginMobileNativeCode](https://github.com/Sovahero/PluginMobileNativeCode) by Sovahero
- Extended for stash-native integration
- MIT License

## Support

- **Plugin issues**: Open an issue in your project repository
- **stash-native issues**: https://github.com/stashgg/stash-native
- **Original plugin**: https://github.com/Sovahero/PluginMobileNativeCode
