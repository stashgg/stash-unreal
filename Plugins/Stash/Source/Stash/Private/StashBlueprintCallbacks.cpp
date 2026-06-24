// Copyright Stash. All Rights Reserved.
//
// Native → game callback path for Stash events.
// Platform bridges (JNI / extern "C") call Handle* below; each event is broadcast on UStashSubsystem
// (Blueprint/C++ recommended) and on legacy static UStashBlueprint delegates (C++ backward compat).
// Bind to one path only — binding both fires your handler twice.

#include "StashBlueprint.h"
#include "Stash.h"
#include "StashSubsystem.h"
#include "Async/Async.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"

FOnStashPaymentSuccess UStashBlueprint::OnPaymentSuccess;
FOnStashPaymentFailure UStashBlueprint::OnPaymentFailure;
FOnStashDialogDismissed UStashBlueprint::OnDialogDismissed;
FOnStashOptInResponse UStashBlueprint::OnOptInResponse;
FOnStashPageLoaded UStashBlueprint::OnPageLoaded;
FOnStashNetworkError UStashBlueprint::OnNetworkError;
FOnStashExternalPayment UStashBlueprint::OnExternalPayment;

namespace
{
#if WITH_EDITOR
	TWeakObjectPtr<UWorld> GEditorPreviewCallbackWorld;
#endif

	/** Resolves subsystem for Blueprint (explicit context) or native callbacks (nullptr → current play world). */
	UStashSubsystem* GetStashSubsystemFromContext(UObject* WorldContextObject)
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
#if WITH_EDITOR
			if (GEditorPreviewCallbackWorld.IsValid())
			{
				World = GEditorPreviewCallbackWorld.Get();
			}
			if (!World && GIsEditor)
			{
				for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
				{
					if (Ctx.World() && Ctx.WorldType == EWorldType::PIE)
					{
						World = Ctx.World();
						break;
					}
				}
			}
#endif
			if (!World)
			{
				World = GEngine->GetCurrentPlayWorld();
			}
		}
		UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
		return GI ? GI->GetSubsystem<UStashSubsystem>() : nullptr;
	}

	/** Native callbacks may arrive off the game thread; always dispatch On* broadcasts on the game thread. */
	template<typename FSubBroadcast, typename FStaticBroadcast>
	void BroadcastStashCallback(FSubBroadcast&& SubBroadcast, FStaticBroadcast&& StaticBroadcast)
	{
		// Resolve while the preview session world is still pinned (EndSession may clear it before AsyncTask runs).
#if WITH_EDITOR
		TWeakObjectPtr<UWorld> WorldAtDispatch = GEditorPreviewCallbackWorld;
#else
		TWeakObjectPtr<UWorld> WorldAtDispatch;
#endif
		TWeakObjectPtr<UStashSubsystem> SubsystemAtDispatch = GetStashSubsystemFromContext(nullptr);
		AsyncTask(ENamedThreads::GameThread, [
			WorldAtDispatch,
			SubsystemAtDispatch,
			SubBroadcast = Forward<FSubBroadcast>(SubBroadcast),
			StaticBroadcast = Forward<FStaticBroadcast>(StaticBroadcast)
		]() mutable {
			UStashSubsystem* Sub = SubsystemAtDispatch.Get();
			if (!Sub && WorldAtDispatch.IsValid())
			{
				if (UGameInstance* GI = WorldAtDispatch->GetGameInstance())
				{
					Sub = GI->GetSubsystem<UStashSubsystem>();
				}
			}
			if (Sub)
			{
				SubBroadcast(Sub);
			}
			else
			{
				UE_LOG(LogStash, Warning, TEXT("[Stash] Subsystem callback skipped (no UStashSubsystem). Use Get Stash Subsystem during PIE and open checkout from Play, not the preview tab alone."));
			}
			StaticBroadcast();
		});
	}
}

#if WITH_EDITOR
void UStashBlueprint::SetEditorPreviewCallbackWorld(UWorld* World)
{
	GEditorPreviewCallbackWorld = World;
}

void UStashBlueprint::ClearEditorPreviewCallbackWorld()
{
	GEditorPreviewCallbackWorld.Reset();
}
#endif

// ---------------------------------------------------------------------------
// Blueprint API — subsystem accessor
// ---------------------------------------------------------------------------

UStashSubsystem* UStashBlueprint::GetStashSubsystem(UObject* WorldContextObject)
{
	return GetStashSubsystemFromContext(WorldContextObject);
}

// ---------------------------------------------------------------------------
// Native callback entry points (StashHelper.java / StashNativeCardWrapper.mm)
// ---------------------------------------------------------------------------

void UStashBlueprint::HandlePaymentSuccess()
{
	UE_LOG(LogStash, Log, TEXT("[Stash] Payment success callback received"));
	BroadcastStashCallback(
		[](UStashSubsystem* Sub) { Sub->OnPaymentSuccess.Broadcast(); },
		[]() { UStashBlueprint::OnPaymentSuccess.Broadcast(); });
}

void UStashBlueprint::HandlePaymentFailure()
{
	UE_LOG(LogStash, Log, TEXT("[Stash] Payment failure callback received"));
	BroadcastStashCallback(
		[](UStashSubsystem* Sub) { Sub->OnPaymentFailure.Broadcast(); },
		[]() { UStashBlueprint::OnPaymentFailure.Broadcast(); });
}

void UStashBlueprint::HandleDialogDismissed()
{
	UE_LOG(LogStash, Log, TEXT("[Stash] Dialog dismissed callback received"));
	BroadcastStashCallback(
		[](UStashSubsystem* Sub) { Sub->OnDialogDismissed.Broadcast(); },
		[]() { UStashBlueprint::OnDialogDismissed.Broadcast(); });
}

void UStashBlueprint::HandleOptInResponse(const FString& OptInType)
{
	UE_LOG(LogStash, Log, TEXT("[Stash] Opt-in response received: %s"), *OptInType);
	BroadcastStashCallback(
		[OptInType](UStashSubsystem* Sub) { Sub->OnOptInResponse.Broadcast(OptInType); },
		[OptInType]() { UStashBlueprint::OnOptInResponse.Broadcast(OptInType); });
}

void UStashBlueprint::HandlePageLoaded(float LoadTimeMs)
{
	UE_LOG(LogStash, Log, TEXT("[Stash] Page loaded callback received: %.2f ms"), LoadTimeMs);
	BroadcastStashCallback(
		[LoadTimeMs](UStashSubsystem* Sub) { Sub->OnPageLoaded.Broadcast(LoadTimeMs); },
		[LoadTimeMs]() { UStashBlueprint::OnPageLoaded.Broadcast(LoadTimeMs); });
}

void UStashBlueprint::HandleNetworkError()
{
	UE_LOG(LogStash, Warning, TEXT("[Stash] Network error callback received"));
	BroadcastStashCallback(
		[](UStashSubsystem* Sub) { Sub->OnNetworkError.Broadcast(); },
		[]() { UStashBlueprint::OnNetworkError.Broadcast(); });
}

void UStashBlueprint::HandleExternalPayment(const FString& URL)
{
	UE_LOG(LogStash, Log, TEXT("[Stash] External payment URL: %s"), *URL);
	BroadcastStashCallback(
		[URL](UStashSubsystem* Sub) { Sub->OnExternalPayment.Broadcast(URL); },
		[URL]() { UStashBlueprint::OnExternalPayment.Broadcast(URL); });
}
