// Copyright Stash. All Rights Reserved.
// Stash Unreal Engine SDK - Java Type Conversion Utilities

#pragma once

#include <Android/AndroidJNI.h>
#include <Android/AndroidApplication.h>
#include <Android/AndroidJava.h>

#include <string>

/**
 * JavaConvert - Type Conversion Utilities
 *
 * Provides static methods for converting between Unreal Engine types
 * and Java JNI types (arrays, strings, primitives).
 */
class JavaConvert
{
public:

	// TArray<unsigned char> to jbyteArray
	static jbyteArray ConvertToJByteArray(const TArray<uint8>& byteArray);

	// FString to jstring
	static jstring GetJavaString(const FString& string);

	// std::string to jstring
	static jstring GetJavaString(const std::string& str);

	// const char* to jstring
	static jstring GetJavaString(const char* str);
};
