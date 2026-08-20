// Copyright Stash. All Rights Reserved.
#include "SStashPreviewPanel.h"
#include "SStashPreviewDraggableSheet.h"
#include "SStashPreviewKeyboard.h"
#include "SStashPreviewStatusIcons.h"
#include "SStashPreviewCornerMask.h"
#include "StashEditorPreviewService.h"
#include "StashPreviewDeviceCatalog.h"
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
#include "IWebBrowserSingleton.h"
#include "IWebBrowserWindow.h"
#include "IWebBrowserResourceLoader.h"
#endif
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
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
	FString PlatformLabel(EStashPreviewPlatform Platform)
	{
		return Platform == EStashPreviewPlatform::Android ? TEXT("Android") : TEXT("iOS");
	}
	FString PresetOptionLabel(const FStashPreviewDeviceSpec& Spec)
	{
		return FString::Printf(TEXT("%s — %s"), *PlatformLabel(Spec.Platform), *Spec.DisplayName);
	}

	// --- Device-chrome (notch) metrics ---------------------------------------------------------------
	// Per-notch-type geometry, kept in one table so the notch switch isn't repeated across the status bar,
	// the portrait/landscape cutout overlays, and the cutout brushes.

	/** Static per-notch metrics. Sentinel -1 = "derive from device size" (resolved in ResolveNotchDims). */
	struct FStashPreviewNotchMetrics
	{
		/** Cutout width, also the portrait status-bar center gap. -1 = derive from device width. */
		float CutoutWidth = 0.f;
		float PortraitHeight = 0.f;
		FMargin PortraitPadding = FMargin(0.f);
		float LandscapeWidth = 0.f;
		/** -1 = derive from device height. */
		float LandscapeHeight = 0.f;
		FMargin LandscapePadding = FMargin(0.f);
		/** Cutout brush corner radii (TL, TR, BR, BL). */
		FVector4 PortraitBrushCorners = FVector4(0.f);
		FVector4 LandscapeBrushCorners = FVector4(0.f);
	};

	FStashPreviewNotchMetrics GetNotchMetrics(EStashPreviewNotchType Type)
	{
		switch (Type)
		{
		case EStashPreviewNotchType::Notch:
			// Portrait: flush to the top bezel (square top, rounded bottom). Landscape: notch on the leading
			// (left) edge — square left, rounded right.
			return { -1.f, 32.f, FMargin(0.f),
			         32.f, -1.f, FMargin(0.f),
			         FVector4(0.f, 0.f, 14.f, 14.f), FVector4(0.f, 14.f, 14.f, 0.f) };
		case EStashPreviewNotchType::DynamicIsland:
			return { 125.f, 36.f, FMargin(0.f, 11.f, 0.f, 0.f),
			         36.f, 125.f, FMargin(11.f, 0.f, 0.f, 0.f),
			         FVector4(18.f, 18.f, 18.f, 18.f), FVector4(18.f, 18.f, 18.f, 18.f) };
		case EStashPreviewNotchType::PunchHole:
			return { 20.f, 20.f, FMargin(0.f, 8.f, 0.f, 0.f),
			         20.f, 20.f, FMargin(8.f, 0.f, 0.f, 0.f),
			         FVector4(10.f, 10.f, 10.f, 10.f), FVector4(10.f, 10.f, 10.f, 10.f) };
		default:
			return FStashPreviewNotchMetrics();
		}
	}

	/** Cutout box dimensions resolved for the given orientation and device size. */
	struct FStashPreviewNotchDims
	{
		float Width = 0.f;
		float Height = 0.f;
		FMargin Padding = FMargin(0.f);
	};

	FStashPreviewNotchDims ResolveNotchDims(EStashPreviewNotchType Type, const FVector2D& DeviceSize, bool bLandscape)
	{
		const FStashPreviewNotchMetrics M = GetNotchMetrics(Type);
		FStashPreviewNotchDims Out;
		if (!bLandscape)
		{
			Out.Width = M.CutoutWidth < 0.f ? FMath::Clamp(DeviceSize.X * 0.45f, 120.f, 180.f) : M.CutoutWidth;
			Out.Height = M.PortraitHeight;
			Out.Padding = M.PortraitPadding;
		}
		else
		{
			Out.Width = M.LandscapeWidth;
			Out.Height = M.LandscapeHeight < 0.f ? FMath::Clamp(DeviceSize.Y * 0.42f, 110.f, 175.f) : M.LandscapeHeight;
			Out.Padding = M.LandscapePadding;
		}
		return Out;
	}

	/** Presets in combo order: catalog entries followed by Custom. */
	TArray<EStashPreviewDevicePreset> BuildPresetOrder()
	{
		TArray<EStashPreviewDevicePreset> Order;
		for (const FStashPreviewDeviceSpec& Spec : StashPreviewGetDeviceCatalog())
		{
			Order.Add(Spec.Preset);
		}
		Order.Add(EStashPreviewDevicePreset::Custom);
		return Order;
	}

#if STASH_HAS_WEBBROWSER
	/**
	 * CEF request-context callback (game thread): stamps the selected device's mobile user-agent and
	 * client-hint headers onto every request so the checkout page serves its true iOS/Android experience.
	 * Reads the singleton service, so it stays valid across preview-panel lifetimes.
	 */
	void StashPreviewInjectMobileHeaders(FString /*Url*/, FString /*ResourceType*/, FContextRequestHeaders& AdditionalHeaders, const bool /*bAllowCredentials*/)
	{
		const TSharedRef<FStashEditorPreviewService> Service = FStashEditorPreviewService::Get();
		const FString& UserAgent = Service->GetPreviewUserAgent();
		if (UserAgent.IsEmpty())
		{
			return;
		}
		AdditionalHeaders.Add(TEXT("User-Agent"), UserAgent);
		AdditionalHeaders.Add(TEXT("sec-ch-ua-mobile"), Service->IsPreviewMobile() ? TEXT("?1") : TEXT("?0"));
		AdditionalHeaders.Add(TEXT("sec-ch-ua-platform"), FString::Printf(TEXT("\"%s\""), *Service->GetPreviewPlatformHint()));
	}
