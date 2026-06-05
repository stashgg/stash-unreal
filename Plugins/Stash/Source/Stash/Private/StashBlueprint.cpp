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
#include "LatentActions.h"
#include "TimerManager.h"

#if PLATFORM_ANDROID
#include "Android/Utils/AndroidUtils.h"
#include "Engine/GameViewportClient.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Modules/ModuleManager.h"
#include "RHI.h"
#include "RHICommandList.h"
#include "RenderingThread.h"
#include "Slate/SceneViewport.h"
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
FOnStashExternalPayment UStashBlueprint::OnExternalPayment;

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
	Config.CardHeightRatioPortrait = FMath::Clamp(CardHeightRatioPortrait, 0.1f, 1.0f);
	Config.CardWidthRatioLandscape = FMath::Clamp(CardWidthRatioLandscape, 0.1f, 1.0f);
	Config.CardHeightRatioLandscape = FMath::Clamp(CardHeightRatioLandscape, 0.1f, 1.0f);
	Config.TabletWidthRatioPortrait = FMath::Clamp(TabletWidthRatioPortrait, 0.1f, 1.0f);
	Config.TabletHeightRatioPortrait = FMath::Clamp(TabletHeightRatioPortrait, 0.1f, 1.0f);
	Config.TabletWidthRatioLandscape = FMath::Clamp(TabletWidthRatioLandscape, 0.1f, 1.0f);
	Config.TabletHeightRatioLandscape = FMath::Clamp(TabletHeightRatioLandscape, 0.1f, 1.0f);
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
	NSString* bgColor = Config.BackgroundColor.IsEmpty() ? nil : Config.BackgroundColor.GetNSString();
	[wrapper openCardWithURL:URL.GetNSString()
		forcePortrait:Config.bForcePortrait
		cardHeightRatioPortrait:FMath::Clamp(Config.CardHeightRatioPortrait, 0.1f, 1.0f)
		cardWidthRatioLandscape:FMath::Clamp(Config.CardWidthRatioLandscape, 0.1f, 1.0f)
		cardHeightRatioLandscape:FMath::Clamp(Config.CardHeightRatioLandscape, 0.1f, 1.0f)
		tabletWidthRatioPortrait:FMath::Clamp(Config.TabletWidthRatioPortrait, 0.1f, 1.0f)
		tabletHeightRatioPortrait:FMath::Clamp(Config.TabletHeightRatioPortrait, 0.1f, 1.0f)
		tabletWidthRatioLandscape:FMath::Clamp(Config.TabletWidthRatioLandscape, 0.1f, 1.0f)
		tabletHeightRatioLandscape:FMath::Clamp(Config.TabletHeightRatioLandscape, 0.1f, 1.0f)
		backgroundColor:bgColor];
#elif PLATFORM_ANDROID
	UE_LOG(LogStash, Log, TEXT("[Stash] Opening card with config on Android: %s"), *URL);
	const int32 BackdropLen = Config.AndroidCheckoutBackdrop.Num();
	UE_LOG(LogStash, Log, TEXT("[StashBackdrop] OpenCardWithConfig: forcePortrait=%d backdropBytes=%d"),
		Config.bForcePortrait ? 1 : 0, BackdropLen);
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
		FMath::Clamp(Config.TabletHeightRatioLandscape, 0.1f, 1.0f),
		Config.BackgroundColor,
		Config.AndroidCheckoutBackdrop
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
	NSString* modalBg = Config.BackgroundColor.IsEmpty() ? nil : Config.BackgroundColor.GetNSString();
	[[StashNativeCardWrapper sharedInstance] openModalWithURL:URL.GetNSString()
		allowDismiss:Config.bAllowDismiss
		phoneWidthRatioPortrait:Config.PhoneWidthRatioPortrait
		phoneHeightRatioPortrait:Config.PhoneHeightRatioPortrait
		phoneWidthRatioLandscape:Config.PhoneWidthRatioLandscape
		phoneHeightRatioLandscape:Config.PhoneHeightRatioLandscape
		tabletWidthRatioPortrait:Config.TabletWidthRatioPortrait
		tabletHeightRatioPortrait:Config.TabletHeightRatioPortrait
		tabletWidthRatioLandscape:Config.TabletWidthRatioLandscape
		tabletHeightRatioLandscape:Config.TabletHeightRatioLandscape
		backgroundColor:modalBg];
