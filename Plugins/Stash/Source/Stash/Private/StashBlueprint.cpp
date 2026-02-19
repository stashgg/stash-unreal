// Copyright Stash. All Rights Reserved.
// Stash Unreal Engine SDK - Blueprint Function Library Implementation

#include "StashBlueprint.h"
#include "Stash.h"
#include "StashSubsystem.h"
#include "Async/Async.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"

#if PLATFORM_ANDROID
#include "Android/Utils/AndroidUtils.h"
#endif

#if PLATFORM_IOS
#include "IOS/Utils/ObjC_Convert.h"
#include "IOS/ObjC/StashNativeCardWrapper.h"
#endif

// Initialize static delegates
FOnStashPaymentSuccess UStashBlueprint::OnPaymentSuccess;
FOnStashPaymentFailure UStashBlueprint::OnPaymentFailure;
FOnStashDialogDismissed UStashBlueprint::OnDialogDismissed;
FOnStashOptInResponse UStashBlueprint::OnOptInResponse;
FOnStashPageLoaded UStashBlueprint::OnPageLoaded;
FOnStashNetworkError UStashBlueprint::OnNetworkError;

FStashCardConfig UStashBlueprint::MakeStashCardConfig(
	bool bForcePortrait,
	float CardHeightRatioPortrait,
	float CardWidthRatioLandscape,
	float CardHeightRatioLandscape,
	float TabletWidthRatioPortrait,
	float TabletHeightRatioPortrait,
	float TabletWidthRatioLandscape,
	float TabletHeightRatioLandscape)
{
	FStashCardConfig Config;
	Config.bForcePortrait = bForcePortrait;
	Config.CardHeightRatioPortrait = FMath::Clamp(CardHeightRatioPortrait, 0.1f, 1.0f);
	Config.CardWidthRatioLandscape = FMath::Clamp(CardWidthRatioLandscape, 0.1f, 1.0f);
	Config.CardHeightRatioLandscape = FMath::Clamp(CardHeightRatioLandscape, 0.1f, 1.0f);
	Config.TabletWidthRatioPortrait = FMath::Clamp(TabletWidthRatioPortrait, 0.1f, 1.0f);
	Config.TabletHeightRatioPortrait = FMath::Clamp(TabletHeightRatioPortrait, 0.1f, 1.0f);
	Config.TabletWidthRatioLandscape = FMath::Clamp(TabletWidthRatioLandscape, 0.1f, 1.0f);
	Config.TabletHeightRatioLandscape = FMath::Clamp(TabletHeightRatioLandscape, 0.1f, 1.0f);
	return Config;
}

void UStashBlueprint::OpenCard(const FString& URL)
{
	if (URL.IsEmpty())
	{
		UE_LOG(LogStash, Warning, TEXT("[Stash] OpenCard called with empty URL"));
		return;
	}
#if PLATFORM_IOS
	UE_LOG(LogStash, Log, TEXT("[Stash] Opening card on iOS: %s"), *URL);
	[[StashNativeCardWrapper sharedInstance] openCardWithURL:URL.GetNSString()];
#elif PLATFORM_ANDROID
	UE_LOG(LogStash, Log, TEXT("[Stash] Opening card on Android: %s"), *URL);
	AndroidUtils::CallJavaCode<void>(
		"com/Plugins/Stash/StashHelper",
		"OpenCard",
		"",
		true,
		URL
	);
#else
	UE_LOG(LogStash, Warning, TEXT("[Stash] OpenCard called on unsupported platform"));
#endif
}

void UStashBlueprint::OpenCardWithConfig(const FString& URL, const FStashCardConfig& Config)
{
	if (URL.IsEmpty())
	{
		UE_LOG(LogStash, Warning, TEXT("[Stash] OpenCardWithConfig called with empty URL"));
		return;
	}
#if PLATFORM_IOS
	UE_LOG(LogStash, Log, TEXT("[Stash] Opening card with config on iOS: %s"), *URL);
	StashNativeCardWrapper* wrapper = [StashNativeCardWrapper sharedInstance];
	[wrapper openCardWithURL:URL.GetNSString()
		forcePortrait:Config.bForcePortrait
		cardHeightRatioPortrait:FMath::Clamp(Config.CardHeightRatioPortrait, 0.1f, 1.0f)
		cardWidthRatioLandscape:FMath::Clamp(Config.CardWidthRatioLandscape, 0.1f, 1.0f)
		cardHeightRatioLandscape:FMath::Clamp(Config.CardHeightRatioLandscape, 0.1f, 1.0f)
		tabletWidthRatioPortrait:FMath::Clamp(Config.TabletWidthRatioPortrait, 0.1f, 1.0f)
		tabletHeightRatioPortrait:FMath::Clamp(Config.TabletHeightRatioPortrait, 0.1f, 1.0f)
		tabletWidthRatioLandscape:FMath::Clamp(Config.TabletWidthRatioLandscape, 0.1f, 1.0f)
		tabletHeightRatioLandscape:FMath::Clamp(Config.TabletHeightRatioLandscape, 0.1f, 1.0f)];
#elif PLATFORM_ANDROID
	UE_LOG(LogStash, Log, TEXT("[Stash] Opening card with config on Android: %s"), *URL);
	AndroidUtils::CallJavaCode<void>(
		"com/Plugins/Stash/StashHelper",
		"OpenCardWithConfig",
		"",
		true,
		URL,
		Config.bForcePortrait,
		FMath::Clamp(Config.CardHeightRatioPortrait, 0.1f, 1.0f),
		FMath::Clamp(Config.CardWidthRatioLandscape, 0.1f, 1.0f),
		FMath::Clamp(Config.CardHeightRatioLandscape, 0.1f, 1.0f),
		FMath::Clamp(Config.TabletWidthRatioPortrait, 0.1f, 1.0f),
		FMath::Clamp(Config.TabletHeightRatioPortrait, 0.1f, 1.0f),
		FMath::Clamp(Config.TabletWidthRatioLandscape, 0.1f, 1.0f),
		FMath::Clamp(Config.TabletHeightRatioLandscape, 0.1f, 1.0f)
	);
#else
	UE_LOG(LogStash, Warning, TEXT("[Stash] OpenCardWithConfig called on unsupported platform"));
#endif
}

