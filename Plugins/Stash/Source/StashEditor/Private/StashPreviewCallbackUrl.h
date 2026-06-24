// Copyright Stash. All Rights Reserved.
// Parses stash-unreal-preview:// callback URLs from CEF (host-as-path and path forms).

#pragma once

#include "CoreMinimal.h"
#include "StashPreviewJsBridge.h"

namespace StashPreviewCallbackUrl
{
	inline FString BuildPreviewCallbackUrl(const FString& Path, const FString& Query = FString())
	{
		FString Url = FString(StashPreviewJsBridge::SchemePrefix) + TEXT("/") + Path;
		if (!Query.IsEmpty())
		{
			Url += TEXT("?") + Query;
		}
		return Url;
	}

	inline FString BuildDedupKey(const FString& Path, const FString& Query)
	{
		return Path + TEXT("\x1f") + Query;
	}

	inline bool ParsePreviewCallbackUrl(const FString& Url, FString& OutPath, FString& OutQuery)
	{
		if (!Url.StartsWith(StashPreviewJsBridge::SchemePrefix))
		{
			return false;
		}

		FString Remainder = Url.Mid(FCString::Strlen(StashPreviewJsBridge::SchemePrefix));
		Remainder.TrimStartAndEndInline();
		if (Remainder.IsEmpty())
		{
			return false;
		}

		FString PathPart;
		FString QueryPart;
		if (!Remainder.Split(TEXT("?"), &PathPart, &QueryPart))
		{
			PathPart = Remainder;
			QueryPart.Reset();
		}

		while (PathPart.StartsWith(TEXT("/")))
		{
			PathPart.RemoveAt(0);
		}
		while (PathPart.EndsWith(TEXT("/")))
		{
			PathPart.LeftChopInline(1);
		}

		if (PathPart.IsEmpty())
		{
			return false;
		}

		OutPath = PathPart;
		OutQuery = QueryPart;
		return true;
	}
}
