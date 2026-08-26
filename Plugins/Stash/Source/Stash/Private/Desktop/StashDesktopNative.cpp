// Copyright Stash. All Rights Reserved.

#include "StashDesktopNative.h"

#if PLATFORM_WINDOWS || PLATFORM_MAC

#include "Stash.h"
#include "StashBlueprint.h"
#include "Async/Async.h"
#include "Engine/Engine.h"
#include "Engine/GameEngine.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/GameUserSettings.h"
#include "GenericPlatform/GenericWindow.h"
#include "HAL/PlatformProcess.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "Widgets/SWindow.h"

#define STASH_NATIVE_DESKTOP_NO_IMPORT 1
#include "StashNativeDesktop.h"

namespace
{
	typedef void (STASH_NATIVE_DESKTOP_CALL *FSetEventCallback)(StashNativeDesktopEventCallback, void*);
	typedef void (STASH_NATIVE_DESKTOP_CALL *FSetHostWindow)(void*);
	typedef void (STASH_NATIVE_DESKTOP_CALL *FOpenWithConfig)(const char*, const char*);
	typedef void (STASH_NATIVE_DESKTOP_CALL *FOpenUrl)(const char*);
	typedef void (STASH_NATIVE_DESKTOP_CALL *FVoidFn)();
	typedef int (STASH_NATIVE_DESKTOP_CALL *FIntFn)();
	typedef void (STASH_NATIVE_DESKTOP_CALL *FIntArgFn)(int);
	typedef const char* (STASH_NATIVE_DESKTOP_CALL *FVersionFn)();

	struct FStashDesktopExports
	{
		FSetEventCallback SetEventCallback = nullptr;
		FSetHostWindow SetHostWindow = nullptr;
		FOpenWithConfig OpenCard = nullptr;
		FOpenWithConfig OpenModal = nullptr;
		FOpenUrl OpenBrowser = nullptr;
		FVoidFn Dismiss = nullptr;
		FVoidFn ResetPresentationState = nullptr;
		FIntFn IsCurrentlyPresented = nullptr;
		FIntFn IsPurchaseProcessing = nullptr;
		FVoidFn Prewarm = nullptr;
		FIntArgFn SetInspectableWebViewsEnabled = nullptr;
		FVersionFn GetVersion = nullptr;
		FVoidFn Shutdown = nullptr;
	};

	void* GHandle = nullptr;
	bool GProbed = false;
	bool GCallbackInstalled = false;
	FStashDesktopExports GExports;

	// Exclusive fullscreen contends with child-window compositing (Windows); borderless for the flow.
	bool GRestoreFullscreenMode = false;
	EWindowMode::Type GFullscreenModeToRestore = EWindowMode::Windowed;

