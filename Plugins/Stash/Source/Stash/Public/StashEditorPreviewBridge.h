// Copyright Stash. All Rights Reserved.
// Editor preview bridge — runtime stub; StashEditor registers the real implementation.

#pragma once

#include "CoreMinimal.h"
#include "StashBlueprint.h"

/** Card, modal, or full-bleed browser presentation in the editor preview. */
enum class EStashPreviewPresentationMode : uint8
{
	Card,
	Modal,
	Browser
};

/**
 * Editor-only preview implementation registered by StashEditor at module startup.
 * Keeps UnrealEd / WebBrowser out of the Stash runtime module.
 */
class STASH_API IStashEditorPreviewBridge
{
public:
	virtual ~IStashEditorPreviewBridge() = default;

	virtual bool IsAvailable() const = 0;
	virtual bool OpenCard(const FString& URL, const FStashCardConfig& Config) = 0;
	virtual bool OpenModal(const FString& URL, const FStashModalConfig& Config) = 0;
	virtual bool OpenBrowser(const FString& URL) = 0;
	virtual bool CloseBrowser() = 0;
	virtual bool DismissCard() = 0;
	virtual bool IsCardOpen() const = 0;
	virtual bool IsPurchaseProcessing() const = 0;
	virtual void SetLandscapeLockWhenCardClosed(bool bEnable) = 0;
	virtual void SetAndroidKeepAliveEnabled(bool bEnabled) = 0;
	virtual void SetAndroidKeepAliveConfig(const FStashKeepAliveConfig& Config) = 0;
	virtual void SetAndroidCheckoutBackdropBytes(const TArray<uint8>& ImageBytes) = 0;
	virtual void ClearAndroidCheckoutBackdrop() = 0;
};

/** Static forwarder from UStashBlueprint to the registered editor preview bridge. */
class STASH_API FStashEditorPreviewBridge
{
public:
	static void Register(TSharedPtr<IStashEditorPreviewBridge> InBridge);
	static void Unregister();

	static bool TryOpenCard(const FString& URL, const FStashCardConfig& Config = FStashCardConfig());
	static bool TryOpenModal(const FString& URL, const FStashModalConfig& Config = FStashModalConfig());
	static bool TryOpenBrowser(const FString& URL);
	static bool TryCloseBrowser();
	static bool TryDismissCard();
	static bool TryIsCardOpen();
	static bool TryIsPurchaseProcessing();
	static void TrySetLandscapeLockWhenCardClosed(bool bEnable);
	static void TrySetAndroidKeepAliveEnabled(bool bEnabled);
	static void TrySetAndroidKeepAliveConfig(const FStashKeepAliveConfig& Config);
	static void TrySetAndroidCheckoutBackdropBytes(const TArray<uint8>& ImageBytes);
	static void TryClearAndroidCheckoutBackdrop();

private:
	static IStashEditorPreviewBridge* GetBridge();
};
