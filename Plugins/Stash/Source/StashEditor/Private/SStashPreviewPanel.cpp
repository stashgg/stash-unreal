// Copyright Stash. All Rights Reserved.
#include "SStashPreviewPanel.h"
#include "SStashPreviewDraggableSheet.h"
#include "StashEditorPreviewService.h"
#include "StashPreviewJsBridge.h"
#include "StashPreviewCallbackUrl.h"
#include "StashPreviewLayout.h"
#include "StashPreviewSchemeHandler.h"
#include "StashEditorSettings.h"
#include "StashEditorLog.h"
#include "Misc/App.h"
#include "Misc/Parse.h"
#include "HAL/PlatformTime.h"
#include "Brushes/SlateDynamicImageBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Modules/ModuleManager.h"
#if STASH_HAS_WEBBROWSER
#include "SWebBrowser.h"
#include "WebBrowserModule.h"
#endif
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/AppStyle.h"
namespace
{
	FString ModeLabel(EStashPreviewPresentationMode Mode)
	{
		switch (Mode)
		{
		case EStashPreviewPresentationMode::Modal:   return TEXT("Modal");
		case EStashPreviewPresentationMode::Browser: return TEXT("Browser");
		default:                                     return TEXT("Card");
		}
	}
	EStashPreviewDevicePreset PresetFromLabel(const FString& Label)
	{
		if (Label.Contains(TEXT("SE"))) return EStashPreviewDevicePreset::iPhoneSE;
		if (Label.Contains(TEXT("Pro Max"))) return EStashPreviewDevicePreset::iPhone14ProMax;
		if (Label.Contains(TEXT("Pro"))) return EStashPreviewDevicePreset::iPhone14Pro;
		if (Label.Contains(TEXT("iPad Pro"))) return EStashPreviewDevicePreset::iPadPro;
		if (Label.Contains(TEXT("iPad"))) return EStashPreviewDevicePreset::iPad;
		if (Label.Contains(TEXT("Custom"))) return EStashPreviewDevicePreset::Custom;
		return EStashPreviewDevicePreset::iPhone14;
	}
}
void SStashPreviewPanel::Construct(const FArguments& InArgs)
{
	const UStashEditorSettings* Settings = GetDefault<UStashEditorSettings>();
	DevicePreset = Settings ? Settings->DefaultDevicePreset : EStashPreviewDevicePreset::iPhone14;
	DevicePresetOptions = {
		MakeShared<FString>(TEXT("iPhone SE (375x667)")),
		MakeShared<FString>(TEXT("iPhone 14 (390x844)")),
		MakeShared<FString>(TEXT("iPhone 14 Pro Max (430x932)")),
		MakeShared<FString>(TEXT("iPhone 14 Pro (393x852)")),
		MakeShared<FString>(TEXT("iPad (810x1080)")),
		MakeShared<FString>(TEXT("iPad Pro (1024x1366)")),
		MakeShared<FString>(TEXT("Custom"))
	};
	for (const TSharedPtr<FString>& Opt : DevicePresetOptions)
	{
		if (PresetFromLabel(*Opt) == DevicePreset)
		{
			SelectedDevicePreset = Opt;
			break;
		}
	}
	if (!SelectedDevicePreset.IsValid())
	{
		SelectedDevicePreset = DevicePresetOptions[1];
	}
	FStashEditorPreviewService::Get()->RegisterPreviewPanel(SharedThis(this));
	ChildSlot
	[
		SNew(SSplitter)
		.Orientation(Orient_Horizontal)
		+ SSplitter::Slot()
		.Value(0.28f)
		[
			BuildControlsPanel()
		]
		+ SSplitter::Slot()
		.Value(0.72f)
		[
			BuildDevicePreviewArea()
		]
	];
	RefreshFromSession();
}
void SStashPreviewPanel::RefreshFromSession()
{
	const FStashPreviewSession& Session = FStashEditorPreviewService::Get()->GetSession();
	if (Session.bIsOpen && Session.bForcePortraitLayout)
	{
		bDeviceLandscape = false;
	}
	const EStashPreviewPresentationMode PreviousMode = LastPresentationMode;
	if (Session.bIsOpen
		&& Session.PresentationMode == EStashPreviewPresentationMode::Card
		&& (PreviousMode != EStashPreviewPresentationMode::Card || LastLoadedUrl != Session.CurrentUrl))
	{
		bCardExpandedToMax = false;
		CardDragHeightOverride = 0.f;
	}
	RebuildPreviewChrome();
	if (Session.bIsOpen && !Session.CurrentUrl.IsEmpty())
	{
		if (PreviousMode != Session.PresentationMode)
		{
			LastSyncedViewportSize = FVector2D::ZeroVector;
		}
		EnsureBrowserForUrl(Session.CurrentUrl);
		NetworkTimeoutRemaining = 5.f;
	}
	else
	{
		NetworkTimeoutRemaining = -1.f;
		LastLoadedUrl.Reset();
		LastSyncedViewportSize = FVector2D::ZeroVector;
		LastPresentationMode = EStashPreviewPresentationMode::Card;
		if (WebBrowser.IsValid())
		{
			WebBrowser.Reset();
		}
		if (CardWebContentBox.IsValid())
		{
			CardWebContentBox->SetContent(SNew(SBox));
		}
		if (ModalWebContentBox.IsValid())
		{
			ModalWebContentBox->SetContent(SNew(SBox));
		}
	}
}
bool SStashPreviewPanel::IsTabletDevice() const
{
	const UStashEditorSettings* Settings = GetDefault<UStashEditorSettings>();
	const float CustomW = Settings ? Settings->CustomDeviceWidth : 390.f;
	const float CustomH = Settings ? Settings->CustomDeviceHeight : 844.f;
	return StashPreviewIsTabletDevice(DevicePreset, CustomW, CustomH);
}
bool SStashPreviewPanel::GetEffectiveLandscape() const
{
	const FStashPreviewSession& Session = FStashEditorPreviewService::Get()->GetSession();
	if (Session.bForcePortraitLayout)
	{
		return false;
	}
	return bDeviceLandscape;
}
bool SStashPreviewPanel::IsCardDragDismissEnabled() const
{
	const FStashPreviewSession& Session = FStashEditorPreviewService::Get()->GetSession();
	return Session.bIsOpen
		&& Session.PresentationMode == EStashPreviewPresentationMode::Card
		&& !Session.bIsPurchaseProcessing;
}

