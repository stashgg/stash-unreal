//
//  StashPayCard.m
//  StashPay
//
//  Native iOS SDK for Stash Pay checkout integration.
//  Ported from Unity plugin - removes Unity dependencies and uses native delegate pattern.
//

#import "StashPayCard.h"
#import <SafariServices/SafariServices.h>
#import <WebKit/WebKit.h>
#import <QuartzCore/QuartzCore.h>
#import <objc/runtime.h>

// Suppress warnings when compiling without ARC (e.g., in game engines like Unreal)
#if !__has_feature(objc_arc)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"
#pragma clang diagnostic ignored "-Wobjc-missing-super-calls"
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

#pragma mark - PopupSizeConfig Implementation

@implementation StashPayPopupSizeConfig

- (instancetype)init {
    self = [super init];
    if (self) {
        _portraitWidthMultiplier = 1.0285;
        _portraitHeightMultiplier = 1.485;
        _landscapeWidthMultiplier = 1.2275445;
        _landscapeHeightMultiplier = 1.1385;
    }
    return self;
}

- (instancetype)initWithPortraitWidth:(CGFloat)portraitWidth
                       portraitHeight:(CGFloat)portraitHeight
                       landscapeWidth:(CGFloat)landscapeWidth
                      landscapeHeight:(CGFloat)landscapeHeight {
    self = [super init];
    if (self) {
        _portraitWidthMultiplier = portraitWidth;
        _portraitHeightMultiplier = portraitHeight;
        _landscapeWidthMultiplier = landscapeWidth;
        _landscapeHeightMultiplier = landscapeHeight;
    }
    return self;
}

@end

#pragma mark - Private State

static BOOL _callbackWasCalled = NO;
static BOOL _isCardCurrentlyPresented = NO;
static BOOL _paymentSuccessHandled = NO;
static BOOL _paymentSuccessCallbackCalled = NO;

static CGFloat _cardHeightRatio = 0.6;
static CGFloat _cardVerticalPosition = 1.0;
static CGFloat _cardWidthRatio = 1.0;
static CGFloat _originalCardHeightRatio = 0.6;
static CGFloat _originalCardVerticalPosition = 1.0;
static CGFloat _originalCardWidthRatio = 1.0;

static BOOL _useCustomPopupSize = NO;
static CGFloat _customPortraitWidthMultiplier = 1.0285;
static CGFloat _customPortraitHeightMultiplier = 1.485;
static CGFloat _customLandscapeWidthMultiplier = 1.753635;
static CGFloat _customLandscapeHeightMultiplier = 1.1385;

static BOOL _forceSafariViewController = NO;
static BOOL _usePopupPresentation = NO;
static BOOL _isCardExpanded = NO;
static BOOL _showScrollbar = NO;

#define ENABLE_IPAD_SUPPORT 1

#pragma mark - Animation Constants

static const CGFloat kSpringDampingDefault = 0.85f;
static const CGFloat kSpringDampingTight = 0.9f;
static const CGFloat kAnimationDurationDefault = 0.4f;
static const CGFloat kAnimationDurationFast = 0.25f;
static const CGFloat kCornerRadiusDefault = 20.0f;
static const CGFloat kCornerRadiusExpanded = 24.0f;
static const CGFloat kDragTrayHeight = 44.0f;

#pragma mark - Helper Function Prototypes

BOOL isRunningOniPad(void);
CGSize calculateiPadCardSize(CGRect screenBounds);
UIColor* getSystemBackgroundColor(void);
void configureScrollViewForWebView(UIScrollView* scrollView);
UIRectCorner getCornersToRoundForPosition(CGFloat verticalPosition, BOOL isiPad);
void setWebViewBackgroundColor(WKWebView* webView, UIColor* color);
CAShapeLayer* createCornerRadiusMask(CGRect bounds, UIRectCorner corners, CGFloat radius);
NSString* appendThemeQueryParameter(NSString* url);

#pragma mark - DragTrayView Interface

@interface DragTrayView : UIView
@end

@implementation DragTrayView

- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
    UIView *handleView = [self viewWithTag:8889];
    if (handleView) {
        CGPoint pointInHandle = [self convertPoint:point toView:handleView];
        CGRect handleBounds = handleView.bounds;
        CGRect expandedBounds = CGRectInset(handleBounds, -15, -15);
        if (CGRectContainsPoint(expandedBounds, pointInHandle)) {
            return [super hitTest:point withEvent:event];
        }
    }
    return nil;
}

@end

#pragma mark - OrientationLockedViewController

@interface OrientationLockedViewController : UIViewController
@property (nonatomic, assign) CGRect customFrame;
@property (nonatomic, assign) BOOL enforcePortrait;
@property (nonatomic, assign) BOOL skipLayoutDuringInitialSetup;
- (void)updateCornerRadiusMask;
@end

@implementation OrientationLockedViewController

- (void)viewWillLayoutSubviews {
    [super viewWillLayoutSubviews];
    
    if (self.skipLayoutDuringInitialSetup) {
        return;
    }
    
    CGRect screenBounds = [UIScreen mainScreen].bounds;
    UIWindow *cardWindow = self.view.window;
    
    if (cardWindow && !CGRectEqualToRect(cardWindow.frame, screenBounds)) {
        cardWindow.frame = screenBounds;
    }
    
    UIView *overlayView = objc_getAssociatedObject(self, "overlayView");
    if (overlayView && !CGRectEqualToRect(overlayView.frame, screenBounds)) {
        overlayView.frame = screenBounds;
    }
    
    if (_usePopupPresentation) {
        BOOL isLandscape = UIInterfaceOrientationIsLandscape([[UIApplication sharedApplication] statusBarOrientation]);
        
        CGFloat smallerDimension = fmin(screenBounds.size.width, screenBounds.size.height);
        CGFloat percentage = isRunningOniPad() ? 0.5 : 0.75;
        CGFloat baseSize = fmax(
            isRunningOniPad() ? 400.0 : 300.0,
            fmin(isRunningOniPad() ? 500.0 : 500.0, smallerDimension * percentage)
        );
        
        CGFloat portraitWidthMultiplier = _useCustomPopupSize ? _customPortraitWidthMultiplier : 1.0285;
        CGFloat portraitHeightMultiplier = _useCustomPopupSize ? _customPortraitHeightMultiplier : 1.485;
        CGFloat landscapeWidthMultiplier = _useCustomPopupSize ? _customLandscapeWidthMultiplier : 1.2275445;
        CGFloat landscapeHeightMultiplier = _useCustomPopupSize ? _customLandscapeHeightMultiplier : 1.1385;
        
        CGFloat popupWidth = baseSize * (isLandscape ? landscapeWidthMultiplier : portraitWidthMultiplier);
        CGFloat popupHeight = baseSize * (isLandscape ? landscapeHeightMultiplier : portraitHeightMultiplier);
        
        CGRect newFrame = CGRectMake(
            (screenBounds.size.width - popupWidth) / 2,
            (screenBounds.size.height - popupHeight) / 2,
            popupWidth,
            popupHeight
        );
        
        if (!CGRectEqualToRect(self.view.frame, newFrame)) {
            CGFloat frameDifference = fabs(self.view.frame.origin.x - newFrame.origin.x) + 
                                     fabs(self.view.frame.origin.y - newFrame.origin.y) +
                                     fabs(self.view.frame.size.width - newFrame.size.width) +
                                     fabs(self.view.frame.size.height - newFrame.size.height);
            
            if (frameDifference > 50.0) {
                [UIView animateWithDuration:0.3 animations:^{
                    self.view.frame = newFrame;
                    self.customFrame = newFrame;
                } completion:^(BOOL finished) {
                    [self updateCornerRadiusMask];
                }];
            } else {
                [CATransaction begin];
                [CATransaction setDisableActions:YES];
                self.view.frame = newFrame;
                self.customFrame = newFrame;
                [CATransaction commit];
                [self updateCornerRadiusMask];
            }
        } else {
            self.customFrame = newFrame;
            [self updateCornerRadiusMask];
        }
    } else {
        if (self.skipLayoutDuringInitialSetup) {
            return;
        }
        
        if (!isRunningOniPad() && _isCardExpanded) {
            return;
        }
        
        CGFloat width, height, x, y;
        
        if (isRunningOniPad()) {
            CGSize cardSize = calculateiPadCardSize(screenBounds);
            if (_isCardExpanded) {
                width = cardSize.width;
                height = cardSize.height;
            } else {
                width = cardSize.width * 0.7;
                height = cardSize.height * 0.7;
            }
            x = (screenBounds.size.width - width) / 2;
            y = (screenBounds.size.height - height) / 2;
        } else {
            if (screenBounds.size.width > screenBounds.size.height) {
                CGFloat temp = screenBounds.size.width;
                screenBounds.size.width = screenBounds.size.height;
                screenBounds.size.height = temp;
            }
            width = screenBounds.size.width * _cardWidthRatio;
            height = screenBounds.size.height * _cardHeightRatio;
            x = (screenBounds.size.width - width) / 2;
            y = screenBounds.size.height * _cardVerticalPosition - height;
            if (y < 0) y = 0;
        }
        
        CGRect newFrame = CGRectMake(x, y, width, height);
        
        if (!CGRectEqualToRect(self.view.frame, newFrame)) {
            CGFloat frameDifference = fabs(self.view.frame.origin.x - newFrame.origin.x) + 
                                     fabs(self.view.frame.origin.y - newFrame.origin.y) +
                                     fabs(self.view.frame.size.width - newFrame.size.width) +
                                     fabs(self.view.frame.size.height - newFrame.size.height);
            
            if (frameDifference > 50.0) {
                [UIView animateWithDuration:0.3 animations:^{
                    self.view.frame = newFrame;
                    self.customFrame = newFrame;
                    
                    UIView *dragTray = [self.view viewWithTag:8888];
                    if (dragTray) {
                        dragTray.frame = CGRectMake(0, 0, newFrame.size.width, kDragTrayHeight);
                        UIView *handle = [dragTray viewWithTag:8889];
                        if (handle) {
                            CGFloat handleX = (newFrame.size.width / 2.0) - 18.0;
                            handle.frame = CGRectMake(handleX, 8, 36, 5);
                        }
                    }
                } completion:^(BOOL finished) {
                    [self updateCornerRadiusMask];
                }];
            } else {
                [CATransaction begin];
                [CATransaction setDisableActions:YES];
                self.view.frame = newFrame;
                self.customFrame = newFrame;
                
                UIView *dragTray = [self.view viewWithTag:8888];
                if (dragTray) {
                    dragTray.frame = CGRectMake(0, 0, newFrame.size.width, kDragTrayHeight);
                    UIView *handle = [dragTray viewWithTag:8889];
                    if (handle) {
                        CGFloat handleX = (newFrame.size.width / 2.0) - 20.0;
                        handle.frame = CGRectMake(handleX, 8, 36, 5);
                    }
                }
                
                [CATransaction commit];
                [self updateCornerRadiusMask];
            }
        } else {
            self.customFrame = newFrame;
            [self updateCornerRadiusMask];
        }
    }
}

- (void)updateCornerRadiusMask {
    CAShapeLayer *maskLayer = (CAShapeLayer *)self.view.layer.mask;
    if (!maskLayer) {
        maskLayer = [[CAShapeLayer alloc] init];
        self.view.layer.mask = maskLayer;
    }
    
    CGRect viewBounds = self.view.bounds;
    UIRectCorner cornersToRound;
    
    if (isRunningOniPad() || _usePopupPresentation) {
        cornersToRound = UIRectCornerAllCorners;
    } else {
        cornersToRound = getCornersToRoundForPosition(_cardVerticalPosition, NO);
    }
    
    CAShapeLayer *newMaskLayer = createCornerRadiusMask(viewBounds, cornersToRound, kCornerRadiusDefault);
    maskLayer.frame = viewBounds;
    maskLayer.path = newMaskLayer.path;
}

- (UIInterfaceOrientationMask)supportedInterfaceOrientations {
    if (self.enforcePortrait && !isRunningOniPad()) {
        return UIInterfaceOrientationMaskPortrait;
    }
    
    if (isRunningOniPad()) {
        return UIInterfaceOrientationMaskAll;
    }
    
    if (_usePopupPresentation) {
        return UIInterfaceOrientationMaskAll;
    }
    
    UIInterfaceOrientation currentOrientation = [[UIApplication sharedApplication] statusBarOrientation];
    return (1 << currentOrientation);
}

- (BOOL)shouldAutorotate {
    if (isRunningOniPad()) {
        return YES;
    }
    return _usePopupPresentation;
}

