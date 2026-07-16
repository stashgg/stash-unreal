// Copyright Stash. All Rights Reserved.

#include "SStashPreviewStatusIcons.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

namespace
{
	const FSlateBrush& FillBrush()
	{
		static const FSlateBrush* Brush = FCoreStyle::Get().GetBrush("WhiteBrush");
		return *Brush;
	}

	void DrawRect(FSlateWindowElementList& Out, int32 Layer, const FGeometry& Geo,
		float X, float Y, float W, float H, const FLinearColor& Color)
	{
		if (W <= 0.f || H <= 0.f)
		{
			return;
		}
		FSlateDrawElement::MakeBox(
			Out, Layer,
			Geo.ToPaintGeometry(FVector2f(W, H), FSlateLayoutTransform(FVector2f(X, Y))),
			&FillBrush(), ESlateDrawEffect::None, Color);
	}
}

void SStashPreviewStatusIcons::Construct(const FArguments& InArgs)
{
	GlyphColor = InArgs._GlyphColor;
	bShowCellular = InArgs._bShowCellular;
	SetVisibility(EVisibility::HitTestInvisible);
}

FVector2D SStashPreviewStatusIcons::ComputeDesiredSize(float) const
{
	// signal (~20) + wifi (~16) + battery (~26) + gaps.
	return FVector2D(72.f, 14.f);
}

int32 SStashPreviewStatusIcons::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	const FVector2f Size = FVector2f(AllottedGeometry.GetLocalSize());
	const FLinearColor Color = GlyphColor.Get(FLinearColor::White);
	const float MidY = Size.Y * 0.5f;

	// Right-aligned row: [signal] [wifi] [battery]. Walk right→left.
	float X = Size.X;

	// Battery: body pill + tip nub.
	const float BattW = 22.f, BattH = 11.f, TipW = 2.f, TipH = 5.f, TipGap = 1.f;
	X -= (BattW + TipW + TipGap);
	const float BattX = X;
	const float BattY = MidY - BattH * 0.5f;
	// Outline effect: full pill, then hollow, then inner fill (shows ~80%).
	DrawRect(OutDrawElements, LayerId, AllottedGeometry, BattX, BattY, BattW, BattH, Color);
	DrawRect(OutDrawElements, LayerId + 1, AllottedGeometry, BattX + 1.f, BattY + 1.f, BattW - 2.f, BattH - 2.f,
		FLinearColor(Color.R, Color.G, Color.B, Color.A * 0.25f));
	DrawRect(OutDrawElements, LayerId + 2, AllottedGeometry, BattX + 1.5f, BattY + 1.5f, (BattW - 3.f) * 0.8f, BattH - 3.f, Color);
	DrawRect(OutDrawElements, LayerId + 1, AllottedGeometry, BattX + BattW + TipGap, MidY - TipH * 0.5f, TipW, TipH, Color);

	const float GlyphGap = 6.f;

	// Wifi: stacked triangle-fan (wide base at top → narrow apex at bottom, like a real signal fan).
	const float WifiW = 15.f, WifiH = 11.f;
	X -= (GlyphGap + WifiW);
	const float WifiX = X;
	const float WifiTop = MidY - WifiH * 0.5f;
	const int32 WifiRows = 5;
	const float RowH = WifiH / WifiRows;
	for (int32 Row = 0; Row < WifiRows; ++Row)
	{
		// Row 0 = base (wide, top); last row = apex (narrow, bottom).
		const float T = static_cast<float>(WifiRows - Row) / WifiRows;
		const float RowW = WifiW * T;
		const float RowX = WifiX + (WifiW - RowW) * 0.5f;
		const float RowY = WifiTop + Row * RowH;
		DrawRect(OutDrawElements, LayerId, AllottedGeometry, RowX, RowY, RowW, RowH - 0.6f, Color);
	}

	// Cellular signal: 4 vertical bars of increasing height.
	if (bShowCellular.Get(true))
	{
		const float BarW = 3.f, BarGap = 1.6f;
		const float Heights[4] = { 4.f, 7.f, 10.f, 13.f };
		const float BlockW = 4 * BarW + 3 * BarGap;
		X -= (GlyphGap + BlockW);
		float BarX = X;
		for (int32 Bar = 0; Bar < 4; ++Bar)
		{
			const float BarH = Heights[Bar];
			DrawRect(OutDrawElements, LayerId, AllottedGeometry, BarX, MidY + 6.5f - BarH, BarW, BarH, Color);
			BarX += BarW + BarGap;
		}
	}

	return LayerId + 3;
}
