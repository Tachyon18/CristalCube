// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_SkillCardWidget.h"
#include "../SkillSystem/CC_SkillCardData.h"
#include "CC_AddonBadgeWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/PanelWidget.h"

void UCC_SkillCardWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    UCC_SkillCardData* CardData = Cast<UCC_SkillCardData>(ListItemObject);
    if (!CardData)
    {
        return;
    }

    const FSkillDisplayData& Data = CardData->SkillData;

    if (SkillIcon && Data.Icon)
    {
        SkillIcon->SetBrushFromTexture(Data.Icon);
    }

    if (SkillName)
    {
        SkillName->SetText(Data.DisplayName);
    }

    if (SkillDescription)
    {
        SkillDescription->SetText(Data.Description);
    }

    RefreshAddonBadges(CardData->AddonBadges);
}

void UCC_SkillCardWidget::RefreshAddonBadges(const TArray<FAddonTableRow>& AddonBadges)
{
    if (!AddonBadgeContainer)
    {
        return;
    }

    AddonBadgeContainer->ClearChildren();

    if (!AddonBadgeWidgetClass)
    {
        // 뱃지 비주얼 미확정 단계 — 스킵. 카드 자체는 정상 동작.
        return;
    }

    for (const FAddonTableRow& Addon : AddonBadges)
    {
        UCC_AddonBadgeWidget* BadgeWidget = CreateWidget<UCC_AddonBadgeWidget>(this, AddonBadgeWidgetClass);
        if (!BadgeWidget)
        {
            continue;
        }

        BadgeWidget->SetAddonData(Addon);
        AddonBadgeContainer->AddChild(BadgeWidget);
    }
}
