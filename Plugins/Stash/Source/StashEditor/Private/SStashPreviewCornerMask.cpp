// Copyright Stash. All Rights Reserved.

#include "SStashPreviewCornerMask.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

void SStashPreviewCornerMask::Construct(const FArguments& InArgs)
{
	Radius = InArgs._Radius;
	MaskColor = InArgs._MaskColor;
	SetVisibility(EVisibility::HitTestInvisible);
}

int32 SStashPreviewCornerMask::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	const float R = Radius.Get(0.f);
	const FVector2f Size = FVector2f(AllottedGeometry.GetLocalSize());
	if (R < 2.f || Size.X < 2.f || Size.Y < 2.f)
	{
		return LayerId;
	}

	const FLinearColor Color = MaskColor.Get(FLinearColor(0.015f, 0.015f, 0.02f, 1.f));
	const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
	const float ClampR = FMath::Min(R, FMath::Min(Size.X, Size.Y) * 0.5f);

	// Fill each corner's concave sliver with 1px horizontal bars following the arc.
	// Row width at distance `y` from the corner edge: R - sqrt(R^2 - (R-y)^2) — full at the corner,
	// tapering to zero where the arc meets the adjacent edge.
	auto DrawBar = [&](float X, float Y, float W, float H)
	{
		if (W <= 0.f || H <= 0.f)
		{
			return;
		}
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(FVector2f(W, H), FSlateLayoutTransform(FVector2f(X, Y))),
			White,
			ESlateDrawEffect::None,
			Color);
	};

	for (float Y = 0.f; Y < ClampR; Y += 1.f)
	{
		const float Dy = ClampR - Y;
		const float W = ClampR - FMath::Sqrt(FMath::Max(0.f, ClampR * ClampR - Dy * Dy));
		if (W <= 0.f)
		{
			continue;
		}
		const float H = FMath::Min(1.f, ClampR - Y);
		DrawBar(0.f, Y, W, H);                          // top-left
		DrawBar(Size.X - W, Y, W, H);                   // top-right
		DrawBar(0.f, Size.Y - Y - H, W, H);             // bottom-left
		DrawBar(Size.X - W, Size.Y - Y - H, W, H);      // bottom-right
	}

	return LayerId + 1;
}
