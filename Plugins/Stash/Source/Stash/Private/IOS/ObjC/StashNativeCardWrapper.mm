// Copyright Stash. All Rights Reserved.
// Stash Unreal Engine SDK - iOS Wrapper Implementation (Stash Native 2.3.0)

#import "StashNativeCardWrapper.h"
#import <StashNative/StashNative.h>
#import <objc/runtime.h>

// C callback functions (implemented in StashBlueprint.cpp)
extern "C" {
	void StashNativeOnPaymentSuccess(void);
	void StashNativeOnPaymentFailure(void);
	void StashNativeOnDialogDismissed(void);
	void StashNativeOnOptInResponse(const char* optinType);
	void StashNativeOnPageLoaded(double loadTimeMs);
	void StashNativeOnNetworkError(void);
	void StashNativeOnExternalPayment(const char* url);
	void StashNativeOnPurchaseProcessing(void);
	void StashNativeOnProcessingCompleted(void);
}

static void StashClearForcePortrait(void);
static void StashStartPurchaseProcessingPoll(void);
static void StashStopPurchaseProcessingPoll(void);

// If the card never presents (bad URL, SDK failure), the poll would otherwise fire on the
// main queue for the app's lifetime. Give it a pre-presentation timeout so it self-terminates.
static const NSTimeInterval kPurchaseProcessingPresentTimeout = 10.0;

static BOOL _lastReportedPurchaseProcessing = NO;
static BOOL _purchaseProcessingPollSeenPresentation = NO;
static dispatch_source_t _purchaseProcessingPollSource = nil;
static NSTimeInterval _purchaseProcessingPollDeadline = 0.0;

static void StashPurchaseProcessingPollFired(void)
{
	StashNativeCard* card = [StashNativeCard sharedInstance];
	if (!card.isCurrentlyPresented)
	{
		if (_purchaseProcessingPollSeenPresentation)
		{
			if (_lastReportedPurchaseProcessing)
			{
				_lastReportedPurchaseProcessing = NO;
				StashNativeOnProcessingCompleted();
			}
			StashStopPurchaseProcessingPoll();
			_purchaseProcessingPollSeenPresentation = NO;
		}
		else if (CFAbsoluteTimeGetCurrent() >= _purchaseProcessingPollDeadline)
		{
			// Card never reached a presented state within the timeout; stop polling.
			StashStopPurchaseProcessingPoll();
		}
		return;
	}

	_purchaseProcessingPollSeenPresentation = YES;

	const BOOL processing = card.isPurchaseProcessing;
	if (processing != _lastReportedPurchaseProcessing)
	{
		_lastReportedPurchaseProcessing = processing;
		if (processing)
		{
			StashNativeOnPurchaseProcessing();
		}
		else
		{
			StashNativeOnProcessingCompleted();
		}
	}
}

static void StashStartPurchaseProcessingPoll(void)
{
	StashStopPurchaseProcessingPoll();
	_purchaseProcessingPollSeenPresentation = NO;
	_purchaseProcessingPollDeadline = CFAbsoluteTimeGetCurrent() + kPurchaseProcessingPresentTimeout;
	dispatch_queue_t queue = dispatch_get_main_queue();
	_purchaseProcessingPollSource = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, queue);
	dispatch_source_set_timer(
		_purchaseProcessingPollSource,
		dispatch_time(DISPATCH_TIME_NOW, (int64_t)(150 * NSEC_PER_MSEC)),
		(int64_t)(150 * NSEC_PER_MSEC),
		(int64_t)(50 * NSEC_PER_MSEC));
	dispatch_source_set_event_handler(_purchaseProcessingPollSource, ^{
		StashPurchaseProcessingPollFired();
	});
	dispatch_resume(_purchaseProcessingPollSource);
}

static void StashStopPurchaseProcessingPoll(void)
{
	_purchaseProcessingPollSeenPresentation = NO;
	if (_purchaseProcessingPollSource)
	{
		dispatch_source_cancel(_purchaseProcessingPollSource);
		_purchaseProcessingPollSource = nil;
	}
}

#pragma mark - StashNativeDelegateBridge

/**
 * Delegate bridge that forwards Stash Native SDK callbacks to C++ Unreal Engine code.
 */
@interface StashNativeDelegateBridge : NSObject <StashNativeCardDelegate>
@end

@implementation StashNativeDelegateBridge

- (void)stashNativeCardDidCompletePayment
{
	StashNativeOnPaymentSuccess();
}

- (void)stashNativeCardDidFailPayment
{
	StashNativeOnPaymentFailure();
}

