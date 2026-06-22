// Copyright Stash. All Rights Reserved.

#include "StashEditorPreviewService.h"
#include "SStashPreviewPanel.h"
#include "StashEditorPreviewTab.h"
#include "StashPreviewJsBridge.h"
#include "StashEditorLog.h"
#include "StashBlueprint.h"
#include "StashEditorSettings.h"
#include "HAL/PlatformProcess.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "Misc/App.h"
#include "Modules/ModuleManager.h"

FStashPreviewDeviceSize StashPreviewGetDeviceSize(EStashPreviewDevicePreset Preset, float CustomWidth, float CustomHeight)
{
	switch (Preset)
	{
	case EStashPreviewDevicePreset::iPhoneSE:       return {375.f, 667.f};
	case EStashPreviewDevicePreset::iPhone14ProMax: return {430.f, 932.f};
	case EStashPreviewDevicePreset::iPhone14Pro:    return {393.f, 852.f};
	case EStashPreviewDevicePreset::iPad:           return {810.f, 1080.f};
	case EStashPreviewDevicePreset::iPadPro:        return {1024.f, 1366.f};
	case EStashPreviewDevicePreset::Custom:         return {CustomWidth, CustomHeight};
	case EStashPreviewDevicePreset::iPhone14:
	default:                                        return {390.f, 844.f};
	}
}

TSharedRef<FStashEditorPreviewService> FStashEditorPreviewService::Get()
{
	static TSharedRef<FStashEditorPreviewService> Instance = MakeShared<FStashEditorPreviewService>();
	return Instance;
}

bool FStashEditorPreviewService::CanUsePreview() const
{
	const UStashEditorSettings* Settings = GetDefault<UStashEditorSettings>();
	if (!Settings || !Settings->bEnableEditorPreview)
	{
		return false;
	}
#if !STASH_HAS_WEBBROWSER
	UE_LOG(LogStashEditor, Warning, TEXT("[StashPreview] Web Browser engine plugin is not installed in this Unreal build. Enable it under Edit → Plugins → Web Browser if available, then rebuild."));
	return false;
#else
	if (!FModuleManager::Get().IsModuleLoaded(TEXT("WebBrowser")))
	{
		if (!FModuleManager::Get().ModuleExists(TEXT("WebBrowser")))
		{
			UE_LOG(LogStashEditor, Warning, TEXT("[StashPreview] WebBrowser module not found. Enable Edit → Plugins → Web Browser and restart the editor."));
			return false;
		}
		FModuleManager::Get().LoadModule(TEXT("WebBrowser"));
	}
	return true;
#endif
}

