// Copyright Stash. All Rights Reserved.
// Stash Pay Unreal Engine SDK - Blueprint Function Library Implementation

#include "StashBlueprint.h"
#include "Stash.h"
#include <Async/Async.h>

#if PLATFORM_ANDROID
#include "Android/Utils/AndroidUtils.h"
#endif

#if PLATFORM_IOS
#include "IOS/Utils/ObjC_Convert.h"
#include "IOS/ObjC/StashPayCardWrapper.h"
#endif

// Initialize static delegates
FOnStashPaymentSuccess UStashBlueprint::OnPaymentSuccess;
FOnStashPaymentFailure UStashBlueprint::OnPaymentFailure;
FOnStashDialogDismissed UStashBlueprint::OnDialogDismissed;
FOnStashOptInResponse UStashBlueprint::OnOptInResponse;
FOnStashPageLoaded UStashBlueprint::OnPageLoaded;
FOnStashNetworkError UStashBlueprint::OnNetworkError;

FStashCheckoutConfig UStashBlueprint::MakeStashCheckoutConfig(
	bool bForcePortraitOnCheckout,
	float CardHeightRatioPortrait,
	float CardWidthRatioLandscape,
	float CardHeightRatioLandscape,
	float TabletWidthRatioPortrait,
	float TabletHeightRatioPortrait,
	float TabletWidthRatioLandscape,
	float TabletHeightRatioLandscape)
{
	FStashCheckoutConfig Config;
	Config.bForcePortraitOnCheckout = bForcePortraitOnCheckout;
	Config.CardHeightRatioPortrait = FMath::Clamp(CardHeightRatioPortrait, 0.1f, 1.0f);
	Config.CardWidthRatioLandscape = FMath::Clamp(CardWidthRatioLandscape, 0.1f, 1.0f);
	Config.CardHeightRatioLandscape = FMath::Clamp(CardHeightRatioLandscape, 0.1f, 1.0f);
	Config.TabletWidthRatioPortrait = FMath::Clamp(TabletWidthRatioPortrait, 0.1f, 1.0f);
	Config.TabletHeightRatioPortrait = FMath::Clamp(TabletHeightRatioPortrait, 0.1f, 1.0f);
	Config.TabletWidthRatioLandscape = FMath::Clamp(TabletWidthRatioLandscape, 0.1f, 1.0f);
	Config.TabletHeightRatioLandscape = FMath::Clamp(TabletHeightRatioLandscape, 0.1f, 1.0f);
	return Config;
}

void UStashBlueprint::OpenCheckout(const FString& CheckoutURL)
{
#if PLATFORM_IOS
	UE_LOG(LogStash, Log, TEXT("[Stash] Opening checkout on iOS: %s"), *CheckoutURL);
	[[StashPayCardWrapper sharedInstance] openCheckoutWithURL:CheckoutURL.GetNSString()];
#elif PLATFORM_ANDROID
	UE_LOG(LogStash, Log, TEXT("[Stash] Opening checkout on Android: %s"), *CheckoutURL);
	AndroidUtils::CallJavaCode<void>(
		"com/Plugins/Stash/StashHelper",
		"OpenCheckout",
		"",
		true,  // Pass activity
		CheckoutURL
	);
#else
	UE_LOG(LogStash, Warning, TEXT("[Stash] OpenCheckout called on unsupported platform"));
#endif
}

