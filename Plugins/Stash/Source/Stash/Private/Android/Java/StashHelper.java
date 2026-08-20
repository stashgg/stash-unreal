// Copyright Stash. All Rights Reserved.
// Stash Unreal Engine SDK - Android JNI Bridge (Stash Native 2.1+)

package com.Plugins.Stash;

import android.app.Activity;
import android.app.Application;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.JavascriptInterface;
import android.webkit.WebView;
import androidx.annotation.Keep;

import com.stash.stashnative.StashNativeCard;

import java.lang.ref.WeakReference;
import java.lang.reflect.Field;

/**
 * StashHelper - Java wrapper for Stash Native Android SDK.
 *
 * Bridges the Stash Native Android SDK with Unreal Engine.
 * Provides static methods callable from C++ via JNI.
 */
@Keep
public class StashHelper {
    private static final String TAG = "StashHelper";
    /** Filter logcat: adb logcat -s StashHelper:I *:S | grep StashBackdrop */
    private static final String BTAG = "[StashBackdrop]";
    // AND-03: purchase-processing detection reflects into stash-native PRIVATE internals. Every name
    // below is coupled to stash-native's implementation (verified present in StashNative-2.3.0.aar):
    //   - class  com.stash.stashnative.StashNativeCardPortraitActivity  (+ its private "isPurchaseProcessing" and "webView" fields)
    //   - field  StashNativeCard.plugin  → plugin.webView / plugin.currentDialog  (see resolveCheckoutWebView)
    // An SDK rename/refactor/obfuscation breaks these silently (Log.w only). If you bump the AAR, re-verify
    // these class/field names in review. This reflection also depends on the "-keep class com.stash.** { *; }"
    // ProGuard rule in Stash_UPL_Android.xml — without it R8 would strip these private fields. Keep the two coupled.
    private static final String PORTRAIT_ACTIVITY_CLASS =
            "com.stash.stashnative.StashNativeCardPortraitActivity";
    private static volatile boolean isInitialized = false;
    /** AND-03: false when the startup reflection self-check found a broken stash-native contract. */
    private static volatile boolean reflectionContractOk = false;
    private static volatile boolean checkoutLifecycleRegistered = false;
    private static volatile WeakReference<Activity> checkoutPortraitActivityRef;
    private static final Object initLock = new Object();
    private static final Handler mainHandler = new Handler(Looper.getMainLooper());

    /**
     * Native C++ callback methods (implemented in StashBlueprint.cpp)
     */
    public static native void nativeOnPaymentSuccess();
    public static native void nativeOnPaymentFailure();
    public static native void nativeOnDialogDismissed();
    public static native void nativeOnOptInResponse(String optinType);
    public static native void nativeOnPageLoaded(long loadTimeMs);
    public static native void nativeOnNetworkError();
    public static native void nativeOnExternalPayment(String url);
    public static native void nativeOnPurchaseProcessing();
    public static native void nativeOnProcessingCompleted();

    // AND-06: these flags are mutated from the main-thread poll AND from the SDK listener
    // (onDialogDismissed, whose thread is not guaranteed), so they are volatile. The
    // @JavascriptInterface bridge callbacks hop onto mainHandler so every write via
    // reportPurchaseProcessingState stays on the main thread.
    private static volatile boolean lastReportedPurchaseProcessing = false;
    private static volatile boolean purchaseProcessingPollActive = false;
    private static volatile boolean purchaseProcessingPollSeenPresentation = false;
    private static final long PURCHASE_PROCESSING_POLL_MS = 75L;
    // AND-05: stop the pre-presentation poll if openCard never reaches isCurrentlyPresented()
    // (bad URL, SDK failure) so the 75 ms runnable does not reschedule forever.
    private static final long PURCHASE_PROCESSING_PRESENT_TIMEOUT_MS = 15000L;
    private static long purchaseProcessingPollStartMs = 0L;
    private static final StashProcessingBridge PROCESSING_BRIDGE = new StashProcessingBridge();
    private static volatile WebView processingBridgeWebView;
    /** AND-05: true once PROCESSING_WRAP_JS has been evaluated for the current page; reset on each (re)load. */
    private static volatile boolean processingBridgeWrapped = false;
    /** Re-wraps stash_sdk after SDK injection (one-shot guard blocked re-wrap and missed portrait checkout). */
    private static final String PROCESSING_WRAP_JS =
        "(function(){"
        + "window.stash_sdk=window.stash_sdk||{};"
        + "var s=window.stash_sdk;"
        + "if(s.onPurchaseProcessing===s.__stashUnrealWrappedPp){return;}"
        + "var rawPp=s.onPurchaseProcessing,rawPc=s.onProcessingCompleted;"
        + "s.onPurchaseProcessing=function(d){"
        + "try{StashUnreal.onPurchaseProcessing();}catch(e){}"
        + "try{if(typeof rawPp==='function')rawPp.call(s,d);}catch(e){}};"
        + "s.onProcessingCompleted=function(d){"
        + "try{StashUnreal.onProcessingCompleted();}catch(e){}"
        + "try{if(typeof rawPc==='function')rawPc.call(s,d);}catch(e){}};"
        + "s.__stashUnrealWrappedPp=s.onPurchaseProcessing;"
        + "s.__stashUnrealWrappedPc=s.onProcessingCompleted;"
        + "})();";

