// Copyright Stash. All Rights Reserved.

#include "Modules/ModuleManager.h"
#include "EdGraphUtilities.h"
#include "StashEditorLog.h"
#include "StashEditorPreviewBridge.h"
#include "StashEditorPreviewService.h"
#include "StashEditorPreviewTab.h"
#include "StashGraphPanelPinFactory.h"
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
		UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateStatic(&FStashEditorPreviewTab::RegisterMenuEntry));
	}

	virtual void ShutdownModule() override
	{
		FStashEditorPreviewBridge::Unregister();
		PreviewService.Reset();

		FStashEditorPreviewTab::UnregisterTabSpawner();

		if (PinFactory.IsValid())
		{
			FEdGraphUtilities::UnregisterVisualPinFactory(PinFactory);
			PinFactory.Reset();
		}
	}

private:
	TSharedPtr<FStashGraphPanelPinFactory> PinFactory;
	TSharedPtr<IStashEditorPreviewBridge> PreviewService;
};

IMPLEMENT_MODULE(FStashEditorModule, StashEditor);