bool SStashPreviewPanel::IsCardExpandDragEnabled() const
{
	if (!IsCardDragDismissEnabled())
	{
		return false;
	}
	return ComputeCardBaseLayout().bCardBottomDrawer;
}

FStashPreviewSheetLayout SStashPreviewPanel::ComputeCardBaseLayout() const
{
	const FStashPreviewSession& Session = FStashEditorPreviewService::Get()->GetSession();
	const FVector2D DeviceSize = GetSelectedDeviceSize();
	return StashPreviewComputeCardLayout(
		Session.CardConfig,
		DeviceSize.X,
		DeviceSize.Y,
		GetEffectiveLandscape(),
		IsTabletDevice());
}

float SStashPreviewPanel::GetCardMaxExpandedHeight() const
{
	return GetSelectedDeviceSize().Y * CardMaxExpandHeightRatio;
}
bool SStashPreviewPanel::IsCardPresentation() const
{
	const FStashPreviewSession& Session = FStashEditorPreviewService::Get()->GetSession();
	return Session.bIsOpen && Session.PresentationMode == EStashPreviewPresentationMode::Card;
}

bool SStashPreviewPanel::IsModalPresentation() const
{
	const FStashPreviewSession& Session = FStashEditorPreviewService::Get()->GetSession();
	return Session.bIsOpen && Session.PresentationMode == EStashPreviewPresentationMode::Modal;
}

SBox* SStashPreviewPanel::GetActiveWebContentBox() const
{
	if (IsModalPresentation() && ModalWebContentBox.IsValid())
	{
		return ModalWebContentBox.Get();
	}
	if (CardWebContentBox.IsValid())
	{
		return CardWebContentBox.Get();
	}
	return nullptr;
}

FStashPreviewSheetLayout SStashPreviewPanel::ComputeCurrentLayout() const
{
	const FStashPreviewSession& Session = FStashEditorPreviewService::Get()->GetSession();
	if (!Session.bIsOpen)
	{
		return FStashPreviewSheetLayout();
	}

	const FVector2D DeviceSize = GetSelectedDeviceSize();
	const float DeviceW = DeviceSize.X;
	const float DeviceH = DeviceSize.Y;
	const bool bIsTablet = IsTabletDevice();
	const bool bEffectiveLandscape = GetEffectiveLandscape();

	if (Session.PresentationMode == EStashPreviewPresentationMode::Browser)
	{
		FStashPreviewSheetLayout Layout;
		Layout.Width = DeviceW;
		Layout.Height = DeviceH;
		Layout.HAlign = HAlign_Fill;
		Layout.VAlign = VAlign_Fill;
		return Layout;
	}
	if (Session.PresentationMode == EStashPreviewPresentationMode::Card)
	{
		FStashPreviewSheetLayout Layout = ComputeCardBaseLayout();
		if (Layout.bCardBottomDrawer)
		{
			const float BaseHeight = Layout.Height;
			const float MaxHeight = GetCardMaxExpandedHeight();
			if (CardDragHeightOverride > 0.f)
			{
				Layout.Height = FMath::Clamp(CardDragHeightOverride, BaseHeight, MaxHeight);
			}
			else if (bCardExpandedToMax)
			{
				Layout.Height = MaxHeight;
			}
		}
		return Layout;
	}
	return StashPreviewComputeModalLayout(
		Session.ModalConfig, DeviceW, DeviceH, bEffectiveLandscape, bIsTablet);
}

FMargin SStashPreviewPanel::ComputeSheetPadding(const FStashPreviewSheetLayout& Layout) const
{
	const FVector2D DeviceSize = GetSelectedDeviceSize();
	const float DeviceW = DeviceSize.X;
	const float DeviceH = DeviceSize.Y;
	FMargin Padding(0.f);
	if (Layout.VAlign == VAlign_Bottom)
	{
		Padding.Top = FMath::Max(0.f, DeviceH - Layout.Height);
	}
	else if (Layout.VAlign == VAlign_Center)
	{
		const float VerticalInset = FMath::Max(0.f, (DeviceH - Layout.Height) * 0.5f);
		Padding.Top = VerticalInset;
		Padding.Bottom = VerticalInset;
	}
	if (Layout.HAlign == HAlign_Center)
	{
		const float HorizontalInset = FMath::Max(0.f, (DeviceW - Layout.Width) * 0.5f);
		Padding.Left = HorizontalInset;
		Padding.Right = HorizontalInset;
	}
	return Padding;
}

