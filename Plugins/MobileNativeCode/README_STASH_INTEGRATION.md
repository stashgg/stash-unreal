# MobileNativeCode + stash-native Integration

This plugin provides a bridge between Unreal Engine and the **stash-native** SDK for mobile payment integration.

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     Unreal Engine Game                       │
│                  (MyPlayerController, Blueprints)            │
└───────────────────────┬─────────────────────────────────────┘
                        │
                        │ C++ API
                        ▼
┌─────────────────────────────────────────────────────────────┐
│              MobileNativeCode Plugin                         │
│         (Thin wrapper/bridge - this plugin)                  │
│                                                              │
│  • MobileNativeCodeBlueprint.h/cpp (Blueprint interface)     │
│  • StashPayHelper.mm (iOS bridge)                           │
│  • StashPayHelper.java (Android bridge)                     │
└───────────────────────┬─────────────────────────────────────┘
                        │
                        │ JNI/Objective-C
                        ▼
┌─────────────────────────────────────────────────────────────┐
│                  stash-native Package                        │
│              (External, self-contained)                      │
│                                                              │
│  • iOS: StashPayCard (Objective-C)                          │
│  • Android: StashPayCard (Java/Kotlin)                      │
│                                                              │
│  Location: Plugins/stash-native-main/                       │
└─────────────────────────────────────────────────────────────┘
```

## Key Principle

**stash-native is kept separate and self-contained.**  
MobileNativeCode is just a thin bridge that calls into stash-native.

---

## iOS Setup

### No Patching Required! ✅

stash-native now includes **built-in Unreal Engine compatibility** using preprocessor directives:
- Uses `__weak` when ARC is enabled (standalone iOS apps)
- Uses `__unsafe_unretained` when ARC is disabled (Unreal Engine)

**How it works:**
```objc
#if __has_feature(objc_arc)
    __weak WKWebView* _webView;  // For standalone apps
#else
    __unsafe_unretained WKWebView* _webView;  // For Unreal
#endif
```

This means:
✅ Works in standalone iOS apps (with ARC)  
✅ Works in Unreal Engine (without ARC)  
✅ No patching or manual changes needed  
✅ No separate branches or forks  

### Build Configuration

The `MobileNativeCode.Build.cs` references stash-native externally:

```csharp
// Path to external stash-native package
string StashNativeRoot = System.IO.Path.Combine(PluginRoot, "../stash-native-main/iOS/StashPay");

// Add headers
PublicIncludePaths.Add(StashNativeInclude);

// StashPayCard.mm will be compiled from stash-native location
```

---

## Android Setup

### No Patching Required

Android integration already references stash-native externally via UPL:

```xml
<!-- MobileNativeCode_UPL_Android.xml -->
<copyDir src="$S(ProjectDir)/Plugins/stash-native-main/Android/stashpay/src/main/java/com/stash/popup" 
         dst="$S(BuildDir)/src/com/stash/popup" />
```

---

## File Structure

```
Plugins/
├── MobileNativeCode/                    # Bridge plugin
│   ├── Source/
│   │   └── MobileNativeCode/
│   │       ├── Public/
│   │       │   └── MobileNativeCodeBlueprint.h  # Blueprint API
│   │       └── Private/
│   │           ├── MobileNativeCodeBlueprint.cpp
│   │           ├── IOS/ObjC/
│   │           │   ├── StashPayHelper.h         # iOS bridge (thin wrapper)
│   │           │   └── StashPayHelper.mm
│   │           └── Android/Java/
│   │               └── StashPayHelper.java      # Android bridge (thin wrapper)
│   ├── MobileNativeCode.Build.cs
│   ├── MobileNativeCode_UPL_iOS.xml
│   ├── MobileNativeCode_UPL_Android.xml
│   └── patch_stashpay_ios.sh                    # iOS compatibility patch
│
└── stash-native-main/                   # External package (separate)
    ├── iOS/
    │   └── StashPay/
    │       └── Sources/StashPay/
    │           ├── StashPayCard.mm      # (patched from .m)
    │           └── include/
    │               ├── StashPayCard.h
    │               └── StashPay.h
    └── Android/
        └── stashpay/
            └── src/main/java/
                └── com/stash/popup/
                    └── StashPayCard.java
```

---

## Usage in Game Code

### C++ Example

```cpp
#include "MobileNativeCodeBlueprint.h"

// Open checkout
UMobileNativeCodeBlueprint::OpenStashPayCheckoutIOS(CheckoutURL);

// Bind payment success callback
UMobileNativeCodeBlueprint::OnPaymentSuccess.AddDynamic(this, &AMyPlayerController::OnPaymentSuccessReceived);
```

### Blueprint Example

1. Call `Open Stash Pay Checkout IOS` (or Android version)
2. Bind to `On Payment Success` event

---

## Benefits of This Architecture

✅ **stash-native remains unchanged** (except one-time patch for iOS)  
✅ **Easy to update** stash-native - just replace the package  
✅ **Clear separation** - MobileNativeCode is just a bridge  
✅ **Portable** - stash-native can be used in other projects  
✅ **Single source of truth** - stash-native is the authoritative SDK  

---

## Troubleshooting

### iOS: "StashPayCard.h not found"

**Solution:** Verify the stash-native path in `MobileNativeCode.Build.cs` points to the correct location.

### iOS: Compilation errors with __weak

**Solution:** Make sure you're using the latest stash-native with built-in Unreal compatibility (uses `__has_feature(objc_arc)` checks).

### Android: Java class not found

**Solution:** Check that `MobileNativeCode_UPL_Android.xml` correctly references `Plugins/stash-native-main/`.

---

## Updating stash-native

1. **Replace** `Plugins/stash-native-main/` with new version
2. **Clean and rebuild** project - that's it!

No patching needed if the new version includes the `__has_feature(objc_arc)` compatibility checks.

---

## Support

- **stash-native issues**: https://github.com/stashgg/stash-native
- **MobileNativeCode issues**: https://github.com/Sovahero/PluginMobileNativeCode
- **Integration issues**: This project's repository
