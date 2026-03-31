// Copyright Stash. All Rights Reserved.
// Stash Unreal Engine SDK - Android JNI Bridge (Stash Native 2.1+)

package com.Plugins.Stash;

import android.app.Activity;
import android.util.Log;
import androidx.annotation.Keep;

import com.stash.stashnative.StashNativeCard;

/**
 * StashHelper - Java wrapper for Stash Native Android SDK.
 *
 * Bridges the Stash Native Android SDK with Unreal Engine.
 * Provides static methods callable from C++ via JNI.
 */
@Keep
public class StashHelper {
    private static final String TAG = "StashHelper";
    private static volatile boolean isInitialized = false;
    private static final Object initLock = new Object();

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

            Log.d(TAG, "Initializing StashHelper (Stash Native 2.0)");

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
                    try {
                        nativeOnDialogDismissed();
                    } catch (Exception e) {
                        Log.e(TAG, "Error calling native onDialogDismissed: " + e.getMessage());
                    }
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
                    Log.d(TAG, "External payment URL: " + url);
                    try {
                        nativeOnExternalPayment(url != null ? url : "");
                    } catch (Exception e) {
                        Log.e(TAG, "Error calling native onExternalPayment: " + e.getMessage());
                    }
                }
            });

            isInitialized = true;
            Log.d(TAG, "StashHelper initialized successfully");
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
        Log.d(TAG, "Opening card with URL: " + url);
        activity.runOnUiThread(() -> {
            try {
                StashNativeCard.getInstance().openCard(url, null);
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
     */
    @Keep
    public static void OpenCardWithConfig(Activity activity, String url,
            boolean forcePortrait,
            float cardHeightRatioPortrait, float cardWidthRatioLandscape, float cardHeightRatioLandscape,
            float tabletWidthRatioPortrait, float tabletHeightRatioPortrait,
            float tabletWidthRatioLandscape, float tabletHeightRatioLandscape,
            String backgroundColor) {
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
        Log.d(TAG, "Opening card with config: " + url);
        activity.runOnUiThread(() -> {
            try {
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
            } catch (Exception e) {
                Log.e(TAG, "Error opening card with config: " + e.getMessage());
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
        });
    }

    // ========================================================================
    // Modal (Stash Native 2.0)
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
        Log.d(TAG, "Opening modal with URL: " + url);
        activity.runOnUiThread(() -> {
            try {
                StashNativeCard.getInstance().openModal(url, null);
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
        Log.d(TAG, "Opening modal with config: " + url);
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
        if (activity != null && !isInitialized) {
            Initialize(activity);
        }
        try {
            StashNativeCard.getInstance().setKeepAliveEnabled(enabled);
        } catch (Exception e) {
            Log.e(TAG, "Error setKeepAliveEnabled: " + e.getMessage());
        }
    }

    /**
     * Sets notification title and text for the keep-alive service.
     */
    @Keep
    public static void SetKeepAliveConfig(Activity activity, String notificationTitle, String notificationText) {
        if (activity != null && !isInitialized) {
            Initialize(activity);
        }
        try {
            StashNativeCard.KeepAliveConfig cfg = new StashNativeCard.KeepAliveConfig();
            cfg.notificationTitle = notificationTitle != null ? notificationTitle : "";
            cfg.notificationText = notificationText != null ? notificationText : "";
            StashNativeCard.getInstance().setKeepAliveConfig(cfg);
        } catch (Exception e) {
            Log.e(TAG, "Error setKeepAliveConfig: " + e.getMessage());
        }
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
        Log.d(TAG, "Opening browser: " + url);
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