FVector2D SStashPreviewPanel::GetWebViewportSize() const
{
	const FStashPreviewSession& Session = FStashEditorPreviewService::Get()->GetSession();
	if (!Session.bIsOpen)
	{
		return FVector2D::ZeroVector;
	}
	if (Session.PresentationMode == EStashPreviewPresentationMode::Browser)
	{
		return GetSelectedDeviceSize();
	}
	const FStashPreviewSheetLayout Layout = ComputeCurrentLayout();
	float WebW = Layout.Width;
	float WebH = Layout.Height;
	if (Layout.bShowDragHandle
		&& FStashEditorPreviewService::Get()->GetSession().PresentationMode == EStashPreviewPresentationMode::Card)
	{
		WebH = FMath::Max(1.f, WebH - DragHandleChromeHeight);
	}
	return FVector2D(WebW, WebH);
}
FReply SStashPreviewPanel::OnDimOverlayClicked()
{
	const FStashPreviewSession& Session = FStashEditorPreviewService::Get()->GetSession();
	if (Session.PresentationMode == EStashPreviewPresentationMode::Modal
		&& Session.bAllowDismiss
		&& !Session.bIsPurchaseProcessing)
	{
		FStashEditorPreviewService::Get()->SimulateDismiss();
	}
	return FReply::Handled();
}
void SStashPreviewPanel::HandleDragDismiss()
{
	FStashEditorPreviewService::Get()->SimulateDismiss();
}

void SStashPreviewPanel::SetCardSheetExpanded(bool bExpanded)
{
	if (!IsCardExpandDragEnabled())
	{
		return;
	}
	ApplyCardSheetExpandedState(bExpanded);
}

void SStashPreviewPanel::ApplyCardSheetExpandedState(bool bExpanded)
{
	bCardExpandedToMax = bExpanded;
	CardDragHeightOverride = 0.f;
	LastSyncedViewportSize = FVector2D::ZeroVector;

	if (PreviewOverlay.IsValid())
	{
		PreviewOverlay->Invalidate(EInvalidateWidget::LayoutAndVolatility);
	}
	if (DraggableSheet.IsValid())
	{
		DraggableSheet->Invalidate(EInvalidateWidget::LayoutAndVolatility);
	}
}

void SStashPreviewPanel::HandleCardSheetHeightDrag(float ProvisionalHeight)
{
	const FStashPreviewSheetLayout BaseLayout = ComputeCardBaseLayout();
	const float BaseHeight = BaseLayout.Height;
	const float MaxHeight = GetCardMaxExpandedHeight();
	const float ClampedHeight = FMath::Clamp(ProvisionalHeight, BaseHeight, MaxHeight);

	CardDragHeightOverride = ClampedHeight;
	bCardExpandedToMax = false;
	LastSyncedViewportSize = FVector2D::ZeroVector;

	if (PreviewOverlay.IsValid())
	{
		PreviewOverlay->Invalidate(EInvalidateWidget::LayoutAndVolatility);
	}
	if (DraggableSheet.IsValid())
	{
		DraggableSheet->Invalidate(EInvalidateWidget::LayoutAndVolatility);
	}
}

void SStashPreviewPanel::HandleCardSheetDragEnded(float FinalHeight, bool bDismissRequested)
{
	if (bDismissRequested)
	{
		bCardExpandedToMax = false;
		CardDragHeightOverride = 0.f;
		return;
	}

	const FStashPreviewSheetLayout BaseLayout = ComputeCardBaseLayout();
	const float BaseHeight = BaseLayout.Height;
	const float MaxHeight = GetCardMaxExpandedHeight();
	const float SnapThreshold = BaseHeight + (MaxHeight - BaseHeight) * 0.5f;

	ApplyCardSheetExpandedState(FinalHeight >= SnapThreshold);
}
void SStashPreviewPanel::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	const FStashPreviewSession& Session = FStashEditorPreviewService::Get()->GetSession();
	if (Session.bIsOpen && NetworkTimeoutRemaining >= 0.f)
	{
		NetworkTimeoutRemaining -= InDeltaTime;
		if (NetworkTimeoutRemaining <= 0.f && !Session.bPageLoadedFired)
		{
			NetworkTimeoutRemaining = -1.f;
			FStashEditorPreviewService::Get()->NotifyLoadError();
		}
	}
#if STASH_HAS_WEBBROWSER
	if (Session.bIsOpen && WebBrowser.IsValid())
	{
		if (SBox* ActiveWebBox = GetActiveWebContentBox())
		{
			const FVector2D TargetSize = GetWebViewportSize();
			if (!TargetSize.Equals(LastSyncedViewportSize, 1.f))
			{
				LastSyncedViewportSize = TargetSize;
				ActiveWebBox->Invalidate(EInvalidateWidget::LayoutAndVolatility);
				WebBrowser->Invalidate(EInvalidateWidget::LayoutAndVolatility);
			}
		}
	}
