// Copyright Stash. All Rights Reserved.

#include "StashEditorPreviewBridge.h"
#include "Stash.h"

#if WITH_EDITOR
namespace
{
	TWeakPtr<IStashEditorPreviewBridge> GStashEditorPreviewBridge;
}
#endif

void FStashEditorPreviewBridge::Register(TSharedPtr<IStashEditorPreviewBridge> InBridge)
{
#if WITH_EDITOR
	GStashEditorPreviewBridge = InBridge;
#endif
}

void FStashEditorPreviewBridge::Unregister()
{
#if WITH_EDITOR
	GStashEditorPreviewBridge.Reset();
#endif
}

// Contract for the whole forwarder: all Try* calls must run on the game thread — the bridge drives
// Slate/WebBrowser widgets. IsAvailable() means "a new preview session can be opened right now"
// (it gates the three Open* entry points); state queries, setters, Close/Dismiss are always allowed.
// GetBridge() returns a pinned TSharedPtr so the implementation stays alive for the duration of the call.
TSharedPtr<IStashEditorPreviewBridge> FStashEditorPreviewBridge::GetBridge()
{
#if WITH_EDITOR
	return GStashEditorPreviewBridge.Pin();
#else
	return nullptr;
#endif
}

bool FStashEditorPreviewBridge::TryOpenCard(const FString& URL, const FStashCardConfig& Config)
{
#if WITH_EDITOR
	if (TSharedPtr<IStashEditorPreviewBridge> Bridge = GetBridge())
	{
		if (Bridge->IsAvailable())
		{
			return Bridge->OpenCard(URL, Config);
		}
	}
#endif
	return false;
}

bool FStashEditorPreviewBridge::TryOpenModal(const FString& URL, const FStashModalConfig& Config)
{
#if WITH_EDITOR
	if (TSharedPtr<IStashEditorPreviewBridge> Bridge = GetBridge())
	{
		if (Bridge->IsAvailable())
		{
			return Bridge->OpenModal(URL, Config);
		}
	}
#endif
	return false;
}

bool FStashEditorPreviewBridge::TryOpenBrowser(const FString& URL)
{
#if WITH_EDITOR
	if (TSharedPtr<IStashEditorPreviewBridge> Bridge = GetBridge())
	{
		if (Bridge->IsAvailable())
		{
			return Bridge->OpenBrowser(URL);
		}
	}
#endif
	return false;
}

bool FStashEditorPreviewBridge::TryCloseBrowser()
{
#if WITH_EDITOR
	if (TSharedPtr<IStashEditorPreviewBridge> Bridge = GetBridge())
	{
		return Bridge->CloseBrowser();
	}
#endif
	return false;
}

bool FStashEditorPreviewBridge::TryDismissCard()
{
#if WITH_EDITOR
	if (TSharedPtr<IStashEditorPreviewBridge> Bridge = GetBridge())
	{
		return Bridge->DismissCard();
	}
#endif
	return false;
}

bool FStashEditorPreviewBridge::TryIsCardOpen()
{
#if WITH_EDITOR
	if (TSharedPtr<IStashEditorPreviewBridge> Bridge = GetBridge())
	{
		return Bridge->IsCardOpen();
	}
#endif
	return false;
}

bool FStashEditorPreviewBridge::TryIsPurchaseProcessing()
{
#if WITH_EDITOR
	if (TSharedPtr<IStashEditorPreviewBridge> Bridge = GetBridge())
	{
		return Bridge->IsPurchaseProcessing();
	}
#endif
	return false;
}

void FStashEditorPreviewBridge::TrySetLandscapeLockWhenCardClosed(bool bEnable)
{
#if WITH_EDITOR
	if (TSharedPtr<IStashEditorPreviewBridge> Bridge = GetBridge())
	{
		Bridge->SetLandscapeLockWhenCardClosed(bEnable);
	}
#endif
}

void FStashEditorPreviewBridge::TrySetAndroidKeepAliveEnabled(bool bEnabled)
{
#if WITH_EDITOR
	if (TSharedPtr<IStashEditorPreviewBridge> Bridge = GetBridge())
	{
		Bridge->SetAndroidKeepAliveEnabled(bEnabled);
	}
#endif
}

void FStashEditorPreviewBridge::TrySetAndroidKeepAliveConfig(const FStashKeepAliveConfig& Config)
{
#if WITH_EDITOR
	if (TSharedPtr<IStashEditorPreviewBridge> Bridge = GetBridge())
	{
		Bridge->SetAndroidKeepAliveConfig(Config);
	}
#endif
}

void FStashEditorPreviewBridge::TrySetAndroidCheckoutBackdropBytes(const TArray<uint8>& ImageBytes)
{
#if WITH_EDITOR
	if (TSharedPtr<IStashEditorPreviewBridge> Bridge = GetBridge())
	{
		Bridge->SetAndroidCheckoutBackdropBytes(ImageBytes);
	}
#endif
}

void FStashEditorPreviewBridge::TryClearAndroidCheckoutBackdrop()
{
#if WITH_EDITOR
	if (TSharedPtr<IStashEditorPreviewBridge> Bridge = GetBridge())
	{
		Bridge->ClearAndroidCheckoutBackdrop();
	}
#endif
}
