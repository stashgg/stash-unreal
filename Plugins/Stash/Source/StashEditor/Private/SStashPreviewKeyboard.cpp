// Copyright Stash. All Rights Reserved.

#include "SStashPreviewKeyboard.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

namespace
{
	struct FKeyboardTheme
	{
		FLinearColor Background;
		FLinearColor Key;
		FLinearColor SpecialKey;
		FLinearColor Text;
	};

	FKeyboardTheme ThemeFor(EStashPreviewPlatform Platform)
	{
		if (Platform == EStashPreviewPlatform::Android)
		{
			// Gboard dark theme.
			return {
				FLinearColor(0.086f, 0.094f, 0.106f, 1.f),
				FLinearColor(0.204f, 0.220f, 0.243f, 1.f),
				FLinearColor(0.141f, 0.153f, 0.169f, 1.f),
				FLinearColor(0.92f, 0.93f, 0.95f, 1.f)
			};
		}
		// iOS light theme.
		return {
			FLinearColor(0.788f, 0.800f, 0.820f, 1.f),
			FLinearColor(0.99f, 0.99f, 1.f, 1.f),
			FLinearColor(0.655f, 0.678f, 0.714f, 1.f),
			FLinearColor(0.05f, 0.05f, 0.06f, 1.f)
		};
	}

	const FSlateRoundedBoxBrush& KeyBrush()
	{
		static const FSlateRoundedBoxBrush Brush(FLinearColor::White, 5.f);
		return Brush;
	}

	const FSlateBrush& FillBrush()
	{
		static const FSlateBrush* Brush = FCoreStyle::Get().GetBrush("WhiteBrush");
		return *Brush;
	}

	bool IsNumericInputType(const FString& InputType)
	{
		return InputType == TEXT("numeric")
			|| InputType == TEXT("decimal")
			|| InputType == TEXT("tel")
			|| InputType == TEXT("number");
	}

	struct FKeyDef
	{
		FString Label;
		float WidthWeight = 1.f;
		bool bSpecial = false;
	};

	// Static key layouts, built once on first paint instead of reallocated every OnPaint pass.
	const TArray<TArray<FKeyDef>>& NumericKeyRows()
	{
		static const TArray<TArray<FKeyDef>> Rows = {
			{{TEXT("1")}, {TEXT("2")}, {TEXT("3")}},
			{{TEXT("4")}, {TEXT("5")}, {TEXT("6")}},
			{{TEXT("7")}, {TEXT("8")}, {TEXT("9")}},
			{{TEXT(""), 1.f, true}, {TEXT("0")}, {TEXT("del"), 1.f, true}}
		};
		return Rows;
	}

	const TArray<TArray<FKeyDef>>& AlphaKeyRows()
	{
		static const TArray<TArray<FKeyDef>> Rows = []()
		{
			auto LetterRow = [](const TCHAR* Letters)
			{
				TArray<FKeyDef> Row;
				for (const TCHAR* C = Letters; *C; ++C)
				{
					Row.Add({FString::Chr(*C)});
				}
				return Row;
			};
			TArray<FKeyDef> ThirdRow;
			ThirdRow.Add({TEXT("shift"), 1.4f, true});
			ThirdRow.Append(LetterRow(TEXT("zxcvbnm")));
			ThirdRow.Add({TEXT("del"), 1.4f, true});
			TArray<TArray<FKeyDef>> Built;
			Built.Add(LetterRow(TEXT("qwertyuiop")));
			Built.Add(LetterRow(TEXT("asdfghjkl")));
			Built.Add(MoveTemp(ThirdRow));
			Built.Add({{TEXT("?123"), 1.4f, true}, {TEXT("space"), 5.f, false}, {TEXT("return"), 1.8f, true}});
			return Built;
		}();
		return Rows;
	}
}

void SStashPreviewKeyboard::Construct(const FArguments& InArgs)
{
	Platform = InArgs._Platform;
	InputType = InArgs._InputType;
	SetVisibility(EVisibility::HitTestInvisible);
}

