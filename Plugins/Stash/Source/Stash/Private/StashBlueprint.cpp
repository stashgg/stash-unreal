// Copyright Stash. All Rights Reserved.
//
// Blueprint-facing Stash API (UStashBlueprint): open card/modal/browser, configs, and Android backdrop helpers.
// Native callbacks and subsystem resolution live in StashBlueprintCallbacks.cpp.
// Viewport capture implementation lives in StashAndroidBackdropCapture.cpp.

#include "StashBlueprint.h"
#include "Stash.h"
#include "StashAndroidBackdropCapture.h"

#if PLATFORM_ANDROID
#include "Android/Utils/AndroidUtils.h"
#endif

#if PLATFORM_IOS
#include "IOS/ObjC/StashNativeCardWrapper.h"
#endif

namespace
{
	constexpr float StashRatioMin = 0.1f;
	constexpr float StashRatioMax = 1.0f;

	float ClampStashRatio(float Ratio)
	{
		return FMath::Clamp(Ratio, StashRatioMin, StashRatioMax);
	}

	void ClampCardConfigRatios(FStashCardConfig& Config)
	{
		Config.CardHeightRatioPortrait = ClampStashRatio(Config.CardHeightRatioPortrait);
		Config.CardWidthRatioLandscape = ClampStashRatio(Config.CardWidthRatioLandscape);
		Config.CardHeightRatioLandscape = ClampStashRatio(Config.CardHeightRatioLandscape);
		Config.TabletWidthRatioPortrait = ClampStashRatio(Config.TabletWidthRatioPortrait);
		Config.TabletHeightRatioPortrait = ClampStashRatio(Config.TabletHeightRatioPortrait);
		Config.TabletWidthRatioLandscape = ClampStashRatio(Config.TabletWidthRatioLandscape);
		Config.TabletHeightRatioLandscape = ClampStashRatio(Config.TabletHeightRatioLandscape);
	}

	void ClampModalConfigRatios(FStashModalConfig& Config)
	{
		Config.PhoneWidthRatioPortrait = ClampStashRatio(Config.PhoneWidthRatioPortrait);
		Config.PhoneHeightRatioPortrait = ClampStashRatio(Config.PhoneHeightRatioPortrait);
		Config.PhoneWidthRatioLandscape = ClampStashRatio(Config.PhoneWidthRatioLandscape);
		Config.PhoneHeightRatioLandscape = ClampStashRatio(Config.PhoneHeightRatioLandscape);
		Config.TabletWidthRatioPortrait = ClampStashRatio(Config.TabletWidthRatioPortrait);
		Config.TabletHeightRatioPortrait = ClampStashRatio(Config.TabletHeightRatioPortrait);
		Config.TabletWidthRatioLandscape = ClampStashRatio(Config.TabletWidthRatioLandscape);
		Config.TabletHeightRatioLandscape = ClampStashRatio(Config.TabletHeightRatioLandscape);
	}
}

// ---------------------------------------------------------------------------
// Config factories (Blueprint "Make" nodes)
// ---------------------------------------------------------------------------

FStashCardConfig UStashBlueprint::MakeStashCardConfig(
	bool bForcePortrait,
	float CardHeightRatioPortrait,
	float CardWidthRatioLandscape,
	float CardHeightRatioLandscape,
	float TabletWidthRatioPortrait,
	float TabletHeightRatioPortrait,
	float TabletWidthRatioLandscape,
	float TabletHeightRatioLandscape,
	FString BackgroundColor)
{
	FStashCardConfig Config;
	Config.bForcePortrait = bForcePortrait;
	Config.BackgroundColor = MoveTemp(BackgroundColor);
	Config.CardHeightRatioPortrait = ClampStashRatio(CardHeightRatioPortrait);
	Config.CardWidthRatioLandscape = ClampStashRatio(CardWidthRatioLandscape);
	Config.CardHeightRatioLandscape = ClampStashRatio(CardHeightRatioLandscape);
	Config.TabletWidthRatioPortrait = ClampStashRatio(TabletWidthRatioPortrait);
	Config.TabletHeightRatioPortrait = ClampStashRatio(TabletHeightRatioPortrait);
	Config.TabletWidthRatioLandscape = ClampStashRatio(TabletWidthRatioLandscape);
	Config.TabletHeightRatioLandscape = ClampStashRatio(TabletHeightRatioLandscape);
	return Config;
}

