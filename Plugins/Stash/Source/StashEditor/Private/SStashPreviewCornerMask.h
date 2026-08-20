// Copyright Stash. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"

/**
 * Paints the concave corner slivers (bezel-colored) over the device screen so the rectangular CEF
 * webview appears clipped to the phone's rounded corners. Slate/CEF can't rounded-clip the webview
 * texture, so we mask only the thin region between each square corner and the rounded arc — the
 * webview body (and the keyboard) is untouched, exactly as a real screen clips content at its corners.
 */
class SStashPreviewCornerMask : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SStashPreviewCornerMask) {}
		/** Corner radius in Slate units; 0 disables the mask. */
		SLATE_ATTRIBUTE(float, Radius)
		/** Fill color — should match the surrounding bezel. */
		SLATE_ATTRIBUTE(FLinearColor, MaskColor)
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

	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }

private:
	TAttribute<float> Radius;
	TAttribute<FLinearColor> MaskColor;
};
