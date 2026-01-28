// Copyright Stash. All Rights Reserved.
// Stash Pay Unreal Engine SDK - Objective-C Type Conversion Utilities

#pragma once

#include <iostream>
#include <string>

using namespace std;

// Type inference macros for cleaner Objective-C++ code
#define let __auto_type const
#define var __auto_type

/**
 * ObjCconvert - Type Conversion Utilities
 * 
 * Provides static methods for converting between Unreal Engine types
 * and Objective-C types (NSString, NSMutableArray, etc.).
 */
class ObjCconvert
{
public:
	// NSString to FString
	static FString ToFString(NSString* String);

	// NSString to std::string
	static string ToString(NSString* String);

	// NSMutableDictionary* to TArray<FString>
	static TArray<FString> NSMutableArrayToTArrayFString(NSMutableArray* mArray);

	// NSMutableDictionary* to TArray<int>
	static TArray<int> NSMutableArrayToTArrayInt(NSMutableArray* mArray);

	// NSMutableDictionary* to TArray<float>
	static TArray<float> NSMutableArrayToTArrayFloat(NSMutableArray* mArray);

	// TArray<FString> to NSMutableDictionary*
	static NSMutableArray* TArrayFStringToNSMutableArray(TArray<FString> mArray);

	// TArray<anyTypeNumber> to NSMutableDictionary*
	template<typename anyType>
	static NSMutableArray* TArrayNumToNSMutableArray(TArray<anyType> mArray);
};
