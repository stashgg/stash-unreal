// Copyright Stash. All Rights Reserved.
// Stash Pay Unreal Engine SDK - Objective-C Type Conversion Utilities

#pragma once

#include <string>

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
	static std::string ToString(NSString* String);

	// NSMutableArray* to TArray<FString>
	static TArray<FString> NSMutableArrayToTArrayFString(NSMutableArray* mArray);

	// NSMutableArray* to TArray<int>
	static TArray<int> NSMutableArrayToTArrayInt(NSMutableArray* mArray);

	// NSMutableArray* to TArray<float>
	static TArray<float> NSMutableArrayToTArrayFloat(NSMutableArray* mArray);

	// TArray<FString> to NSMutableArray*
	static NSMutableArray* TArrayFStringToNSMutableArray(TArray<FString> mArray);

	// TArray<anyTypeNumber> to NSMutableArray*
	template<typename anyType>
	static NSMutableArray* TArrayNumToNSMutableArray(TArray<anyType> mArray);
};
