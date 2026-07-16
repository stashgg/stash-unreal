// Copyright Stash. All Rights Reserved.

#include "StashPreviewLayout.h"

namespace
{
	float ClampRatio(float Ratio)
	{
		// StashRatioMin/Max come from the runtime module (StashBlueprint.h) so the preview clamps
		// to the same range as the device — one source of truth for the SDK's accepted range.
		return FMath::Clamp(Ratio, StashRatioMin, StashRatioMax);
	}

	bool UsePortraitLayout(bool bDeviceLandscape, bool bForcePortrait)
	{
		return bForcePortrait || !bDeviceLandscape;
	}

	FString DescribeCardRatios(
		const FStashCardConfig& Config,
		const FStashPreviewSheetLayout& Layout,
		bool bDeviceLandscape,
		bool bForcePortrait)
	{
		const bool bPortrait = UsePortraitLayout(bDeviceLandscape, bForcePortrait);
		if (Layout.bIsTablet)
		{
			if (bPortrait)
			{
				return FString::Printf(
					TEXT("TabletWidthRatioPortrait=%.2f, TabletHeightRatioPortrait=%.2f"),
					Config.TabletWidthRatioPortrait,
					Config.TabletHeightRatioPortrait);
			}
			return FString::Printf(
				TEXT("TabletWidthRatioLandscape=%.2f, TabletHeightRatioLandscape=%.2f"),
				Config.TabletWidthRatioLandscape,
				Config.TabletHeightRatioLandscape);
		}
		if (bPortrait)
		{
			return FString::Printf(TEXT("CardHeightRatioPortrait=%.2f"), Config.CardHeightRatioPortrait);
		}
		return FString::Printf(
			TEXT("CardWidthRatioLandscape=%.2f, CardHeightRatioLandscape=%.2f"),
			Config.CardWidthRatioLandscape,
			Config.CardHeightRatioLandscape);
	}

	FString DescribeModalRatios(
		const FStashModalConfig& Config,
		const FStashPreviewSheetLayout& Layout,
		bool bDeviceLandscape)
	{
		if (Layout.bIsTablet)
		{
			if (bDeviceLandscape)
			{
				return FString::Printf(
					TEXT("TabletWidthRatioLandscape=%.2f, TabletHeightRatioLandscape=%.2f"),
					Config.TabletWidthRatioLandscape,
					Config.TabletHeightRatioLandscape);
			}
			return FString::Printf(
				TEXT("TabletWidthRatioPortrait=%.2f, TabletHeightRatioPortrait=%.2f"),
				Config.TabletWidthRatioPortrait,
				Config.TabletHeightRatioPortrait);
		}
		if (bDeviceLandscape)
		{
			return FString::Printf(
				TEXT("PhoneWidthRatioLandscape=%.2f, PhoneHeightRatioLandscape=%.2f"),
				Config.PhoneWidthRatioLandscape,
				Config.PhoneHeightRatioLandscape);
		}
		return FString::Printf(
			TEXT("PhoneWidthRatioPortrait=%.2f, PhoneHeightRatioPortrait=%.2f"),
			Config.PhoneWidthRatioPortrait,
			Config.PhoneHeightRatioPortrait);
	}
}

FStashPreviewSheetLayout StashPreviewComputeCardLayout(
	const FStashCardConfig& Config,
	const FStashPreviewDeviceSpec& Spec,
	bool bDeviceLandscape)
{
	FStashPreviewSheetLayout Layout;
	Layout.bIsTablet = Spec.bTablet;
	const bool bPortrait = UsePortraitLayout(bDeviceLandscape, Config.bForcePortrait);
	const bool bEffectiveLandscape = !bPortrait;
	const FVector2D DeviceSize = Spec.GetSize(bEffectiveLandscape);
	const float DeviceW = DeviceSize.X;
	const float DeviceH = DeviceSize.Y;
	Layout.SafeArea = Spec.GetInsets(bEffectiveLandscape);

	if (Spec.bTablet)
	{
		if (bPortrait)
		{
			Layout.Width = DeviceW * ClampRatio(Config.TabletWidthRatioPortrait);
			Layout.Height = DeviceH * ClampRatio(Config.TabletHeightRatioPortrait);
		}
		else
		{
			Layout.Width = DeviceW * ClampRatio(Config.TabletWidthRatioLandscape);
			Layout.Height = DeviceH * ClampRatio(Config.TabletHeightRatioLandscape);
		}
		Layout.HAlign = HAlign_Center;
		Layout.VAlign = VAlign_Center;
		Layout.bCardBottomDrawer = false;
		Layout.bShowDragHandle = true;
	}
	else if (bPortrait)
	{
		Layout.Width = DeviceW;
		Layout.Height = DeviceH * ClampRatio(Config.CardHeightRatioPortrait);
		Layout.HAlign = HAlign_Center;
		Layout.VAlign = VAlign_Bottom;
		Layout.bCardBottomDrawer = true;
		Layout.bShowDragHandle = true;
	}
	else
	{
		Layout.Width = DeviceW * ClampRatio(Config.CardWidthRatioLandscape);
		Layout.Height = DeviceH * ClampRatio(Config.CardHeightRatioLandscape);
		Layout.HAlign = HAlign_Center;
		Layout.VAlign = VAlign_Bottom;
		Layout.bCardBottomDrawer = true;
		Layout.bShowDragHandle = true;
	}

	return Layout;
}

