// Copyright Stash. All Rights Reserved.
// Stash Pay Unreal Engine SDK - iOS Wrapper Interface

#pragma once

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * StashPayCardWrapper - Objective-C wrapper for Stash Pay iOS SDK
 * 
 * This class bridges the Stash Pay native iOS SDK with Unreal Engine.
 * It provides singleton access and handles delegate callbacks.
 */
@interface StashPayCardWrapper : NSObject

/**
 * Returns the shared singleton instance.
 */
+ (StashPayCardWrapper*)sharedInstance;

/**
 * Opens the Stash Pay checkout dialog with the specified URL.
 * @param urlString The checkout URL to load
 */
- (void)openCheckoutWithURL:(NSString*)urlString;

/**
 * Checks if the checkout dialog is currently open.
 * @return YES if the checkout is currently displayed
 */
- (BOOL)isCheckoutOpen;

/**
 * Dismisses the currently displayed checkout dialog.
 */
- (void)dismissCheckout;

@end

#ifdef __cplusplus
}
#endif
