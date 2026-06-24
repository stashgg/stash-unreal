// Copyright Stash. All Rights Reserved.
// CEF scheme handler for stash-unreal-preview:// (editor preview callback bridge).

#pragma once

#include "CoreMinimal.h"
#include "IWebBrowserSchemeHandler.h"

class FStashPreviewSchemeHandlerFactory : public IWebBrowserSchemeHandlerFactory
{
public:
	virtual TUniquePtr<IWebBrowserSchemeHandler> Create(FString Verb, FString Url) override;
};

void RegisterStashPreviewSchemeHandler();
void UnregisterStashPreviewSchemeHandler();
void EnsureStashPreviewSchemeHandlerRegistered();