	FString LibraryPath()
	{
		TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("Stash"));
		const FString Base = Plugin.IsValid() ? Plugin->GetBaseDir() : FPaths::ProjectPluginsDir() / TEXT("Stash");
#if PLATFORM_WINDOWS
		return FPaths::ConvertRelativePathToFull(Base / TEXT("Source/Stash/ThirdParty/Windows/x64/StashNativeDesktop.dll"));
#else
		return FPaths::ConvertRelativePathToFull(Base / TEXT("Source/Stash/ThirdParty/macOS/StashNativeDesktop.bundle"));
#endif
	}

	template <typename T>
	T Resolve(const TCHAR* Name)
	{
		return reinterpret_cast<T>(FPlatformProcess::GetDllExport(GHandle, Name));
	}

	bool EnsureLoaded()
	{
		if (GHandle != nullptr)
		{
			return true;
		}
		if (GProbed)
		{
			return false;
		}
		GProbed = true;
		const FString Path = LibraryPath();
		GHandle = FPlatformProcess::GetDllHandle(*Path);
		if (GHandle == nullptr)
		{
			UE_LOG(LogStash, Error, TEXT("[Stash] Desktop host not found or failed to load: %s"), *Path);
			return false;
		}
		GExports.SetEventCallback = Resolve<FSetEventCallback>(TEXT("StashNativeDesktop_SetEventCallback"));
		GExports.SetHostWindow = Resolve<FSetHostWindow>(TEXT("StashNativeDesktop_SetHostWindow"));
		GExports.OpenCard = Resolve<FOpenWithConfig>(TEXT("StashNativeDesktop_OpenCard"));
		GExports.OpenModal = Resolve<FOpenWithConfig>(TEXT("StashNativeDesktop_OpenModal"));
		GExports.OpenBrowser = Resolve<FOpenUrl>(TEXT("StashNativeDesktop_OpenBrowser"));
		GExports.Dismiss = Resolve<FVoidFn>(TEXT("StashNativeDesktop_Dismiss"));
		GExports.ResetPresentationState = Resolve<FVoidFn>(TEXT("StashNativeDesktop_ResetPresentationState"));
		GExports.IsCurrentlyPresented = Resolve<FIntFn>(TEXT("StashNativeDesktop_IsCurrentlyPresented"));
		GExports.IsPurchaseProcessing = Resolve<FIntFn>(TEXT("StashNativeDesktop_IsPurchaseProcessing"));
		GExports.Prewarm = Resolve<FVoidFn>(TEXT("StashNativeDesktop_Prewarm"));
		GExports.SetInspectableWebViewsEnabled = Resolve<FIntArgFn>(TEXT("StashNativeDesktop_SetInspectableWebViewsEnabled"));
		GExports.GetVersion = Resolve<FVersionFn>(TEXT("StashNativeDesktop_GetVersion"));
		GExports.Shutdown = Resolve<FVoidFn>(TEXT("StashNativeDesktop_Shutdown"));
		const bool bComplete = GExports.SetEventCallback && GExports.SetHostWindow && GExports.OpenCard && GExports.OpenModal
			&& GExports.OpenBrowser && GExports.Dismiss && GExports.ResetPresentationState && GExports.IsCurrentlyPresented
			&& GExports.IsPurchaseProcessing && GExports.Prewarm && GExports.SetInspectableWebViewsEnabled
			&& GExports.GetVersion && GExports.Shutdown;
		if (!bComplete)
		{
			UE_LOG(LogStash, Error, TEXT("[Stash] Desktop host is missing exports (not the expected version): %s"), *Path);
			// Not freed on purpose; the handle stays parked and the host is reported unavailable.
			GHandle = nullptr;
			return false;
		}
		UE_LOG(LogStash, Log, TEXT("[Stash] Desktop host %s loaded from %s"), UTF8_TO_TCHAR(GExports.GetVersion()), *Path);
		return true;
	}

	// The host calls this on the UI thread, possibly inside window-message dispatch: only hand the event to
	// UStashBlueprint::Handle*, which AsyncTask to the game thread and resolve the subsystem there.
	void STASH_NATIVE_DESKTOP_CALL OnHostEvent(const char* Type, const char* Payload, void* /*UserData*/)
	{
		FStashDesktopNative::DispatchEvent(FString(UTF8_TO_TCHAR(Type ? Type : "")), FString(UTF8_TO_TCHAR(Payload ? Payload : "")));
	}

	bool EnsureReady()
	{
		if (!EnsureLoaded())
		{
			return false;
		}
		if (!GCallbackInstalled)
		{
			GExports.SetEventCallback(&OnHostEvent, nullptr);
			GCallbackInstalled = true;
		}
		return true;
	}

	void* GameWindowHandle()
	{
		if (GEngine && GEngine->GameViewport)
		{
			TSharedPtr<SWindow> Window = GEngine->GameViewport->GetWindow();
			if (Window.IsValid() && Window->GetNativeWindow().IsValid())
			{
				return Window->GetNativeWindow()->GetOSWindowHandle();
			}
		}
		return nullptr;
	}

	void SwitchFromExclusiveFullscreenIfNeeded()
	{
#if PLATFORM_WINDOWS
		UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
		if (Settings && Settings->GetFullscreenMode() == EWindowMode::Fullscreen)
		{
			GFullscreenModeToRestore = EWindowMode::Fullscreen;
			GRestoreFullscreenMode = true;
			Settings->SetFullscreenMode(EWindowMode::WindowedFullscreen);
			Settings->ApplyResolutionSettings(false);
		}
#endif
	}

	void RestoreFullscreenModeIfNeeded()
	{
		if (!GRestoreFullscreenMode)
		{
			return;
		}
		AsyncTask(ENamedThreads::GameThread, []()
		{
			if (!GRestoreFullscreenMode || FStashDesktopNative::IsCardOpen())
			{
				return;
			}
			GRestoreFullscreenMode = false;
			UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
			if (Settings)
			{
				Settings->SetFullscreenMode(GFullscreenModeToRestore);
				Settings->ApplyResolutionSettings(false);
			}
		});
	}

	FString JsonString(const FString& Value)
	{
		FString Escaped = Value;
		Escaped.ReplaceInline(TEXT("\\"), TEXT("\\\\"));
		Escaped.ReplaceInline(TEXT("\""), TEXT("\\\""));
		Escaped.ReplaceInline(TEXT("\n"), TEXT("\\n"));
		Escaped.ReplaceInline(TEXT("\r"), TEXT("\\r"));
		Escaped.ReplaceInline(TEXT("\t"), TEXT("\\t"));
		return FString::Printf(TEXT("\"%s\""), *Escaped);
	}

	FString JsonNumber(float Value)
	{
		return FString::SanitizeFloat(Value);
	}

	const TCHAR* JsonBool(bool bValue)
	{
		return bValue ? TEXT("true") : TEXT("false");
	}

	FString PresentationField(bool bWindowPresentation)
	{
		return FString::Printf(TEXT("\"presentation\":\"%s\""), bWindowPresentation ? TEXT("window") : TEXT("attached"));
	}
}

