// Copyright Stash. All Rights Reserved.

#pragma once

#include "EdGraph/EdGraphPin.h"
#include "EdGraphUtilities.h"

/** Blueprint pin widgets with UIMin/UIMax sliders for Stash ratio fields (function + struct Make nodes). */
class FStashGraphPanelPinFactory : public FGraphPanelPinFactory
{
public:
	virtual TSharedPtr<class SGraphPin> CreatePin(UEdGraphPin* InPin) const override;
};
