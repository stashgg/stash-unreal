#include "MobileNativeCodeBlueprint.h"

#include "MobileNativeCode.h"
#include <Async/Async.h>
#include <Engine.h>

// All Java classes are on the path: "MobileNativeCode\Source\MobileNativeCode\Private\Android\Java\"
// All Objective-C classes are on the path: "MobileNativeCode\Source\MobileNativeCode\Private\IOS\ObjC\"

#if PLATFORM_ANDROID
#include "Android/Utils/AndroidUtils.h"
#endif

#if PLATFORM_IOS
#include "IOS/Utils/ObjC_Convert.h"

#include "IOS/ObjC/IosHelloWorld.h"
#include "IOS/ObjC/IosAsyncHelloWorld.h"
#include "IOS/ObjC/IosNativeUI.h"
#include "IOS/ObjC/IosExampleArray.h"
#include "IOS/ObjC/IosDeviceInfo.h"
#include "IOS/ObjC/StashPayCardWrapper.h"
#endif



// #~~~~~~~~~~~~~~~~~~~~~~~~~~~ begin 1 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
FString UMobileNativeCodeBlueprint::HelloWorld(FString MyStr /*= "Hello World"*/)
{
#if PLATFORM_ANDROID

  MyStr = AndroidUtils::CallJavaCode<FString>(
    "com/Plugins/MobileNativeCode/HelloWorldClass",     // package (used by com/Plugins/MobileNativeCode) and the name of your Java class.
    "HelloWorldOnAndroid",                              //Name of your Java function.
    "",                                                 //Set your own signature instead of an automatic one (Send an empty one if you need an automatic one).
    false,                                              //Determines whether to pass Activity UE4 to Java.
    MyStr                                               //A list of your parameters in the Java function.
  );

#endif //Android


#if PLATFORM_IOS

  //The Objective-C language can be mixed with C++ in a single file
  MyStr = FString([IosHelloWorld HelloWorldOnIOS : MyStr.GetNSString()]);

#endif// IOS

  return MyStr;
}
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ end 1 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~



// #~~~~~~~~~~~~~~~~~~~~~~~~~~~ begin 2 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//--  Initialization of static variables
FTypeDispacth UMobileNativeCodeBlueprint::StaticValueDispatch;


void UMobileNativeCodeBlueprint::asyncHelloWorld(const FTypeDispacth& CallBackPlatform, FString MyStr /*= "async Hello World"*/)
{
  UMobileNativeCodeBlueprint::StaticValueDispatch = CallBackPlatform;

#if PLATFORM_ANDROID

  AndroidUtils::CallJavaCode<void>(
    "com/Plugins/MobileNativeCode/asyncHelloWorldClass",
    "asyncHelloWorldOnAndroid",
    "",
    true,
    MyStr
  );

#endif //Android


#if PLATFORM_IOS

  [IosAsyncHelloWorld asyncHelloWorldOnIOS : MyStr.GetNSString()];

#endif // IOS
}

void UMobileNativeCodeBlueprint::StaticFunctDispatch(const FString& ReturnValue)
{  
  //Lambda function for the dispatcher
  AsyncTask(ENamedThreads::GameThread, [ReturnValue]() {
      StaticValueDispatch.ExecuteIfBound(ReturnValue);
  });
}

//-- Functions CallBack for Java code
#if PLATFORM_ANDROID
JNI_METHOD void Java_com_Plugins_MobileNativeCode_asyncHelloWorldClass_CallBackCppAndroid(JNIEnv* env, jclass clazz, jstring returnStr)
{
  FString result = JavaConvert::FromJavaFString(returnStr);
  UE_LOG(LogTemp, Warning, TEXT("asyncHelloWorld callback caught in C++! - [%s]"), *FString(result)); //Debug log for UE4
  UMobileNativeCodeBlueprint::StaticFunctDispatch(result);// Call Dispatcher
}
#endif //PLATFORM_ANDROID