- (UIInterfaceOrientation)preferredInterfaceOrientationForPresentation {
    if (self.enforcePortrait && !isRunningOniPad()) {
        return UIInterfaceOrientationPortrait;
    }
    return [[UIApplication sharedApplication] statusBarOrientation];
}

@end

#pragma mark - WebViewLoadDelegate

@interface WebViewLoadDelegate : NSObject <WKNavigationDelegate>
@property (nonatomic, weak) WKWebView *webView;
@property (nonatomic, assign) CFAbsoluteTime pageLoadStartTime;
- (instancetype)initWithWebView:(WKWebView*)webView loadingView:(UIView*)loadingView;
@end

@implementation WebViewLoadDelegate {
#if __has_feature(objc_arc)
    __weak WKWebView* _webView;
#else
    __unsafe_unretained WKWebView* _webView;
#endif
    UIView* _loadingView;
    NSTimer* _timeoutTimer;
}

- (instancetype)initWithWebView:(WKWebView*)webView loadingView:(UIView*)loadingView {
    self = [super init];
    if (self) {
        _webView = webView;
        self.webView = webView;
        _loadingView = loadingView;
        
        _timeoutTimer = [NSTimer scheduledTimerWithTimeInterval:0.1
                                                        target:self 
                                                      selector:@selector(handleTimeout:) 
                                                      userInfo:nil 
                                                       repeats:NO];
    }
    return self;
}

- (void)webView:(WKWebView *)webView decidePolicyForNavigationAction:(WKNavigationAction *)navigationAction decisionHandler:(void (^)(WKNavigationActionPolicy))decisionHandler {
    
    NSURL *url = navigationAction.request.URL;
    NSString *urlString = url.absoluteString;
    
    if ([url.scheme isEqualToString:@"tel"] ||
        [url.scheme isEqualToString:@"mailto"] ||
        [url.scheme isEqualToString:@"sms"]) {
        decisionHandler(WKNavigationActionPolicyCancel);
        [[UIApplication sharedApplication] openURL:url options:@{} completionHandler:nil];
        return;
    }
    
    if ([urlString containsString:@"apps.apple.com"] ||
        [urlString containsString:@"itunes.apple.com"]) {
        decisionHandler(WKNavigationActionPolicyCancel);
        [[UIApplication sharedApplication] openURL:url options:@{} completionHandler:nil];
        return;
    }
    
    decisionHandler(WKNavigationActionPolicyAllow);
}

- (void)handleTimeout:(NSTimer*)timer {
    [self showWebViewAndRemoveLoading];
}

- (void)showWebViewAndRemoveLoading {
    if (_timeoutTimer) {
        [_timeoutTimer invalidate];
        _timeoutTimer = nil;
    }
    
    if (_webView.alpha < 0.01) {
        UIColor *backgroundColor = getSystemBackgroundColor();
        _webView.backgroundColor = backgroundColor;
        _webView.scrollView.backgroundColor = backgroundColor;
        _webView.scrollView.opaque = YES;
        _webView.opaque = YES;
        
        if (@available(iOS 13.0, *)) {
            UIUserInterfaceStyle currentStyle = [UITraitCollection currentTraitCollection].userInterfaceStyle;
            if (currentStyle == UIUserInterfaceStyleDark) {
                NSString *forceColor = @"document.documentElement.style.backgroundColor = 'black'; \
                                      document.body.style.backgroundColor = 'black'; \
                                      var style = document.createElement('style'); \
                                      style.innerHTML = 'body, html { background-color: black !important; }'; \
                                      document.head.appendChild(style);";
                [_webView evaluateJavaScript:forceColor completionHandler:nil];
            }
        }
        
        [UIView animateWithDuration:0.2 delay:0 options:UIViewAnimationOptionCurveEaseInOut animations:^{
            self->_loadingView.alpha = 0.0;
            self->_webView.alpha = 1.0;
        } completion:^(BOOL finished) {
            [self->_loadingView removeFromSuperview];
        }];
    }
}

- (void)webView:(WKWebView *)webView didFinishNavigation:(WKNavigation *)navigation {
    if (self.pageLoadStartTime > 0) {
        CFAbsoluteTime loadEndTime = CFAbsoluteTimeGetCurrent();
        double loadTimeSeconds = loadEndTime - self.pageLoadStartTime;
        double loadTimeMs = loadTimeSeconds * 1000.0;
        
        id<StashPayCardDelegate> delegate = [StashPayCard sharedInstance].delegate;
        if (delegate && [delegate respondsToSelector:@selector(stashPayCardDidLoadPage:)]) {
            dispatch_async(dispatch_get_main_queue(), ^{
                [delegate stashPayCardDidLoadPage:loadTimeMs];
            });
        }
        
        self.pageLoadStartTime = 0;
    }
    
#if __has_feature(objc_arc)
    __weak WebViewLoadDelegate *weakSelf = self;
#else
    __unsafe_unretained WebViewLoadDelegate *weakSelf = self;
#endif
    __block void (^checkPageReady)(void);
    checkPageReady = ^{
        NSString *readyCheck = @"(function() { \
            if (document.readyState !== 'complete') return false; \
            if (document.documentElement.style.display === 'none') return false; \
            if (document.body === null) return false; \
            if (window.getComputedStyle(document.body).display === 'none') return false; \
            return true; \
        })()";
        
        [webView evaluateJavaScript:readyCheck completionHandler:^(id result, NSError *error) {
            if ([result boolValue]) {
                WebViewLoadDelegate *strongSelf = weakSelf;
                if (strongSelf) {
                    if (@available(iOS 13.0, *)) {
                        UIUserInterfaceStyle currentStyle = [UITraitCollection currentTraitCollection].userInterfaceStyle;
                        if (currentStyle == UIUserInterfaceStyleDark) {
                            NSString *forceColor = @"document.documentElement.style.backgroundColor = 'black'; \
                                                  document.body.style.backgroundColor = 'black'; \
                                                  var style = document.createElement('style'); \
                                                  style.innerHTML = 'body, html { background-color: black !important; }'; \
                                                  document.head.appendChild(style);";
                            [webView evaluateJavaScript:forceColor completionHandler:^(id result, NSError *error) {
                                [strongSelf showWebViewAndRemoveLoading];
                            }];
                        } else {
                            [strongSelf showWebViewAndRemoveLoading];
                        }
                    } else {
                        [strongSelf showWebViewAndRemoveLoading];
                    }
                }
            } else {
#if __has_feature(objc_arc)
                __weak void (^weakCheckPageReady)(void) = checkPageReady;
#else
                __unsafe_unretained void (^weakCheckPageReady)(void) = checkPageReady;
#endif
                dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.1 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
                    if (weakCheckPageReady) {
                        weakCheckPageReady();
                    }
                });
            }
        }];
    };
    
    checkPageReady();
}

- (void)webView:(WKWebView *)webView didCommitNavigation:(WKNavigation *)navigation {
    if (!_usePopupPresentation && !isRunningOniPad()) {
        [self showWebViewAndRemoveLoading];
    }
}

- (void)webView:(WKWebView *)webView didFailNavigation:(WKNavigation *)navigation withError:(NSError *)error {
    if (error.code == NSURLErrorCancelled) {
        return;
    }
    [[StashPayCard sharedInstance] dismiss];
}

- (void)webView:(WKWebView *)webView didFailProvisionalNavigation:(WKNavigation *)navigation withError:(NSError *)error {
    if (error.code == NSURLErrorCancelled) {
        return;
    }
    [[StashPayCard sharedInstance] dismiss];
}

- (void)dealloc {
    if (_timeoutTimer) {
        [_timeoutTimer invalidate];
        _timeoutTimer = nil;
    }
}

@end

#pragma mark - WebViewUIDelegate

@interface WebViewUIDelegate : NSObject <WKUIDelegate>
@end

@implementation WebViewUIDelegate

- (void)webView:(WKWebView *)webView contextMenuConfigurationForElement:(WKContextMenuElementInfo *)elementInfo completionHandler:(void (^)(UIContextMenuConfiguration *))completionHandler API_AVAILABLE(ios(13.0)) {
    completionHandler(nil);
}

- (void)webView:(WKWebView *)webView runJavaScriptAlertPanelWithMessage:(NSString *)message initiatedByFrame:(WKFrameInfo *)frame completionHandler:(void (^)(void))completionHandler {
    UIAlertController *alert = [UIAlertController alertControllerWithTitle:nil
                                                                   message:message
                                                            preferredStyle:UIAlertControllerStyleAlert];
    UIAlertAction *okAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
        completionHandler();
    }];
    [alert addAction:okAction];
    
    UIViewController *presentingVC = [UIApplication sharedApplication].keyWindow.rootViewController;
    while (presentingVC.presentedViewController) {
        presentingVC = presentingVC.presentedViewController;
    }
    [presentingVC presentViewController:alert animated:YES completion:nil];
}

- (void)webView:(WKWebView *)webView runJavaScriptConfirmPanelWithMessage:(NSString *)message initiatedByFrame:(WKFrameInfo *)frame completionHandler:(void (^)(BOOL))completionHandler {
    UIAlertController *alert = [UIAlertController alertControllerWithTitle:nil
                                                                   message:message
                                                            preferredStyle:UIAlertControllerStyleAlert];
    UIAlertAction *cancelAction = [UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel handler:^(UIAlertAction *action) {
        completionHandler(NO);
    }];
    UIAlertAction *okAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
        completionHandler(YES);
    }];
    [alert addAction:cancelAction];
    [alert addAction:okAction];
    
    UIViewController *presentingVC = [UIApplication sharedApplication].keyWindow.rootViewController;
    while (presentingVC.presentedViewController) {
        presentingVC = presentingVC.presentedViewController;
    }
    [presentingVC presentViewController:alert animated:YES completion:nil];
}

@end

#pragma mark - StashPayCardInternal

@interface StashPayCardInternal : NSObject <SFSafariViewControllerDelegate, UIGestureRecognizerDelegate, WKScriptMessageHandler>

@property (nonatomic, strong) UIViewController *currentPresentedVC;
@property (nonatomic, strong) UIWindow *portraitWindow;
@property (nonatomic, strong) UIWindow *previousKeyWindow;
@property (nonatomic, strong) UIView *dragTrayView;
@property (nonatomic, assign) CGFloat initialY;
@property (nonatomic, assign) BOOL isObservingKeyboard;
@property (nonatomic, assign) BOOL isPurchaseProcessing;
@property (nonatomic, strong) SFSafariViewController *currentSafariViewController;

+ (instancetype)sharedInstance;
- (void)dismissWithAnimation:(void (^)(void))completion;
- (void)cleanupCardInstance;
- (void)callDelegateCallbackOnce;
- (UIView *)createDragTray:(CGFloat)cardWidth;
- (void)expandCardToFullScreen;
- (void)collapseCardToOriginal;
- (void)updateCardExpansionProgress:(CGFloat)progress cardView:(UIView *)cardView;
- (void)startKeyboardObserving;
- (void)stopKeyboardObserving;

@end

@implementation StashPayCardInternal

+ (instancetype)sharedInstance {
    static StashPayCardInternal *sharedInstance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        sharedInstance = [[StashPayCardInternal alloc] init];
    });
    return sharedInstance;
}

#pragma mark - UIGestureRecognizerDelegate

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer {
    if ([gestureRecognizer.view isEqual:self.dragTrayView] || [otherGestureRecognizer.view isEqual:self.dragTrayView]) {
        return NO;
    }
    return YES;
}

- (BOOL)gestureRecognizerShouldBegin:(UIGestureRecognizer *)gestureRecognizer {
    if (isRunningOniPad() && [gestureRecognizer isKindOfClass:[UIPanGestureRecognizer class]]) {
        UIPanGestureRecognizer *panGesture = (UIPanGestureRecognizer *)gestureRecognizer;
        if ([panGesture.view isEqual:self.dragTrayView]) {
            UIView *referenceView = self.portraitWindow ? self.portraitWindow : panGesture.view.superview;
            CGPoint translation = [panGesture translationInView:referenceView];
            CGPoint velocity = [panGesture velocityInView:referenceView];
            // iPad: block upward drags
            if (translation.y < 0 || velocity.y < 0) {
                return NO;
            }
        }
    }
    return YES;
}

