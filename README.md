# Stash for Unreal Engine 4

<p align="left">
  <img src="https://github.com/stashgg/stash-native/raw/main/.github/assets/stash_unreal.png" width="128" height="128" alt="Stash Unreal Logo"/>
</p>

> **For Unreal Engine 5:**  
> This branch targets Unreal Engine 4.27+. For new projects, we recommend Unreal Engine 5 with our actively maintained SDK. See the [main branch](https://github.com/stashgg/stash-unreal) for UE5 support.

Unreal Engine plugin wrapper for [stash-native](https://github.com/stashgg/stash-native), enabling Stash Pay IAP checkout and webshop presentation on Android and iOS via C++ and Blueprints. The plugin uses **Stash Native** (**2.1.1+**).

## Requirements

- Unreal Engine 4.27+
- iOS 12.0+ / Android API 21+
- Xcode (iOS), Visual Studio (Windows/Android), Android SDK (Android)

## Sample / Downloads

- **Run the sample:** Clone this repo and open `StashUnreal4.uproject` in Unreal Engine 4.27+.
- **Use in your project:** Copy the `Plugins/Stash/` folder into your project’s `Plugins/` directory and enable the plugin under **Edit → Plugins → Mobile → Stash**.

## Quick Start

1. Add the Stash plugin (see above).
2. Enable **Edit → Plugins → Installed → Mobile → Stash** and restart the editor.
3. Call Stash Blueprint functions from the **Stash** category (e.g. **Open Card**, **Open Modal**, **Open Browser**).
4. **To react to payment success, dismiss, or other callbacks in Blueprint:** use **Get Stash Subsystem** (Stash category), then **Assign** or **Add** the event you need (e.g. **Add On Payment Success**, **Add On Dialog Dismissed**) on the returned subsystem. The static Stash nodes do not expose bindable delegates; the subsystem does.

### Folder structure

- **Plugins/Stash/** – Plugin root: `Source/Stash` (module), `ThirdParty` (StashNative AAR + XCFramework), `Resources`.
- **StashBlueprint** – Blueprint function library: `OpenCard`, `OpenModal`, `OpenBrowser`, `CloseBrowser`, config structs, delegates.
- **Key files:** `StashBlueprint.h`, iOS wrapper `StashNativeCardWrapper` (ObjC), Android `StashHelper.java`, ThirdParty StashNative binaries.

## Usage

Stash Native presents Stash Pay and webshop links in three ways: **openCard** (drawer/card), **openModal** (centered modal), and **openBrowser** (system browser). Checkout URLs must be generated on your backend; see the [Stash Pay Integration Guide](https://docs.stash.gg/guides/stash-pay/integration).

**iOS note:** The first OpenCard/OpenModal call can be slow under the Xcode debugger (WKWebView); production builds are unaffected.

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
    0.6f, 0.8f, 0.8f, 0.65f  // tablet ratios
);
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
2. Optionally: add **Make StashKeepAliveConfig** (or **Make** for struct **Stash Keep Alive Config**), set **Notification Title** and **Notification Text** (e.g. “Payment in progress” / “Tap to return to the app”), then call **Set Android Keep Alive Config** with that struct.

#### C++

```cpp
#include "StashBlueprint.h"

UStashBlueprint::SetAndroidKeepAliveEnabled(true);
FStashKeepAliveConfig KA;
KA.NotificationTitle = TEXT("Payment in progress");
KA.NotificationText = TEXT("Tap to return to the app");
UStashBlueprint::SetAndroidKeepAliveConfig(KA);
```

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
| `SetAndroidKeepAliveEnabled(bEnabled)` | (Android) Enable foreground keep-alive service during browser flows |
| `SetAndroidKeepAliveConfig(Config)` | (Android) Notification title/text for keep-alive (`FStashKeepAliveConfig`) |
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

**FStashCardConfig** (card): `bForcePortrait`, `CardHeightRatioPortrait`, `CardWidthRatioLandscape`, `CardHeightRatioLandscape`, `TabletWidthRatioPortrait`, `TabletHeightRatioPortrait`, `TabletWidthRatioLandscape`, `TabletHeightRatioLandscape`, **`BackgroundColor`** (optional HTML hex, e.g. `#RRGGBB`; leave empty for SDK default—see [stash-native](https://github.com/stashgg/stash-native/blob/main/README.md)).

**FStashModalConfig** (modal): `bAllowDismiss`, `PhoneWidthRatioPortrait`, `PhoneHeightRatioPortrait`, `PhoneWidthRatioLandscape`, `PhoneHeightRatioLandscape`, `TabletWidthRatioPortrait`, `TabletHeightRatioPortrait`, `TabletWidthRatioLandscape`, `TabletHeightRatioLandscape`, **`BackgroundColor`** (optional hex; empty = default).

**FStashKeepAliveConfig** (Android): `NotificationTitle`, `NotificationText` for the keep-alive notification.

---

## Blueprint usage

Use the **Stash** category nodes for Open Card, Open Modal, Open Browser, configs, and landscape lock. The two screenshots above show card and modal flows. For callbacks, use a C++ bridge that binds delegates and calls BlueprintImplementableEvent.

---

## Troubleshooting

- **Get Stash Subsystem returns null:** Ensure you pass a valid world context (e.g. **Self** from Level Blueprint or an Actor in a running game). In the editor before Play-in-Editor there is no play world, so the subsystem is not available.
- **Blueprint shows old nodes (Open Checkout, Set Force Web Based Checkout):** The plugin API is **Open Card**, **Open Card With Config**, **Open Browser**, **Close Browser**, **Is Card Open**, **Dismiss Card** (no Open Checkout, no Force Web Based). If you still see old names, do a **clean rebuild**: close the editor, delete the `Intermediate` and `Binaries` folders in your project root, then reopen the `.uproject`. The editor will recompile and Blueprint will show only the new Stash nodes.
- **iOS – undefined symbol / Library not loaded:** Add WebKit and SafariServices if needed; ensure **StashNative.xcframework** is embedded (Embed & Sign) and present under `Plugins/Stash/Source/Stash/ThirdParty/iOS/`.
- **Android – class not found / blank card:** Ensure **StashNative** AAR is in `ThirdParty/Android/` (e.g. `StashNative-2.1.1.aar`). Add internet permission. ProGuard: keep `com.stash.**`.
- **Android – crash on Open Browser with keep-alive:** `NoSuchMethodError` on `ServiceCompat.startForeground(Service, int, Notification, int)` means **`androidx.core` is too old** in the packaged APK. Ensure `Stash_UPL_Android.xml` still includes **`implementation 'androidx.core:core:1.13.1'`** (or newer); see the **Android keep-alive service** section above.
- **Android – Gradle `checkDebugDuplicateClasses` (Kotlin):** Duplicate classes in `kotlin-stdlib` vs `kotlin-stdlib-jdk7` / `kotlin-stdlib-jdk8` usually means mixed Kotlin versions. The plugin’s UPL should force **`org.jetbrains.kotlin` to 1.8.22**; if you still see this after merging other Gradle snippets, align or exclude conflicting Kotlin artifacts.
- **Xcode: "ExternalBuildToolExecution failed" / "never received target ended message":** The real error is from UnrealBuildTool. Check `~/Library/Application Support/Epic/UnrealBuildTool/Log.txt`. Clear Xcode caches: delete `~/Library/Developer/Xcode/DerivedData`, then regenerate project files and reopen. Alternatively, build from Unreal Editor (open the `.uproject`) or from the command line with the engine's `Build.sh` instead of Xcode.

---

## Rebuilding the project

**Option A:** Open `StashUnreal4.uproject` in Unreal Editor; the editor will compile the plugin.

**Option B:** Regenerate project files from the engine, then open the generated solution/workspace or the `.uproject`:

**macOS:**

```bash
UE_ROOT="/path/to/UnrealEngine-4.27"
"$UE_ROOT/Engine/Build/BatchFiles/Mac/GenerateProjectFiles.sh" -project="$(pwd)/StashUnreal4.uproject" -game
```

**Windows:**

```bat
"C:\Path\To\UnrealEngine\Engine\Build\BatchFiles\GenerateProjectFiles.bat" "C:\Path\To\stash-unreal-4\StashUnreal4.uproject" -game
```

**Clean rebuild:** Remove `Binaries`, `Intermediate`, `Saved/StagedBuilds`, then reopen the project.

---

## Documentation

- [Stash Documentation](https://docs.stash.gg)
- [stash-native](https://github.com/stashgg/stash-native)

## Versioning

This plugin follows semantic versioning (major.minor.patch). Pair it with **Stash Native 2.1.x** binaries in `ThirdParty` for **backgroundColor**, **onExternalPayment**, and **keep-alive** APIs. Stash Native 2.0+ uses **OpenCard**, **OpenModal**, **OpenBrowser**, and **CloseBrowser**; legacy **OpenCheckout** / **StashPay** naming is no longer used.

## Support

- Documentation: https://docs.stash.gg  
- Email: developers@stash.gg
