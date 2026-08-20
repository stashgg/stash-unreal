// Copyright Stash. All Rights Reserved.

#include "StashEditorPreviewService.h"
#include "SStashPreviewPanel.h"
#include "StashEditorPreviewTab.h"
#include "StashPreviewJsBridge.h"
#include "StashPreviewCallbackUrl.h"
#include "StashEditorLog.h"
#include "StashBlueprint.h"
#include "StashEditorSettings.h"
#include "HAL/PlatformProcess.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "Misc/App.h"
#include "Async/Async.h"
#include "Modules/ModuleManager.h"

TSharedRef<FStashEditorPreviewService> FStashEditorPreviewService::Get()
{
	static TSharedRef<FStashEditorPreviewService> Instance = MakeShared<FStashEditorPreviewService>();
	return Instance;
}

bool FStashEditorPreviewService::IsPreviewEnabled() const
{
	const UStashEditorSettings* Settings = GetDefault<UStashEditorSettings>();
	if (!Settings || !Settings->bEnableEditorPreview)
	{
		return false;
	}
#if !STASH_HAS_WEBBROWSER
	static bool bWarnedNoWebBrowser = false;
	if (!bWarnedNoWebBrowser)
	{
		bWarnedNoWebBrowser = true;
		UE_LOG(LogStashEditor, Warning, TEXT("[StashPreview] Web Browser engine plugin is not installed in this Unreal build. Enable it under Edit → Plugins → Web Browser if available, then rebuild."));
	}
	return false;
#else
	if (!FModuleManager::Get().IsModuleLoaded(TEXT("WebBrowser")) && !FModuleManager::Get().ModuleExists(TEXT("WebBrowser")))
	{
		static bool bWarnedNoModule = false;
		if (!bWarnedNoModule)
		{
			bWarnedNoModule = true;
			UE_LOG(LogStashEditor, Warning, TEXT("[StashPreview] WebBrowser module not found. Enable Edit → Plugins → Web Browser and restart the editor."));
		}
		return false;
	}
	return true;
#endif
}

void FStashEditorPreviewService::EnsureWebBrowserLoaded()
{
#if STASH_HAS_WEBBROWSER
	if (!FModuleManager::Get().IsModuleLoaded(TEXT("WebBrowser")) && FModuleManager::Get().ModuleExists(TEXT("WebBrowser")))
	{
		FModuleManager::Get().LoadModule(TEXT("WebBrowser"));
	}
#endif
}

bool FStashEditorPreviewService::IsAvailable() const
{
	return IsPreviewEnabled();
}

FString FStashEditorPreviewService::NormalizeUrl(const FString& URL) const
{
	if (URL.IsEmpty())
	{
		return FString();
	}
	if (URL.StartsWith(TEXT("http://")) || URL.StartsWith(TEXT("https://")))
	{
		return URL;
	}
	return FString(TEXT("https://")) + URL;
}

bool FStashEditorPreviewService::PrepareSession(const FString& URL, EStashPreviewPresentationMode Mode)
{
	if (!IsPreviewEnabled())
	{
		return false;
	}
	EnsureWebBrowserLoaded();
	const FString Normalized = NormalizeUrl(URL);
	if (Normalized.IsEmpty())
	{
		return false;
	}

	Session.bIsOpen = true;
	Session.bIsPurchaseProcessing = false;
	Session.bForcePortraitLayout = false;
	Session.bAllowDismiss = true;
	Session.bKeyboardVisible = false;
	Session.KeyboardInputType.Reset();
	Session.CurrentUrl = Normalized;
	Session.PresentationMode = Mode;
	Session.LoadStartSeconds = FApp::GetCurrentTime();
	Session.bPageLoadedFired = false;
	ResetCallbackDedup();
	return true;
}

void FStashEditorPreviewService::CommitSession()
{
	EnsurePreviewTabOpen();
	RefreshAllPanels();
	UE_LOG(LogStashEditor, Log, TEXT("[StashPreview] Session committed (%d): %s"),
		static_cast<int32>(Session.PresentationMode), *Session.CurrentUrl);
}

bool FStashEditorPreviewService::OpenCard(const FString& URL, const FStashCardConfig& Config)
{
	if (!PrepareSession(URL, EStashPreviewPresentationMode::Card))
	{
		return false;
	}
	Session.CardConfig = Config;
	Session.bForcePortraitLayout = Config.bForcePortrait;
	if (Config.AndroidCheckoutBackdrop.Num() > 0)
	{
		Session.BackdropBytes = Config.AndroidCheckoutBackdrop;
	}
	CommitSession();
	return true;
}