- (void)callDelegateCallbackOnce {
    if (!_callbackWasCalled) {
        _callbackWasCalled = YES;
        _isCardCurrentlyPresented = NO;
        
        id<StashPayCardDelegate> delegate = [StashPayCard sharedInstance].delegate;
        if (delegate && [delegate respondsToSelector:@selector(stashPayCardDidDismiss)]) {
            dispatch_async(dispatch_get_main_queue(), ^{
                [delegate stashPayCardDidDismiss];
            });
        }
    }
}

- (void)cleanupCardInstance {
    [self stopKeyboardObserving];
    
    if (self.dragTrayView) {
        [self.dragTrayView removeFromSuperview];
        self.dragTrayView = nil;
    }

    if (self.currentPresentedVC) {
        objc_setAssociatedObject(self.currentPresentedVC, "webViewDelegate", nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(self.currentPresentedVC, "webViewUIDelegate", nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(self.currentPresentedVC, "overlayView", nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(self.currentPresentedVC, "loadingView", nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        
        for (UIView *subview in [self.currentPresentedVC.view.subviews copy]) {
            if ([subview isKindOfClass:[WKWebView class]]) {
                WKWebView *webView = (WKWebView *)subview;
                
                [webView stopLoading];
                webView.navigationDelegate = nil;
                webView.UIDelegate = nil;
                
                [webView.configuration.userContentController removeScriptMessageHandlerForName:@"stashPaymentSuccess"];
                [webView.configuration.userContentController removeScriptMessageHandlerForName:@"stashPaymentFailure"];
                [webView.configuration.userContentController removeScriptMessageHandlerForName:@"stashPurchaseProcessing"];
                [webView.configuration.userContentController removeScriptMessageHandlerForName:@"stashOptin"];
                [webView.configuration.userContentController removeScriptMessageHandlerForName:@"stashExpand"];
                [webView.configuration.userContentController removeScriptMessageHandlerForName:@"stashCollapse"];
                [webView.configuration.userContentController removeAllUserScripts];
                
                [webView loadHTMLString:@"" baseURL:nil];
                
                dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.05 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
                    [webView removeFromSuperview];
                });
                break;
            }
        }
        
        UIView *overlayView = objc_getAssociatedObject(self.currentPresentedVC, "overlayView");
        if (overlayView) {
            for (UIView *subview in [overlayView.subviews copy]) {
                [subview removeFromSuperview];
            }
            [overlayView removeFromSuperview];
        }
    }
    
    if (self.portraitWindow) {
        if (self.portraitWindow.rootViewController) {
            [self.portraitWindow.rootViewController dismissViewControllerAnimated:NO completion:nil];
        }
        
        self.portraitWindow.hidden = YES;
        self.portraitWindow.rootViewController = nil;
        
        if (self.previousKeyWindow) {
            [self.previousKeyWindow makeKeyAndVisible];
            self.previousKeyWindow = nil;
        }
        
        self.portraitWindow = nil;
    }
    
    self.currentPresentedVC = nil;
    self.isPurchaseProcessing = NO;
    _isCardExpanded = NO;
    _isCardCurrentlyPresented = NO;
    _usePopupPresentation = NO;
    _useCustomPopupSize = NO;
    _callbackWasCalled = NO;
    _paymentSuccessHandled = NO;
    _paymentSuccessCallbackCalled = NO;
}

- (void)dismissWithAnimation:(void (^)(void))completion {
    if (!self.currentPresentedVC) {
        if (completion) completion();
        return;
    }
    
    CGRect screenBounds = [UIScreen mainScreen].bounds;
    CGFloat dismissY = screenBounds.size.height;
    
    OrientationLockedViewController *containerVC = (OrientationLockedViewController *)self.currentPresentedVC;
    UIView *overlayView = objc_getAssociatedObject(containerVC, "overlayView");
    
    containerVC.skipLayoutDuringInitialSetup = YES;
    
    CGFloat animationDuration = _usePopupPresentation ? 0.18 : kAnimationDurationFast;
    
    [UIView animateWithDuration:animationDuration delay:0 options:UIViewAnimationOptionCurveEaseInOut animations:^{
        if (_usePopupPresentation) {
            containerVC.view.alpha = 0.0;
            containerVC.view.transform = CGAffineTransformMakeScale(0.9, 0.9);
        } else {
            CGRect frame = containerVC.view.frame;
            frame.origin.y = dismissY;
            containerVC.customFrame = frame;
            containerVC.view.frame = frame;
        }
        
        if (overlayView) {
            overlayView.backgroundColor = [UIColor colorWithWhite:0.0 alpha:0.0];
        }
    } completion:^(BOOL finished) {
        containerVC.skipLayoutDuringInitialSetup = NO;
        if (completion) completion();
    }];
}

- (void)safariViewControllerDidFinish:(SFSafariViewController *)controller {
    if (_forceSafariViewController) {
        self.currentSafariViewController = nil;
        id<StashPayCardDelegate> delegate = [StashPayCard sharedInstance].delegate;
        if (delegate && [delegate respondsToSelector:@selector(stashPayCardDidDismiss)]) {
            dispatch_async(dispatch_get_main_queue(), ^{
                [delegate stashPayCardDidDismiss];
            });
        }
    } else {
        [self cleanupCardInstance];
        [self callDelegateCallbackOnce];
    }
}

- (UIView *)createDragTray:(CGFloat)cardWidth {
    // Use custom DragTrayView that only intercepts touches in handle area
    DragTrayView *dragTrayView = [[DragTrayView alloc] init];
    dragTrayView.frame = CGRectMake(0, 0, cardWidth, kDragTrayHeight);
    dragTrayView.tag = 8888;
    
    // Add black gradient fade for visual separation
    CAGradientLayer *gradientLayer = [CAGradientLayer layer];
    gradientLayer.frame = dragTrayView.bounds;
    
    if (isRunningOniPad()) {
        gradientLayer.colors = @[
            (id)[UIColor colorWithWhite:0.0 alpha:0.25].CGColor,
            (id)[UIColor colorWithWhite:0.0 alpha:0.15].CGColor,
            (id)[UIColor colorWithWhite:0.0 alpha:0.0].CGColor
        ];
    } else {
        gradientLayer.colors = @[
            (id)[UIColor colorWithWhite:0.0 alpha:0.35].CGColor,
            (id)[UIColor colorWithWhite:0.0 alpha:0.20].CGColor,
            (id)[UIColor colorWithWhite:0.0 alpha:0.0].CGColor
        ];
    }
    gradientLayer.locations = @[@0.0, @0.5, @1.0];
    [dragTrayView.layer addSublayer:gradientLayer];
    
    dragTrayView.backgroundColor = [UIColor clearColor];
    
    UIView *handleView = [[UIView alloc] init];
    // Apple Pay style handle - light gray, thicker, more prominent
    handleView.backgroundColor = [UIColor colorWithWhite:0.8 alpha:1.0];
    handleView.layer.cornerRadius = 3.0; // Slightly larger radius for modern look
    handleView.tag = 8889; // Tag for easy access
    // Handle is always centered - Apple Pay style dimensions (36pt wide, 5pt tall)
    handleView.frame = CGRectMake(cardWidth/2 - 18, 8, 36, 5);
    handleView.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin; // Keep centered
    handleView.layer.shadowColor = [UIColor blackColor].CGColor;
    handleView.layer.shadowOffset = CGSizeMake(0, 1);
    handleView.layer.shadowOpacity = 0.15; // Subtle shadow like Apple Pay
    handleView.layer.shadowRadius = 2.0; // Subtle shadow
    [dragTrayView addSubview:handleView];
    
    // Add pan gesture recognizer to drag tray (it will only receive touches in handle area due to hitTest override)
    UIPanGestureRecognizer *dragTrayPanGesture = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(handleDragTrayPanGesture:)];
    dragTrayPanGesture.delegate = self;
    [dragTrayView addGestureRecognizer:dragTrayPanGesture];
    
    return dragTrayView;
}

- (void)expandCardToFullScreen {
    if (!self.currentPresentedVC) return;

    _isCardExpanded = YES;

    UIView *cardView = self.currentPresentedVC.view;
    CGRect screenBounds = [UIScreen mainScreen].bounds;

    UIEdgeInsets safeAreaInsets = UIEdgeInsetsZero;
    if (@available(iOS 11.0, *)) {
        UIView *parentView = cardView.superview;
        if (parentView && [parentView respondsToSelector:@selector(safeAreaInsets)]) {
            safeAreaInsets = parentView.safeAreaInsets;
        }
    }

    CGFloat safeTop = safeAreaInsets.top;
    CGRect fullScreenFrame = CGRectMake(0, safeTop, screenBounds.size.width, screenBounds.size.height - safeTop);
    (void)fullScreenFrame; // Suppress unused variable warning

    for (UIView *subview in cardView.subviews) {
        if ([subview isKindOfClass:[WKWebView class]]) {
            WKWebView *webView = (WKWebView *)subview;

            NSMutableArray *constraintsToRemove = [NSMutableArray array];
            for (NSLayoutConstraint *constraint in cardView.constraints) {
                if (constraint.firstItem == webView || constraint.secondItem == webView) {
                    [constraintsToRemove addObject:constraint];
                }
            }
            [NSLayoutConstraint deactivateConstraints:constraintsToRemove];
            webView.translatesAutoresizingMaskIntoConstraints = NO;

            [NSLayoutConstraint activateConstraints:@[
                [webView.leadingAnchor constraintEqualToAnchor:cardView.leadingAnchor],
                [webView.trailingAnchor constraintEqualToAnchor:cardView.trailingAnchor],
                [webView.topAnchor constraintEqualToAnchor:cardView.topAnchor],
                [webView.bottomAnchor constraintEqualToAnchor:cardView.bottomAnchor]
            ]];

            break;
        }
    }

    if ([self.currentPresentedVC isKindOfClass:[OrientationLockedViewController class]]) {
        OrientationLockedViewController *containerVC = (OrientationLockedViewController *)self.currentPresentedVC;
        containerVC.skipLayoutDuringInitialSetup = YES;
    }

    [UIView animateWithDuration:kAnimationDurationDefault
                          delay:0
         usingSpringWithDamping:kSpringDampingDefault
          initialSpringVelocity:0.5
                        options:UIViewAnimationOptionCurveEaseOut
                     animations:^{
        [self updateCardExpansionProgress:1.0 cardView:cardView];
        cardView.backgroundColor = getSystemBackgroundColor();
    } completion:^(BOOL finished) {
        CGFloat radius = isRunningOniPad() ? kCornerRadiusExpanded : kCornerRadiusDefault;
        CAShapeLayer *maskLayer = createCornerRadiusMask(cardView.bounds, UIRectCornerTopLeft | UIRectCornerTopRight, radius);
        cardView.layer.mask = maskLayer;
    }];
}

- (void)collapseCardToOriginal {
    if (!self.currentPresentedVC) return;

    _isCardExpanded = NO;

    UIView *cardView = self.currentPresentedVC.view;
    CGRect screenBounds = [UIScreen mainScreen].bounds;

    CGFloat width, height, x, finalY;

    if (isRunningOniPad()) {
        CGSize cardSize = calculateiPadCardSize(screenBounds);
        width = cardSize.width;
        height = cardSize.height;
        x = (screenBounds.size.width - width) / 2;
        finalY = (screenBounds.size.height - height) / 2;
    } else {
        width = screenBounds.size.width * _originalCardWidthRatio;
        height = screenBounds.size.height * _originalCardHeightRatio;
        x = (screenBounds.size.width - width) / 2;
        finalY = screenBounds.size.height * _originalCardVerticalPosition - height;
        if (finalY < 0) finalY = 0;
    }

    for (UIView *subview in cardView.subviews) {
        if ([subview isKindOfClass:[WKWebView class]]) {
            WKWebView *webView = (WKWebView *)subview;

            NSMutableArray *constraintsToRemove = [NSMutableArray array];
            for (NSLayoutConstraint *constraint in cardView.constraints) {
                if (constraint.firstItem == webView || constraint.secondItem == webView) {
                    [constraintsToRemove addObject:constraint];
                }
            }
            [NSLayoutConstraint deactivateConstraints:constraintsToRemove];
            webView.translatesAutoresizingMaskIntoConstraints = NO;

            [NSLayoutConstraint activateConstraints:@[
                [webView.leadingAnchor constraintEqualToAnchor:cardView.leadingAnchor],
                [webView.trailingAnchor constraintEqualToAnchor:cardView.trailingAnchor],
                [webView.topAnchor constraintEqualToAnchor:cardView.topAnchor],
                [webView.bottomAnchor constraintEqualToAnchor:cardView.bottomAnchor]
            ]];
            break;
        }
    }

    CGRect collapsedFrame = CGRectMake(x, finalY, width, height);

    if ([self.currentPresentedVC isKindOfClass:[OrientationLockedViewController class]]) {
        OrientationLockedViewController *containerVC = (OrientationLockedViewController *)self.currentPresentedVC;
        containerVC.skipLayoutDuringInitialSetup = YES;
    }

    [UIView animateWithDuration:kAnimationDurationDefault
                          delay:0
         usingSpringWithDamping:kSpringDampingDefault
          initialSpringVelocity:0.3
                        options:UIViewAnimationOptionCurveEaseOut
                     animations:^{
        cardView.frame = collapsedFrame;

        UIView *dragTray = [cardView viewWithTag:8888];
        if (dragTray) {
            dragTray.frame = CGRectMake(0, 0, width, kDragTrayHeight);
            UIView *handle = [dragTray viewWithTag:8889];
            if (handle) {
                CGFloat handleX = (width / 2.0) - 18.0;
                handle.frame = CGRectMake(handleX, 8, 36, 5);
            }
        }

        if ([self.currentPresentedVC isKindOfClass:[OrientationLockedViewController class]]) {
            OrientationLockedViewController *containerVC = (OrientationLockedViewController *)self.currentPresentedVC;
            containerVC.customFrame = collapsedFrame;
        }
    } completion:^(BOOL finished) {
        UIRectCorner corners = getCornersToRoundForPosition(_cardVerticalPosition, isRunningOniPad());
        CAShapeLayer *maskLayer = createCornerRadiusMask(cardView.bounds, corners, kCornerRadiusDefault);
        cardView.layer.mask = maskLayer;

        if ([self.currentPresentedVC isKindOfClass:[OrientationLockedViewController class]]) {
            OrientationLockedViewController *containerVC = (OrientationLockedViewController *)self.currentPresentedVC;
            containerVC.skipLayoutDuringInitialSetup = NO;
        }
    }];
}

- (void)updateCardExpansionProgress:(CGFloat)progress cardView:(UIView *)cardView {
    if (!cardView) return;

    progress = MAX(0.0, MIN(1.0, progress));

    CGRect screenBounds = [UIScreen mainScreen].bounds;
    UIEdgeInsets safeAreaInsets = UIEdgeInsetsZero;
    if (@available(iOS 11.0, *)) {
        UIView *parentView = cardView.superview;
        if (parentView && [parentView respondsToSelector:@selector(safeAreaInsets)]) {
            safeAreaInsets = parentView.safeAreaInsets;
        }
    }
    CGFloat safeTop = safeAreaInsets.top;

    CGFloat collapsedWidth, collapsedHeight, collapsedX, collapsedY;
    CGFloat expandedWidth, expandedHeight, expandedX, expandedY;

    if (isRunningOniPad()) {
        CGSize cardSize = calculateiPadCardSize(screenBounds);
        expandedWidth = cardSize.width;
        expandedHeight = cardSize.height;
        expandedX = (screenBounds.size.width - expandedWidth) / 2;
        expandedY = (screenBounds.size.height - expandedHeight) / 2;

        collapsedWidth = expandedWidth * 0.7;
        collapsedHeight = expandedHeight * 0.7;
        collapsedX = (screenBounds.size.width - collapsedWidth) / 2;
        collapsedY = (screenBounds.size.height - collapsedHeight) / 2;
    } else {
        collapsedWidth = screenBounds.size.width * _originalCardWidthRatio;
        collapsedHeight = screenBounds.size.height * _originalCardHeightRatio;
        collapsedX = (screenBounds.size.width - collapsedWidth) / 2;
        collapsedY = screenBounds.size.height * _originalCardVerticalPosition - collapsedHeight;
        if (collapsedY < 0) collapsedY = 0;

        expandedWidth = screenBounds.size.width;
        expandedHeight = screenBounds.size.height - safeTop;
        expandedX = 0;
        expandedY = safeTop;
    }

    CGFloat currentWidth = collapsedWidth + (expandedWidth - collapsedWidth) * progress;
    CGFloat currentHeight = collapsedHeight + (expandedHeight - collapsedHeight) * progress;
    CGFloat currentX = collapsedX + (expandedX - collapsedX) * progress;
    CGFloat currentY = collapsedY + (expandedY - collapsedY) * progress;

    cardView.frame = CGRectMake(currentX, currentY, currentWidth, currentHeight);

    for (UIView *subview in cardView.subviews) {
        if ([subview isKindOfClass:[WKWebView class]]) {
            WKWebView *webView = (WKWebView *)subview;
            if (!webView.translatesAutoresizingMaskIntoConstraints) {
                webView.translatesAutoresizingMaskIntoConstraints = YES;
            }
            webView.frame = cardView.bounds;
            break;
        }
    }

    if ([self.currentPresentedVC isKindOfClass:[OrientationLockedViewController class]]) {
        OrientationLockedViewController *containerVC = (OrientationLockedViewController *)self.currentPresentedVC;
        containerVC.customFrame = cardView.frame;
    }

    UIView *dragTray = [cardView viewWithTag:8888];
    if (dragTray) {
        dragTray.frame = CGRectMake(0, 0, currentWidth, kDragTrayHeight);

        CAGradientLayer *gradientLayer = (CAGradientLayer*)dragTray.layer.sublayers.firstObject;
        if (gradientLayer && [gradientLayer isKindOfClass:[CAGradientLayer class]]) {
            gradientLayer.frame = dragTray.bounds;
        }

        UIView *handle = [dragTray viewWithTag:8889];
        if (handle) {
            CGFloat handleX = (currentWidth / 2.0) - 18.0;
            handle.frame = CGRectMake(handleX, 8, 36, 5);
        }
    }

    if (isRunningOniPad()) {
        CAShapeLayer *maskLayer = createCornerRadiusMask(cardView.bounds, UIRectCornerAllCorners, kCornerRadiusDefault);
        cardView.layer.mask = maskLayer;
    } else {
        if (progress > 0.9) {
            CAShapeLayer *maskLayer = createCornerRadiusMask(cardView.bounds, UIRectCornerTopLeft | UIRectCornerTopRight, kCornerRadiusDefault);
            cardView.layer.mask = maskLayer;
        } else if (progress > 0.5) {
            UIRectCorner corners = getCornersToRoundForPosition(_cardVerticalPosition, NO);
            corners |= UIRectCornerTopLeft | UIRectCornerTopRight;
            CAShapeLayer *maskLayer = createCornerRadiusMask(cardView.bounds, corners, kCornerRadiusDefault);
            cardView.layer.mask = maskLayer;
        } else {
            UIRectCorner corners = getCornersToRoundForPosition(_cardVerticalPosition, NO);
            CAShapeLayer *maskLayer = createCornerRadiusMask(cardView.bounds, corners, kCornerRadiusDefault);
            cardView.layer.mask = maskLayer;
        }
    }
}

- (void)startKeyboardObserving {
    if (self.isObservingKeyboard) return;
    self.isObservingKeyboard = YES;
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(keyboardWillShow:)
                                                 name:UIKeyboardWillShowNotification
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(keyboardWillHide:)
                                                 name:UIKeyboardWillHideNotification
                                               object:nil];
}

