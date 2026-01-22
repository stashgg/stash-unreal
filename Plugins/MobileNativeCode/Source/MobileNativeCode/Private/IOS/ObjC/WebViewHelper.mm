// Copyright Epic Games, Inc. All Rights Reserved.

#include "WebViewHelper.h"
#import <UIKit/UIKit.h>
#import <WebKit/WebKit.h>

static WKWebView* s_WebView = nil;
static UIView* s_WebViewContainer = nil;

void OpenIOSWebView_Impl(const FString& URL)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        UIViewController* RootViewController = [UIApplication sharedApplication].keyWindow.rootViewController;
        
        if (s_WebView == nil)
        {
            // Create container
            s_WebViewContainer = [[UIView alloc] initWithFrame:RootViewController.view.bounds];
            s_WebViewContainer.backgroundColor = [UIColor whiteColor];
            
            // Create WebView
            WKWebViewConfiguration* config = [[WKWebViewConfiguration alloc] init];
            s_WebView = [[WKWebView alloc] initWithFrame:s_WebViewContainer.bounds configuration:config];
            s_WebView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
            
            [s_WebViewContainer addSubview:s_WebView];
            [RootViewController.view addSubview:s_WebViewContainer];
        }
        
        NSString* URLString = [NSString stringWithUTF8String:TCHAR_TO_UTF8(*URL)];
        NSURL* nsURL = [NSURL URLWithString:URLString];
        NSURLRequest* request = [NSURLRequest requestWithURL:nsURL];
        [s_WebView loadRequest:request];
    });
}

void CloseIOSWebView_Impl()
{
    dispatch_async(dispatch_get_main_queue(), ^{
        if (s_WebViewContainer != nil)
        {
            [s_WebViewContainer removeFromSuperview];
            s_WebViewContainer = nil;
            s_WebView = nil;
        }
    });
}

bool IsIOSWebViewOpen_Impl()
{
    return s_WebView != nil && s_WebViewContainer != nil;
}
