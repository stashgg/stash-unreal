// Copyright Stash. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StashBlueprint.h"
#include "StashEditorSettings.h"
#include "StashEditorPreviewBridge.h"
#include "StashPreviewDeviceCatalog.h"

struct FStashPreviewSheetLayout
{
	float Width = 0.f;
	float Height = 0.f;
	EHorizontalAlignment HAlign = HAlign_Center;
	EVerticalAlignment VAlign = VAlign_Center;
	bool bIsTablet = false;
	bool bIsModal = false;
	bool bCardBottomDrawer = false;
	bool bShowDragHandle = false;
	/** Effective safe-area insets for the current orientation (status bar / home indicator / gesture bar). */
	FMargin SafeArea = FMargin(0.f);
};

FStashPreviewSheetLayout StashPreviewComputeCardLayout(
	const FStashCardConfig& Config,
	const FStashPreviewDeviceSpec& Spec,
	bool bDeviceLandscape);

FStashPreviewSheetLayout StashPreviewComputeModalLayout(
	const FStashModalConfig& Config,
	const FStashPreviewDeviceSpec& Spec,
	bool bDeviceLandscape);

FString StashPreviewDescribeActiveConfig(
	EStashPreviewPresentationMode Mode,
	const FStashPreviewSheetLayout& Layout,
	const FStashCardConfig& CardConfig,
	const FStashModalConfig& ModalConfig,
	const FStashPreviewDeviceSpec& Spec,
	bool bDeviceLandscape,
	bool bForcePortrait);