    @Keep
    private static final class StashProcessingBridge {
        @JavascriptInterface
        public void onPurchaseProcessing() {
            // AND-06: bridge callbacks arrive on the WebView's JS thread; hop onto the main thread so
            // all purchase-processing state mutation happens on a single thread (matches the poll).
            mainHandler.post(() -> reportPurchaseProcessingState(true));
        }

        @JavascriptInterface
        public void onProcessingCompleted() {
            mainHandler.post(() -> reportPurchaseProcessingState(false));
        }
    }

    private static void reportPurchaseProcessingState(boolean processing) {
        if (processing == lastReportedPurchaseProcessing) {
            return;
        }
        lastReportedPurchaseProcessing = processing;
        try {
            if (processing) {
                nativeOnPurchaseProcessing();
            } else {
                nativeOnProcessingCompleted();
            }
        } catch (Exception e) {
            Log.e(TAG, "Error reporting purchase processing state: " + e.getMessage());
        }
    }

    private static void attachProcessingBridge() {
        try {
            WebView webView = resolveCheckoutWebView();
            if (webView == null) {
                return;
            }
            if (processingBridgeWebView != webView) {
                // AND-04: addJavascriptInterface only exposes "StashUnreal" to JS on the WebView's NEXT
                // page (re)load — the object is not visible to the page already loaded here, and the
                // wrapping JS swallows the resulting ReferenceError. The 75 ms reflection poll (not this
                // JS bridge) is therefore the AUTHORITATIVE delivery mechanism for purchase-processing
                // events; the bridge is only a best-effort supplement for pages loaded after injection.
                webView.addJavascriptInterface(PROCESSING_BRIDGE, "StashUnreal");
                processingBridgeWebView = webView;
                processingBridgeWrapped = false;
            }
            // AND-05: evaluate the wrap JS once per page instead of on every ~75 ms poll tick.
            if (!processingBridgeWrapped) {
                webView.evaluateJavascript(PROCESSING_WRAP_JS, null);
                processingBridgeWrapped = true;
            }
        } catch (Exception e) {
            Log.w(TAG, "Processing bridge attach failed: " + e.getMessage());
        }
    }

    private static Activity getCheckoutPortraitActivity() {
        WeakReference<Activity> ref = checkoutPortraitActivityRef;
        return ref != null ? ref.get() : null;
    }

    private static void registerCheckoutActivityTracking(Activity activity) {
        if (checkoutLifecycleRegistered || activity == null) {
            return;
        }
        checkoutLifecycleRegistered = true;
        activity.getApplication().registerActivityLifecycleCallbacks(new Application.ActivityLifecycleCallbacks() {
            @Override
            public void onActivityCreated(Activity createdActivity, Bundle savedInstanceState) {
            }

            @Override
            public void onActivityStarted(Activity startedActivity) {
            }

            @Override
            public void onActivityResumed(Activity resumedActivity) {
                if (PORTRAIT_ACTIVITY_CLASS.equals(resumedActivity.getClass().getName())) {
                    checkoutPortraitActivityRef = new WeakReference<>(resumedActivity);
                    scheduleProcessingBridgeAttach();
                }
            }

            @Override
            public void onActivityPaused(Activity pausedActivity) {
            }

            @Override
            public void onActivityStopped(Activity stoppedActivity) {
            }

            @Override
            public void onActivitySaveInstanceState(Activity activityState, Bundle outState) {
            }

            @Override
            public void onActivityDestroyed(Activity destroyedActivity) {
                Activity portrait = getCheckoutPortraitActivity();
                if (portrait == destroyedActivity) {
                    checkoutPortraitActivityRef = null;
                    processingBridgeWebView = null;
                }
            }
        });
    }

    private static WebView readWebViewField(Object target, String fieldName) {
        try {
            Field field = target.getClass().getDeclaredField(fieldName);
            field.setAccessible(true);
            return (WebView) field.get(target);
        } catch (Exception ignored) {
            return null;
        }
    }

    private static WebView findWebViewInView(View view) {
        if (view instanceof WebView) {
            return (WebView) view;
        }
        if (view instanceof ViewGroup) {
            ViewGroup group = (ViewGroup) view;
            for (int i = 0; i < group.getChildCount(); i++) {
                WebView found = findWebViewInView(group.getChildAt(i));
                if (found != null) {
                    return found;
                }
            }
        }
        return null;
    }

