// Copyright Stash. All Rights Reserved.
// Stash Unreal Engine SDK - iOS Wrapper Implementation (Stash Native 2.3.0)

#import "StashNativeCardWrapper.h"
#import <StashNative/StashNative.h>
#import <objc/runtime.h>

// UE only resizes its Metal backbuffer from -[IOSAppDelegate didRotate:], which is driven by
// *physical* UIDeviceOrientationDidChangeNotification. The Stash SDK's forcePortrait rotates the
// UIWindowScene *programmatically* (no device notification), so UE would keep rendering at the old
// size, stretched into the rotated window. We call didRotate: directly after our transitions.
#import "IOS/IOSAppDelegate.h"
// NOTE: IOSView.h (IOSViewController/FIOSView) cannot be imported here — its include chain pulls
// in AppleControllerInterface.h, which uses MRC retain/release and fails under ARC. The landscape
// illusion resolves IOSViewController via NSClassFromString and treats FIOSView as a plain UIView.

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
static UIWindow* StashMainGameWindow(void);

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
			// Universal closure hook: auto-close after payment success/failure closes the card
			// WITHOUT firing stashNativeCardDidDismiss, which left force portrait (and the
			// landscape illusion) engaged forever. The poll sees every closure path; clearing
			// here is idempotent with the didDismiss/network-error/external-payment clears.
			StashClearForcePortrait();
		}
		else if (CFAbsoluteTimeGetCurrent() >= _purchaseProcessingPollDeadline)
		{
			// Card never reached a presented state within the timeout; stop polling and undo
			// force portrait so a failed presentation can't leave the scene stuck in portrait.
			StashStopPurchaseProcessingPoll();
			StashClearForcePortrait();
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

#pragma mark - Landscape Illusion (live game stays landscape during force-portrait)

// EXPERIMENTAL: keeps the live UE game view rendering in landscape while the scene rotates to
// portrait for a force-portrait checkout (portrait webview + portrait keyboard).
//
// How: UE 5.7 resizes its Metal backbuffer ONLY from -[IOSAppDelegate didRotate:]
// (GIOSDelayRotationUntilPresent defaults to 1, which makes FIOSView.layoutSubviews inert), so
// suppressing didRotate: freezes UE at its landscape size no matter what triggers it (physical
// device rotation, didBecomeActive, our own nudges). The FIOSView is then pinned to landscape
// bounds and counter-rotated 90° *inside* UIKit's own rotation transition animation
// (animateAlongsideTransition:), so the parent's rotation and our counter-rotation cancel out
// frame-by-frame — the game never visibly moves. The SDK's card/keyboard live in other windows
// and rotate to portrait normally.
//
// Engine-version sensitivity: relies on the didRotate:/GIOSDelayRotationUntilPresent contract and
// the Window→RootView→IOSView hierarchy (UE 5.7 LaunchIOS.cpp). Re-verify on engine upgrades.
static BOOL _landscapeIllusionEnabled = YES; // prototype default; the legacy rotate-the-game path runs when NO
static BOOL _landscapeIllusionActive = NO;   // didRotate suppression + transition pinning engaged
static BOOL _landscapeIllusionPinned = NO;   // Metal view currently counter-rotated
static UIInterfaceOrientation _illusionSourceOrientation = UIInterfaceOrientationLandscapeRight;

static void (*OriginalDidRotate)(id, SEL, NSNotification*) = NULL;
static void (*OriginalViewWillTransitionToSize)(id, SEL, CGSize, id) = NULL;

static void StashIllusionDidRotateOverride(id self, SEL _cmd, NSNotification* notification)
{
	if (_landscapeIllusionActive)
		return; // UE must not learn about the programmatic portrait scene
	if (OriginalDidRotate) OriginalDidRotate(self, _cmd, notification);
}

static UIView* StashUnrealMetalView(void)
{
	return (UIView*)[IOSAppDelegate GetDelegate].IOSView;
}

// Counter-rotation that keeps the landscape-rendered frame visually fixed relative to the physical
// screen once the interface is portrait. LandscapeRight's content is drawn +90° CW relative to
// portrait, so re-apply +90° after the scene un-rotates; LandscapeLeft is the mirror case.
static CGAffineTransform StashIllusionTransformFrom(UIInterfaceOrientation sourceOrientation)
{
	const CGFloat angle = (sourceOrientation == UIInterfaceOrientationLandscapeLeft) ? -M_PI_2 : M_PI_2;
	return CGAffineTransformMakeRotation(angle);
}

// Pin the Metal view: keep landscape bounds, center it in the portrait root view, counter-rotate.
// A WxH landscape layer rotated inside an HxW portrait parent fills it exactly — no scaling.
//
// Geometry is always DERIVED from the transition's target size, never saved/restored: by the time
// the alongside-animation block runs, UIKit has already re-laid the view for the new orientation,
// so a snapshot taken here would capture portrait geometry (verified on device — restoring it left
// a portrait-shaped view on the landscape screen).
static void StashIllusionPin(CGSize portraitSize)
{
	UIView* metalView = StashUnrealMetalView();
	if (!metalView) return;

	_landscapeIllusionPinned = YES;
	metalView.autoresizingMask = UIViewAutoresizingNone;
	const CGFloat landscapeW = MAX(portraitSize.width, portraitSize.height);
	const CGFloat landscapeH = MIN(portraitSize.width, portraitSize.height);
	metalView.bounds = CGRectMake(0, 0, landscapeW, landscapeH);
	metalView.center = CGPointMake(portraitSize.width * 0.5, portraitSize.height * 0.5);
	metalView.transform = StashIllusionTransformFrom(_illusionSourceOrientation);

	NSLog(@"[StashIllusion] pin: target=%@ superview=%@ drawable=%@",
		NSStringFromCGSize(portraitSize), NSStringFromCGRect(metalView.superview.bounds),
		NSStringFromCGSize(((CAMetalLayer*)metalView.layer).drawableSize));
}

// Un-pin: identity transform and landscape geometry filling the transition's target size.
static void StashIllusionRestore(CGSize landscapeSize)
{
	if (!_landscapeIllusionPinned) return;
	UIView* metalView = StashUnrealMetalView();
	if (metalView)
	{
		metalView.transform = CGAffineTransformIdentity;
		const CGFloat landscapeW = MAX(landscapeSize.width, landscapeSize.height);
		const CGFloat landscapeH = MIN(landscapeSize.width, landscapeSize.height);
		metalView.bounds = CGRectMake(0, 0, landscapeW, landscapeH);
		metalView.center = CGPointMake(landscapeSize.width * 0.5, landscapeSize.height * 0.5);
	}
	_landscapeIllusionPinned = NO;

	dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1.0 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
		UIView* v = StashUnrealMetalView();
		NSLog(@"[StashIllusion] post-restore: frame=%@ superview=%@ drawable=%@",
			NSStringFromCGRect(v.frame), NSStringFromCGRect(v.superview.bounds),
			NSStringFromCGSize(((CAMetalLayer*)v.layer).drawableSize));
	});
}

