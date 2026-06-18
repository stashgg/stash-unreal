// Copyright Stash. All Rights Reserved.
// Android JNI → C++ callback bridge (called from StashHelper.java).

#include "StashBlueprint.h"

#if PLATFORM_ANDROID
#include <jni.h>

extern "C" {
	JNIEXPORT void JNICALL Java_com_Plugins_Stash_StashHelper_nativeOnPaymentSuccess(JNIEnv* env, jclass clazz)
	{
		UStashBlueprint::HandlePaymentSuccess();
	}

	JNIEXPORT void JNICALL Java_com_Plugins_Stash_StashHelper_nativeOnPaymentFailure(JNIEnv* env, jclass clazz)
	{
		UStashBlueprint::HandlePaymentFailure();
	}

	JNIEXPORT void JNICALL Java_com_Plugins_Stash_StashHelper_nativeOnDialogDismissed(JNIEnv* env, jclass clazz)
	{
		UStashBlueprint::HandleDialogDismissed();
	}

	JNIEXPORT void JNICALL Java_com_Plugins_Stash_StashHelper_nativeOnOptInResponse(JNIEnv* env, jclass clazz, jstring optinType)
	{
		FString OptInTypeStr;
		if (optinType)
		{
			const char* UTFString = env->GetStringUTFChars(optinType, nullptr);
			if (UTFString)
			{
				OptInTypeStr = FString(UTF8_TO_TCHAR(UTFString));
				env->ReleaseStringUTFChars(optinType, UTFString);
			}
		}
		UStashBlueprint::HandleOptInResponse(OptInTypeStr);
	}

	JNIEXPORT void JNICALL Java_com_Plugins_Stash_StashHelper_nativeOnPageLoaded(JNIEnv* env, jclass clazz, jlong loadTimeMs)
	{
		UStashBlueprint::HandlePageLoaded(static_cast<float>(loadTimeMs));
	}

	JNIEXPORT void JNICALL Java_com_Plugins_Stash_StashHelper_nativeOnNetworkError(JNIEnv* env, jclass clazz)
	{
		UStashBlueprint::HandleNetworkError();
	}

	JNIEXPORT void JNICALL Java_com_Plugins_Stash_StashHelper_nativeOnExternalPayment(JNIEnv* env, jclass clazz, jstring url)
	{
		FString UrlStr;
		if (url)
		{
			const char* UTFString = env->GetStringUTFChars(url, nullptr);
			if (UTFString)
			{
				UrlStr = FString(UTF8_TO_TCHAR(UTFString));
				env->ReleaseStringUTFChars(url, UTFString);
			}
		}
		UStashBlueprint::HandleExternalPayment(UrlStr);
	}
}
#endif
