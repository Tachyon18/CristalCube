// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_AddonUpgradeRowWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"


void UCC_AddonUpgradeRowWidget::SetAttributeData(ESkillAddonType InAddonType, const FAddonUpgradeAttribute& InAttribute, int32 CurrentSpent, int32 BankAvailable)
{
    AddonType = InAddonType;
    AttributeID = InAttribute.AttributeID;

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
        if (!SpendButton->OnClicked.IsAlreadyBound(this, &UCC_AddonUpgradeRowWidget::HandleSpendClicked))
        {
            SpendButton->OnClicked.AddDynamic(this, &UCC_AddonUpgradeRowWidget::HandleSpendClicked);
        }
    }

	if (RefundButton)
	{
        RefundButton->SetIsEnabled(CurrentSpent > 0);
        if (!RefundButton->OnClicked.IsAlreadyBound(this, &UCC_AddonUpgradeRowWidget::HandleRefundClicked))
        {
            RefundButton->OnClicked.AddDynamic(this, &UCC_AddonUpgradeRowWidget::HandleRefundClicked);
        }
	}
}

void UCC_AddonUpgradeRowWidget::HandleSpendClicked()
{
    OnPointRequested.Broadcast(AddonType, AttributeID, false);
}

void UCC_AddonUpgradeRowWidget::HandleRefundClicked()
{
    OnPointRequested.Broadcast(AddonType, AttributeID, true);
}
