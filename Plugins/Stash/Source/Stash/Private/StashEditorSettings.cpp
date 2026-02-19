// Copyright Stash. All Rights Reserved.
// Stash Unreal Engine SDK - Editor Settings Implementation

#include "StashEditorSettings.h"

#define LOCTEXT_NAMESPACE "StashModule"

UStashEditorSettings::UStashEditorSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UStashEditorSettings::RegisterEditorSettings(ISettingsModule* SettingsModule)
{
	SettingsModule->RegisterSettings("Project", "Plugins", "Stash",
		LOCTEXT("SettingsName", "Stash"),
		LOCTEXT("SettingsDescription", "Configure Stash Native (card, modal, browser) integration"),
		GetMutableDefault<UStashEditorSettings>());
}

#undef LOCTEXT_NAMESPACE