bool FStashEditorPreviewService::OpenModal(const FString& URL, const FStashModalConfig& Config)
{
	if (!PrepareSession(URL, EStashPreviewPresentationMode::Modal))
	{
		return false;
	}
	Session.ModalConfig = Config;
	Session.bAllowDismiss = Config.bAllowDismiss;
	CommitSession();
	return true;
}

bool FStashEditorPreviewService::OpenBrowser(const FString& URL)
{
	const UStashEditorSettings* Settings = GetDefault<UStashEditorSettings>();
	if (Settings && !Settings->bOpenBrowserInPreviewPanel)
	{
		const FString Normalized = NormalizeUrl(URL);
		if (!Normalized.IsEmpty())
		{
			FPlatformProcess::LaunchURL(*Normalized, nullptr, nullptr);
			UE_LOG(LogStashEditor, Log, TEXT("[StashPreview] OpenBrowser → OS browser: %s"), *Normalized);
			return true;
		}
		return false;
	}
	if (!PrepareSession(URL, EStashPreviewPresentationMode::Browser))
	{
		return false;
	}
	CommitSession();
	return true;
}

void FStashEditorPreviewService::EndSession(bool bFireDismiss)
{
	if (!Session.bIsOpen)
	{
		return;
	}
	Session.bIsOpen = false;
	Session.bIsPurchaseProcessing = false;
	Session.bKeyboardVisible = false;
	Session.KeyboardInputType.Reset();
	ResetCallbackDedup();
	RefreshAllPanels();
	if (bFireDismiss)
	{
		UStashBlueprint::HandleDialogDismissed();
	}
	// Deferred on purpose: the game-thread task queue is FIFO, so the broadcast tasks that the Handle*
	// callbacks (HandleDialogDismissed / HandlePaymentSuccess / …) enqueue run before this one and still
	// see the pinned preview world. Collapsing this to a direct call would clear the world first and make
	// late callbacks resolve against whatever world the fallback finds.
	AsyncTask(ENamedThreads::GameThread, []()
	{
		UStashBlueprint::ClearEditorPreviewCallbackWorld();
	});
}

void FStashEditorPreviewService::ResetTransientSessionState()
{
	Session.bIsPurchaseProcessing = false;
	Session.bKeyboardVisible = false;
	Session.KeyboardInputType.Reset();
	ResetCallbackDedup();
	RefreshAllPanels();
}

void FStashEditorPreviewService::ResetCallbackDedup()
{
	LastPreviewCallbackDedupKey.Reset();
	LastPreviewCallbackTime = -1.0;
	DispatchedTerminalKeys.Reset();
}

bool FStashEditorPreviewService::CloseBrowser()
{
	if (Session.bIsOpen && Session.PresentationMode == EStashPreviewPresentationMode::Browser)
	{
		if (Session.ActivePlatform == EStashPreviewPlatform::Android)
		{
			// Chrome Custom Tabs cannot be closed by the app — mirror the device no-op.
			UE_LOG(LogStashEditor, Warning, TEXT("[StashPreview] CloseBrowser is a no-op on Android (Chrome Custom Tabs). Use the simulated back button or Dismiss to close the preview."));
			return true;
		}
		EndSession(true);
		return true;
	}
	return false;
}

bool FStashEditorPreviewService::DismissCard()
{
	// Device parity: openBrowser launches Custom Tabs / SFSafariViewController, which the native
	// card-state / dismiss API does not act on. A full-bleed Browser session is not a dismissable card.
	if (!Session.bIsOpen || Session.PresentationMode == EStashPreviewPresentationMode::Browser)
	{
		return false;
	}
	if (Session.bIsPurchaseProcessing)
	{
		UE_LOG(LogStashEditor, Log, TEXT("[StashPreview] DismissCard ignored while purchase is processing"));
		return false;
	}
	EndSession(true);
	return true;
}

bool FStashEditorPreviewService::IsCardOpen() const
{
	// Device parity: a Browser session (Custom Tabs / Safari VC on device) is not reflected by the
	// native card-state query, so game logic branching on IsCardOpen behaves the same in preview and on hardware.
	return Session.bIsOpen && Session.PresentationMode != EStashPreviewPresentationMode::Browser;
}

bool FStashEditorPreviewService::IsPurchaseProcessing() const
{
	return Session.bIsPurchaseProcessing;
}

void FStashEditorPreviewService::SetLandscapeLockWhenCardClosed(bool bEnable)
{
	Session.bLandscapeLockWhenCardClosed = bEnable;
	UE_LOG(LogStashEditor, Log, TEXT("[StashPreview] SetLandscapeLockWhenCardClosed: %d (preview device stays landscape while no Stash UI is open)"), bEnable ? 1 : 0);
	RefreshAllPanels();
}