#endif
}
void SStashPreviewPanel::Construct(const FArguments& InArgs)
{
	const UStashEditorSettings* Settings = GetDefault<UStashEditorSettings>();
	DevicePreset = Settings ? Settings->DefaultDevicePreset : EStashPreviewDevicePreset::iPhone14;

	DevicePresetOptions.Reset();
	int32 SelectedIndex = INDEX_NONE;
	const TArray<EStashPreviewDevicePreset> PresetOrder = BuildPresetOrder();
	for (int32 Index = 0; Index < PresetOrder.Num(); ++Index)
	{
		const EStashPreviewDevicePreset Preset = PresetOrder[Index];
		const FString Label = Preset == EStashPreviewDevicePreset::Custom
			? FString(TEXT("Custom (Project Settings)"))
			: PresetOptionLabel(StashPreviewGetDeviceSpec(Preset, EStashPreviewPlatform::iOS, 0.f, 0.f));
		DevicePresetOptions.Add(MakeShared<FString>(Label));
		if (Preset == DevicePreset)
		{
			SelectedIndex = Index;
		}
	}
	if (SelectedIndex == INDEX_NONE)
	{
		DevicePreset = EStashPreviewDevicePreset::iPhone14;
		SelectedIndex = PresetOrder.IndexOfByKey(DevicePreset);
	}
	SelectedDevicePreset = DevicePresetOptions[SelectedIndex];

	RebuildDeviceChromeBrushes();
	FStashEditorPreviewService::Get()->RegisterPreviewPanel(SharedThis(this));
	LastSyncedActivePlatform = GetSelectedDeviceSpec().Platform;
	FStashEditorPreviewService::Get()->SetActivePlatform(LastSyncedActivePlatform);
	PushDeviceEmulation(false);
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
SStashPreviewPanel::~SStashPreviewPanel()
{
	// Symmetric with RegisterPreviewPanel in Construct. The service prunes stale weak refs on its own, but
	// unregistering here keeps its panel list tidy. Passing an empty ptr removes the now-expired weak entry
	// (this instance) without needing a shared ref to a destructing object.
	FStashEditorPreviewService::Get()->UnregisterPreviewPanel(nullptr);
}
void SStashPreviewPanel::RefreshFromSession()
{
	const FStashPreviewSession& Session = FStashEditorPreviewService::Get()->GetSession();
	if (Session.bIsOpen && Session.bForcePortraitLayout)
	{
		// A real device only flashes when it was actually landscape before the forced rotation — whether
		// that came from the manual toggle (bDeviceLandscape) or a native landscape lock
		// (bLandscapeLockWhenCardClosed). The episode latch keeps it a one-shot per rotation.
		const bool bWasLandscape = bDeviceLandscape || Session.bLandscapeLockWhenCardClosed;
		if (bWasLandscape && !bForcePortraitEpisodeActive)
		{
			bForcePortraitEpisodeActive = true;
			if (bDeviceLandscape)
			{
				// Only the manual toggle needs restoring; the landscape lock restores itself when the card closes.
				bRestoreLandscapeAfterSession = true;
				bDeviceLandscape = false;
			}
			if (IsAndroidPreset() && Session.BackdropBytes.Num() == 0)
			{
				// Without a backdrop bitmap the real Android rotation flashes white.
				BackdropFlashRemaining = BackdropFlashSeconds;
				bShowBackdropFlashHint = true;
			}
		}
	}
	else
	{
		// Not in a forced-portrait card: clear the latch so a later rotation can flash again.
		bForcePortraitEpisodeActive = false;
		if (!Session.bIsOpen)
		{
			if (bRestoreLandscapeAfterSession)
			{
				bRestoreLandscapeAfterSession = false;
				bDeviceLandscape = true;
			}
			BackdropFlashRemaining = -1.f;
		}
	}
	if (Session.bIsOpen && Session.BackdropBytes.Num() > 0)
	{
		bShowBackdropFlashHint = false;
	}

	const bool bKeyboardNowVisible = Session.bIsOpen && Session.bKeyboardVisible;
	if (bKeyboardNowVisible && !bLastKeyboardVisible)
	{
		// Real sheets rise so the focused field clears the keyboard — raise the drawer card too.
		if (IsCardPresentation() && ComputeCardBaseLayout().bCardBottomDrawer
			&& IsCardExpandDragEnabled() && !bCardExpandedToMax && CardDragHeightOverride <= 0.f)
		{
			bAutoExpandedForKeyboard = true;
			ApplyCardSheetExpandedState(true);
		}
#if STASH_HAS_WEBBROWSER
		if (WebBrowser.IsValid())
		{
			// Mirror WKWebView/Chrome: keep the focused field visible above the keyboard.
			WebBrowser->ExecuteJavascript(TEXT("setTimeout(function(){var el=document.activeElement;if(el&&el.scrollIntoView){try{el.scrollIntoView({block:'nearest'});}catch(e){}}},50);"));
		}
#endif
	}
	else if (!bKeyboardNowVisible && bLastKeyboardVisible && bAutoExpandedForKeyboard)
	{
		// Collapse back only if the keyboard was what expanded the card.
		bAutoExpandedForKeyboard = false;
		if (IsCardPresentation())
		{
			ApplyCardSheetExpandedState(false);
		}
	}
	if (bKeyboardNowVisible != bLastKeyboardVisible)
	{
		LastSyncedViewportSize = FVector2D::ZeroVector;
	}
	bLastKeyboardVisible = bKeyboardNowVisible;

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
		// EnsureBrowserForUrl arms the load-failure timer only when it actually kicks off a load, so a
		// mid-load session refresh (keyboard toggle, simulate button) no longer postpones NotifyLoadError.
		EnsureBrowserForUrl(Session.CurrentUrl);
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
const FStashPreviewDeviceSpec& SStashPreviewPanel::GetSelectedDeviceSpec() const
{
	return CachedDeviceSpec;
}
void SStashPreviewPanel::RefreshCachedDeviceSpec()
{
	const UStashEditorSettings* Settings = GetDefault<UStashEditorSettings>();
	CachedDeviceSpec = StashPreviewGetDeviceSpec(
		DevicePreset,
		Settings ? Settings->CustomDevicePlatform : EStashPreviewPlatform::iOS,
		Settings ? Settings->CustomDeviceWidth : 390.f,
		Settings ? Settings->CustomDeviceHeight : 844.f);
}
bool SStashPreviewPanel::IsTabletDevice() const
{
	return GetSelectedDeviceSpec().bTablet;
}
bool SStashPreviewPanel::IsAndroidPreset() const
{
	return GetSelectedDeviceSpec().Platform == EStashPreviewPlatform::Android;
}
bool SStashPreviewPanel::IsDeviceChromeEnabled() const
{
	const UStashEditorSettings* Settings = GetDefault<UStashEditorSettings>();
	return Settings && Settings->bShowDeviceChrome;
}
FMargin SStashPreviewPanel::GetEffectiveInsets() const
{
	return GetSelectedDeviceSpec().GetInsets(GetEffectiveLandscape());
}
bool SStashPreviewPanel::IsStatusContentLight() const
{
	const FStashPreviewSession& Session = FStashEditorPreviewService::Get()->GetSession();
	// Card/modal draw the status bar over the dim overlay (dark). Browser is full-bleed over the
	// checkout page, which is light in practice — so status glyphs must go dark to stay legible.
	return Session.bIsOpen && Session.PresentationMode == EStashPreviewPresentationMode::Browser;
}
FLinearColor SStashPreviewPanel::GetStatusGlyphColor() const
{
	return IsStatusContentLight()
		? FLinearColor(0.05f, 0.05f, 0.06f, 1.f)
		: FLinearColor::White;
}
float SStashPreviewPanel::GetStatusCutoutWidth() const
{
	if (GetEffectiveLandscape())
	{
		// Landscape status bar isn't split by a center cutout.
		return 0.f;
	}
	return ResolveNotchDims(GetSelectedDeviceSpec().NotchType, GetSelectedDeviceSize(), false).Width;
}
bool SStashPreviewPanel::ShouldShowCellular() const
{
	// Tablets are treated as wifi-only (no cellular glyph).
	return !GetSelectedDeviceSpec().bTablet;
}
float SStashPreviewPanel::GetKeyboardHeight() const
{
	const FStashPreviewDeviceSpec Spec = GetSelectedDeviceSpec();
	const bool bLandscape = GetEffectiveLandscape();
	const float SpecHeight = Spec.GetKeyboardHeight(bLandscape);
	// Never let the keyboard swallow more than half the (short) landscape screen.
	const float Cap = Spec.GetSize(bLandscape).Y * 0.5f;
	return FMath::Min(SpecHeight, Cap);
}
bool SStashPreviewPanel::GetEffectiveLandscape() const
{
	const FStashPreviewSession& Session = FStashEditorPreviewService::Get()->GetSession();
	if (Session.bIsOpen && Session.bForcePortraitLayout)
	{
		return false;
	}
	if (Session.bLandscapeLockWhenCardClosed)
	{
		// Native landscape lock keeps the game landscape-only except while a force-portrait card is presented.
		return true;
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
	return StashPreviewComputeCardLayout(
		Session.CardConfig,
		GetSelectedDeviceSpec(),
		GetEffectiveLandscape());
}

float SStashPreviewPanel::GetCardMaxExpandedHeight() const
{
	const FVector2D DeviceSize = GetSelectedDeviceSize();
	const FMargin Insets = GetEffectiveInsets();
	// Native expand never rises into the status-bar / notch zone.
	return FMath::Min(DeviceSize.Y * CardMaxExpandHeightRatio, DeviceSize.Y - Insets.Top);
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

	const FStashPreviewDeviceSpec Spec = GetSelectedDeviceSpec();
	const bool bEffectiveLandscape = GetEffectiveLandscape();
	const FVector2D DeviceSize = Spec.GetSize(bEffectiveLandscape);

	if (Session.PresentationMode == EStashPreviewPresentationMode::Browser)
	{
		FStashPreviewSheetLayout Layout;
		Layout.Width = DeviceSize.X;
		Layout.Height = DeviceSize.Y;
		Layout.HAlign = HAlign_Fill;
		Layout.VAlign = VAlign_Fill;
		Layout.SafeArea = Spec.GetInsets(bEffectiveLandscape);
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
	return StashPreviewComputeModalLayout(Session.ModalConfig, Spec, bEffectiveLandscape);
}

FMargin SStashPreviewPanel::ComputeSheetPadding(const FStashPreviewSheetLayout& Layout) const
{
	const FVector2D DeviceSize = GetSelectedDeviceSize();
	const float DeviceW = DeviceSize.X;
	const float DeviceH = DeviceSize.Y;
	const FStashPreviewSession& Session = FStashEditorPreviewService::Get()->GetSession();
	const FMargin Insets = Layout.SafeArea;
	FMargin Padding(0.f);

	if (Layout.VAlign == VAlign_Fill && Layout.HAlign == HAlign_Fill)
	{
		// Browser: full bleed; the keyboard shrinks it from the bottom (adjustResize / visualViewport).
		if (Session.bKeyboardVisible)
		{
			// GetKeyboardHeight already caps at half the screen, so no extra cap is needed here.
			Padding.Bottom = GetKeyboardHeight();
		}
		return Padding;
	}
	if (Layout.VAlign == VAlign_Bottom)
	{
		Padding.Top = FMath::Max(0.f, DeviceH - Layout.Height);
	}
	else if (Layout.VAlign == VAlign_Center)
	{
		const float SafeH = FMath::Max(0.f, DeviceH - Insets.Top - Insets.Bottom);
		float Top = Insets.Top + FMath::Max(0.f, (SafeH - Layout.Height) * 0.5f);
		float Bottom = FMath::Max(0.f, DeviceH - Top - Layout.Height);
		if (Session.bKeyboardVisible)
		{
			// Centered sheets (modal / tablet card) shift up so the keyboard doesn't cover them.
			const float KeyboardTop = DeviceH - GetKeyboardHeight();
			const float Overlap = (Top + Layout.Height) - (KeyboardTop - 8.f);
			if (Overlap > 0.f)
			{
				const float MinTop = Insets.Top + 4.f;
				const float Shift = FMath::Min(Overlap, FMath::Max(0.f, Top - MinTop));
				Top -= Shift;
				Bottom += Shift;
			}
		}
		Padding.Top = Top;
		Padding.Bottom = Bottom;
	}
	if (Layout.HAlign == HAlign_Center)
	{
		const float SafeW = FMath::Max(0.f, DeviceW - Insets.Left - Insets.Right);
		if (Layout.Width <= SafeW)
		{
			Padding.Left = Insets.Left + FMath::Max(0.f, (SafeW - Layout.Width) * 0.5f);
			Padding.Right = FMath::Max(0.f, DeviceW - Padding.Left - Layout.Width);
		}
		else
		{
			// Sheet wider than the safe area (e.g. full-width drawer): center on the full screen.
			const float HorizontalInset = FMath::Max(0.f, (DeviceW - Layout.Width) * 0.5f);
			Padding.Left = HorizontalInset;
			Padding.Right = HorizontalInset;
		}
	}
	return Padding;
}

float SStashPreviewPanel::GetWebContentBottomInset() const
{
	if (!IsCardPresentation())
	{
		return 0.f;
	}
	const FStashPreviewSheetLayout Layout = ComputeCurrentLayout();
	if (!Layout.bCardBottomDrawer)
	{
		return 0.f;
	}
	const FStashPreviewSession& Session = FStashEditorPreviewService::Get()->GetSession();
	// Sheet background extends into the home-indicator / gesture zone; web content stays above it.
	float Inset = Layout.SafeArea.Bottom;
	if (Session.bKeyboardVisible)
	{
		Inset = FMath::Max(Inset, GetKeyboardHeight());
	}
	const float MaxInset = FMath::Max(0.f, Layout.Height - DragHandleChromeHeight - 40.f);
	return FMath::Min(Inset, MaxInset);
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
		FVector2D Size = GetSelectedDeviceSize();
		if (Session.bKeyboardVisible)
		{
			// GetKeyboardHeight already caps at half the screen height.
			Size.Y = FMath::Max(1.f, Size.Y - GetKeyboardHeight());
		}
		return Size;
	}
	const FStashPreviewSheetLayout Layout = ComputeCurrentLayout();
	float WebW = Layout.Width;
	float WebH = Layout.Height;
	if (Layout.bShowDragHandle
		&& Session.PresentationMode == EStashPreviewPresentationMode::Card)
	{
		WebH = FMath::Max(1.f, WebH - DragHandleChromeHeight);
	}
	if (Layout.bCardBottomDrawer)
	{
		WebH = FMath::Max(1.f, WebH - GetWebContentBottomInset());
	}
	return FVector2D(WebW, WebH);
}
FReply SStashPreviewPanel::OnDimOverlayClicked()
{
	const FStashPreviewSession& Session = FStashEditorPreviewService::Get()->GetSession();
	if (Session.bKeyboardVisible)
	{
		// Tapping outside an input dismisses the keyboard first, like on device.
		FStashEditorPreviewService::Get()->SetKeyboardVisible(false, FString());
		return FReply::Handled();
	}
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

	// Resolve the device spec once per frame instead of in every attribute lambda (dozens of
	// struct-with-FString constructions + catalog scans per layout/paint pass). Refreshing here keeps
	// live Project Settings edits (Custom device dimensions) reflecting with at most one frame of latency.
	RefreshCachedDeviceSpec();
	if (CachedDeviceSpec.Platform != LastSyncedActivePlatform)
	{
		// Editing the Custom device platform must also update behavior that reads Session.ActivePlatform
		// (CloseBrowser / backdrop / keep-alive), not just the live chrome. Guarded so it fires only on change.
		LastSyncedActivePlatform = CachedDeviceSpec.Platform;
		FStashEditorPreviewService::Get()->SetActivePlatform(CachedDeviceSpec.Platform);
	}
	if (CachedDeviceSpec.UserAgent != LastPushedUserAgent)
	{
		// Live Project Settings edits (Custom platform / size crossing the tablet threshold) change the
		// emulated user-agent; push it and reload so the page re-fetches with the new platform identity.
		// Compared against this panel's own last-pushed UA (not the service-global) so two panels with
		// different presets can't ping-pong reloads by each "correcting" the shared value every frame.
		PushDeviceEmulation(true);
	}

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
	if (BackdropFlashRemaining > 0.f)
	{
		BackdropFlashRemaining -= InDeltaTime;
		if (PreviewOverlay.IsValid())
		{
			PreviewOverlay->Invalidate(EInvalidateWidget::LayoutAndVolatility);
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
FReply SStashPreviewPanel::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape && IsAndroidPreset())
	{
		FStashEditorPreviewService::Get()->HandleAndroidBack();
		return FReply::Handled();
	}
	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}
FVector2D SStashPreviewPanel::GetSelectedDeviceSize() const
{
	return GetSelectedDeviceSpec().GetSize(GetEffectiveLandscape());
}
FVector2D SStashPreviewPanel::GetDeviceFrameSize() const
{
	FVector2D Size = GetSelectedDeviceSize();
	if (IsDeviceChromeEnabled())
	{
		Size.X += 2.f * DeviceBezelThickness;
		Size.Y += 2.f * DeviceBezelThickness;
	}
	return Size;
}
FReply SStashPreviewPanel::OnReloadClicked()
{
	const FStashPreviewSession& Session = FStashEditorPreviewService::Get()->GetSession();
	if (Session.bIsOpen && WebBrowser.IsValid())
	{
		StartLoad(Session.CurrentUrl);
	}
	return FReply::Handled();
}
FReply SStashPreviewPanel::OnDismissClicked()
{
	FStashEditorPreviewService::Get()->SimulateDismiss();
	return FReply::Handled();
}
FReply SStashPreviewPanel::OnAndroidBackClicked()
{
	FStashEditorPreviewService::Get()->HandleAndroidBack();
	return FReply::Handled();
}
FReply SStashPreviewPanel::OnToggleKeyboardClicked()
{
	const FStashPreviewSession& Session = FStashEditorPreviewService::Get()->GetSession();
	FStashEditorPreviewService::Get()->SetKeyboardVisible(!Session.bKeyboardVisible, TEXT("text"));
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
FReply SStashPreviewPanel::OnSimulateProcessingClicked()
{
	FStashEditorPreviewService::Get()->SimulatePurchaseProcessing();
	return FReply::Handled();
}
FReply SStashPreviewPanel::OnSimulateProcessingCompletedClicked()
{
	FStashEditorPreviewService::Get()->SimulateProcessingCompleted();
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
	if (!NewSelection.IsValid())
	{
		return;
	}
	const int32 Index = DevicePresetOptions.IndexOfByKey(NewSelection);
	const TArray<EStashPreviewDevicePreset> PresetOrder = BuildPresetOrder();
	if (!PresetOrder.IsValidIndex(Index))
	{
		return;
	}
	SelectedDevicePreset = NewSelection;
	DevicePreset = PresetOrder[Index];
	bCardExpandedToMax = false;
	CardDragHeightOverride = 0.f;
	RebuildDeviceChromeBrushes();
	LastSyncedActivePlatform = GetSelectedDeviceSpec().Platform;
	FStashEditorPreviewService::Get()->SetActivePlatform(LastSyncedActivePlatform);
	// New device → new mobile user-agent; reload so the checkout re-fetches with the new platform identity.
	PushDeviceEmulation(true);
	RebuildPreviewChrome();
}
// --- Browser hosting & callback plumbing -------------------------------------------------------------
void SStashPreviewPanel::HandlePreviewSchemeNavigation(const FString& Url, bool bRestoreAfterBlank)
{
	if (!Url.StartsWith(StashPreviewJsBridge::SchemePrefix))
	{
		return;
	}

	FString Path;
	FString Query;
	if (StashPreviewCallbackUrl::ParsePreviewCallbackUrl(Url, Path, Query)
		&& StashPreviewCallbackUrl::IsPassiveCallbackPath(Path))
	{
		// Console-only events (keyboard focus): dispatch without touching webview navigation.
		FStashEditorPreviewService::Get()->DispatchPreviewCallbackUrl(Url);
		return;
	}

	if (!FStashEditorPreviewService::Get()->DispatchPreviewCallbackUrl(Url))
	{
		return;
	}

#if STASH_HAS_WEBBROWSER
	// Only the OnLoadUrl scheme-handler path blanks the webview (it answers the request with an empty
	// document), so it alone needs the checkout page restored. Console-message / blocked-navigation
	// callbacks leave the page intact — reloading them would wipe in-page state and visibly flash.
	if (bRestoreAfterBlank && WebBrowser.IsValid())
	{
		WebBrowser->StopLoad();
		const FStashPreviewSession& Session = FStashEditorPreviewService::Get()->GetSession();
		if (Session.bIsOpen && !LastLoadedUrl.IsEmpty())
		{
			StartLoad(LastLoadedUrl);
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

	// This path answers the request with a blank document below, so ask for the checkout page to be restored.
	HandlePreviewSchemeNavigation(Url, /*bRestoreAfterBlank*/ true);
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
		Path = Payload;
	}
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
		// navigator.* reflects CEF's (desktop) UA regardless of request headers, so also spoof it in JS
		// for client-side platform checks. Runs post-load — a best-effort complement to the header UA.
		const FStashPreviewDeviceSpec& Spec = GetSelectedDeviceSpec();
		WebBrowser->ExecuteJavascript(StashPreviewJsBridge::GetNavigatorSpoofScript(
			Spec.UserAgent,
			!Spec.bTablet,
			Spec.Platform == EStashPreviewPlatform::Android));
		WebBrowser->ExecuteJavascript(StashPreviewJsBridge::GetInjectionScript());
	}
#endif
}
void SStashPreviewPanel::ArmLoadTimer()
{
	// Reset load-progress state and arm the failure timer for a load that is about to start. Called only
	// when a load actually begins, so a session refresh mid-load can't keep postponing NotifyLoadError.
	FStashPreviewSession& Session = FStashEditorPreviewService::Get()->GetMutableSession();
	Session.LoadStartSeconds = FApp::GetCurrentTime();
	Session.bPageLoadedFired = false;
	NetworkTimeoutRemaining = LoadTimeoutSeconds;
}
void SStashPreviewPanel::StartLoad(const FString& Url)
{
#if STASH_HAS_WEBBROWSER
	if (!WebBrowser.IsValid())
	{
		return;
	}
	ArmLoadTimer();
	// Authoritative SDK injection happens in HandleLoadCompleted; no best-effort inject needed here.
	WebBrowser->LoadURL(Url);
#endif
}
void SStashPreviewPanel::PushDeviceEmulation(bool bReload)
{
	const FStashPreviewDeviceSpec& Spec = GetSelectedDeviceSpec();
	const FString PlatformHint = Spec.Platform == EStashPreviewPlatform::Android ? TEXT("Android") : TEXT("iOS");
	FStashEditorPreviewService::Get()->SetPreviewDeviceEmulation(Spec.UserAgent, !Spec.bTablet, PlatformHint);
	// Remember what THIS panel pushed so the per-Tick UA sync compares against its own state, not the global.
	LastPushedUserAgent = Spec.UserAgent;
#if STASH_HAS_WEBBROWSER
	if (bReload && WebBrowser.IsValid())
	{
		const FStashPreviewSession& Session = FStashEditorPreviewService::Get()->GetSession();
		if (Session.bIsOpen && !Session.CurrentUrl.IsEmpty())
		{
			StartLoad(Session.CurrentUrl);
		}
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
		// Make sure the emulated UA is live before the window's first request goes out.
		PushDeviceEmulation(false);

		// Build the window inside a request context whose resource-load hook injects the mobile
		// user-agent, so the checkout page renders its true per-platform experience.
		TSharedPtr<IWebBrowserWindow> BrowserWindow;
		if (IWebBrowserSingleton* Singleton = IWebBrowserModule::Get().GetSingleton())
		{
			FCreateBrowserWindowSettings WindowSettings;
			WindowSettings.InitialURL = Url;
			WindowSettings.bUseTransparency = false;
			WindowSettings.BackgroundColor = FColor(255, 255, 255, 255);
			FBrowserContextSettings ContextSettings(TEXT("StashPreviewMobileEmu"));
			ContextSettings.OnBeforeContextResourceLoad =
				FOnBeforeContextResourceLoadDelegate::CreateStatic(&StashPreviewInjectMobileHeaders);
			WindowSettings.Context = ContextSettings;
			BrowserWindow = Singleton->CreateBrowserWindow(WindowSettings);
		}

		// If the window couldn't be created, SWebBrowser falls back to its own (global-context) window.
		SAssignNew(WebBrowser, SWebBrowser, BrowserWindow)
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
		// The InitialURL kicks off a load; arm the load-failure timer for it.
		ArmLoadTimer();
	}
	else if (bUrlChanged)
	{
		StartLoad(Url);
	}
	if (WebBrowser.IsValid())
	{
		if (SBox* ActiveWebBox = GetActiveWebContentBox())
		{
			// Detach the browser from the other host first: PrepareSession switches PresentationMode in place
			// (e.g. OpenModal over an open card), so without this the one SWebBrowser would briefly be a child
			// of two slots — breaking Slate's parent-pointer/invalidation assumptions.
			SBox* InactiveWebBox = (ActiveWebBox == ModalWebContentBox.Get())
				? CardWebContentBox.Get()
				: ModalWebContentBox.Get();
			if (InactiveWebBox && InactiveWebBox != ActiveWebBox)
			{
				InactiveWebBox->SetContent(SNullWidget::NullWidget);
			}
			ActiveWebBox->SetContent(WebBrowser.ToSharedRef());
		}
	}
#else
	LastLoadedUrl = Url;
#endif
}
FLinearColor SStashPreviewPanel::ParseBackgroundColor(const FString& HtmlHex) const
{
	// BackgroundColor is an HTML/CSS hex string, matching stash-native's convention: #RGB, #RRGGBB, or
	// #RRGGBBAA — i.e. 8-digit values are RRGGBBAA (alpha last), NOT AARRGGBB. FColor::FromHex handles all
	// three CSS forms; the value is authored in sRGB space, so decode to linear for Slate brushes.
	const FLinearColor Fallback(0.12f, 0.12f, 0.14f, 1.f);
	const FString Trimmed = HtmlHex.TrimStartAndEnd();
	const FString Hex = Trimmed.StartsWith(TEXT("#")) ? Trimmed.Mid(1) : Trimmed;
	if (Hex.Len() != 3 && Hex.Len() != 4 && Hex.Len() != 6 && Hex.Len() != 8)
	{
		return Fallback;
	}
	return FLinearColor::FromSRGBColor(FColor::FromHex(Trimmed));
}
void SStashPreviewPanel::UpdateBackdropBrush(const FStashPreviewSession& Session)
{
	BackdropBrush.Reset();
	// The checkout backdrop is an Android-only mechanism (flash reduction during forced rotation).
	if (Session.BackdropBytes.Num() > 0 && Session.ActivePlatform == EStashPreviewPlatform::Android)
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

// --- Device chrome (bezel, notch, status bar, home indicator) ----------------------------------------
void SStashPreviewPanel::RebuildDeviceChromeBrushes()
{
	// Keep the cache current: this runs from Construct and on preset change, before the spec is read below.
	RefreshCachedDeviceSpec();
	const FStashPreviewDeviceSpec Spec = GetSelectedDeviceSpec();
	const FLinearColor BezelColor(0.015f, 0.015f, 0.02f, 1.f);
	const float BezelRadius = FMath::Max(Spec.DeviceCornerRadius, 6.f) + DeviceBezelThickness * 0.5f;
	BezelBrush = MakeShared<FSlateRoundedBoxBrush>(BezelColor, BezelRadius);

	const FLinearColor CutoutColor(0.f, 0.f, 0.f, 1.f);
	if (Spec.NotchType == EStashPreviewNotchType::None)
	{
		NotchBrush.Reset();
		NotchBrushLandscape.Reset();
	}
	else
	{
		// Corner radii per notch type come from the shared metrics table (notch is square on the bezel-flush
		// edge, rounded on the free edge; island/punch-hole are uniform pills).
		const FStashPreviewNotchMetrics Metrics = GetNotchMetrics(Spec.NotchType);
		NotchBrush = MakeShared<FSlateRoundedBoxBrush>(CutoutColor, Metrics.PortraitBrushCorners);
		NotchBrushLandscape = MakeShared<FSlateRoundedBoxBrush>(CutoutColor, Metrics.LandscapeBrushCorners);
	}

	// White brush; the overlay tints it to contrast with the content beneath.
	HomeIndicatorBrush = MakeShared<FSlateRoundedBoxBrush>(FLinearColor::White, 2.5f);
	KeepAliveBrush = MakeShared<FSlateRoundedBoxBrush>(FLinearColor(0.09f, 0.09f, 0.11f, 0.97f), 10.f);
	KeepAliveIconBrush = MakeShared<FSlateRoundedBoxBrush>(FLinearColor(0.95f, 0.35f, 0.25f, 1.f), 7.f);
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
			.Padding(TAttribute<FMargin>::CreateLambda([this]()
			{
				// Home-indicator / gesture / keyboard zone: shell color shows through beneath the web content.
				return FMargin(0.f, 0.f, 0.f, GetWebContentBottomInset());
			}))
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
TSharedRef<SWidget> SStashPreviewPanel::BuildDeviceChromeOverlays()
{
	const FSlateFontInfo StatusFont = FCoreStyle::GetDefaultFontStyle("Bold", 9);
	const FSlateFontInfo KeepAliveTitleFont = FCoreStyle::GetDefaultFontStyle("Bold", 9);
	const FSlateFontInfo KeepAliveTextFont = FCoreStyle::GetDefaultFontStyle("Regular", 8);

	const auto ChromeVisible = [this]() -> EVisibility
	{
		return IsDeviceChromeEnabled() ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
	};

	return SNew(SOverlay)
		.Visibility(EVisibility::SelfHitTestInvisible)

		// Simulated soft keyboard (behavior aid — shown regardless of the chrome toggle).
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Bottom)
		[
			SNew(SBox)
			.HeightOverride_Lambda([this]() { return GetKeyboardHeight(); })
			.Visibility_Lambda([this]()
			{
				const FStashPreviewSession& S = FStashEditorPreviewService::Get()->GetSession();
				return S.bIsOpen && S.bKeyboardVisible ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
			})
			[
				SNew(SStashPreviewKeyboard)
				.Platform_Lambda([this]() { return GetSelectedDeviceSpec().Platform; })
				.InputType_Lambda([this]()
				{
					return FStashEditorPreviewService::Get()->GetSession().KeyboardInputType;
				})
			]
		]

		// Status bar (transparent — content shows through; glyphs adapt to luminance beneath).
		// Split into left ear (clock) / center cutout gap / right ear (indicators), like iOS.
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Top)
		[
			SNew(SBox)
			.HeightOverride_Lambda([this]() { return FMath::Max(GetEffectiveInsets().Top, 0.f); })
			.Padding(FMargin(18.f, 0.f))
			.Visibility_Lambda([this, ChromeVisible]()
			{
				return GetEffectiveInsets().Top >= 16.f ? ChromeVisible() : EVisibility::Collapsed;
			})
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Left)
				[
					SNew(STextBlock)
					.Text(NSLOCTEXT("StashEditor", "StatusBarTime", "9:41"))
					.Font(StatusFont)
					.ColorAndOpacity_Lambda([this]() { return FSlateColor(GetStatusGlyphColor()); })
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SBox)
					.WidthOverride_Lambda([this]() { return GetStatusCutoutWidth(); })
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Right)
				[
					SNew(SStashPreviewStatusIcons)
					.GlyphColor_Lambda([this]() { return GetStatusGlyphColor(); })
					.bShowCellular_Lambda([this]() { return ShouldShowCellular(); })
				]
			]
		]

		// Portrait cutout: notch flush to the top bezel; island/punch-hole inset slightly.
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Top)
		[
			SNew(SBox)
			.WidthOverride_Lambda([this]() -> FOptionalSize
			{
				return ResolveNotchDims(GetSelectedDeviceSpec().NotchType, GetSelectedDeviceSize(), false).Width;
			})
			.HeightOverride_Lambda([this]() -> FOptionalSize
			{
				return ResolveNotchDims(GetSelectedDeviceSpec().NotchType, GetSelectedDeviceSize(), false).Height;
			})
			.Padding(0.f)
			.Visibility_Lambda([this, ChromeVisible]()
			{
				if (GetEffectiveLandscape() || GetSelectedDeviceSpec().NotchType == EStashPreviewNotchType::None)
				{
					return EVisibility::Collapsed;
				}
				return ChromeVisible();
			})
			[
				SNew(SBox)
				.Padding_Lambda([this]()
				{
					return ResolveNotchDims(GetSelectedDeviceSpec().NotchType, GetSelectedDeviceSize(), false).Padding;
				})
				[
					SNew(SImage)
					.Image_Lambda([this]() { return NotchBrush.Get(); })
				]
			]
		]

		// Landscape cutout: camera moves to the leading (left) edge when rotated.
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride_Lambda([this]() -> FOptionalSize
			{
				return ResolveNotchDims(GetSelectedDeviceSpec().NotchType, GetSelectedDeviceSize(), true).Width;
			})
			.HeightOverride_Lambda([this]() -> FOptionalSize
			{
				return ResolveNotchDims(GetSelectedDeviceSpec().NotchType, GetSelectedDeviceSize(), true).Height;
			})
			.Padding_Lambda([this]()
			{
				return ResolveNotchDims(GetSelectedDeviceSpec().NotchType, GetSelectedDeviceSize(), true).Padding;
			})
			.Visibility_Lambda([this, ChromeVisible]()
			{
				if (!GetEffectiveLandscape() || GetSelectedDeviceSpec().NotchType == EStashPreviewNotchType::None)
				{
					return EVisibility::Collapsed;
				}
				return ChromeVisible();
			})
			[
				SNew(SImage)
				.Image_Lambda([this]() { return NotchBrushLandscape.Get(); })
			]
		]

		// Home indicator (iOS) / gesture bar pill (Android) — tinted to contrast with content.
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(0.f, 0.f, 0.f, 8.f))
		[
			SNew(SBox)
			.WidthOverride_Lambda([this]() { return IsAndroidPreset() ? 108.f : 134.f; })
			.HeightOverride_Lambda([this]() { return IsAndroidPreset() ? 4.f : 5.f; })
			.Padding(0.f)
			.Visibility_Lambda([this, ChromeVisible]()
			{
				return GetEffectiveInsets().Bottom > 0.f ? ChromeVisible() : EVisibility::Collapsed;
			})
			[
				SNew(SImage)
				.Image_Lambda([this]() { return HomeIndicatorBrush.Get(); })
				.ColorAndOpacity_Lambda([this]() -> FSlateColor
				{
					const FStashPreviewSession& S = FStashEditorPreviewService::Get()->GetSession();
					const float BgLum = S.PresentationMode == EStashPreviewPresentationMode::Browser
						? 0.9f
						: 0.2126f * ActiveShellColor.R + 0.7152f * ActiveShellColor.G + 0.0722f * ActiveShellColor.B;
					return BgLum > 0.5f
						? FSlateColor(FLinearColor(0.f, 0.f, 0.f, 0.55f))
						: FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.85f));
				})
			]
		]

		// Safe-area guides (debug toggle): thin lines at the inset boundaries.
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Top)
		.Padding(TAttribute<FMargin>::CreateLambda([this]() { return FMargin(0.f, GetEffectiveInsets().Top, 0.f, 0.f); }))
		[
			SNew(SBox).HeightOverride(1.f)
			.Visibility_Lambda([this]()
			{
				const UStashEditorSettings* Settings = GetDefault<UStashEditorSettings>();
				return Settings && Settings->bShowSafeAreaGuides && GetEffectiveInsets().Top > 0.f
					? EVisibility::HitTestInvisible : EVisibility::Collapsed;
			})
			[
				SNew(SImage).Image(FAppStyle::GetBrush("WhiteBrush")).ColorAndOpacity(FLinearColor(0.2f, 0.8f, 1.f, 0.5f))
			]
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Bottom)
		.Padding(TAttribute<FMargin>::CreateLambda([this]() { return FMargin(0.f, 0.f, 0.f, GetEffectiveInsets().Bottom); }))
		[
			SNew(SBox).HeightOverride(1.f)
			.Visibility_Lambda([this]()
			{
				const UStashEditorSettings* Settings = GetDefault<UStashEditorSettings>();
				return Settings && Settings->bShowSafeAreaGuides && GetEffectiveInsets().Bottom > 0.f
					? EVisibility::HitTestInvisible : EVisibility::Collapsed;
			})
			[
				SNew(SImage).Image(FAppStyle::GetBrush("WhiteBrush")).ColorAndOpacity(FLinearColor(0.2f, 0.8f, 1.f, 0.5f))
			]
		]

		// Keep-alive foreground-service notification mock (Android, Browser mode).
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Top)
		.Padding(TAttribute<FMargin>::CreateLambda([this]()
		{
			return FMargin(10.f, GetEffectiveInsets().Top + 8.f, 10.f, 0.f);
		}))
		[
			SNew(SBorder)
			.BorderImage_Lambda([this]() { return KeepAliveBrush.Get(); })
			.Padding(FMargin(10.f, 8.f))
			.Visibility_Lambda([this]()
			{
				const FStashPreviewSession& S = FStashEditorPreviewService::Get()->GetSession();
				const bool bShow = S.bIsOpen
					&& S.ActivePlatform == EStashPreviewPlatform::Android
					&& S.bKeepAliveEnabled
					&& S.PresentationMode == EStashPreviewPresentationMode::Browser;
				return bShow ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
			})
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.f, 0.f, 8.f, 0.f)
				[
					SNew(SBox)
					.WidthOverride(14.f)
					.HeightOverride(14.f)
					[
						SNew(SImage)
						.Image_Lambda([this]() { return KeepAliveIconBrush.Get(); })
					]
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Font(KeepAliveTitleFont)
						.ColorAndOpacity(FLinearColor::White)
						.Text_Lambda([]()
						{
							const FStashPreviewSession& S = FStashEditorPreviewService::Get()->GetSession();
							return FText::FromString(S.KeepAliveConfig.NotificationTitle.IsEmpty()
								? TEXT("Stash Pay") : *S.KeepAliveConfig.NotificationTitle);
						})
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Font(KeepAliveTextFont)
						.ColorAndOpacity(FLinearColor(0.75f, 0.75f, 0.78f, 1.f))
						.Text_Lambda([]()
						{
							const FStashPreviewSession& S = FStashEditorPreviewService::Get()->GetSession();
							return FText::FromString(S.KeepAliveConfig.NotificationText.IsEmpty()
								? TEXT("Completing your purchase…") : *S.KeepAliveConfig.NotificationText);
						})
					]
				]
			]
		]

		// White rotation flash (Android force-portrait without a backdrop).
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FLinearColor::White)
			.Visibility_Lambda([this]()
			{
				return BackdropFlashRemaining > 0.f ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
			})
		]

		// Topmost: round the device screen corners over the (rectangular) webview and keyboard.
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SStashPreviewCornerMask)
			.Radius_Lambda([this]() { return IsDeviceChromeEnabled() ? GetSelectedDeviceSpec().DeviceCornerRadius : 0.f; })
			.MaskColor(FLinearColor(0.015f, 0.015f, 0.02f, 1.f))
		];
}
// --- Widget builders (preview area & controls panel) -------------------------------------------------
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
				SNew(SOverlay)
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
					.WidthOverride_Lambda([this]() { return GetDeviceFrameSize().X; })
					.HeightOverride_Lambda([this]() { return GetDeviceFrameSize().Y; })
					.Visibility_Lambda([]()
					{
						return FStashEditorPreviewService::Get()->GetSession().bIsOpen
							? EVisibility::Visible : EVisibility::Collapsed;
					})
					[
						SNew(SBorder)
						.BorderImage_Lambda([this]() -> const FSlateBrush*
						{
							return IsDeviceChromeEnabled() && BezelBrush.IsValid()
								? BezelBrush.Get()
								: FAppStyle::GetBrush("ToolPanel.GroupBorder");
						})
						.Padding_Lambda([this]()
						{
							return FMargin(IsDeviceChromeEnabled() ? DeviceBezelThickness : 0.f);
						})
						[
							SNew(SBorder)
							.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
							.Padding(0.f)
							.Clipping(EWidgetClipping::ClipToBounds)
							[
								SAssignNew(PreviewOverlay, SOverlay)
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
													// Match the drag hit region to the visible handle chrome so the extra band (which the
													// webview would consume mouse-downs from anyway) isn't an unreachable trap.
													.DragHeaderHeight(SStashPreviewPanel::DragHandleChromeHeight)
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
								+ SOverlay::Slot()
								[
									BuildDeviceChromeOverlays()
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
			.Padding(0.f, 4.f, 0.f, 0.f)
			[
				SNew(STextBlock)
				.Text_Lambda([this]()
				{
					const FStashPreviewDeviceSpec Spec = GetSelectedDeviceSpec();
					const FVector2D Physical = Spec.LogicalSize * Spec.ScaleFactor;
					return FText::FromString(FString::Printf(
						TEXT("%s %s · @%.3gx → %.0f x %.0f px"),
						*PlatformLabel(Spec.Platform),
						Spec.bTablet ? TEXT("tablet") : TEXT("phone"),
						Spec.ScaleFactor,
						Physical.X, Physical.Y));
				})
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 6.f, 0.f, 0.f)
			[
				SNew(SCheckBox)
				.IsChecked_Lambda([]()
				{
					const UStashEditorSettings* Settings = GetDefault<UStashEditorSettings>();
					return Settings && Settings->bShowDeviceChrome ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
				})
				.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
				{
					if (UStashEditorSettings* Settings = GetMutableDefault<UStashEditorSettings>())
					{
						Settings->bShowDeviceChrome = NewState == ECheckBoxState::Checked;
						Settings->SaveConfig();
					}
					RebuildPreviewChrome();
				})
				[
					SNew(STextBlock)
					.Text(NSLOCTEXT("StashEditor", "ShowDeviceChrome", "Show device chrome"))
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
						GetSelectedDeviceSpec(),
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
			.Padding(0.f, 0.f, 0.f, 4.f)
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("StashEditor", "BackdropFlashHint", "Tip: the white flash on forced rotation happens on real Android devices too — set a backdrop via Capture Viewport For Android Checkout Backdrop to avoid it."))
				.AutoWrapText(true)
				.ColorAndOpacity(FLinearColor(1.f, 0.7f, 0.3f, 1.f))
				.Visibility_Lambda([this]()
				{
					return bShowBackdropFlashHint ? EVisibility::Visible : EVisibility::Collapsed;
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
					if (S.bLandscapeLockWhenCardClosed)
					{
						return NSLOCTEXT("StashEditor", "ToggleLandscapeLocked", "Landscape (locked by Set Landscape Lock)");
					}
					return NSLOCTEXT("StashEditor", "ToggleLandscape", "Toggle landscape");
				})
				.IsEnabled_Lambda([]()
				{
					const FStashPreviewSession& S = FStashEditorPreviewService::Get()->GetSession();
					if (S.bLandscapeLockWhenCardClosed)
					{
						return false;
					}
					return !(S.bIsOpen && S.bForcePortraitLayout);
				})
				.OnClicked_Lambda([this]()
				{
					const FStashPreviewSession& S = FStashEditorPreviewService::Get()->GetSession();
					if (S.bLandscapeLockWhenCardClosed || (S.bIsOpen && S.bForcePortraitLayout))
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
			[
				SNew(SButton)
				.Text(NSLOCTEXT("StashEditor", "AndroidBack", "Back (Android)"))
				.ToolTipText(NSLOCTEXT("StashEditor", "AndroidBackTip", "Simulates the Android back gesture: hides the keyboard first, then dismisses the Stash UI (blocked while processing or when allowDismiss is off). Esc key does the same while the panel is focused."))
				.OnClicked(this, &SStashPreviewPanel::OnAndroidBackClicked)
				.Visibility_Lambda([this]()
				{
					return IsAndroidPreset() ? EVisibility::Visible : EVisibility::Collapsed;
				})
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SButton)
				.Text_Lambda([]()
				{
					const FStashPreviewSession& S = FStashEditorPreviewService::Get()->GetSession();
					return S.bKeyboardVisible
						? NSLOCTEXT("StashEditor", "HideKeyboard", "Hide keyboard")
						: NSLOCTEXT("StashEditor", "ShowKeyboard", "Show keyboard");
				})
				.ToolTipText(NSLOCTEXT("StashEditor", "ToggleKeyboardTip", "Simulates the soft keyboard. It also appears automatically when a checkout input field is focused."))
				.OnClicked(this, &SStashPreviewPanel::OnToggleKeyboardClicked)
				.IsEnabled_Lambda([]()
				{
					return FStashEditorPreviewService::Get()->GetSession().bIsOpen;
				})
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
				.Text(NSLOCTEXT("StashEditor", "SimProcessing", "Purchase Processing"))
				.OnClicked(this, &SStashPreviewPanel::OnSimulateProcessingClicked)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SButton)
				.Text(NSLOCTEXT("StashEditor", "SimProcessingDone", "Processing Completed"))
				.OnClicked(this, &SStashPreviewPanel::OnSimulateProcessingCompletedClicked)
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
						NSLOCTEXT("StashEditor", "PreviewDebugFmt", "Open: {0}  Processing: {1}  Keyboard: {2}\nLandscape lock: {3}  Keep-alive: {4}"),
						S.bIsOpen ? FText::FromString(TEXT("yes")) : FText::FromString(TEXT("no")),
						S.bIsPurchaseProcessing ? FText::FromString(TEXT("yes")) : FText::FromString(TEXT("no")),
						S.bKeyboardVisible ? FText::FromString(TEXT("yes")) : FText::FromString(TEXT("no")),
						S.bLandscapeLockWhenCardClosed ? FText::FromString(TEXT("on")) : FText::FromString(TEXT("off")),
						S.bKeepAliveEnabled ? FText::FromString(TEXT("on")) : FText::FromString(TEXT("off")));
				})
				.AutoWrapText(true)
			]
		];
}