//-- Functions CallBack for iOS code
#if PLATFORM_IOS
void UMobileNativeCodeBlueprint::CallBackCppIOS(NSString* sResult)
{
  FString fResult = FString(sResult);
  UE_LOG(LogTemp, Warning, TEXT("asyncHelloWorld callback caught in C++! - [%s]"), *FString(fResult)); //Debug log for UE4
  UMobileNativeCodeBlueprint::StaticFunctDispatch(fResult); // Call Dispatcher
}
#endif //PLATFORM_IOS
//~~~~~~~~~~~~~~~~~~~~~~~~~~~ end 2 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~



// #~~~~~~~~~~~~~~~~~~~~~~~~~~~ begin 3 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void UMobileNativeCodeBlueprint::ShowToastMobile(FString Message, EToastLengthMessage Length)
{
#if PLATFORM_ANDROID

  AndroidUtils::CallJavaCode<void>("com/Plugins/MobileNativeCode/NativeUI", "showToast", "", true, Message, (int)Length);

#endif //Android

#if PLATFORM_IOS

  // Calling a function using the singleton pattern
  [[IosNativeUI Singleton] showToast:Message.GetNSString() Duration:(int)Length];

#endif // IOS
}
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ end 3 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~



// #~~~~~~~~~~~~~~~~~~~~~~~~~~~ begin 4 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void UMobileNativeCodeBlueprint::ExampleArray(FString& Arr1, FString& Arr2)
{
  //Working with an array is done via TArray

  //Support argType = FString, bool, int, long, float, const char*, std::string
  TArray<FString> a1;
  TArray<bool> a2;
  TArray<int> a3;
  TArray<long> a4;
  TArray<float> a5;

  //Support returnType = FString, int, float, long
  TArray<FString> TestStrArr;

#if PLATFORM_ANDROID

  TestStrArr = AndroidUtils::CallJavaCode<TArray<FString>>(
    "com/Plugins/MobileNativeCode/ExampleArrayClass",             // package (used by com/Plugins/MobileNativeCode) and the name of your Java class.
    "TestArray",                                                  // Name of your Java function.
    "",                                                           // Set your own signature instead of an automatic one (Send an empty one if you need an automatic one).
    false,                                                        // Determines whether to pass Activity UE4 to Java.
    a1, a2, a3, a4, a5                                            //A list of your parameters in the Java function.
  );

#endif //Android


#if PLATFORM_IOS

  //The Objective-C language can be mixed with C++ in a single file
  TestStrArr = ObjCconvert::NSMutableArrayToTArrayFString([IosExampleArray    
      TestArray : ObjCconvert::TArrayFStringToNSMutableArray(a1)
                    b : ObjCconvert::TArrayNumToNSMutableArray(a2)
                    i : ObjCconvert::TArrayNumToNSMutableArray(a3)
                    f : ObjCconvert::TArrayNumToNSMutableArray(a4)
                    l : ObjCconvert::TArrayNumToNSMutableArray(a5)
    ]);

#endif // iOS

  Arr1 = TestStrArr[0];
  Arr2 = TestStrArr[1];
}
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ end 4 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~



// #~~~~~~~~~~~~~~~~~~~~~~~~~~~ begin 5 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
FString UMobileNativeCodeBlueprint::GetDeviceInfo()
{
  FString sDeviceInfo;

#if PLATFORM_ANDROID

  sDeviceInfo += " Your phone: ";
  sDeviceInfo += AndroidUtils::CallJavaCode<FString>("com/Plugins/MobileNativeCode/DeviceInfo", "getBrand", "", false);
  sDeviceInfo += " " + AndroidUtils::CallJavaCode<FString>("com/Plugins/MobileNativeCode/DeviceInfo", "getModel", "", false);
    
  sDeviceInfo += " Path to save files: ";
  // "storage/emulated/0/Android/data/data/%PROJECT_NAME%/"
  sDeviceInfo += AndroidUtils::CallJavaCode<FString>("com/Plugins/MobileNativeCode/DeviceInfo", "GetExternalFilesDir", "", true);

#endif //Android


#if PLATFORM_IOS

  sDeviceInfo += " Your phone: ";
  sDeviceInfo += FString([IosDeviceInfo getModel]);
    
  sDeviceInfo += " Path to save files: ";
  // "/var/mobile/Containers/Data/Application/%PROJECT_ID%/Library/Caches/"
  sDeviceInfo += FString([IosDeviceInfo getTmpFilePath]);

#endif //PLATFORM_IOS

  return sDeviceInfo;
}
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ end 5 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