    private static WebView resolveCheckoutWebView() {
        Activity portraitActivity = getCheckoutPortraitActivity();
        if (portraitActivity != null) {
            WebView portraitWebView = readWebViewField(portraitActivity, "webView");
            if (portraitWebView != null) {
                return portraitWebView;
            }
        }

        try {
            // AND-03: reflects into stash-native private fields ("plugin", then "webView"/"currentDialog").
            // These names are coupled to the AAR (see PORTRAIT_ACTIVITY_CLASS comment) and to the com.stash.** ProGuard keep.
            StashNativeCard card = StashNativeCard.getInstance();
            Field pluginField = StashNativeCard.class.getDeclaredField("plugin");
            pluginField.setAccessible(true);
            Object plugin = pluginField.get(card);
            if (plugin == null) {
                return null;
            }

            WebView pluginWebView = readWebViewField(plugin, "webView");
            if (pluginWebView != null) {
                return pluginWebView;
            }

            Field dialogField = plugin.getClass().getDeclaredField("currentDialog");
            dialogField.setAccessible(true);
            android.app.Dialog dialog = (android.app.Dialog) dialogField.get(plugin);
            if (dialog != null && dialog.getWindow() != null) {
                return findWebViewInView(dialog.getWindow().getDecorView());
            }
        } catch (Exception e) {
            Log.w(TAG, "resolveCheckoutWebView failed: " + e.getMessage());
        }
        return null;
    }

    private static boolean resolvePurchaseProcessing() {
        Activity portraitActivity = getCheckoutPortraitActivity();
        if (portraitActivity != null) {
            try {
                // AND-03: reflects into the portrait activity's private "isPurchaseProcessing" field (coupled to the AAR).
                Field field = portraitActivity.getClass().getDeclaredField("isPurchaseProcessing");
                field.setAccessible(true);
                return field.getBoolean(portraitActivity);
            } catch (Exception e) {
                Log.w(TAG, "Failed to read portrait isPurchaseProcessing: " + e.getMessage());
            }
        }
        try {
            return StashNativeCard.getInstance().isPurchaseProcessing();
        } catch (Exception e) {
            return false;
        }
    }

    /**
     * AND-03 follow-up: one-time verification that every private stash-native name this class
     * reflects into still exists in the linked AAR.
     *
     * Without it a broken contract degrades SILENTLY: every reflection site catches and returns
     * null/false, which is indistinguishable from "no purchase is processing", so
     * OnPurchaseProcessing / OnProcessingCompleted simply stop firing with only a Log.w behind
     * them. The two realistic causes are (a) the AAR was upgraded and these private names changed,
     * and (b) the "-keep class com.stash.** { *; }" rule in Stash_UPL_Android.xml was narrowed, so
     * R8 stripped the private fields from a RELEASE build only (debug keeps working).
     *
     * Logs Log.e naming the exact missing members so the breakage is visible in logcat at startup
     * instead of surfacing as a missing spinner in the field.
     *
     * Verified against StashNative-2.3.0.aar:
     *   com.stash.stashnative.StashNativeCardPortraitActivity { isPurchaseProcessing, webView }
     *   com.stash.stashnative.StashNativeCard { plugin } -> { webView, currentDialog }
     */
    private static void verifyReflectionContract() {
        final StringBuilder missing = new StringBuilder();

        // Portrait checkout activity: authoritative purchase-processing source for the 75 ms poll.
        Class<?> portraitClass = null;
        try {
            portraitClass = Class.forName(PORTRAIT_ACTIVITY_CLASS);
        } catch (Throwable t) {
            appendMissing(missing, "class " + PORTRAIT_ACTIVITY_CLASS);
        }
        if (portraitClass != null) {
            if (!hasFieldInHierarchy(portraitClass, "isPurchaseProcessing")) {
                appendMissing(missing, PORTRAIT_ACTIVITY_CLASS + ".isPurchaseProcessing");
            }
            if (!hasFieldInHierarchy(portraitClass, "webView")) {
                appendMissing(missing, PORTRAIT_ACTIVITY_CLASS + ".webView");
            }
        }

        // StashNativeCard.plugin -> plugin.webView / plugin.currentDialog (see resolveCheckoutWebView).
        Class<?> pluginType = null;
        try {
            pluginType = StashNativeCard.class.getDeclaredField("plugin").getType();
        } catch (Throwable t) {
            appendMissing(missing, "StashNativeCard.plugin");
        }
        // The DECLARED type may be a base class/interface while the runtime instance is a subclass,
        // so a miss here is unverifiable rather than missing — resolveCheckoutWebView also falls
        // back to scanning the dialog's view tree. Warn instead of failing the check.
        if (pluginType != null
                && !hasFieldInHierarchy(pluginType, "webView")
                && !hasFieldInHierarchy(pluginType, "currentDialog")) {
            Log.w(TAG, "AND-03 self-check: neither 'webView' nor 'currentDialog' found on declared plugin type "
                    + pluginType.getName()
                    + "; they may live on a runtime subclass. WebView resolution falls back to a view-tree scan.");
        }

        String sdkVersion;
        try {
            sdkVersion = StashNativeCard.getVersion();
        } catch (Throwable t) {
            sdkVersion = "unknown";
        }

        if (missing.length() == 0) {
            reflectionContractOk = true;
            Log.i(TAG, "AND-03 self-check OK: stash-native reflection contract intact (SDK " + sdkVersion + ").");
            return;
        }

        reflectionContractOk = false;
        Log.e(TAG, "AND-03 self-check FAILED (SDK " + sdkVersion + "). Missing: " + missing + "."
                + " Purchase-processing callbacks (OnPurchaseProcessing / OnProcessingCompleted) will NOT fire."
                + " Either the StashNative AAR was upgraded and these private names changed - re-verify them"
                + " in StashHelper.java - or the '-keep class com.stash.** { *; }' rule in Stash_UPL_Android.xml"
                + " was narrowed and R8 stripped them from this build.");
    }

