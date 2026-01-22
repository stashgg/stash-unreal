// Copyright Epic Games, Inc. All Rights Reserved.
// Wrapper that includes stash-native StashPayCard.mm for compilation
// This allows the external stash-native SDK to be compiled as part of MobileNativeCode module

#if PLATFORM_IOS

// Include the stash-native implementation directly
#import "../../../../../../stash-native-main/iOS/StashPay/Sources/StashPay/StashPayCard.m"

#endif


