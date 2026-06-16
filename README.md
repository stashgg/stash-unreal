# Stash for Unreal Engine 5

<p align="left">
  <img src="https://github.com/stashgg/stash-native/raw/main/.github/assets/stash_unreal.png" width="128" height="128" alt="Stash Unreal Logo"/>
</p>

Unreal Engine plugin wrapper for [stash-native](https://github.com/stashgg/stash-native), enabling Stash Pay IAP checkout and webshop presentation on Android and iOS via C++ and Blueprints. The plugin uses **Stash Native** (**2.2.1**).

## Requirements

- Unreal Engine 5.0+ (sample project targets **5.7**)
- iOS 12.0+ / Android API 21+
- Xcode (iOS), Visual Studio (Windows/Android), Android SDK (Android)

## Sample / Downloads

- **Run the sample:** Clone this repo and open `StashUnreal5.uproject` in Unreal Engine 5.
- **Use in your project:** Copy the `Plugins/Stash/` folder into your project’s `Plugins/` directory and enable the plugin under **Edit → Plugins → Mobile → Stash**.

## Quick Start

1. Add the Stash plugin (see above).
2. Enable **Edit → Plugins → Installed → Mobile → Stash** and restart the editor.
3. Call Stash Blueprint functions from the **Stash** category (e.g. **Open Card**, **Open Modal**, **Open Browser**).
4. **To react to payment success, dismiss, or other callbacks in Blueprint:** use **Get Stash Subsystem** (Stash category), then **Assign** or **Add** the event you need (e.g. **Add On Payment Success**, **Add On Dialog Dismissed**) on the returned subsystem. The static Stash nodes do not expose bindable delegates; the subsystem does.

### Folder structure

- **Plugins/Stash/** – Plugin root: `Source/Stash` (module), `ThirdParty` (StashNative AAR + XCFramework), `Resources`.
- **StashBlueprint** – Blueprint function library: `OpenCard`, `OpenModal`, `OpenBrowser`, `CloseBrowser`, `SetAndroidCheckoutBackdropBytes`, `ClearAndroidCheckoutBackdrop`, `CaptureViewportForAndroidCheckoutBackdrop`, config structs, delegates.
- **Key files:** `StashBlueprint.h`, iOS wrapper `StashNativeCardWrapper` (ObjC), Android `StashHelper.java`, ThirdParty StashNative binaries.

## Usage

Stash Native presents Stash Pay and webshop links in three ways: **openCard** (drawer/card), **openModal** (centered modal), and **openBrowser** (system browser). Checkout URLs must be generated on your backend; see the [Stash Pay Integration Guide](https://docs.stash.gg/guides/stash-pay/integration).

**iOS note:** The first OpenCard/OpenModal call can be slow under the Xcode debugger (WKWebView); production builds are unaffected.

**Android checkout backdrop (landscape → portrait):** The OS may still **animate** rotation when checkout forces portrait; stash-native shows your capture **behind the dim overlay** (not a frozen game swapchain). Prefer: **Capture Viewport For Android Checkout Backdrop** (JPEG, end-of-frame), assign bytes to **Android Checkout Backdrop** on your **Stash Card Config**, then **Open Card With Config** so backdrop and `openCard` run in one UI-thread step. Alternatively call **Set Android Checkout Backdrop Bytes** immediately before open (same thread order as Unity’s `WaitForEndOfFrame` → `setBackdropBytes` → `OpenCard`). Match Unity’s README: consider locking **screen orientation** while the card is open. Requires **StashNative-2.2.0+** AAR (`Stash_UPL_Android.xml` `gradleCopies`; 2.1.4+ also supports backdrop via `setBackdropBytes`).

---

### OpenCard()

Drawer-style card: slides up from the bottom on phones, centered on tablets. Use for Stash Pay checkout or channel selection.

#### C++

```cpp
#include "StashBlueprint.h"

// Default config
UStashBlueprint::OpenCard(CheckoutURL);

// Custom config
FStashCardConfig Config = UStashBlueprint::MakeStashCardConfig(
    false,   // bForcePortrait
    0.68f,   // CardHeightRatioPortrait
    0.9f,    // CardWidthRatioLandscape
    0.6f,    // CardHeightRatioLandscape
    0.6f, 0.8f, 0.8f, 0.65f,  // tablet ratios
    FString() // BackgroundColor (HTML hex)
);
// Config.AndroidCheckoutBackdrop = captured JPEG bytes before OpenCardWithConfig
UStashBlueprint::OpenCardWithConfig(CheckoutURL, Config);
```

#### Blueprint

Use **Open Card** or **Open Card With Config** from the Stash category:

![Card Blueprint Example](.github/blueprint_checkout.png)

---

### OpenModal()

Centered modal on all devices. Use for channel selection or alternative checkout style.

#### C++

```cpp
UStashBlueprint::OpenModal(URL);

// Or with config
FStashModalConfig Config;
Config.bAllowDismiss = true;
// ... set phone/tablet ratios as needed
UStashBlueprint::OpenModalWithConfig(URL, Config);
```

#### Blueprint

Use **Open Modal** or **Open Modal With Config** from the Stash category:

![Modal Blueprint Example](.github/blueprint_modal.png)

---

### OpenBrowser() / CloseBrowser()

Opens the URL in the platform browser (Chrome Custom Tabs on Android, SFSafariViewController on iOS). No in-app UI, no config, no callbacks. Use when you only need a simple browser view.

- **CloseBrowser()** dismisses the Safari view on **iOS**; on **Android** it is a no-op (Chrome Custom Tabs cannot be closed by the app).

```cpp
UStashBlueprint::OpenBrowser(URL);
// Optionally on iOS:
UStashBlueprint::CloseBrowser();
```

---

### IsPurchaseProcessing()

Returns whether a purchase is currently being processed. When true, the checkout UI cannot be dismissed by the user. Use this to avoid showing conflicting UI or allowing the user to leave the flow during payment.

```cpp
if (UStashBlueprint::IsPurchaseProcessing())
{
    // Show "Processing…" or disable back button
}
```

In Blueprint: use **Is Purchase Processing** (Stash category).

---

### SetLandscapeLockWhenCardClosed (iOS, landscape games)

Stash Native card on phone is designed for portrait. If your game is **landscape-only** but you want the card overlay to rotate to portrait when opened, call this so the **game** stays landscape and only the **overlay** can go portrait.

1. **Enable portrait in project:** **Edit → Project Settings → Platform → iOS** → **Supported Orientations** → enable **Portrait** (and **Upside Down** if desired).
2. **Call once at startup (e.g. Begin Play):**  
   - Blueprint: Stash → **Set Landscape Lock When Card Closed** → **true**.  
   - C++: `UStashBlueprint::SetLandscapeLockWhenCardClosed(true);`

**Platform:** iOS only. No effect on Android.

---

### Android keep-alive service (Android, Optional)

On low-memory or Android Go-class devices, the Android OS may kill your app when the user leaves for **Chrome Custom Tabs** during checkout. Stash Native can run a short **foreground service** with a low-priority notification to prevent the game from suspending.

- **Default:** keep-alive is **off**; enable explicitly if you need it.
- **Manifest:** the Stash Native AAR merges the required service and permissions; you normally do not add them by hand.

#### Blueprint

1. Call **Set Android Keep Alive Enabled** (Stash category) with **true** — e.g. once at startup (**Begin Play**) or before opening checkout.
2. Optionally: use **Make Stash Keep Alive Config** (Stash category — not the generic struct **Make** node), set **Notification Title**, **Notification Text**, and **Notification Icon Drawable Name** (e.g. “Payment in progress” / “Tap to return to the app” / `stash_payment_icon`), then call **Set Android Keep Alive Config** with the returned struct.

#### C++

```cpp
#include "StashBlueprint.h"

UStashBlueprint::SetAndroidKeepAliveEnabled(true);
FStashKeepAliveConfig KA;
KA.NotificationTitle = TEXT("Payment in progress");
KA.NotificationText = TEXT("Tap to return to the app");
KA.NotificationIconDrawableName = TEXT("stash_payment_icon"); // or leave empty for SDK default
UStashBlueprint::SetAndroidKeepAliveConfig(KA);
```

#### Custom notification icon

Add a white/alpha silhouette drawable under your game project, for example:

`YourProject/Build/Android/res/drawable/stash_payment_icon.xml`

In Blueprint, set **Notification Icon Drawable Name** to `stash_payment_icon` (no `@drawable/`, no `.xml` extension). Call **Set Android Keep Alive Enabled** → **true**, then **Set Android Keep Alive Config**, before **Open Browser** or other flows that leave the app.

Custom notification icons require **StashNative-2.2.0+** in `ThirdParty/Android/` (bundled with this plugin via `Stash_UPL_Android.xml`).

**If you will never use keep-alive** (no **Set Android Keep Alive Enabled** set to true, no need for the foreground service), you can trim **`Plugins/Stash/Source/Stash/Stash_UPL_Android.xml`** so Gradle does not pull the keep-alive–related pins:

1. **Remove the `androidx.core:core` dependency** — Delete the **`implementation 'androidx.core:core:1.13.1'`** line and the two-line comment directly above it (*“Stash Native 2.1+ keep-alive calls ServiceCompat…”*). Unreal’s default transitive **`androidx.core`** is then used; that is usually enough when keep-alive is never started.
2. **Remove the Kotlin resolution block** — Delete the entire **`<insert>`** that contains **`configurations.all { resolutionStrategy.eachDependency { ... } }`**, plus the **XML comment** immediately above it (*“androidx.core:1.13.x pulls kotlin-stdlib…”*). That block fixes **Kotlin duplicate-class** errors that show up **because** Core 1.13.x was added; if you drop the Core pin, you typically drop this too.

---

### Listening to callbacks

Bind to events for payment success/failure, dismiss, opt-in, page loaded, network error, and external payment URLs.

> **Blueprint:** You cannot bind to the Stash callbacks directly on the Blueprint function library (it has no object reference). Use **Get Stash Subsystem** to get the Stash Subsystem, then bind to its **On Payment Success**, **On Dialog Dismissed**, etc. on that object.

#### Blueprint (recommended)

1. Call **Get Stash Subsystem** (under Stash), passing **Self** or your world context. You get the Stash Subsystem object.
2. On that object, use **Assign** or **Add** for the event you want (e.g. **Add On Payment Success**, **Add On Dialog Dismissed**).
3. Choose the Blueprint function or event to run when the callback fires.

Example: in Level Blueprint or an Actor, from **Begin Play** → **Get Stash Subsystem** (Self) → **Assign On Payment Success** / **Add On Payment Success** → choose your event.

#### C++

```cpp
UStashBlueprint::OnPaymentSuccess.AddDynamic(this, &AYourClass::OnStashPaymentSuccess);
// ... or get the subsystem and bind to its delegates:
if (UStashSubsystem* Stash = UStashBlueprint::GetStashSubsystem(this))
    Stash->OnPaymentSuccess.AddDynamic(this, &AYourClass::OnStashPaymentSuccess);
```

---

## Full API Reference

**Access:** Static Blueprint library `UStashBlueprint`.

### Methods

| Method | Description |
|--------|-------------|
| `OpenCard(URL)` | Opens card with default config |
| `OpenCardWithConfig(URL, Config)` | Opens card with custom config |
| `OpenModal(URL)` | Opens modal with default config |
| `OpenModalWithConfig(URL, Config)` | Opens modal with custom config |
| `OpenBrowser(URL)` | Opens URL in system browser |
| `CloseBrowser()` | Dismisses browser (iOS only; no-op on Android) |
| `IsCardOpen()` | Returns true if card or modal is displayed |
| `IsPurchaseProcessing()` | Returns true if a purchase is currently being processed (checkout cannot be dismissed) |
| `DismissCard()` | Dismisses card/modal |
| `SetLandscapeLockWhenCardClosed(bEnable)` | (iOS) Lock game to landscape when card closed |
| `MakeStashKeepAliveConfig(Title, Text, IconDrawableName)` | (Android) Builds `FStashKeepAliveConfig` for keep-alive notification |
| `SetAndroidKeepAliveEnabled(bEnabled)` | (Android) Enable foreground keep-alive service during browser flows |
| `SetAndroidKeepAliveConfig(Config)` | (Android) Notification title, text, and optional icon drawable name for keep-alive (`FStashKeepAliveConfig`) |
| `GetStashSubsystem(WorldContextObject)` | Returns the Stash Subsystem so you can bind to On Payment Success, On Dialog Dismissed, etc. in Blueprint |
| `MakeStashCardConfig(...)` | Builds `FStashCardConfig` for OpenCardWithConfig |

### Delegates

| Delegate | Description |
|----------|-------------|
| `OnPaymentSuccess` | Payment completed successfully |
| `OnPaymentFailure` | Payment failed |
| `OnDialogDismissed` | User dismissed the dialog |
| `OnOptInResponse(OptInType)` | Opt-in / channel selection response |
| `OnPageLoaded(LoadTimeMs)` | Page finished loading |
| `OnNetworkError` | Network error during load |
| `OnExternalPayment(URL)` | Checkout opened an external URL (e.g. Google Pay, Klarna, crypto); payment may complete in browser or another app; user returns via deep link |

### Config types

**FStashCardConfig** (card): `bForcePortrait`, `CardHeightRatioPortrait`, `CardWidthRatioLandscape`, `CardHeightRatioLandscape`, `TabletWidthRatioPortrait`, `TabletHeightRatioPortrait`, `TabletWidthRatioLandscape`, `TabletHeightRatioLandscape`, **`BackgroundColor`** (optional HTML hex, e.g. `#RRGGBB`; leave empty for SDK default—see [stash-native](https://github.com/stashgg/stash-native/blob/main/README.md)), **`AndroidCheckoutBackdrop`** (Android, optional JPEG/PNG bytes for force-portrait checkout backdrop; empty array is ignored).

**FStashModalConfig** (modal): `bAllowDismiss`, `PhoneWidthRatioPortrait`, `PhoneHeightRatioPortrait`, `PhoneWidthRatioLandscape`, `PhoneHeightRatioLandscape`, `TabletWidthRatioPortrait`, `TabletHeightRatioPortrait`, `TabletWidthRatioLandscape`, `TabletHeightRatioLandscape`, **`BackgroundColor`** (optional hex; empty = default).

**FStashKeepAliveConfig** (Android): `NotificationTitle`, `NotificationText`, `NotificationIconDrawableName` (drawable base name in the game APK; empty = SDK default) for the keep-alive notification.

---

## Blueprint usage

Use the **Stash** category nodes for Open Card, Open Modal, Open Browser, configs, and landscape lock. The two screenshots above show card and modal flows. For callbacks, use **Get Stash Subsystem** and bind to its events (see **Listening to callbacks** above)—no C++ bridge required.

---

## Troubleshooting

- **Get Stash Subsystem returns null:** Ensure you pass a valid world context (e.g. **Self** from Level Blueprint or an Actor in a running game). In the editor before Play-in-Editor there is no play world, so the subsystem is not available.
- **Blueprint shows old nodes (Open Checkout, Set Force Web Based Checkout):** The plugin API is **Open Card**, **Open Card With Config**, **Open Browser**, **Close Browser**, **Is Card Open**, **Dismiss Card** (no Open Checkout, no Force Web Based). If you still see old names, do a **clean rebuild**: close the editor, delete the `Intermediate` and `Binaries` folders in your project root, then reopen the `.uproject`. The editor will recompile and Blueprint will show only the new Stash nodes.
- **iOS – undefined symbol / Library not loaded:** Add WebKit and SafariServices if needed; ensure **StashNative.xcframework** is embedded (Embed & Sign) and present under `Plugins/Stash/Source/Stash/ThirdParty/iOS/`.
- **Android – class not found / blank card:** Ensure **StashNative** AAR is in `ThirdParty/Android/` (e.g. `StashNative-2.2.1.aar`; the filename must match `Stash_UPL_Android.xml` → `gradleCopies`). Add internet permission. ProGuard: keep `com.stash.**`.
- **Android – checkout backdrop has no effect:** Use a Stash Native Android AAR that includes `setBackdropBytes(byte[])` on `StashNativeCard` (2.1.4+ in this repo). Confirm call order: set bytes (or capture delegate) **before** Open Card. If you replace the AAR with another build, update the `gradleCopies` `copyFile` `src` path to that filename.
- **Blueprint – "Only exactly matching structures" between Make Stash Card Config and Open Card With Config:** Usually a **stale graph** after the `FStashCardConfig` struct changed. **Close the editor**, rebuild the **Stash** plugin (or full project), reopen, then **delete** the **Make Stash Card Config** and **Open Card With Config** nodes and place them again from the **Stash** category (or use **Refresh All Nodes** on the Blueprint). Ensure you are not mixing a **User-defined struct** with the same display name as the plugin’s **Stash Card Config**; the shell color pin must be **String** (HTML hex like `#RRGGBB`), not Linear Color.
- **Blueprint – Make Stash Keep Alive Config missing Notification Icon Drawable Name:** The C++ struct changed; the editor is still using old plugin bytecode. **Close the editor**, rebuild the **Stash** plugin, reopen, **delete** the old **Make Stash Keep Alive Config** node, and add a fresh one from the **Stash** category (right-click → Stash → **Make Stash Keep Alive Config**). Prefer that node over the generic struct **Make** pin on **Stash Keep Alive Config**.
- **Android – `Assertion failed: Result` in `Stack.h`:** Stale **WBP_StashUI** (or your widget) bytecode still references a removed **Make Stash Card Config** pin (e.g. old **Use Android Checkout Backdrop**). Open the widget, **delete** the **Make Stash Card Config** node, place a new one, wire only **Android Checkout Backdrop** from capture → **Open Card With Config**, compile, save, then **recook** Android.
- **Android – crash on Open Browser with keep-alive:** `NoSuchMethodError` on `ServiceCompat.startForeground(Service, int, Notification, int)` means **`androidx.core` is too old** in the packaged APK. Ensure `Stash_UPL_Android.xml` still includes **`implementation 'androidx.core:core:1.13.1'`** (or newer); see the **Android keep-alive service** section above.
- **Android – Gradle `checkDebugDuplicateClasses` (Kotlin):** Duplicate classes in `kotlin-stdlib` vs `kotlin-stdlib-jdk7` / `kotlin-stdlib-jdk8` usually means mixed Kotlin versions. The plugin’s UPL should force **`org.jetbrains.kotlin` to 1.8.22**; if you still see this after merging other Gradle snippets, align or exclude conflicting Kotlin artifacts.
- **Xcode: "ExternalBuildToolExecution failed" / "never received target ended message":** The real error is from UnrealBuildTool. Check `~/Library/Application Support/Epic/UnrealBuildTool/Log.txt`. Clear Xcode caches: delete `~/Library/Developer/Xcode/DerivedData`, then regenerate project files and reopen. Alternatively, build from Unreal Editor (open the `.uproject`) or from the command line with the engine's `Build.sh` instead of Xcode.

---

## Rebuilding the project

**Option A:** Open `StashUnreal5.uproject` in Unreal Editor; the editor will compile the plugin.

**Option B:** Regenerate project files from the engine, then open the generated solution/workspace or the `.uproject`:

**macOS:**

```bash
UE_ROOT="/path/to/UnrealEngine"
"$UE_ROOT/Engine/Build/BatchFiles/Mac/GenerateProjectFiles.sh" -project="$(pwd)/StashUnreal5.uproject" -game
```

**Windows:**

```bat
"C:\Path\To\UnrealEngine\Engine\Build\BatchFiles\GenerateProjectFiles.bat" "C:\Path\To\stash-unreal-2\StashUnreal5.uproject" -game
```

**Clean rebuild:** Remove `Binaries`, `Intermediate`, `Saved/StagedBuilds`, then reopen the project.

---

## Documentation

- [Stash Documentation](https://docs.stash.gg)
- [stash-native](https://github.com/stashgg/stash-native)

## Versioning

This plugin follows semantic versioning (major.minor.patch). Pair it with **Stash Native 2.2.x** binaries in `ThirdParty` for **keep-alive notification icon** (`KeepAliveConfig.notificationIconResId`), **backgroundColor**, **onExternalPayment**, and **keep-alive** APIs. 
**StashNative-2.1.4+** remains required for checkout backdrop (`setBackdropBytes`).
**Stash Native 2.0+** uses **OpenCard**, **OpenModal**, **OpenBrowser**, and **CloseBrowser**; legacy **OpenCheckout** / **StashPay** naming is no longer used.

## Support

- Documentation: https://docs.stash.gg  
- Email: developers@stash.gg