#endif
}
FVector2D SStashPreviewPanel::GetSelectedDeviceSize() const
{
	const UStashEditorSettings* Settings = GetDefault<UStashEditorSettings>();
	const float CustomW = Settings ? Settings->CustomDeviceWidth : 390.f;
	const float CustomH = Settings ? Settings->CustomDeviceHeight : 844.f;
	FStashPreviewDeviceSize Size = StashPreviewGetDeviceSize(DevicePreset, CustomW, CustomH);
	if (GetEffectiveLandscape())
	{
		Swap(Size.Width, Size.Height);
	}
	return FVector2D(Size.Width, Size.Height);
}
FReply SStashPreviewPanel::OnReloadClicked()
{
	const FStashPreviewSession& Session = FStashEditorPreviewService::Get()->GetSession();
	if (Session.bIsOpen && WebBrowser.IsValid())
	{
		FStashEditorPreviewService::Get()->GetMutableSession().LoadStartSeconds = FApp::GetCurrentTime();
		FStashEditorPreviewService::Get()->GetMutableSession().bPageLoadedFired = false;
		NetworkTimeoutRemaining = 5.f;
		WebBrowser->LoadURL(Session.CurrentUrl);
		InjectStashSdkScript();
	}
	return FReply::Handled();
}
FReply SStashPreviewPanel::OnDismissClicked()
{
	FStashEditorPreviewService::Get()->SimulateDismiss();
	return FReply::Handled();
}
FReply SStashPreviewPanel::OnSimulateSuccessClicked()
{
	FStashEditorPreviewService::Get()->SimulatePaymentSuccess();
	return FReply::Handled();
}
FReply SStashPreviewPanel::OnSimulateFailureClicked()
{
	FStashEditorPreviewService::Get()->SimulatePaymentFailure();
	return FReply::Handled();
}
FReply SStashPreviewPanel::OnSimulateDismissClicked()
{
	FStashEditorPreviewService::Get()->SimulateDismiss();
	return FReply::Handled();
}
FReply SStashPreviewPanel::OnSimulateOptInClicked()
{
	FStashEditorPreviewService::Get()->SimulateOptInResponse(TEXT("stash_pay"));
	return FReply::Handled();
}
void SStashPreviewPanel::OnDevicePresetChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	if (NewSelection.IsValid())
	{
		SelectedDevicePreset = NewSelection;
		DevicePreset = PresetFromLabel(*NewSelection);
		bCardExpandedToMax = false;
		CardDragHeightOverride = 0.f;
		RebuildPreviewChrome();
	}
}
void SStashPreviewPanel::HandlePreviewSchemeNavigation(const FString& Url)
{
	if (!Url.StartsWith(StashPreviewJsBridge::SchemePrefix))
	{
		return;
	}

	if (!FStashEditorPreviewService::Get()->DispatchPreviewCallbackUrl(Url))
	{
		return;
	}

#if STASH_HAS_WEBBROWSER
	if (WebBrowser.IsValid())
	{
		WebBrowser->StopLoad();
		const FStashPreviewSession& Session = FStashEditorPreviewService::Get()->GetSession();
		if (Session.bIsOpen && !LastLoadedUrl.IsEmpty())
		{
			WebBrowser->LoadURL(LastLoadedUrl);
			InjectStashSdkScript();
		}
		else if (WebBrowser->CanGoBack())
		{
			WebBrowser->GoBack();
		}
	}
#endif
}

bool SStashPreviewPanel::HandlePreviewLoadUrl(const FString& Method, const FString& Url, FString& OutResponse)
{
	if (!Url.StartsWith(StashPreviewJsBridge::SchemePrefix))
	{
		return false;
	}

	HandlePreviewSchemeNavigation(Url);
	OutResponse = TEXT("<!DOCTYPE html><html><body></body></html>");
	return true;
}

void SStashPreviewPanel::HandlePreviewConsoleMessage(
	const FString& Message,
	const FString& Source,
	int32 Line,
	EWebBrowserConsoleLogSeverity Severity)
{
	static const FString ConsolePrefix = TEXT("__STASH_PREVIEW__:");
	if (!Message.StartsWith(ConsolePrefix))
	{
		return;
	}

	const FString Payload = Message.Mid(ConsolePrefix.Len());
	FString Path;
	FString Query;
	Payload.Split(TEXT("?"), &Path, &Query);
	if (Path.IsEmpty())
	{
		return;
	}

	HandlePreviewSchemeNavigation(StashPreviewCallbackUrl::BuildPreviewCallbackUrl(Path, Query));
}

void SStashPreviewPanel::HandleUrlChanged(const FText& InText)
{
	HandlePreviewSchemeNavigation(InText.ToString());
}

