// Copyright Stash. All Rights Reserved.
// Stash Unreal Engine SDK - Java Type Conversion Implementation

#include "JavaConvert.h"

// TArray<unsigned char> to jbyteArray
jbyteArray JavaConvert::ConvertToJByteArray(const TArray<uint8>& byteArray)
{
  JNIEnv* Env = FAndroidApplication::GetJavaEnv();
  jbyteArray javaByteArray = Env->NewByteArray(byteArray.Num());
  Env->SetByteArrayRegion(javaByteArray, 0, byteArray.Num(), reinterpret_cast<const jbyte*>(byteArray.GetData()));
  return javaByteArray;
}

// FString to jstring
jstring JavaConvert::GetJavaString(const FString& string)
{
  return GetJavaString(TCHAR_TO_UTF8(*string));
}

// std::string to jstring
jstring JavaConvert::GetJavaString(const std::string& str)
{
  return GetJavaString(str.c_str());
}

// const char* to jstring
jstring JavaConvert::GetJavaString(const char* str)
{
  JNIEnv* JEnv = AndroidJavaEnv::GetJavaEnv();
  return JEnv->NewStringUTF(str);
}
