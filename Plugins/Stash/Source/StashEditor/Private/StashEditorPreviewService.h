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
	FStashKeepAliveConfig KeepAliveConfig;
	TArray<uint8> BackdropBytes;
	FString CurrentUrl;
	EStashPreviewPresentationMode PresentationMode = EStashPreviewPresentationMode::Card;
	FStashCardConfig CardConfig;
	FStashModalConfig ModalConfig;
	double LoadStartSeconds = 0.0;
	bool bPageLoadedFired = false;
};

/** Device preset dimensions (points, portrait). */
struct FStashPreviewDeviceSize
{
	float Width = 390.f;
	float Height = 844.f;
};

FStashPreviewDeviceSize StashPreviewGetDeviceSize(EStashPreviewDevicePreset Preset, float CustomWidth, float CustomHeight);

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
	void SimulatePaymentSuccess();
	void SimulatePaymentFailure();
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
};
