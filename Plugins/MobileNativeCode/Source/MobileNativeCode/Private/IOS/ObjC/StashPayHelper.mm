// Copyright Epic Games, Inc. All Rights Reserved.
// Bridge between Unreal Engine and stash-native iOS SDK

#include "StashPayHelper.h"
#include "CoreMinimal.h"

#if PLATFORM_IOS
#import <Foundation/Foundation.h>
#import "StashPayCard.h"

// Declare the C++ callback that will be called from Objective-C
// Must use extern "C" to prevent C++ name mangling
#ifdef __cplusplus
extern "C" {
#endif

void NotifyPaymentSuccessFromIOS(const char* ItemName);

#ifdef __cplusplus
}
#endif

// Objective-C delegate to handle StashPay callbacks
@interface StashPayHelperDelegate : NSObject <StashPayCardDelegate>
@end

@implementation StashPayHelperDelegate

- (void)stashPayCardDidCompletePayment {
    NSLog(@"[StashPayHelper] Payment completed successfully");
    UE_LOG(LogTemp, Warning, TEXT("[StashPayHelper] iOS Payment Success"));
    
    // Notify Unreal Engine of payment success
    NotifyPaymentSuccessFromIOS("iOS Purchase");
}

- (void)stashPayCardDidFailPayment {
    NSLog(@"[StashPayHelper] Payment failed");
    UE_LOG(LogTemp, Warning, TEXT("[StashPayHelper] iOS Payment Failed"));
}

- (void)stashPayCardDidDismiss {
    NSLog(@"[StashPayHelper] Checkout dismissed");
    UE_LOG(LogTemp, Warning, TEXT("[StashPayHelper] iOS Checkout Dismissed"));
}

- (void)stashPayCardDidLoadPage:(double)loadTimeMs {
    NSLog(@"[StashPayHelper] Page loaded in %.2f ms", loadTimeMs);
    UE_LOG(LogTemp, Warning, TEXT("[StashPayHelper] iOS Page Loaded in %.2f ms"), loadTimeMs);
}

@end

// Global delegate instance (retained manually - no ARC)
static StashPayHelperDelegate* g_StashPayDelegate = nil;

// Initialize the delegate if needed
void InitializeStashPayDelegate() {
    if (g_StashPayDelegate == nil) {
        g_StashPayDelegate = [[StashPayHelperDelegate alloc] init]; // Retained
        StashPayCard* stashPay = [StashPayCard sharedInstance];
        stashPay.delegate = g_StashPayDelegate;
        
        NSLog(@"[StashPayHelper] Initialized StashPay delegate");
        UE_LOG(LogTemp, Warning, TEXT("[StashPayHelper] Initialized StashPay delegate"));
    }
}

void OpenStashPayCheckoutIOS_Impl(const FString& URL)
{
    NSLog(@"[StashPayHelper] OpenStashPayCheckoutIOS_Impl called with URL: %s", TCHAR_TO_UTF8(*URL));
    UE_LOG(LogTemp, Warning, TEXT("[StashPayHelper] Opening StashPay checkout with URL: %s"), *URL);
    
    // Initialize delegate if needed
    InitializeStashPayDelegate();
    
    // Get the shared instance and open checkout
    StashPayCard* stashPay = [StashPayCard sharedInstance];
    NSString* urlString = URL.GetNSString();
    
    dispatch_async(dispatch_get_main_queue(), ^{
        [stashPay openCheckoutWithURL:urlString];
        NSLog(@"[StashPayHelper] openCheckoutWithURL called on main thread");
        UE_LOG(LogTemp, Warning, TEXT("[StashPayHelper] openCheckoutWithURL called on main thread"));
    });
}

void DismissStashPayCheckoutIOS_Impl()
{
    NSLog(@"[StashPayHelper] DismissStashPayCheckoutIOS_Impl called");
    UE_LOG(LogTemp, Warning, TEXT("[StashPayHelper] Dismissing StashPay checkout"));
    
    StashPayCard* stashPay = [StashPayCard sharedInstance];
    dispatch_async(dispatch_get_main_queue(), ^{
        [stashPay dismiss];
    });
}

bool IsStashPayCheckoutOpenIOS_Impl()
{
    StashPayCard* stashPay = [StashPayCard sharedInstance];
    BOOL isOpen = stashPay.isCurrentlyPresented;
    
    // Log occasionally for debugging
    static int logCounter = 0;
    if (logCounter++ % 100 == 0) {
        NSLog(@"[StashPayHelper] IsStashPayCheckoutOpenIOS_Impl: %d", isOpen);
    }
    
    return isOpen;
}

#endif
