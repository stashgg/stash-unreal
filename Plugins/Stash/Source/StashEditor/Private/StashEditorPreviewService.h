// Copyright Stash. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StashEditorPreviewBridge.h"
#include "StashEditorSettings.h"
#include "StashBlueprint.h"

struct FStashPreviewSession
{
	bool bIsOpen = false;
	bool bIsPurchaseProcessing = false;
	bool bLandscapeLockWhenCardClosed = false;
	bool bKeepAliveEnabled = false;
	bool bForcePortraitLayout = false;
	bool bAllowDismiss = true;
	/** Platform of the selected preview device; drives Close Browser / back button / keep-alive semantics. */
	EStashPreviewPlatform ActivePlatform = EStashPreviewPlatform::iOS;
	/** Simulated soft keyboard (driven by checkout page focus events or the manual toggle). */
	bool bKeyboardVisible = false;
	FString KeyboardInputType;
	FStashKeepAliveConfig KeepAliveConfig;
	TArray<uint8> BackdropBytes;
	FString CurrentUrl;
	EStashPreviewPresentationMode PresentationMode = EStashPreviewPresentationMode::Card;
	FStashCardConfig CardConfig;
	FStashModalConfig ModalConfig;
	double LoadStartSeconds = 0.0;
	bool bPageLoadedFired = false;
};

class SStashPreviewPanel;

/**
 * Editor-only service: session state, bridge implementation, and preview panel hosting.
 */
class FStashEditorPreviewService : public IStashEditorPreviewBridge, public TSharedFromThis<FStashEditorPreviewService>
{
public:
	static TSharedRef<FStashEditorPreviewService> Get();

	FStashEditorPreviewService() = default;

	virtual bool IsAvailable() const override;
	virtual bool OpenCard(const FString& URL, const FStashCardConfig& Config) override;
	virtual bool OpenModal(const FString& URL, const FStashModalConfig& Config) override;
	virtual bool OpenBrowser(const FString& URL) override;
	virtual bool CloseBrowser() override;
	virtual bool DismissCard() override;
	virtual bool IsCardOpen() const override;
	virtual bool IsPurchaseProcessing() const override;
	virtual void SetLandscapeLockWhenCardClosed(bool bEnable) override;
	virtual void SetAndroidKeepAliveEnabled(bool bEnabled) override;
	virtual void SetAndroidKeepAliveConfig(const FStashKeepAliveConfig& Config) override;
	virtual void SetAndroidCheckoutBackdropBytes(const TArray<uint8>& ImageBytes) override;
	virtual void ClearAndroidCheckoutBackdrop() override;

	void RegisterPreviewPanel(const TSharedPtr<SStashPreviewPanel>& Panel);
	void UnregisterPreviewPanel(const TSharedPtr<SStashPreviewPanel>& Panel);

	const FStashPreviewSession& GetSession() const { return Session; }
	FStashPreviewSession& GetMutableSession() { return Session; }

	void NotifyUrlChanged(const FString& Url);
	/** Parses, deduplicates, and dispatches a preview callback URL from any bridge (CEF scheme, console, navigation). */
	bool DispatchPreviewCallbackUrl(const FString& Url);
	void NotifyLoadCompleted();
	void NotifyLoadError();
	void SetCardSheetExpandedFromSdk(bool bExpanded);
	/** Panel writes through the platform of the selected device preset. */
	void SetActivePlatform(EStashPreviewPlatform Platform);
	/** Panel writes through the selected device's mobile emulation params; the CEF request-context delegate reads these. */
	void SetPreviewDeviceEmulation(const FString& UserAgent, bool bMobile, const FString& PlatformHint);
	const FString& GetPreviewUserAgent() const { return PreviewUserAgent; }
	bool IsPreviewMobile() const { return bPreviewMobile; }
	const FString& GetPreviewPlatformHint() const { return PreviewPlatformHint; }
	/** Simulated Android back gesture: hides the keyboard first, then dismisses like the device back button. */
	void HandleAndroidBack();
	void SetKeyboardVisible(bool bVisible, const FString& InputType);
	void SimulatePaymentSuccess();
	void SimulatePaymentFailure();
	void SimulatePurchaseProcessing();
	void SimulateProcessingCompleted();
	void SimulateOptInResponse(const FString& OptInType);
	void SimulateDismiss();

	void EnsurePreviewTabOpen();

	bool CanUsePreview() const;
	bool PrepareSession(const FString& URL, EStashPreviewPresentationMode Mode);
	void CommitSession();
	void EndSession(bool bFireDismiss);
	void RefreshAllPanels();
	FString NormalizeUrl(const FString& URL) const;

	FStashPreviewSession Session;
	TArray<TWeakPtr<SStashPreviewPanel>> PreviewPanels;

	FString LastPreviewCallbackDedupKey;
	double LastPreviewCallbackTime = -1.0;
	static constexpr double PreviewCallbackDedupSeconds = 0.25;

	/** Mobile emulation for the CEF webview — device-scoped (not per session), read by the request-context header injector. */
	FString PreviewUserAgent;
	bool bPreviewMobile = true;
	FString PreviewPlatformHint = TEXT("iOS");
};
