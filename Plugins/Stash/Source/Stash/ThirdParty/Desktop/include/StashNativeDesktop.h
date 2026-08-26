// C ABI of the Stash Native desktop hosts (Windows WebView2, macOS WKWebView).
//
// Identical on both OSes; this is the contract game engines bind to. Exported from
// StashNativeDesktop.dll / StashNativeDesktop.bundle. All strings are UTF-8, cdecl on Windows.
// Native apps and custom engines can use the typed facades instead (StashNativeCard.h on
// macOS, StashNativeCard.hpp on Windows); both layers drive the same core.
//
// Threading: calls may come from any thread on macOS (marshalled to the main queue). On
// Windows every call must come from the thread that owns the host window's message loop.
// Events are delivered on the UI thread, possibly from inside window-message dispatch:
// enqueue them and drain on the game loop, never touch engine APIs in the callback.
#ifndef STASH_NATIVE_DESKTOP_H
#define STASH_NATIVE_DESKTOP_H

#include "StashNativeDesktopVersion.h"

#if defined(_WIN32)
  #define STASH_NATIVE_DESKTOP_CALL __cdecl
  #if defined(STASH_NATIVE_DESKTOP_BUILDING)
    #define STASH_NATIVE_DESKTOP_API __declspec(dllexport)
  #elif defined(STASH_NATIVE_DESKTOP_NO_IMPORT)
    #define STASH_NATIVE_DESKTOP_API
  #else
    #define STASH_NATIVE_DESKTOP_API __declspec(dllimport)
  #endif
#else
  #define STASH_NATIVE_DESKTOP_CALL
  #define STASH_NATIVE_DESKTOP_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Event types. Names map 1:1 to the mobile listener / delegate callbacks.
#define STASH_NATIVE_DESKTOP_EVENT_PAYMENT_SUCCESS      "paymentSuccess"      /* payload: order string or empty */
#define STASH_NATIVE_DESKTOP_EVENT_PAYMENT_FAILURE      "paymentFailure"      /* payload: empty */
#define STASH_NATIVE_DESKTOP_EVENT_DIALOG_DISMISSED     "dialogDismissed"     /* payload: empty */
#define STASH_NATIVE_DESKTOP_EVENT_OPT_IN_RESPONSE      "optInResponse"       /* payload: opt-in type */
#define STASH_NATIVE_DESKTOP_EVENT_PAGE_LOADED          "pageLoaded"          /* payload: load time in ms */
#define STASH_NATIVE_DESKTOP_EVENT_NETWORK_ERROR        "networkError"        /* payload: empty */
#define STASH_NATIVE_DESKTOP_EVENT_EXTERNAL_PAYMENT     "externalPayment"     /* payload: themed URL */
#define STASH_NATIVE_DESKTOP_EVENT_PURCHASE_PROCESSING  "purchaseProcessing"  /* payload: empty */
#define STASH_NATIVE_DESKTOP_EVENT_PROCESSING_COMPLETED "processingCompleted" /* payload: empty */
// Diagnostics. Wrappers log these; they carry no host-facing semantics.
#define STASH_NATIVE_DESKTOP_EVENT_NAVIGATION           "navigation"          /* payload: URL */
#define STASH_NATIVE_DESKTOP_EVENT_NAVIGATION_BLOCKED   "navigationBlocked"   /* payload: {"url","reason"} */
#define STASH_NATIVE_DESKTOP_EVENT_WEB_PROCESS_CRASHED  "webProcessCrashed"   /* payload: "reloading" | "terminal" */
#define STASH_NATIVE_DESKTOP_EVENT_ERROR                "error"               /* payload: message */

// Config JSON keys (see docs/windows.md / docs/macos.md). Mobile field names are used verbatim so
// wrappers serialize their existing config structs. Desktop-only keys are set by wrappers.
#define STASH_NATIVE_DESKTOP_CONFIG_PRESENTATION   "presentation"   /* "attached" (default) | "window" */
#define STASH_NATIVE_DESKTOP_CONFIG_WIDTH          "width"          /* points, optional */
#define STASH_NATIVE_DESKTOP_CONFIG_HEIGHT         "height"         /* points, optional */
#define STASH_NATIVE_DESKTOP_CONFIG_ALLOW_FILE_URLS "allowFileUrls" /* test pages only */

typedef void (STASH_NATIVE_DESKTOP_CALL *StashNativeDesktopEventCallback)(const char *type,
                                                                          const char *payload,
                                                                          void *userData);

// One callback for every event. Pass NULL to clear. userData is handed back untouched.
STASH_NATIVE_DESKTOP_API void STASH_NATIVE_DESKTOP_CALL
StashNativeDesktop_SetEventCallback(StashNativeDesktopEventCallback callback, void *userData);

// HWND on Windows, NSWindow* on macOS. Optional: without it the core uses the key / main window.
STASH_NATIVE_DESKTOP_API void STASH_NATIVE_DESKTOP_CALL
StashNativeDesktop_SetHostWindow(void *nativeWindowHandle);

// The three presentation modes. configJson may be NULL or "{}" for defaults.
STASH_NATIVE_DESKTOP_API void STASH_NATIVE_DESKTOP_CALL
StashNativeDesktop_OpenCard(const char *url, const char *configJson);
STASH_NATIVE_DESKTOP_API void STASH_NATIVE_DESKTOP_CALL
StashNativeDesktop_OpenModal(const char *url, const char *configJson);
STASH_NATIVE_DESKTOP_API void STASH_NATIVE_DESKTOP_CALL
StashNativeDesktop_OpenBrowser(const char *url);

// Host-driven close (emits dialogDismissed) and recovery (no events).
STASH_NATIVE_DESKTOP_API void STASH_NATIVE_DESKTOP_CALL StashNativeDesktop_Dismiss(void);
STASH_NATIVE_DESKTOP_API void STASH_NATIVE_DESKTOP_CALL StashNativeDesktop_ResetPresentationState(void);

// Atomic state reads, safe from any thread.
STASH_NATIVE_DESKTOP_API int STASH_NATIVE_DESKTOP_CALL StashNativeDesktop_IsCurrentlyPresented(void);
STASH_NATIVE_DESKTOP_API int STASH_NATIVE_DESKTOP_CALL StashNativeDesktop_IsPurchaseProcessing(void);

// Creates the browser processes and a hidden webview so the first open is instant.
STASH_NATIVE_DESKTOP_API void STASH_NATIVE_DESKTOP_CALL StashNativeDesktop_Prewarm(void);

// Debug inspection of the checkout webviews (Safari Web Inspector / Edge DevTools). Off by default.
STASH_NATIVE_DESKTOP_API void STASH_NATIVE_DESKTOP_CALL
StashNativeDesktop_SetInspectableWebViewsEnabled(int enabled);

// Static version string (STASH_NATIVE_DESKTOP_VERSION). Never freed by the caller.
STASH_NATIVE_DESKTOP_API const char *STASH_NATIVE_DESKTOP_CALL StashNativeDesktop_GetVersion(void);

// Releases the webview environment and clears the callback. Call on quit and, in the Unity
// editor, before assembly reload. The library itself is never unloaded.
STASH_NATIVE_DESKTOP_API void STASH_NATIVE_DESKTOP_CALL StashNativeDesktop_Shutdown(void);

#ifdef __cplusplus
}
#endif

#endif
