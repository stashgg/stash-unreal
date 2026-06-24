// Copyright Stash. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StashBlueprint.h"
#include "StashEditorSettings.h"
#include "StashEditorPreviewBridge.h"

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
};

bool StashPreviewIsTabletDevice(
	EStashPreviewDevicePreset Preset,
	float CustomDeviceWidth,
	float CustomDeviceHeight);

FStashPreviewSheetLayout StashPreviewComputeCardLayout(
	const FStashCardConfig& Config,
	float DeviceW,
	float DeviceH,
	bool bDeviceLandscape,
	bool bIsTabletDevice);

FStashPreviewSheetLayout StashPreviewComputeModalLayout(
	const FStashModalConfig& Config,
	float DeviceW,
	float DeviceH,
	bool bDeviceLandscape,
	bool bIsTabletDevice);

FString StashPreviewDescribeActiveConfig(
	EStashPreviewPresentationMode Mode,
	const FStashPreviewSheetLayout& Layout,
	const FStashCardConfig& CardConfig,
	const FStashModalConfig& ModalConfig,
	bool bDeviceLandscape,
	bool bForcePortrait);
