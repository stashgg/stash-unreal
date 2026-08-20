// Copyright Stash. All Rights Reserved.
// Stash Unreal Engine SDK - Android JNI Utilities
//
// This file provides a template-based system for calling Java methods from C++.
// It handles automatic type conversion, signature generation, and JNI lifecycle management.

#pragma once

#include <Android/AndroidJNI.h>
#include <Android/AndroidApplication.h>
#include <Android/AndroidJava.h>

#include "JavaConvert.h"
#include "StashBlueprint.h"
#include "Stash.h"

#include <string>
#include <vector>

/**
 * AndroidUtils - JNI Bridge Utilities
 *
 * Provides static methods for calling Java code from C++ with automatic
 * type conversion and signature generation.
 */
class AndroidUtils
{
private:
  static bool m_supportedPlatform;
  static bool bLoggedUninitializedWarning;

public:
  static void Initialize()
  {
    // Pre-set to true so the probe call below passes the internal
    // `if (!m_supportedPlatform)` guard in every CallJni* method; the guard
    // otherwise short-circuits before we have confirmed platform support.
    // The real result is assigned from the probe's return value immediately after.
    m_supportedPlatform = true;
    bLoggedUninitializedWarning = false;
    m_supportedPlatform = (bool)CallJavaCode<int>(
      "com/Plugins/Stash/StashInit",
      "initialize",
      "",
      false
    );

    if(!m_supportedPlatform)
    {
      UE_LOG(LogStash, Error, TEXT("[Stash] Android platform initialization failed - JNI communication unavailable"));
    }
    else
    {
      UE_LOG(LogStash, Verbose, TEXT("[Stash] Android platform initialized successfully"));
    }
  }

  /** Check if the Android platform is properly initialized for JNI calls */
  static bool IsPlatformSupported()
  {
    if (!m_supportedPlatform && !bLoggedUninitializedWarning)
    {
      UE_LOG(LogStash, Warning, TEXT("[Stash] Android platform not initialized - JNI calls disabled"));
      bLoggedUninitializedWarning = true;
    }
    return m_supportedPlatform;
  }

  // -- Why do we need this structure?
  // -- https://stackoverflow.com/questions/47373354/c-void-argument-with-variadic-template
  template <typename T>
  struct type { };

  template<typename anyType>
  static const anyType& convertArg(const anyType& value)
  {
    return value;
  }
  static jstring convertArg(const char* str)
  {
    return JavaConvert::GetJavaString(str);
  }
  static jstring convertArg(const std::string& str)
  {
    return JavaConvert::GetJavaString(str);
  }
  static jstring convertArg(const FString& str)
  {
    return JavaConvert::GetJavaString(str);
  }
  //---array
  static jbyteArray convertArg(const TArray<uint8>& byteArray)
  {
    return JavaConvert::ConvertToJByteArray(byteArray);
  }


  ///=============== Override Template ===========================
  static std::string GetTypeName(void)
  {
    return "V";
  }
  static std::string GetTypeName(bool)
  {
    return "Z";
  }
  static std::string GetTypeName(unsigned char)
  {
    return "B";
  }
  static std::string GetTypeName(char)
  {
    return "C";
  }
  static std::string GetTypeName(short)
  {
    return "S";
  }
  static std::string GetTypeName(int)
  {
    return "I";
  }
  static std::string GetTypeName(unsigned int)
  {
    return "I";
  }
  static std::string GetTypeName(long)
  {
    return "J";
  }
  static std::string GetTypeName(float)
  {
    return "F";
  }
  static std::string GetTypeName(double)
  {
    return "D";
  }
  static std::string GetTypeName(const char*)
  {
    return "Ljava/lang/String;";
  }
  static std::string GetTypeName(const std::string&)
  {
    return "Ljava/lang/String;";
  }
  static std::string GetTypeName(const FString&)
  {
    return "Ljava/lang/String;";
  }
  static std::string GetTypeName(jstring)
  {
    return "Ljava/lang/String;";
  }

  //----array
  template<typename anyType>
  static std::string GetTypeName(const TArray<anyType>&)
  {
    anyType SymbolType{};
    return std::string("[" + GetTypeName(SymbolType));
  }
  template<typename anyType>
  static std::string GetTypeName(const std::vector<anyType>&)
  {
    anyType SymbolType{};
    return std::string("[" + GetTypeName(SymbolType));
  }


  ///=============== Recursion Method for Variadic Template===========================
  // ------------ GetType
  template<typename anyType, typename ...Args>
  static void GetType(std::string& signatureString, anyType value, Args ...args)
  {
    signatureString += GetTypeName(value);
    GetType(signatureString, args...);
  }
  // ------------ GetType
  static void GetType(std::string&) {   }


