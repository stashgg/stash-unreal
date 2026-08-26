// Copyright Stash. All Rights Reserved.
// Stash Unreal Engine SDK - Module Implementation

#include "Stash.h"

#if PLATFORM_ANDROID
#include "Android/Utils/AndroidUtils.h"
#endif

#if PLATFORM_WINDOWS || PLATFORM_MAC
#include "Desktop/StashDesktopNative.h"
#endif

// Define the log category declared in Stash.h
DEFINE_LOG_CATEGORY(LogStash);

void FStashModule::StartupModule()
{
	Initialize();
}

void FStashModule::ShutdownModule()
{
	// iOS: StashNativeCardWrapper is a singleton that persists
	// Android: StashNativeCard is a singleton managed by the SDK
#if PLATFORM_WINDOWS || PLATFORM_MAC
	// Desktop: release the webview environment and clear the callback (also runs on Live Coding /
	// hot reload; the library handle itself is never freed).
	FStashDesktopNative::Shutdown();
#endif
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
#elif PLATFORM_WINDOWS || PLATFORM_MAC
	return FStashDesktopNative::IsAvailable();
#else
	return false;
#endif
}

IMPLEMENT_MODULE(FStashModule, Stash)