void UStashBlueprint::OpenCheckoutWithConfig(const FString& CheckoutURL, const FStashCheckoutConfig& Config)
{
#if PLATFORM_IOS
	UE_LOG(LogStash, Log, TEXT("[Stash] Opening checkout with config on iOS: %s"), *CheckoutURL);
	
	// Apply checkout configuration
	StashPayCardWrapper* wrapper = [StashPayCardWrapper sharedInstance];
	[wrapper setForcePortraitOnCheckout:Config.bForcePortraitOnCheckout];
	[wrapper setCardHeightRatioPortrait:FMath::Clamp(Config.CardHeightRatioPortrait, 0.1f, 1.0f)];
	[wrapper setCardWidthRatioLandscape:FMath::Clamp(Config.CardWidthRatioLandscape, 0.1f, 1.0f)];
	[wrapper setCardHeightRatioLandscape:FMath::Clamp(Config.CardHeightRatioLandscape, 0.1f, 1.0f)];
	[wrapper setTabletWidthRatioPortrait:FMath::Clamp(Config.TabletWidthRatioPortrait, 0.1f, 1.0f)];
	[wrapper setTabletHeightRatioPortrait:FMath::Clamp(Config.TabletHeightRatioPortrait, 0.1f, 1.0f)];
	[wrapper setTabletWidthRatioLandscape:FMath::Clamp(Config.TabletWidthRatioLandscape, 0.1f, 1.0f)];
	[wrapper setTabletHeightRatioLandscape:FMath::Clamp(Config.TabletHeightRatioLandscape, 0.1f, 1.0f)];
	
	// Open checkout
	[wrapper openCheckoutWithURL:CheckoutURL.GetNSString()];
#elif PLATFORM_ANDROID
	UE_LOG(LogStash, Log, TEXT("[Stash] Opening checkout with config on Android: %s"), *CheckoutURL);
	
	// Apply checkout configuration
	AndroidUtils::CallJavaCode<void>("com/Plugins/Stash/StashHelper", "SetForcePortraitOnCheckout", "", false,
		Config.bForcePortraitOnCheckout);
	AndroidUtils::CallJavaCode<void>("com/Plugins/Stash/StashHelper", "SetCardHeightRatioPortrait", "", false,
		FMath::Clamp(Config.CardHeightRatioPortrait, 0.1f, 1.0f));
	AndroidUtils::CallJavaCode<void>("com/Plugins/Stash/StashHelper", "SetCardWidthRatioLandscape", "", false,
		FMath::Clamp(Config.CardWidthRatioLandscape, 0.1f, 1.0f));
	AndroidUtils::CallJavaCode<void>("com/Plugins/Stash/StashHelper", "SetCardHeightRatioLandscape", "", false,
		FMath::Clamp(Config.CardHeightRatioLandscape, 0.1f, 1.0f));
	AndroidUtils::CallJavaCode<void>("com/Plugins/Stash/StashHelper", "SetTabletWidthRatioPortrait", "", false,
		FMath::Clamp(Config.TabletWidthRatioPortrait, 0.1f, 1.0f));
	AndroidUtils::CallJavaCode<void>("com/Plugins/Stash/StashHelper", "SetTabletHeightRatioPortrait", "", false,
		FMath::Clamp(Config.TabletHeightRatioPortrait, 0.1f, 1.0f));
	AndroidUtils::CallJavaCode<void>("com/Plugins/Stash/StashHelper", "SetTabletWidthRatioLandscape", "", false,
		FMath::Clamp(Config.TabletWidthRatioLandscape, 0.1f, 1.0f));
	AndroidUtils::CallJavaCode<void>("com/Plugins/Stash/StashHelper", "SetTabletHeightRatioLandscape", "", false,
		FMath::Clamp(Config.TabletHeightRatioLandscape, 0.1f, 1.0f));
	
	// Open checkout
	AndroidUtils::CallJavaCode<void>(
		"com/Plugins/Stash/StashHelper",
		"OpenCheckout",
		"",
		true,  // Pass activity
		CheckoutURL
	);
#else
	UE_LOG(LogStash, Warning, TEXT("[Stash] OpenCheckoutWithConfig called on unsupported platform"));
#endif
}

bool UStashBlueprint::IsCheckoutOpen()
{
#if PLATFORM_IOS
	return [[StashPayCardWrapper sharedInstance] isCheckoutOpen];
#elif PLATFORM_ANDROID
	return AndroidUtils::CallJavaCode<bool>(
		"com/Plugins/Stash/StashHelper",
		"IsCheckoutOpen",
		"",
		false
	);
#else
	return false;
#endif
}

void UStashBlueprint::DismissCheckout()
{
#if PLATFORM_IOS
	UE_LOG(LogStash, Log, TEXT("[Stash] Dismissing checkout on iOS"));
	[[StashPayCardWrapper sharedInstance] dismissCheckout];
#elif PLATFORM_ANDROID
	UE_LOG(LogStash, Log, TEXT("[Stash] Dismissing checkout on Android"));
	AndroidUtils::CallJavaCode<void>(
		"com/Plugins/Stash/StashHelper",
		"DismissCheckout",
		"",
		true  // Pass activity
	);
#endif
}

// ============================================================================
// Modal Presentation (SDK 1.2.0+)
// ============================================================================