  ///=============== Call Target Jni ========================================
  //========== UserObjectClass ==============
  static void CallJniVoidMethod(const ANSICHAR* ClassName, const ANSICHAR* MethodName, const ANSICHAR* MethodSignature, ...)
  {
    if (!m_supportedPlatform)
      return;

    UE_LOG(LogStash, Verbose, TEXT("Stash -> Method CallJniVoidMethod [%s][%s]"), *FString(MethodName), *FString(MethodSignature));

    JNIEnv* Env = FAndroidApplication::GetJavaEnv();
    // Push a local frame so argument local refs (jstring/jbyteArray) and the
    // jclass created here are reclaimed on return (AND-01).
    Env->PushLocalFrame(32);

    jclass Class = FAndroidApplication::FindJavaClass(ClassName);
    if (!Class)
    {
      UE_LOG(LogStash, Error, TEXT("Stash -> Err: jclass = null  [%s][%s]"), *FString(ClassName), *FString(MethodName));
      Env->PopLocalFrame(nullptr);
      return;
    }
    jmethodID Method = FJavaWrapper::FindStaticMethod(Env, Class, MethodName, MethodSignature, false);
    if (!Method)
    {
      UE_LOG(LogStash, Error, TEXT("Stash -> Err: jmethodID = null  [%s][%s]"), *FString(MethodName), *FString(MethodSignature));
      Env->PopLocalFrame(nullptr);
      return;
    }

    va_list Args;
    va_start(Args, MethodSignature);
    Env->CallStaticVoidMethodV(Class, Method, Args);
    va_end(Args);

    if (Env->ExceptionCheck())
    {
      Env->ExceptionDescribe();
      Env->ExceptionClear();
      Env->PopLocalFrame(nullptr);
      return;
    }

    Env->PopLocalFrame(nullptr);
  }

  static bool CallJniBoolMethod(const ANSICHAR* ClassName, const ANSICHAR* MethodName, const ANSICHAR* MethodSignature, ...)
  {
    if (!m_supportedPlatform)
      return false;

    UE_LOG(LogStash, Verbose, TEXT("Stash -> Method CallJniBoolMethod [%s][%s]"), *FString(MethodName), *FString(MethodSignature));

    JNIEnv* Env = FAndroidApplication::GetJavaEnv();
    // Push a local frame so argument local refs (jstring/jbyteArray) and the
    // jclass created here are reclaimed on return (AND-01).
    Env->PushLocalFrame(32);

    jclass Class = FAndroidApplication::FindJavaClass(ClassName);
    if (!Class)
    {
      UE_LOG(LogStash, Error, TEXT("Stash -> Err: jclass = null  [%s][%s]"), *FString(ClassName), *FString(MethodName));
      Env->PopLocalFrame(nullptr);
      return false;
    }
    jmethodID Method = FJavaWrapper::FindStaticMethod(Env, Class, MethodName, MethodSignature, false);
    if (!Method)
    {
      UE_LOG(LogStash, Error, TEXT("Stash -> Err: jmethodID = null  [%s][%s]"), *FString(MethodName), *FString(MethodSignature));
      Env->PopLocalFrame(nullptr);
      return false;
    }

    va_list Args;
    va_start(Args, MethodSignature);
    bool Result = Env->CallStaticBooleanMethodV(Class, Method, Args);
    va_end(Args);

    if (Env->ExceptionCheck())
    {
      Env->ExceptionDescribe();
      Env->ExceptionClear();
      Env->PopLocalFrame(nullptr);
      return false;
    }

    Env->PopLocalFrame(nullptr);

    return Result;
  }
  static int CallJniIntMethod(const ANSICHAR* ClassName, const ANSICHAR* MethodName, const ANSICHAR* MethodSignature, ...)
  {
    if (!m_supportedPlatform)
      return 0;

    UE_LOG(LogStash, Verbose, TEXT("Stash -> Method CallJniIntMethod [%s][%s]"), *FString(MethodName), *FString(MethodSignature));

    JNIEnv* Env = FAndroidApplication::GetJavaEnv();
    // Push a local frame so argument local refs (jstring/jbyteArray) and the
    // jclass created here are reclaimed on return (AND-01).
    Env->PushLocalFrame(32);

    jclass Class = FAndroidApplication::FindJavaClass(ClassName);
    if (!Class)
    {
      UE_LOG(LogStash, Error, TEXT("Stash -> Err: jclass = null  [%s][%s]"), *FString(ClassName), *FString(MethodName));
      Env->PopLocalFrame(nullptr);
      return 0;
    }
    jmethodID Method = FJavaWrapper::FindStaticMethod(Env, Class, MethodName, MethodSignature, false);
    if (!Method)
    {
      UE_LOG(LogStash, Error, TEXT("Stash -> Err: jmethodID = null  [%s][%s]"), *FString(MethodName), *FString(MethodSignature));
      Env->PopLocalFrame(nullptr);
      return 0;
    }

    va_list Args;
    va_start(Args, MethodSignature);
    int Result = Env->CallStaticIntMethodV(Class, Method, Args);
    va_end(Args);

    if (Env->ExceptionCheck())
    {
      Env->ExceptionDescribe();
      Env->ExceptionClear();
      Env->PopLocalFrame(nullptr);
      return 0;
    }

    Env->PopLocalFrame(nullptr);

    return Result;
  }