#elif PLATFORM_ANDROID
	UE_LOG(LogStash, Log, TEXT("[Stash] Opening modal with config on Android: %s"), *URL);
	AndroidUtils::CallJavaCode<void>(
		"com/Plugins/Stash/StashHelper",
		"OpenModalWithConfig",
		"",
		true,  // Pass activity
		URL,
		Config.bAllowDismiss,
		Config.PhoneWidthRatioPortrait,
		Config.PhoneHeightRatioPortrait,
		Config.PhoneWidthRatioLandscape,
		Config.PhoneHeightRatioLandscape,
		Config.TabletWidthRatioPortrait,
		Config.TabletHeightRatioPortrait,
		Config.TabletWidthRatioLandscape,
		Config.TabletHeightRatioLandscape,
		Config.BackgroundColor
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

namespace
{
#if PLATFORM_ANDROID
	static constexpr int32 StashBackdropMaxCaptureAttempts = 15;

	struct FStashCaptureRetryState
	{
		TWeakObjectPtr<UWorld> WeakWorld;
		TFunction<void(TArray<uint8>)> OnDone;
		int32 AttemptIndex = 0;
	};

	static void ScheduleViewportCaptureAttempt(TSharedRef<FStashCaptureRetryState> State);

	static TArray<uint8> EncodeViewportBitmapToJpeg(const TArray<FColor>& Bitmap, const FIntPoint& Size)
	{
		TArray<uint8> OutBytes;
		if (Bitmap.Num() == 0 || Size.X <= 0 || Size.Y <= 0)
		{
			return OutBytes;
		}
		IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
		TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::JPEG);
		if (!ImageWrapper.IsValid() || !ImageWrapper->SetRaw(Bitmap.GetData(), Bitmap.Num() * sizeof(FColor), Size.X, Size.Y, ERGBFormat::BGRA, 8))
		{
			UE_LOG(LogStash, Warning, TEXT("[StashBackdrop] CaptureViewport: JPEG wrap failed (bitmap pixels=%d)"), Bitmap.Num());
			return OutBytes;
		}
		const TArray64<uint8>& Compressed = ImageWrapper->GetCompressed(85);
		OutBytes.Append(Compressed.GetData(), static_cast<int32>(Compressed.Num()));
		return OutBytes;
	}
#endif

	/** Read back the game viewport on the render thread (safe for Android Vulkan). Callback runs on game thread. */
	static void CaptureViewportToJpegBytesAsync(TFunction<void(TArray<uint8>&&)> OnComplete, int32 AttemptIndex)
	{
#if PLATFORM_ANDROID
		if (!IsInGameThread())
		{
			UE_LOG(LogStash, Warning, TEXT("[StashBackdrop] CaptureViewport: must run on game thread"));
			OnComplete(TArray<uint8>());
			return;
		}
		if (!GEngine || !GEngine->GameViewport || !GEngine->GameViewport->Viewport)
		{
			UE_LOG(LogStash, Warning, TEXT("[StashBackdrop] CaptureViewport: no GameViewport"));
			OnComplete(TArray<uint8>());
			return;
		}
		FSceneViewport* SceneViewport = static_cast<FSceneViewport*>(GEngine->GameViewport->Viewport);
		const FIntPoint Size = SceneViewport->GetSizeXY();
		if (Size.X <= 0 || Size.Y <= 0)
		{
			UE_LOG(LogStash, Warning, TEXT("[StashBackdrop] CaptureViewport: bad viewport size %dx%d"), Size.X, Size.Y);
			OnComplete(TArray<uint8>());
			return;
		}

		const FViewportRHIRef ViewportRHIRef = SceneViewport->GetViewportRHI();

		struct FStashBackdropCaptureState
		{
			FIntPoint Size = FIntPoint::ZeroValue;
			int32 AttemptIndex = 0;
			TFunction<void(TArray<uint8>&&)> OnComplete;
		};
		TSharedRef<FStashBackdropCaptureState> State = MakeShared<FStashBackdropCaptureState>();
		State->Size = Size;
		State->AttemptIndex = AttemptIndex;
		State->OnComplete = MoveTemp(OnComplete);

		UE_LOG(LogStash, Log, TEXT("[StashBackdrop] CaptureViewport: enqueue readback %dx%d (attempt %d/%d, viewportRHI=%d)"),
			Size.X, Size.Y, AttemptIndex + 1, StashBackdropMaxCaptureAttempts, ViewportRHIRef.IsValid() ? 1 : 0);

		ENQUEUE_RENDER_COMMAND(StashBackdropCaptureViewport)(
			[SceneViewport, ViewportRHIRef, State](FRHICommandListImmediate& RHICmdList)
			{
				TArray<FColor> Bitmap;
				FTextureRHIRef TextureRef = SceneViewport->GetRenderTargetTexture();
				const TCHAR* TextureSource = TEXT("scene-render-target");
				if (!TextureRef.IsValid() && ViewportRHIRef.IsValid())
				{
					TextureRef = RHIGetViewportBackBuffer(ViewportRHIRef.GetReference());
					TextureSource = TEXT("swapchain-backbuffer");
				}
				if (!TextureRef.IsValid())
				{
					UE_LOG(LogStash, Verbose, TEXT("[StashBackdrop] CaptureViewport: no texture (attempt %d)"),
						State->AttemptIndex + 1);
					AsyncTask(ENamedThreads::GameThread, [State]()
						{
							State->OnComplete(TArray<uint8>());
						});
					return;
				}

				FReadSurfaceDataFlags ReadFlags;
				ReadFlags.SetLinearToGamma(false);
				const FIntRect SrcRect(0, 0, State->Size.X, State->Size.Y);
				RHICmdList.ReadSurfaceData(TextureRef, SrcRect, Bitmap, ReadFlags);

				AsyncTask(ENamedThreads::GameThread, [State, Bitmap = MoveTemp(Bitmap), TextureSource]() mutable
					{
						TArray<uint8> JpegBytes = EncodeViewportBitmapToJpeg(Bitmap, State->Size);
						if (JpegBytes.Num() > 0)
						{
							UE_LOG(LogStash, Log, TEXT("[StashBackdrop] CaptureViewport: OK %s %dx%d jpegOut=%d bytes (attempt %d)"),
								TextureSource, State->Size.X, State->Size.Y, JpegBytes.Num(), State->AttemptIndex + 1);
						}
						else
						{
							UE_LOG(LogStash, Verbose, TEXT("[StashBackdrop] CaptureViewport: readback empty from %s %dx%d (attempt %d)"),
								TextureSource, State->Size.X, State->Size.Y, State->AttemptIndex + 1);
						}
						State->OnComplete(MoveTemp(JpegBytes));
					});
			});
#else
		OnComplete(TArray<uint8>());
#endif
	}

#if PLATFORM_ANDROID
	static void ScheduleViewportCaptureAttempt(TSharedRef<FStashCaptureRetryState> State)
	{
		if (!State->WeakWorld.IsValid())
		{
			UE_LOG(LogStash, Warning, TEXT("[StashBackdrop] CaptureViewport: world gone before capture"));
			State->OnDone(TArray<uint8>());
			return;
		}

		CaptureViewportToJpegBytesAsync(
			[State](TArray<uint8>&& Bytes) mutable
			{
				if (Bytes.Num() > 0)
				{
					State->OnDone(MoveTemp(Bytes));
					return;
				}

				++State->AttemptIndex;
				if (State->AttemptIndex >= StashBackdropMaxCaptureAttempts)
				{
					UE_LOG(LogStash, Warning, TEXT("[StashBackdrop] CaptureViewport: gave up after %d attempts (null backbuffer)"),
						StashBackdropMaxCaptureAttempts);
					State->OnDone(TArray<uint8>());
					return;
				}

				if (UWorld* World = State->WeakWorld.Get())
				{
					World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([State]()
						{
							ScheduleViewportCaptureAttempt(State);
						}));
				}
				else
				{
					State->OnDone(TArray<uint8>());
				}
			},
			State->AttemptIndex);
	}