void FStashEditorPreviewService::SetAndroidKeepAliveEnabled(bool bEnabled)
{
	Session.bKeepAliveEnabled = bEnabled;
	UE_LOG(LogStashEditor, Log, TEXT("[StashPreview] SetAndroidKeepAliveEnabled: %d (shown as a notification mock on Android presets in Browser mode)"), bEnabled ? 1 : 0);
	RefreshAllPanels();
}

void FStashEditorPreviewService::SetAndroidKeepAliveConfig(const FStashKeepAliveConfig& Config)
{
	Session.KeepAliveConfig = Config;
	UE_LOG(LogStashEditor, Log, TEXT("[StashPreview] SetAndroidKeepAliveConfig: title=%s"), *Config.NotificationTitle);
	RefreshAllPanels();
}

void FStashEditorPreviewService::SetActivePlatform(EStashPreviewPlatform Platform)
{
	if (Session.ActivePlatform != Platform)
	{
		Session.ActivePlatform = Platform;
		RefreshAllPanels();
	}
}

void FStashEditorPreviewService::SetPreviewDeviceEmulation(const FString& UserAgent, bool bMobile, const FString& PlatformHint)
{
	PreviewUserAgent = UserAgent;
	bPreviewMobile = bMobile;
	PreviewPlatformHint = PlatformHint;
}

void FStashEditorPreviewService::HandleAndroidBack()
{
	if (!Session.bIsOpen)
	{
		return;
	}
	if (Session.bKeyboardVisible)
	{
		SetKeyboardVisible(false, FString());
		return;
	}
	if (Session.bIsPurchaseProcessing)
	{
		UE_LOG(LogStashEditor, Log, TEXT("[StashPreview] Back ignored while purchase is processing"));
		return;
	}
	if (Session.PresentationMode == EStashPreviewPresentationMode::Modal && !Session.bAllowDismiss)
	{
		UE_LOG(LogStashEditor, Log, TEXT("[StashPreview] Back ignored (modal allowDismiss=false)"));
		return;
	}
	UE_LOG(LogStashEditor, Log, TEXT("[StashPreview] Back → dismiss"));
	EndSession(true);
}

void FStashEditorPreviewService::SetKeyboardVisible(bool bVisible, const FString& InputType)
{
	if (Session.bKeyboardVisible == bVisible && Session.KeyboardInputType == InputType)
	{
		return;
	}
	Session.bKeyboardVisible = bVisible;
	Session.KeyboardInputType = bVisible ? InputType : FString();
	RefreshAllPanels();
}

void FStashEditorPreviewService::SetAndroidCheckoutBackdropBytes(const TArray<uint8>& ImageBytes)
{
	Session.BackdropBytes = ImageBytes;
	UE_LOG(LogStashEditor, Log, TEXT("[StashPreview] SetAndroidCheckoutBackdropBytes: %d bytes"), ImageBytes.Num());
	RefreshAllPanels();
}

void FStashEditorPreviewService::ClearAndroidCheckoutBackdrop()
{
	Session.BackdropBytes.Reset();
	UE_LOG(LogStashEditor, Log, TEXT("[StashPreview] ClearAndroidCheckoutBackdrop"));
	RefreshAllPanels();
}

void FStashEditorPreviewService::RegisterPreviewPanel(const TSharedPtr<SStashPreviewPanel>& Panel)
{
	if (Panel.IsValid())
	{
		PreviewPanels.AddUnique(Panel);
	}
}

void FStashEditorPreviewService::UnregisterPreviewPanel(const TSharedPtr<SStashPreviewPanel>& Panel)
{
	PreviewPanels.RemoveAll([&Panel](const TWeakPtr<SStashPreviewPanel>& Weak)
	{
		return !Weak.IsValid() || Weak.Pin() == Panel;
	});
}

void FStashEditorPreviewService::RefreshAllPanels()
{
	PreviewPanels.RemoveAll([](const TWeakPtr<SStashPreviewPanel>& Weak) { return !Weak.IsValid(); });
	for (const TWeakPtr<SStashPreviewPanel>& Weak : PreviewPanels)
	{
		if (TSharedPtr<SStashPreviewPanel> Panel = Weak.Pin())
		{
			Panel->RefreshFromSession();
		}
	}
}

void FStashEditorPreviewService::EnsurePreviewTabOpen()
{
	const UStashEditorSettings* Settings = GetDefault<UStashEditorSettings>();
	if (Settings && Settings->bAutoOpenPreviewTab)
	{
		FStashEditorPreviewTab::OpenPreviewTab();
	}
}

