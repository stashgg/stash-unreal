#pragma once

#include <Kismet/BlueprintFunctionLibrary.h>
#include <Runtime/Launch/Resources/Version.h>
#include <Async/Async.h>
#include <Engine.h>

#include "NativeUI/Enums/ToastLengthMessage.h"

#include "MobileNativeCodeBlueprint.generated.h"



// #~~~~~~~~~~~~~~~~~~~~~~~~~ begin 2 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//-- Dispatcher
DECLARE_DYNAMIC_DELEGATE_OneParam(FTypeDispacth, const FString&, ReturnValue); // DispatchName, ParamType, ParamName  
//~~~~~~~~~~~~~~~~~~~~~~~~~~~ end 2 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// #~~~~~~~~~~~~~~~~~~~~~~~~~ Stash Pay Delegates ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//-- Stash Pay Payment Callbacks
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStashPaymentSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStashPaymentFailure);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStashDialogDismissed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStashPageLoaded, float, LoadTimeMs);
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


UCLASS()
class MOBILENATIVECODE_API UMobileNativeCodeBlueprint : public UBlueprintFunctionLibrary
{
  GENERATED_BODY()
public:
  UMobileNativeCodeBlueprint(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {};

  // #~~~~~~~~~~~~~~~~~~~~~~~~ begin 2 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //-- Dispatcher
  
  static FTypeDispacth StaticValueDispatch;
  static void StaticFunctDispatch(const FString& ReturnValue);

#if PLATFORM_IOS
  static void CallBackCppIOS(NSString* sResult);
#endif //PLATFORM_IOS
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~ end 2 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


  /**
   * Concatenation of the platform name from native code
   */
  UFUNCTION(BlueprintCallable, Category = "MobileNativeCode Category")
  static FString HelloWorld(FString MyStr = "Hello World");

  /**
   * Asynchronous platform name concatenation from native code
   */
  UFUNCTION(BlueprintCallable, Category = "MobileNativeCode Category")
  static void asyncHelloWorld(const FTypeDispacth& CallBackPlatform, FString MyStr = "async Hello World");

  /**
   * Displaying a pop-up message
   */
  UFUNCTION(BlueprintCallable, Category = "MobileNativeCode Category")
  static void ShowToastMobile(FString Message, EToastLengthMessage Length);

  /**
   * Example of passing different types of arrays and returning a String array with two values
   */
  UFUNCTION(BlueprintCallable, Category = "MobileNativeCode Category")
  static void ExampleArray(FString& Arr1, FString& Arr2);

  /**
   * Returns information about the device
   */
  UFUNCTION(BlueprintCallable, Category = "MobileNativeCode Category")
  static FString GetDeviceInfo();

  /**
   * Only for Android. Example of working with Java objects inside C++
   */
  UFUNCTION(BlueprintCallable, Category = "MobileNativeCode Category")
  static void ExampleMyJavaObject(FString& JavaBundle);

  // #~~~~~~~~~~~~~~~~~~~~~~~~~ Stash Pay Functions ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  
  /**
   * Opens the Stash Pay checkout dialog on iOS.
   * @param CheckoutURL The URL to load in the checkout dialog
   */
  UFUNCTION(BlueprintCallable, Category = "StashPay")
  static void OpenStashPayCheckoutIOS(const FString& CheckoutURL);

  /**
   * Opens the Stash Pay checkout dialog on Android.
   * @param CheckoutURL The URL to load in the checkout dialog
   */
  UFUNCTION(BlueprintCallable, Category = "StashPay")
  static void OpenStashPayCheckout(const FString& CheckoutURL);

  /**
   * Checks if the Stash Pay checkout dialog is currently open on iOS.
   * @return true if the checkout dialog is displayed
   */
  UFUNCTION(BlueprintCallable, Category = "StashPay")
  static bool IsStashPayCheckoutOpenIOS();

  /**
   * Checks if the Stash Pay checkout dialog is currently open on Android.
   * @return true if the checkout dialog is displayed
   */
  UFUNCTION(BlueprintCallable, Category = "StashPay")
  static bool IsStashPayCheckoutOpen();

  /**
   * Dismisses the Stash Pay checkout dialog on iOS.
   */
  UFUNCTION(BlueprintCallable, Category = "StashPay")
  static void DismissStashPayCheckoutIOS();

  /**
   * Dismisses the Stash Pay checkout dialog on Android.
   */
  UFUNCTION(BlueprintCallable, Category = "StashPay")
  static void DismissStashPayCheckout();

  // Stash Pay Delegates - Bind to these to receive payment callbacks
  
  /** Called when a payment completes successfully */
  static FOnStashPaymentSuccess OnPaymentSuccess;
  
  /** Called when a payment fails */
  static FOnStashPaymentFailure OnPaymentFailure;
  
  /** Called when the checkout dialog is dismissed by the user */
  static FOnStashDialogDismissed OnDialogDismissed;
  
  /** Called when the checkout page finishes loading */
  static FOnStashPageLoaded OnPageLoaded;
  
  // Internal callback functions called from native code
  static void HandlePaymentSuccess();
  static void HandlePaymentFailure();
  static void HandleDialogDismissed();
  static void HandlePageLoaded(float LoadTimeMs);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
};
