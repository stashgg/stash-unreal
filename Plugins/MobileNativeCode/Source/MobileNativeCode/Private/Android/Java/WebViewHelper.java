package com.Plugins.MobileNativeCode;

import android.app.Activity;
import android.graphics.Color;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.webkit.WebChromeClient;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.FrameLayout;
import androidx.annotation.Keep;

@Keep
public class WebViewHelper {
    private static WebView webView = null;
    private static FrameLayout webViewContainer = null;
    private static Activity mainActivity = null;

    @Keep
    public static void Initialize(Activity activity) {
        mainActivity = activity;
    }

    @Keep
    public static void OpenWebView(String url) {
        if (mainActivity == null) {
            return;
        }

        mainActivity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (webView == null) {
                    // Create container
                    webViewContainer = new FrameLayout(mainActivity);
                    FrameLayout.LayoutParams containerParams = new FrameLayout.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT,
                        ViewGroup.LayoutParams.MATCH_PARENT
                    );
                    webViewContainer.setLayoutParams(containerParams);
                    webViewContainer.setBackgroundColor(Color.WHITE);
                    
                    // Make container intercept all touch events
                    webViewContainer.setClickable(true);
                    webViewContainer.setFocusable(true);
                    webViewContainer.setFocusableInTouchMode(true);

                    // Create WebView
                    webView = new WebView(mainActivity);
                    webView.getSettings().setJavaScriptEnabled(true);
                    webView.getSettings().setDomStorageEnabled(true);
                    webView.getSettings().setLoadWithOverviewMode(true);
                    webView.getSettings().setUseWideViewPort(true);
                    webView.setWebViewClient(new WebViewClient() {
                        @Override
                        public boolean shouldOverrideUrlLoading(WebView view, String url) {
                            view.loadUrl(url);
                            return true;
                        }
                    });
                    webView.setWebChromeClient(new WebChromeClient());
                    
                    // Make WebView intercept all touch events
                    webView.setClickable(true);
                    webView.setFocusable(true);
                    webView.setFocusableInTouchMode(true);
                    
                    // Handle back button to close WebView
                    webView.setOnKeyListener(new View.OnKeyListener() {
                        @Override
                        public boolean onKey(View v, int keyCode, KeyEvent event) {
                            if (keyCode == KeyEvent.KEYCODE_BACK && event.getAction() == KeyEvent.ACTION_DOWN) {
                                if (webView.canGoBack()) {
                                    webView.goBack();
                                    return true;
                                } else {
                                    CloseWebView();
                                    return true;
                                }
                            }
                            return false;
                        }
                    });

                    FrameLayout.LayoutParams webViewParams = new FrameLayout.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT,
                        ViewGroup.LayoutParams.MATCH_PARENT
                    );
                    webViewParams.gravity = Gravity.CENTER;
                    webView.setLayoutParams(webViewParams);

                    webViewContainer.addView(webView);

                    // Add to activity and bring to front
                    ViewGroup rootView = (ViewGroup) mainActivity.findViewById(android.R.id.content);
                    rootView.addView(webViewContainer);
                    // Bring to front to ensure it's above Unreal Engine view
                    webViewContainer.bringToFront();
                    // Request focus to intercept all input
                    webViewContainer.requestFocus();
                    webView.requestFocus();
                    rootView.requestLayout();
                    rootView.invalidate();
                } else {
                    // If WebView already exists, bring it to front
                    webViewContainer.bringToFront();
                    webViewContainer.requestFocus();
                    webView.requestFocus();
                    ViewGroup rootView = (ViewGroup) mainActivity.findViewById(android.R.id.content);
                    rootView.requestLayout();
                    rootView.invalidate();
                }

                if (url != null && !url.isEmpty()) {
                    // Check if it's a data URL - if so, use loadDataWithBaseURL for better support
                    if (url.startsWith("data:")) {
                        // Extract the HTML content from data URL
                        int commaIndex = url.indexOf(',');
                        if (commaIndex > 0) {
                            String htmlContent = url.substring(commaIndex + 1);
                            // URL decode if needed (data URLs may be encoded)
                            try {
                                htmlContent = java.net.URLDecoder.decode(htmlContent, "UTF-8");
                            } catch (Exception e) {
                                // If decoding fails, use as-is
                            }
                            webView.loadDataWithBaseURL("https://localhost/", htmlContent, "text/html", "UTF-8", null);
                        } else {
                            webView.loadUrl(url);
                        }
                    } else {
                        webView.loadUrl(url);
                    }
                }
            }
        });
    }

    @Keep
    public static void CloseWebView() {
        if (mainActivity == null) {
            return;
        }

        mainActivity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (webViewContainer != null) {
                    ViewGroup rootView = (ViewGroup) mainActivity.findViewById(android.R.id.content);
                    rootView.removeView(webViewContainer);
                    webViewContainer = null;
                    webView = null;
                }
            }
        });
    }

    @Keep
    public static boolean IsWebViewOpen() {
        return webView != null && webViewContainer != null;
    }
}