    private static void appendMissing(StringBuilder sb, String name) {
        if (sb.length() > 0) {
            sb.append(", ");
        }
        sb.append(name);
    }

    /** getDeclaredField does not search superclasses; the reflected members may be inherited. */
    private static boolean hasFieldInHierarchy(Class<?> type, String fieldName) {
        for (Class<?> c = type; c != null && c != Object.class; c = c.getSuperclass()) {
            try {
                c.getDeclaredField(fieldName);
                return true;
            } catch (NoSuchFieldException ignored) {
                // Keep walking up the hierarchy.
            } catch (Throwable t) {
                return false;
            }
        }
        return false;
    }

    private static void scheduleProcessingBridgeAttach() {
        // AND-05: a page (re)load invalidates any previously injected wrap; force re-evaluation on the next attach.
        processingBridgeWrapped = false;
        mainHandler.post(StashHelper::attachProcessingBridge);
        mainHandler.postDelayed(StashHelper::attachProcessingBridge, 250);
        mainHandler.postDelayed(StashHelper::attachProcessingBridge, 750);
        mainHandler.postDelayed(StashHelper::attachProcessingBridge, 1500);
        mainHandler.postDelayed(StashHelper::attachProcessingBridge, 3000);
    }

    private static final Runnable purchaseProcessingPollRunnable = new Runnable() {
        @Override
        public void run() {
            try {
                StashNativeCard card = StashNativeCard.getInstance();
                if (!card.isCurrentlyPresented()) {
                    if (purchaseProcessingPollSeenPresentation) {
                        if (lastReportedPurchaseProcessing) {
                            reportPurchaseProcessingState(false);
                        }
                        purchaseProcessingPollActive = false;
                        purchaseProcessingPollSeenPresentation = false;
                        return;
                    }
                    if (purchaseProcessingPollActive) {
                        // AND-05: give up if the card never presents (bad URL / SDK failure) instead of
                        // rescheduling forever. Once presentation is seen this branch no longer applies.
                        if (System.currentTimeMillis() - purchaseProcessingPollStartMs >= PURCHASE_PROCESSING_PRESENT_TIMEOUT_MS) {
                            Log.w(TAG, "Purchase processing poll timed out waiting for card presentation; stopping.");
                            purchaseProcessingPollActive = false;
                            return;
                        }
                        mainHandler.postDelayed(this, PURCHASE_PROCESSING_POLL_MS);
                    }
                    return;
                }

                purchaseProcessingPollSeenPresentation = true;
                attachProcessingBridge();
                reportPurchaseProcessingState(resolvePurchaseProcessing());
            } catch (Exception e) {
                Log.w(TAG, "Purchase processing poll error: " + e.getMessage());
            }
            if (purchaseProcessingPollActive) {
                mainHandler.postDelayed(this, PURCHASE_PROCESSING_POLL_MS);
            }
        }
    };

    private static void startPurchaseProcessingPoll() {
        if (!purchaseProcessingPollActive) {
            purchaseProcessingPollActive = true;
            purchaseProcessingPollSeenPresentation = false;
            purchaseProcessingPollStartMs = System.currentTimeMillis();
            mainHandler.post(purchaseProcessingPollRunnable);
        }
    }

    private static void stopPurchaseProcessingPoll() {
        purchaseProcessingPollActive = false;
        purchaseProcessingPollSeenPresentation = false;
        mainHandler.removeCallbacks(purchaseProcessingPollRunnable);
    }

