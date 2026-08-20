// Copyright Stash. All Rights Reserved.

#include "StashPreviewDeviceCatalog.h"

namespace
{
	// Mobile user-agent strings so the checkout page serves its true per-platform experience.
	// iOS → Mobile Safari (WebKit); Android → Chrome Mobile; Android tablets drop the "Mobile" token.
	const TCHAR* const UA_iPhone   = TEXT("Mozilla/5.0 (iPhone; CPU iPhone OS 17_5 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/17.5 Mobile/15E148 Safari/604.1");
	const TCHAR* const UA_iPad     = TEXT("Mozilla/5.0 (iPad; CPU OS 17_5 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/17.5 Mobile/15E148 Safari/604.1");
	const TCHAR* const UA_Pixel8   = TEXT("Mozilla/5.0 (Linux; Android 14; Pixel 8) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/125.0.0.0 Mobile Safari/537.36");
	const TCHAR* const UA_GalaxyS24= TEXT("Mozilla/5.0 (Linux; Android 14; SM-S921B) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/125.0.0.0 Mobile Safari/537.36");
	const TCHAR* const UA_PixelTab = TEXT("Mozilla/5.0 (Linux; Android 14; Pixel Tablet) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/125.0.0.0 Safari/537.36");
	const TCHAR* const UA_GalaxyTab= TEXT("Mozilla/5.0 (Linux; Android 14; SM-X710) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/125.0.0.0 Safari/537.36");

	FStashPreviewDeviceSpec MakeSpec(
		EStashPreviewDevicePreset Preset,
		EStashPreviewPlatform Platform,
		const TCHAR* DisplayName,
		FVector2D LogicalSize,
		float ScaleFactor,
		bool bTablet,
		FMargin PortraitInsets,
		FMargin LandscapeInsets,
		EStashPreviewNotchType NotchType,
		float DeviceCornerRadius,
		float KeyboardHeightPortrait,
		float KeyboardHeightLandscape,
		const TCHAR* UserAgent)
	{
		FStashPreviewDeviceSpec Spec;
		Spec.Preset = Preset;
		Spec.Platform = Platform;
		Spec.DisplayName = DisplayName;
		Spec.LogicalSize = LogicalSize;
		Spec.ScaleFactor = ScaleFactor;
		Spec.bTablet = bTablet;
		Spec.PortraitInsets = PortraitInsets;
		Spec.LandscapeInsets = LandscapeInsets;
		Spec.NotchType = NotchType;
		Spec.DeviceCornerRadius = DeviceCornerRadius;
		Spec.KeyboardHeightPortrait = KeyboardHeightPortrait;
		Spec.KeyboardHeightLandscape = KeyboardHeightLandscape;
		Spec.UserAgent = UserAgent;
		return Spec;
	}
}

