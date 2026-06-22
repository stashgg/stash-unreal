// Copyright Stash. All Rights Reserved.

#include "SStashPreviewPanel.h"
#include "StashEditorPreviewService.h"
#include "StashPreviewJsBridge.h"
#include "StashEditorSettings.h"
#include "StashEditorLog.h"
#include "Misc/App.h"
#include "Misc/Parse.h"
#include "Engine/Texture2D.h"
#include "Brushes/SlateDynamicImageBrush.h"
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
	if (Session.bIsOpen && !Session.CurrentUrl.IsEmpty())
	{
		EnsureBrowserForUrl(Session.CurrentUrl);
		NetworkTimeoutRemaining = 5.f;
	}
	else
	{
		NetworkTimeoutRemaining = -1.f;
		LastLoadedUrl.Reset();
		if (WebBrowser.IsValid())
		{
			WebBrowser.Reset();
		}
	}
	RebuildPreviewChrome();
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
}

FVector2D SStashPreviewPanel::GetSelectedDeviceSize() const
{
	const UStashEditorSettings* Settings = GetDefault<UStashEditorSettings>();
	const float CustomW = Settings ? Settings->CustomDeviceWidth : 390.f;
	const float CustomH = Settings ? Settings->CustomDeviceHeight : 844.f;
	FStashPreviewDeviceSize Size = StashPreviewGetDeviceSize(DevicePreset, CustomW, CustomH);
	if (bDeviceLandscape)
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
		RebuildPreviewChrome();
	}
}

void SStashPreviewPanel::HandleUrlChanged(const FText& InText)
{
	const FString Url = InText.ToString();
	if (Url.StartsWith(StashPreviewJsBridge::SchemePrefix))
	{
		FStashEditorPreviewService::Get()->NotifyUrlChanged(Url);
		const FStashPreviewSession& Session = FStashEditorPreviewService::Get()->GetSession();
		if (Session.bIsOpen && WebBrowser.IsValid() && !LastLoadedUrl.IsEmpty())
		{
			WebBrowser->LoadURL(LastLoadedUrl);
		}
	}
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
			.OnUrlChanged(this, &SStashPreviewPanel::HandleUrlChanged)
			.OnLoadCompleted(this, &SStashPreviewPanel::HandleLoadCompleted);
	}
	else if (bUrlChanged)
	{
		WebBrowser->LoadURL(Url);
	}

	if (SheetBox.IsValid() && WebBrowser.IsValid())
	{
		SheetBox->SetContent(WebBrowser.ToSharedRef());
	}
#else
	LastLoadedUrl = Url;
	if (SheetBox.IsValid())
	{
		SheetBox->SetContent(
			SNew(STextBlock)
			.Text(FText::Format(
				NSLOCTEXT("StashEditor", "PreviewNoWebBrowser", "Web Browser plugin is not available in this engine install.\n\nURL: {0}\n\nUse Simulate callbacks in the left panel, or install/enable the Web Browser plugin and rebuild."),
				FText::FromString(Url)))
			.AutoWrapText(true));
	}
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

void SStashPreviewPanel::RebuildPreviewChrome()
{
	if (!PreviewOverlay.IsValid())
	{
		return;
	}

	const FStashPreviewSession& Session = FStashEditorPreviewService::Get()->GetSession();
	PreviewOverlay->ClearChildren();

	const FVector2D DeviceSize = GetSelectedDeviceSize();
	const float DeviceW = DeviceSize.X;
	const float DeviceH = DeviceSize.Y;

	if (!Session.bIsOpen)
	{
		PreviewOverlay->AddSlot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(NSLOCTEXT("StashEditor", "PreviewIdle", "Call Open Card, Open Modal, or Open Browser during PIE to preview checkout here."))
			.AutoWrapText(true)
		];
		return;
	}

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

	const FString ShellColorStr = Session.PresentationMode == EStashPreviewPresentationMode::Modal
		? Session.ModalConfig.BackgroundColor
		: Session.CardConfig.BackgroundColor;
	const FLinearColor ShellColor = ParseBackgroundColor(ShellColorStr);

	float SheetW = DeviceW;
	float SheetH = DeviceH;
	EHorizontalAlignment HAlign = HAlign_Fill;
	EVerticalAlignment VAlign = VAlign_Fill;

	if (Session.PresentationMode == EStashPreviewPresentationMode::Card)
	{
		if (bDeviceLandscape)
		{
			SheetW = DeviceW * Session.CardConfig.CardWidthRatioLandscape;
			SheetH = DeviceH * Session.CardConfig.CardHeightRatioLandscape;
		}
		else
		{
			SheetW = DeviceW;
			SheetH = DeviceH * Session.CardConfig.CardHeightRatioPortrait;
		}
		HAlign = HAlign_Center;
		VAlign = VAlign_Bottom;
	}
	else if (Session.PresentationMode == EStashPreviewPresentationMode::Modal)
	{
		if (bDeviceLandscape)
		{
			SheetW = DeviceW * Session.ModalConfig.PhoneWidthRatioLandscape;
			SheetH = DeviceH * Session.ModalConfig.PhoneHeightRatioLandscape;
		}
		else
		{
			SheetW = DeviceW * Session.ModalConfig.PhoneWidthRatioPortrait;
			SheetH = DeviceH * Session.ModalConfig.PhoneHeightRatioPortrait;
		}
		HAlign = HAlign_Center;
		VAlign = VAlign_Center;
	}

	PreviewOverlay->AddSlot()
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Center)
	[
		SNew(SBox)
		.WidthOverride(DeviceW)
		.HeightOverride(DeviceH)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
				.Padding(0.f)
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					[
						BackdropBrush.IsValid()
							? StaticCastSharedRef<SWidget>(SNew(SImage).Image(BackdropBrush.Get()))
							: StaticCastSharedRef<SWidget>(SNew(SBorder).BorderBackgroundColor(FLinearColor(0.05f, 0.05f, 0.06f, 1.f)))
					]
					+ SOverlay::Slot()
					[
						SNew(SBorder)
						.BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, Session.PresentationMode == EStashPreviewPresentationMode::Browser ? 0.f : 0.45f))
						.Visibility(Session.PresentationMode == EStashPreviewPresentationMode::Browser ? EVisibility::Collapsed : EVisibility::Visible)
					]
					+ SOverlay::Slot()
					.HAlign(HAlign)
					.VAlign(VAlign)
					[
						SAssignNew(SheetBox, SBox)
						.WidthOverride(SheetW)
						.HeightOverride(SheetH)
						[
							SNew(SBorder)
							.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
							.BorderBackgroundColor(ShellColor)
							.Padding(0.f)
							[
#if STASH_HAS_WEBBROWSER
								WebBrowser.IsValid()
									? StaticCastSharedRef<SWidget>(WebBrowser.ToSharedRef())
									: StaticCastSharedRef<SWidget>(SNew(STextBlock).Text(FText::FromString(Session.CurrentUrl)))
#else
								SNew(STextBlock)
								.Text(FText::Format(
									NSLOCTEXT("StashEditor", "PreviewNoWebBrowserShort", "Web Browser plugin not installed.\nURL: {0}"),
									FText::FromString(Session.CurrentUrl)))
								.AutoWrapText(true)
#endif
							]
						]
					]
				]
			]
		]
	];
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
			.Padding(0.f, 4.f)
			[
				SNew(SButton)
				.Text(NSLOCTEXT("StashEditor", "ToggleLandscape", "Toggle landscape"))
				.OnClicked_Lambda([this]()
				{
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
			]
		];
}