bool UStashBlueprint::IsCardOpen()
{
#if PLATFORM_IOS
	return [[StashNativeCardWrapper sharedInstance] isCardOpen];
#elif PLATFORM_ANDROID
	return AndroidUtils::CallJavaCode<bool>(
		"com/Plugins/Stash/StashHelper",
		"IsCardOpen",
		"",
		false
	);
#else
	return false;
#endif
}

bool UStashBlueprint::IsPurchaseProcessing()
{
#if PLATFORM_IOS
	return [[StashNativeCardWrapper sharedInstance] isPurchaseProcessing];
#elif PLATFORM_ANDROID
	return AndroidUtils::CallJavaCode<bool>(
		"com/Plugins/Stash/StashHelper",
		"IsPurchaseProcessing",
		"",
		false
	);
#else
	return false;
#endif
}

void UStashBlueprint::DismissCard()
{
#if PLATFORM_IOS
	UE_LOG(LogStash, Log, TEXT("[Stash] Dismissing card on iOS"));
	[[StashNativeCardWrapper sharedInstance] dismissCard];
#elif PLATFORM_ANDROID
	UE_LOG(LogStash, Log, TEXT("[Stash] Dismissing card on Android"));
	AndroidUtils::CallJavaCode<void>(
		"com/Plugins/Stash/StashHelper",
		"DismissCard",
		"",
		true
	);
#endif
}

void UStashBlueprint::OpenBrowser(const FString& URL)
{
	if (URL.IsEmpty())
	{
		UE_LOG(LogStash, Warning, TEXT("[Stash] OpenBrowser called with empty URL"));
		return;
	}
#if PLATFORM_IOS
	UE_LOG(LogStash, Log, TEXT("[Stash] Opening browser on iOS: %s"), *URL);
	[[StashNativeCardWrapper sharedInstance] openBrowserWithURL:URL.GetNSString()];
#elif PLATFORM_ANDROID
	UE_LOG(LogStash, Log, TEXT("[Stash] Opening browser on Android: %s"), *URL);
	AndroidUtils::CallJavaCode<void>(
		"com/Plugins/Stash/StashHelper",
		"OpenBrowser",
		"",
		true,
		URL
	);
#else
	UE_LOG(LogStash, Warning, TEXT("[Stash] OpenBrowser called on unsupported platform"));
#endif
}

void UStashBlueprint::CloseBrowser()
{
#if PLATFORM_IOS
	UE_LOG(LogStash, Log, TEXT("[Stash] Closing browser on iOS"));
	[[StashNativeCardWrapper sharedInstance] closeBrowser];
#elif PLATFORM_ANDROID
	// No-op on Android (Chrome Custom Tabs cannot be closed by the app)
#else
	UE_LOG(LogStash, Warning, TEXT("[Stash] CloseBrowser called on unsupported platform"));
#endif
}

// ============================================================================
// Modal Presentation (Stash Native 2.0)
// ============================================================================