- (void)stashNativeCardDidDismiss
{
	StashStopPurchaseProcessingPoll();
	if (_lastReportedPurchaseProcessing)
	{
		_lastReportedPurchaseProcessing = NO;
		StashNativeOnProcessingCompleted();
	}
	StashClearForcePortrait();
	StashNativeOnDialogDismissed();
}

- (void)stashNativeCardDidReceiveOptIn:(NSString*)optinType
{
	const char* utf8 = optinType ? [optinType UTF8String] : "";
	if (utf8) StashNativeOnOptInResponse(utf8);
}

- (void)stashNativeCardDidLoadPage:(double)loadTimeMs
{
	StashNativeOnPageLoaded(loadTimeMs);
}

- (void)stashNativeCardDidEncounterNetworkError
{
	StashStopPurchaseProcessingPoll();
	if (_lastReportedPurchaseProcessing)
	{
		_lastReportedPurchaseProcessing = NO;
		StashNativeOnProcessingCompleted();
	}
	StashClearForcePortrait();
	StashNativeOnNetworkError();
}

- (void)stashNativeCardDidRequestExternalPaymentWithURL:(NSString*)url
{
	StashClearForcePortrait();
	const char* utf8 = url ? [url UTF8String] : "";
	if (utf8) StashNativeOnExternalPayment(utf8);
}

@end

#pragma mark - Landscape Lock (supportedInterfaceOrientations)

static BOOL _landscapeLockWhenCardClosed = NO;
static BOOL _forcePortraitActive = NO;
static NSUInteger (*OriginalRootVCSupportedOrientations)(id, SEL) = NULL;
static NSUInteger (*OriginalDelegateSupportedOrientationsForWindow)(id, SEL, UIApplication*, UIWindow*) = NULL;
static Class _swizzledRootVCClass = nil;

static UIWindow* StashMainGameWindow(void)
{
	UIApplication* app = [UIApplication sharedApplication];
	for (UIWindow* w in app.windows)
	{
		if (w.rootViewController != nil && w.windowLevel == UIWindowLevelNormal)
			return w;
	}
	if (@available(iOS 15.0, *))
	{
		for (UIScene* scene in app.connectedScenes)
		{
			if ([scene isKindOfClass:[UIWindowScene class]])
			{
				UIWindowScene* ws = (UIWindowScene*)scene;
				if (ws.activationState == UISceneActivationStateForegroundActive)
				{
					for (UIWindow* w in ws.windows)
					{
						if (w.rootViewController != nil && w.windowLevel == UIWindowLevelNormal)
							return w;
					}
				}
			}
		}
	}
	return nil;
}

// Desired orientation mask for the host window in our deterministic states (landscape lock or
// force portrait). Returns 0 when we have no explicit preference and the VC chain should decide.
static UIInterfaceOrientationMask StashDesiredInterfaceOrientationMask(void)
{
	if (_landscapeLockWhenCardClosed)
	{
		if (_forcePortraitActive || [[StashNativeCard sharedInstance] isCurrentlyPresented])
			return UIInterfaceOrientationMaskAll;
		return UIInterfaceOrientationMaskLandscapeLeft | UIInterfaceOrientationMaskLandscapeRight;
	}
	if (_forcePortraitActive)
		return UIInterfaceOrientationMaskAll;
	return 0;
}

// Re-evaluates supported interface orientations after our flags change.
//
// NOTE: -[UIViewController attemptRotationToDeviceOrientation] is deprecated and a no-op on
// iOS 16+, so on iOS 16+ we ask the VC chain to recompute its supported orientations
// (setNeedsUpdateOfSupportedInterfaceOrientations) and, when we have a deterministic mask,
// request a geometry update on the window scene so the window snaps immediately.
//
// TODO: rotation behavior of this path needs on-device verification on iOS 16/17.
static void StashRefreshSupportedInterfaceOrientations(void)
{
	if (@available(iOS 16.0, *))
	{
		UIWindow* window = StashMainGameWindow();
		for (UIViewController* vc = window.rootViewController; vc != nil; vc = vc.presentedViewController)
		{
			[vc setNeedsUpdateOfSupportedInterfaceOrientations];
		}

		UIInterfaceOrientationMask desired = StashDesiredInterfaceOrientationMask();
		if (desired != 0 && [window.windowScene isKindOfClass:[UIWindowScene class]])
		{
			UIWindowScene* scene = window.windowScene;
			UIWindowSceneGeometryPreferencesIOS* prefs =
				[[UIWindowSceneGeometryPreferencesIOS alloc] initWithInterfaceOrientations:desired];
			[scene requestGeometryUpdateWithPreferences:prefs errorHandler:^(NSError* error) {
				// Best-effort: setNeedsUpdateOfSupportedInterfaceOrientations above still applies.
			}];
		}
	}
	else
	{
		[UIViewController attemptRotationToDeviceOrientation];
	}
}

