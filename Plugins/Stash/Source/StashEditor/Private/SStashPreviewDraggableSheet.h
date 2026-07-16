// Copyright Stash. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

DECLARE_DELEGATE_OneParam(FOnStashPreviewCardSheetHeightDrag, float /*ProvisionalHeight*/);
DECLARE_DELEGATE_TwoParams(FOnStashPreviewCardSheetDragEnded, float /*FinalHeight*/, bool /*bDismissRequested*/);

/** Card sheet wrapper: drag handle supports swipe-up expand and swipe-down dismiss. */
class SStashPreviewDraggableSheet : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SStashPreviewDraggableSheet)
		: _DragHeaderHeight(32.f)
		, _DismissDragThreshold(80.f)
	{}
		SLATE_ATTRIBUTE(float, SheetHeight)
		SLATE_ATTRIBUTE(float, BaseSheetHeight)
		SLATE_ATTRIBUTE(float, MaxExpandHeight)
		SLATE_ATTRIBUTE(bool, bEnableDragDismiss)
		SLATE_ATTRIBUTE(bool, bEnableExpandDrag)
		SLATE_EVENT(FSimpleDelegate, OnDismissRequested)
		SLATE_EVENT(FOnStashPreviewCardSheetHeightDrag, OnSheetHeightDragChanged)
		SLATE_EVENT(FOnStashPreviewCardSheetDragEnded, OnSheetDragEnded)
		SLATE_ARGUMENT(float, DragHeaderHeight)
		SLATE_ARGUMENT(float, DismissDragThreshold)
		SLATE_DEFAULT_SLOT(FArguments, Content)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override;

private:
	bool CanStartDrag(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) const;
	float ComputeProvisionalHeight(float TotalDeltaY) const;
	void ResetDrag();

	TAttribute<float> SheetHeight;
	TAttribute<float> BaseSheetHeight;
	TAttribute<float> MaxExpandHeight;
	TAttribute<bool> bEnableDragDismiss;
	TAttribute<bool> bEnableExpandDrag;
	FSimpleDelegate OnDismissRequested;
	FOnStashPreviewCardSheetHeightDrag OnSheetHeightDragChanged;
	FOnStashPreviewCardSheetDragEnded OnSheetDragEnded;
	float DragHeaderHeight = 32.f;
	float DismissDragThreshold = 80.f;
	float DragOffsetY = 0.f;
	float DragStartSheetHeight = 0.f;
	float DragTotalDeltaY = 0.f;
	float LastProvisionalHeight = 0.f;
	bool bDragging = false;
};