- (void)stopKeyboardObserving {
    if (!self.isObservingKeyboard) return;
    self.isObservingKeyboard = NO;
    
    [[NSNotificationCenter defaultCenter] removeObserver:self name:UIKeyboardWillShowNotification object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:UIKeyboardWillHideNotification object:nil];
}

- (void)keyboardWillShow:(NSNotification *)notification {
    if (_usePopupPresentation || isRunningOniPad()) return;
    if (_isCardExpanded) return;
    
    if (!self.currentPresentedVC) return;
    
    if ([self.currentPresentedVC isKindOfClass:[OrientationLockedViewController class]]) {
        OrientationLockedViewController *containerVC = (OrientationLockedViewController *)self.currentPresentedVC;
        containerVC.skipLayoutDuringInitialSetup = YES;
    }
    
    [self expandCardToFullScreen];
}

- (void)keyboardWillHide:(NSNotification *)notification {
    // Keep expanded after keyboard hides - user can collapse manually
}

- (void)handleDragTrayPanGesture:(UIPanGestureRecognizer *)gesture {
    if (!self.currentPresentedVC) return;
    if (self.isPurchaseProcessing) return;
    
    UIView *cardView = self.currentPresentedVC.view;
    
    switch (gesture.state) {
        case UIGestureRecognizerStateBegan:
            [self handleDragGestureBegan:gesture cardView:cardView];
            break;
            
        case UIGestureRecognizerStateChanged:
            [self handleDragGestureChanged:gesture cardView:cardView];
            break;
            
        case UIGestureRecognizerStateEnded:
        case UIGestureRecognizerStateCancelled:
            [self handleDragGestureEnded:gesture cardView:cardView];
            break;
            
        default:
            break;
    }
}

#pragma mark - Gesture Handling Methods (Matching Unity)