bool FStashDesktopNative::IsAvailable()
{
	return EnsureLoaded();
}

// autoClose is not part of FStashCardConfig; omitted so the host applies its default (true), as mobile does.
FString FStashDesktopNative::CardConfigToJson(const FStashCardConfig& Config, bool bWindowPresentation)
{
	return FString::Printf(TEXT("{\"forcePortrait\":%s,\"cardHeightRatioPortrait\":%s,\"cardWidthRatioLandscape\":%s,")
		TEXT("\"cardHeightRatioLandscape\":%s,\"tabletWidthRatioPortrait\":%s,\"tabletHeightRatioPortrait\":%s,")
		TEXT("\"tabletWidthRatioLandscape\":%s,\"tabletHeightRatioLandscape\":%s,\"backgroundColor\":%s,%s}"),
		JsonBool(Config.bForcePortrait),
		*JsonNumber(Config.CardHeightRatioPortrait), *JsonNumber(Config.CardWidthRatioLandscape),
		*JsonNumber(Config.CardHeightRatioLandscape), *JsonNumber(Config.TabletWidthRatioPortrait),
		*JsonNumber(Config.TabletHeightRatioPortrait), *JsonNumber(Config.TabletWidthRatioLandscape),
		*JsonNumber(Config.TabletHeightRatioLandscape), *JsonString(Config.BackgroundColor),
		*PresentationField(bWindowPresentation));
}

FString FStashDesktopNative::ModalConfigToJson(const FStashModalConfig& Config, bool bWindowPresentation)
{
	return FString::Printf(TEXT("{\"allowDismiss\":%s,\"phoneWidthRatioPortrait\":%s,\"phoneHeightRatioPortrait\":%s,")
		TEXT("\"phoneWidthRatioLandscape\":%s,\"phoneHeightRatioLandscape\":%s,\"tabletWidthRatioPortrait\":%s,")
		TEXT("\"tabletHeightRatioPortrait\":%s,\"tabletWidthRatioLandscape\":%s,\"tabletHeightRatioLandscape\":%s,")
		TEXT("\"backgroundColor\":%s,%s}"),
		JsonBool(Config.bAllowDismiss),
		*JsonNumber(Config.PhoneWidthRatioPortrait), *JsonNumber(Config.PhoneHeightRatioPortrait),
		*JsonNumber(Config.PhoneWidthRatioLandscape), *JsonNumber(Config.PhoneHeightRatioLandscape),
		*JsonNumber(Config.TabletWidthRatioPortrait), *JsonNumber(Config.TabletHeightRatioPortrait),
		*JsonNumber(Config.TabletWidthRatioLandscape), *JsonNumber(Config.TabletHeightRatioLandscape),
		*JsonString(Config.BackgroundColor), *PresentationField(bWindowPresentation));
}

bool FStashDesktopNative::OpenCard(const FString& URL, const FStashCardConfig& Config, bool bWindowPresentation)
{
	if (!EnsureReady())
	{
		return false;
	}
	if (!bWindowPresentation)
	{
		GExports.SetHostWindow(GameWindowHandle());
		SwitchFromExclusiveFullscreenIfNeeded();
	}
	const FString Json = CardConfigToJson(Config, bWindowPresentation);
	GExports.OpenCard(TCHAR_TO_UTF8(*URL), TCHAR_TO_UTF8(*Json));
	return true;
}

