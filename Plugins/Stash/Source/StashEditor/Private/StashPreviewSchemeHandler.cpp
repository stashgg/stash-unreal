// Copyright Stash. All Rights Reserved.

#include "StashPreviewSchemeHandler.h"

#if STASH_HAS_WEBBROWSER
#include "StashEditorPreviewService.h"
#include "StashEditorLog.h"
#include "WebBrowserModule.h"
#include "IWebBrowserSingleton.h"
#include "Async/Async.h"
#endif

namespace
{
#if STASH_HAS_WEBBROWSER
	class FStashPreviewSchemeHandler : public IWebBrowserSchemeHandler
	{
	public:
		explicit FStashPreviewSchemeHandler(FString InUrl)
			: RequestUrl(MoveTemp(InUrl))
		{
		}

		virtual bool ProcessRequest(const FString& Verb, const FString& Url, const FSimpleDelegate& OnHeadersReady) override
		{
			const FString CallbackUrl = Url.IsEmpty() ? RequestUrl : Url;
			AsyncTask(ENamedThreads::GameThread, [CallbackUrl]()
			{
				FStashEditorPreviewService::Get()->DispatchPreviewCallbackUrl(CallbackUrl);
			});
			OnHeadersReady.ExecuteIfBound();
			return true;
		}

		virtual void GetResponseHeaders(IHeaders& OutHeaders) override
		{
			OutHeaders.SetStatusCode(200);
			OutHeaders.SetMimeType(TEXT("text/html"));
			OutHeaders.SetContentLength(0);
		}

		virtual bool ReadResponse(uint8* OutBytes, int32 BytesToRead, int32& BytesRead, const FSimpleDelegate& OnMoreDataReady) override
		{
			BytesRead = 0;
			return false;
		}

		virtual void Cancel() override
		{
		}

	private:
		FString RequestUrl;
	};

	FStashPreviewSchemeHandlerFactory GStashPreviewSchemeHandlerFactory;
	bool bSchemeHandlerRegistered = false;
#endif
}

TUniquePtr<IWebBrowserSchemeHandler> FStashPreviewSchemeHandlerFactory::Create(FString Verb, FString Url)
{
#if STASH_HAS_WEBBROWSER
	return MakeUnique<FStashPreviewSchemeHandler>(MoveTemp(Url));
#else
	return nullptr;
#endif
}

void RegisterStashPreviewSchemeHandler()
{
#if STASH_HAS_WEBBROWSER
	if (bSchemeHandlerRegistered)
	{
		return;
	}
	if (!FModuleManager::Get().IsModuleLoaded(TEXT("WebBrowser")))
	{
		FModuleManager::Get().LoadModule(TEXT("WebBrowser"));
	}
	if (IWebBrowserModule::IsAvailable())
	{
		if (IWebBrowserSingleton* Singleton = IWebBrowserModule::Get().GetSingleton())
		{
			if (Singleton->RegisterSchemeHandlerFactory(TEXT("stash-unreal-preview"), TEXT(""), &GStashPreviewSchemeHandlerFactory))
			{
				bSchemeHandlerRegistered = true;
				UE_LOG(LogStashEditor, Log, TEXT("[StashPreview] Registered stash-unreal-preview scheme handler"));
			}
		}
	}
#endif
}

void EnsureStashPreviewSchemeHandlerRegistered()
{
#if STASH_HAS_WEBBROWSER
	RegisterStashPreviewSchemeHandler();
#endif
}

void UnregisterStashPreviewSchemeHandler()
{
#if STASH_HAS_WEBBROWSER
	if (!bSchemeHandlerRegistered)
	{
		return;
	}
	if (IWebBrowserModule::IsAvailable())
	{
		if (IWebBrowserSingleton* Singleton = IWebBrowserModule::Get().GetSingleton())
		{
			Singleton->UnregisterSchemeHandlerFactory(&GStashPreviewSchemeHandlerFactory);
		}
	}
	bSchemeHandlerRegistered = false;
#endif
}
