// Copyright Stash. All Rights Reserved.
// Windows / macOS: loader and bridge for the stash-native desktop hosts (StashNativeDesktop.dll,
// StashNativeDesktop.bundle). One C ABI on both OSes, see ThirdParty/Desktop/include/StashNativeDesktop.h.

#pragma once

#include "CoreMinimal.h"

#if PLATFORM_WINDOWS || PLATFORM_MAC

struct FStashCardConfig;
struct FStashModalConfig;

/**
 * Binds the desktop host's exports at first use (the library is never freed: late WebView2 / WebKit
 * completions can still arrive; Shutdown releases the webview environment instead) and turns its
 * events into UStashBlueprint::Handle* calls, which marshal to the game thread themselves.
 *
 * Calls must come from the game thread: on Windows the host requires the thread that owns the host
 * window's message loop, which is the game thread in Unreal.
 */
class FStashDesktopNative
{
public:
	/** True when the host library loaded and every export resolved. */
	static bool IsAvailable();

	/** JSON the hosts parse: mobile field names plus presentation / width / height / allowFileUrls. */
	static FString CardConfigToJson(const FStashCardConfig& Config, bool bWindowPresentation);
	static FString ModalConfigToJson(const FStashModalConfig& Config, bool bWindowPresentation);

	/** bWindowPresentation: standalone window (editor PIE) instead of the card over the game window. */
	static bool OpenCard(const FString& URL, const FStashCardConfig& Config, bool bWindowPresentation);
	static bool OpenModal(const FString& URL, const FStashModalConfig& Config, bool bWindowPresentation);
	static bool OpenBrowser(const FString& URL);
	static void Dismiss();
	static void ResetPresentationState();
	static bool IsCardOpen();
	static bool IsPurchaseProcessing();
	static void Prewarm();
	static void SetInspectableWebViewsEnabled(bool bEnabled);
	static FString GetVersion();

	/** Releases the webview environment and clears the callback (module shutdown, PIE end). */
	static void Shutdown();

	/** Maps one host event onto UStashBlueprint::Handle*; exposed for the automation tests. */
	static void DispatchEvent(const FString& Type, const FString& Payload);
};

#endif