bool SStashPreviewPanel::HandleBeforePreviewNavigation(const FString& Url, const FWebNavigationRequest& Request)
{
	if (!Url.StartsWith(StashPreviewJsBridge::SchemePrefix))
	{
		return false;
	}

	HandlePreviewSchemeNavigation(Url);
	return true;
}
void SStashPreviewPanel::HandleLoadCompleted()
{
	NetworkTimeoutRemaining = -1.f;
	InjectStashSdkScript();
	FStashEditorPreviewService::Get()->NotifyLoadCompleted();
}
void SStashPreviewPanel::InjectStashSdkScript()
{
#if STASH_HAS_WEBBROWSER
	if (WebBrowser.IsValid())
	{
		WebBrowser->ExecuteJavascript(StashPreviewJsBridge::GetInjectionScript());
	}
#endif
}
void SStashPreviewPanel::EnsureBrowserForUrl(const FString& Url)
{
#if STASH_HAS_WEBBROWSER
	EnsureStashPreviewSchemeHandlerRegistered();
	if (!FModuleManager::Get().IsModuleLoaded(TEXT("WebBrowser")))
	{
		FModuleManager::Get().LoadModule(TEXT("WebBrowser"));
	}
	const bool bUrlChanged = (LastLoadedUrl != Url);
	LastLoadedUrl = Url;
	if (!WebBrowser.IsValid())
	{
		SAssignNew(WebBrowser, SWebBrowser)
			.InitialURL(Url)
			.ShowControls(false)
			.ShowAddressBar(false)
			.SupportsTransparency(false)
			.UseSmoothResizing(true)
			.ViewportSize_Lambda([this]() { return GetWebViewportSize(); })
			.OnBeforeNavigation(this, &SStashPreviewPanel::HandleBeforePreviewNavigation)
			.OnUrlChanged(this, &SStashPreviewPanel::HandleUrlChanged)
			.OnLoadCompleted(this, &SStashPreviewPanel::HandleLoadCompleted)
			.OnLoadUrl(this, &SStashPreviewPanel::HandlePreviewLoadUrl)
			.OnConsoleMessage(this, &SStashPreviewPanel::HandlePreviewConsoleMessage);
	}
	else if (bUrlChanged)
	{
		WebBrowser->LoadURL(Url);
	}
	if (WebBrowser.IsValid())
	{
		if (SBox* ActiveWebBox = GetActiveWebContentBox())
		{
			ActiveWebBox->SetContent(WebBrowser.ToSharedRef());
		}
	}
#else
	LastLoadedUrl = Url;
#endif
}
FLinearColor SStashPreviewPanel::ParseBackgroundColor(const FString& HtmlHex) const
{
	FString Hex = HtmlHex.TrimStartAndEnd();
	if (Hex.IsEmpty())
	{
		return FLinearColor(0.12f, 0.12f, 0.14f, 1.f);
	}
	if (Hex.StartsWith(TEXT("#")))
	{
		Hex = Hex.Mid(1);
	}
	uint32 Value = 0;
	if (Hex.Len() == 3)
	{
		const uint32 R = FParse::HexDigit(Hex[0]);
		const uint32 G = FParse::HexDigit(Hex[1]);
		const uint32 B = FParse::HexDigit(Hex[2]);
		return FLinearColor(R / 15.f, G / 15.f, B / 15.f, 1.f);
	}
	if (Hex.Len() == 6 || Hex.Len() == 8)
	{
		Value = FParse::HexNumber(*Hex);
		const float A = Hex.Len() == 8 ? ((Value >> 24) & 0xFF) / 255.f : 1.f;
		const float R = ((Value >> 16) & 0xFF) / 255.f;
		const float G = ((Value >> 8) & 0xFF) / 255.f;
		const float B = (Value & 0xFF) / 255.f;
		return FLinearColor(R, G, B, A);
	}
	return FLinearColor(0.12f, 0.12f, 0.14f, 1.f);
}
void SStashPreviewPanel::UpdateBackdropBrush(const FStashPreviewSession& Session)
{
	BackdropBrush.Reset();
	if (Session.BackdropBytes.Num() > 0)
	{
		IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
		const EImageFormat Format = ImageWrapperModule.DetectImageFormat(Session.BackdropBytes.GetData(), Session.BackdropBytes.Num());
		if (Format != EImageFormat::Invalid)
		{
			const TSharedPtr<IImageWrapper> Wrapper = ImageWrapperModule.CreateImageWrapper(Format);
			if (Wrapper.IsValid() && Wrapper->SetCompressed(Session.BackdropBytes.GetData(), Session.BackdropBytes.Num()))
			{
				TArray<uint8> RawData;
				if (Wrapper->GetRaw(ERGBFormat::BGRA, 8, RawData))
				{
					BackdropBrush = FSlateDynamicImageBrush::CreateWithImageData(
						TEXT("StashPreviewBackdrop"),
						FVector2D(static_cast<float>(Wrapper->GetWidth()), static_cast<float>(Wrapper->GetHeight())),
						RawData);
				}
			}
		}
	}
	if (BackdropImage.IsValid())
	{
		BackdropImage->SetImage(BackdropBrush.Get());
		BackdropImage->Invalidate(EInvalidateWidget::LayoutAndVolatility);
	}
}
void SStashPreviewPanel::UpdateSheetBrushes()
{
	if (CardSheetBorder.IsValid())
	{
		CardSheetBorder->SetBorderImage(SheetBrush.Get());
		CardSheetBorder->Invalidate(EInvalidateWidget::LayoutAndVolatility);
	}
	if (ModalSheetBorder.IsValid())
	{
		ModalSheetBorder->SetBorderImage(SheetBrush.Get());
		ModalSheetBorder->Invalidate(EInvalidateWidget::LayoutAndVolatility);
	}
}