bool FStashDesktopNative::OpenModal(const FString& URL, const FStashModalConfig& Config, bool bWindowPresentation)
{
	if (!EnsureReady())
	{
		return false;
	}
	if (!bWindowPresentation)
	{
		GExports.SetHostWindow(GameWindowHandle());
		SwitchFromExclusiveFullscreenIfNeeded();
	}
	const FString Json = ModalConfigToJson(Config, bWindowPresentation);
	GExports.OpenModal(TCHAR_TO_UTF8(*URL), TCHAR_TO_UTF8(*Json));
	return true;
}

bool FStashDesktopNative::OpenBrowser(const FString& URL)
{
	if (!EnsureReady())
	{
		return false;
	}
	GExports.OpenBrowser(TCHAR_TO_UTF8(*URL));
	return true;
}

void FStashDesktopNative::Dismiss()
{
	if (GHandle != nullptr)
	{
		GExports.Dismiss();
	}
}

void FStashDesktopNative::ResetPresentationState()
{
	if (GHandle != nullptr)
	{
		GExports.ResetPresentationState();
	}
}

bool FStashDesktopNative::IsCardOpen()
{
	return GHandle != nullptr && GExports.IsCurrentlyPresented() != 0;
}

bool FStashDesktopNative::IsPurchaseProcessing()
{
	return GHandle != nullptr && GExports.IsPurchaseProcessing() != 0;
}

void FStashDesktopNative::Prewarm()
{
	if (EnsureReady())
	{
		GExports.Prewarm();
	}
}

void FStashDesktopNative::SetInspectableWebViewsEnabled(bool bEnabled)
{
	if (EnsureReady())
	{
		GExports.SetInspectableWebViewsEnabled(bEnabled ? 1 : 0);
	}
}

FString FStashDesktopNative::GetVersion()
{
	return EnsureLoaded() ? FString(UTF8_TO_TCHAR(GExports.GetVersion())) : FString();
}

void FStashDesktopNative::Shutdown()
{
	if (GHandle == nullptr)
	{
		return;
	}
	GExports.Shutdown();
	GCallbackInstalled = false;
	GRestoreFullscreenMode = false;
}

void FStashDesktopNative::DispatchEvent(const FString& Type, const FString& Payload)
{
	if (Type == TEXT(STASH_NATIVE_DESKTOP_EVENT_PAYMENT_SUCCESS))
	{
		// UStashBlueprint has no order payload on its success delegate; the order stays in the log.
		if (!Payload.IsEmpty())
		{
			UE_LOG(LogStash, Log, TEXT("[Stash] Desktop payment success order=%s"), *Payload);
		}
		UStashBlueprint::HandlePaymentSuccess();
		RestoreFullscreenModeIfNeeded();
	}
	else if (Type == TEXT(STASH_NATIVE_DESKTOP_EVENT_PAYMENT_FAILURE))
	{
		UStashBlueprint::HandlePaymentFailure();
		RestoreFullscreenModeIfNeeded();
	}
	else if (Type == TEXT(STASH_NATIVE_DESKTOP_EVENT_DIALOG_DISMISSED))
	{
		UStashBlueprint::HandleDialogDismissed();
		RestoreFullscreenModeIfNeeded();
	}
	else if (Type == TEXT(STASH_NATIVE_DESKTOP_EVENT_OPT_IN_RESPONSE))
	{
		UStashBlueprint::HandleOptInResponse(Payload);
	}
	else if (Type == TEXT(STASH_NATIVE_DESKTOP_EVENT_PAGE_LOADED))
	{
		UStashBlueprint::HandlePageLoaded(FCString::Atof(*Payload));
	}
	else if (Type == TEXT(STASH_NATIVE_DESKTOP_EVENT_NETWORK_ERROR))
	{
		UStashBlueprint::HandleNetworkError();
		RestoreFullscreenModeIfNeeded();
	}
	else if (Type == TEXT(STASH_NATIVE_DESKTOP_EVENT_EXTERNAL_PAYMENT))
	{
		UStashBlueprint::HandleExternalPayment(Payload);
		RestoreFullscreenModeIfNeeded();
	}
	else if (Type == TEXT(STASH_NATIVE_DESKTOP_EVENT_PURCHASE_PROCESSING))
	{
		UStashBlueprint::HandlePurchaseProcessing();
	}
	else if (Type == TEXT(STASH_NATIVE_DESKTOP_EVENT_PROCESSING_COMPLETED))
	{
		UStashBlueprint::HandleProcessingCompleted();
	}
	else
	{
		// navigation, navigationBlocked, webProcessCrashed, error: diagnostics only.
		UE_LOG(LogStash, Log, TEXT("[Stash] Desktop %s %s"), *Type, *Payload);
	}
}

#endif