int32 SStashPreviewKeyboard::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	const FVector2f Size = FVector2f(AllottedGeometry.GetLocalSize());
	if (Size.X <= 1.f || Size.Y <= 1.f)
	{
		return LayerId;
	}

	const FKeyboardTheme Theme = ThemeFor(Platform.Get(EStashPreviewPlatform::iOS));
	const bool bNumeric = IsNumericInputType(InputType.Get(FString()));

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(),
		&FillBrush(),
		ESlateDrawEffect::None,
		Theme.Background);
	++LayerId;

	const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

	// Reserve a suggestion strip at the top and a small bottom margin (home area is drawn by the panel).
	const float TopStrip = FMath::Min(34.f, Size.Y * 0.14f);
	const float BottomMargin = 8.f;

	// Suggestion / autofill strip: three faint placeholder chips + a divider.
	if (TopStrip > 10.f)
	{
		const float StripPad = 10.f;
		const float ChipH = FMath::Min(TopStrip * 0.55f, 18.f);
		const float ChipY = (TopStrip - ChipH) * 0.5f;
		const float ChipGap = 10.f;
		const float ChipW = (Size.X - 2.f * StripPad - 2.f * ChipGap) / 3.f;
		const FLinearColor ChipColor(Theme.Key.R, Theme.Key.G, Theme.Key.B, Theme.Key.A * 0.5f);
		for (int32 Chip = 0; Chip < 3; ++Chip)
		{
			const float ChipX = StripPad + Chip * (ChipW + ChipGap);
			FSlateDrawElement::MakeBox(
				OutDrawElements, LayerId,
				AllottedGeometry.ToPaintGeometry(FVector2f(ChipW, ChipH), FSlateLayoutTransform(FVector2f(ChipX, ChipY))),
				&KeyBrush(), ESlateDrawEffect::None, ChipColor);
		}
		FSlateDrawElement::MakeBox(
			OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(FVector2f(Size.X, 1.f), FSlateLayoutTransform(FVector2f(0.f, TopStrip - 1.f))),
			&FillBrush(), ESlateDrawEffect::None, FLinearColor(0.f, 0.f, 0.f, 0.18f));
	}
	++LayerId;
	const float SidePadding = 4.f;
	const float KeyGap = 5.f;
	const float RowsTop = TopStrip;
	const float RowsHeight = FMath::Max(1.f, Size.Y - TopStrip - BottomMargin);

	const TArray<TArray<FKeyDef>>& Rows = bNumeric ? NumericKeyRows() : AlphaKeyRows();

	const int32 NumRows = Rows.Num();
	const float RowHeight = (RowsHeight - KeyGap * (NumRows - 1)) / NumRows;
	const FSlateFontInfo KeyFont = FCoreStyle::GetDefaultFontStyle("Regular", FMath::Clamp(RowHeight * 0.34f, 8.f, 13.f));

	float RowY = RowsTop;
	for (const TArray<FKeyDef>& Row : Rows)
	{
		float TotalWeight = 0.f;
		for (const FKeyDef& Key : Row)
		{
			TotalWeight += Key.WidthWeight;
		}
		const float AvailableW = Size.X - 2.f * SidePadding - KeyGap * (Row.Num() - 1);
		const float UnitW = AvailableW / FMath::Max(TotalWeight, 1.f);

		float KeyX = SidePadding;
		for (const FKeyDef& Key : Row)
		{
			const float KeyW = UnitW * Key.WidthWeight;
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(FVector2f(KeyW, RowHeight), FSlateLayoutTransform(FVector2f(KeyX, RowY))),
				&KeyBrush(),
				ESlateDrawEffect::None,
				Key.bSpecial ? Theme.SpecialKey : Theme.Key);

			if (!Key.Label.IsEmpty())
			{
				const FVector2D TextSize = FontMeasure->Measure(Key.Label, KeyFont);
				const FVector2f TextPos(
					KeyX + (KeyW - static_cast<float>(TextSize.X)) * 0.5f,
					RowY + (RowHeight - static_cast<float>(TextSize.Y)) * 0.5f);
				FSlateDrawElement::MakeText(
					OutDrawElements,
					LayerId + 1,
					AllottedGeometry.ToPaintGeometry(FVector2f(TextSize), FSlateLayoutTransform(TextPos)),
					Key.Label,
					KeyFont,
					ESlateDrawEffect::None,
					Theme.Text);
			}
			KeyX += KeyW + KeyGap;
		}
		RowY += RowHeight + KeyGap;
	}

	return LayerId + 2;
}
