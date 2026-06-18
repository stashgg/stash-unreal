// Copyright Stash. All Rights Reserved.
//
// Android force-portrait checkout: read back the game viewport on the render thread, encode JPEG,
// retry until the swapchain/backbuffer is ready (see StashBackdropMaxCaptureAttempts).

#include "StashAndroidBackdropCapture.h"
#include "Stash.h"
#include "Async/Async.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "LatentActions.h"
#include "TimerManager.h"

#if PLATFORM_ANDROID
#include "Engine/GameViewportClient.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Modules/ModuleManager.h"
#include "RHI.h"
#include "RHICommandList.h"
#include "RenderingThread.h"
#include "Slate/SceneViewport.h"
#endif

namespace
{
#if PLATFORM_ANDROID
	/** Vulkan/Android may return an empty backbuffer for a few frames after rotation — retry on next tick. */
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

	class FStashLatentCaptureState
	{
	public:
		TArray<uint8> CapturedBytes;
		bool bFinished = false;
	};

	class FStashViewportCaptureLatentAction : public FPendingLatentAction
	{
	public:
		FName ExecutionFunction;
		int32 OutputLink;
		FWeakObjectPtr CallbackTarget;
		TArray<uint8>& OutImageBytes;
		TSharedRef<FStashLatentCaptureState> CaptureState;

		FStashViewportCaptureLatentAction(
			const FLatentActionInfo& LatentInfo,
			TArray<uint8>& InOutImageBytes,
			TSharedRef<FStashLatentCaptureState> InCaptureState)
			: ExecutionFunction(LatentInfo.ExecutionFunction)
			, OutputLink(LatentInfo.Linkage)
			, CallbackTarget(LatentInfo.CallbackTarget)
			, OutImageBytes(InOutImageBytes)
			, CaptureState(InCaptureState)
		{
		}

		virtual void UpdateOperation(FLatentResponse& Response) override
		{
			if (CaptureState->bFinished)
			{
				OutImageBytes = MoveTemp(CaptureState->CapturedBytes);
			}
			Response.FinishAndTriggerIf(CaptureState->bFinished, ExecutionFunction, OutputLink, CallbackTarget);
		}
	};

	static void BeginLatentBackdropCapture(
		UWorld* World,
		UObject* WorldContextObject,
		TArray<uint8>& OutImageBytes,
		const FLatentActionInfo& LatentInfo)
	{
		TSharedRef<FStashLatentCaptureState> CaptureState = MakeShared<FStashLatentCaptureState>();
		FLatentActionManager& LatentManager = World->GetLatentActionManager();
		// FPendingLatentAction is heap-allocated; FLatentActionManager owns and deletes it.
		FStashViewportCaptureLatentAction* LatentAction = new FStashViewportCaptureLatentAction(LatentInfo, OutImageBytes, CaptureState);
		LatentManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, LatentAction);

		TWeakObjectPtr<UWorld> WeakWorld(World);
		StashScheduleAndroidCheckoutBackdropCapture(WorldContextObject, [CaptureState, WeakWorld](TArray<uint8> Bytes) mutable
			{
				if (!WeakWorld.IsValid())
				{
					return;
				}
				CaptureState->CapturedBytes = MoveTemp(Bytes);
				CaptureState->bFinished = true;
			});
	}
}

// ---------------------------------------------------------------------------
// Public entry points (called from UStashBlueprint capture nodes)
// ---------------------------------------------------------------------------

void StashScheduleAndroidCheckoutBackdropCapture(UObject* WorldContextObject, TFunction<void(TArray<uint8>)> OnDone)
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

void StashCaptureAndroidCheckoutBackdropLatent(UObject* WorldContextObject, TArray<uint8>& OutImageBytes, FLatentActionInfo LatentInfo)
{
	if (UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr)
	{
		BeginLatentBackdropCapture(World, WorldContextObject, OutImageBytes, LatentInfo);
		return;
	}

	UE_LOG(LogStash, Warning, TEXT("[StashBackdrop] CaptureViewportLatent: no world from context — empty bytes"));
	OutImageBytes.Reset();
}
