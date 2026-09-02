// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_CoreUpgradeRowWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"

void UCC_CoreUpgradeRowWidget::SetAttributeData(const FCoreUpgradeAttribute& InAttribute, int32 CurrentSpent, int32 BankAvailable)
{
    AttributeType = InAttribute.AttributeType;

    if (AttributeName) AttributeName->SetText(InAttribute.DisplayName);

    if (PointsText)
    {
        PointsText->SetText(InAttribute.MaxPoints > 0
            ? FText::FromString(FString::Printf(TEXT("%d / %d"), CurrentSpent, InAttribute.MaxPoints))
            : FText::FromString(FString::Printf(TEXT("%d"), CurrentSpent)));
    }

    const bool bMaxed = InAttribute.MaxPoints > 0 && CurrentSpent >= InAttribute.MaxPoints;
    if (SpendButton)
    {
        SpendButton->SetIsEnabled(BankAvailable > 0 && !bMaxed);
        if (!SpendButton->OnClicked.IsAlreadyBound(this, &UCC_CoreUpgradeRowWidget::HandleSpendClicked))
        {
            SpendButton->OnClicked.AddDynamic(this, &UCC_CoreUpgradeRowWidget::HandleSpendClicked);
        }
    }

    if (RefundButton)
    {
        RefundButton->SetIsEnabled(CurrentSpent > 0);
        if (!RefundButton->OnClicked.IsAlreadyBound(this, &UCC_CoreUpgradeRowWidget::HandleRefundClicked))
        {
            RefundButton->OnClicked.AddDynamic(this, &UCC_CoreUpgradeRowWidget::HandleRefundClicked);
        }
    }
}

void UCC_CoreUpgradeRowWidget::HandleSpendClicked()
{
    OnPointRequested.Broadcast(AttributeType, false);
}

void UCC_CoreUpgradeRowWidget::HandleRefundClicked()
{
    OnPointRequested.Broadcast(AttributeType, true);
}
