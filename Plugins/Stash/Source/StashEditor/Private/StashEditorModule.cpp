// Copyright Stash. All Rights Reserved.

#include "Modules/ModuleManager.h"
#include "EdGraphUtilities.h"
#include "StashGraphPanelPinFactory.h"

class FStashEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		PinFactory = MakeShareable(new FStashGraphPanelPinFactory());
		FEdGraphUtilities::RegisterVisualPinFactory(PinFactory);
	}

	virtual void ShutdownModule() override
	{
		if (PinFactory.IsValid())
		{
			FEdGraphUtilities::UnregisterVisualPinFactory(PinFactory);
			PinFactory.Reset();
		}
	}

private:
	TSharedPtr<FStashGraphPanelPinFactory> PinFactory;
};

IMPLEMENT_MODULE(FStashEditorModule, StashEditor);
