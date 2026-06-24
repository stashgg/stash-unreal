// Copyright Stash. All Rights Reserved.
// Stash Unreal Engine SDK - iOS Wrapper Implementation (Stash Native 2.1+)

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

static BOOL _lastReportedPurchaseProcessing = NO;
static BOOL _purchaseProcessingPollSeenPresentation = NO;
static dispatch_source_t _purchaseProcessingPollSource = nil;

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

static void StashClearForcePortrait(void)
{
	if (_forcePortraitActive)
	{
		_forcePortraitActive = NO;
		[UIViewController attemptRotationToDeviceOrientation];
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
		if ((_forcePortraitActive || [[StashNativeCard sharedInstance] isCurrentlyPresented])
			&& window && window.windowLevel >= UIWindowLevelAlert)
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

static void TryInstallRootVCOrientationHookAndRetry(int attempt);

static void TryInstallRootVCOrientationHook(void)
{
	UIWindow* mainWindow = StashMainGameWindow();
	UIViewController* rootVC = mainWindow.rootViewController;
	if (!rootVC)
	{
		TryInstallRootVCOrientationHookAndRetry(0);
		return;
	}
	Class vcClass = [rootVC class];
	if (vcClass == _swizzledRootVCClass)
	{
		[UIViewController attemptRotationToDeviceOrientation];
		return;
	}
	SEL sel = @selector(supportedInterfaceOrientations);
	Method m = class_getInstanceMethod(vcClass, sel);
	if (m)
	{
		OriginalRootVCSupportedOrientations = (NSUInteger(*)(id, SEL))method_getImplementation(m);
		method_setImplementation(m, (IMP)StashRootVCSupportedInterfaceOrientations);
		_swizzledRootVCClass = vcClass;
		[UIViewController attemptRotationToDeviceOrientation];
	}
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
			Class vcClass = [rootVC class];
			if (vcClass != _swizzledRootVCClass)
			{
				SEL sel = @selector(supportedInterfaceOrientations);
				Method m = class_getInstanceMethod(vcClass, sel);
				if (m)
				{
					OriginalRootVCSupportedOrientations = (NSUInteger(*)(id, SEL))method_getImplementation(m);
					method_setImplementation(m, (IMP)StashRootVCSupportedInterfaceOrientations);
					_swizzledRootVCClass = vcClass;
				}
			}
			[UIViewController attemptRotationToDeviceOrientation];
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
		[StashNativeCard sharedInstance].delegate = _delegateBridge;
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
			[UIViewController attemptRotationToDeviceOrientation];
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
			[UIViewController attemptRotationToDeviceOrientation];
		}
	});
}

@end
