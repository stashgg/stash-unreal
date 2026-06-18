// Copyright Stash. All Rights Reserved.
// Stash Unreal Engine SDK - Module Implementation

#include "Stash.h"

#if PLATFORM_ANDROID
#include "Android/Utils/AndroidUtils.h"
#endif

// Define the log category declared in Stash.h
DEFINE_LOG_CATEGORY(LogStash);

#define LOCTEXT_NAMESPACE "StashModule"

void FStashModule::StartupModule()
{
	Initialize();
}

void FStashModule::ShutdownModule()
{
	// No cleanup needed - native SDKs handle their own lifecycle
	// iOS: StashNativeCardWrapper is a singleton that persists
	// Android: StashNativeCard is a singleton managed by the SDK
}

void FStashModule::Initialize()
{
#if PLATFORM_ANDROID
	AndroidUtils::Initialize();
#endif

#if PLATFORM_IOS
	// iOS initialization handled by StashNativeCardWrapper
#endif
}

bool FStashModule::IsSupported()
{
#if PLATFORM_ANDROID
	return AndroidUtils::IsPlatformSupported();
#elif PLATFORM_IOS
	return true;
#else
	return false;
#endif
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FStashModule, Stash)
