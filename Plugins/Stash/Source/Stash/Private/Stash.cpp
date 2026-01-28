// Copyright Stash. All Rights Reserved.
// Stash Pay Unreal Engine SDK - Module Implementation

#include "Stash.h"
#include "StashEditorSettings.h"

#if PLATFORM_ANDROID
#include "Android/Utils/AndroidUtils.h"
#endif

#define LOCTEXT_NAMESPACE "StashModule"

void FStashModule::StartupModule()
{
	// Register settings in Project Settings -> Plugins -> Stash
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		UStashEditorSettings::RegisterEditorSettings(SettingsModule);
	}

	// Initialize platform-specific components
	Initialization();
}

void FStashModule::ShutdownModule()
{
	// Cleanup if needed
}

void FStashModule::Initialization()
{
#if PLATFORM_ANDROID
	AndroidUtils::Initialization();
#endif

#if PLATFORM_IOS
	// iOS initialization handled by StashPayCardWrapper
#endif
}

bool FStashModule::IsSupported()
{
#if PLATFORM_ANDROID
	return AndroidUtils::isSupportPlatform();
#elif PLATFORM_IOS
	return true;
#else
	return false;
#endif
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FStashModule, Stash)
