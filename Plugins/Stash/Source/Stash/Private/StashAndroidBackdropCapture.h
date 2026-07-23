// Copyright Stash. All Rights Reserved.
// Android checkout backdrop — viewport readback and JPEG encode.
// Lives under Private/ (not Private/Android/) so editor Win64 builds link the non-Android stubs.

#pragma once

#include "CoreMinimal.h"
#include "Engine/LatentActionManager.h" // FLatentActionInfo (was pulled in transitively via the engine shared PCH)

/** Schedules end-of-frame viewport capture with retries; invokes OnDone on the game thread with JPEG bytes (or empty on failure). */
void StashScheduleAndroidCheckoutBackdropCapture(UObject* WorldContextObject, TFunction<void(TArray<uint8>)> OnDone);

/** Latent Blueprint capture: writes JPEG bytes to OutImageBytes when complete. */
void StashCaptureAndroidCheckoutBackdropLatent(UObject* WorldContextObject, TArray<uint8>& OutImageBytes, FLatentActionInfo LatentInfo);