#endif

	static void ScheduleViewportCapture(UObject* WorldContextObject, TFunction<void(TArray<uint8>)> OnDone)
	{
		UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
		if (!World)
		{
			UE_LOG(LogStash, Warning, TEXT("[StashBackdrop] CaptureViewport: no world from context — empty bytes"));
			OnDone(TArray<uint8>());
			return;
		}

		UE_LOG(LogStash, Log, TEXT("[StashBackdrop] CaptureViewport: scheduling capture with retries (world=%s)"), *World->GetName());
#if PLATFORM_ANDROID
		TSharedRef<FStashCaptureRetryState> State = MakeShared<FStashCaptureRetryState>();
		State->WeakWorld = World;
		State->OnDone = MoveTemp(OnDone);
		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([State]()
			{
				ScheduleViewportCaptureAttempt(State);
			}));
#else
		UE_LOG(LogStash, Verbose, TEXT("[StashBackdrop] CaptureViewport: non-Android, empty bytes"));
		OnDone(TArray<uint8>());
#endif
	}

	class FStashViewportCaptureLatentAction : public FPendingLatentAction
	{
	public:
		FName ExecutionFunction;
		int32 OutputLink;
		FWeakObjectPtr CallbackTarget;
		TArray<uint8>& OutImageBytes;
		TArray<uint8> CapturedBytes;
		bool bFinished;

		FStashViewportCaptureLatentAction(const FLatentActionInfo& LatentInfo, TArray<uint8>& InOutImageBytes)
			: ExecutionFunction(LatentInfo.ExecutionFunction)
			, OutputLink(LatentInfo.Linkage)
			, CallbackTarget(LatentInfo.CallbackTarget)
			, OutImageBytes(InOutImageBytes)
			, bFinished(false)
		{
		}

		virtual void UpdateOperation(FLatentResponse& Response) override
		{
			if (bFinished)
			{
				OutImageBytes = MoveTemp(CapturedBytes);
			}
			Response.FinishAndTriggerIf(bFinished, ExecutionFunction, OutputLink, CallbackTarget);
		}
	};
}

