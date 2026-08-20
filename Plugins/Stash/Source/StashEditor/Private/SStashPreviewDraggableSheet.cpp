// Copyright Stash. All Rights Reserved.

#include "SStashPreviewDraggableSheet.h"
#include "Widgets/Layout/SBox.h"

void SStashPreviewDraggableSheet::Construct(const FArguments& InArgs)
{
	SheetHeight = InArgs._SheetHeight;
	BaseSheetHeight = InArgs._BaseSheetHeight;
	MaxExpandHeight = InArgs._MaxExpandHeight;
	bEnableDragDismiss = InArgs._bEnableDragDismiss;
	bEnableExpandDrag = InArgs._bEnableExpandDrag;
	OnDismissRequested = InArgs._OnDismissRequested;
	OnSheetHeightDragChanged = InArgs._OnSheetHeightDragChanged;
	OnSheetDragEnded = InArgs._OnSheetDragEnded;
	DragHeaderHeight = InArgs._DragHeaderHeight;
	DismissDragThreshold = InArgs._DismissDragThreshold;

	ChildSlot
	[
		SNew(SBox)
		.RenderTransform_Lambda([this]()
		{
			return FSlateRenderTransform(FVector2D(0.f, DragOffsetY));
		})
		.RenderTransformPivot(FVector2D(0.5f, 0.f))
		[
			InArgs._Content.Widget
		]
	];
}

bool SStashPreviewDraggableSheet::CanStartDrag(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) const
{
	if (!bEnableDragDismiss.Get(false) && !bEnableExpandDrag.Get(false))
	{
		return false;
	}
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return false;
	}
	const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	return LocalPos.Y <= DragHeaderHeight;
}

float SStashPreviewDraggableSheet::ComputeProvisionalHeight(float TotalDeltaY) const
{
	const float BaseHeight = BaseSheetHeight.Get(SheetHeight.Get(0.f));
	const float MaxHeight = MaxExpandHeight.Get(BaseHeight);
	const float RawHeight = DragStartSheetHeight - TotalDeltaY;
	return FMath::Clamp(RawHeight, BaseHeight, MaxHeight);
}

FReply SStashPreviewDraggableSheet::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (CanStartDrag(MyGeometry, MouseEvent))
	{
		bDragging = true;
		DragOffsetY = 0.f;
		DragTotalDeltaY = 0.f;
		DragStartSheetHeight = SheetHeight.Get(0.f);
		LastProvisionalHeight = DragStartSheetHeight;
		return FReply::Handled().CaptureMouse(SharedThis(this));
	}
	return FReply::Unhandled();
}

FReply SStashPreviewDraggableSheet::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (bDragging && HasMouseCapture())
	{
		// GetCursorDelta() is in screen-space (desktop) pixels; divide by the geometry scale so deltas —
		// and the expand/dismiss thresholds they're compared against — are in local Slate units. This stays
		// correct under DPI scale and is immune to the drawer re-anchoring as its height changes (unlike an
		// AbsoluteToLocal-of-screen-position approach, which would drift when the sheet grows upward).
		const float DeltaY = MouseEvent.GetCursorDelta().Y / FMath::Max(MyGeometry.Scale, KINDA_SMALL_NUMBER);
		DragTotalDeltaY += DeltaY;

		if (bEnableExpandDrag.Get(false))
		{
			const float BaseHeight = BaseSheetHeight.Get(DragStartSheetHeight);
			const float ProvisionalHeight = ComputeProvisionalHeight(DragTotalDeltaY);
			LastProvisionalHeight = ProvisionalHeight;

			if (ProvisionalHeight > BaseHeight + KINDA_SMALL_NUMBER)
			{
				DragOffsetY = 0.f;
				OnSheetHeightDragChanged.ExecuteIfBound(ProvisionalHeight);
			}
			else
			{
				OnSheetHeightDragChanged.ExecuteIfBound(BaseHeight);
				const float ExcessDownDrag = FMath::Max(0.f, DragTotalDeltaY - (DragStartSheetHeight - BaseHeight));
				DragOffsetY = bEnableDragDismiss.Get(false) ? ExcessDownDrag : 0.f;
			}
		}
		else if (bEnableDragDismiss.Get(false))
		{
			DragOffsetY = FMath::Max(0.f, DragOffsetY + DeltaY);
		}

		Invalidate(EInvalidateWidget::PaintAndVolatility);
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FReply SStashPreviewDraggableSheet::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	// Gate on HasMouseCapture(): a left-up routed here without an active capture (e.g. after capture was
	// lost to window deactivation) must not evaluate the dismiss branch with stale drag state.
	if (bDragging && HasMouseCapture() && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bDragging = false;
		const float SheetH = SheetHeight.Get(0.f);
		const float Threshold = FMath::Max(DismissDragThreshold, SheetH * 0.25f);
		if (DragOffsetY >= Threshold && bEnableDragDismiss.Get(false))
		{
			ResetDrag();
			OnSheetDragEnded.ExecuteIfBound(LastProvisionalHeight, true);
			OnDismissRequested.ExecuteIfBound();
		}
		else
		{
			OnSheetDragEnded.ExecuteIfBound(LastProvisionalHeight, false);
			DragOffsetY = 0.f;
			Invalidate(EInvalidateWidget::PaintAndVolatility);
		}
		return FReply::Handled().ReleaseMouseCapture();
	}
	return FReply::Unhandled();
}

void SStashPreviewDraggableSheet::OnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
	// Capture can vanish without a routed left-up (window deactivation, PIE focus change). Drop the drag
	// so the sheet doesn't render permanently translated and a later left-up can't spuriously dismiss it.
	if (bDragging || !FMath::IsNearlyZero(DragOffsetY))
	{
		ResetDrag();
		Invalidate(EInvalidateWidget::PaintAndVolatility);
	}
	SCompoundWidget::OnMouseCaptureLost(CaptureLostEvent);
}

void SStashPreviewDraggableSheet::ResetDrag()
{
	bDragging = false;
	DragOffsetY = 0.f;
	DragTotalDeltaY = 0.f;
}