- (void)handleDragGestureBegan:(UIPanGestureRecognizer *)gesture cardView:(UIView *)cardView {
    CGPoint translation = [gesture translationInView:self.portraitWindow ? self.portraitWindow : cardView.superview];
    
    if (isRunningOniPad() && translation.y < 0) {
        gesture.enabled = NO;
        gesture.enabled = YES;
        return;
    }
    
    self.initialY = cardView.frame.origin.y;
    
    if (!isRunningOniPad()) {
        objc_setAssociatedObject(self.currentPresentedVC, "initialCardHeight", @(cardView.frame.size.height), OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    
    if ([self.currentPresentedVC isKindOfClass:[OrientationLockedViewController class]]) {
        OrientationLockedViewController *containerVC = (OrientationLockedViewController *)self.currentPresentedVC;
        containerVC.skipLayoutDuringInitialSetup = YES;
    }
}
    
- (void)handleDragGestureChanged:(UIPanGestureRecognizer *)gesture cardView:(UIView *)cardView {
    CGPoint translation = [gesture translationInView:self.portraitWindow ? self.portraitWindow : cardView.superview];
    CGFloat currentTravel = translation.y;
    CGFloat screenHeight = self.portraitWindow ? self.portraitWindow.bounds.size.height : cardView.superview.bounds.size.height;
    CGFloat height = cardView.frame.size.height;
    
    if (isRunningOniPad()) {
        if (currentTravel <= 0) return;
        
        if (currentTravel > 0) {
            CGFloat newY = MIN(screenHeight, self.initialY + currentTravel);
            
            [CATransaction begin];
            [CATransaction setDisableActions:YES];
            cardView.frame = CGRectMake(cardView.frame.origin.x, newY, cardView.frame.size.width, height);
            [CATransaction commit];
            
            if ([self.currentPresentedVC isKindOfClass:[OrientationLockedViewController class]]) {
                OrientationLockedViewController *containerVC = (OrientationLockedViewController *)self.currentPresentedVC;
                containerVC.customFrame = cardView.frame;
            }
        }
    } else {
        CGRect screenBounds = [UIScreen mainScreen].bounds;
        UIEdgeInsets safeAreaInsets = UIEdgeInsetsZero;
        if (@available(iOS 11.0, *)) {
            UIView *parentView = cardView.superview;
            if (parentView && [parentView respondsToSelector:@selector(safeAreaInsets)]) {
                safeAreaInsets = parentView.safeAreaInsets;
            }
        }
        CGFloat safeTop = safeAreaInsets.top;
        
        CGFloat collapsedHeight = screenBounds.size.height * _originalCardHeightRatio;
        CGFloat expandedHeight = screenBounds.size.height - safeTop;
        CGFloat currentProgress = 0.0;
        
        if (currentTravel < 0) {
            if (_isCardExpanded) {
                currentProgress = 1.0;
            } else {
                CGFloat dragAmount = fabs(currentTravel);
                CGFloat heightRange = expandedHeight - collapsedHeight;
                currentProgress = MIN(1.0, dragAmount / heightRange);
            }
        } else if (currentTravel > 0) {
            if (_isCardExpanded) {
                CGFloat dragAmount = currentTravel;
                CGFloat heightRange = expandedHeight - collapsedHeight;
                currentProgress = MAX(0.0, 1.0 - (dragAmount / heightRange));
                
                if (currentProgress <= 0.0 && currentTravel > height * 0.1) {
                    CGFloat newY = MIN(screenHeight, screenBounds.size.height - collapsedHeight + (currentTravel - heightRange));
                    
                    [CATransaction begin];
                    [CATransaction setDisableActions:YES];
                    cardView.frame = CGRectMake(0, newY, screenBounds.size.width, collapsedHeight);
                    [CATransaction commit];
                    
                    if ([self.currentPresentedVC isKindOfClass:[OrientationLockedViewController class]]) {
                        OrientationLockedViewController *containerVC = (OrientationLockedViewController *)self.currentPresentedVC;
                        containerVC.customFrame = cardView.frame;
                    }
                    
                    [CATransaction begin];
                    [CATransaction setDisableActions:YES];
                    [self updateCardExpansionProgress:0.0 cardView:cardView];
                    [CATransaction commit];
                    
                    return;
                }
            } else {
                CGFloat newY = MIN(screenHeight, self.initialY + currentTravel);
                
                [CATransaction begin];
                [CATransaction setDisableActions:YES];
                cardView.frame = CGRectMake(cardView.frame.origin.x, newY, cardView.frame.size.width, cardView.frame.size.height);
                [CATransaction commit];
                
                if ([self.currentPresentedVC isKindOfClass:[OrientationLockedViewController class]]) {
                    OrientationLockedViewController *containerVC = (OrientationLockedViewController *)self.currentPresentedVC;
                    containerVC.customFrame = cardView.frame;
                }
                
                return;
            }
        }
        
        [CATransaction begin];
        [CATransaction setDisableActions:YES];
        [self updateCardExpansionProgress:currentProgress cardView:cardView];
        [CATransaction commit];
    }
}
    
- (void)handleDragGestureEnded:(UIPanGestureRecognizer *)gesture cardView:(UIView *)cardView {
    if (!isRunningOniPad()) {
        objc_setAssociatedObject(self.currentPresentedVC, "initialCardHeight", nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    
    UIView *referenceView = self.portraitWindow ? self.portraitWindow : cardView.superview;
    CGPoint translation = [gesture translationInView:referenceView];
    CGPoint velocity = [gesture velocityInView:referenceView];
    CGFloat currentTravel = translation.y;
    CGFloat height = cardView.frame.size.height;
    UIView *overlayView = self.portraitWindow ? objc_getAssociatedObject(self.currentPresentedVC, "overlayView") : nil;
    
    BOOL shouldExpand = NO;
    BOOL shouldCollapse = NO;
    BOOL shouldDismiss = NO;
    
    if (isRunningOniPad()) {
        if (currentTravel > 0) {
            CGFloat currentY = cardView.frame.origin.y;
            CGFloat screenHeight = self.portraitWindow ? self.portraitWindow.bounds.size.height : cardView.superview.bounds.size.height;
            CGFloat cardBottom = currentY + height;
            CGFloat distanceToBottom = screenHeight - cardBottom;
            CGFloat dismissVelocityThreshold = 1040; // 800 * 1.3
            if (distanceToBottom < 10.0 || (velocity.y > dismissVelocityThreshold && currentTravel > height * 0.325)) {
                shouldDismiss = YES;
            }
        }
    } else {
        CGFloat expandThreshold = height * 0.15;
        CGFloat collapseThreshold = height * 0.25;
        CGFloat dismissThreshold = height * 0.4;
        CGFloat expandVelocityThreshold = -300;
        CGFloat collapseVelocityThreshold = 300;
        CGFloat dismissVelocityThreshold = 500;
        
        if (currentTravel < -expandThreshold || velocity.y < expandVelocityThreshold) {
            if (!_isCardExpanded) shouldExpand = YES;
        } else if (currentTravel > 0) {
            if (_isCardExpanded) {
                if (currentTravel > dismissThreshold && velocity.y > dismissVelocityThreshold) {
                    shouldDismiss = YES;
                } else if (currentTravel > collapseThreshold || velocity.y > collapseVelocityThreshold) {
                    shouldCollapse = YES;
                }
            } else {
                if (currentTravel > dismissThreshold || velocity.y > dismissVelocityThreshold) {
                    shouldDismiss = YES;
                }
            }
        }
    }
    
    if (shouldExpand) {
        [UIView animateWithDuration:0.4 
                              delay:0 
             usingSpringWithDamping:kSpringDampingTight 
              initialSpringVelocity:fabs(velocity.y) / 1000.0 
                            options:UIViewAnimationOptionCurveEaseOut 
                         animations:^{
            [self updateCardExpansionProgress:1.0 cardView:cardView];
        } completion:^(BOOL finished) {
            _isCardExpanded = YES;
            [self expandCardToFullScreen];
        }];
    } else if (shouldCollapse) {
        CGFloat animationDuration = 0.38;
        CGFloat springVelocity = velocity.y / 1000.0;
        if (velocity.y > 600) {
            animationDuration = 0.3;
            springVelocity = velocity.y / 800.0;
        }
        
        [UIView animateWithDuration:animationDuration 
                              delay:0 
             usingSpringWithDamping:kSpringDampingTight 
              initialSpringVelocity:springVelocity 
                            options:UIViewAnimationOptionCurveEaseOut 
                         animations:^{
            [self updateCardExpansionProgress:0.0 cardView:cardView];
        } completion:^(BOOL finished) {
            _isCardExpanded = NO;
            [self collapseCardToOriginal];
        }];
    } else if (shouldDismiss) {
        if ([self.currentPresentedVC isKindOfClass:[OrientationLockedViewController class]]) {
            OrientationLockedViewController *containerVC = (OrientationLockedViewController *)self.currentPresentedVC;
            containerVC.skipLayoutDuringInitialSetup = YES;
        }
        
        CGFloat animationDuration = (velocity.y > 1000) ? 0.22 : 0.35;
        CGFloat finalY = self.portraitWindow ? self.portraitWindow.bounds.size.height : cardView.superview.bounds.size.height;
        
        [UIView animateWithDuration:animationDuration 
                              delay:0 
             usingSpringWithDamping:kSpringDampingTight 
              initialSpringVelocity:velocity.y / 1000.0 
                            options:UIViewAnimationOptionCurveEaseOut 
                         animations:^{
            cardView.frame = CGRectMake(cardView.frame.origin.x, finalY, cardView.frame.size.width, cardView.frame.size.height);
            
            if ([self.currentPresentedVC isKindOfClass:[OrientationLockedViewController class]]) {
                OrientationLockedViewController *containerVC = (OrientationLockedViewController *)self.currentPresentedVC;
                containerVC.customFrame = cardView.frame;
            }
            if (overlayView) {
                overlayView.backgroundColor = [UIColor colorWithWhite:0.0 alpha:0.0];
            }
        } completion:^(BOOL finished) {
            if (!self.currentPresentedVC) {
                [self cleanupCardInstance];
                [self callDelegateCallbackOnce];
                return;
            }
            
            if ([self.currentPresentedVC isKindOfClass:[OrientationLockedViewController class]]) {
                OrientationLockedViewController *containerVC = (OrientationLockedViewController *)self.currentPresentedVC;
                containerVC.skipLayoutDuringInitialSetup = NO;
            }
            
            UIViewController *vcToDismiss = self.currentPresentedVC;
            [vcToDismiss dismissViewControllerAnimated:NO completion:^{
                if (self.currentPresentedVC == vcToDismiss) {
                    [self cleanupCardInstance];
                    [self callDelegateCallbackOnce];
                }
            }];
        }];
    } else {
        // Snap back logic
        if (isRunningOniPad()) {
            CGRect screenBounds = [UIScreen mainScreen].bounds;
            CGFloat originalWidth, originalHeight, originalX, originalY;
            
            CGSize cardSize = calculateiPadCardSize(screenBounds);
            
            if (_isCardExpanded) {
                // Snap back to expanded size (default size)
                originalWidth = cardSize.width;
                originalHeight = cardSize.height;
                originalX = (screenBounds.size.width - originalWidth) / 2;
                originalY = (screenBounds.size.height - originalHeight) / 2;
            } else {
                // Snap back to collapsed size (30% smaller)
                originalWidth = cardSize.width * 0.7;
                originalHeight = cardSize.height * 0.7;
                originalX = (screenBounds.size.width - originalWidth) / 2;
                originalY = (screenBounds.size.height - originalHeight) / 2;
            }
            
            [UIView animateWithDuration:kAnimationDurationFast 
                                  delay:0 
                 usingSpringWithDamping:0.92 
                  initialSpringVelocity:fabs(velocity.y) / 1000.0 
                                options:UIViewAnimationOptionCurveEaseOut 
                             animations:^{
                cardView.frame = CGRectMake(originalX, originalY, originalWidth, originalHeight);
                
                for (UIView *subview in cardView.subviews) {
                    if ([subview isKindOfClass:[WKWebView class]]) {
                        subview.frame = cardView.bounds;
                        break;
                    }
                }
                
                UIView *dragTray = [cardView viewWithTag:8888];
                if (dragTray) {
                    dragTray.frame = CGRectMake(0, 0, originalWidth, kDragTrayHeight);
                    UIView *handle = [dragTray viewWithTag:8889];
                    if (handle) {
                        handle.frame = CGRectMake((originalWidth / 2.0) - 18.0, 8, 36, 5);
                    }
                }
                
                if ([self.currentPresentedVC isKindOfClass:[OrientationLockedViewController class]]) {
                    OrientationLockedViewController *containerVC = (OrientationLockedViewController *)self.currentPresentedVC;
                    containerVC.customFrame = cardView.frame;
                }
            } completion:^(BOOL finished) {
                if ([self.currentPresentedVC isKindOfClass:[OrientationLockedViewController class]]) {
                    OrientationLockedViewController *containerVC = (OrientationLockedViewController *)self.currentPresentedVC;
                    containerVC.skipLayoutDuringInitialSetup = NO;
                }
            }];
        } else {
            // iPhone snap back
            CGFloat targetProgress = _isCardExpanded ? 1.0 : 0.0;
            [UIView animateWithDuration:0.32 
                                  delay:0 
                 usingSpringWithDamping:0.92 
                  initialSpringVelocity:fabs(velocity.y) / 1000.0 
                                options:UIViewAnimationOptionCurveEaseOut 
                             animations:^{
                [self updateCardExpansionProgress:targetProgress cardView:cardView];
            } completion:^(BOOL finished) {
                if ([self.currentPresentedVC isKindOfClass:[OrientationLockedViewController class]]) {
                    OrientationLockedViewController *containerVC = (OrientationLockedViewController *)self.currentPresentedVC;
                    containerVC.skipLayoutDuringInitialSetup = NO;
                }
            }];
        }
    }
}

#pragma mark - WKScriptMessageHandler

- (void)userContentController:(WKUserContentController *)userContentController didReceiveScriptMessage:(WKScriptMessage *)message {
    NSString *name = message.name;
    id<StashPayCardDelegate> delegate = [StashPayCard sharedInstance].delegate;
    
    if ([name isEqualToString:@"stashPaymentSuccess"]) {
        if (_paymentSuccessHandled) return;
        _paymentSuccessHandled = YES;
        _paymentSuccessCallbackCalled = YES;
        self.isPurchaseProcessing = NO;
        
        if (delegate && [delegate respondsToSelector:@selector(stashPayCardDidCompletePayment)]) {
            dispatch_async(dispatch_get_main_queue(), ^{
                [delegate stashPayCardDidCompletePayment];
            });
        }
        
        [self dismissWithAnimation:^{
            [self cleanupCardInstance];
        }];
    } else if ([name isEqualToString:@"stashPaymentFailure"]) {
        if (_paymentSuccessHandled) return;
        _paymentSuccessHandled = YES;
        self.isPurchaseProcessing = NO;
        
        if (delegate && [delegate respondsToSelector:@selector(stashPayCardDidFailPayment)]) {
            dispatch_async(dispatch_get_main_queue(), ^{
                [delegate stashPayCardDidFailPayment];
            });
        }
        
        [self dismissWithAnimation:^{
            [self cleanupCardInstance];
        }];
    } else if ([name isEqualToString:@"stashPurchaseProcessing"]) {
        self.isPurchaseProcessing = YES;
    } else if ([name isEqualToString:@"stashOptin"]) {
        NSString *optinType = [message.body isKindOfClass:[NSString class]] ? message.body : @"";

        if (delegate && [delegate respondsToSelector:@selector(stashPayCardDidReceiveOptIn:)]) {
            dispatch_async(dispatch_get_main_queue(), ^{
                [delegate stashPayCardDidReceiveOptIn:optinType];
            });
        }

        [self dismissWithAnimation:^{
            [self cleanupCardInstance];
        }];
    } else if ([name isEqualToString:@"stashExpand"]) {
        if (!_usePopupPresentation && !_isCardExpanded && self.currentPresentedVC) {
            UIView *cardView = self.currentPresentedVC.view;

            OrientationLockedViewController *containerVC = nil;
            if ([self.currentPresentedVC isKindOfClass:[OrientationLockedViewController class]]) {
                containerVC = (OrientationLockedViewController *)self.currentPresentedVC;
                containerVC.skipLayoutDuringInitialSetup = YES;
            }

            if (isRunningOniPad()) {
                if (containerVC) {
                    containerVC.skipLayoutDuringInitialSetup = YES;
                }

                _isCardExpanded = YES;

                [UIView animateWithDuration:0.35
                                      delay:0
                     usingSpringWithDamping:0.88
                      initialSpringVelocity:0.3
                                    options:UIViewAnimationOptionCurveEaseOut
                                 animations:^{
                    [self updateCardExpansionProgress:1.0 cardView:cardView];
                } completion:^(BOOL finished) {
                    if (containerVC) {
                        containerVC.skipLayoutDuringInitialSetup = NO;
                    }
                }];
            } else {
                [UIView animateWithDuration:kAnimationDurationDefault
                                      delay:0
                     usingSpringWithDamping:kSpringDampingDefault
                      initialSpringVelocity:0.5
                                    options:UIViewAnimationOptionCurveEaseOut
                                 animations:^{
                    [self updateCardExpansionProgress:1.0 cardView:cardView];
                } completion:^(BOOL finished) {
                    [self expandCardToFullScreen];

                    if ([self.currentPresentedVC isKindOfClass:[OrientationLockedViewController class]]) {
                        OrientationLockedViewController *containerVC = (OrientationLockedViewController *)self.currentPresentedVC;
                        containerVC.skipLayoutDuringInitialSetup = NO;
                    }
                }];
            }
        }
    } else if ([name isEqualToString:@"stashCollapse"]) {
        if (!_usePopupPresentation && _isCardExpanded && self.currentPresentedVC) {
            UIView *cardView = self.currentPresentedVC.view;

            if ([self.currentPresentedVC isKindOfClass:[OrientationLockedViewController class]]) {
                OrientationLockedViewController *containerVC = (OrientationLockedViewController *)self.currentPresentedVC;
                containerVC.skipLayoutDuringInitialSetup = YES;
            }

            [UIView animateWithDuration:kAnimationDurationDefault
                                  delay:0
                 usingSpringWithDamping:kSpringDampingDefault
                  initialSpringVelocity:0.3
                                options:UIViewAnimationOptionCurveEaseOut
                             animations:^{
                [self updateCardExpansionProgress:0.0 cardView:cardView];
            } completion:^(BOOL finished) {
                _isCardExpanded = NO;
                [self collapseCardToOriginal];

                if ([self.currentPresentedVC isKindOfClass:[OrientationLockedViewController class]]) {
                    OrientationLockedViewController *containerVC = (OrientationLockedViewController *)self.currentPresentedVC;
                    containerVC.skipLayoutDuringInitialSetup = NO;
                }
            }];
        }
    }
}

@end

#pragma mark - Helper Functions

BOOL isRunningOniPad(void) {
#if !ENABLE_IPAD_SUPPORT
    return NO;
#endif
    
    if (![NSThread isMainThread]) {
        __block BOOL result = NO;
        dispatch_sync(dispatch_get_main_queue(), ^{
            result = isRunningOniPad();
        });
        return result;
    }
    
    return ([UIDevice currentDevice].userInterfaceIdiom == UIUserInterfaceIdiomPad);
}

CGSize calculateiPadCardSize(CGRect screenBounds) {
    if (screenBounds.size.width <= 0 || screenBounds.size.height <= 0) {
        return CGSizeMake(600, 700);
    }
    
    CGFloat landscapeWidth = fmax(screenBounds.size.width, screenBounds.size.height);
    CGFloat landscapeHeight = fmin(screenBounds.size.width, screenBounds.size.height);
    
    CGFloat targetAspectRatio = 0.75;
    
    CGFloat maxCardWidth = landscapeWidth * 0.8;
    CGFloat maxCardHeight = landscapeHeight * 0.75;
    
    if (maxCardWidth <= 0 || maxCardHeight <= 0) {
        return CGSizeMake(600, 700);
    }
    
    CGFloat cardWidth, cardHeight;
    
    if (maxCardWidth / targetAspectRatio <= maxCardHeight) {
        cardWidth = maxCardWidth;
        cardHeight = cardWidth / targetAspectRatio;
    } else {
        cardHeight = maxCardHeight;
        cardWidth = cardHeight * targetAspectRatio;
    }
    
    if (cardWidth < 400 || cardHeight < 500) {
        return CGSizeMake(600, 700);
    }
    
    return CGSizeMake(cardWidth, cardHeight);
}

UIColor* getSystemBackgroundColor(void) {
    if (@available(iOS 13.0, *)) {
        UIUserInterfaceStyle currentStyle = [UITraitCollection currentTraitCollection].userInterfaceStyle;
        return (currentStyle == UIUserInterfaceStyleDark) ? [UIColor blackColor] : [UIColor systemBackgroundColor];
    }
    return [UIColor whiteColor];
}

void configureScrollViewForWebView(UIScrollView* scrollView) {
    if (@available(iOS 11.0, *)) {
        scrollView.contentInsetAdjustmentBehavior = UIScrollViewContentInsetAdjustmentNever;
    }
    scrollView.contentInset = UIEdgeInsetsZero;
    scrollView.scrollIndicatorInsets = UIEdgeInsetsZero;
    scrollView.bounces = NO;
    scrollView.alwaysBounceVertical = NO;
    scrollView.alwaysBounceHorizontal = NO;
}

UIRectCorner getCornersToRoundForPosition(CGFloat verticalPosition, BOOL isiPad) {
    if (isiPad) {
        return UIRectCornerAllCorners;
    }
    if (verticalPosition < 0.1) {
        return UIRectCornerBottomLeft | UIRectCornerBottomRight;
    } else if (verticalPosition > 0.9) {
        return UIRectCornerTopLeft | UIRectCornerTopRight;
    }
    return UIRectCornerAllCorners;
}

void setWebViewBackgroundColor(WKWebView* webView, UIColor* color) {
    webView.backgroundColor = color;
    webView.scrollView.backgroundColor = color;
    for (UIView *subview in webView.subviews) {
        subview.backgroundColor = color;
        subview.opaque = YES;
    }
    for (UIView *subview in webView.scrollView.subviews) {
        subview.backgroundColor = color;
        subview.opaque = YES;
    }
}

CAShapeLayer* createCornerRadiusMask(CGRect bounds, UIRectCorner corners, CGFloat radius) {
    UIBezierPath *maskPath = [UIBezierPath bezierPathWithRoundedRect:bounds
                                                  byRoundingCorners:corners
                                                        cornerRadii:CGSizeMake(radius, radius)];
    CAShapeLayer *maskLayer = [[CAShapeLayer alloc] init];
    maskLayer.frame = bounds;
    maskLayer.path = maskPath.CGPath;
    return maskLayer;
}

NSString* appendThemeQueryParameter(NSString* url) {
    if (url == nil || url.length == 0) {
        return url;
    }
    
    NSString *theme = @"light";
    if (@available(iOS 13.0, *)) {
        UIUserInterfaceStyle currentStyle = [UITraitCollection currentTraitCollection].userInterfaceStyle;
        if (currentStyle == UIUserInterfaceStyleDark) {
            theme = @"dark";
        }
    }
    
    NSURLComponents *components = [NSURLComponents componentsWithString:url];
    if (components == nil) {
        NSString *separator = [url containsString:@"?"] ? @"&" : @"?";
        return [NSString stringWithFormat:@"%@%@theme=%@", url, separator, theme];
    }
    
    NSMutableArray *queryItems = [NSMutableArray arrayWithArray:components.queryItems ?: @[]];
    
    NSMutableArray *filteredItems = [NSMutableArray array];
    for (NSURLQueryItem *item in queryItems) {
        if (![item.name isEqualToString:@"theme"]) {
            [filteredItems addObject:item];
        }
    }
    
    [filteredItems addObject:[NSURLQueryItem queryItemWithName:@"theme" value:theme]];
    components.queryItems = filteredItems;
    
    return components.URL.absoluteString;
}

#pragma mark - StashPayCard Implementation

@interface StashPayCard ()
@property (nonatomic, assign) BOOL isCardExpanded;
@end

@implementation StashPayCard

+ (instancetype)sharedInstance {
    static StashPayCard *sharedInstance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        sharedInstance = [[StashPayCard alloc] init];
    });
    return sharedInstance;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        // Note: These values are for the instance but static vars are used for actual sizing
        // Default height ratio 0.6 matches Unity C# side
        _forceWebBasedCheckout = NO;
    }
    return self;
}

- (BOOL)isCurrentlyPresented {
    return _isCardCurrentlyPresented;
}

- (BOOL)isPurchaseProcessing {
    return [StashPayCardInternal sharedInstance].isPurchaseProcessing;
}

- (CGFloat)cardHeightRatio {
    return _cardHeightRatio;
}

- (void)setCardHeightRatio:(CGFloat)ratio {
    // Update both static and instance - static is used by OrientationLockedViewController
    CGFloat clampedRatio = ratio < 0.1 ? 0.1 : (ratio > 1.0 ? 1.0 : ratio);
    _cardHeightRatio = clampedRatio;
    _originalCardHeightRatio = clampedRatio;
}

- (CGFloat)cardVerticalPosition {
    return _cardVerticalPosition;
}

- (void)setCardVerticalPosition:(CGFloat)position {
    CGFloat clampedPosition = position < 0.0 ? 0.0 : (position > 1.0 ? 1.0 : position);
    _cardVerticalPosition = clampedPosition;
    _originalCardVerticalPosition = clampedPosition;
}

- (CGFloat)cardWidthRatio {
    return _cardWidthRatio;
}

- (void)setCardWidthRatio:(CGFloat)ratio {
    CGFloat clampedRatio = ratio < 0.1 ? 0.1 : (ratio > 1.0 ? 1.0 : ratio);
    _cardWidthRatio = clampedRatio;
    _originalCardWidthRatio = clampedRatio;
}

- (void)openCheckoutWithURL:(NSString *)url {
    if (url == nil || url.length == 0) {
        return;
    }
    
    _usePopupPresentation = NO;
    [self openURLInternal:url];
}

- (void)openPopupWithURL:(NSString *)url {
    [self openPopupWithURL:url sizeConfig:nil];
}

- (void)openPopupWithURL:(NSString *)url sizeConfig:(StashPayPopupSizeConfig *)sizeConfig {
    if (url == nil || url.length == 0) {
        return;
    }
    
    _usePopupPresentation = YES;
    
    if (sizeConfig) {
        _useCustomPopupSize = YES;
        _customPortraitWidthMultiplier = sizeConfig.portraitWidthMultiplier;
        _customPortraitHeightMultiplier = sizeConfig.portraitHeightMultiplier;
        _customLandscapeWidthMultiplier = sizeConfig.landscapeWidthMultiplier;
        _customLandscapeHeightMultiplier = sizeConfig.landscapeHeightMultiplier;
    } else {
        _useCustomPopupSize = NO;
    }
    
    [self openURLInternal:url];
}

- (void)openURLInternal:(NSString *)url {
    if (_isCardCurrentlyPresented) {
        return;
    }
    
    NSString *urlWithTheme = appendThemeQueryParameter(url);
    
    if (_forceWebBasedCheckout && !_usePopupPresentation) {
        [self openInSafariViewController:urlWithTheme];
        return;
    }
    
    [self openInCardUI:urlWithTheme];
}

- (void)openInSafariViewController:(NSString *)url {
    NSURL *nsurl = [NSURL URLWithString:url];
    if (!nsurl) return;
    
    SFSafariViewController *safariVC = [[SFSafariViewController alloc] initWithURL:nsurl];
    safariVC.delegate = [StashPayCardInternal sharedInstance];
    [StashPayCardInternal sharedInstance].currentSafariViewController = safariVC;
    
    UIViewController *rootVC = [UIApplication sharedApplication].keyWindow.rootViewController;
    while (rootVC.presentedViewController) {
        rootVC = rootVC.presentedViewController;
    }
    
    [rootVC presentViewController:safariVC animated:YES completion:^{
        _isCardCurrentlyPresented = YES;
    }];
}

- (void)openInCardUI:(NSString *)url {
    // Full implementation matching Unity plugin exactly
    _isCardCurrentlyPresented = YES;
    _callbackWasCalled = NO;
    _paymentSuccessHandled = NO;
    _paymentSuccessCallbackCalled = NO;
    _isCardExpanded = NO;
    
    // Store original configuration values
    _originalCardHeightRatio = _cardHeightRatio;
    _originalCardVerticalPosition = _cardVerticalPosition;
    _originalCardWidthRatio = _cardWidthRatio;
    
    StashPayCardInternal *internal = [StashPayCardInternal sharedInstance];
    
    CGRect screenBounds = [UIScreen mainScreen].bounds;
    
    // For portrait-locked OpenCheckout on iPhone, ensure we use portrait dimensions
    if (!_usePopupPresentation && !isRunningOniPad()) {
        if (screenBounds.size.width > screenBounds.size.height) {
            CGFloat temp = screenBounds.size.width;
            screenBounds.size.width = screenBounds.size.height;
            screenBounds.size.height = temp;
        }
    }
    
    CGFloat width, height, x, finalY;
    
    if (_usePopupPresentation) {
        // Popup mode: calculate size using same method as viewWillLayoutSubviews
        BOOL isLandscape = UIInterfaceOrientationIsLandscape([[UIApplication sharedApplication] statusBarOrientation]);
        
        CGFloat smallerDimension = fmin(screenBounds.size.width, screenBounds.size.height);
        CGFloat percentage = isRunningOniPad() ? 0.5 : 0.75;
        CGFloat baseSize = fmax(
            isRunningOniPad() ? 400.0 : 300.0,
            fmin(isRunningOniPad() ? 500.0 : 500.0, smallerDimension * percentage)
        );
        
        CGFloat portraitWidthMultiplier = _useCustomPopupSize ? _customPortraitWidthMultiplier : 1.0285;
        CGFloat portraitHeightMultiplier = _useCustomPopupSize ? _customPortraitHeightMultiplier : 1.485;
        CGFloat landscapeWidthMultiplier = _useCustomPopupSize ? _customLandscapeWidthMultiplier : 1.2275445;
        CGFloat landscapeHeightMultiplier = _useCustomPopupSize ? _customLandscapeHeightMultiplier : 1.1385;
        
        width = baseSize * (isLandscape ? landscapeWidthMultiplier : portraitWidthMultiplier);
        height = baseSize * (isLandscape ? landscapeHeightMultiplier : portraitHeightMultiplier);
        x = (screenBounds.size.width - width) / 2;
        finalY = (screenBounds.size.height - height) / 2;
    } else {
        // Card mode: split implementation for iPhone and iPad
        if (isRunningOniPad()) {
            CGSize cardSize = calculateiPadCardSize(screenBounds);
            width = cardSize.width;
            height = cardSize.height;
            x = (screenBounds.size.width - width) / 2;
            finalY = (screenBounds.size.height - height) / 2;
        } else {
            // iPhone: forced portrait, slides up from bottom
            if (screenBounds.size.width > screenBounds.size.height) {
                CGFloat temp = screenBounds.size.width;
                screenBounds.size.width = screenBounds.size.height;
                screenBounds.size.height = temp;
            }
            width = screenBounds.size.width * _cardWidthRatio;
            height = screenBounds.size.height * _cardHeightRatio;
            x = (screenBounds.size.width - width) / 2;
            finalY = screenBounds.size.height * _cardVerticalPosition - height;
            if (finalY < 0) finalY = 0;
        }
    }
    
    // Create container view controller
    OrientationLockedViewController *containerVC = [[OrientationLockedViewController alloc] init];
    containerVC.modalPresentationStyle = UIModalPresentationOverFullScreen;
    containerVC.enforcePortrait = !_usePopupPresentation;
    containerVC.view.backgroundColor = getSystemBackgroundColor();
    
    // Create WebView configuration
    WKWebViewConfiguration *config = [[WKWebViewConfiguration alloc] init];
    config.allowsInlineMediaPlayback = YES;
    config.allowsAirPlayForMediaPlayback = YES;
    config.allowsPictureInPictureMediaPlayback = YES;
    
    if (@available(iOS 14.0, *)) {
        config.limitsNavigationsToAppBoundDomains = NO;
    }
    if (@available(iOS 11.0, *)) {
        config.websiteDataStore = [WKWebsiteDataStore defaultDataStore];
        config.dataDetectorTypes = WKDataDetectorTypeAll;
    }
    
    WKPreferences *preferences = [[WKPreferences alloc] init];
    preferences.javaScriptEnabled = YES;
    preferences.javaScriptCanOpenWindowsAutomatically = YES;
    if (@available(iOS 14.0, *)) {
        preferences.fraudulentWebsiteWarningEnabled = YES;
    }
    config.preferences = preferences;
    
    if (@available(iOS 14.0, *)) {
        config.defaultWebpagePreferences.allowsContentJavaScript = YES;
    }
    if (@available(iOS 13.0, *)) {
        config.defaultWebpagePreferences.preferredContentMode = WKContentModeRecommended;
    }
    
    // User content controller with Stash SDK scripts
    WKUserContentController *userContentController = [[WKUserContentController alloc] init];
    
    // Add viewport meta tag
    NSString *viewportScript = @"var meta = document.createElement('meta'); \
        meta.name = 'viewport'; \
        meta.content = 'width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no, viewport-fit=cover'; \
        document.head.appendChild(meta);";
    WKUserScript *viewportInjection = [[WKUserScript alloc] initWithSource:viewportScript
                                                             injectionTime:WKUserScriptInjectionTimeAtDocumentEnd
                                                          forMainFrameOnly:YES];
    [userContentController addUserScript:viewportInjection];
    
    // JavaScript code to set up Stash SDK functions
    NSString *stashSDKScript = @"(function() {"
        "window.stash_sdk = window.stash_sdk || {};"
        "window.stash_sdk.onPaymentSuccess = function(data) {"
            "window.webkit.messageHandlers.stashPaymentSuccess.postMessage(data || {});"
        "};"
        "window.stash_sdk.onPaymentFailure = function(data) {"
            "window.webkit.messageHandlers.stashPaymentFailure.postMessage(data || {});"
        "};"
        "window.stash_sdk.onPurchaseProcessing = function(data) {"
            "window.webkit.messageHandlers.stashPurchaseProcessing.postMessage(data || {});"
        "};"
        "window.stash_sdk.setPaymentChannel = function(optinType) {"
            "window.webkit.messageHandlers.stashOptin.postMessage(optinType || '');"
        "};"
        "window.stash_sdk.expand = function() {"
            "window.webkit.messageHandlers.stashExpand.postMessage({});"
        "};"
        "window.stash_sdk.collapse = function() {"
            "window.webkit.messageHandlers.stashCollapse.postMessage({});"
        "};"
    "})();";
    WKUserScript *stashSDKInjection = [[WKUserScript alloc] initWithSource:stashSDKScript
                                                             injectionTime:WKUserScriptInjectionTimeAtDocumentStart
                                                          forMainFrameOnly:YES];
    [userContentController addUserScript:stashSDKInjection];
    
    // Add viewport meta tag to prevent any margins
    NSString *noMarginsScript = @"var style = document.createElement('style'); \
        style.innerHTML = 'body { margin: 0 !important; padding: 0 !important; min-height: 100% !important; } \
        html { margin: 0 !important; padding: 0 !important; height: 100% !important; }'; \
        document.head.appendChild(style);";
    WKUserScript *noMarginsInjection = [[WKUserScript alloc] initWithSource:noMarginsScript
                                                              injectionTime:WKUserScriptInjectionTimeAtDocumentStart
                                                           forMainFrameOnly:YES];
    [userContentController addUserScript:noMarginsInjection];
    
    // Add script message handlers
    [userContentController addScriptMessageHandler:internal name:@"stashPaymentSuccess"];
    [userContentController addScriptMessageHandler:internal name:@"stashPaymentFailure"];
    [userContentController addScriptMessageHandler:internal name:@"stashPurchaseProcessing"];
    [userContentController addScriptMessageHandler:internal name:@"stashOptin"];
    [userContentController addScriptMessageHandler:internal name:@"stashExpand"];
    [userContentController addScriptMessageHandler:internal name:@"stashCollapse"];
    config.userContentController = userContentController;
    
    // Create WebView
    UIColor *systemBackgroundColor = getSystemBackgroundColor();
    WKWebView *webView = [[WKWebView alloc] initWithFrame:CGRectZero configuration:config];
    webView.opaque = YES;
    webView.hidden = NO;
    webView.alpha = 0.0; // Start at 0 opacity for seamless cross-fade
    webView.translatesAutoresizingMaskIntoConstraints = NO;
    setWebViewBackgroundColor(webView, systemBackgroundColor);
    webView.scrollView.opaque = YES;
    configureScrollViewForWebView(webView.scrollView);
    webView.scrollView.scrollEnabled = YES;
    webView.scrollView.showsVerticalScrollIndicator = _showScrollbar;
    webView.scrollView.showsHorizontalScrollIndicator = NO;
    
    // Create loading view
    UIView *loadingView = [self createLoadingViewWithFrame:CGRectZero];
    loadingView.backgroundColor = systemBackgroundColor;
    loadingView.translatesAutoresizingMaskIntoConstraints = NO;
    loadingView.opaque = YES;
    loadingView.alpha = _usePopupPresentation ? 0.0 : 1.0;
    
    [containerVC.view addSubview:webView];
    [containerVC.view addSubview:loadingView];
    
    // Pin views to edges
    [NSLayoutConstraint activateConstraints:@[
        [loadingView.leadingAnchor constraintEqualToAnchor:containerVC.view.leadingAnchor],
        [loadingView.trailingAnchor constraintEqualToAnchor:containerVC.view.trailingAnchor],
        [loadingView.topAnchor constraintEqualToAnchor:containerVC.view.topAnchor],
        [loadingView.bottomAnchor constraintEqualToAnchor:containerVC.view.bottomAnchor]
    ]];
    [NSLayoutConstraint activateConstraints:@[
        [webView.leadingAnchor constraintEqualToAnchor:containerVC.view.leadingAnchor],
        [webView.trailingAnchor constraintEqualToAnchor:containerVC.view.trailingAnchor],
        [webView.topAnchor constraintEqualToAnchor:containerVC.view.topAnchor],
        [webView.bottomAnchor constraintEqualToAnchor:containerVC.view.bottomAnchor]
    ]];
    
    // Create delegates
    WebViewLoadDelegate *delegate = [[WebViewLoadDelegate alloc] initWithWebView:webView loadingView:loadingView];
    webView.navigationDelegate = delegate;
    
    WebViewUIDelegate *uiDelegate = [[WebViewUIDelegate alloc] init];
    webView.UIDelegate = uiDelegate;
    
    objc_setAssociatedObject(containerVC, "webViewDelegate", delegate, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    objc_setAssociatedObject(containerVC, "webViewUIDelegate", uiDelegate, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    if (_usePopupPresentation) {
        objc_setAssociatedObject(containerVC, "loadingView", loadingView, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    
    // Load request
    NSURL *nsurl = [NSURL URLWithString:url];
    if (nsurl) {
        NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:nsurl
                                                               cachePolicy:NSURLRequestReturnCacheDataElseLoad
                                                           timeoutInterval:15.0];
        [request setValue:@"gzip, deflate, br" forHTTPHeaderField:@"Accept-Encoding"];
        delegate.pageLoadStartTime = CFAbsoluteTimeGetCurrent();
        [webView loadRequest:request];
    }
    
    // Use window-based presentation for all cases (iPhone, iPad, popup)
    internal.previousKeyWindow = [UIApplication sharedApplication].keyWindow;
    
    // NOTE: iPhone card starts below screen (frame-based animation), iPad/popup start at final position
    CGFloat initialY;
    if (!_usePopupPresentation && !isRunningOniPad()) {
        CGFloat screenHeight = [UIScreen mainScreen].bounds.size.height;
        initialY = screenHeight + height;
    } else {
        initialY = finalY;
    }
    
    // Set frame BEFORE creating window
    containerVC.customFrame = CGRectMake(x, initialY, width, height);
    containerVC.view.autoresizingMask = UIViewAutoresizingNone;
    containerVC.view.frame = containerVC.customFrame;
    containerVC.view.alpha = 0.0;
    containerVC.view.transform = CGAffineTransformIdentity;
    
    // Create window
    UIWindow *cardWindow = [[UIWindow alloc] initWithFrame:[UIScreen mainScreen].bounds];
    cardWindow.windowLevel = UIWindowLevelAlert;
    cardWindow.backgroundColor = [UIColor clearColor];
    cardWindow.hidden = YES;
    internal.portraitWindow = cardWindow;
    internal.currentPresentedVC = containerVC;
    
    // Disable layout updates during initial setup
    containerVC.skipLayoutDuringInitialSetup = YES;
    
    // Set rootViewController
    cardWindow.rootViewController = containerVC;
    
    [CATransaction begin];
    [CATransaction setDisableActions:YES];
    [UIView setAnimationsEnabled:NO];
    
    // NOTE: iPhone card frame starts below screen for slide-up animation, iPad/popup at final position
    if (!_usePopupPresentation && !isRunningOniPad()) {
        CGFloat screenHeight = [UIScreen mainScreen].bounds.size.height;
        CGFloat belowScreenY = screenHeight + height;
        containerVC.view.frame = CGRectMake(x, belowScreenY, width, height);
        containerVC.customFrame = CGRectMake(x, belowScreenY, width, height);
    } else {
        containerVC.view.frame = containerVC.customFrame;
    }
    containerVC.view.autoresizingMask = UIViewAutoresizingNone;
    containerVC.view.alpha = 0.0;
    containerVC.view.transform = CGAffineTransformIdentity;
    
    [CATransaction commit];
    [UIView setAnimationsEnabled:YES];
    
    // iPad and popup: Force layout to ensure frame is applied
    if (isRunningOniPad() || _usePopupPresentation) {
        [containerVC.view setNeedsLayout];
        [containerVC.view layoutIfNeeded];
        
        if (!CGRectEqualToRect(containerVC.view.frame, containerVC.customFrame)) {
            [CATransaction begin];
            [CATransaction setDisableActions:YES];
            containerVC.view.frame = containerVC.customFrame;
            containerVC.view.bounds = CGRectMake(0, 0, containerVC.customFrame.size.width, containerVC.customFrame.size.height);
            [CATransaction commit];
        }
    }
    
    // CRITICAL: Verify iPhone card frame is below screen before showing (iOS may reset it)
    if (!_usePopupPresentation && !isRunningOniPad()) {
        CGFloat actualScreenHeight = [UIScreen mainScreen].bounds.size.height;
        CGFloat currentY = containerVC.view.frame.origin.y;
        if (currentY < actualScreenHeight) {
            CGFloat belowScreenY = actualScreenHeight + height;
            [CATransaction begin];
            [CATransaction setDisableActions:YES];
            containerVC.view.frame = CGRectMake(x, belowScreenY, width, height);
            containerVC.customFrame = CGRectMake(x, belowScreenY, width, height);
            [CATransaction commit];
        }
    }
    
    cardWindow.hidden = NO;
    [cardWindow makeKeyAndVisible];
    
    // On iPad, default size is the "expanded" state
    if (isRunningOniPad() && !_usePopupPresentation) {
        _isCardExpanded = YES;
    }
    
    if (isRunningOniPad() || _usePopupPresentation) {
        dispatch_async(dispatch_get_main_queue(), ^{
            if (!CGRectEqualToRect(containerVC.view.frame, containerVC.customFrame)) {
                [CATransaction begin];
                [CATransaction setDisableActions:YES];
                containerVC.view.frame = containerVC.customFrame;
                containerVC.view.bounds = CGRectMake(0, 0, containerVC.customFrame.size.width, containerVC.customFrame.size.height);
                [CATransaction commit];
            }
        });
    } else {
        // NOTE: Re-verify iPhone card frame after window appears (iOS can reset during window creation)
        dispatch_async(dispatch_get_main_queue(), ^{
            CGFloat actualScreenHeight = [UIScreen mainScreen].bounds.size.height;
            CGFloat currentY = containerVC.view.frame.origin.y;
            if (currentY < actualScreenHeight) {
                CGFloat belowScreenY = actualScreenHeight + height;
                [CATransaction begin];
                [CATransaction setDisableActions:YES];
                containerVC.view.frame = CGRectMake(x, belowScreenY, width, height);
                containerVC.customFrame = CGRectMake(x, belowScreenY, width, height);
                [CATransaction commit];
            }
        });
    }
    
    // Create dark overlay UNDER the card view
    UIView *overlayView = [[UIView alloc] initWithFrame:[UIScreen mainScreen].bounds];
    overlayView.backgroundColor = [UIColor colorWithWhite:0.0 alpha:0.0];
    overlayView.userInteractionEnabled = YES;
    [cardWindow insertSubview:overlayView atIndex:0];
    
    // Store overlay reference for gesture handlers
    objc_setAssociatedObject(containerVC, "overlayView", overlayView, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    
    // Apply corner radius
    UIRectCorner cornersToRound = getCornersToRoundForPosition(_cardVerticalPosition, isRunningOniPad());
    CGRect maskBounds = CGRectMake(0, 0, containerVC.customFrame.size.width, containerVC.customFrame.size.height);
    CAShapeLayer *maskLayer = createCornerRadiusMask(maskBounds, cornersToRound, kCornerRadiusDefault);
    containerVC.view.layer.mask = maskLayer;
    
    // Add shadow
    containerVC.view.layer.shadowColor = [UIColor blackColor].CGColor;
    containerVC.view.layer.shadowOffset = CGSizeMake(0, -2);
    containerVC.view.layer.shadowOpacity = 0.15;
    containerVC.view.layer.shadowRadius = kCornerRadiusDefault;
    
    CGFloat overlayOpacity = isRunningOniPad() ? 0.25 : 0.35;
    CGFloat animationDuration = _usePopupPresentation ? 0.18 : kAnimationDurationDefault;
    
    // Capture final values for animation block
    CGFloat animFinalY = finalY;
    CGFloat animWidth = width;
    CGFloat animHeight = height;
    CGFloat animX = x;
    
    // Animate
    if (_usePopupPresentation) {
        // Popup mode: fade-in
        [UIView animateWithDuration:animationDuration
                              delay:0
                            options:UIViewAnimationOptionCurveEaseOut
                         animations:^{
            containerVC.view.alpha = 1.0;
            overlayView.backgroundColor = [UIColor colorWithWhite:0.0 alpha:overlayOpacity];
            UIView *loadingViewRef = objc_getAssociatedObject(containerVC, "loadingView");
            if (loadingViewRef) {
                loadingViewRef.alpha = 1.0;
            }
        } completion:^(BOOL finished) {
            containerVC.skipLayoutDuringInitialSetup = NO;
            if (!isRunningOniPad()) {
                [internal startKeyboardObserving];
            }
        }];
    } else {
        if (isRunningOniPad()) {
            // iPad card mode: fade-in with spring
            [UIView animateWithDuration:animationDuration
                                  delay:0
                 usingSpringWithDamping:kSpringDampingDefault
                  initialSpringVelocity:0.5
                                options:UIViewAnimationOptionCurveEaseOut
                             animations:^{
                containerVC.view.alpha = 1.0;
                containerVC.view.transform = CGAffineTransformIdentity;
                overlayView.backgroundColor = [UIColor colorWithWhite:0.0 alpha:overlayOpacity];
            } completion:^(BOOL finished) {
                containerVC.skipLayoutDuringInitialSetup = NO;
                
                UIView *dragTray = [internal createDragTray:animWidth];
                [containerVC.view addSubview:dragTray];
                internal.dragTrayView = dragTray;
            }];
        } else {
            // iPhone card animation - Apple Pay style: quick, responsive slide up from BOTTOM
            // First fade in the overlay
            [UIView animateWithDuration:0.1 animations:^{
                overlayView.backgroundColor = [UIColor colorWithWhite:0.0 alpha:overlayOpacity];
            }];
            
            // Apple Pay animation: slide up from below screen with spring
            [UIView animateWithDuration:0.45
                                  delay:0.05
                 usingSpringWithDamping:0.88
                  initialSpringVelocity:0.2
                                options:UIViewAnimationOptionCurveEaseOut
                             animations:^{
                containerVC.view.frame = CGRectMake(animX, animFinalY, animWidth, animHeight);
                containerVC.customFrame = CGRectMake(animX, animFinalY, animWidth, animHeight);
                containerVC.view.alpha = 1.0;
            } completion:^(BOOL finished) {
                containerVC.skipLayoutDuringInitialSetup = NO;
                
                // Add drag tray
                UIView *dragTray = [internal createDragTray:animWidth];
                [containerVC.view addSubview:dragTray];
                internal.dragTrayView = dragTray;
                
                // iPhone: tap-to-dismiss on overlay
                UIButton *dismissButton = [UIButton buttonWithType:UIButtonTypeCustom];
                dismissButton.frame = overlayView.bounds;
                dismissButton.backgroundColor = [UIColor clearColor];
                dismissButton.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
                [overlayView addSubview:dismissButton];
                [dismissButton addTarget:self
                                  action:@selector(handleOverlayTap)
                        forControlEvents:UIControlEventTouchUpInside];
                
                // Start keyboard observing
                [internal startKeyboardObserving];
            }];
        }
    }
}

- (UIView *)createLoadingViewWithFrame:(CGRect)frame {
    UIView *loadingView = [[UIView alloc] initWithFrame:frame];
    
    BOOL isDarkMode = NO;
    if (@available(iOS 13.0, *)) {
        UIUserInterfaceStyle currentStyle = [UITraitCollection currentTraitCollection].userInterfaceStyle;
        isDarkMode = (currentStyle == UIUserInterfaceStyleDark);
    }
    
    UIColor *backgroundColor = isDarkMode ? [UIColor blackColor] : [UIColor whiteColor];
    loadingView.backgroundColor = backgroundColor;
    loadingView.opaque = YES;
    
    UIActivityIndicatorView *spinner;
    if (@available(iOS 13.0, *)) {
        spinner = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleLarge];
        spinner.color = isDarkMode ? [UIColor whiteColor] : [UIColor darkGrayColor];
    } else {
        spinner = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleWhiteLarge];
        if (!isDarkMode) {
            spinner.color = [UIColor darkGrayColor];
        }
    }
    
    spinner.translatesAutoresizingMaskIntoConstraints = NO;
    spinner.hidesWhenStopped = NO;
    [spinner startAnimating];
    [loadingView addSubview:spinner];
    
    [NSLayoutConstraint activateConstraints:@[
        [spinner.centerXAnchor constraintEqualToAnchor:loadingView.centerXAnchor],
        [spinner.centerYAnchor constraintEqualToAnchor:loadingView.centerYAnchor]
    ]];
    
    return loadingView;
}

- (void)handleOverlayTap {
    if ([StashPayCardInternal sharedInstance].isPurchaseProcessing) {
        return;
    }
    [self dismiss];
}

- (void)dismiss {
    StashPayCardInternal *internal = [StashPayCardInternal sharedInstance];
    [internal dismissWithAnimation:^{
        [internal cleanupCardInstance];
        [internal callDelegateCallbackOnce];
    }];
}

- (void)resetPresentationState {
    StashPayCardInternal *internal = [StashPayCardInternal sharedInstance];
    [internal cleanupCardInstance];
    _isCardCurrentlyPresented = NO;
}

- (void)dismissSafariViewController {
    StashPayCardInternal *internal = [StashPayCardInternal sharedInstance];
    if (internal.currentSafariViewController) {
        [internal.currentSafariViewController dismissViewControllerAnimated:YES completion:^{
            internal.currentSafariViewController = nil;
            
            if (self.delegate && [self.delegate respondsToSelector:@selector(stashPayCardDidDismiss)]) {
                [self.delegate stashPayCardDidDismiss];
            }
        }];
    }
}

- (void)dismissSafariViewControllerWithResult:(BOOL)success {
    StashPayCardInternal *internal = [StashPayCardInternal sharedInstance];
    if (internal.currentSafariViewController) {
        if (success) {
            if (self.delegate && [self.delegate respondsToSelector:@selector(stashPayCardDidCompletePayment)]) {
                [self.delegate stashPayCardDidCompletePayment];
            }
        } else {
            if (self.delegate && [self.delegate respondsToSelector:@selector(stashPayCardDidFailPayment)]) {
                [self.delegate stashPayCardDidFailPayment];
            }
        }
        
        [internal.currentSafariViewController dismissViewControllerAnimated:YES completion:^{
            internal.currentSafariViewController = nil;
            
            if (self.delegate && [self.delegate respondsToSelector:@selector(stashPayCardDidDismiss)]) {
                [self.delegate stashPayCardDidDismiss];
            }
        }];
    }
}

@end

#if !__has_feature(objc_arc)
#pragma clang diagnostic pop
#endif
