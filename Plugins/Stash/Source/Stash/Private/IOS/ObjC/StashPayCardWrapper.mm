// Copyright Stash. All Rights Reserved.
// Stash Pay Unreal Engine SDK - iOS Wrapper Implementation

#import "StashPayCardWrapper.h"
#import <StashPay/StashPay.h>
#import <objc/runtime.h>

// Forward declaration of C++ callback functions
extern "C" {
    void StashPayOnPaymentSuccess(void);
    void StashPayOnPaymentFailure(void);
    void StashPayOnDialogDismissed(void);
    void StashPayOnOptInResponse(const char* optinType);
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
    const char* utf8 = optinType ? [optinType UTF8String] : "";
    if (utf8) StashPayOnOptInResponse(utf8);
}

- (void)stashPayCardDidLoadPage:(double)loadTimeMs {
    StashPayOnPageLoaded(loadTimeMs);
}

- (void)stashPayCardDidEncounterNetworkError {
    StashPayOnNetworkError();
}

@end

#pragma mark - Landscape Lock (supportedInterfaceOrientations)

static BOOL _landscapeLockWhenCheckoutClosed = NO;
static NSUInteger (*OriginalRootVCSupportedOrientations)(id, SEL) = NULL;
static NSUInteger (*OriginalDelegateSupportedOrientationsForWindow)(id, SEL, UIApplication*, UIWindow*) = NULL;
static Class _swizzledRootVCClass = nil;

static UIWindow* StashMainGameWindow(void) {
    UIApplication* app = [UIApplication sharedApplication];
    for (UIWindow* w in app.windows) {
        if (w.rootViewController != nil && w.windowLevel == UIWindowLevelNormal)
            return w;
    }
    if (@available(iOS 15.0, *)) {
        for (UIScene* scene in app.connectedScenes) {
            if ([scene isKindOfClass:[UIWindowScene class]]) {
                UIWindowScene* ws = (UIWindowScene*)scene;
                if (ws.activationState == UISceneActivationStateForegroundActive) {
                    for (UIWindow* w in ws.windows) {
                        if (w.rootViewController != nil && w.windowLevel == UIWindowLevelNormal)
                            return w;
                    }
                }
            }
        }
    }
    return nil;
}

static NSUInteger StashRootVCSupportedInterfaceOrientations(UIViewController* self, SEL _cmd) {
    if (!_landscapeLockWhenCheckoutClosed) {
        if (OriginalRootVCSupportedOrientations) return OriginalRootVCSupportedOrientations(self, _cmd);
        return UIInterfaceOrientationMaskAll;
    }
    BOOL checkoutOpen = [[StashPayCard sharedInstance] isCurrentlyPresented];
    UIWindow* w = self.view.window;
    if (checkoutOpen && w && w.windowLevel >= UIWindowLevelAlert) {
        return UIInterfaceOrientationMaskAll;
    }
    return UIInterfaceOrientationMaskLandscapeLeft | UIInterfaceOrientationMaskLandscapeRight;
}

static NSUInteger StashDelegateSupportedOrientationsForWindow(id self, SEL _cmd, UIApplication* app, UIWindow* window) {
    if (!_landscapeLockWhenCheckoutClosed) {
        if (OriginalDelegateSupportedOrientationsForWindow) return OriginalDelegateSupportedOrientationsForWindow(self, _cmd, app, window);
        return UIInterfaceOrientationMaskAll;
    }
    BOOL checkoutOpen = [[StashPayCard sharedInstance] isCurrentlyPresented];
    if (checkoutOpen && window && window.windowLevel >= UIWindowLevelAlert) {
        return UIInterfaceOrientationMaskAll;
    }
    return UIInterfaceOrientationMaskLandscapeLeft | UIInterfaceOrientationMaskLandscapeRight;
}