bool FStashEditorPreviewService::IsAvailable() const
{
	return CanUsePreview();
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

bool FStashEditorPreviewService::BeginSession(const FString& URL, EStashPreviewPresentationMode Mode)
{
	if (!CanUsePreview())
	{
		return false;
	}
	const FString Normalized = NormalizeUrl(URL);
	if (Normalized.IsEmpty())
	{
		return false;
	}

	Session.bIsOpen = true;
	Session.bIsPurchaseProcessing = false;
	Session.CurrentUrl = Normalized;
	Session.PresentationMode = Mode;
	Session.LoadStartSeconds = FApp::GetCurrentTime();
	Session.bPageLoadedFired = false;

	EnsurePreviewTabOpen();
	RefreshAllPanels();
	UE_LOG(LogStashEditor, Log, TEXT("[StashPreview] Session started (%d): %s"), static_cast<int32>(Mode), *Normalized);
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
	RefreshAllPanels();
	if (bFireDismiss)
	{
		UStashBlueprint::HandleDialogDismissed();
	}
}

bool FStashEditorPreviewService::OpenCard(const FString& URL, const FStashCardConfig& Config)
{
	if (!BeginSession(URL, EStashPreviewPresentationMode::Card))
	{
		return false;
	}
	Session.CardConfig = Config;
	if (Config.AndroidCheckoutBackdrop.Num() > 0)
	{
		Session.BackdropBytes = Config.AndroidCheckoutBackdrop;
	}
	return true;
}

bool FStashEditorPreviewService::OpenModal(const FString& URL, const FStashModalConfig& Config)
{
	if (!BeginSession(URL, EStashPreviewPresentationMode::Modal))
	{
		return false;
	}
	Session.ModalConfig = Config;
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
	return BeginSession(URL, EStashPreviewPresentationMode::Browser);
}

bool FStashEditorPreviewService::CloseBrowser()
{
	if (Session.bIsOpen && Session.PresentationMode == EStashPreviewPresentationMode::Browser)
	{
		EndSession(true);
		return true;
	}
	return false;
}

bool FStashEditorPreviewService::DismissCard()
{
	if (!Session.bIsOpen)
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
	return Session.bIsOpen;
}

bool FStashEditorPreviewService::IsPurchaseProcessing() const
{
	return Session.bIsPurchaseProcessing;
}

void FStashEditorPreviewService::SetLandscapeLockWhenCardClosed(bool bEnable)
{
	Session.bLandscapeLockWhenCardClosed = bEnable;
	UE_LOG(LogStashEditor, Log, TEXT("[StashPreview] SetLandscapeLockWhenCardClosed (editor stub): %d"), bEnable ? 1 : 0);
	RefreshAllPanels();
}

void FStashEditorPreviewService::SetAndroidKeepAliveEnabled(bool bEnabled)
{
	Session.bKeepAliveEnabled = bEnabled;
	UE_LOG(LogStashEditor, Log, TEXT("[StashPreview] SetAndroidKeepAliveEnabled (editor stub): %d"), bEnabled ? 1 : 0);
	RefreshAllPanels();
}

void FStashEditorPreviewService::SetAndroidKeepAliveConfig(const FStashKeepAliveConfig& Config)
{
	Session.KeepAliveConfig = Config;
	UE_LOG(LogStashEditor, Log, TEXT("[StashPreview] SetAndroidKeepAliveConfig (editor stub): title=%s"), *Config.NotificationTitle);
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

void FStashEditorPreviewService::NotifyUrlChanged(const FString& Url)
{
	if (!Url.StartsWith(StashPreviewJsBridge::SchemePrefix))
	{
		return;
	}

	FString PathAndQuery = Url.Mid(FCString::Strlen(StashPreviewJsBridge::SchemePrefix));
	FString Path;
	FString Query;
	PathAndQuery.Split(TEXT("?"), &Path, &Query);

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

	if (Path == TEXT("paymentSuccess"))
	{
		Session.bIsPurchaseProcessing = false;
		UStashBlueprint::HandlePaymentSuccess();
		EndSession(false);
	}
	else if (Path == TEXT("paymentFailure"))
	{
		Session.bIsPurchaseProcessing = false;
		UStashBlueprint::HandlePaymentFailure();
		EndSession(false);
	}
	else if (Path == TEXT("purchaseProcessing"))
	{
		Session.bIsPurchaseProcessing = true;
		RefreshAllPanels();
	}
	else if (Path == TEXT("optin"))
	{
		const FString OptInType = ParseQueryParam(TEXT("type"));
		UStashBlueprint::HandleOptInResponse(OptInType);
		EndSession(false);
	}
	else if (Path == TEXT("externalBrowser"))
	{
		const FString ExternalUrl = ParseQueryParam(TEXT("url"));
		UStashBlueprint::HandleExternalPayment(ExternalUrl);
		EndSession(false);
	}
	else if (Path == TEXT("dismiss"))
	{
		if (!Session.bIsPurchaseProcessing)
		{
			EndSession(true);
		}
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
	NotifyUrlChanged(TEXT("stash-unreal-preview://paymentSuccess"));
}

void FStashEditorPreviewService::SimulatePaymentFailure()
{
	NotifyUrlChanged(TEXT("stash-unreal-preview://paymentFailure"));
}

void FStashEditorPreviewService::SimulateOptInResponse(const FString& OptInType)
{
	NotifyUrlChanged(FString::Printf(TEXT("stash-unreal-preview://optin?type=%s"), *FGenericPlatformHttp::UrlEncode(OptInType)));
}

void FStashEditorPreviewService::SimulateDismiss()
{
	if (Session.bIsOpen && !Session.bIsPurchaseProcessing)
	{
		EndSession(true);
	}
}
