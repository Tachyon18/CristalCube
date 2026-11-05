// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_CoreUpgradeRowWidget.h"

void UCC_CoreUpgradeRowWidget::SetAttributeData(const FCoreUpgradeAttribute& InAttribute, int32 CurrentSpent, int32 BankAvailable)
{
	AttributeType = InAttribute.AttributeType;

	// Core는 배율 가산(UCC_SkillBase::SpendCoreAttributePoint — Multiplier += ValuePerPoint) → 퍼센트 표기
	ApplyRow(InAttribute.DisplayName,
		FormatPercent(CurrentSpent * InAttribute.ValuePerPoint),
		CurrentSpent, InAttribute.MaxPoints, BankAvailable);
}

void UCC_CoreUpgradeRowWidget::RequestPoint(bool bIsRefund)
{
	OnPointRequested.Broadcast(AttributeType, bIsRefund);
}