// Runs synchronized with UIKit's rotation animation so pin/restore cancel the visible rotation.
static void StashIllusionViewWillTransitionOverride(id self, SEL _cmd, CGSize size, id<UIViewControllerTransitionCoordinator> coordinator)
{
	UIInterfaceOrientation preTransitionOrientation = UIInterfaceOrientationUnknown;
	if (_landscapeIllusionActive)
	{
		UIWindow* window = StashMainGameWindow();
		preTransitionOrientation = window.windowScene ? window.windowScene.interfaceOrientation : UIInterfaceOrientationUnknown;
	}

	if (OriginalViewWillTransitionToSize) OriginalViewWillTransitionToSize(self, _cmd, size, coordinator);

	if (!_landscapeIllusionActive)
		return;

	const BOOL toPortrait = size.height > size.width;
	if (toPortrait && UIInterfaceOrientationIsLandscape(preTransitionOrientation))
	{
		_illusionSourceOrientation = preTransitionOrientation;
	}

	[coordinator animateAlongsideTransition:^(id<UIViewControllerTransitionCoordinatorContext> context) {
		if (toPortrait)
			StashIllusionPin(size);
		else
			StashIllusionRestore(size);
	} completion:^(id<UIViewControllerTransitionCoordinatorContext> context) {
		if (!toPortrait)
		{
			// Back to landscape: end suppression. UE's cached orientation still matches the scene
			// (it never saw the round-trip), so the follow-up nudges are no-ops.
			_landscapeIllusionActive = NO;
		}
	}];
}