TSharedRef<SWidget> SStashPreviewPanel::BuildSheetInnerChrome(
	TSharedPtr<SBorder>& OutBorder,
	TSharedPtr<SBox>& OutWebBox,
	const bool bIncludeDragHandle)
{
	if (!bIncludeDragHandle)
	{
		return SAssignNew(OutBorder, SBorder)
			.BorderImage_Lambda([this]() { return SheetBrush.Get(); })
			.Padding(0.f)
			.Clipping(EWidgetClipping::ClipToBounds)
			[
				SAssignNew(OutWebBox, SBox)
				.Clipping(EWidgetClipping::ClipToBounds)
				[
					SNew(SBox)
				]
			];
	}

	return SAssignNew(OutBorder, SBorder)
		.BorderImage_Lambda([this]() { return SheetBrush.Get(); })
		.Padding(0.f)
		.Clipping(EWidgetClipping::ClipToBounds)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 8.f, 0.f, 6.f)
			.HAlign(HAlign_Center)
			[
				SNew(SBox)
				.WidthOverride(36.f)
				.HeightOverride(4.f)
				.Visibility_Lambda([this]()
				{
					return bShowDragHandle ? EVisibility::Visible : EVisibility::Collapsed;
				})
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
					.BorderBackgroundColor(FLinearColor(0.75f, 0.75f, 0.78f, 0.9f))
				]
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			[
				SAssignNew(OutWebBox, SBox)
				.Clipping(EWidgetClipping::ClipToBounds)
				[
					SNew(SBox)
				]
			]
		];
}

