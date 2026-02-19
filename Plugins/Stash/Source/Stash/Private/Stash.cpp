// Copyright Stash. All Rights Reserved.
// Stash Unreal Engine SDK - Module Implementation

#include "Stash.h"
#include "StashEditorSettings.h"

#if PLATFORM_ANDROID
#include "Android/Utils/AndroidUtils.h"
#endif

// Define the log category declared in Stash.h
DEFINE_LOG_CATEGORY(LogStash);

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
	// No cleanup needed - native SDKs handle their own lifecycle
	// iOS: StashNativeCardWrapper is a singleton that persists
	// Android: StashNativeCard is a singleton managed by the SDK
}

void FStashModule::Initialization()
{
#if PLATFORM_ANDROID
	AndroidUtils::Initialization();
#endif

#if PLATFORM_IOS
	// iOS initialization handled by StashNativeCardWrapper
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
