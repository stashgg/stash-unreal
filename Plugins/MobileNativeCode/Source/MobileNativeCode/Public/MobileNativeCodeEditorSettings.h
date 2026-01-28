// Copyright Stash. All Rights Reserved.
// Stash Pay Unreal Engine SDK - Editor Settings

#pragma once

#include "Developer/Settings/Public/ISettingsModule.h"
#include "MobileNativeCodeEditorSettings.generated.h"

/**
 * Stash Pay Plugin Settings
 * 
 * Configuration options for the Stash Pay checkout integration.
 * Access via: Project Settings -> Plugins -> StashPay
 */
UCLASS(config = Engine, defaultconfig)
class UMobileNativeCodeEditorSettings : public UObject
{
	GENERATED_BODY()

public:
	UMobileNativeCodeEditorSettings(const FObjectInitializer& ObjectInitializer);

	static void RegisterEditorSettings(ISettingsModule* SettingsModule);
};
