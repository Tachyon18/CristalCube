// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_AddonUpgradeRowWidget.h"

void UCC_AddonUpgradeRowWidget::SetAttributeData(ESkillAddonType InAddonType, const FAddonUpgradeAttribute& InAttribute,
	int32 CurrentSpent, int32 BankAvailable)
{
	AddonType = InAddonType;
	AttributeID = InAttribute.AttributeID;

	ApplyRow(InAttribute.DisplayName,
		FormatSigned(CurrentSpent * InAttribute.ValuePerPoint),
		CurrentSpent, InAttribute.MaxPoints, BankAvailable);
}

void UCC_AddonUpgradeRowWidget::RequestPoint(bool bIsRefund)
{
	OnPointRequested.Broadcast(AddonType, AttributeID, bIsRefund);
}