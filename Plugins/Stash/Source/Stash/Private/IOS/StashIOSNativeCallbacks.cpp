// Copyright Stash. All Rights Reserved.
// iOS native → C++ callback bridge (called from StashNativeCardWrapper.mm).

#include "StashBlueprint.h"

#if PLATFORM_IOS
extern "C" {
	void StashNativeOnPaymentSuccess()
	{
		UStashBlueprint::HandlePaymentSuccess();
	}

	void StashNativeOnPaymentFailure()
	{
		UStashBlueprint::HandlePaymentFailure();
	}

	void StashNativeOnDialogDismissed()
	{
		UStashBlueprint::HandleDialogDismissed();
	}

	void StashNativeOnOptInResponse(const char* optinType)
	{
		UStashBlueprint::HandleOptInResponse(FString(UTF8_TO_TCHAR(optinType ? optinType : "")));
	}

	void StashNativeOnPageLoaded(double loadTimeMs)
	{
		UStashBlueprint::HandlePageLoaded(static_cast<float>(loadTimeMs));
	}

	void StashNativeOnNetworkError()
	{
		UStashBlueprint::HandleNetworkError();
	}

	void StashNativeOnExternalPayment(const char* url)
	{
		UStashBlueprint::HandleExternalPayment(FString(UTF8_TO_TCHAR(url ? url : "")));
	}

	void StashNativeOnPurchaseProcessing()
	{
		UStashBlueprint::HandlePurchaseProcessing();
	}

	void StashNativeOnProcessingCompleted()
	{
		UStashBlueprint::HandleProcessingCompleted();
	}
}
#endif