void UStashBlueprint::OpenModal(const FString& URL)
{
#if PLATFORM_IOS
	UE_LOG(LogStash, Log, TEXT("[Stash] Opening modal on iOS: %s"), *URL);
	[[StashPayCardWrapper sharedInstance] openModalWithURL:URL.GetNSString()];
#elif PLATFORM_ANDROID
	UE_LOG(LogStash, Log, TEXT("[Stash] Opening modal on Android: %s"), *URL);
	AndroidUtils::CallJavaCode<void>(
		"com/Plugins/Stash/StashHelper",
		"OpenModal",
		"",
		true,  // Pass activity
		URL
	);
#else
	UE_LOG(LogStash, Warning, TEXT("[Stash] OpenModal called on unsupported platform"));
#endif
}

void UStashBlueprint::OpenModalWithConfig(const FString& URL, const FStashModalConfig& Config)
{
#if PLATFORM_IOS
	UE_LOG(LogStash, Log, TEXT("[Stash] Opening modal with config on iOS: %s"), *URL);
	[[StashPayCardWrapper sharedInstance] openModalWithURL:URL.GetNSString()
		showDragBar:Config.bShowDragBar
		allowDismiss:Config.bAllowDismiss
		phoneWidthRatioPortrait:Config.PhoneWidthRatioPortrait
		phoneHeightRatioPortrait:Config.PhoneHeightRatioPortrait
		phoneWidthRatioLandscape:Config.PhoneWidthRatioLandscape
		phoneHeightRatioLandscape:Config.PhoneHeightRatioLandscape
		tabletWidthRatioPortrait:Config.TabletWidthRatioPortrait
		tabletHeightRatioPortrait:Config.TabletHeightRatioPortrait
		tabletWidthRatioLandscape:Config.TabletWidthRatioLandscape
		tabletHeightRatioLandscape:Config.TabletHeightRatioLandscape];
#elif PLATFORM_ANDROID
	UE_LOG(LogStash, Log, TEXT("[Stash] Opening modal with config on Android: %s"), *URL);
	AndroidUtils::CallJavaCode<void>(
		"com/Plugins/Stash/StashHelper",
		"OpenModalWithConfig",
		"",
		true,  // Pass activity
		URL,
		Config.bShowDragBar,
		Config.bAllowDismiss,
		Config.PhoneWidthRatioPortrait,
		Config.PhoneHeightRatioPortrait,
		Config.PhoneWidthRatioLandscape,
		Config.PhoneHeightRatioLandscape,
		Config.TabletWidthRatioPortrait,
		Config.TabletHeightRatioPortrait,
		Config.TabletWidthRatioLandscape,
		Config.TabletHeightRatioLandscape
	);
#else
	UE_LOG(LogStash, Warning, TEXT("[Stash] OpenModalWithConfig called on unsupported platform"));
#endif
}

// ============================================================================
// Configuration (SDK 1.2.0+)
// ============================================================================

void UStashBlueprint::SetForceWebBasedCheckout(bool bForce)
{
#if PLATFORM_IOS
	UE_LOG(LogStash, Log, TEXT("[Stash] Setting force web-based checkout: %s"), bForce ? TEXT("true") : TEXT("false"));
	[[StashPayCardWrapper sharedInstance] setForceWebBasedCheckout:bForce];
#elif PLATFORM_ANDROID
	UE_LOG(LogStash, Log, TEXT("[Stash] Setting force web-based checkout: %s"), bForce ? TEXT("true") : TEXT("false"));
	AndroidUtils::CallJavaCode<void>(
		"com/Plugins/Stash/StashHelper",
		"SetForceWebBasedCheckout",
		"",
		false,
		bForce
	);
#endif
}

void UStashBlueprint::SetLandscapeLockWhenCheckoutClosed(bool bEnable)
{
#if PLATFORM_IOS
	[[StashPayCardWrapper sharedInstance] setLandscapeLockWhenCheckoutClosed:bEnable];
#else
	// Android: no-op; orientation lock is handled by project/activity settings
	(void)bEnable;
#endif
}

// Callback handlers - called from native code
void UStashBlueprint::HandlePaymentSuccess()
{
	UE_LOG(LogStash, Log, TEXT("[Stash] Payment success callback received"));
	AsyncTask(ENamedThreads::GameThread, []() {
		OnPaymentSuccess.Broadcast();
	});
}

