// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_SkillInventorySlotWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "CC_SkillSlotDragOp.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

void UCC_SkillInventorySlotWidget::SetSkillData(const FSkillDisplayData& InData)
{
    if (SkillIcon && InData.Icon)
    {
        SkillIcon->SetBrushFromTexture(InData.Icon);
        SkillIcon->SetVisibility(ESlateVisibility::Visible);
    }

    if (SkillName)
    {
        SkillName->SetText(InData.DisplayName);
    }

    if (EmptySlotOverlay)
    {
        EmptySlotOverlay->SetVisibility(ESlateVisibility::Collapsed);
    }

    bIsOccupied = true;
}

void UCC_SkillInventorySlotWidget::ClearSlot()
{
    if (SkillIcon)
    {
        SkillIcon->SetVisibility(ESlateVisibility::Hidden);
    }

    if (SkillName)
    {
        SkillName->SetText(FText::GetEmpty());
    }

    if (EmptySlotOverlay)
    {
        EmptySlotOverlay->SetVisibility(ESlateVisibility::Visible);
    }

    bIsOccupied = false;
}

FReply UCC_SkillInventorySlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{   
    // Ctrl 안 누르면 그대로 게임(조준/공격)으로 흘려보냄 — 평소 플레이 방해 없음
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && InMouseEvent.IsControlDown() && bIsOccupied)
    {
        return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
    }
    return FReply::Unhandled();
}

void UCC_SkillInventorySlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
    UCC_SkillSlotDragOp* DragOp = NewObject<UCC_SkillSlotDragOp>(this);
    DragOp->SourceSlotIndex = SlotIndex;
    DragOp->DefaultDragVisual = this;
    DragOp->Pivot = EDragPivot::MouseDown;
    OutOperation = DragOp;
}

bool UCC_SkillInventorySlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    if (UCC_SkillSlotDragOp* DragOp = Cast<UCC_SkillSlotDragOp>(InOperation))
    {
        OnSlotDropped.Broadcast(DragOp->SourceSlotIndex, SlotIndex);
        return true;
    }
    return false;
}