    /**
     * Initializes the Stash Native SDK with the given activity.
     * Must be called before opening card, modal, or browser.
     *
     * @param activity The current Android activity
     */
    @Keep
    public static void Initialize(Activity activity) {
        if (activity == null) {
            Log.e(TAG, "Cannot initialize with null activity");
            return;
        }

        if (isInitialized) {
            Log.d(TAG, "StashHelper already initialized");
            return;
        }

        synchronized (initLock) {
            if (isInitialized) {
                return;
            }

            Log.d(TAG, "Initializing StashHelper (Stash Native 2.3.0)");

            // AND-03: verify the private stash-native names this class reflects into before
            // anything depends on them, so a broken contract is loud instead of silent.
            verifyReflectionContract();

            registerCheckoutActivityTracking(activity);
            StashNativeCard card = StashNativeCard.getInstance();
            card.setActivity(activity);
            card.setListener(new StashNativeCard.StashNativeCardListenerAdapter() {
                @Override
                public void onPaymentSuccess(String orderId) {
                    Log.d(TAG, "Payment completed successfully"
                            + (orderId != null && !orderId.isEmpty() ? (", orderId=" + orderId) : ""));
                    try {
                        nativeOnPaymentSuccess();
                    } catch (Exception e) {
                        Log.e(TAG, "Error calling native onPaymentSuccess: " + e.getMessage());
                    }
                }

                @Override
                public void onPaymentFailure() {
                    Log.d(TAG, "Payment failed");
                    try {
                        nativeOnPaymentFailure();
                    } catch (Exception e) {
                        Log.e(TAG, "Error calling native onPaymentFailure: " + e.getMessage());
                    }
                }

                @Override
                public void onDialogDismissed() {
                    Log.d(TAG, "Card/modal dismissed");
                    stopPurchaseProcessingPoll();
                    if (lastReportedPurchaseProcessing) {
                        reportPurchaseProcessingState(false);
                    }
                    Log.i(TAG, BTAG + " onDialogDismissed → scheduling clearCheckoutBackdropInternal (thread="
                            + Thread.currentThread().getName() + ")");
                    try {
                        nativeOnDialogDismissed();
                    } catch (Exception e) {
                        Log.e(TAG, "Error calling native onDialogDismissed: " + e.getMessage());
                    }
                    runOnMainThread(StashHelper::clearCheckoutBackdropInternal);
                }

                @Override
                public void onOptInResponse(String optinType) {
                    Log.d(TAG, "Opt-in received: " + optinType);
                    try {
                        nativeOnOptInResponse(optinType != null ? optinType : "");
                    } catch (Exception e) {
                        Log.e(TAG, "Error calling native onOptInResponse: " + e.getMessage());
                    }
                }

                @Override
                public void onPageLoaded(long loadTimeMs) {
                    Log.d(TAG, "Page loaded in " + loadTimeMs + " ms");
                    try {
                        nativeOnPageLoaded(loadTimeMs);
                    } catch (Exception e) {
                        Log.e(TAG, "Error calling native onPageLoaded: " + e.getMessage());
                    }
                    scheduleProcessingBridgeAttach();
                }

                @Override
                public void onNetworkError() {
                    Log.d(TAG, "Network error occurred");
                    try {
                        nativeOnNetworkError();
                    } catch (Exception e) {
                        Log.e(TAG, "Error calling native onNetworkError: " + e.getMessage());
                    }
                }

                @Override
                public void onExternalPayment(String url) {
                    // AND-14: external-payment URLs may carry tokens; log length only, not the full URL.
                Log.d(TAG, "External payment urlLen=" + (url != null ? url.length() : 0));
                    try {
                        nativeOnExternalPayment(url != null ? url : "");
                    } catch (Exception e) {
                        Log.e(TAG, "Error calling native onExternalPayment: " + e.getMessage());
                    }
                }
            });

            isInitialized = true;
            Log.d(TAG, "StashHelper initialized successfully");
            try {
                Log.i(TAG, BTAG + " StashNativeCard SDK version: " + StashNativeCard.getVersion());
            } catch (Exception e) {
                Log.w(TAG, BTAG + " Could not read StashNativeCard.getVersion(): " + e.getMessage());
            }
        }
    }

    /**
     * Opens the Stash card with the specified URL (default config).
     *
     * @param activity The current Android activity
     * @param url The URL to load in the card
     */
    @Keep
    public static void OpenCard(Activity activity, String url) {
        if (activity == null) {
            Log.e(TAG, "Error: Cannot open card with null activity");
            return;
        }
        if (url == null || url.isEmpty()) {
            Log.e(TAG, "Error: Empty card URL provided");
            return;
        }
        if (!isInitialized) {
            Initialize(activity);
        }
        // AND-14: checkout URLs carry session/checkout tokens; log length only (see OpenCardWithConfig urlLen).
        Log.d(TAG, "Opening card urlLen=" + url.length());
        activity.runOnUiThread(() -> {
            try {
                StashNativeCard.getInstance().openCard(url, null);
                startPurchaseProcessingPoll();
                scheduleProcessingBridgeAttach();
            } catch (Exception e) {
                Log.e(TAG, "Error opening card: " + e.getMessage());
            }
        });
    }

