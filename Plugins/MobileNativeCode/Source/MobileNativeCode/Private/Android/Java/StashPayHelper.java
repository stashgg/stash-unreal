// Copyright Stash. All Rights Reserved.
// Stash Pay Unreal Engine SDK - Android JNI Bridge

package com.Plugins.MobileNativeCode;

import android.app.Activity;
import android.util.Log;
import androidx.annotation.Keep;

import com.stash.popup.StashPayCard;

/**
 * StashPayHelper - Java wrapper for Stash Pay Android SDK
 * 
 * This class bridges the Stash Pay native Android SDK with Unreal Engine.
 * It provides static methods callable from C++ via JNI.
 */
@Keep
public class StashPayHelper {
    private static final String TAG = "StashPayHelper";
    private static boolean isInitialized = false;
    
    /**
     * Native C++ callback methods (implemented in MobileNativeCodeBlueprint.cpp)
     */
    public static native void nativeOnPaymentSuccess();
    public static native void nativeOnPaymentFailure();
    public static native void nativeOnDialogDismissed();
    public static native void nativeOnPageLoaded(long loadTimeMs);
    
    /**
     * Initializes the Stash Pay SDK with the given activity.
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
        
        Log.d(TAG, "Initializing StashPayHelper");
        
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
        });
        
        isInitialized = true;
        Log.d(TAG, "StashPayHelper initialized successfully");
    }
    
    /**
     * Opens the Stash Pay checkout dialog with the specified URL.
     * 
     * @param activity The current Android activity
     * @param url The checkout URL to load
     */
    @Keep
    public static void OpenCheckout(Activity activity, String url) {
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
}