void UStashBlueprint::OpenModal(const FString& URL)
{
	if (URL.IsEmpty())
	{
		UE_LOG(LogStash, Warning, TEXT("[Stash] OpenModal called with empty URL"));
		return;
	}
#if PLATFORM_IOS
	UE_LOG(LogStash, Log, TEXT("[Stash] Opening modal on iOS: %s"), *URL);
	[[StashNativeCardWrapper sharedInstance] openModalWithURL:URL.GetNSString()];
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
	if (URL.IsEmpty())
	{
		UE_LOG(LogStash, Warning, TEXT("[Stash] OpenModalWithConfig called with empty URL"));
		return;
	}
#if PLATFORM_IOS
	UE_LOG(LogStash, Log, TEXT("[Stash] Opening modal with config on iOS: %s"), *URL);
	[[StashNativeCardWrapper sharedInstance] openModalWithURL:URL.GetNSString()
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
// Configuration (Stash Native 2.0)
// ============================================================================

void UStashBlueprint::SetLandscapeLockWhenCardClosed(bool bEnable)
{
#if PLATFORM_IOS
	[[StashNativeCardWrapper sharedInstance] setLandscapeLockWhenCardClosed:bEnable];
#else
	// Android: no-op; orientation lock is handled by project/activity settings
	(void)bEnable;
#endif
}

/** Resolves Stash subsystem from an optional world context. Used by GetStashSubsystem and by native callbacks. */
static UStashSubsystem* GetStashSubsystemFromContext(UObject* WorldContextObject)
{
	UWorld* World = nullptr;
	if (WorldContextObject)
	{
		if (UWorld* W = Cast<UWorld>(WorldContextObject))
		{
			World = W;
		}
		else if (AActor* A = Cast<AActor>(WorldContextObject))
		{
			World = A->GetWorld();
		}
		else if (APlayerController* PC = Cast<APlayerController>(WorldContextObject))
		{
			World = PC->GetWorld();
		}
		else if (UGameInstance* GI = Cast<UGameInstance>(WorldContextObject))
		{
			if (FWorldContext* const Ctx = GI->GetWorldContext())
			{
				World = Ctx->World();
			}
		}
	}
	if (!World && GEngine)
	{
		World = GEngine->GetCurrentPlayWorld();
	}
	UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	return GI ? GI->GetSubsystem<UStashSubsystem>() : nullptr;
}

UStashSubsystem* UStashBlueprint::GetStashSubsystem(UObject* WorldContextObject)
{
	return GetStashSubsystemFromContext(WorldContextObject);
}

void UStashBlueprint::HandlePaymentSuccess()
{
	UE_LOG(LogStash, Log, TEXT("[Stash] Payment success callback received"));
	AsyncTask(ENamedThreads::GameThread, []() {
		if (UStashSubsystem* Sub = GetStashSubsystemFromContext(nullptr)) { Sub->OnPaymentSuccess.Broadcast(); }
		OnPaymentSuccess.Broadcast();
	});
}

void UStashBlueprint::HandlePaymentFailure()
{
	UE_LOG(LogStash, Log, TEXT("[Stash] Payment failure callback received"));
	AsyncTask(ENamedThreads::GameThread, []() {
		if (UStashSubsystem* Sub = GetStashSubsystemFromContext(nullptr)) { Sub->OnPaymentFailure.Broadcast(); }
		OnPaymentFailure.Broadcast();
	});
}

void UStashBlueprint::HandleDialogDismissed()
{
	UE_LOG(LogStash, Log, TEXT("[Stash] Dialog dismissed callback received"));
	AsyncTask(ENamedThreads::GameThread, []() {
		if (UStashSubsystem* Sub = GetStashSubsystemFromContext(nullptr)) { Sub->OnDialogDismissed.Broadcast(); }
		OnDialogDismissed.Broadcast();
	});
}

void UStashBlueprint::HandleOptInResponse(const FString& OptInType)
{
	UE_LOG(LogStash, Log, TEXT("[Stash] Opt-in response received: %s"), *OptInType);
	AsyncTask(ENamedThreads::GameThread, [OptInType]() {
		if (UStashSubsystem* Sub = GetStashSubsystemFromContext(nullptr)) { Sub->OnOptInResponse.Broadcast(OptInType); }
		OnOptInResponse.Broadcast(OptInType);
	});
}

void UStashBlueprint::HandlePageLoaded(float LoadTimeMs)
{
	UE_LOG(LogStash, Log, TEXT("[Stash] Page loaded callback received: %.2f ms"), LoadTimeMs);
	AsyncTask(ENamedThreads::GameThread, [LoadTimeMs]() {
		if (UStashSubsystem* Sub = GetStashSubsystemFromContext(nullptr)) { Sub->OnPageLoaded.Broadcast(LoadTimeMs); }
		OnPageLoaded.Broadcast(LoadTimeMs);
	});
}

void UStashBlueprint::HandleNetworkError()
{
	UE_LOG(LogStash, Warning, TEXT("[Stash] Network error callback received"));
	AsyncTask(ENamedThreads::GameThread, []() {
		if (UStashSubsystem* Sub = GetStashSubsystemFromContext(nullptr)) { Sub->OnNetworkError.Broadcast(); }
		OnNetworkError.Broadcast();
	});
}

// iOS callback bridge (called from StashNativeCardWrapper.mm)
#if PLATFORM_IOS
extern "C" {
	void StashNativeOnPaymentSuccess()
	{
		UStashBlueprint::HandlePaymentSuccess();
	}

	void StashNativeOnPaymentFailure()
	{
		UStashBlueprint::HandlePaymentFailure();
	}

	void StashNativeOnDialogDismissed()
	{
		UStashBlueprint::HandleDialogDismissed();
	}

	void StashNativeOnOptInResponse(const char* optinType)
	{
		UStashBlueprint::HandleOptInResponse(FString(UTF8_TO_TCHAR(optinType ? optinType : "")));
	}

	void StashNativeOnPageLoaded(double loadTimeMs)
	{
		UStashBlueprint::HandlePageLoaded((float)loadTimeMs);
	}

	void StashNativeOnNetworkError()
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
