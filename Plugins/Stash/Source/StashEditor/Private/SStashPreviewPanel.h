// Copyright Stash. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "Widgets/SCompoundWidget.h"
#include "StashEditorSettings.h"
#include "StashPreviewLayout.h"
#include "StashEditorPreviewService.h"

class SWebBrowser;
class SStashPreviewDraggableSheet;
struct FSlateRoundedBoxBrush;
struct FWebNavigationRequest;
enum class EWebBrowserConsoleLogSeverity;

class SStashPreviewPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SStashPreviewPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void RefreshFromSession();
	void SetCardSheetExpanded(bool bExpanded);

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	static constexpr float DragHandleChromeHeight = 18.f;
	static constexpr float CardMaxExpandHeightRatio = 0.9f;

	FReply OnReloadClicked();
	FReply OnDismissClicked();
	FReply OnSimulateSuccessClicked();
	FReply OnSimulateFailureClicked();
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
	void HandlePreviewSchemeNavigation(const FString& Url);
	bool HandlePreviewLoadUrl(const FString& Method, const FString& Url, FString& OutResponse);
	void HandlePreviewConsoleMessage(const FString& Message, const FString& Source, int32 Line, EWebBrowserConsoleLogSeverity Severity);
	void HandleLoadCompleted();
	void InjectStashSdkScript();
	void EnsureBrowserForUrl(const FString& Url);
	void RebuildPreviewChrome();
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
	FVector2D GetSelectedDeviceSize() const;
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
	bool bDeviceLandscape = false;

	FString LastLoadedUrl;
	float NetworkTimeoutRemaining = -1.f;
	FVector2D LastSyncedViewportSize = FVector2D::ZeroVector;
	EStashPreviewPresentationMode LastPresentationMode = EStashPreviewPresentationMode::Card;

	TSharedPtr<FSlateBrush> BackdropBrush;
	TSharedPtr<FSlateRoundedBoxBrush> SheetBrush;
	FStashPreviewSheetLayout ActiveLayout;
	FLinearColor ActiveShellColor = FLinearColor(0.12f, 0.12f, 0.14f, 1.f);
	bool bShowDimOverlay = false;
	bool bDimDismissible = false;
	bool bShowDragHandle = false;
	bool bCardExpandedToMax = false;
	float CardDragHeightOverride = 0.f;
};