    /**
     * Opens the Stash card with the specified URL and config.
     *
     * @param activity The current Android activity
     * @param url The URL to load in the card
     * @param forcePortrait Whether to force portrait for phone card
     * @param cardHeightRatioPortrait Phone card height ratio portrait (0.1-1.0)
     * @param cardWidthRatioLandscape Phone card width ratio landscape (0.1-1.0)
     * @param cardHeightRatioLandscape Phone card height ratio landscape (0.1-1.0)
     * @param tabletWidthRatioPortrait Tablet width ratio portrait (0.1-1.0)
     * @param tabletHeightRatioPortrait Tablet height ratio portrait (0.1-1.0)
     * @param tabletWidthRatioLandscape Tablet width ratio landscape (0.1-1.0)
     * @param tabletHeightRatioLandscape Tablet height ratio landscape (0.1-1.0)
     * @param backgroundColor optional shell color
     * @param backdropOptional optional PNG/JPEG; applied on this UI runnable immediately before openCard (Android force-portrait)
     */
    @Keep
    public static void OpenCardWithConfig(Activity activity, String url,
            boolean forcePortrait,
            float cardHeightRatioPortrait, float cardWidthRatioLandscape, float cardHeightRatioLandscape,
            float tabletWidthRatioPortrait, float tabletHeightRatioPortrait,
            float tabletWidthRatioLandscape, float tabletHeightRatioLandscape,
            String backgroundColor,
            byte[] backdropOptional) {
        if (activity == null) {
            Log.e(TAG, "Error: Cannot open card with null activity");
            return;
        }
        if (url == null || url.isEmpty()) {
            Log.e(TAG, "Error: Empty card URL provided");
            return;
        }
        if (!isInitialized) {
            Initialize(activity);
        }
        final int backdropLen = backdropOptional != null ? backdropOptional.length : 0;
        Log.i(TAG, BTAG + " OpenCardWithConfig enqueue urlLen=" + (url != null ? url.length() : 0)
                + " forcePortrait=" + forcePortrait + " backdropBytes=" + backdropLen
                + " thread=" + Thread.currentThread().getName());
        activity.runOnUiThread(() -> {
            try {
                Log.i(TAG, BTAG + " OpenCardWithConfig UI runnable start thread=" + Thread.currentThread().getName());
                if (backdropOptional != null && backdropOptional.length > 0) {
                    applyDecodedBackdropOrLog(backdropOptional);
                } else {
                    Log.i(TAG, BTAG + " OpenCardWithConfig: no backdrop bytes on config (skipping setBackdrop)");
                }
                StashNativeCard.CardConfig config = new StashNativeCard.CardConfig();
                config.forcePortrait = forcePortrait;
                config.cardHeightRatioPortrait = cardHeightRatioPortrait;
                config.cardWidthRatioLandscape = cardWidthRatioLandscape;
                config.cardHeightRatioLandscape = cardHeightRatioLandscape;
                config.tabletWidthRatioPortrait = tabletWidthRatioPortrait;
                config.tabletHeightRatioPortrait = tabletHeightRatioPortrait;
                config.tabletWidthRatioLandscape = tabletWidthRatioLandscape;
                config.tabletHeightRatioLandscape = tabletHeightRatioLandscape;
                if (backgroundColor != null && !backgroundColor.isEmpty()) {
                    config.backgroundColor = backgroundColor;
                }
                StashNativeCard.getInstance().openCard(url, config);
                startPurchaseProcessingPoll();
                scheduleProcessingBridgeAttach();
                Log.i(TAG, BTAG + " openCard() returned (config.forcePortrait=" + config.forcePortrait + ")");
            } catch (Exception e) {
                Log.e(TAG, BTAG + " Error opening card with config: " + e.getMessage(), e);
            }
        });
    }

    /**
     * Checks if the card or modal is currently open.
     *
     * @return true if card/modal is currently displayed
     */
    @Keep
    public static boolean IsCardOpen() {
        try {
            return StashNativeCard.getInstance().isCurrentlyPresented();
        } catch (Exception e) {
            Log.e(TAG, "Error checking card state: " + e.getMessage());
            return false;
        }
    }

    /**
     * Checks if a purchase is currently being processed.
     *
     * @return true if a purchase is in progress
     */
    @Keep
    public static boolean IsPurchaseProcessing() {
        try {
            return StashNativeCard.getInstance().isPurchaseProcessing();
        } catch (Exception e) {
            Log.e(TAG, "Error checking purchase state: " + e.getMessage());
            return false;
        }
    }

    /**
     * Dismisses the currently displayed card or modal.
     *
     * @param activity The current Android activity
     */
    @Keep
    public static void DismissCard(Activity activity) {
        Log.d(TAG, "Dismissing card");
        if (activity == null) {
            Log.e(TAG, "Cannot dismiss with null activity");
            return;
        }
        activity.runOnUiThread(() -> {
            try {
                StashNativeCard.getInstance().dismiss();
            } catch (Exception e) {
                Log.e(TAG, "Error dismissing card: " + e.getMessage());
            }
            Log.i(TAG, BTAG + " DismissCard → clearCheckoutBackdropInternal");
            clearCheckoutBackdropInternal();
        });
    }

    // ========================================================================
    // Checkout backdrop (Android landscape → portrait; Stash Native 2.1.4+ setBackdropBytes)
    // ========================================================================

    private static void runOnMainThread(Runnable r) {
        if (Looper.myLooper() == Looper.getMainLooper()) {
            r.run();
        } else {
            mainHandler.post(r);
        }
    }

    private static void clearCheckoutBackdropInternal() {
        Log.i(TAG, BTAG + " clearCheckoutBackdropInternal thread=" + Thread.currentThread().getName());
        try {
            StashNativeCard.setBackdropBitmap(null);
            Log.i(TAG, BTAG + " setBackdropBitmap(null) OK");
        } catch (Exception e) {
            Log.e(TAG, BTAG + " Error clearing checkout backdrop: " + e.getMessage(), e);
        }
    }