void SStashPreviewPanel::RebuildPreviewChrome()
{
	if (!PreviewOverlay.IsValid())
	{
		return;
	}
	const FStashPreviewSession& Session = FStashEditorPreviewService::Get()->GetSession();
	if (!Session.bIsOpen)
	{
		ActiveLayout = FStashPreviewSheetLayout();
		bShowDimOverlay = false;
		bDimDismissible = false;
		bShowDragHandle = false;
		bCardExpandedToMax = false;
		CardDragHeightOverride = 0.f;
		LastSyncedViewportSize = FVector2D::ZeroVector;
		LastPresentationMode = EStashPreviewPresentationMode::Card;
		PreviewOverlay->Invalidate(EInvalidateWidget::LayoutAndVolatility);
		return;
	}
	UpdateBackdropBrush(Session);
	const FString ShellColorStr = Session.PresentationMode == EStashPreviewPresentationMode::Modal
		? Session.ModalConfig.BackgroundColor
		: Session.CardConfig.BackgroundColor;
	ActiveShellColor = ParseBackgroundColor(ShellColorStr);
	ActiveLayout = ComputeCurrentLayout();
	bShowDragHandle = Session.PresentationMode == EStashPreviewPresentationMode::Card
		&& ActiveLayout.bShowDragHandle;

	if (Session.PresentationMode == EStashPreviewPresentationMode::Card
		&& LastPresentationMode != EStashPreviewPresentationMode::Card)
	{
		bCardExpandedToMax = false;
		CardDragHeightOverride = 0.f;
	}

	const float CornerRadius = 12.f;
	const FVector4 CornerRadii = ActiveLayout.bCardBottomDrawer
		? FVector4(CornerRadius, CornerRadius, 0.f, 0.f)
		: FVector4(CornerRadius);
	SheetBrush = MakeShared<FSlateRoundedBoxBrush>(ActiveShellColor, CornerRadii);
	bShowDimOverlay = Session.PresentationMode != EStashPreviewPresentationMode::Browser;
	bDimDismissible = Session.PresentationMode == EStashPreviewPresentationMode::Modal
		&& Session.bAllowDismiss
		&& !Session.bIsPurchaseProcessing;

	UpdateSheetBrushes();
	LastPresentationMode = Session.PresentationMode;
	LastSyncedViewportSize = FVector2D::ZeroVector;
	PreviewOverlay->Invalidate(EInvalidateWidget::LayoutAndVolatility);
}
TSharedRef<SWidget> SStashPreviewPanel::BuildControlsPanel()
{
	return SNew(SScrollBox)
		+ SScrollBox::Slot()
		.Padding(8.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 8.f)
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("StashEditor", "PreviewHelp", "Test Stash Pay in the editor. Callbacks fire as on device. Use simulate buttons when checkout JS is unavailable."))
				.AutoWrapText(true)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text_Lambda([]()
				{
					const FStashPreviewSession& S = FStashEditorPreviewService::Get()->GetSession();
					return FText::Format(NSLOCTEXT("StashEditor", "PreviewModeFmt", "Mode: {0}"), FText::FromString(ModeLabel(S.PresentationMode)));
				})
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 4.f)
			[
				SNew(STextBlock)
				.Text_Lambda([]()
				{
					const FStashPreviewSession& S = FStashEditorPreviewService::Get()->GetSession();
					return FText::Format(NSLOCTEXT("StashEditor", "PreviewUrlFmt", "URL: {0}"), FText::FromString(S.CurrentUrl.IsEmpty() ? TEXT("(none)") : S.CurrentUrl));
				})
				.AutoWrapText(true)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 8.f)
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("StashEditor", "DevicePresetLabel", "Preview device:"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SComboBox<TSharedPtr<FString>>)
				.OptionsSource(&DevicePresetOptions)
				.InitiallySelectedItem(SelectedDevicePreset)
				.OnSelectionChanged(this, &SStashPreviewPanel::OnDevicePresetChanged)
				.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
				{
					return SNew(STextBlock).Text(FText::FromString(*Item));
				})
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						return FText::FromString(SelectedDevicePreset.IsValid() ? *SelectedDevicePreset : TEXT("iPhone 14"));
					})
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 8.f, 0.f, 4.f)
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("StashEditor", "ActiveConfigLabel", "Active config:"))
				.Visibility_Lambda([]()
				{
					return FStashEditorPreviewService::Get()->GetSession().bIsOpen
						? EVisibility::Visible : EVisibility::Collapsed;
				})
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 8.f)
			[
				SNew(STextBlock)
				.Text_Lambda([this]()
				{
					const FStashPreviewSession& S = FStashEditorPreviewService::Get()->GetSession();
					return FText::FromString(StashPreviewDescribeActiveConfig(
						S.PresentationMode,
						ActiveLayout,
						S.CardConfig,
						S.ModalConfig,
						GetEffectiveLandscape(),
						S.bForcePortraitLayout));
				})
				.AutoWrapText(true)
				.Visibility_Lambda([]()
				{
					return FStashEditorPreviewService::Get()->GetSession().bIsOpen
						? EVisibility::Visible : EVisibility::Collapsed;
				})
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 4.f)
			[
				SNew(SButton)
				.Text_Lambda([]()
				{
					const FStashPreviewSession& S = FStashEditorPreviewService::Get()->GetSession();
					if (S.bIsOpen && S.bForcePortraitLayout)
					{
						return NSLOCTEXT("StashEditor", "ToggleLandscapeForced", "Landscape (forced portrait by card config)");
					}
					return NSLOCTEXT("StashEditor", "ToggleLandscape", "Toggle landscape");
				})
				.IsEnabled_Lambda([]()
				{
					const FStashPreviewSession& S = FStashEditorPreviewService::Get()->GetSession();
					return !(S.bIsOpen && S.bForcePortraitLayout);
				})
				.OnClicked_Lambda([this]()
				{
					const FStashPreviewSession& S = FStashEditorPreviewService::Get()->GetSession();
					if (S.bIsOpen && S.bForcePortraitLayout)
					{
						return FReply::Handled();
					}
					bDeviceLandscape = !bDeviceLandscape;
					RebuildPreviewChrome();
					return FReply::Handled();
				})
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 8.f)
			[
				SNew(SButton)
				.Text(NSLOCTEXT("StashEditor", "ReloadPreview", "Reload"))
				.OnClicked(this, &SStashPreviewPanel::OnReloadClicked)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SButton)
				.Text(NSLOCTEXT("StashEditor", "DismissPreview", "Dismiss"))
				.OnClicked(this, &SStashPreviewPanel::OnDismissClicked)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 12.f, 0.f, 4.f)
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("StashEditor", "SimulateCallbacks", "Simulate callbacks:"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SButton)
				.Text(NSLOCTEXT("StashEditor", "SimSuccess", "Payment Success"))
				.OnClicked(this, &SStashPreviewPanel::OnSimulateSuccessClicked)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SButton)
				.Text(NSLOCTEXT("StashEditor", "SimFailure", "Payment Failure"))
				.OnClicked(this, &SStashPreviewPanel::OnSimulateFailureClicked)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SButton)
				.Text(NSLOCTEXT("StashEditor", "SimDismiss", "Dismiss Dialog"))
				.OnClicked(this, &SStashPreviewPanel::OnSimulateDismissClicked)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SButton)
				.Text(NSLOCTEXT("StashEditor", "SimOptIn", "Opt-in (stash_pay)"))
				.OnClicked(this, &SStashPreviewPanel::OnSimulateOptInClicked)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 12.f, 0.f, 0.f)
			[
				SNew(STextBlock)
				.Text_Lambda([]()
				{
					const FStashPreviewSession& S = FStashEditorPreviewService::Get()->GetSession();
					return FText::Format(
						NSLOCTEXT("StashEditor", "PreviewDebugFmt", "Open: {0}  Processing: {1}\nLandscape lock: {2}  Keep-alive: {3}"),
						S.bIsOpen ? FText::FromString(TEXT("yes")) : FText::FromString(TEXT("no")),
						S.bIsPurchaseProcessing ? FText::FromString(TEXT("yes")) : FText::FromString(TEXT("no")),
						S.bLandscapeLockWhenCardClosed ? FText::FromString(TEXT("on")) : FText::FromString(TEXT("off")),
						S.bKeepAliveEnabled ? FText::FromString(TEXT("on")) : FText::FromString(TEXT("off")));
				})
				.AutoWrapText(true)
			]
		];
}
TSharedRef<SWidget> SStashPreviewPanel::BuildDevicePreviewArea()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
		.Padding(12.f)
		[
			SAssignNew(PreviewHost, SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SAssignNew(PreviewOverlay, SOverlay)
				+ SOverlay::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(NSLOCTEXT("StashEditor", "PreviewIdle", "Call Open Card, Open Modal, or Open Browser during PIE to preview checkout here."))
					.AutoWrapText(true)
					.Visibility_Lambda([]()
					{
						return FStashEditorPreviewService::Get()->GetSession().bIsOpen
							? EVisibility::Collapsed : EVisibility::Visible;
					})
				]
				+ SOverlay::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SAssignNew(DeviceFrameBox, SBox)
					.WidthOverride_Lambda([this]() { return GetSelectedDeviceSize().X; })
					.HeightOverride_Lambda([this]() { return GetSelectedDeviceSize().Y; })
					.Visibility_Lambda([]()
					{
						return FStashEditorPreviewService::Get()->GetSession().bIsOpen
							? EVisibility::Visible : EVisibility::Collapsed;
					})
					[
						SNew(SBorder)
						.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
						.Padding(0.f)
						.Clipping(EWidgetClipping::ClipToBounds)
						[
							SNew(SOverlay)
							+ SOverlay::Slot()
							[
								SAssignNew(BackdropImage, SImage)
								.Visibility_Lambda([this]()
								{
									return BackdropBrush.IsValid() ? EVisibility::Visible : EVisibility::Collapsed;
								})
							]
							+ SOverlay::Slot()
							[
								SNew(SBorder)
								.BorderBackgroundColor(FLinearColor(0.05f, 0.05f, 0.06f, 1.f))
								.Visibility_Lambda([this]()
								{
									return BackdropBrush.IsValid() ? EVisibility::Collapsed : EVisibility::Visible;
								})
							]
							+ SOverlay::Slot()
							[
								SNew(SBorder)
								.BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.45f))
								.Visibility_Lambda([this]()
								{
									return bShowDimOverlay && !bDimDismissible ? EVisibility::Visible : EVisibility::Collapsed;
								})
							]
							+ SOverlay::Slot()
							[
								SNew(SButton)
								.ButtonStyle(FAppStyle::Get(), "NoBorder")
								.ContentPadding(FMargin(0.f))
								.Visibility_Lambda([this]()
								{
									return bShowDimOverlay && bDimDismissible ? EVisibility::Visible : EVisibility::Collapsed;
								})
								.OnClicked(this, &SStashPreviewPanel::OnDimOverlayClicked)
								[
									SNew(SBorder)
									.BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.45f))
								]
							]
							+ SOverlay::Slot()
							.HAlign(HAlign_Fill)
							.VAlign(VAlign_Fill)
							[
								SNew(SBox)
								.Padding_Lambda([this]()
								{
									return ComputeSheetPadding(ComputeCurrentLayout());
								})
								[
									SAssignNew(SheetAlignBox, SBox)
									.WidthOverride_Lambda([this]() -> FOptionalSize
									{
										const FStashPreviewSheetLayout Layout = ComputeCurrentLayout();
										return Layout.HAlign == HAlign_Fill
											? FOptionalSize()
											: FOptionalSize(Layout.Width);
									})
									.HeightOverride_Lambda([this]() -> FOptionalSize
									{
										const FStashPreviewSheetLayout Layout = ComputeCurrentLayout();
										return Layout.VAlign == VAlign_Fill
											? FOptionalSize()
											: FOptionalSize(Layout.Height);
									})
									.Clipping(EWidgetClipping::ClipToBounds)
									[
										SNew(SOverlay)
										+ SOverlay::Slot()
										[
											SAssignNew(CardSheetHost, SBox)
											.Visibility_Lambda([this]()
											{
												return IsCardPresentation()
													? EVisibility::Visible : EVisibility::Collapsed;
											})
											[
												SAssignNew(DraggableSheet, SStashPreviewDraggableSheet)
												.SheetHeight_Lambda([this]() { return ComputeCurrentLayout().Height; })
												.BaseSheetHeight_Lambda([this]() { return ComputeCardBaseLayout().Height; })
												.MaxExpandHeight_Lambda([this]() { return GetCardMaxExpandedHeight(); })
												.bEnableDragDismiss_Lambda([this]() { return IsCardDragDismissEnabled(); })
												.bEnableExpandDrag_Lambda([this]() { return IsCardExpandDragEnabled(); })
												.OnDismissRequested(FSimpleDelegate::CreateSP(this, &SStashPreviewPanel::HandleDragDismiss))
												.OnSheetHeightDragChanged(FOnStashPreviewCardSheetHeightDrag::CreateSP(
													this, &SStashPreviewPanel::HandleCardSheetHeightDrag))
												.OnSheetDragEnded(FOnStashPreviewCardSheetDragEnded::CreateSP(
													this, &SStashPreviewPanel::HandleCardSheetDragEnded))
												[
													BuildSheetInnerChrome(CardSheetBorder, CardWebContentBox, true)
												]
											]
										]
										+ SOverlay::Slot()
										[
											SAssignNew(ModalSheetHost, SBox)
											.Visibility_Lambda([this]()
											{
												const FStashPreviewSession& S = FStashEditorPreviewService::Get()->GetSession();
												return S.bIsOpen
													&& S.PresentationMode != EStashPreviewPresentationMode::Card
													? EVisibility::Visible : EVisibility::Collapsed;
											})
											[
												BuildSheetInnerChrome(ModalSheetBorder, ModalWebContentBox, false)
											]
										]
									]
								]
							]
						]
					]
				]
			]
		];
}