FStashModalConfig UStashBlueprint::MakeStashModalConfig(
	bool bAllowDismiss,
	float PhoneWidthRatioPortrait,
	float PhoneHeightRatioPortrait,
	float PhoneWidthRatioLandscape,
	float PhoneHeightRatioLandscape,
	float TabletWidthRatioPortrait,
	float TabletHeightRatioPortrait,
	float TabletWidthRatioLandscape,
	float TabletHeightRatioLandscape,
	FString BackgroundColor)
{
	FStashModalConfig Config;
	Config.bAllowDismiss = bAllowDismiss;
	Config.BackgroundColor = MoveTemp(BackgroundColor);
	Config.PhoneWidthRatioPortrait = ClampStashRatio(PhoneWidthRatioPortrait);
	Config.PhoneHeightRatioPortrait = ClampStashRatio(PhoneHeightRatioPortrait);
	Config.PhoneWidthRatioLandscape = ClampStashRatio(PhoneWidthRatioLandscape);
	Config.PhoneHeightRatioLandscape = ClampStashRatio(PhoneHeightRatioLandscape);
	Config.TabletWidthRatioPortrait = ClampStashRatio(TabletWidthRatioPortrait);
	Config.TabletHeightRatioPortrait = ClampStashRatio(TabletHeightRatioPortrait);
	Config.TabletWidthRatioLandscape = ClampStashRatio(TabletWidthRatioLandscape);
	Config.TabletHeightRatioLandscape = ClampStashRatio(TabletHeightRatioLandscape);
	return Config;
}

FStashKeepAliveConfig UStashBlueprint::MakeStashKeepAliveConfig(
	FString NotificationTitle,
	FString NotificationText,
	FString NotificationIconDrawableName)
{
	FStashKeepAliveConfig Config;
	Config.NotificationTitle = MoveTemp(NotificationTitle);
	Config.NotificationText = MoveTemp(NotificationText);
	Config.NotificationIconDrawableName = MoveTemp(NotificationIconDrawableName);
	return Config;
}

// ---------------------------------------------------------------------------
// Card presentation (openCard)
// ---------------------------------------------------------------------------

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
	FStashCardConfig SafeConfig = Config;
	ClampCardConfigRatios(SafeConfig);
#if PLATFORM_IOS
	UE_LOG(LogStash, Log, TEXT("[Stash] Opening card with config on iOS: %s"), *URL);
	StashNativeCardWrapper* wrapper = [StashNativeCardWrapper sharedInstance];
	NSString* bgColor = SafeConfig.BackgroundColor.IsEmpty() ? nil : SafeConfig.BackgroundColor.GetNSString();
	[wrapper openCardWithURL:URL.GetNSString()
		forcePortrait:SafeConfig.bForcePortrait
		cardHeightRatioPortrait:SafeConfig.CardHeightRatioPortrait
		cardWidthRatioLandscape:SafeConfig.CardWidthRatioLandscape
		cardHeightRatioLandscape:SafeConfig.CardHeightRatioLandscape
		tabletWidthRatioPortrait:SafeConfig.TabletWidthRatioPortrait
		tabletHeightRatioPortrait:SafeConfig.TabletHeightRatioPortrait
		tabletWidthRatioLandscape:SafeConfig.TabletWidthRatioLandscape
		tabletHeightRatioLandscape:SafeConfig.TabletHeightRatioLandscape
		backgroundColor:bgColor];