static void InstallLandscapeIllusionHooks(void)
{
	static dispatch_once_t once;
	dispatch_once(&once, ^{
		// Both selectors are defined (not inherited) by their classes in UE 5.7
		// (IOSAppDelegate.cpp / IOSView.cpp), so method_setImplementation is safe (IOS-07).
		Method didRotateMethod = class_getInstanceMethod([IOSAppDelegate class], @selector(didRotate:));
		if (didRotateMethod)
		{
			OriginalDidRotate = (void (*)(id, SEL, NSNotification*))method_getImplementation(didRotateMethod);
			method_setImplementation(didRotateMethod, (IMP)StashIllusionDidRotateOverride);
		}

		Class engineVCClass = NSClassFromString(@"IOSViewController");
		Method transitionMethod = engineVCClass
			? class_getInstanceMethod(engineVCClass, @selector(viewWillTransitionToSize:withTransitionCoordinator:))
			: NULL;
		if (transitionMethod)
		{
			OriginalViewWillTransitionToSize = (void (*)(id, SEL, CGSize, id))method_getImplementation(transitionMethod);
			method_setImplementation(transitionMethod, (IMP)StashIllusionViewWillTransitionOverride);
		}
	});
}

// Engage before the portrait geometry request so the transition swizzle catches the rotation.
// Only meaningful when the scene is actually landscape right now.
static void StashIllusionEngageIfApplicable(void)
{
	if (!_landscapeIllusionEnabled) return;
	UIWindow* window = StashMainGameWindow();
	UIInterfaceOrientation current = window.windowScene ? window.windowScene.interfaceOrientation : UIInterfaceOrientationUnknown;
	if (!UIInterfaceOrientationIsLandscape(current)) return;

	InstallLandscapeIllusionHooks();
	_illusionSourceOrientation = current;
	_landscapeIllusionActive = YES;
}

// Failsafe: if the return-to-landscape transition never fires (geometry request failed, card
// dismissed while already landscape), don't leave didRotate suppressed forever.
static void StashIllusionScheduleDisengageFailsafe(void)
{
	dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1.5 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
		if (!_landscapeIllusionActive) return;
		UIWindow* window = StashMainGameWindow();
		UIInterfaceOrientation current = window.windowScene ? window.windowScene.interfaceOrientation : UIInterfaceOrientationUnknown;
		if (UIInterfaceOrientationIsLandscape(current))
		{
			StashIllusionRestore(window.rootViewController.view.bounds.size);
			_landscapeIllusionActive = NO;
			[[IOSAppDelegate GetDelegate] didRotate:nil];
		}
	});
}

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
//
// Force portrait must report portrait-ONLY (not MaskAll): UE's didRotate prefers the physical
// device orientation whenever the mask allows it, so MaskAll let UE pick landscape while the
// window scene was portrait — a permanent backbuffer/window mismatch (squished rendering).
// With portrait-only, every UE re-evaluation can only land on the scene's actual orientation.
static UIInterfaceOrientationMask StashDesiredInterfaceOrientationMask(void)
{
	if (_forcePortraitActive)
		return UIInterfaceOrientationMaskPortrait;
	if (_landscapeLockWhenCardClosed)
	{
		if ([[StashNativeCard sharedInstance] isCurrentlyPresented])
			return UIInterfaceOrientationMaskAll;
		return UIInterfaceOrientationMaskLandscapeLeft | UIInterfaceOrientationMaskLandscapeRight;
	}
	return 0;
}

// Asks UE to re-check the scene orientation and resize its backbuffer. Two staggered calls:
// one right after the geometry request is submitted, one after UIKit's rotation animation has
// settled (didRotate is idempotent — it no-ops when the cached orientation already matches).
static void StashNudgeUnrealOrientationRefresh(void)
{
	for (const double delay : {0.05, 0.45})
	{
		dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(delay * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
			[[IOSAppDelegate GetDelegate] didRotate:nil];
		});
	}
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

	// Programmatic scene rotation fires no device-orientation notification, so UE never resizes
	// on its own — always follow up with an explicit engine refresh.
	StashNudgeUnrealOrientationRefresh();
}

