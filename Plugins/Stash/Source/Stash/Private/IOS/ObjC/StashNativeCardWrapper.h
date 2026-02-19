// Copyright Stash. All Rights Reserved.
// Stash Unreal Engine SDK - iOS Wrapper Interface (Stash Native 2.0.0)

#pragma once

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Objective-C wrapper for Stash Native iOS SDK.
 * Bridges the native Stash Native SDK with Unreal Engine C++ code.
 */
@interface StashNativeCardWrapper : NSObject

+ (StashNativeCardWrapper*)sharedInstance;

#pragma mark - Card

- (void)openCardWithURL:(NSString*)urlString;
- (void)openCardWithURL:(NSString*)urlString
    forcePortrait:(BOOL)forcePortrait
    cardHeightRatioPortrait:(float)cardHeightRatioPortrait
    cardWidthRatioLandscape:(float)cardWidthRatioLandscape
    cardHeightRatioLandscape:(float)cardHeightRatioLandscape
    tabletWidthRatioPortrait:(float)tabletWidthRatioPortrait
    tabletHeightRatioPortrait:(float)tabletHeightRatioPortrait
    tabletWidthRatioLandscape:(float)tabletWidthRatioLandscape
    tabletHeightRatioLandscape:(float)tabletHeightRatioLandscape;
- (BOOL)isCardOpen;
- (BOOL)isPurchaseProcessing;
- (void)dismissCard;

#pragma mark - Modal

- (void)openModalWithURL:(NSString*)urlString;
- (void)openModalWithURL:(NSString*)urlString
              showDragBar:(BOOL)showDragBar
             allowDismiss:(BOOL)allowDismiss
   phoneWidthRatioPortrait:(float)phoneWidthRatioPortrait
  phoneHeightRatioPortrait:(float)phoneHeightRatioPortrait
  phoneWidthRatioLandscape:(float)phoneWidthRatioLandscape
 phoneHeightRatioLandscape:(float)phoneHeightRatioLandscape
  tabletWidthRatioPortrait:(float)tabletWidthRatioPortrait
 tabletHeightRatioPortrait:(float)tabletHeightRatioPortrait
 tabletWidthRatioLandscape:(float)tabletWidthRatioLandscape
tabletHeightRatioLandscape:(float)tabletHeightRatioLandscape;

#pragma mark - Browser

- (void)openBrowserWithURL:(NSString*)urlString;
- (void)closeBrowser;

#pragma mark - Configuration

/** When YES, app is restricted to landscape when card/modal is not open; portrait allowed when Stash Native UI is open. Call at startup for landscape-only games. */
- (void)setLandscapeLockWhenCardClosed:(BOOL)enable;

@end

#ifdef __cplusplus
}
#endif
