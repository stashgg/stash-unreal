// Copyright Stash. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "Widgets/SCompoundWidget.h"
#include "StashEditorSettings.h"
#include "StashPreviewDeviceCatalog.h"
#include "StashPreviewLayout.h"
#include "StashEditorPreviewService.h"

class SWebBrowser;
class SStashPreviewDraggableSheet;
class SStashPreviewKeyboard;
struct FSlateRoundedBoxBrush;
struct FWebNavigationRequest;
enum class EWebBrowserConsoleLogSeverity;

class SStashPreviewPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SStashPreviewPanel) {}
	SLATE_END_ARGS()

	virtual ~SStashPreviewPanel() override;

	void Construct(const FArguments& InArgs);
	void RefreshFromSession();
	void SetCardSheetExpanded(bool bExpanded);

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

private:
	static constexpr float DragHandleChromeHeight = 18.f;
	static constexpr float CardMaxExpandHeightRatio = 0.9f;
	static constexpr float DeviceBezelThickness = 12.f;
	static constexpr float BackdropFlashSeconds = 0.25f;
	/** How long a load may run before we surface a load error if OnLoadCompleted never fires. */
	static constexpr float LoadTimeoutSeconds = 5.f;

	FReply OnReloadClicked();
	FReply OnDismissClicked();
	FReply OnAndroidBackClicked();
	FReply OnToggleKeyboardClicked();
	FReply OnSimulateSuccessClicked();
	FReply OnSimulateFailureClicked();
	FReply OnSimulateProcessingClicked();
	FReply OnSimulateProcessingCompletedClicked();
	FReply OnSimulateDismissClicked();
	FReply OnSimulateOptInClicked();
	FReply OnDimOverlayClicked();
	void HandleDragDismiss();
	void HandleCardSheetHeightDrag(float ProvisionalHeight);
	void HandleCardSheetDragEnded(float FinalHeight, bool bDismissRequested);
	void ApplyCardSheetExpandedState(bool bExpanded);
	void OnDevicePresetChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	void HandleUrlChanged(const FText& InText);
	bool HandleBeforePreviewNavigation(const FString& Url, const FWebNavigationRequest& Request);
	/** @param bRestoreAfterBlank true only for the OnLoadUrl scheme-handler path, which blanks the webview. */
	void HandlePreviewSchemeNavigation(const FString& Url, bool bRestoreAfterBlank = false);
	bool HandlePreviewLoadUrl(const FString& Method, const FString& Url, FString& OutResponse);
	void HandlePreviewConsoleMessage(const FString& Message, const FString& Source, int32 Line, EWebBrowserConsoleLogSeverity Severity);
	void HandleLoadCompleted();
	void InjectStashSdkScript();
	/** Reset the load-progress flags and arm the load-failure timer for a load that is about to start. */
	void ArmLoadTimer();
	/** Reset load-progress flags, arm the load-failure timer, and (re)load Url in the existing browser. */
	void StartLoad(const FString& Url);
	/** Push the selected device's mobile-emulation params (UA / mobile flag / platform) to the service; optionally reload. */
	void PushDeviceEmulation(bool bReload);
	void EnsureBrowserForUrl(const FString& Url);
	void RebuildPreviewChrome();
	void RebuildDeviceChromeBrushes();
	void UpdateBackdropBrush(const FStashPreviewSession& Session);
	void UpdateSheetBrushes();
	FLinearColor ParseBackgroundColor(const FString& HtmlHex) const;
	FStashPreviewSheetLayout ComputeCurrentLayout() const;
	FStashPreviewSheetLayout ComputeCardBaseLayout() const;
	float GetCardMaxExpandedHeight() const;
	FMargin ComputeSheetPadding(const FStashPreviewSheetLayout& Layout) const;
	FVector2D GetWebViewportSize() const;
	TSharedRef<SWidget> BuildSheetInnerChrome(
		TSharedPtr<SBorder>& OutBorder,
		TSharedPtr<SBox>& OutWebBox,
		bool bIncludeDragHandle);
	TSharedRef<SWidget> BuildControlsPanel();
	TSharedRef<SWidget> BuildDevicePreviewArea();
	TSharedRef<SWidget> BuildDeviceChromeOverlays();
	/** Resolved device spec for the current preset + settings. Cached; refreshed each Tick and on preset change. */
	const FStashPreviewDeviceSpec& GetSelectedDeviceSpec() const;
	/** Recompute CachedDeviceSpec from the current preset and Project Settings (Custom device dims/platform). */
	void RefreshCachedDeviceSpec();
	FVector2D GetSelectedDeviceSize() const;
	FVector2D GetDeviceFrameSize() const;
	FMargin GetEffectiveInsets() const;
	/** True when the content beneath the status-bar inset is light (browser checkout page) → dark glyphs. */
	bool IsStatusContentLight() const;
	FLinearColor GetStatusGlyphColor() const;
	/** Width of the top-center cutout, used to split the status bar into left/right ears (0 = no gap). */
	float GetStatusCutoutWidth() const;
	bool ShouldShowCellular() const;
	float GetKeyboardHeight() const;
	/** Padding under the drawer-card web content: home indicator / gesture inset, or the keyboard when visible. */
	float GetWebContentBottomInset() const;
	bool IsDeviceChromeEnabled() const;
	bool IsAndroidPreset() const;
	bool IsTabletDevice() const;
	bool GetEffectiveLandscape() const;
	bool IsCardDragDismissEnabled() const;
	bool IsCardExpandDragEnabled() const;
	bool IsCardPresentation() const;
	bool IsModalPresentation() const;
	SBox* GetActiveWebContentBox() const;

	TSharedPtr<SWebBrowser> WebBrowser;
	TSharedPtr<SBox> PreviewHost;
	TSharedPtr<SOverlay> PreviewOverlay;
	TSharedPtr<SBox> DeviceFrameBox;
	TSharedPtr<SBox> SheetAlignBox;
	TSharedPtr<SBox> CardSheetHost;
	TSharedPtr<SStashPreviewDraggableSheet> DraggableSheet;
	TSharedPtr<SBorder> CardSheetBorder;
	TSharedPtr<SBox> CardWebContentBox;
	TSharedPtr<SBox> ModalSheetHost;
	TSharedPtr<SBorder> ModalSheetBorder;
	TSharedPtr<SBox> ModalWebContentBox;

	TSharedPtr<SImage> BackdropImage;

	TArray<TSharedPtr<FString>> DevicePresetOptions;
	TSharedPtr<FString> SelectedDevicePreset;
	EStashPreviewDevicePreset DevicePreset = EStashPreviewDevicePreset::iPhone14;
	/** Cached resolved spec — avoids reconstructing an FString-bearing struct + catalog scan per Slate attribute lambda. */
	FStashPreviewDeviceSpec CachedDeviceSpec;
	/** Last platform pushed to the preview service; used to detect Custom-platform edits made in Project Settings. */
	EStashPreviewPlatform LastSyncedActivePlatform = EStashPreviewPlatform::iOS;
	/** Last user-agent THIS panel pushed to the service; compared against the panel's own spec (not the
	 *  service-global UA) so multiple panels can't ping-pong reloads against each other. */
	FString LastPushedUserAgent;
	bool bDeviceLandscape = false;
	/** Landscape state to restore when a force-portrait session ends. */
	bool bRestoreLandscapeAfterSession = false;
	/** Latches a forced-portrait rotation episode so the white-flash fires once, not on every session refresh. */
	bool bForcePortraitEpisodeActive = false;
	bool bLastKeyboardVisible = false;
	/** Set when the keyboard auto-expanded a drawer card, so it collapses back when the keyboard hides. */
	bool bAutoExpandedForKeyboard = false;
	/** Android force-portrait rotation without a backdrop: brief white flash (teaches why the backdrop API exists). */
	float BackdropFlashRemaining = -1.f;
	bool bShowBackdropFlashHint = false;

	FString LastLoadedUrl;
	float NetworkTimeoutRemaining = -1.f;
	FVector2D LastSyncedViewportSize = FVector2D::ZeroVector;
	EStashPreviewPresentationMode LastPresentationMode = EStashPreviewPresentationMode::Card;

	TSharedPtr<FSlateBrush> BackdropBrush;
	TSharedPtr<FSlateRoundedBoxBrush> SheetBrush;
	TSharedPtr<FSlateRoundedBoxBrush> BezelBrush;
	TSharedPtr<FSlateRoundedBoxBrush> NotchBrush;
	TSharedPtr<FSlateRoundedBoxBrush> NotchBrushLandscape;
	TSharedPtr<FSlateRoundedBoxBrush> HomeIndicatorBrush;
	TSharedPtr<FSlateRoundedBoxBrush> KeepAliveBrush;
	TSharedPtr<FSlateRoundedBoxBrush> KeepAliveIconBrush;
	FStashPreviewSheetLayout ActiveLayout;
	FLinearColor ActiveShellColor = FLinearColor(0.12f, 0.12f, 0.14f, 1.f);
	bool bShowDimOverlay = false;
	bool bDimDismissible = false;
	bool bShowDragHandle = false;
	bool bCardExpandedToMax = false;
	float CardDragHeightOverride = 0.f;
};