static void StashClearForcePortrait(void)
{
	if (_forcePortraitActive)
	{
		_forcePortraitActive = NO;
		StashRefreshSupportedInterfaceOrientations();
		if (_landscapeIllusionActive)
		{
			// Normal disengage happens in the transition swizzle when the scene rotates back;
			// this covers transitions that never fire.
			StashIllusionScheduleDisengageFailsafe();
		}
	}
}

static NSUInteger StashRootVCSupportedInterfaceOrientations(UIViewController* self, SEL _cmd)
{
	// Force portrait wins over landscape lock: portrait-only keeps UIKit's window state and UE's
	// backbuffer orientation in lockstep (see StashDesiredInterfaceOrientationMask).
	if (_forcePortraitActive)
		return UIInterfaceOrientationMaskPortrait;

	if (_landscapeLockWhenCardClosed)
		return UIInterfaceOrientationMaskLandscapeLeft | UIInterfaceOrientationMaskLandscapeRight;

	if (OriginalRootVCSupportedOrientations) return OriginalRootVCSupportedOrientations(self, _cmd);
	return UIInterfaceOrientationMaskAll;
}

static NSUInteger StashDelegateSupportedOrientationsForWindow(id self, SEL _cmd, UIApplication* app, UIWindow* window)
{
	if (_landscapeLockWhenCardClosed || _forcePortraitActive)
	{
		// Ask the SDK whether its own (portrait card / browser) window is the active one. This is
		// the SDK's supported manual path (see StashNativeCard.h) and replaces the previous
		// hand-rolled window-level heuristic.
		UIInterfaceOrientationMask stash = [StashNativeCard supportedInterfaceOrientationsForWindow:window];
		if (stash != 0)
			return stash;
		// Host/game window. Portrait-only during force portrait so UE's didRotate can never pick
		// the physical (landscape) orientation while the scene is programmatically portrait.
		if (_forcePortraitActive)
			return UIInterfaceOrientationMaskPortrait;
		return UIInterfaceOrientationMaskLandscapeLeft | UIInterfaceOrientationMaskLandscapeRight;
	}

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

// UE drops rotation events while suspended (didRotate early-returns on GIsSuspended), so a scene
// rotation that happens around device lock/unlock leaves the backbuffer sized for the wrong
// orientation until the *next* rotation event. Re-sync once the app is active again; the small
// delay lets UIKit finish restoring the scene geometry first.
static void InstallDidBecomeActiveResync(void)
{
	static dispatch_once_t once;
	dispatch_once(&once, ^{
		[[NSNotificationCenter defaultCenter] addObserverForName:UIApplicationDidBecomeActiveNotification
			object:nil queue:[NSOperationQueue mainQueue] usingBlock:^(NSNotification* note) {
			if (_forcePortraitActive || _landscapeLockWhenCardClosed)
			{
				dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.3 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
					StashRefreshSupportedInterfaceOrientations();
				});
			}
		}];
	});
}

static void InstallLandscapeLockHooks(void)
{
	InstallAppDelegateOrientationHook();
	TryInstallRootVCOrientationHook();
	InstallDidBecomeActiveResync();
}

#pragma mark - Force Portrait Watchdog

// Single owner of the invariant "no card on screen ⇒ no forced portrait": rather than enumerating
// every closure path (didDismiss, auto-close after payment success/failure, window.close(),
// resetPresentationState, host dismissCard, SDK-internal dismissals), poll the SDK's presentation
// state while force portrait is engaged. When the card is gone: clear force portrait, then keep
// watching until the scene is really landscape again and the illusion is fully disengaged —
// re-requesting the rotation (bounded retries) if UIKit ignored the geometry update, and
// force-restoring the pinned Metal view if the return transition never ran.
// The per-path clears (didDismiss/network-error/external-payment/processing-poll) stay as fast
// paths; everything here is idempotent with them.
static dispatch_source_t _forcePortraitWatchdog = nil;
static BOOL _watchdogSeenPresented = NO;
static NSTimeInterval _watchdogPresentDeadline = 0;
static NSTimeInterval _watchdogClearTime = 0;
static NSTimeInterval _watchdogLastRefresh = 0;
static int _watchdogRefreshRetries = 0;
static const NSTimeInterval kWatchdogInterval = 0.3;
static const NSTimeInterval kWatchdogPresentTimeout = 10.0; // matches purchase-processing poll
static const NSTimeInterval kWatchdogRefreshRetryInterval = 1.2;
static const int kWatchdogMaxRefreshRetries = 5;
static const NSTimeInterval kWatchdogPinnedGrace = 4.0; // transition + 1.5s illusion failsafe