#elif PLATFORM_ANDROID
	UE_LOG(LogStash, Log, TEXT("[Stash] Opening card with config on Android: %s"), *URL);
	const int32 BackdropLen = SafeConfig.AndroidCheckoutBackdrop.Num();
	UE_LOG(LogStash, Log, TEXT("[StashBackdrop] OpenCardWithConfig: forcePortrait=%d backdropBytes=%d"),
		SafeConfig.bForcePortrait ? 1 : 0, BackdropLen);
	AndroidUtils::CallJavaCode<void>(
		"com/Plugins/Stash/StashHelper",
		"OpenCardWithConfig",
		"",
		true,
		URL,
		SafeConfig.bForcePortrait,
		SafeConfig.CardHeightRatioPortrait,
		SafeConfig.CardWidthRatioLandscape,
		SafeConfig.CardHeightRatioLandscape,
		SafeConfig.TabletWidthRatioPortrait,
		SafeConfig.TabletHeightRatioPortrait,
		SafeConfig.TabletWidthRatioLandscape,
		SafeConfig.TabletHeightRatioLandscape,
		SafeConfig.BackgroundColor,
		SafeConfig.AndroidCheckoutBackdrop
	);
#else
	UE_LOG(LogStash, Warning, TEXT("[Stash] OpenCardWithConfig called on unsupported platform"));
#endif
}

// ---------------------------------------------------------------------------
// Card / modal state
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// System browser (openBrowser / closeBrowser)
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// Modal presentation (openModal)
// ---------------------------------------------------------------------------

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
		true,
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
	FStashModalConfig SafeConfig = Config;
	ClampModalConfigRatios(SafeConfig);
#if PLATFORM_IOS
	UE_LOG(LogStash, Log, TEXT("[Stash] Opening modal with config on iOS: %s"), *URL);
	NSString* modalBg = SafeConfig.BackgroundColor.IsEmpty() ? nil : SafeConfig.BackgroundColor.GetNSString();
	[[StashNativeCardWrapper sharedInstance] openModalWithURL:URL.GetNSString()
		allowDismiss:SafeConfig.bAllowDismiss
		phoneWidthRatioPortrait:SafeConfig.PhoneWidthRatioPortrait
		phoneHeightRatioPortrait:SafeConfig.PhoneHeightRatioPortrait
		phoneWidthRatioLandscape:SafeConfig.PhoneWidthRatioLandscape
		phoneHeightRatioLandscape:SafeConfig.PhoneHeightRatioLandscape
		tabletWidthRatioPortrait:SafeConfig.TabletWidthRatioPortrait
		tabletHeightRatioPortrait:SafeConfig.TabletHeightRatioPortrait
		tabletWidthRatioLandscape:SafeConfig.TabletWidthRatioLandscape
		tabletHeightRatioLandscape:SafeConfig.TabletHeightRatioLandscape
		backgroundColor:modalBg];
#elif PLATFORM_ANDROID
	UE_LOG(LogStash, Log, TEXT("[Stash] Opening modal with config on Android: %s"), *URL);
	AndroidUtils::CallJavaCode<void>(
		"com/Plugins/Stash/StashHelper",
		"OpenModalWithConfig",
		"",
		true,
		URL,
		SafeConfig.bAllowDismiss,
		SafeConfig.PhoneWidthRatioPortrait,
		SafeConfig.PhoneHeightRatioPortrait,
		SafeConfig.PhoneWidthRatioLandscape,
		SafeConfig.PhoneHeightRatioLandscape,
		SafeConfig.TabletWidthRatioPortrait,
		SafeConfig.TabletHeightRatioPortrait,
		SafeConfig.TabletWidthRatioLandscape,
		SafeConfig.TabletHeightRatioLandscape,
		SafeConfig.BackgroundColor
	);
#else
	UE_LOG(LogStash, Warning, TEXT("[Stash] OpenModalWithConfig called on unsupported platform"));
#endif
}

// ---------------------------------------------------------------------------
// Platform configuration (iOS orientation lock, Android keep-alive)
// ---------------------------------------------------------------------------