  ///=============== Override Callback and Return JNI===========================
  //========== UserObjectClass ==============

  // ------------ void case
  template <typename... Args>
  static void isTypeJNI(type<void>, const char* ClassName, const char* FunctionName, std::string OverrideSignature, bool isActivity, Args ...args)
  {
    std::string MethodSignature;
    if (OverrideSignature.empty()) {
      MethodSignature = "(";
      MethodSignature += isActivity ? "Landroid/app/Activity;" : "";
      GetType(MethodSignature, args...);
      MethodSignature += ")";
      MethodSignature += GetTypeName();
    }
    else
    {
      MethodSignature = OverrideSignature;
    }

    // convertArg() creates the argument local refs (jstring/jbyteArray) in this
    // frame, so the frame that reclaims them must be pushed here around the call
    // (AND-01). CallJniVoidMethod pushes its own inner frame for the jclass.
    JNIEnv* Env = FAndroidApplication::GetJavaEnv();
    Env->PushLocalFrame(32);
    if (isActivity)
      CallJniVoidMethod(ClassName, FunctionName, MethodSignature.c_str(), FJavaWrapper::GameActivityThis, convertArg(args)...);
    else
      CallJniVoidMethod(ClassName, FunctionName, MethodSignature.c_str(), convertArg(args)...);
    Env->PopLocalFrame(nullptr);
  }

  // ------------ non-void case
  template <typename MethodType, typename... Args>
  static MethodType isTypeJNI(type<MethodType>, const char* ClassName, const char* FunctionName, std::string OverrideSignature, bool isActivity, Args ...args)
  {
    MethodType returnType{};

    std::string MethodSignature;
    if (OverrideSignature.empty()) {
      MethodSignature = "(";
      MethodSignature += isActivity ? "Landroid/app/Activity;" : "";
      GetType(MethodSignature, args...);
      MethodSignature += ")";
      MethodSignature += GetTypeName(returnType);
    }
    else
    {
      MethodSignature = OverrideSignature;
    }

    // convertArg() creates the argument local refs (jstring/jbyteArray) in this
    // frame, so the frame that reclaims them must be pushed here around the call
    // (AND-01). The result is a primitive, so it is safe to hold across PopLocalFrame.
    JNIEnv* Env = FAndroidApplication::GetJavaEnv();
    Env->PushLocalFrame(32);
    if (isActivity)
      returnType = CallJNI(returnType, ClassName, FunctionName, MethodSignature.c_str(), FJavaWrapper::GameActivityThis, convertArg(args)...);
    else
      returnType = CallJNI(returnType, ClassName, FunctionName, MethodSignature.c_str(), convertArg(args)...);
    Env->PopLocalFrame(nullptr);

    return returnType;
  }

  // ------------ bool
  template<typename ...Args>
  static bool CallJNI(bool, const char* ClassName, const char* MethodName, const char* MethodSignature, Args ...args)
  {
    return CallJniBoolMethod(ClassName, MethodName, MethodSignature, args...);
  }
  // ------------ int
  template<typename ...Args>
  static int CallJNI(int, const char* ClassName, const char* MethodName, const char* MethodSignature, Args ...args)
  {
    return CallJniIntMethod(ClassName, MethodName, MethodSignature, args...);
  }


  ///============Calling native Android code from C++===============

  /**
  * @param ClassName - package and the name of your Java class.
  * @param FunctionName -  Name of your Java function.
  * @param OverrideSignature -  Set your own signature instead of an automatic one (Send an empty one if you need an automatic one).
  * @param isActivity -  Determines whether to pass Activity UE4 to Java.
  * @param args... - A list of your parameters in the Java function.
  */
  template<typename MethodType, typename ...Args>
  static MethodType CallJavaCode(const char* ClassName, const char* FunctionName, const char* OverrideSignature, bool isActivity, Args ...args)
  {
    return isTypeJNI(type<MethodType>{}, ClassName, FunctionName, OverrideSignature, isActivity, args...);
  }

};