namespace
{
	bool IsTerminalCallbackPath(const FString& Path)
	{
		return Path == TEXT("paymentSuccess")
			|| Path == TEXT("paymentFailure")
			|| Path == TEXT("optin")
			|| Path == TEXT("externalBrowser");
	}
}

bool FStashEditorPreviewService::DispatchPreviewCallbackUrl(const FString& Url)
{
	FString Path;
	FString Query;
	if (!StashPreviewCallbackUrl::ParsePreviewCallbackUrl(Url, Path, Query))
	{
		return false;
	}

	const FString DedupKey = StashPreviewCallbackUrl::BuildDedupKey(Path, Query);
	if (IsTerminalCallbackPath(Path))
	{
		// Terminal callbacks fire at most once per session, no matter how many channels
		// (console message, navigation intercept, CEF scheme handler) deliver them or how far apart —
		// an editor hitch longer than the rolling window must never double-fire OnPaymentSuccess.
		if (DispatchedTerminalKeys.Contains(DedupKey))
		{
			return false;
		}
		DispatchedTerminalKeys.Add(DedupKey);
	}
	else
	{
		// Non-terminal (repeatable) callbacks keep the short rolling-time window so the same key
		// arriving on multiple channels within a frame or two collapses to one dispatch.
		const double Now = FApp::GetCurrentTime();
		if (DedupKey == LastPreviewCallbackDedupKey
			&& LastPreviewCallbackTime >= 0.0
			&& (Now - LastPreviewCallbackTime) < PreviewCallbackDedupSeconds)
		{
			return false;
		}
		LastPreviewCallbackDedupKey = DedupKey;
		LastPreviewCallbackTime = Now;
	}

	const FString CanonicalUrl = StashPreviewCallbackUrl::BuildPreviewCallbackUrl(Path, Query);
	UE_LOG(LogStashEditor, Log, TEXT("[StashPreview] Callback URL: %s"), *CanonicalUrl);
	HandleCallback(Path, Query);
	return true;
}

void FStashEditorPreviewService::FinishProcessingIfActive()
{
	if (Session.bIsPurchaseProcessing)
	{
		Session.bIsPurchaseProcessing = false;
		UStashBlueprint::HandleProcessingCompleted();
	}
}

void FStashEditorPreviewService::HandleCallback(const FString& Path, const FString& Query)
{
	auto ParseQueryParam = [&Query](const TCHAR* Key) -> FString
	{
		if (Query.IsEmpty())
		{
			return FString();
		}
		TArray<FString> Pairs;
		Query.ParseIntoArray(Pairs, TEXT("&"));
		const FString Prefix = FString(Key) + TEXT("=");
		for (const FString& Pair : Pairs)
		{
			if (Pair.StartsWith(Prefix))
			{
				FString Value = Pair.Mid(Prefix.Len());
				Value = FGenericPlatformHttp::UrlDecode(Value);
				return Value;
			}
		}
		return FString();
	};

	// Terminal callbacks are gated on an open session: Simulate buttons (and late/duplicate channel
	// deliveries) must not broadcast payment events into game code when nothing is open.
	if (Path == TEXT("paymentSuccess"))
	{
		if (!Session.bIsOpen)
		{
			return;
		}
		UE_LOG(LogStashEditor, Log, TEXT("[StashPreview] Callback: paymentSuccess"));
		FinishProcessingIfActive();
		UStashBlueprint::HandlePaymentSuccess();
		EndSession(false);
	}
	else if (Path == TEXT("paymentFailure"))
	{
		if (!Session.bIsOpen)
		{
			return;
		}
		UE_LOG(LogStashEditor, Log, TEXT("[StashPreview] Callback: paymentFailure"));
		FinishProcessingIfActive();
		UStashBlueprint::HandlePaymentFailure();
		EndSession(false);
	}
	else if (Path == TEXT("purchaseProcessing"))
	{
		UE_LOG(LogStashEditor, Log, TEXT("[StashPreview] Callback: purchaseProcessing"));
		Session.bIsPurchaseProcessing = true;
		UStashBlueprint::HandlePurchaseProcessing();
		RefreshAllPanels();
	}
	else if (Path == TEXT("optin"))
	{
		if (!Session.bIsOpen)
		{
			return;
		}
		const FString OptInType = ParseQueryParam(TEXT("type"));
		UE_LOG(LogStashEditor, Log, TEXT("[StashPreview] Callback: optin (type=%s)"), *OptInType);
		UStashBlueprint::HandleOptInResponse(OptInType);
		EndSession(false);
	}
	else if (Path == TEXT("externalBrowser"))
	{
		if (!Session.bIsOpen)
		{
			return;
		}
		const FString ExternalUrl = ParseQueryParam(TEXT("url"));
		UE_LOG(LogStashEditor, Log, TEXT("[StashPreview] Callback: externalBrowser (url=%s)"), *ExternalUrl);
		UStashBlueprint::HandleExternalPayment(ExternalUrl);
		EndSession(false);
	}
	else if (Path == TEXT("dismiss"))
	{
		UE_LOG(LogStashEditor, Log, TEXT("[StashPreview] Callback: dismiss"));
		if (!Session.bIsPurchaseProcessing)
		{
			EndSession(true);
		}
	}
	else if (Path == TEXT("expand"))
	{
		UE_LOG(LogStashEditor, Log, TEXT("[StashPreview] Callback: expand"));
		SetCardSheetExpandedFromSdk(true);
	}
	else if (Path == TEXT("collapse"))
	{
		UE_LOG(LogStashEditor, Log, TEXT("[StashPreview] Callback: collapse"));
		SetCardSheetExpandedFromSdk(false);
	}
	else if (Path == TEXT("processingCompleted"))
	{
		UE_LOG(LogStashEditor, Log, TEXT("[StashPreview] Callback: processingCompleted"));
		Session.bIsPurchaseProcessing = false;
		UStashBlueprint::HandleProcessingCompleted();
		RefreshAllPanels();
	}
	else if (Path == TEXT("keyboardShow"))
	{
		const FString InputType = ParseQueryParam(TEXT("type"));
		UE_LOG(LogStashEditor, Verbose, TEXT("[StashPreview] Callback: keyboardShow (type=%s)"), *InputType);
		SetKeyboardVisible(true, InputType);
	}
	else if (Path == TEXT("keyboardHide"))
	{
		UE_LOG(LogStashEditor, Verbose, TEXT("[StashPreview] Callback: keyboardHide"));
		SetKeyboardVisible(false, FString());
	}
	else
	{
		UE_LOG(LogStashEditor, Warning, TEXT("[StashPreview] Unknown callback path: %s"), *Path);
	}
}

