// Copyright Stash. All Rights Reserved.
// Stash Unreal Engine SDK - Android JNI Bridge

package com.Plugins.Stash;

import android.app.Activity;
import android.util.Log;
import androidx.annotation.Keep;

import com.stash.popup.StashPayCard;

/**
 * StashHelper - Java wrapper for Stash Android SDK
 * 
 * This class bridges the Stash native Android SDK with Unreal Engine.
 * It provides static methods callable from C++ via JNI.
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
    public static native void nativeOnPageLoaded(long loadTimeMs);
    public static native void nativeOnNetworkError();
    
    /**
     * Initializes the Stash SDK with the given activity.
     * Must be called before opening checkout.
     * 
     * @param activity The current Android activity
     */
    @Keep
    public static void Initialize(Activity activity) {
        if (activity == null) {
            Log.e(TAG, "Cannot initialize with null activity");
            return;
        }
        
        // Prevent redundant initialization using double-checked locking
        if (isInitialized) {
            Log.d(TAG, "StashHelper already initialized");
            return;
        }
        
        synchronized (initLock) {
            if (isInitialized) {
                return;
            }
            
            Log.d(TAG, "Initializing StashHelper");
            
            StashPayCard stashPay = StashPayCard.getInstance();
            stashPay.setActivity(activity);
            
            // Set up the listener to receive payment callbacks
            stashPay.setListener(new StashPayCard.StashPayListenerAdapter() {
            @Override
            public void onPaymentSuccess() {
                Log.d(TAG, "Payment completed successfully");
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
                Log.d(TAG, "Checkout dialog dismissed");
                try {
                    nativeOnDialogDismissed();
                } catch (Exception e) {
                    Log.e(TAG, "Error calling native onDialogDismissed: " + e.getMessage());
                }
            }
            
            @Override
            public void onOptInResponse(String optinType) {
                Log.d(TAG, "Opt-in received: " + optinType);
                // Opt-in handling can be extended if needed
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
            });
            
            isInitialized = true;
            Log.d(TAG, "StashHelper initialized successfully");
        }
    }
    
    /**
     * Opens the Stash checkout dialog with the specified URL.
     * 
     * @param activity The current Android activity
     * @param url The checkout URL to load
     */
    @Keep
    public static void OpenCheckout(Activity activity, String url) {
        if (activity == null) {
            Log.e(TAG, "Error: Cannot open checkout with null activity");
            return;
        }
        
        if (url == null || url.isEmpty()) {
            Log.e(TAG, "Error: Empty checkout URL provided");
            return;
        }
        
        // Auto-initialize if needed
        if (!isInitialized) {
            Initialize(activity);
        }
        
        Log.d(TAG, "Opening checkout with URL: " + url);
        
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                try {
                    StashPayCard stashPay = StashPayCard.getInstance();
                    stashPay.openCheckout(url);
                } catch (Exception e) {
                    Log.e(TAG, "Error opening checkout: " + e.getMessage());
                }
            }
        });
    }
    
    /**
     * Checks if the checkout dialog is currently open.
     * 
     * @return true if the checkout is currently displayed
     */
    @Keep
    public static boolean IsCheckoutOpen() {
        try {
            StashPayCard stashPay = StashPayCard.getInstance();
            return stashPay.isCurrentlyPresented();
        } catch (Exception e) {
            Log.e(TAG, "Error checking checkout state: " + e.getMessage());
            return false;
        }
    }
    
    /**
     * Dismisses the currently displayed checkout dialog.
     * 
     * @param activity The current Android activity
     */
    @Keep
    public static void DismissCheckout(Activity activity) {
        Log.d(TAG, "Dismissing checkout");
        
        if (activity == null) {
            Log.e(TAG, "Cannot dismiss with null activity");
            return;
        }
        
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                try {
                    StashPayCard stashPay = StashPayCard.getInstance();
                    stashPay.dismiss();
                } catch (Exception e) {
                    Log.e(TAG, "Error dismissing checkout: " + e.getMessage());
                }
            }
        });
    }
    
    // ========================================================================
    // Modal Presentation (SDK 1.2.0+)
    // ========================================================================
    
    /**
     * Opens a URL in a centered modal dialog with default configuration.
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
        
        // Auto-initialize if needed
        if (!isInitialized) {
            Initialize(activity);
        }
        
        Log.d(TAG, "Opening modal with URL: " + url);
        
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                try {
                    StashPayCard stashPay = StashPayCard.getInstance();
                    stashPay.openModal(url);
                } catch (Exception e) {
                    Log.e(TAG, "Error opening modal: " + e.getMessage());
                }
            }
        });
    }
    
    /**
     * Opens a URL in a centered modal dialog with custom configuration.
     * 
     * @param activity The current Android activity
     * @param url The URL to load in the modal
     * @param showDragBar Whether to show drag bar at top
     * @param allowDismiss Whether tap outside can dismiss
     * @param phoneWidthPortrait Phone width ratio in portrait
     * @param phoneHeightPortrait Phone height ratio in portrait
     * @param phoneWidthLandscape Phone width ratio in landscape
     * @param phoneHeightLandscape Phone height ratio in landscape
     * @param tabletWidthPortrait Tablet width ratio in portrait
     * @param tabletHeightPortrait Tablet height ratio in portrait
     * @param tabletWidthLandscape Tablet width ratio in landscape
     * @param tabletHeightLandscape Tablet height ratio in landscape
     */
    @Keep
    public static void OpenModalWithConfig(Activity activity, String url,
            boolean showDragBar, boolean allowDismiss,
            float phoneWidthPortrait, float phoneHeightPortrait,
            float phoneWidthLandscape, float phoneHeightLandscape,
            float tabletWidthPortrait, float tabletHeightPortrait,
            float tabletWidthLandscape, float tabletHeightLandscape) {
        if (activity == null) {
            Log.e(TAG, "Error: Cannot open modal with null activity");
            return;
        }
        
        if (url == null || url.isEmpty()) {
            Log.e(TAG, "Error: Empty modal URL provided");
            return;
        }
        
        // Auto-initialize if needed
        if (!isInitialized) {
            Initialize(activity);
        }
        
        Log.d(TAG, "Opening modal with URL: " + url + " (showDragBar=" + showDragBar + ", allowDismiss=" + allowDismiss + ")");
        
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                try {
                    StashPayCard stashPay = StashPayCard.getInstance();
                    
                    // Create configuration object
                    StashPayCard.ModalConfig config = new StashPayCard.ModalConfig();
                    config.showDragBar = showDragBar;
                    config.allowDismiss = allowDismiss;
                    config.phoneWidthRatioPortrait = phoneWidthPortrait;
                    config.phoneHeightRatioPortrait = phoneHeightPortrait;
                    config.phoneWidthRatioLandscape = phoneWidthLandscape;
                    config.phoneHeightRatioLandscape = phoneHeightLandscape;
                    config.tabletWidthRatioPortrait = tabletWidthPortrait;
                    config.tabletHeightRatioPortrait = tabletHeightPortrait;
                    config.tabletWidthRatioLandscape = tabletWidthLandscape;
                    config.tabletHeightRatioLandscape = tabletHeightLandscape;
                    
                    stashPay.openModal(url, config);
                } catch (Exception e) {
                    Log.e(TAG, "Error opening modal with config: " + e.getMessage());
                }
            }
        });
    }
    
    // ========================================================================
    // Checkout Sizing Configuration (SDK 1.2.0+)
    // ========================================================================
    
    /**
     * Sets the phone card height ratio for portrait orientation.
     * 
     * @param ratio Height ratio (0.1-1.0)
     */
    @Keep
    public static void SetCardHeightRatioPortrait(float ratio) {
        Log.d(TAG, "Setting card height ratio portrait: " + ratio);
        try {
            StashPayCard stashPay = StashPayCard.getInstance();
            stashPay.setCardHeightRatioPortrait(ratio);
        } catch (Exception e) {
            Log.e(TAG, "Error setting card height ratio: " + e.getMessage());
        }
    }
    
    /**
     * Sets the tablet card width ratio for portrait orientation.
     * 
     * @param ratio Width ratio (0.1-1.0)
     */
    @Keep
    public static void SetTabletWidthRatioPortrait(float ratio) {
        Log.d(TAG, "Setting tablet width ratio portrait: " + ratio);
        try {
            StashPayCard stashPay = StashPayCard.getInstance();
            stashPay.setTabletWidthRatioPortrait(ratio);
        } catch (Exception e) {
            Log.e(TAG, "Error setting tablet width ratio portrait: " + e.getMessage());
        }
    }
    
    /**
     * Sets the tablet card height ratio for portrait orientation.
     * 
     * @param ratio Height ratio (0.1-1.0)
     */
    @Keep
    public static void SetTabletHeightRatioPortrait(float ratio) {
        Log.d(TAG, "Setting tablet height ratio portrait: " + ratio);
        try {
            StashPayCard stashPay = StashPayCard.getInstance();
            stashPay.setTabletHeightRatioPortrait(ratio);
        } catch (Exception e) {
            Log.e(TAG, "Error setting tablet height ratio portrait: " + e.getMessage());
        }
    }
    
    /**
     * Sets the tablet card width ratio for landscape orientation.
     * 
     * @param ratio Width ratio (0.1-1.0)
     */
    @Keep
    public static void SetTabletWidthRatioLandscape(float ratio) {
        Log.d(TAG, "Setting tablet width ratio landscape: " + ratio);
        try {
            StashPayCard stashPay = StashPayCard.getInstance();
            stashPay.setTabletWidthRatioLandscape(ratio);
        } catch (Exception e) {
            Log.e(TAG, "Error setting tablet width ratio landscape: " + e.getMessage());
        }
    }
    
    /**
     * Sets the tablet card height ratio for landscape orientation.
     * 
     * @param ratio Height ratio (0.1-1.0)
     */
    @Keep
    public static void SetTabletHeightRatioLandscape(float ratio) {
        Log.d(TAG, "Setting tablet height ratio landscape: " + ratio);
        try {
            StashPayCard stashPay = StashPayCard.getInstance();
            stashPay.setTabletHeightRatioLandscape(ratio);
        } catch (Exception e) {
            Log.e(TAG, "Error setting tablet height ratio landscape: " + e.getMessage());
        }
    }
    
    /**
     * Sets whether to use web-based checkout (Chrome) instead of in-app UI.
     * 
     * @param force true to use Chrome Custom Tabs, false for in-app UI
     */
    @Keep
    public static void SetForceWebBasedCheckout(boolean force) {
        Log.d(TAG, "Setting force web-based checkout: " + force);
        try {
            StashPayCard stashPay = StashPayCard.getInstance();
            stashPay.setForceWebBasedCheckout(force);
        } catch (Exception e) {
            Log.e(TAG, "Error setting force web-based checkout: " + e.getMessage());
        }
    }
}
