// Copyright Stash. All Rights Reserved.

#include "StashPreviewLayout.h"

namespace
{
	float ClampRatio(float Ratio)
	{
		return FMath::Clamp(Ratio, 0.1f, 1.0f);
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

bool StashPreviewIsTabletDevice(
	EStashPreviewDevicePreset Preset,
	float CustomDeviceWidth,
	float CustomDeviceHeight)
{
	switch (Preset)
	{
	case EStashPreviewDevicePreset::iPad:
	case EStashPreviewDevicePreset::iPadPro:
		return true;
	case EStashPreviewDevicePreset::Custom:
		return FMath::Max(CustomDeviceWidth, CustomDeviceHeight) >= 768.f;
	default:
		return false;
	}
}

FStashPreviewSheetLayout StashPreviewComputeCardLayout(
	const FStashCardConfig& Config,
	float DeviceW,
	float DeviceH,
	bool bDeviceLandscape,
	bool bIsTabletDevice)
{
	FStashPreviewSheetLayout Layout;
	Layout.bIsTablet = bIsTabletDevice;
	const bool bPortrait = UsePortraitLayout(bDeviceLandscape, Config.bForcePortrait);

	if (bIsTabletDevice)
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
	float DeviceW,
	float DeviceH,
	bool bDeviceLandscape,
	bool bIsTabletDevice)
{
	FStashPreviewSheetLayout Layout;
	Layout.bIsTablet = bIsTabletDevice;
	Layout.bIsModal = true;
	Layout.bCardBottomDrawer = false;
	Layout.bShowDragHandle = false;
	Layout.HAlign = HAlign_Center;
	Layout.VAlign = VAlign_Center;

	if (bIsTabletDevice)
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
	bool bDeviceLandscape,
	bool bForcePortrait)
{
	if (Mode == EStashPreviewPresentationMode::Browser)
	{
		return TEXT("Browser: full bleed");
	}

	const FString DeviceClass = Layout.bIsTablet ? TEXT("tablet") : TEXT("phone");
	const FString SheetSize = FString::Printf(TEXT("%.0f x %.0f"), Layout.Width, Layout.Height);

	if (Mode == EStashPreviewPresentationMode::Card)
	{
		return FString::Printf(
			TEXT("Card (%s, %s drawer)\nforcePortrait: %s\nratios: %s\nshell: %s\napplied size: %s"),
			*DeviceClass,
			Layout.bCardBottomDrawer ? TEXT("bottom") : TEXT("centered"),
			CardConfig.bForcePortrait ? TEXT("yes") : TEXT("no"),
			*DescribeCardRatios(CardConfig, Layout, bDeviceLandscape, bForcePortrait),
			CardConfig.BackgroundColor.IsEmpty() ? TEXT("(default)") : *CardConfig.BackgroundColor,
			*SheetSize);
	}

	return FString::Printf(
		TEXT("Modal (%s, centered)\nallowDismiss: %s\nratios: %s\nshell: %s\napplied size: %s"),
		*DeviceClass,
		ModalConfig.bAllowDismiss ? TEXT("yes") : TEXT("no"),
		*DescribeModalRatios(ModalConfig, Layout, bDeviceLandscape),
		ModalConfig.BackgroundColor.IsEmpty() ? TEXT("(default)") : *ModalConfig.BackgroundColor,
		*SheetSize);
}
