// Copyright Stash. All Rights Reserved.
// Automation tests for the desktop host bridge: config to JSON, event dispatch, host availability.
// Run: UnrealEditor StashUnreal5.uproject -ExecCmds="Automation RunTests Stash.Desktop; Quit" -unattended -nullrhi

#include "Misc/AutomationTest.h"

#if PLATFORM_WINDOWS || PLATFORM_MAC

#include "StashBlueprint.h"
#include "Desktop/StashDesktopNative.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStashDesktopCardConfigJsonTest, "Stash.Desktop.CardConfigJson",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

bool FStashDesktopCardConfigJsonTest::RunTest(const FString& Parameters)
{
	FStashCardConfig Config;
	Config.bForcePortrait = true;
	Config.BackgroundColor = TEXT("#1e1e1e");
	const FString Json = FStashDesktopNative::CardConfigToJson(Config, false);
	TestTrue(TEXT("forcePortrait"), Json.Contains(TEXT("\"forcePortrait\":true")));
	TestTrue(TEXT("cardHeightRatioPortrait default"), Json.Contains(TEXT("\"cardHeightRatioPortrait\":0.68")));
	TestTrue(TEXT("tabletWidthRatioLandscape default"), Json.Contains(TEXT("\"tabletWidthRatioLandscape\":0.8")));
	TestTrue(TEXT("backgroundColor"), Json.Contains(TEXT("\"backgroundColor\":\"#1e1e1e\"")));
	TestTrue(TEXT("attached presentation"), Json.Contains(TEXT("\"presentation\":\"attached\"")));
	TestFalse(TEXT("no autoClose key (host default applies)"), Json.Contains(TEXT("autoClose")));
	TestTrue(TEXT("object"), Json.StartsWith(TEXT("{")) && Json.EndsWith(TEXT("}")));

	const FString WindowJson = FStashDesktopNative::CardConfigToJson(FStashCardConfig(), true);
	TestTrue(TEXT("window presentation"), WindowJson.Contains(TEXT("\"presentation\":\"window\"")));
	TestTrue(TEXT("empty background"), WindowJson.Contains(TEXT("\"backgroundColor\":\"\"")));

	FStashCardConfig Quoted;
	Quoted.BackgroundColor = TEXT("a\"b\\c");
	TestTrue(TEXT("escaped background"), FStashDesktopNative::CardConfigToJson(Quoted, false).Contains(TEXT("\"backgroundColor\":\"a\\\"b\\\\c\"")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStashDesktopModalConfigJsonTest, "Stash.Desktop.ModalConfigJson",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

bool FStashDesktopModalConfigJsonTest::RunTest(const FString& Parameters)
{
	FStashModalConfig Config;
	Config.bAllowDismiss = false;
	const FString Json = FStashDesktopNative::ModalConfigToJson(Config, false);
	TestTrue(TEXT("allowDismiss"), Json.Contains(TEXT("\"allowDismiss\":false")));
	TestTrue(TEXT("phoneWidthRatioPortrait default"), Json.Contains(TEXT("\"phoneWidthRatioPortrait\":0.9")));
	TestTrue(TEXT("tabletHeightRatioLandscape default"), Json.Contains(TEXT("\"tabletHeightRatioLandscape\":0.8")));
	TestFalse(TEXT("no card keys"), Json.Contains(TEXT("forcePortrait")));
	TestTrue(TEXT("attached presentation"), Json.Contains(TEXT("\"presentation\":\"attached\"")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStashDesktopEventDispatchTest, "Stash.Desktop.EventDispatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

bool FStashDesktopEventDispatchTest::RunTest(const FString& Parameters)
{
	// Every event type the hosts emit is accepted, with or without payload; unknown types only log.
	const TCHAR* Types[] = {
		TEXT("paymentSuccess"), TEXT("paymentFailure"), TEXT("dialogDismissed"), TEXT("optInResponse"),
		TEXT("pageLoaded"), TEXT("networkError"), TEXT("externalPayment"), TEXT("purchaseProcessing"),
		TEXT("processingCompleted"), TEXT("navigation"), TEXT("navigationBlocked"), TEXT("webProcessCrashed"),
		TEXT("error"), TEXT("somethingNew")
	};
	for (const TCHAR* Type : Types)
	{
		FStashDesktopNative::DispatchEvent(Type, TEXT(""));
		FStashDesktopNative::DispatchEvent(Type, TEXT("{\"orderId\":\"1\"}"));
	}
	FStashDesktopNative::DispatchEvent(TEXT("pageLoaded"), TEXT("1234.5"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStashDesktopHostLoadsTest, "Stash.Desktop.HostLoads",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

bool FStashDesktopHostLoadsTest::RunTest(const FString& Parameters)
{
	// The vendored host under ThirdParty loads through the plugin base dir and reports its version.
	TestTrue(TEXT("desktop host available"), FStashDesktopNative::IsAvailable());
	const FString Version = FStashDesktopNative::GetVersion();
	TestTrue(TEXT("version is semver-like"), Version.Contains(TEXT(".")));
	TestFalse(TEXT("nothing presented"), FStashDesktopNative::IsCardOpen());
	TestFalse(TEXT("nothing processing"), FStashDesktopNative::IsPurchaseProcessing());
	// Safe without a presentation.
	FStashDesktopNative::Dismiss();
	FStashDesktopNative::ResetPresentationState();
	return true;
}

#endif