void FStashEditorPreviewService::NotifyLoadCompleted()
{
	if (Session.bPageLoadedFired || !Session.bIsOpen)
	{
		return;
	}
	Session.bPageLoadedFired = true;
	const float LoadTimeMs = static_cast<float>((FApp::GetCurrentTime() - Session.LoadStartSeconds) * 1000.0);
	UStashBlueprint::HandlePageLoaded(LoadTimeMs);
}

void FStashEditorPreviewService::NotifyLoadError()
{
	if (!Session.bIsOpen)
	{
		return;
	}
	UStashBlueprint::HandleNetworkError();
	EndSession(false);
}

void FStashEditorPreviewService::SimulatePaymentSuccess()
{
	DispatchPreviewCallbackUrl(TEXT("stash-unreal-preview:///paymentSuccess"));
}

void FStashEditorPreviewService::SimulatePaymentFailure()
{
	DispatchPreviewCallbackUrl(TEXT("stash-unreal-preview:///paymentFailure"));
}

void FStashEditorPreviewService::SimulatePurchaseProcessing()
{
	DispatchPreviewCallbackUrl(TEXT("stash-unreal-preview:///purchaseProcessing"));
}

void FStashEditorPreviewService::SimulateProcessingCompleted()
{
	DispatchPreviewCallbackUrl(TEXT("stash-unreal-preview:///processingCompleted"));
}

void FStashEditorPreviewService::SimulateOptInResponse(const FString& OptInType)
{
	DispatchPreviewCallbackUrl(FString::Printf(TEXT("stash-unreal-preview:///optin?type=%s"), *FGenericPlatformHttp::UrlEncode(OptInType)));
}

void FStashEditorPreviewService::SimulateDismiss()
{
	if (Session.bIsOpen && !Session.bIsPurchaseProcessing)
	{
		EndSession(true);
	}
}

void FStashEditorPreviewService::SetCardSheetExpandedFromSdk(bool bExpanded)
{
	if (!Session.bIsOpen || Session.PresentationMode != EStashPreviewPresentationMode::Card)
	{
		return;
	}

	PreviewPanels.RemoveAll([](const TWeakPtr<SStashPreviewPanel>& Weak) { return !Weak.IsValid(); });
	for (const TWeakPtr<SStashPreviewPanel>& Weak : PreviewPanels)
	{
		if (TSharedPtr<SStashPreviewPanel> Panel = Weak.Pin())
		{
			Panel->SetCardSheetExpanded(bExpanded);
		}
	}
}
