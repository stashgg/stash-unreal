// Copyright Stash. All Rights Reserved.
// Stash Pay Unreal Engine SDK - iOS Wrapper Implementation

#import "StashPayCardWrapper.h"
#import "StashPayCard.h"

// Forward declaration of C++ callback functions with extern "C" linkage
extern "C" {
    void StashPayOnPaymentSuccess(void);
    void StashPayOnPaymentFailure(void);
    void StashPayOnDialogDismissed(void);
    void StashPayOnPageLoaded(double loadTimeMs);
}

/**
 * Internal delegate implementation to receive StashPayCard callbacks
 */
@interface StashPayCardDelegateHandler : NSObject <StashPayCardDelegate>
@end

@implementation StashPayCardDelegateHandler

- (void)stashPayCardDidCompletePayment {
    NSLog(@"[StashPay] Payment completed successfully");
    StashPayOnPaymentSuccess();
}

- (void)stashPayCardDidFailPayment {
    NSLog(@"[StashPay] Payment failed");
    StashPayOnPaymentFailure();
}

- (void)stashPayCardDidDismiss {
    NSLog(@"[StashPay] Checkout dialog dismissed");
    StashPayOnDialogDismissed();
}

- (void)stashPayCardDidReceiveOptIn:(NSString *)optinType {
    NSLog(@"[StashPay] Opt-in received: %@", optinType);
    // Opt-in handling can be extended if needed
}

- (void)stashPayCardDidLoadPage:(double)loadTimeMs {
    NSLog(@"[StashPay] Page loaded in %.2f ms", loadTimeMs);
    StashPayOnPageLoaded(loadTimeMs);
}

@end

/**
 * StashPayCardWrapper implementation
 */
@implementation StashPayCardWrapper {
    StashPayCardDelegateHandler* _delegateHandler;
}

static StashPayCardWrapper* _sharedInstance = nil;

+ (StashPayCardWrapper*)sharedInstance {
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        _sharedInstance = [[StashPayCardWrapper alloc] init];
    });
    return _sharedInstance;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        // Create and retain the delegate handler
        _delegateHandler = [[StashPayCardDelegateHandler alloc] init];
        
        // Set up the StashPayCard singleton with our delegate
        StashPayCard* stashPay = [StashPayCard sharedInstance];
        stashPay.delegate = _delegateHandler;
        
        NSLog(@"[StashPay] StashPayCardWrapper initialized");
    }
    return self;
}

- (void)openCheckoutWithURL:(NSString*)urlString {
    if (urlString == nil || urlString.length == 0) {
        NSLog(@"[StashPay] Error: Empty checkout URL provided");
        return;
    }
    
    NSLog(@"[StashPay] Opening checkout with URL: %@", urlString);
    
    dispatch_async(dispatch_get_main_queue(), ^{
        // Force rotation to portrait on iPhone before showing checkout
        UIDevice* device = [UIDevice currentDevice];
        BOOL isPhone = (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone);
        UIInterfaceOrientation currentOrientation = [[UIApplication sharedApplication] statusBarOrientation];
        
        if (isPhone && UIInterfaceOrientationIsLandscape(currentOrientation)) {
            NSLog(@"[StashPay] Device is in landscape, forcing rotation to portrait");
            
            // Force the device orientation to portrait
            NSNumber* portraitValue = [NSNumber numberWithInt:UIInterfaceOrientationPortrait];
            [device setValue:portraitValue forKey:@"orientation"];
            
            // Notify the system of orientation change
            [UIViewController attemptRotationToDeviceOrientation];
            
            // Small delay to allow rotation to complete before showing checkout
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.3 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
                StashPayCard* stashPay = [StashPayCard sharedInstance];
                [stashPay openCheckoutWithURL:urlString];
            });
        } else {
            StashPayCard* stashPay = [StashPayCard sharedInstance];
            [stashPay openCheckoutWithURL:urlString];
        }
    });
}

- (BOOL)isCheckoutOpen {
    StashPayCard* stashPay = [StashPayCard sharedInstance];
    return stashPay.isCurrentlyPresented;
}

- (void)dismissCheckout {
    NSLog(@"[StashPay] Dismissing checkout");
    
    dispatch_async(dispatch_get_main_queue(), ^{
        StashPayCard* stashPay = [StashPayCard sharedInstance];
        [stashPay dismiss];
    });
}

@end
