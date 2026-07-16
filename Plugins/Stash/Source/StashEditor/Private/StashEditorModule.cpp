// Copyright Stash. All Rights Reserved.

#include "Modules/ModuleManager.h"
#include "EdGraphUtilities.h"
#include "Editor.h"
#include "StashEditorLog.h"
#include "StashEditorPreviewBridge.h"
#include "StashEditorPreviewService.h"
#include "StashEditorPreviewTab.h"
#include "StashGraphPanelPinFactory.h"
#include "StashPreviewSchemeHandler.h"
#include "ToolMenus.h"

DEFINE_LOG_CATEGORY(LogStashEditor);

class FStashEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		PinFactory = MakeShareable(new FStashGraphPanelPinFactory());
		FEdGraphUtilities::RegisterVisualPinFactory(PinFactory);

		PreviewService = FStashEditorPreviewService::Get();
		FStashEditorPreviewBridge::Register(PreviewService);

		FStashEditorPreviewTab::RegisterTabSpawner();
		ToolMenusStartupHandle = UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateStatic(&FStashEditorPreviewTab::RegisterMenuEntry));

		// Tear down any live preview session when PIE stops so state (open card, running CEF page,
		// late callbacks) never survives into the next PIE run. Do not fire the dismiss callback —
		// stopping PIE is not a user dismiss.
		BeginPIEHandle = FEditorDelegates::BeginPIE.AddRaw(this, &FStashEditorModule::HandleBeginPIE);
		EndPIEHandle = FEditorDelegates::EndPIE.AddRaw(this, &FStashEditorModule::HandleEndPIE);

		// SVC-02: scheme-handler registration is deferred to the lazy EnsureStashPreviewSchemeHandlerRegistered()
		// path (called before the first browser is created) so we don't force-initialize CEF on every editor launch.
	}

	virtual void ShutdownModule() override
	{
		FEditorDelegates::BeginPIE.Remove(BeginPIEHandle);
		FEditorDelegates::EndPIE.Remove(EndPIEHandle);

		UnregisterStashPreviewSchemeHandler();

		FStashEditorPreviewBridge::Unregister();
		PreviewService.Reset();

		UToolMenus::UnRegisterStartupCallback(ToolMenusStartupHandle);
		if (UObjectInitialized() && UToolMenus::IsToolMenuUIEnabled() && UToolMenus::Get())
		{
			// Owner name must match the FToolMenuOwnerScoped in FStashEditorPreviewTab::RegisterMenuEntry.
			UToolMenus::Get()->UnregisterOwnerByName(FStashEditorPreviewTab::GetMenuOwnerName());
		}

		FStashEditorPreviewTab::UnregisterTabSpawner();

		if (PinFactory.IsValid())
		{
			FEdGraphUtilities::UnregisterVisualPinFactory(PinFactory);
			PinFactory.Reset();
		}
	}

private:
	void HandleBeginPIE(bool /*bIsSimulating*/)
	{
		// Clear transient flags so a crashed/aborted prior session can't poison the next PIE run.
		FStashEditorPreviewService::Get()->ResetTransientSessionState();
	}

	void HandleEndPIE(bool /*bIsSimulating*/)
	{
		FStashEditorPreviewService::Get()->EndSession(false);
	}

	TSharedPtr<FStashGraphPanelPinFactory> PinFactory;
	TSharedPtr<IStashEditorPreviewBridge> PreviewService;
	FDelegateHandle ToolMenusStartupHandle;
	FDelegateHandle BeginPIEHandle;
	FDelegateHandle EndPIEHandle;
};

IMPLEMENT_MODULE(FStashEditorModule, StashEditor);