static void InstallAppDelegateOrientationHook(void) {
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        id<UIApplicationDelegate> delegate = [UIApplication sharedApplication].delegate;
        if (delegate) {
            Class dClass = [delegate class];
            SEL dSel = @selector(application:supportedInterfaceOrientationsForWindow:);
            Method dM = class_getInstanceMethod(dClass, dSel);
            if (dM) {
                OriginalDelegateSupportedOrientationsForWindow = (NSUInteger(*)(id, SEL, UIApplication*, UIWindow*))method_getImplementation(dM);
                method_setImplementation(dM, (IMP)StashDelegateSupportedOrientationsForWindow);
            } else {
                class_addMethod(dClass, dSel, (IMP)StashDelegateSupportedOrientationsForWindow, "Q@:@@");
            }
        }
    });
}

static void TryInstallRootVCOrientationHookAndRetry(int attempt);

static void TryInstallRootVCOrientationHook(void) {
    UIWindow* mainWindow = StashMainGameWindow();
    UIViewController* rootVC = mainWindow.rootViewController;
    if (!rootVC) {
        TryInstallRootVCOrientationHookAndRetry(0);
        return;
    }
    Class vcClass = [rootVC class];
    if (vcClass == _swizzledRootVCClass) {
        [UIViewController attemptRotationToDeviceOrientation];
        return;
    }
    SEL sel = @selector(supportedInterfaceOrientations);
    Method m = class_getInstanceMethod(vcClass, sel);
    if (m) {
        OriginalRootVCSupportedOrientations = (NSUInteger(*)(id, SEL))method_getImplementation(m);
        method_setImplementation(m, (IMP)StashRootVCSupportedInterfaceOrientations);
        _swizzledRootVCClass = vcClass;
        [UIViewController attemptRotationToDeviceOrientation];
    }
}

static const int kLandscapeLockMaxRetries = 15;
static const NSTimeInterval kLandscapeLockRetryInterval = 0.2;

static void TryInstallRootVCOrientationHookAndRetry(int attempt) {
    if (attempt >= kLandscapeLockMaxRetries) return;
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(kLandscapeLockRetryInterval * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        UIViewController* rootVC = StashMainGameWindow().rootViewController;
        if (rootVC) {
            Class vcClass = [rootVC class];
            if (vcClass != _swizzledRootVCClass) {
                SEL sel = @selector(supportedInterfaceOrientations);
                Method m = class_getInstanceMethod(vcClass, sel);
                if (m) {
                    OriginalRootVCSupportedOrientations = (NSUInteger(*)(id, SEL))method_getImplementation(m);
                    method_setImplementation(m, (IMP)StashRootVCSupportedInterfaceOrientations);
                    _swizzledRootVCClass = vcClass;
                }
            }
            [UIViewController attemptRotationToDeviceOrientation];
        } else {
            TryInstallRootVCOrientationHookAndRetry(attempt + 1);
        }
    });
}

static void InstallLandscapeLockHooks(void) {
    InstallAppDelegateOrientationHook();
    TryInstallRootVCOrientationHook();
}

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

- (void)setForcePortraitOnCheckout:(BOOL)force {
    [StashPayCard sharedInstance].forcePortraitOnCheckout = force;
}

- (void)setCardHeightRatioPortrait:(float)ratio {
    [StashPayCard sharedInstance].cardHeightRatioPortrait = ratio;
}

- (void)setCardWidthRatioLandscape:(float)ratio {
    [StashPayCard sharedInstance].cardWidthRatioLandscape = ratio;
}

- (void)setCardHeightRatioLandscape:(float)ratio {
    [StashPayCard sharedInstance].cardHeightRatioLandscape = ratio;
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

- (void)setLandscapeLockWhenCheckoutClosed:(BOOL)enable {
    _landscapeLockWhenCheckoutClosed = enable;
    dispatch_async(dispatch_get_main_queue(), ^{
        InstallLandscapeLockHooks();
        if (enable) {
            [UIViewController attemptRotationToDeviceOrientation];
        }
    });
}

@end