static void StashStopForcePortraitWatchdog(void)
{
	if (_forcePortraitWatchdog)
	{
		dispatch_source_cancel(_forcePortraitWatchdog);
		_forcePortraitWatchdog = nil;
	}
}

static void StashForcePortraitWatchdogFired(void)
{
	const NSTimeInterval now = CFAbsoluteTimeGetCurrent();

	if ([StashNativeCard sharedInstance].isCurrentlyPresented)
	{
		_watchdogSeenPresented = YES;
		return;
	}

	// --- Card is not on screen ---

	if (_forcePortraitActive)
	{
		// Closed by any path that didn't clear, or never managed to present at all.
		if (_watchdogSeenPresented || now >= _watchdogPresentDeadline)
		{
			NSLog(@"[StashIllusion] watchdog: card gone with force portrait still active — clearing");
			StashClearForcePortrait();
		}
		return; // wait for presentation (grace period) or for the clear to take effect
	}

	// --- Force portrait cleared; verify the world actually rotated back ---

	if (_watchdogClearTime == 0) _watchdogClearTime = now;

	UIWindow* window = StashMainGameWindow();
	UIInterfaceOrientation orientation = window.windowScene
		? window.windowScene.interfaceOrientation : UIInterfaceOrientationUnknown;

	if (UIInterfaceOrientationIsLandscape(orientation))
	{
		if (!_landscapeIllusionActive && !_landscapeIllusionPinned)
		{
			StashStopForcePortraitWatchdog(); // fully restored — done
		}
		else if (now - _watchdogClearTime > kWatchdogPinnedGrace)
		{
			// Scene is landscape but the return transition (and its 1.5s failsafe) never
			// disengaged the illusion — force it.
			NSLog(@"[StashIllusion] watchdog: scene landscape but illusion still pinned — force restore");
			StashIllusionRestore(window.rootViewController.view.bounds.size);
			_landscapeIllusionActive = NO;
			[[IOSAppDelegate GetDelegate] didRotate:nil];
			StashStopForcePortraitWatchdog();
		}
		return;
	}

	// Scene still portrait after the clear: UIKit ignored/dropped the geometry request. Re-ask.
	if (now - MAX(_watchdogClearTime, _watchdogLastRefresh) > kWatchdogRefreshRetryInterval)
	{
		if (_watchdogRefreshRetries < kWatchdogMaxRefreshRetries)
		{
			_watchdogRefreshRetries++;
			_watchdogLastRefresh = now;
			NSLog(@"[StashIllusion] watchdog: scene still portrait after clear — re-requesting landscape (retry %d)", _watchdogRefreshRetries);
			StashRefreshSupportedInterfaceOrientations();
		}
		else
		{
			NSLog(@"[StashIllusion] watchdog: giving up after %d rotation retries", kWatchdogMaxRefreshRetries);
			StashStopForcePortraitWatchdog();
		}
	}
}

static void StashStartForcePortraitWatchdog(void)
{
	StashStopForcePortraitWatchdog();
	_watchdogSeenPresented = NO;
	_watchdogClearTime = 0;
	_watchdogLastRefresh = 0;
	_watchdogRefreshRetries = 0;
	_watchdogPresentDeadline = CFAbsoluteTimeGetCurrent() + kWatchdogPresentTimeout;

	_forcePortraitWatchdog = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());
	dispatch_source_set_timer(
		_forcePortraitWatchdog,
		dispatch_time(DISPATCH_TIME_NOW, (int64_t)(kWatchdogInterval * NSEC_PER_SEC)),
		(int64_t)(kWatchdogInterval * NSEC_PER_SEC),
		(int64_t)(100 * NSEC_PER_MSEC));
	dispatch_source_set_event_handler(_forcePortraitWatchdog, ^{
		StashForcePortraitWatchdogFired();
	});
	dispatch_resume(_forcePortraitWatchdog);
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
			StashIllusionEngageIfApplicable(); // before the geometry request so the transition swizzle catches it
			_forcePortraitActive = YES;
			InstallLandscapeLockHooks();
			StashRefreshSupportedInterfaceOrientations();
			StashStartForcePortraitWatchdog(); // rotate back on ANY closure path
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
