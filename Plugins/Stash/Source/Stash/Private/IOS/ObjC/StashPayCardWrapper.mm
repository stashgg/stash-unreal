// Copyright Stash. All Rights Reserved.
// Stash Pay Unreal Engine SDK - iOS Wrapper Implementation

#import "StashPayCardWrapper.h"
#import <StashPay/StashPay.h>

// Forward declaration of C++ callback functions
extern "C" {
    void StashPayOnPaymentSuccess(void);
    void StashPayOnPaymentFailure(void);
    void StashPayOnDialogDismissed(void);
    void StashPayOnPageLoaded(double loadTimeMs);
    void StashPayOnNetworkError(void);
}

#pragma mark - StashPayCardDelegate Bridge

/**
 * Delegate bridge that forwards StashPay SDK callbacks to C++ Unreal Engine code.
 */
@interface StashPayDelegateBridge : NSObject <StashPayCardDelegate>
@end

@implementation StashPayDelegateBridge

- (void)stashPayCardDidCompletePayment {
    StashPayOnPaymentSuccess();
}

- (void)stashPayCardDidFailPayment {
    StashPayOnPaymentFailure();
}

- (void)stashPayCardDidDismiss {
    StashPayOnDialogDismissed();
}

- (void)stashPayCardDidReceiveOptIn:(NSString *)optinType {
    // Opt-in responses are handled via dismiss callback
    (void)optinType;
}

- (void)stashPayCardDidLoadPage:(double)loadTimeMs {
    StashPayOnPageLoaded(loadTimeMs);
}

- (void)stashPayCardDidEncounterNetworkError {
    StashPayOnNetworkError();
}

@end

#pragma mark - StashPayCardWrapper Implementation

@implementation StashPayCardWrapper {
    StashPayDelegateBridge* _delegateBridge;
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
        _delegateBridge = [[StashPayDelegateBridge alloc] init];
        [StashPayCard sharedInstance].delegate = _delegateBridge;
    }
    return self;
}

#pragma mark - Checkout

- (void)openCheckoutWithURL:(NSString*)urlString {
    if (!urlString || urlString.length == 0) return;
    
    dispatch_async(dispatch_get_main_queue(), ^{
        [[StashPayCard sharedInstance] openCheckoutWithURL:urlString];
    });
}

- (BOOL)isCheckoutOpen {
    return [StashPayCard sharedInstance].isCurrentlyPresented;
}

- (void)dismissCheckout {
    dispatch_async(dispatch_get_main_queue(), ^{
        [[StashPayCard sharedInstance] dismiss];
    });
}

#pragma mark - Modal

- (void)openModalWithURL:(NSString*)urlString {
    if (!urlString || urlString.length == 0) return;
    
    dispatch_async(dispatch_get_main_queue(), ^{
        StashPayModalConfig* config = [[StashPayModalConfig alloc] init];
        config.showDragBar = YES;
        config.allowDismiss = YES;
        [[StashPayCard sharedInstance] openModalWithURL:urlString config:config];
    });
}

- (void)openModalWithURL:(NSString*)urlString
              showDragBar:(BOOL)showDragBar
             allowDismiss:(BOOL)allowDismiss
   phoneWidthRatioPortrait:(float)phoneWidthPortrait
  phoneHeightRatioPortrait:(float)phoneHeightPortrait
  phoneWidthRatioLandscape:(float)phoneWidthLandscape
 phoneHeightRatioLandscape:(float)phoneHeightLandscape
  tabletWidthRatioPortrait:(float)tabletWidthPortrait
 tabletHeightRatioPortrait:(float)tabletHeightPortrait
 tabletWidthRatioLandscape:(float)tabletWidthLandscape
tabletHeightRatioLandscape:(float)tabletHeightLandscape {
    if (!urlString || urlString.length == 0) return;
    
    dispatch_async(dispatch_get_main_queue(), ^{
        StashPayModalConfig* config = [[StashPayModalConfig alloc] init];
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
        [[StashPayCard sharedInstance] openModalWithURL:urlString config:config];
    });
}

#pragma mark - Checkout Sizing

- (void)setCardHeightRatioPortrait:(float)ratio {
    [StashPayCard sharedInstance].cardHeightRatioPortrait = ratio;
}

- (void)setTabletWidthRatioPortrait:(float)ratio {
    [StashPayCard sharedInstance].tabletWidthRatioPortrait = ratio;
}

- (void)setTabletHeightRatioPortrait:(float)ratio {
    [StashPayCard sharedInstance].tabletHeightRatioPortrait = ratio;
}

- (void)setTabletWidthRatioLandscape:(float)ratio {
    [StashPayCard sharedInstance].tabletWidthRatioLandscape = ratio;
}

- (void)setTabletHeightRatioLandscape:(float)ratio {
    [StashPayCard sharedInstance].tabletHeightRatioLandscape = ratio;
}

- (void)setForceWebBasedCheckout:(BOOL)force {
    [StashPayCard sharedInstance].forceWebBasedCheckout = force;
}

@end
