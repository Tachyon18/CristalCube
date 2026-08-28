// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_SkillSlotWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/Button.h"
#include "CC_SkillSlotDragOp.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

void UCC_SkillSlotWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (UpgradeButton && !UpgradeButton->OnClicked.IsAlreadyBound(this, &UCC_SkillSlotWidget::HandleUpgradeClicked))
    {
        UpgradeButton->OnClicked.AddDynamic(this, &UCC_SkillSlotWidget::HandleUpgradeClicked);
    }
}

void UCC_SkillSlotWidget::SetSkillData(const FSkillDisplayData& InData)
{
    if (SkillIcon)
    {
        if (InData.Icon)
        {
            SkillIcon->SetBrushFromTexture(InData.Icon);
        }
        // 텍스처 유무와 무관하게 Visible로 무조건 되돌림 — ClearSlot()의 Hidden과 항상 대칭되게
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

void UCC_SkillSlotWidget::ClearSlot()
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

void UCC_SkillSlotWidget::SetCooldownProgress(float Progress01)
{
    const bool bNewReady = Progress01 >= 1.0f;

    if (bNewReady && !bIsReady && ReadyFlashAnim)
    {
        PlayAnimation(ReadyFlashAnim);   // 완료 "순간"에만 1회 재생, 매 프레임 재생 안 됨
    }

    bIsReady = bNewReady;

    if (!CooldownOverlay) return;

    CooldownOverlay->SetPercent(1.0f - FMath::Clamp(Progress01, 0.0f, 1.0f));
    CooldownOverlay->SetVisibility(Progress01 >= 1.0f ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);

}

void UCC_SkillSlotWidget::SetSlotIndex(int32 InIndex)
{
    SlotIndex = InIndex;
    if (HotkeyLabel)
    {
        HotkeyLabel->SetText(FText::AsNumber(InIndex + 1));
    }
}

FReply UCC_SkillSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{   
    // Ctrl 안 누르면 그대로 게임(조준/공격)으로 흘려보냄 — 평소 플레이 방해 없음
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && InMouseEvent.IsControlDown() && bIsOccupied && bIsReady)
    {
        return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
    }
    return FReply::Unhandled();
}

void UCC_SkillSlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
    UCC_SkillSlotDragOp* DragOp = NewObject<UCC_SkillSlotDragOp>(this);
    DragOp->SourceSlotIndex = SlotIndex;

    // this(실제 슬롯 본체)를 드래그 비주얼로 쓰면 패널에서 뽑혀나가서 드롭 후 사라짐.
    // 아이콘만 복제한 별도 UImage를 유령으로 띄운다.
    if (SkillIcon)
    {
        UImage* GhostImage = NewObject<UImage>(this);
        GhostImage->SetBrush(SkillIcon->GetBrush());
        GhostImage->SetRenderOpacity(0.75f);
        // 원본 슬롯의 실제 렌더 크기를 그대로 강제 지정 — 안 하면 텍스처 원본 크기로 그려짐
        GhostImage->SetDesiredSizeOverride(InGeometry.GetLocalSize());
        DragOp->DefaultDragVisual = GhostImage;
    
    }
    DragOp->Pivot = EDragPivot::MouseDown;
    OutOperation = DragOp;
}

bool UCC_SkillSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    // 쿨다운 중인 슬롯엔(= 도착지가 이 슬롯) 드롭 자체를 막음 — 출발지 가드와 대칭
    if (!bIsReady)
    {
        return false;
    }

    if (UCC_SkillSlotDragOp* DragOp = Cast<UCC_SkillSlotDragOp>(InOperation))
    {
        OnSlotDropped.Broadcast(DragOp->SourceSlotIndex, SlotIndex);
        return true;
    }
    return false;
}

void UCC_SkillSlotWidget::HandleUpgradeClicked()
{
    if (bIsOccupied)
    {
        OnSlotUpgradeRequested.Broadcast(SlotIndex);
    }
}
