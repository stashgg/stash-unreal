// Copyright Stash. All Rights Reserved.
// Stash Pay Unreal Engine SDK - Blueprint Function Library Implementation

#include "StashBlueprint.h"
#include "Stash.h"
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
FOnStashPaymentSuccess UStashBlueprint::OnPaymentSuccess;
FOnStashPaymentFailure UStashBlueprint::OnPaymentFailure;
FOnStashDialogDismissed UStashBlueprint::OnDialogDismissed;
FOnStashPageLoaded UStashBlueprint::OnPageLoaded;

void UStashBlueprint::OpenCheckout(const FString& CheckoutURL)
{
#if PLATFORM_IOS
	UE_LOG(LogTemp, Log, TEXT("[Stash] Opening checkout on iOS: %s"), *CheckoutURL);
	[[StashPayCardWrapper sharedInstance] openCheckoutWithURL:CheckoutURL.GetNSString()];
#elif PLATFORM_ANDROID
	UE_LOG(LogTemp, Log, TEXT("[Stash] Opening checkout on Android: %s"), *CheckoutURL);
	AndroidUtils::CallJavaCode<void>(
		"com/Plugins/Stash/StashHelper",
		"OpenCheckout",
		"",
		true,  // Pass activity
		CheckoutURL
	);
#else
	UE_LOG(LogTemp, Warning, TEXT("[Stash] OpenCheckout called on unsupported platform"));
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
	UE_LOG(LogTemp, Log, TEXT("[Stash] Dismissing checkout on iOS"));
	[[StashPayCardWrapper sharedInstance] dismissCheckout];
#elif PLATFORM_ANDROID
	UE_LOG(LogTemp, Log, TEXT("[Stash] Dismissing checkout on Android"));
	AndroidUtils::CallJavaCode<void>(
		"com/Plugins/Stash/StashHelper",
		"DismissCheckout",
		"",
		true  // Pass activity
	);
#endif
}

// Callback handlers - called from native code
void UStashBlueprint::HandlePaymentSuccess()
{
	UE_LOG(LogTemp, Log, TEXT("[Stash] Payment success callback received"));
	AsyncTask(ENamedThreads::GameThread, []() {
		OnPaymentSuccess.Broadcast();
	});
}

void UStashBlueprint::HandlePaymentFailure()
{
	UE_LOG(LogTemp, Log, TEXT("[Stash] Payment failure callback received"));
	AsyncTask(ENamedThreads::GameThread, []() {
		OnPaymentFailure.Broadcast();
	});
}

void UStashBlueprint::HandleDialogDismissed()
{
	UE_LOG(LogTemp, Log, TEXT("[Stash] Dialog dismissed callback received"));
	AsyncTask(ENamedThreads::GameThread, []() {
		OnDialogDismissed.Broadcast();
	});
}

void UStashBlueprint::HandlePageLoaded(float LoadTimeMs)
{
	UE_LOG(LogTemp, Log, TEXT("[Stash] Page loaded callback received: %.2f ms"), LoadTimeMs);
	AsyncTask(ENamedThreads::GameThread, [LoadTimeMs]() {
		OnPageLoaded.Broadcast(LoadTimeMs);
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
	
	void StashPayOnPageLoaded(double loadTimeMs)
	{
		UStashBlueprint::HandlePageLoaded((float)loadTimeMs);
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
	
	JNIEXPORT void JNICALL Java_com_Plugins_Stash_StashHelper_nativeOnPageLoaded(JNIEnv* env, jclass clazz, jlong loadTimeMs)
	{
		UStashBlueprint::HandlePageLoaded((float)loadTimeMs);
	}
}
#endif