    private static String hexHead(byte[] data, int max) {
        if (data == null || data.length == 0) {
            return "<empty>";
        }
        int n = Math.min(max, data.length);
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < n; i++) {
            sb.append(String.format("%02X", data[i] & 0xff));
            if (i + 1 < n) {
                sb.append(' ');
            }
        }
        if (data.length > n) {
            sb.append("…(total ").append(data.length).append(" bytes)");
        }
        return sb.toString();
    }

    /**
     * Decodes PNG/JPEG and forwards to {@link StashNativeCard#setBackdropBitmap}; logs if decode fails.
     */
    private static void applyDecodedBackdropOrLog(byte[] imageBytes) {
        final int len = imageBytes != null ? imageBytes.length : 0;
        Log.i(TAG, BTAG + " applyDecodedBackdropOrLog inputBytes=" + len + " head=" + hexHead(imageBytes, 8));
        Bitmap bmp = BitmapFactory.decodeByteArray(imageBytes, 0, imageBytes.length);
        if (bmp == null) {
            Log.e(TAG, BTAG + " BitmapFactory.decodeByteArray returned null (not valid PNG/JPEG?)");
            return;
        }
        Log.i(TAG, BTAG + " decoded bitmap " + bmp.getWidth() + "x" + bmp.getHeight()
                + " config=" + bmp.getConfig());
        StashNativeCard.setBackdropBitmap(bmp);
        Log.i(TAG, BTAG + " setBackdropBitmap OK");
    }

    /**
     * Sets image bytes (typically PNG or JPEG) shown behind the checkout shell during orientation transitions.
     * Call before OpenCard / OpenModal; cleared automatically on dismiss or via {@link #ClearCheckoutBackdrop}.
     *
     * @param activity   Game activity
     * @param imageBytes Encoded image bytes, or null / empty to clear
     */
    @Keep
    public static void SetCheckoutBackdropBytes(Activity activity, byte[] imageBytes) {
        final int len = imageBytes != null ? imageBytes.length : 0;
        Log.i(TAG, BTAG + " SetCheckoutBackdropBytes called bytes=" + len + " thread=" + Thread.currentThread().getName());
        if (activity == null) {
            Log.e(TAG, BTAG + " SetCheckoutBackdropBytes: null activity");
            return;
        }
        if (!isInitialized) {
            Initialize(activity);
        }
        activity.runOnUiThread(() -> {
            Log.i(TAG, BTAG + " SetCheckoutBackdropBytes UI runnable thread=" + Thread.currentThread().getName());
            try {
                if (imageBytes == null || imageBytes.length == 0) {
                    clearCheckoutBackdropInternal();
                } else {
                    applyDecodedBackdropOrLog(imageBytes);
                }
            } catch (Exception e) {
                Log.e(TAG, BTAG + " Error setting checkout backdrop: " + e.getMessage(), e);
            }
        });
    }

    /**
     * Clears any checkout backdrop set via {@link #SetCheckoutBackdropBytes}; safe to call repeatedly.
     */
    @Keep
    public static void ClearCheckoutBackdrop(Activity activity) {
        Log.i(TAG, BTAG + " ClearCheckoutBackdrop called thread=" + Thread.currentThread().getName());
        if (activity == null) {
            Log.e(TAG, BTAG + " ClearCheckoutBackdrop: null activity");
            return;
        }
        if (!isInitialized) {
            Initialize(activity);
        }
        activity.runOnUiThread(StashHelper::clearCheckoutBackdropInternal);
    }

    // ========================================================================
    // Modal (Stash Native 2.0+)
    // ========================================================================

    /**
     * Opens a URL in a centered modal with default configuration.
     *
     * @param activity The current Android activity
     * @param url The URL to load in the modal
     */
    @Keep
    public static void OpenModal(Activity activity, String url) {
        if (activity == null) {
            Log.e(TAG, "Error: Cannot open modal with null activity");
            return;
        }
        if (url == null || url.isEmpty()) {
            Log.e(TAG, "Error: Empty modal URL provided");
            return;
        }
        if (!isInitialized) {
            Initialize(activity);
        }
        // AND-14: log length only, not the token-bearing URL.
        Log.d(TAG, "Opening modal urlLen=" + url.length());
        activity.runOnUiThread(() -> {
            try {
                StashNativeCard.getInstance().openModal(url, null);
                startPurchaseProcessingPoll();
            } catch (Exception e) {
                Log.e(TAG, "Error opening modal: " + e.getMessage());
            }
        });
    }

    /**
     * Opens a URL in a centered modal with custom configuration.
     */
    @Keep
    public static void OpenModalWithConfig(Activity activity, String url,
            boolean allowDismiss,
            float phoneWidthPortrait, float phoneHeightPortrait,
            float phoneWidthLandscape, float phoneHeightLandscape,
            float tabletWidthPortrait, float tabletHeightPortrait,
            float tabletWidthLandscape, float tabletHeightLandscape,
            String backgroundColor) {
        if (activity == null) {
            Log.e(TAG, "Error: Cannot open modal with null activity");
            return;
        }
        if (url == null || url.isEmpty()) {
            Log.e(TAG, "Error: Empty modal URL provided");
            return;
        }
        if (!isInitialized) {
            Initialize(activity);
        }
        // AND-14: log length only, not the token-bearing URL.
        Log.d(TAG, "Opening modal with config urlLen=" + url.length());
        activity.runOnUiThread(() -> {
            try {
                StashNativeCard.ModalConfig config = new StashNativeCard.ModalConfig();
                config.allowDismiss = allowDismiss;
                config.phoneWidthRatioPortrait = phoneWidthPortrait;
                config.phoneHeightRatioPortrait = phoneHeightPortrait;
                config.phoneWidthRatioLandscape = phoneWidthLandscape;
                config.phoneHeightRatioLandscape = phoneHeightLandscape;
                config.tabletWidthRatioPortrait = tabletWidthPortrait;
                config.tabletHeightRatioPortrait = tabletHeightPortrait;
                config.tabletWidthRatioLandscape = tabletWidthLandscape;
                config.tabletHeightRatioLandscape = tabletHeightLandscape;
                if (backgroundColor != null && !backgroundColor.isEmpty()) {
                    config.backgroundColor = backgroundColor;
                }
                StashNativeCard.getInstance().openModal(url, config);
                startPurchaseProcessingPoll();
            } catch (Exception e) {
                Log.e(TAG, "Error opening modal with config: " + e.getMessage());
            }
        });
    }

    /**
     * Enables the foreground keep-alive service (Android low-memory / Chrome Custom Tabs).
     */
    @Keep
    public static void SetKeepAliveEnabled(Activity activity, boolean enabled) {
        // AND-15: mirror the other entry points — require non-null activity, ensure init, hop to the UI thread.
        if (activity == null) {
            Log.e(TAG, "Error: Cannot set keep-alive enabled with null activity");
            return;
        }
        if (!isInitialized) {
            Initialize(activity);
        }
        activity.runOnUiThread(() -> {
            try {
                StashNativeCard.getInstance().setKeepAliveEnabled(enabled);
            } catch (Exception e) {
                Log.e(TAG, "Error setKeepAliveEnabled: " + e.getMessage());
            }
        });
    }

    /**
     * Sets notification title, text, and optional small icon for the keep-alive service.
     *
     * @param notificationIconDrawableName drawable base name in the app package, or empty for SDK default
     */
    @Keep
    public static void SetKeepAliveConfig(
            Activity activity,
            String notificationTitle,
            String notificationText,
            String notificationIconDrawableName) {
        // AND-15: mirror the other entry points — require non-null activity, ensure init, hop to the UI thread.
        if (activity == null) {
            Log.e(TAG, "Error: Cannot set keep-alive config with null activity");
            return;
        }
        if (!isInitialized) {
            Initialize(activity);
        }
        activity.runOnUiThread(() -> {
            try {
                StashNativeCard.KeepAliveConfig cfg = new StashNativeCard.KeepAliveConfig();
                cfg.notificationTitle = notificationTitle != null ? notificationTitle : "";
                cfg.notificationText = notificationText != null ? notificationText : "";
                int iconResId = 0;
                if (notificationIconDrawableName != null) {
                    String name = notificationIconDrawableName.trim();
                    if (!name.isEmpty()) {
                        Context ctx = activity.getApplicationContext();
                        iconResId = ctx.getResources().getIdentifier(
                                name, "drawable", ctx.getPackageName());
                        if (iconResId == 0) {
                            Log.w(TAG, "Keep-alive notification icon drawable not found: "
                                    + name + " (add to Build/Android/res/drawable/)");
                        }
                    }
                }
                cfg.notificationIconResId = iconResId;
                StashNativeCard.getInstance().setKeepAliveConfig(cfg);
            } catch (Exception e) {
                Log.e(TAG, "Error setKeepAliveConfig: " + e.getMessage());
            }
        });
    }

    // ========================================================================
    // Browser (Stash Native 2.0)
    // ========================================================================

    /**
     * Opens the URL in Chrome Custom Tabs. No callbacks.
     *
     * @param activity The current Android activity
     * @param url The URL to open
     */
    @Keep
    public static void OpenBrowser(Activity activity, String url) {
        if (activity == null) {
            Log.e(TAG, "Error: Cannot open browser with null activity");
            return;
        }
        if (url == null || url.isEmpty()) {
            Log.e(TAG, "Error: Empty browser URL provided");
            return;
        }
        if (!isInitialized) {
            Initialize(activity);
        }
        // AND-14: log length only, not the token-bearing URL.
        Log.d(TAG, "Opening browser urlLen=" + url.length());
        activity.runOnUiThread(() -> {
            try {
                StashNativeCard.getInstance().openBrowser(url);
            } catch (Exception e) {
                Log.e(TAG, "Error opening browser: " + e.getMessage());
            }
        });
    }

    /**
     * No-op on Android (Chrome Custom Tabs cannot be closed by the app).
     * Exists for API consistency with iOS.
     */
    @Keep
    public static void CloseBrowser() {
        // No-op on Android
    }

}