void UStashBlueprint::HandlePaymentFailure()
{
	UE_LOG(LogStash, Log, TEXT("[Stash] Payment failure callback received"));
	AsyncTask(ENamedThreads::GameThread, []() {
		OnPaymentFailure.Broadcast();
	});
}

void UStashBlueprint::HandleDialogDismissed()
{
	UE_LOG(LogStash, Log, TEXT("[Stash] Dialog dismissed callback received"));
	AsyncTask(ENamedThreads::GameThread, []() {
		OnDialogDismissed.Broadcast();
	});
}

void UStashBlueprint::HandleOptInResponse(const FString& OptInType)
{
	UE_LOG(LogStash, Log, TEXT("[Stash] Opt-in response received: %s"), *OptInType);
	AsyncTask(ENamedThreads::GameThread, [OptInType]() {
		OnOptInResponse.Broadcast(OptInType);
	});
}

void UStashBlueprint::HandlePageLoaded(float LoadTimeMs)
{
	UE_LOG(LogStash, Log, TEXT("[Stash] Page loaded callback received: %.2f ms"), LoadTimeMs);
	AsyncTask(ENamedThreads::GameThread, [LoadTimeMs]() {
		OnPageLoaded.Broadcast(LoadTimeMs);
	});
}

void UStashBlueprint::HandleNetworkError()
{
	UE_LOG(LogStash, Warning, TEXT("[Stash] Network error callback received"));
	AsyncTask(ENamedThreads::GameThread, []() {
		OnNetworkError.Broadcast();
	});
}

// iOS Callback bridge functions (called from StashPayCardWrapper.mm)
#if PLATFORM_IOS
extern "C" {
	void StashPayOnPaymentSuccess()
	{
		UStashBlueprint::HandlePaymentSuccess();
	}
	
	void StashPayOnPaymentFailure()
	{
		UStashBlueprint::HandlePaymentFailure();
	}
	
	void StashPayOnDialogDismissed()
	{
		UStashBlueprint::HandleDialogDismissed();
	}
	
	void StashPayOnOptInResponse(const char* optinType)
	{
		UStashBlueprint::HandleOptInResponse(FString(UTF8_TO_TCHAR(optinType ? optinType : "")));
	}
	
	void StashPayOnPageLoaded(double loadTimeMs)
	{
		UStashBlueprint::HandlePageLoaded((float)loadTimeMs);
	}
	
	void StashPayOnNetworkError()
	{
		UStashBlueprint::HandleNetworkError();
	}
}
#endif

// Android JNI Callback functions (called from StashHelper.java)
#if PLATFORM_ANDROID
extern "C" {
	JNIEXPORT void JNICALL Java_com_Plugins_Stash_StashHelper_nativeOnPaymentSuccess(JNIEnv* env, jclass clazz)
	{
		UStashBlueprint::HandlePaymentSuccess();
	}
	
	JNIEXPORT void JNICALL Java_com_Plugins_Stash_StashHelper_nativeOnPaymentFailure(JNIEnv* env, jclass clazz)
	{
		UStashBlueprint::HandlePaymentFailure();
	}
	
	JNIEXPORT void JNICALL Java_com_Plugins_Stash_StashHelper_nativeOnDialogDismissed(JNIEnv* env, jclass clazz)
	{
		UStashBlueprint::HandleDialogDismissed();
	}
	
	JNIEXPORT void JNICALL Java_com_Plugins_Stash_StashHelper_nativeOnOptInResponse(JNIEnv* env, jclass clazz, jstring optinType)
	{
		FString OptInTypeStr;
		if (optinType)
		{
			const char* UTFString = env->GetStringUTFChars(optinType, nullptr);
			if (UTFString)
			{
				OptInTypeStr = FString(UTF8_TO_TCHAR(UTFString));
				env->ReleaseStringUTFChars(optinType, UTFString);
			}
		}
		UStashBlueprint::HandleOptInResponse(OptInTypeStr);
	}
	
	JNIEXPORT void JNICALL Java_com_Plugins_Stash_StashHelper_nativeOnPageLoaded(JNIEnv* env, jclass clazz, jlong loadTimeMs)
	{
		UStashBlueprint::HandlePageLoaded((float)loadTimeMs);
	}
	
	JNIEXPORT void JNICALL Java_com_Plugins_Stash_StashHelper_nativeOnNetworkError(JNIEnv* env, jclass clazz)
	{
		UStashBlueprint::HandleNetworkError();
	}
}
#endif
