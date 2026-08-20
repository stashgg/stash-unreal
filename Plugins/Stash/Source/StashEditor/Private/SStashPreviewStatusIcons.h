// Copyright Stash. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

/**
 * Display-only status-bar indicator glyphs (cellular signal, wifi, battery),
 * painted to match the phone status bar. Color adapts to the content beneath.
 */
class SStashPreviewStatusIcons : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SStashPreviewStatusIcons) {}
		/** Glyph tint (dark on light content, light on dark content). */
		SLATE_ATTRIBUTE(FLinearColor, GlyphColor)
		/** Show the cellular signal glyph (phones with a modem; hidden on wifi-only tablets). */
		SLATE_ATTRIBUTE(bool, bShowCellular)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual FVector2D ComputeDesiredSize(float) const override;

	virtual int32 OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

private:
	TAttribute<FLinearColor> GlyphColor;
	TAttribute<bool> bShowCellular;
};
