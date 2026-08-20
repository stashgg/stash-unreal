// Copyright Stash. All Rights Reserved.

#include "StashGraphPanelPinFactory.h"

#include "EdGraphSchema_K2.h"
#include "K2Node_CallFunction.h"
#include "K2Node_MakeStruct.h"
#include "KismetPins/SGraphPinNumSlider.h"
#include "StashBlueprint.h"
#include "UObject/UnrealType.h"

namespace
{
	static constexpr float StashRatioPinMinWidth = 120.0f;

	static bool HasRatioSliderMetadata(const FProperty* Property)
	{
		return Property
			&& Property->IsA<FFloatProperty>()
			&& Property->HasMetaData(TEXT("UIMin"))
			&& Property->HasMetaData(TEXT("UIMax"));
	}

	static TSharedPtr<SGraphPin> CreateRatioSliderPin(UEdGraphPin* InPin, const FProperty* Property)
	{
		return SNew(SGraphPinNumSlider<float>, InPin, const_cast<FProperty*>(Property))
			.MinDesiredBoxWidth(StashRatioPinMinWidth)
			.ShouldShowDisabledWhenConnected(true);
	}

	static const FProperty* FindStashCallFunctionPinProperty(UEdGraphPin* InPin)
	{
		const UK2Node_CallFunction* CallNode = Cast<UK2Node_CallFunction>(InPin->GetOuter());
		if (!CallNode)
		{
			return nullptr;
		}

		const UFunction* Function = CallNode->GetTargetFunction();
		if (!Function || Function->GetOuterUClass() != UStashBlueprint::StaticClass())
		{
			return nullptr;
		}

		return Function->FindPropertyByName(InPin->PinName);
	}

	static const FProperty* FindStashMakeStructPinProperty(UEdGraphPin* InPin)
	{
		const UK2Node_MakeStruct* MakeStructNode = Cast<UK2Node_MakeStruct>(InPin->GetOuter());
		if (!MakeStructNode || !MakeStructNode->StructType)
		{
			return nullptr;
		}

		const UScriptStruct* StructType = MakeStructNode->StructType;
		if (StructType != FStashModalConfig::StaticStruct()
			&& StructType != FStashCardConfig::StaticStruct())
		{
			return nullptr;
		}

		return StructType->FindPropertyByName(InPin->PinName);
	}
}

TSharedPtr<SGraphPin> FStashGraphPanelPinFactory::CreatePin(UEdGraphPin* InPin) const
{
	if (!InPin || InPin->Direction != EGPD_Input)
	{
		return nullptr;
	}

	if (InPin->PinType.PinCategory != UEdGraphSchema_K2::PC_Real)
	{
		return nullptr;
	}

	const FProperty* Property = FindStashCallFunctionPinProperty(InPin);
	if (!Property)
	{
		Property = FindStashMakeStructPinProperty(InPin);
	}

	if (!HasRatioSliderMetadata(Property))
	{
		return nullptr;
	}

	return CreateRatioSliderPin(InPin, Property);
}
