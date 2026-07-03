// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_SkillInventorySlotWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"


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
}
