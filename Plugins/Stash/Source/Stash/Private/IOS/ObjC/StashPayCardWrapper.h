// Copyright Stash. All Rights Reserved.
// Stash Pay Unreal Engine SDK - iOS Wrapper Interface

#pragma once

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Objective-C wrapper for Stash Pay iOS SDK.
 * Bridges the native iOS SDK with Unreal Engine C++ code.
 */
@interface StashPayCardWrapper : NSObject

+ (StashPayCardWrapper*)sharedInstance;

#pragma mark - Checkout

- (void)openCheckoutWithURL:(NSString*)urlString;
- (BOOL)isCheckoutOpen;
- (void)dismissCheckout;

#pragma mark - Modal

- (void)openModalWithURL:(NSString*)urlString;

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
tabletHeightRatioLandscape:(float)tabletHeightLandscape;

#pragma mark - Configuration

- (void)setForcePortraitOnCheckout:(BOOL)force;
- (void)setCardHeightRatioPortrait:(float)ratio;
- (void)setCardWidthRatioLandscape:(float)ratio;
- (void)setCardHeightRatioLandscape:(float)ratio;
- (void)setTabletWidthRatioPortrait:(float)ratio;
- (void)setTabletHeightRatioPortrait:(float)ratio;
- (void)setTabletWidthRatioLandscape:(float)ratio;
- (void)setTabletHeightRatioLandscape:(float)ratio;
- (void)setForceWebBasedCheckout:(BOOL)force;

/** When YES, app is restricted to landscape when checkout is not open; portrait is allowed when checkout is open (for Stash Pay phone checkout). Call at startup for landscape-only games. */
- (void)setLandscapeLockWhenCheckoutClosed:(BOOL)enable;

@end

#ifdef __cplusplus
}
#endif
