// Copyright Stash. All Rights Reserved.
// Stash Pay Unreal Engine SDK - Editor Settings

#pragma once

#include "Developer/Settings/Public/ISettingsModule.h"
#include "StashEditorSettings.generated.h"

/**
 * Stash Plugin Settings
 * 
 * Configuration options for the Stash Pay checkout integration.
 * Access via: Project Settings -> Plugins -> Stash
 */
UCLASS(config = Engine, defaultconfig)
class UStashEditorSettings : public UObject
{
	GENERATED_BODY()

public:
	UStashEditorSettings(const FObjectInitializer& ObjectInitializer);

	static void RegisterEditorSettings(ISettingsModule* SettingsModule);
};