void UMobileNativeCodeBlueprint::ExampleMyJavaObject(FString& JavaBundle)
{
#if PLATFORM_ANDROID

  // Creating an empty jobject object, assigning it the standard java Bundle class
  // Bundle: https://developer.android.com/reference/android/os/Bundle

  //In order to populate a jobject with a Java class, you need to return it from your static Java function.

  jobject myJavaObject = AndroidUtils::CallJavaCode<jobject>(
    "com/Plugins/MobileNativeCode/MyJavaObjects",  // package (used by com/Plugins/MobileNativeCode) and the name of your Java class.
    "getBundleJava",                               // Name of your Java function.
    "()Landroid/os/Bundle;",                       // Set your own signature instead of an automatic one (Send an empty one if you need an automatic one).
    false                                          // Determines whether to pass Activity UE4 to Java.
    );

  // Functions such as putString and putDouble are defined inside Bunde. Let's call them with our parameters:

  AndroidUtils::CallJavaCode<void>(
    myJavaObject,         // the type of jobject from which you want to call a local Java function
    "putFloat",           // Name of Java function.
    "",                   // Set your own signature instead of an automatic one (Send an empty one if you need an automatic one).
    "myKey", 1234.f       // A list of your parameters in the Java function.
    );

  AndroidUtils::CallJavaCode<void>(
    myJavaObject,
    "putString",
    "",
    "myKey2", "myValueForBundle"
    );


  // Let's see what's inside our Bundle
  JavaBundle = AndroidUtils::CallJavaCode<FString>(
    myJavaObject,         // the type of jobject from which you want to call a local Java function
    "toString",           // Name of Java function.
    ""                    // Set your own signature instead of an automatic one (Send an empty one if you need an automatic one).
    );

  // After you have finished working with your jobject, you need to delete it
  AndroidUtils::DeleteJavaObject(myJavaObject);

#endif //Android
}


// #~~~~~~~~~~~~~~~~~~~~~~~~~~~ Stash Pay Implementation ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Initialize static delegates
FOnStashPaymentSuccess UMobileNativeCodeBlueprint::OnPaymentSuccess;
FOnStashPaymentFailure UMobileNativeCodeBlueprint::OnPaymentFailure;
FOnStashDialogDismissed UMobileNativeCodeBlueprint::OnDialogDismissed;
FOnStashPageLoaded UMobileNativeCodeBlueprint::OnPageLoaded;

// iOS Implementation
void UMobileNativeCodeBlueprint::OpenStashPayCheckoutIOS(const FString& CheckoutURL)
{
#if PLATFORM_IOS
  UE_LOG(LogTemp, Log, TEXT("[StashPay] Opening checkout on iOS: %s"), *CheckoutURL);
  [[StashPayCardWrapper sharedInstance] openCheckoutWithURL:CheckoutURL.GetNSString()];
#else
  UE_LOG(LogTemp, Warning, TEXT("[StashPay] OpenStashPayCheckoutIOS called on non-iOS platform"));
#endif
}

bool UMobileNativeCodeBlueprint::IsStashPayCheckoutOpenIOS()
{
#if PLATFORM_IOS
  return [[StashPayCardWrapper sharedInstance] isCheckoutOpen];
#else
  return false;
#endif
}

void UMobileNativeCodeBlueprint::DismissStashPayCheckoutIOS()
{
#if PLATFORM_IOS
  UE_LOG(LogTemp, Log, TEXT("[StashPay] Dismissing checkout on iOS"));
  [[StashPayCardWrapper sharedInstance] dismissCheckout];
#endif
}

// Android Implementation
void UMobileNativeCodeBlueprint::OpenStashPayCheckout(const FString& CheckoutURL)
{
#if PLATFORM_ANDROID
  UE_LOG(LogTemp, Log, TEXT("[StashPay] Opening checkout on Android: %s"), *CheckoutURL);
  AndroidUtils::CallJavaCode<void>(
    "com/Plugins/MobileNativeCode/StashPayHelper",
    "OpenCheckout",
    "",
    true,  // Pass activity
    CheckoutURL
  );
#else
  UE_LOG(LogTemp, Warning, TEXT("[StashPay] OpenStashPayCheckout called on non-Android platform"));
#endif
}