const TArray<FStashPreviewDeviceSpec>& StashPreviewGetDeviceCatalog()
{
	// FMargin order: (Left, Top, Right, Bottom).
	static const TArray<FStashPreviewDeviceSpec> Catalog = {
		MakeSpec(EStashPreviewDevicePreset::iPhoneSE, EStashPreviewPlatform::iOS,
			TEXT("iPhone SE (375x667)"), FVector2D(375.f, 667.f), 2.f, false,
			FMargin(0.f, 20.f, 0.f, 0.f), FMargin(0.f, 0.f, 0.f, 0.f),
			EStashPreviewNotchType::None, 0.f, 260.f, 200.f, UA_iPhone),
		MakeSpec(EStashPreviewDevicePreset::iPhone14, EStashPreviewPlatform::iOS,
			TEXT("iPhone 14 (390x844)"), FVector2D(390.f, 844.f), 3.f, false,
			FMargin(0.f, 47.f, 0.f, 34.f), FMargin(47.f, 0.f, 47.f, 21.f),
			EStashPreviewNotchType::Notch, 47.f, 336.f, 210.f, UA_iPhone),
		MakeSpec(EStashPreviewDevicePreset::iPhone14ProMax, EStashPreviewPlatform::iOS,
			TEXT("iPhone 14 Pro Max (430x932)"), FVector2D(430.f, 932.f), 3.f, false,
			FMargin(0.f, 59.f, 0.f, 34.f), FMargin(59.f, 0.f, 59.f, 21.f),
			EStashPreviewNotchType::DynamicIsland, 55.f, 336.f, 210.f, UA_iPhone),
		MakeSpec(EStashPreviewDevicePreset::iPhone14Pro, EStashPreviewPlatform::iOS,
			TEXT("iPhone 14 Pro (393x852)"), FVector2D(393.f, 852.f), 3.f, false,
			FMargin(0.f, 59.f, 0.f, 34.f), FMargin(59.f, 0.f, 59.f, 21.f),
			EStashPreviewNotchType::DynamicIsland, 55.f, 336.f, 210.f, UA_iPhone),
		MakeSpec(EStashPreviewDevicePreset::iPad, EStashPreviewPlatform::iOS,
			TEXT("iPad (810x1080)"), FVector2D(810.f, 1080.f), 2.f, true,
			FMargin(0.f, 24.f, 0.f, 20.f), FMargin(0.f, 24.f, 0.f, 20.f),
			EStashPreviewNotchType::None, 18.f, 320.f, 260.f, UA_iPad),
		MakeSpec(EStashPreviewDevicePreset::iPadPro, EStashPreviewPlatform::iOS,
			TEXT("iPad Pro (1024x1366)"), FVector2D(1024.f, 1366.f), 2.f, true,
			FMargin(0.f, 24.f, 0.f, 20.f), FMargin(0.f, 24.f, 0.f, 20.f),
			EStashPreviewNotchType::None, 18.f, 320.f, 260.f, UA_iPad),
		MakeSpec(EStashPreviewDevicePreset::Pixel8, EStashPreviewPlatform::Android,
			TEXT("Pixel 8 (412x915)"), FVector2D(412.f, 915.f), 2.625f, false,
			FMargin(0.f, 28.f, 0.f, 24.f), FMargin(28.f, 24.f, 0.f, 24.f),
			EStashPreviewNotchType::PunchHole, 28.f, 282.f, 210.f, UA_Pixel8),
		MakeSpec(EStashPreviewDevicePreset::GalaxyS24, EStashPreviewPlatform::Android,
			TEXT("Galaxy S24 (360x780)"), FVector2D(360.f, 780.f), 3.f, false,
			FMargin(0.f, 28.f, 0.f, 24.f), FMargin(28.f, 24.f, 0.f, 24.f),
			EStashPreviewNotchType::PunchHole, 30.f, 282.f, 200.f, UA_GalaxyS24),
		MakeSpec(EStashPreviewDevicePreset::PixelTablet, EStashPreviewPlatform::Android,
			TEXT("Pixel Tablet (800x1280)"), FVector2D(800.f, 1280.f), 2.f, true,
			FMargin(0.f, 24.f, 0.f, 24.f), FMargin(0.f, 24.f, 0.f, 24.f),
			EStashPreviewNotchType::None, 24.f, 320.f, 240.f, UA_PixelTab),
		MakeSpec(EStashPreviewDevicePreset::GalaxyTabS9, EStashPreviewPlatform::Android,
			TEXT("Galaxy Tab S9 (800x1280)"), FVector2D(800.f, 1280.f), 2.f, true,
			FMargin(0.f, 24.f, 0.f, 24.f), FMargin(0.f, 24.f, 0.f, 24.f),
			EStashPreviewNotchType::None, 22.f, 320.f, 240.f, UA_GalaxyTab),
	};
	return Catalog;
}

FStashPreviewDeviceSpec StashPreviewGetDeviceSpec(
	EStashPreviewDevicePreset Preset,
	EStashPreviewPlatform CustomPlatform,
	float CustomWidth,
	float CustomHeight)
{
	if (Preset != EStashPreviewDevicePreset::Custom)
	{
		for (const FStashPreviewDeviceSpec& Spec : StashPreviewGetDeviceCatalog())
		{
			if (Spec.Preset == Preset)
			{
				return Spec;
			}
		}
	}

	FStashPreviewDeviceSpec Custom;
	Custom.Preset = EStashPreviewDevicePreset::Custom;
	Custom.Platform = CustomPlatform;
	Custom.DisplayName = TEXT("Custom");
	Custom.LogicalSize = FVector2D(CustomWidth, CustomHeight);
	Custom.ScaleFactor = CustomPlatform == EStashPreviewPlatform::Android ? 2.625f : 3.f;
	Custom.bTablet = FMath::Max(CustomWidth, CustomHeight) >= 768.f && FMath::Min(CustomWidth, CustomHeight) >= 600.f;
	Custom.PortraitInsets = FMargin(0.f);
	Custom.LandscapeInsets = FMargin(0.f);
	Custom.NotchType = EStashPreviewNotchType::None;
	Custom.DeviceCornerRadius = 0.f;
	Custom.KeyboardHeightPortrait = CustomPlatform == EStashPreviewPlatform::Android ? 282.f : 300.f;
	Custom.KeyboardHeightLandscape = 210.f;
	if (CustomPlatform == EStashPreviewPlatform::Android)
	{
		Custom.UserAgent = Custom.bTablet ? UA_PixelTab : UA_Pixel8;
	}
	else
	{
		Custom.UserAgent = Custom.bTablet ? UA_iPad : UA_iPhone;
	}
	return Custom;
}
