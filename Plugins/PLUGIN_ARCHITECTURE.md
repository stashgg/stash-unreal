# Plugin Architecture

This project uses a **clean separation** between the bridge plugin and the native SDK.

## Structure

```
Plugins/
├── MobileNativeCode/              # Bridge plugin (thin wrapper)
│   ├── Source/
│   │   └── MobileNativeCode/
│   │       ├── Public/
│   │       │   ├── MobileNativeCode.h
│   │       │   └── MobileNativeCodeBlueprint.h
│   │       └── Private/
│   │           ├── MobileNativeCodeBlueprint.cpp
│   │           ├── IOS/ObjC/
│   │           │   ├── StashPayHelper.mm        # iOS bridge (113 lines)
│   │           │   └── WebViewHelper.mm
│   │           └── Android/Java/
│   │               ├── StashPayHelper.java      # Android bridge
│   │               └── WebViewHelper.java
│   ├── MobileNativeCode.Build.cs
│   ├── MobileNativeCode_UPL_iOS.xml
│   ├── MobileNativeCode_UPL_Android.xml
│   └── README.md
│
└── stash-native-main/              # Native SDK (self-contained)
    ├── iOS/
    │   └── StashPay/
    │       └── Sources/StashPay/
    │           ├── StashPayCard.mm              # Full iOS implementation
    │           └── include/
    │               ├── StashPayCard.h
    │               └── StashPay.h
    └── Android/
        └── stashpay/
            └── src/main/java/
                └── com/stash/popup/
                    └── StashPayCard.java        # Full Android implementation
```

## Design Principles

### 1. Separation of Concerns

**MobileNativeCode** (Bridge)
- ✅ Minimal wrapper code only
- ✅ Exposes Blueprint API
- ✅ Platform-specific bridges (JNI, Objective-C)
- ✅ References stash-native externally
- ❌ Does NOT contain SDK logic
- ❌ Does NOT duplicate code

**stash-native** (SDK)
- ✅ Self-contained native SDK
- ✅ Platform-specific implementations
- ✅ Can be updated independently
- ✅ Can be used in other projects
- ✅ Built-in Unreal compatibility
- ❌ Not Unreal-specific

### 2. Zero Duplication

```
MobileNativeCode references stash-native:
  - iOS:     #import <StashPay/StashPayCard.h>
  - Android: import com.stash.popup.StashPayCard;
```

No copying, no forking, no duplication.

### 3. Built-in Compatibility

stash-native uses conditional compilation for Unreal:

```objc
#if __has_feature(objc_arc)
    __weak id delegate;  // For standalone iOS apps
#else
    __unsafe_unretained id delegate;  // For Unreal Engine
#endif
```

Works in **both**:
- Standalone iOS/Android apps (with ARC)
- Unreal Engine (without ARC)

## Data Flow

### Opening Checkout (iOS Example)

```
Blueprint
    ↓
UMobileNativeCodeBlueprint::OpenStashPayCheckoutIOS()
    ↓
OpenStashPayCheckoutIOS_Impl()  [StashPayHelper.mm]
    ↓
[[StashPayCard sharedInstance] openCheckoutWithURL:]  [stash-native]
```

### Payment Success Callback

```
[StashPayCard delegate callback]  [stash-native]
    ↓
-[StashPayHelperDelegate stashPayCardDidCompletePayment]
    ↓
NotifyPaymentSuccessFromIOS()  [C function]
    ↓
UMobileNativeCodeBlueprint::NotifyPaymentSuccess()
    ↓
OnPaymentSuccess.Broadcast()  [Blueprint delegate]
```

## Build Process

### iOS
1. MobileNativeCode.Build.cs references stash-native path
2. Adds stash-native include directories
3. Compiles StashPayCard.mm from stash-native location
4. Links with iOS frameworks (SafariServices, WebKit)
5. No preprocessing or patching needed

### Android
1. MobileNativeCode_UPL_Android.xml copies stash-native Java files
2. Gradle compiles everything together
3. Resources merged from stash-native
4. No preprocessing needed

## File Counts

### MobileNativeCode (Bridge)
- **C++ Files**: 4 (headers + impl)
- **Objective-C++ Files**: 4 (2 bridges × 2 files)
- **Java Files**: 2 (bridges)
- **Total LOC**: ~500 lines (bridge code only)

### stash-native (SDK)
- **iOS**: StashPayCard.mm (~2400 lines)
- **Android**: StashPayCard.java + helpers (~1500 lines)
- **Total LOC**: ~4000 lines (full SDK implementation)

## Benefits

✅ **Clean separation** - Easy to understand  
✅ **Easy updates** - Replace stash-native, rebuild  
✅ **No duplication** - Single source of truth  
✅ **Portable** - stash-native works standalone  
✅ **Maintainable** - Clear responsibilities  
✅ **No patching** - Built-in compatibility  

## Updating stash-native

```bash
# 1. Pull latest stash-native
cd Plugins/stash-native-main
git pull origin main

# 2. Rebuild Unreal project
# (No patching or modifications needed)

# 3. Done!
```

## Testing Both Environments

### Test in Unreal Engine (no ARC)
```bash
# Build iOS in Unreal
# MobileNativeCode compiles with stash-native
```

### Test Standalone (with ARC)
```bash
cd Plugins/stash-native-main/iOS/Sample
xcodebuild -scheme StashPaySample
```

Both work with the **same** stash-native code!

---

## Summary

This architecture achieves:
- **MobileNativeCode**: Thin, clean bridge (~500 LOC)
- **stash-native**: Full-featured, self-contained SDK (~4000 LOC)
- **Zero duplication**: References, not copies
- **Universal compatibility**: Works with and without ARC
- **Easy maintenance**: Update either plugin independently
