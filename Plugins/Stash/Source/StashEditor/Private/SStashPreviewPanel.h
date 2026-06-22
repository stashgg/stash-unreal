// Copyright Stash. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "Widgets/SCompoundWidget.h"
#include "StashEditorSettings.h"

class SWebBrowser;
class SStashPreviewPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SStashPreviewPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void RefreshFromSession();

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	FReply OnReloadClicked();
	FReply OnDismissClicked();
	FReply OnSimulateSuccessClicked();
	FReply OnSimulateFailureClicked();
	FReply OnSimulateDismissClicked();
	FReply OnSimulateOptInClicked();
	void OnDevicePresetChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	void HandleUrlChanged(const FText& InText);
	void HandleLoadCompleted();
	void InjectStashSdkScript();
	void EnsureBrowserForUrl(const FString& Url);
	void RebuildPreviewChrome();
	FLinearColor ParseBackgroundColor(const FString& HtmlHex) const;
	TSharedRef<SWidget> BuildControlsPanel();
	TSharedRef<SWidget> BuildDevicePreviewArea();
	FVector2D GetSelectedDeviceSize() const;
	bool IsLandscapeDevice() const { return bDeviceLandscape; }

	TSharedPtr<SWebBrowser> WebBrowser;
	TSharedPtr<SBox> PreviewHost;
	TSharedPtr<SOverlay> PreviewOverlay;
	TSharedPtr<SBox> SheetBox;
	TSharedPtr<SImage> BackdropImage;
	TSharedPtr<SBorder> DimOverlay;

	TArray<TSharedPtr<FString>> DevicePresetOptions;
	TSharedPtr<FString> SelectedDevicePreset;
	EStashPreviewDevicePreset DevicePreset = EStashPreviewDevicePreset::iPhone14;
	bool bDeviceLandscape = false;

	FString LastLoadedUrl;
	float NetworkTimeoutRemaining = -1.f;

	TSharedPtr<FSlateBrush> BackdropBrush;
};