FStashPreviewSheetLayout StashPreviewComputeModalLayout(
	const FStashModalConfig& Config,
	const FStashPreviewDeviceSpec& Spec,
	bool bDeviceLandscape)
{
	FStashPreviewSheetLayout Layout;
	Layout.bIsTablet = Spec.bTablet;
	Layout.bIsModal = true;
	Layout.bCardBottomDrawer = false;
	Layout.bShowDragHandle = false;
	Layout.HAlign = HAlign_Center;
	Layout.VAlign = VAlign_Center;
	const FVector2D DeviceSize = Spec.GetSize(bDeviceLandscape);
	const float DeviceW = DeviceSize.X;
	const float DeviceH = DeviceSize.Y;
	Layout.SafeArea = Spec.GetInsets(bDeviceLandscape);

	if (Spec.bTablet)
	{
		if (bDeviceLandscape)
		{
			Layout.Width = DeviceW * ClampRatio(Config.TabletWidthRatioLandscape);
			Layout.Height = DeviceH * ClampRatio(Config.TabletHeightRatioLandscape);
		}
		else
		{
			Layout.Width = DeviceW * ClampRatio(Config.TabletWidthRatioPortrait);
			Layout.Height = DeviceH * ClampRatio(Config.TabletHeightRatioPortrait);
		}
	}
	else if (bDeviceLandscape)
	{
		Layout.Width = DeviceW * ClampRatio(Config.PhoneWidthRatioLandscape);
		Layout.Height = DeviceH * ClampRatio(Config.PhoneHeightRatioLandscape);
	}
	else
	{
		Layout.Width = DeviceW * ClampRatio(Config.PhoneWidthRatioPortrait);
		Layout.Height = DeviceH * ClampRatio(Config.PhoneHeightRatioPortrait);
	}

	return Layout;
}

FString StashPreviewDescribeActiveConfig(
	EStashPreviewPresentationMode Mode,
	const FStashPreviewSheetLayout& Layout,
	const FStashCardConfig& CardConfig,
	const FStashModalConfig& ModalConfig,
	const FStashPreviewDeviceSpec& Spec,
	bool bDeviceLandscape,
	bool bForcePortrait)
{
	// Resolution is shown under the device dropdown; keep this block focused on layout-affecting facts.
	const FString DeviceLine = FString::Printf(
		TEXT("safe area: T=%.0f B=%.0f L=%.0f R=%.0f"),
		Layout.SafeArea.Top, Layout.SafeArea.Bottom, Layout.SafeArea.Left, Layout.SafeArea.Right);

	if (Mode == EStashPreviewPresentationMode::Browser)
	{
		return DeviceLine + TEXT("\nBrowser: full bleed");
	}

	const FString SheetSize = FString::Printf(TEXT("%.0f x %.0f"), Layout.Width, Layout.Height);

	if (Mode == EStashPreviewPresentationMode::Card)
	{
		return FString::Printf(
			TEXT("%s\nCard (%s drawer)\nforcePortrait: %s\nratios: %s\nshell: %s\napplied size: %s"),
			*DeviceLine,
			Layout.bCardBottomDrawer ? TEXT("bottom") : TEXT("centered"),
			CardConfig.bForcePortrait ? TEXT("yes") : TEXT("no"),
			*DescribeCardRatios(CardConfig, Layout, bDeviceLandscape, bForcePortrait),
			CardConfig.BackgroundColor.IsEmpty() ? TEXT("(default)") : *CardConfig.BackgroundColor,
			*SheetSize);
	}

	return FString::Printf(
		TEXT("%s\nModal (centered)\nallowDismiss: %s\nratios: %s\nshell: %s\napplied size: %s"),
		*DeviceLine,
		ModalConfig.bAllowDismiss ? TEXT("yes") : TEXT("no"),
		*DescribeModalRatios(ModalConfig, Layout, bDeviceLandscape),
		ModalConfig.BackgroundColor.IsEmpty() ? TEXT("(default)") : *ModalConfig.BackgroundColor,
		*SheetSize);
}
