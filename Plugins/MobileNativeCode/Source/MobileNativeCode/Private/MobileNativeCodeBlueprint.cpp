// Copyright Epic Games, Inc. All Rights Reserved.

#include "MobileNativeCodeBlueprint.h"
#include "Engine/Engine.h"

#if PLATFORM_ANDROID
#include "Android/AndroidApplication.h"
#include "Android/AndroidJNI.h"
#include "Android/AndroidJava.h"
#endif

#if PLATFORM_IOS
#include "IOS/IOSApplication.h"
#include "IOS/ObjC/WebViewHelper.h"
#include "IOS/ObjC/StashPayHelper.h"
#endif

// Initialize static delegate
FOnPaymentSuccess UMobileNativeCodeBlueprint::OnPaymentSuccess;

void UMobileNativeCodeBlueprint::OpenAndroidWebView(const FString& URL)
{
#if PLATFORM_ANDROID
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		static jmethodID Method = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_OpenWebView", "(Ljava/lang/String;)V", false);
		if (Method)
		{
			jstring JavaURL = Env->NewStringUTF(TCHAR_TO_UTF8(*URL));
			FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, Method, JavaURL);
			Env->DeleteLocalRef(JavaURL);
		}
	}
#else
	UE_LOG(LogTemp, Warning, TEXT("OpenAndroidWebView called on non-Android platform"));
#endif
}

void UMobileNativeCodeBlueprint::CloseAndroidWebView()
{
#if PLATFORM_ANDROID
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		static jmethodID Method = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_CloseWebView", "()V", false);
		if (Method)
		{
			FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, Method);
		}
	}
#else
	UE_LOG(LogTemp, Warning, TEXT("CloseAndroidWebView called on non-Android platform"));
#endif
}

bool UMobileNativeCodeBlueprint::IsAndroidWebViewOpen()
{
#if PLATFORM_ANDROID
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		static jmethodID Method = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_IsWebViewOpen", "()Z", false);
		if (Method)
		{
			return FJavaWrapper::CallBooleanMethod(Env, FJavaWrapper::GameActivityThis, Method);
		}
	}
	return false;
#else
	return false;
#endif
}

void UMobileNativeCodeBlueprint::OpenIOSWebView(const FString& URL)
{
#if PLATFORM_IOS
	// iOS implementation will be in Objective-C
	extern void OpenIOSWebView_Impl(const FString& URL);
	OpenIOSWebView_Impl(URL);
#else
	UE_LOG(LogTemp, Warning, TEXT("OpenIOSWebView called on non-iOS platform"));
#endif
}

void UMobileNativeCodeBlueprint::CloseIOSWebView()
{
#if PLATFORM_IOS
	extern void CloseIOSWebView_Impl();
	CloseIOSWebView_Impl();
#else
	UE_LOG(LogTemp, Warning, TEXT("CloseIOSWebView called on non-iOS platform"));
#endif
}

bool UMobileNativeCodeBlueprint::IsIOSWebViewOpen()
{
#if PLATFORM_IOS
	extern bool IsIOSWebViewOpen_Impl();
	return IsIOSWebViewOpen_Impl();
#else
	return false;
#endif
}

void UMobileNativeCodeBlueprint::OpenStashPayCheckout(const FString& URL)
{
#if PLATFORM_ANDROID
	UE_LOG(LogTemp, Warning, TEXT("OpenStashPayCheckout: Attempting to call Java method with URL: %s"), *URL);
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		static jmethodID Method = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_OpenStashPayCheckout", "(Ljava/lang/String;)V", false);
		if (Method)
		{
			UE_LOG(LogTemp, Warning, TEXT("OpenStashPayCheckout: Found Java method, calling it..."));
			jstring JavaURL = Env->NewStringUTF(TCHAR_TO_UTF8(*URL));
			FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, Method, JavaURL);
			Env->DeleteLocalRef(JavaURL);
			UE_LOG(LogTemp, Warning, TEXT("OpenStashPayCheckout: Java method called successfully"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("OpenStashPayCheckout: Failed to find Java method AndroidThunkJava_OpenStashPayCheckout"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("OpenStashPayCheckout: Failed to get JNI environment"));
	}
#else
	UE_LOG(LogTemp, Warning, TEXT("OpenStashPayCheckout called on non-Android platform"));
#endif
}

void UMobileNativeCodeBlueprint::DismissStashPayCheckout()
{
#if PLATFORM_ANDROID
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		static jmethodID Method = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_DismissStashPayCheckout", "()V", false);
		if (Method)
		{
			FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, Method);
		}
	}