static void StashClearForcePortrait(void)
{
	if (_forcePortraitActive)
	{
		_forcePortraitActive = NO;
		StashRefreshSupportedInterfaceOrientations();
	}
}

static NSUInteger StashRootVCSupportedInterfaceOrientations(UIViewController* self, SEL _cmd)
{
	if (_landscapeLockWhenCardClosed)
		return UIInterfaceOrientationMaskLandscapeLeft | UIInterfaceOrientationMaskLandscapeRight;

	if (_forcePortraitActive)
		return UIInterfaceOrientationMaskAll;

	if (OriginalRootVCSupportedOrientations) return OriginalRootVCSupportedOrientations(self, _cmd);
	return UIInterfaceOrientationMaskAll;
}

static NSUInteger StashDelegateSupportedOrientationsForWindow(id self, SEL _cmd, UIApplication* app, UIWindow* window)
{
	if (_landscapeLockWhenCardClosed)
	{
		// Ask the SDK whether its own (portrait card / browser) window is the active one. This is
		// the SDK's supported manual path (see StashNativeCard.h) and replaces the previous
		// hand-rolled window-level heuristic.
		UIInterfaceOrientationMask stash = [StashNativeCard supportedInterfaceOrientationsForWindow:window];
		if (stash != 0)
			return stash;
		if (_forcePortraitActive)
			return UIInterfaceOrientationMaskAll;
		return UIInterfaceOrientationMaskLandscapeLeft | UIInterfaceOrientationMaskLandscapeRight;
	}

	if (_forcePortraitActive)
		return UIInterfaceOrientationMaskAll;

	if (OriginalDelegateSupportedOrientationsForWindow) return OriginalDelegateSupportedOrientationsForWindow(self, _cmd, app, window);
	return UIInterfaceOrientationMaskAll;
}

static void InstallAppDelegateOrientationHook(void)
{
	static dispatch_once_t once;
	dispatch_once(&once, ^{
		id<UIApplicationDelegate> delegate = [UIApplication sharedApplication].delegate;
		if (delegate)
		{
			Class dClass = [delegate class];
			SEL dSel = @selector(application:supportedInterfaceOrientationsForWindow:);
			Method dM = class_getInstanceMethod(dClass, dSel);
			if (dM)
			{
				OriginalDelegateSupportedOrientationsForWindow = (NSUInteger(*)(id, SEL, UIApplication*, UIWindow*))method_getImplementation(dM);
				method_setImplementation(dM, (IMP)StashDelegateSupportedOrientationsForWindow);
			}
			else
			{
				class_addMethod(dClass, dSel, (IMP)StashDelegateSupportedOrientationsForWindow, "Q@:@@");
			}
		}
	});
}

// Installs the supportedInterfaceOrientations hook on the given root VC's class.
//
// Safe-swizzle rules (IOS-07):
//   - class_getInstanceMethod returns inherited methods and method_setImplementation mutates the
//     *defining* class, so we must not blindly setImplementation on a possibly-inherited method
//     (that would rewrite UIViewController itself app-wide, including the SDK's card VC).
//   - We try class_addMethod first: it succeeds only when the class does NOT already define the
//     selector, adding our override and leaving the inherited IMP as the fallthrough. When it
//     fails, the class implements the selector itself and it is safe to setImplementation on it.
//   - Because there is a single "original IMP" global, we install on exactly one class. If a
//     different root VC class appears later, we log and skip rather than clobber the saved IMP.
//
// Returns YES when the hook is installed for rootVC's class (either newly or already).
static BOOL StashSwizzleRootVC(UIViewController* rootVC)
{
	if (!rootVC) return NO;

	Class vcClass = [rootVC class];
	if (_swizzledRootVCClass != nil)
	{
		if (vcClass == _swizzledRootVCClass) return YES;
		NSLog(@"[Stash] Root VC orientation hook already installed on %@; skipping re-swizzle on %@ to avoid overwriting the saved original implementation.",
			NSStringFromClass(_swizzledRootVCClass), NSStringFromClass(vcClass));
		return NO;
	}

	SEL sel = @selector(supportedInterfaceOrientations);
	Method inherited = class_getInstanceMethod(vcClass, sel);
	const char* types = inherited ? method_getTypeEncoding(inherited) : "Q@:";
	IMP newImp = (IMP)StashRootVCSupportedInterfaceOrientations;

	if (class_addMethod(vcClass, sel, newImp, types))
	{
		// The class did not define the selector itself; our override now shadows the inherited
		// implementation, which we call through for the pass-through case.
		OriginalRootVCSupportedOrientations = inherited
			? (NSUInteger(*)(id, SEL))method_getImplementation(inherited)
			: NULL;
	}
	else
	{
		// The class implements the selector itself; safe to swap its own implementation.
		Method own = class_getInstanceMethod(vcClass, sel);
		OriginalRootVCSupportedOrientations = (NSUInteger(*)(id, SEL))method_getImplementation(own);
		method_setImplementation(own, newImp);
	}
	_swizzledRootVCClass = vcClass;
	return YES;
}

