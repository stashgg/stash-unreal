// Copyright Stash. All Rights Reserved.
// Stash Pay Unreal Engine SDK - Module Implementation

#include "MobileNativeCode.h"
#include "MobileNativeCodeEditorSettings.h"

#if PLATFORM_ANDROID
#include "Android/Utils/AndroidUtils.h"
#endif

#define LOCTEXT_NAMESPACE "StashPayModule"

void FMobileNativeCodeModule::StartupModule()
{
	// Register settings in Project Settings -> Plugins -> StashPay
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		UMobileNativeCodeEditorSettings::RegisterEditorSettings(SettingsModule);
	}

	// Initialize platform-specific components
	Initialization();
}

void FMobileNativeCodeModule::ShutdownModule()
{
	// Cleanup if needed
}

void FMobileNativeCodeModule::Initialization()
{
#if PLATFORM_ANDROID
	AndroidUtils::Initialization();
#endif

#if PLATFORM_IOS
	// iOS initialization handled by StashPayCardWrapper
#endif
}

bool FMobileNativeCodeModule::IsSupported()
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

IMPLEMENT_MODULE(FMobileNativeCodeModule, MobileNativeCode)
