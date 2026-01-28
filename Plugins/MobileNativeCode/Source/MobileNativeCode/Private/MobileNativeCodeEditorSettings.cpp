// Copyright Stash. All Rights Reserved.
// Stash Pay Unreal Engine SDK - Editor Settings Implementation

#include "MobileNativeCodeEditorSettings.h"

#define LOCTEXT_NAMESPACE "StashPayModule"

UMobileNativeCodeEditorSettings::UMobileNativeCodeEditorSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UMobileNativeCodeEditorSettings::RegisterEditorSettings(ISettingsModule* SettingsModule)
{
	SettingsModule->RegisterSettings("Project", "Plugins", "StashPay",
		LOCTEXT("SettingsName", "StashPay"),
		LOCTEXT("SettingsDescription", "Configure Stash Pay checkout integration"),
		GetMutableDefault<UMobileNativeCodeEditorSettings>());
}

#undef LOCTEXT_NAMESPACE
