// Copyright Stash. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "StashEditorSettings.h"

/**
 * Display-only soft-keyboard mock for the editor preview (iOS / Android styling,
 * QWERTY or numeric layout). Purely visual: real text entry still goes through the
 * physical keyboard into the CEF webview.
 */
class SStashPreviewKeyboard : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SStashPreviewKeyboard) {}
		SLATE_ATTRIBUTE(EStashPreviewPlatform, Platform)
		/** Input type reported by the focused field ("numeric" shows the number pad). */
		SLATE_ATTRIBUTE(FString, InputType)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual int32 OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

private:
	TAttribute<EStashPreviewPlatform> Platform;
	TAttribute<FString> InputType;
};
