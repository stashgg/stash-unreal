// Copyright Stash. All Rights Reserved.
// Stash Pay Unreal Engine SDK - Blueprint Function Library Implementation

#include "MobileNativeCodeBlueprint.h"
#include "MobileNativeCode.h"
#include <Async/Async.h>
#include <Engine.h>

#if PLATFORM_ANDROID
#include "Android/Utils/AndroidUtils.h"
#endif

#if PLATFORM_IOS
#include "IOS/Utils/ObjC_Convert.h"
#include "IOS/ObjC/StashPayCardWrapper.h"
#endif

// Initialize static delegates
FOnStashPaymentSuccess UMobileNativeCodeBlueprint::OnPaymentSuccess;
FOnStashPaymentFailure UMobileNativeCodeBlueprint::OnPaymentFailure;
FOnStashDialogDismissed UMobileNativeCodeBlueprint::OnDialogDismissed;
FOnStashPageLoaded UMobileNativeCodeBlueprint::OnPageLoaded;

void UMobileNativeCodeBlueprint::OpenCheckout(const FString& CheckoutURL)
{
#if PLATFORM_IOS
	UE_LOG(LogTemp, Log, TEXT("[StashPay] Opening checkout on iOS: %s"), *CheckoutURL);
	[[StashPayCardWrapper sharedInstance] openCheckoutWithURL:CheckoutURL.GetNSString()];
#elif PLATFORM_ANDROID
	UE_LOG(LogTemp, Log, TEXT("[StashPay] Opening checkout on Android: %s"), *CheckoutURL);
	AndroidUtils::CallJavaCode<void>(
		"com/Plugins/MobileNativeCode/StashPayHelper",
		"OpenCheckout",
		"",
		true,  // Pass activity
		CheckoutURL
	);
#else
	UE_LOG(LogTemp, Warning, TEXT("[StashPay] OpenCheckout called on unsupported platform"));
#endif
}

bool UMobileNativeCodeBlueprint::IsCheckoutOpen()
{
#if PLATFORM_IOS
	return [[StashPayCardWrapper sharedInstance] isCheckoutOpen];
#elif PLATFORM_ANDROID
	return AndroidUtils::CallJavaCode<bool>(
		"com/Plugins/MobileNativeCode/StashPayHelper",
		"IsCheckoutOpen",
		"",
		false
	);
#else
	return false;
#endif
}

void UMobileNativeCodeBlueprint::DismissCheckout()
{
#if PLATFORM_IOS
	UE_LOG(LogTemp, Log, TEXT("[StashPay] Dismissing checkout on iOS"));
	[[StashPayCardWrapper sharedInstance] dismissCheckout];
#elif PLATFORM_ANDROID
	UE_LOG(LogTemp, Log, TEXT("[StashPay] Dismissing checkout on Android"));
	AndroidUtils::CallJavaCode<void>(
		"com/Plugins/MobileNativeCode/StashPayHelper",
		"DismissCheckout",
		"",
		true  // Pass activity
	);
#endif
}

// Callback handlers - called from native code
void UMobileNativeCodeBlueprint::HandlePaymentSuccess()
{
	UE_LOG(LogTemp, Log, TEXT("[StashPay] Payment success callback received"));
	AsyncTask(ENamedThreads::GameThread, []() {
		OnPaymentSuccess.Broadcast();
	});
}

void UMobileNativeCodeBlueprint::HandlePaymentFailure()
{
	UE_LOG(LogTemp, Log, TEXT("[StashPay] Payment failure callback received"));
	AsyncTask(ENamedThreads::GameThread, []() {
		OnPaymentFailure.Broadcast();
	});
}

void UMobileNativeCodeBlueprint::HandleDialogDismissed()
{
	UE_LOG(LogTemp, Log, TEXT("[StashPay] Dialog dismissed callback received"));
	AsyncTask(ENamedThreads::GameThread, []() {
		OnDialogDismissed.Broadcast();
	});
}

void UMobileNativeCodeBlueprint::HandlePageLoaded(float LoadTimeMs)
{
	UE_LOG(LogTemp, Log, TEXT("[StashPay] Page loaded callback received: %.2f ms"), LoadTimeMs);
	AsyncTask(ENamedThreads::GameThread, [LoadTimeMs]() {
		OnPageLoaded.Broadcast(LoadTimeMs);
	});
}

// iOS Callback bridge functions (called from StashPayCardWrapper.mm)
#if PLATFORM_IOS
extern "C" {
	void StashPayOnPaymentSuccess()
	{
		UMobileNativeCodeBlueprint::HandlePaymentSuccess();
	}
	
	void StashPayOnPaymentFailure()
	{
		UMobileNativeCodeBlueprint::HandlePaymentFailure();
	}
	
	void StashPayOnDialogDismissed()
	{
		UMobileNativeCodeBlueprint::HandleDialogDismissed();
	}
	
	void StashPayOnPageLoaded(double loadTimeMs)
	{
		UMobileNativeCodeBlueprint::HandlePageLoaded((float)loadTimeMs);
	}
}
#endif

// Android JNI Callback functions (called from StashPayHelper.java)
#if PLATFORM_ANDROID
extern "C" {
	JNIEXPORT void JNICALL Java_com_Plugins_MobileNativeCode_StashPayHelper_nativeOnPaymentSuccess(JNIEnv* env, jclass clazz)
	{
		UMobileNativeCodeBlueprint::HandlePaymentSuccess();
	}
	
	JNIEXPORT void JNICALL Java_com_Plugins_MobileNativeCode_StashPayHelper_nativeOnPaymentFailure(JNIEnv* env, jclass clazz)
	{
		UMobileNativeCodeBlueprint::HandlePaymentFailure();
	}
	
	JNIEXPORT void JNICALL Java_com_Plugins_MobileNativeCode_StashPayHelper_nativeOnDialogDismissed(JNIEnv* env, jclass clazz)
	{
		UMobileNativeCodeBlueprint::HandleDialogDismissed();
	}
	
	JNIEXPORT void JNICALL Java_com_Plugins_MobileNativeCode_StashPayHelper_nativeOnPageLoaded(JNIEnv* env, jclass clazz, jlong loadTimeMs)
	{
		UMobileNativeCodeBlueprint::HandlePageLoaded((float)loadTimeMs);
	}
}
#endif