static void TryInstallRootVCOrientationHookAndRetry(int attempt);

static void TryInstallRootVCOrientationHook(void)
{
	UIViewController* rootVC = StashMainGameWindow().rootViewController;
	if (!rootVC)
	{
		TryInstallRootVCOrientationHookAndRetry(0);
		return;
	}
	StashSwizzleRootVC(rootVC);
	StashRefreshSupportedInterfaceOrientations();
}

static const int kLandscapeLockMaxRetries = 15;
static const NSTimeInterval kLandscapeLockRetryInterval = 0.2;

static void TryInstallRootVCOrientationHookAndRetry(int attempt)
{
	if (attempt >= kLandscapeLockMaxRetries) return;
	dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(kLandscapeLockRetryInterval * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
		UIViewController* rootVC = StashMainGameWindow().rootViewController;
		if (rootVC)
		{
			StashSwizzleRootVC(rootVC);
			StashRefreshSupportedInterfaceOrientations();
		}
		else
		{
			TryInstallRootVCOrientationHookAndRetry(attempt + 1);
		}
	});
}

static void InstallLandscapeLockHooks(void)
{
	InstallAppDelegateOrientationHook();
	TryInstallRootVCOrientationHook();
}

#pragma mark - StashNativeCardWrapper Implementation

@implementation StashNativeCardWrapper
{
	StashNativeDelegateBridge* _delegateBridge;
}

static StashNativeCardWrapper* _sharedInstance = nil;

+ (StashNativeCardWrapper*)sharedInstance
{
	static dispatch_once_t once;
	dispatch_once(&once, ^{
		_sharedInstance = [[StashNativeCardWrapper alloc] init];
	});
	return _sharedInstance;
}

- (instancetype)init
{
	self = [super init];
	if (self)
	{
		_delegateBridge = [[StashNativeDelegateBridge alloc] init];
		StashNativeCard* card = [StashNativeCard sharedInstance];
		card.delegate = _delegateBridge;
		// Disable the SDK's own auto-swizzle of application:supportedInterfaceOrientationsForWindow:
		// so it does not compete with our delegate hook. Our hook defers to the SDK's supported
		// +supportedInterfaceOrientationsForWindow: API when the SDK's own window is active (IOS-06).
		card.disableAutoOrientationUnlock = YES;
	}
	return self;
}

#pragma mark - Card

- (void)openCardWithURL:(NSString*)urlString
{
	if (!urlString || urlString.length == 0) return;

	dispatch_async(dispatch_get_main_queue(), ^{
		[[StashNativeCard sharedInstance] openCardWithURL:urlString config:nil];
		StashStartPurchaseProcessingPoll();
	});
}

- (void)openCardWithURL:(NSString*)urlString
    forcePortrait:(BOOL)forcePortrait
    cardHeightRatioPortrait:(float)cardHeightRatioPortrait
    cardWidthRatioLandscape:(float)cardWidthRatioLandscape
    cardHeightRatioLandscape:(float)cardHeightRatioLandscape
    tabletWidthRatioPortrait:(float)tabletWidthRatioPortrait
    tabletHeightRatioPortrait:(float)tabletHeightRatioPortrait
    tabletWidthRatioLandscape:(float)tabletWidthRatioLandscape
    tabletHeightRatioLandscape:(float)tabletHeightRatioLandscape
    backgroundColor:(NSString*)backgroundColor
{
	if (!urlString || urlString.length == 0) return;

	dispatch_async(dispatch_get_main_queue(), ^{
		if (forcePortrait)
		{
			_forcePortraitActive = YES;
			InstallLandscapeLockHooks();
			StashRefreshSupportedInterfaceOrientations();
		}
		StashNativeCardConfig* config = [[StashNativeCardConfig alloc] init];
		config.forcePortrait = forcePortrait;
		config.cardHeightRatioPortrait = cardHeightRatioPortrait;
		config.cardWidthRatioLandscape = cardWidthRatioLandscape;
		config.cardHeightRatioLandscape = cardHeightRatioLandscape;
		config.tabletWidthRatioPortrait = tabletWidthRatioPortrait;
		config.tabletHeightRatioPortrait = tabletHeightRatioPortrait;
		config.tabletWidthRatioLandscape = tabletWidthRatioLandscape;
		config.tabletHeightRatioLandscape = tabletHeightRatioLandscape;
		if (backgroundColor != nil && backgroundColor.length > 0)
		{
			config.backgroundColor = backgroundColor;
		}
		[[StashNativeCard sharedInstance] openCardWithURL:urlString config:config];
		StashStartPurchaseProcessingPoll();
	});
}

