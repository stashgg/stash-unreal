package com.Plugins.MobileNativeCode;

import android.app.Activity;
import com.stash.popup.StashPayCard;
import androidx.annotation.Keep;
import android.util.Log;

@Keep
public class StashPayHelper {
    private static final String TAG = "StashPayHelper";
    private static StashPayCard stashPay = null;
    private static Activity mainActivity = null;
    private static boolean isDismissed = false; // Track if dialog was explicitly dismissed
    private static android.os.Handler dismissHandler = null; // Handler for delayed dismissal
    private static Runnable dismissRunnable = null; // Runnable for delayed dismissal
    private static StashPayCard.StashPayListener currentListener = null; // Keep reference to prevent GC

    @Keep
    public static void Initialize(Activity activity) {
        Log.i(TAG, "Initialize called with activity: " + (activity != null ? activity.getClass().getName() : "null"));
        mainActivity = activity;
        // Note: We now do the full setup in OpenCheckout() to match GitHub example pattern
        // This just stores the activity reference
    }

    @Keep
    public static void OpenCheckout(String url) {
        Log.i(TAG, "OpenCheckout called with URL: " + url);
        
        if (mainActivity == null) {
            Log.e(TAG, "OpenCheckout: mainActivity is null!");
            return;
        }

        if (url == null || url.isEmpty()) {
            Log.e(TAG, "OpenCheckout: URL is null or empty!");
            return;
        }

        mainActivity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                try {
                    Log.i(TAG, "Running on UI thread...");
                    
                    // Follow GitHub example pattern exactly:
                    // 1. Get instance
                    if (stashPay == null) {
                        Log.i(TAG, "Getting StashPayCard instance...");
                        stashPay = StashPayCard.getInstance();
                        Log.i(TAG, "StashPayCard instance: " + (stashPay != null ? "success" : "null"));
                    }
                    
                    if (stashPay == null) {
                        Log.e(TAG, "Failed to get StashPayCard instance!");
                        return;
                    }
                    
                    // 2. Set activity (ensure it's set fresh each time)
                    Log.i(TAG, "Setting activity on StashPayCard...");
                    stashPay.setActivity(mainActivity);
                    Log.i(TAG, "Activity set successfully");
                    
                    // 3. Set listener (ensure it's set fresh each time)
                    Log.i(TAG, "Setting listener on StashPayCard...");
                    isDismissed = false; // Reset dismissed flag when opening
                    
                    // Cancel any pending dismissal when opening a new checkout
                    if (dismissHandler != null && dismissRunnable != null) {
                        dismissHandler.removeCallbacks(dismissRunnable);
                        dismissRunnable = null;
                    }
                    
                    // Create and store listener to prevent garbage collection
                    // Wrap in try-catch to ensure exceptions don't prevent callbacks
                    currentListener = new StashPayCard.StashPayListenerAdapter() {
                        @Override
                        public void onPaymentSuccess() {
                            try {
                                Log.i(TAG, "========== CALLBACK FIRED: onPaymentSuccess ==========");
                                Log.i(TAG, "StashPay: Payment successful!");
                                Log.i(TAG, "Callback thread: " + Thread.currentThread().getName());
                                Log.i(TAG, "Listener object: " + this);
                                
                                // Cancel any pending dismissal on success
                                if (dismissHandler != null && dismissRunnable != null) {
                                    dismissHandler.removeCallbacks(dismissRunnable);
                                    dismissRunnable = null;
                                    Log.i(TAG, "Cancelled pending auto-dismiss on payment success");
                                }
                                
                                // Mark as dismissed since payment succeeded
                                isDismissed = true;
                                
                                // Notify C++ that payment succeeded
                                // We'll pass an empty string for item name - C++ will use the stored CurrentPurchaseItemName
                                if (mainActivity != null) {
                                    try {
                                        // Call the GameActivity method via reflection
                                        java.lang.reflect.Method method = mainActivity.getClass().getMethod("AndroidThunkJava_NotifyPaymentSuccess", String.class);
                                        method.invoke(mainActivity, ""); // Empty string - C++ will use stored item name
                                        Log.i(TAG, "Notified C++ of payment success");
                                    } catch (Exception e) {
                                        Log.e(TAG, "Error calling AndroidThunkJava_NotifyPaymentSuccess: " + e.getMessage(), e);
                                    }
                                }
                                
                                Log.i(TAG, "Payment success callback completed successfully");
                            } catch (Exception e) {
                                Log.e(TAG, "CRITICAL ERROR in onPaymentSuccess callback: " + e.getMessage(), e);
                                e.printStackTrace();
                            } catch (Error e) {
                                Log.e(TAG, "CRITICAL ERROR (Error) in onPaymentSuccess callback: " + e.getMessage(), e);
                                e.printStackTrace();
                            }
                        }

                        @Override
                        public void onPaymentFailure() {
                            try {
                                Log.e(TAG, "========== CALLBACK FIRED: onPaymentFailure ==========");
                                Log.e(TAG, "StashPay: Payment failed! Will auto-dismiss in 5 seconds...");
                                Log.e(TAG, "Callback thread: " + Thread.currentThread().getName());
                                Log.e(TAG, "Listener object: " + this);
                                
                                // When payment fails, automatically dismiss after 5 seconds
                                // This gives the user time to see the error message
                                if (mainActivity != null) {
                                    dismissHandler = new android.os.Handler(android.os.Looper.getMainLooper());
                                    dismissRunnable = new Runnable() {
                                        @Override
                                        public void run() {
                                            Log.i(TAG, "Auto-dismissing StashPay after payment failure");
                                            if (stashPay != null) {
                                                try {
                                                    stashPay.dismiss();
                                                    isDismissed = true;
                                                    Log.i(TAG, "StashPay dismissed after payment failure");
                                                } catch (Exception e) {
                                                    Log.e(TAG, "Error dismissing StashPay after payment failure: " + e.getMessage(), e);
                                                }
                                            }
                                            dismissRunnable = null;
                                        }
                                    };
                                    // Dismiss after 5 seconds (5000ms)
                                    dismissHandler.postDelayed(dismissRunnable, 5000);
                                }
                                
                                Log.e(TAG, "Payment failure callback completed successfully");
                            } catch (Exception e) {
                                Log.e(TAG, "CRITICAL ERROR in onPaymentFailure callback: " + e.getMessage(), e);
                                e.printStackTrace();
                            } catch (Error e) {
                                Log.e(TAG, "CRITICAL ERROR (Error) in onPaymentFailure callback: " + e.getMessage(), e);
                                e.printStackTrace();
                            }
                        }

                        @Override
                        public void onDialogDismissed() {
                            try {
                                Log.i(TAG, "========== CALLBACK FIRED: onDialogDismissed ==========");
                                Log.i(TAG, "StashPay: Dialog dismissed.");
                                Log.i(TAG, "Callback thread: " + Thread.currentThread().getName());
                                Log.i(TAG, "Listener object: " + this);
                                
                                isDismissed = true; // Mark as dismissed when callback fires
                                // Cancel any pending dismissal since it's already dismissed
                                if (dismissHandler != null && dismissRunnable != null) {
                                    dismissHandler.removeCallbacks(dismissRunnable);
                                    dismissRunnable = null;
                                    Log.i(TAG, "Cancelled pending auto-dismiss on dialog dismissed");
                                }
                                
                                Log.i(TAG, "Dialog dismissed callback completed successfully");
                            } catch (Exception e) {
                                Log.e(TAG, "CRITICAL ERROR in onDialogDismissed callback: " + e.getMessage(), e);
                                e.printStackTrace();
                            } catch (Error e) {
                                Log.e(TAG, "CRITICAL ERROR (Error) in onDialogDismissed callback: " + e.getMessage(), e);
                                e.printStackTrace();
                            }
                        }
                        
                        @Override
                        public void onOptInResponse(String optinType) {
                            try {
                                Log.i(TAG, "========== CALLBACK FIRED: onOptInResponse ==========");
                                Log.i(TAG, "StashPay: Opt-in response: " + optinType);
                            } catch (Exception e) {
                                Log.e(TAG, "CRITICAL ERROR in onOptInResponse callback: " + e.getMessage(), e);
                            }
                        }
                        
                        @Override
                        public void onPageLoaded(long loadTimeMs) {
                            try {
                                Log.i(TAG, "========== CALLBACK FIRED: onPageLoaded ==========");
                                Log.i(TAG, "StashPay: Page loaded in " + loadTimeMs + "ms");
                            } catch (Exception e) {
                                Log.e(TAG, "CRITICAL ERROR in onPageLoaded callback: " + e.getMessage(), e);
                            }
                        }
                    };
                    
                    // Set the listener on StashPayCard
                    stashPay.setListener(currentListener);
                    Log.i(TAG, "Listener set successfully. Listener object: " + (currentListener != null ? "valid" : "null"));
                    
                    // Verify listener was set correctly
                    try {
                        StashPayCard.StashPayListener retrievedListener = stashPay.getListener();
                        if (retrievedListener != null) {
                            Log.i(TAG, "Listener verification: SUCCESS - Listener is attached to StashPayCard");
                            if (retrievedListener == currentListener) {
                                Log.i(TAG, "Listener verification: Listener reference matches!");
                            } else {
                                Log.w(TAG, "Listener verification: WARNING - Listener reference does not match (might be OK if SDK creates wrapper)");
                            }
                        } else {
                            Log.e(TAG, "Listener verification: FAILED - getListener() returned null!");
                        }
                    } catch (Exception e) {
                        Log.e(TAG, "Error verifying listener: " + e.getMessage(), e);
                    }
                    
                    // 4. Open checkout (following GitHub example exactly)
                    Log.i(TAG, "Calling stashPay.openCheckout with URL: " + url);
                    Log.i(TAG, "Before openCheckout - Listener status: " + (stashPay.getListener() != null ? "SET" : "NULL"));
                    Log.i(TAG, "Before openCheckout - Current listener object: " + currentListener);
                    Log.i(TAG, "Before openCheckout - Retrieved listener object: " + stashPay.getListener());
                    Log.i(TAG, "Before openCheckout - Listener objects match: " + (stashPay.getListener() == currentListener));
                    
                    // Verify the listener can be retrieved the same way the SDK does it
                    try {
                        StashPayCard testStashPay = StashPayCard.getInstance();
                        StashPayCard.StashPayListener testListener = testStashPay.getListener();
                        if (testListener != null && testListener == currentListener) {
                            Log.i(TAG, "VERIFICATION: StashPayCard.getInstance().getListener() returns our listener correctly");
                        } else {
                            Log.e(TAG, "VERIFICATION FAILED: StashPayCard.getInstance().getListener() does NOT return our listener!");
                            Log.e(TAG, "  Expected: " + currentListener);
                            Log.e(TAG, "  Got: " + testListener);
                        }
                    } catch (Exception e) {
                        Log.e(TAG, "VERIFICATION ERROR: " + e.getMessage(), e);
                    }
                    
                    stashPay.openCheckout(url);
                    Log.i(TAG, "stashPay.openCheckout() returned successfully");
                    Log.i(TAG, "After openCheckout - Listener status: " + (stashPay.getListener() != null ? "SET" : "NULL"));
                    Log.i(TAG, "After openCheckout - Current listener object: " + currentListener);
                    Log.i(TAG, "After openCheckout - Retrieved listener object: " + stashPay.getListener());
                    
                    // Verify again after openCheckout
                    try {
                        StashPayCard testStashPay2 = StashPayCard.getInstance();
                        StashPayCard.StashPayListener testListener2 = testStashPay2.getListener();
                        if (testListener2 != null && testListener2 == currentListener) {
                            Log.i(TAG, "POST-OPEN VERIFICATION: Listener still accessible via getInstance().getListener()");
                        } else {
                            Log.e(TAG, "POST-OPEN VERIFICATION FAILED: Listener lost after openCheckout!");
                        }
                    } catch (Exception e) {
                        Log.e(TAG, "POST-OPEN VERIFICATION ERROR: " + e.getMessage(), e);
                    }
                    
                    // Check status after a brief delay and periodically verify listener is still accessible
                    final android.os.Handler checkHandler = new android.os.Handler(android.os.Looper.getMainLooper());
                    final Runnable checkRunnable = new Runnable() {
                        private int checkCount = 0;
                        @Override
                        public void run() {
                            checkCount++;
                            boolean isOpen = stashPay.isCurrentlyPresented();
                            Log.i(TAG, String.format("Periodic check #%d: isCurrentlyPresented = %s", checkCount, isOpen));
                            
                            // Test if listener is still accessible
                            try {
                                StashPayCard.StashPayListener testListener = stashPay.getListener();
                                if (testListener != null) {
                                    Log.i(TAG, String.format("Periodic check #%d: Listener is accessible", checkCount));
                                    Log.i(TAG, String.format("Periodic check #%d: Listener matches currentListener: %s", 
                                        checkCount, (testListener == currentListener)));
                                    
                                    // Verify listener is the same instance we set
                                    if (testListener == currentListener) {
                                        Log.i(TAG, String.format("Periodic check #%d: Listener reference is CORRECT", checkCount));
                                    } else {
                                        Log.w(TAG, String.format("Periodic check #%d: WARNING - Listener reference changed! Expected: %s, Got: %s", 
                                            checkCount, currentListener, testListener));
                                    }
                                } else {
                                    Log.e(TAG, String.format("Periodic check #%d: CRITICAL - Listener is NULL!", checkCount));
                                    // Try to re-set the listener if it's null
                                    if (currentListener != null) {
                                        Log.w(TAG, String.format("Periodic check #%d: Attempting to re-set listener...", checkCount));
                                        stashPay.setListener(currentListener);
                                        Log.i(TAG, String.format("Periodic check #%d: Listener re-set. Verification: %s", 
                                            checkCount, (stashPay.getListener() == currentListener ? "SUCCESS" : "FAILED")));
                                    }
                                }
                            } catch (Exception e) {
                                Log.e(TAG, String.format("Periodic check #%d: Error checking listener: %s", checkCount, e.getMessage()), e);
                            }
                            
                            // Continue checking every 2 seconds while StashPay is open (max 30 checks = 60 seconds)
                            if (isOpen && checkCount < 30) {
                                checkHandler.postDelayed(this, 2000);
                            } else if (!isOpen) {
                                Log.i(TAG, String.format("Periodic check #%d: StashPay closed, stopping checks", checkCount));
                            } else {
                                Log.w(TAG, String.format("Periodic check #%d: Reached max checks (30), stopping", checkCount));
                            }
                        }
                    };
                    checkHandler.postDelayed(checkRunnable, 500);
                } catch (Exception e) {
                    Log.e(TAG, "Exception in OpenCheckout: " + e.getMessage(), e);
                    e.printStackTrace();
                } catch (Error e) {
                    Log.e(TAG, "Error in OpenCheckout: " + e.getMessage(), e);
                    e.printStackTrace();
                }
            }
        });
    }

    @Keep
    public static void DismissCheckout() {
        if (mainActivity == null || stashPay == null) {
            return;
        }

        // Cancel any pending auto-dismiss
        if (dismissHandler != null && dismissRunnable != null) {
            dismissHandler.removeCallbacks(dismissRunnable);
            dismissRunnable = null;
            Log.i(TAG, "Cancelled pending auto-dismiss");
        }

        mainActivity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (stashPay != null) {
                    try {
                        stashPay.dismiss();
                        isDismissed = true; // Mark as dismissed when we explicitly dismiss
                        Log.i(TAG, "StashPay manually dismissed");
                    } catch (Exception e) {
                        Log.e(TAG, "Error manually dismissing StashPay: " + e.getMessage(), e);
                    }
                }
            }
        });
    }

    @Keep
    public static boolean IsCheckoutOpen() {
        if (stashPay == null) {
            Log.w(TAG, "IsCheckoutOpen: stashPay is null, returning false");
            return false;
        }
        
        // If dialog was explicitly dismissed via callback, return false immediately
        if (isDismissed) {
            Log.i(TAG, "IsCheckoutOpen: Dialog was dismissed, returning false");
            return false;
        }
        
        try {
            boolean isOpen = stashPay.isCurrentlyPresented();
            Log.i(TAG, "IsCheckoutOpen: stashPay.isCurrentlyPresented() = " + isOpen);
            
            // If SDK says it's not open, update our dismissed flag
            if (!isOpen) {
                isDismissed = true;
            }
            
            return isOpen;
        } catch (Exception e) {
            Log.e(TAG, "Exception in IsCheckoutOpen: " + e.getMessage(), e);
            isDismissed = true; // On exception, assume dismissed
            return false;
        }
    }
}
