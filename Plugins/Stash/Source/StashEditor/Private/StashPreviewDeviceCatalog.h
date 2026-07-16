// Copyright Stash. All Rights Reserved.
// Per-preset device metadata driving the editor preview: platform, logical size,
// safe-area insets, chrome (notch / punch-hole / home indicator), and keyboard heights.
// Values come from public device specs, not measured hardware — approximations.

#pragma once

#include "CoreMinimal.h"
#include "Layout/Margin.h"
#include "StashEditorSettings.h"

enum class EStashPreviewNotchType : uint8
{
	None,
	Notch,
	DynamicIsland,
	PunchHole
};

struct FStashPreviewDeviceSpec
{
	EStashPreviewDevicePreset Preset = EStashPreviewDevicePreset::iPhone14;
	EStashPreviewPlatform Platform = EStashPreviewPlatform::iOS;
	FString DisplayName;
	/** Logical points (iOS) / dp (Android), portrait orientation. */
	FVector2D LogicalSize = FVector2D(390.f, 844.f);
	/** Logical → physical pixel scale (@2x, @3x, @2.625x). Readout only; the preview renders at 1:1 Slate units. */
	float ScaleFactor = 3.f;
	bool bTablet = false;
	/** Safe-area insets in portrait: Top = status bar / notch, Bottom = home indicator / gesture bar. */
	FMargin PortraitInsets = FMargin(0.f);
	FMargin LandscapeInsets = FMargin(0.f);
	EStashPreviewNotchType NotchType = EStashPreviewNotchType::None;
	/** Screen corner radius (device silhouette). */
	float DeviceCornerRadius = 0.f;
	/** Soft-keyboard heights including suggestion bar. */
	float KeyboardHeightPortrait = 300.f;
	float KeyboardHeightLandscape = 210.f;
	/** Mobile user-agent injected into the CEF webview so the checkout serves its true per-platform experience. */
	FString UserAgent;

	FVector2D GetSize(bool bLandscape) const
	{
		return bLandscape ? FVector2D(LogicalSize.Y, LogicalSize.X) : LogicalSize;
	}

	const FMargin& GetInsets(bool bLandscape) const
	{
		return bLandscape ? LandscapeInsets : PortraitInsets;
	}

	float GetKeyboardHeight(bool bLandscape) const
	{
		return bLandscape ? KeyboardHeightLandscape : KeyboardHeightPortrait;
	}
};

/** All presets except Custom, in display order. */
const TArray<FStashPreviewDeviceSpec>& StashPreviewGetDeviceCatalog();

/** Spec for a preset; Custom is built from the given settings values. */
FStashPreviewDeviceSpec StashPreviewGetDeviceSpec(
	EStashPreviewDevicePreset Preset,
	EStashPreviewPlatform CustomPlatform,
	float CustomWidth,
	float CustomHeight);