- (BOOL)isCardOpen
{
	return [StashNativeCard sharedInstance].isCurrentlyPresented;
}

- (BOOL)isPurchaseProcessing
{
	return [StashNativeCard sharedInstance].isPurchaseProcessing;
}

- (void)dismissCard
{
	dispatch_async(dispatch_get_main_queue(), ^{
		[[StashNativeCard sharedInstance] dismiss];
	});
}

#pragma mark - Modal

- (void)openModalWithURL:(NSString*)urlString
{
	if (!urlString || urlString.length == 0) return;

	dispatch_async(dispatch_get_main_queue(), ^{
		StashNativeModalConfig* config = [[StashNativeModalConfig alloc] init];
		config.allowDismiss = YES;
		[[StashNativeCard sharedInstance] openModalWithURL:urlString config:config];
		StashStartPurchaseProcessingPoll();
	});
}

- (void)openModalWithURL:(NSString*)urlString
             allowDismiss:(BOOL)allowDismiss
   phoneWidthRatioPortrait:(float)phoneWidthRatioPortrait
  phoneHeightRatioPortrait:(float)phoneHeightRatioPortrait
  phoneWidthRatioLandscape:(float)phoneWidthRatioLandscape
 phoneHeightRatioLandscape:(float)phoneHeightRatioLandscape
  tabletWidthRatioPortrait:(float)tabletWidthRatioPortrait
 tabletHeightRatioPortrait:(float)tabletHeightRatioPortrait
 tabletWidthRatioLandscape:(float)tabletWidthRatioLandscape
tabletHeightRatioLandscape:(float)tabletHeightRatioLandscape
    backgroundColor:(NSString*)backgroundColor
{
	if (!urlString || urlString.length == 0) return;

	dispatch_async(dispatch_get_main_queue(), ^{
		StashNativeModalConfig* config = [[StashNativeModalConfig alloc] init];
		config.allowDismiss = allowDismiss;
		config.phoneWidthRatioPortrait = phoneWidthRatioPortrait;
		config.phoneHeightRatioPortrait = phoneHeightRatioPortrait;
		config.phoneWidthRatioLandscape = phoneWidthRatioLandscape;
		config.phoneHeightRatioLandscape = phoneHeightRatioLandscape;
		config.tabletWidthRatioPortrait = tabletWidthRatioPortrait;
		config.tabletHeightRatioPortrait = tabletHeightRatioPortrait;
		config.tabletWidthRatioLandscape = tabletWidthRatioLandscape;
		config.tabletHeightRatioLandscape = tabletHeightRatioLandscape;
		if (backgroundColor != nil && backgroundColor.length > 0)
		{
			config.backgroundColor = backgroundColor;
		}
		[[StashNativeCard sharedInstance] openModalWithURL:urlString config:config];
		StashStartPurchaseProcessingPoll();
	});
}

#pragma mark - Browser

- (void)openBrowserWithURL:(NSString*)urlString
{
	if (!urlString || urlString.length == 0) return;

	dispatch_async(dispatch_get_main_queue(), ^{
		[[StashNativeCard sharedInstance] openBrowserWithURL:urlString];
	});
}

- (void)closeBrowser
{
	dispatch_async(dispatch_get_main_queue(), ^{
		[[StashNativeCard sharedInstance] closeBrowser];
	});
}

#pragma mark - Configuration

- (void)setLandscapeLockWhenCardClosed:(BOOL)enable
{
	_landscapeLockWhenCardClosed = enable;
	dispatch_async(dispatch_get_main_queue(), ^{
		InstallLandscapeLockHooks();
		if (enable)
		{
			StashRefreshSupportedInterfaceOrientations();
		}
	});
}

@end
