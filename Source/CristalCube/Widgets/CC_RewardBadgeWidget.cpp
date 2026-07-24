// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_RewardBadgeWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

void UCC_RewardBadgeWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (BadgeButton)
    {
        BadgeButton->OnClicked.AddDynamic(this, &UCC_RewardBadgeWidget::HandleBadgeClicked);
    }

    SetPendingCount(0);
}

void UCC_RewardBadgeWidget::SetPendingCount(int32 Count)
{
    SetVisibility(Count > 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

    if (CountText)
    {
        CountText->SetText(FText::AsNumber(Count));
    }
}

void UCC_RewardBadgeWidget::HandleBadgeClicked()
{
    OnRewardBadgeClicked.Broadcast();
}