void UStashBlueprint::CaptureViewportForAndroidCheckoutBackdrop(UObject* WorldContextObject, FOnStashViewportCaptureComplete OnComplete)
{
	ScheduleViewportCapture(WorldContextObject, [OnComplete](TArray<uint8> Bytes) mutable
		{
			OnComplete.ExecuteIfBound(Bytes);
		});
}

void UStashBlueprint::CaptureViewportForAndroidCheckoutBackdropLatent(UObject* WorldContextObject, TArray<uint8>& OutImageBytes, FLatentActionInfo LatentInfo)
{
	if (UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr)
	{
		FLatentActionManager& LatentManager = World->GetLatentActionManager();
		FStashViewportCaptureLatentAction* LatentAction = new FStashViewportCaptureLatentAction(LatentInfo, OutImageBytes);
		LatentManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, LatentAction);
		ScheduleViewportCapture(WorldContextObject, [LatentAction](TArray<uint8> Bytes)
			{
				LatentAction->CapturedBytes = MoveTemp(Bytes);
				LatentAction->bFinished = true;
			});
		return;
	}

	UE_LOG(LogStash, Warning, TEXT("[StashBackdrop] CaptureViewportLatent: no world from context — empty bytes"));
	OutImageBytes.Reset();
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

void UStashBlueprint::HandleExternalPayment(const FString& URL)
{
	UE_LOG(LogStash, Log, TEXT("[Stash] External payment URL: %s"), *URL);
	AsyncTask(ENamedThreads::GameThread, [URL]() {
		if (UStashSubsystem* Sub = GetStashSubsystemFromContext(nullptr)) { Sub->OnExternalPayment.Broadcast(URL); }
		OnExternalPayment.Broadcast(URL);
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

	void StashNativeOnExternalPayment(const char* url)
	{
		UStashBlueprint::HandleExternalPayment(FString(UTF8_TO_TCHAR(url ? url : "")));
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

	JNIEXPORT void JNICALL Java_com_Plugins_Stash_StashHelper_nativeOnExternalPayment(JNIEnv* env, jclass clazz, jstring url)
	{
		FString UrlStr;
		if (url)
		{
			const char* UTFString = env->GetStringUTFChars(url, nullptr);
			if (UTFString)
			{
				UrlStr = FString(UTF8_TO_TCHAR(UTFString));
				env->ReleaseStringUTFChars(url, UTFString);
			}
		}
		UStashBlueprint::HandleExternalPayment(UrlStr);
	}
}
#endif