void UStashBlueprint::SetLandscapeLockWhenCardClosed(bool bEnable)
{
#if PLATFORM_IOS
	[[StashNativeCardWrapper sharedInstance] setLandscapeLockWhenCardClosed:bEnable];
#else
	(void)bEnable;
#endif
}

void UStashBlueprint::SetAndroidKeepAliveEnabled(bool bEnabled)
{
#if PLATFORM_ANDROID
	AndroidUtils::CallJavaCode<void>(
		"com/Plugins/Stash/StashHelper",
		"SetKeepAliveEnabled",
		"",
		true,
		bEnabled
	);
#else
	UE_LOG(LogStash, Log, TEXT("[Stash] SetAndroidKeepAliveEnabled: no-op on this platform"));
	(void)bEnabled;
#endif
}

void UStashBlueprint::SetAndroidKeepAliveConfig(const FStashKeepAliveConfig& Config)
{
#if PLATFORM_ANDROID
	AndroidUtils::CallJavaCode<void>(
		"com/Plugins/Stash/StashHelper",
		"SetKeepAliveConfig",
		"",
		true,
		Config.NotificationTitle,
		Config.NotificationText,
		Config.NotificationIconDrawableName
	);
#else
	UE_LOG(LogStash, Log, TEXT("[Stash] SetAndroidKeepAliveConfig: no-op on this platform"));
#endif
}

// ---------------------------------------------------------------------------
// Android checkout backdrop (setBackdropBitmap path; see StashAndroidBackdropCapture for capture)
// ---------------------------------------------------------------------------

void UStashBlueprint::SetAndroidCheckoutBackdropBytes(const TArray<uint8>& ImageBytes)
{
#if PLATFORM_ANDROID
	if (!AndroidUtils::isSupportPlatform())
	{
		UE_LOG(LogStash, Warning, TEXT("[StashBackdrop] SetAndroidCheckoutBackdropBytes: AndroidUtils platform not ready (JNI init failed?)"));
		return;
	}
	UE_LOG(LogStash, Log, TEXT("[StashBackdrop] SetAndroidCheckoutBackdropBytes: forwarding %d bytes to Java"), ImageBytes.Num());
	AndroidUtils::CallJavaCode<void>(
		"com/Plugins/Stash/StashHelper",
		"SetCheckoutBackdropBytes",
		"",
		true,
		ImageBytes
	);
#else
	(void)ImageBytes;
#endif
}

void UStashBlueprint::ClearAndroidCheckoutBackdrop()
{
#if PLATFORM_ANDROID
	if (!AndroidUtils::isSupportPlatform())
	{
		UE_LOG(LogStash, Warning, TEXT("[StashBackdrop] ClearAndroidCheckoutBackdrop: AndroidUtils platform not ready"));
		return;
	}
	UE_LOG(LogStash, Log, TEXT("[StashBackdrop] ClearAndroidCheckoutBackdrop → Java"));
	AndroidUtils::CallJavaCode<void>(
		"com/Plugins/Stash/StashHelper",
		"ClearCheckoutBackdrop",
		"",
		true
	);
#endif
}

// Thin wrappers; capture logic is in StashAndroidBackdropCapture.cpp (render-thread readback + JPEG).

void UStashBlueprint::CaptureViewportForAndroidCheckoutBackdrop(UObject* WorldContextObject, FOnStashViewportCaptureComplete OnComplete)
{
	StashScheduleAndroidCheckoutBackdropCapture(WorldContextObject, [OnComplete](TArray<uint8> Bytes) mutable
		{
			OnComplete.ExecuteIfBound(Bytes);
		});
}

void UStashBlueprint::CaptureViewportForAndroidCheckoutBackdropLatent(UObject* WorldContextObject, TArray<uint8>& OutImageBytes, FLatentActionInfo LatentInfo)
{
	StashCaptureAndroidCheckoutBackdropLatent(WorldContextObject, OutImageBytes, LatentInfo);
}
