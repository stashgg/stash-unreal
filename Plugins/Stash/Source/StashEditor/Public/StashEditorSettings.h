// Copyright Stash. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "StashEditorSettings.generated.h"

/** Mobile platform the preview device emulates (drives back button, Close Browser semantics, keep-alive, chrome styling). */
UENUM(BlueprintType)
enum class EStashPreviewPlatform : uint8
{
	iOS     UMETA(DisplayName = "iOS"),
	Android UMETA(DisplayName = "Android")
};

UENUM(BlueprintType)
enum class EStashPreviewDevicePreset : uint8
{
	iPhoneSE       UMETA(DisplayName = "iPhone SE (375x667)"),
	iPhone14       UMETA(DisplayName = "iPhone 14 (390x844)"),
	iPhone14ProMax UMETA(DisplayName = "iPhone 14 Pro Max (430x932)"),
	iPhone14Pro    UMETA(DisplayName = "iPhone 14 Pro (393x852)"),
	iPad           UMETA(DisplayName = "iPad (810x1080)"),
	iPadPro        UMETA(DisplayName = "iPad Pro (1024x1366)"),
	Pixel8         UMETA(DisplayName = "Pixel 8 (412x915)"),
	GalaxyS24      UMETA(DisplayName = "Galaxy S24 (360x780)"),
	PixelTablet    UMETA(DisplayName = "Pixel Tablet (800x1280)"),
	GalaxyTabS9    UMETA(DisplayName = "Galaxy Tab S9 (800x1280)"),
	Custom         UMETA(DisplayName = "Custom")
};

/**
 * Project settings for the Stash editor checkout preview (editor-only).
 * Edit → Project Settings → Plugins → Stash.
 */
UCLASS(Config = Editor, DefaultConfig, meta = (DisplayName = "Stash"))
class STASHEDITOR_API UStashEditorSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UStashEditorSettings();

	/** When enabled, Open Card / Open Modal in the editor route to the Stash Preview panel instead of no-op. */
	UPROPERTY(Config, EditAnywhere, Category = "Editor Preview")
	bool bEnableEditorPreview = true;

	/** When enabled, Open Browser shows inside the preview panel (Close Browser works). When disabled, opens the OS browser (Unity parity). */
	UPROPERTY(Config, EditAnywhere, Category = "Editor Preview")
	bool bOpenBrowserInPreviewPanel = true;

	/** Automatically open or focus the Stash Preview tab when a preview session starts. */
	UPROPERTY(Config, EditAnywhere, Category = "Editor Preview")
	bool bAutoOpenPreviewTab = true;

	/** Show phone chrome around the preview: bezel, status bar, notch/punch-hole, and home indicator / gesture bar. */
	UPROPERTY(Config, EditAnywhere, Category = "Editor Preview")
	bool bShowDeviceChrome = true;

	/** Overlay faint guide lines at the safe-area inset boundaries (debugging aid). */
	UPROPERTY(Config, EditAnywhere, Category = "Editor Preview")
	bool bShowSafeAreaGuides = false;

	/** Default device frame used when the preview tab opens. */
	UPROPERTY(Config, EditAnywhere, Category = "Editor Preview")
	EStashPreviewDevicePreset DefaultDevicePreset = EStashPreviewDevicePreset::iPhone14;

	/** Platform the Custom device emulates. */
	UPROPERTY(Config, EditAnywhere, Category = "Editor Preview", meta = (EditCondition = "DefaultDevicePreset == EStashPreviewDevicePreset::Custom"))
	EStashPreviewPlatform CustomDevicePlatform = EStashPreviewPlatform::iOS;

	/** Custom device width when Default Device Preset is Custom. */
	UPROPERTY(Config, EditAnywhere, Category = "Editor Preview", meta = (EditCondition = "DefaultDevicePreset == EStashPreviewDevicePreset::Custom", ClampMin = "200", ClampMax = "2048"))
	float CustomDeviceWidth = 390.f;

	/** Custom device height when Default Device Preset is Custom. */
	UPROPERTY(Config, EditAnywhere, Category = "Editor Preview", meta = (EditCondition = "DefaultDevicePreset == EStashPreviewDevicePreset::Custom", ClampMin = "200", ClampMax = "2048"))
	float CustomDeviceHeight = 844.f;

	virtual FName GetCategoryName() const override { return FName(TEXT("Plugins")); }
};