bool UMobileNativeCodeBlueprint::IsStashPayCheckoutOpen()
{
#if PLATFORM_ANDROID
  return AndroidUtils::CallJavaCode<bool>(
    "com/Plugins/MobileNativeCode/StashPayHelper",
    "IsCheckoutOpen",
    "",
    false
  );
#else
  return false;
#endif
}

void UMobileNativeCodeBlueprint::DismissStashPayCheckout()
{
#if PLATFORM_ANDROID
  UE_LOG(LogTemp, Log, TEXT("[StashPay] Dismissing checkout on Android"));
  AndroidUtils::CallJavaCode<void>(
    "com/Plugins/MobileNativeCode/StashPayHelper",
    "DismissCheckout",
    "",
    true  // Pass activity
  );
#endif
}

// Callback handlers - called from native code
void UMobileNativeCodeBlueprint::HandlePaymentSuccess()
{
  UE_LOG(LogTemp, Log, TEXT("[StashPay] Payment success callback received"));
  AsyncTask(ENamedThreads::GameThread, []() {
    OnPaymentSuccess.Broadcast();
  });
}

void UMobileNativeCodeBlueprint::HandlePaymentFailure()
{
  UE_LOG(LogTemp, Log, TEXT("[StashPay] Payment failure callback received"));
  AsyncTask(ENamedThreads::GameThread, []() {
    OnPaymentFailure.Broadcast();
  });
}

void UMobileNativeCodeBlueprint::HandleDialogDismissed()
{
  UE_LOG(LogTemp, Log, TEXT("[StashPay] Dialog dismissed callback received"));
  AsyncTask(ENamedThreads::GameThread, []() {
    OnDialogDismissed.Broadcast();
  });
}

void UMobileNativeCodeBlueprint::HandlePageLoaded(float LoadTimeMs)
{
  UE_LOG(LogTemp, Log, TEXT("[StashPay] Page loaded callback received: %.2f ms"), LoadTimeMs);
  AsyncTask(ENamedThreads::GameThread, [LoadTimeMs]() {
    OnPageLoaded.Broadcast(LoadTimeMs);
  });
}

// iOS Callback bridge functions (called from StashPayCardWrapper.mm)
#if PLATFORM_IOS
extern "C" {
  void StashPayOnPaymentSuccess()
  {
    UMobileNativeCodeBlueprint::HandlePaymentSuccess();
  }
  
  void StashPayOnPaymentFailure()
  {
    UMobileNativeCodeBlueprint::HandlePaymentFailure();
  }
  
  void StashPayOnDialogDismissed()
  {
    UMobileNativeCodeBlueprint::HandleDialogDismissed();
  }
  
  void StashPayOnPageLoaded(double loadTimeMs)
  {
    UMobileNativeCodeBlueprint::HandlePageLoaded((float)loadTimeMs);
  }
}
#endif

// Android JNI Callback functions (called from StashPayHelper.java)
#if PLATFORM_ANDROID
extern "C" {
  JNIEXPORT void JNICALL Java_com_Plugins_MobileNativeCode_StashPayHelper_nativeOnPaymentSuccess(JNIEnv* env, jclass clazz)
  {
    UMobileNativeCodeBlueprint::HandlePaymentSuccess();
  }
  
  JNIEXPORT void JNICALL Java_com_Plugins_MobileNativeCode_StashPayHelper_nativeOnPaymentFailure(JNIEnv* env, jclass clazz)
  {
    UMobileNativeCodeBlueprint::HandlePaymentFailure();
  }
  
  JNIEXPORT void JNICALL Java_com_Plugins_MobileNativeCode_StashPayHelper_nativeOnDialogDismissed(JNIEnv* env, jclass clazz)
  {
    UMobileNativeCodeBlueprint::HandleDialogDismissed();
  }
  
  JNIEXPORT void JNICALL Java_com_Plugins_MobileNativeCode_StashPayHelper_nativeOnPageLoaded(JNIEnv* env, jclass clazz, jlong loadTimeMs)
  {
    UMobileNativeCodeBlueprint::HandlePageLoaded((float)loadTimeMs);
  }
}
#endif
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ end Stash Pay ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