#else
	UE_LOG(LogTemp, Warning, TEXT("DismissStashPayCheckout called on non-Android platform"));
#endif
}

bool UMobileNativeCodeBlueprint::IsStashPayCheckoutOpen()
{
#if PLATFORM_ANDROID
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		static jmethodID Method = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_IsStashPayCheckoutOpen", "()Z", false);
		if (Method)
		{
			bool Result = FJavaWrapper::CallBooleanMethod(Env, FJavaWrapper::GameActivityThis, Method);
			UE_LOG(LogTemp, Warning, TEXT("IsStashPayCheckoutOpen: Java returned %s"), Result ? TEXT("true") : TEXT("false"));
			return Result;
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("IsStashPayCheckoutOpen: Failed to find Java method"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("IsStashPayCheckoutOpen: Failed to get JNI environment"));
	}
	return false;
#else
	return false;
#endif
}

void UMobileNativeCodeBlueprint::OpenStashPayCheckoutIOS(const FString& URL)
{
#if PLATFORM_IOS
	// iOS implementation will be in Objective-C
	extern void OpenStashPayCheckoutIOS_Impl(const FString& URL);
	OpenStashPayCheckoutIOS_Impl(URL);
#else
	UE_LOG(LogTemp, Warning, TEXT("OpenStashPayCheckoutIOS called on non-iOS platform"));
#endif
}

void UMobileNativeCodeBlueprint::DismissStashPayCheckoutIOS()
{
#if PLATFORM_IOS
	extern void DismissStashPayCheckoutIOS_Impl();
	DismissStashPayCheckoutIOS_Impl();
#else
	UE_LOG(LogTemp, Warning, TEXT("DismissStashPayCheckoutIOS called on non-iOS platform"));
#endif
}

bool UMobileNativeCodeBlueprint::IsStashPayCheckoutOpenIOS()
{
#if PLATFORM_IOS
	extern bool IsStashPayCheckoutOpenIOS_Impl();
	return IsStashPayCheckoutOpenIOS_Impl();
#else
	return false;
#endif
}

void UMobileNativeCodeBlueprint::NotifyPaymentSuccess(const FString& ItemName)
{
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("NotifyPaymentSuccess CALLED!"));
	UE_LOG(LogTemp, Warning, TEXT("ItemName: %s"), *ItemName);
	UE_LOG(LogTemp, Warning, TEXT("Delegate bound count: %d"), OnPaymentSuccess.GetAllObjects().Num());
	UE_LOG(LogTemp, Warning, TEXT("Broadcasting delegate..."));
	
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Magenta, TEXT("NotifyPaymentSuccess called - broadcasting delegate!"));
	}
	
	OnPaymentSuccess.Broadcast(ItemName);
	
	UE_LOG(LogTemp, Warning, TEXT("Delegate broadcast complete"));
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
}

// JNI function called from Java GameActivity
#if PLATFORM_ANDROID
extern "C" JNIEXPORT void JNICALL Java_com_epicgames_unreal_GameActivity_nativeNotifyPaymentSuccess(JNIEnv* Env, jobject Thiz, jstring ItemName)
{
	if (ItemName)
	{
		const char* UTFString = Env->GetStringUTFChars(ItemName, nullptr);
		if (UTFString)
		{
			FString ItemNameStr = UTFString;
			Env->ReleaseStringUTFChars(ItemName, UTFString);
			
			// Call on game thread
			AsyncTask(ENamedThreads::GameThread, [ItemNameStr]()
			{
				UMobileNativeCodeBlueprint::NotifyPaymentSuccess(ItemNameStr);
			});
		}
	}
}
#endif

// C function called from iOS Objective-C
#if PLATFORM_IOS
extern "C" void NotifyPaymentSuccessFromIOS(const char* ItemName)
{
	if (ItemName)
	{
		FString ItemNameStr = FString(UTF8_TO_TCHAR(ItemName));
		
		// Call on game thread
		AsyncTask(ENamedThreads::GameThread, [ItemNameStr]()
		{
			UMobileNativeCodeBlueprint::NotifyPaymentSuccess(ItemNameStr);
		});
	}
}
#endif
