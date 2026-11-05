// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CC_UpgradeRowWidget.h"
#include "../CristalCubeStruct.h"
#include "CC_CoreUpgradeRowWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCoreAttributePointRequested,
	ECoreUpgradeAttribute, AttributeType, bool, bIsRefund);

UCLASS()
class CRISTALCUBE_API UCC_CoreUpgradeRowWidget : public UCC_UpgradeRowWidget
{
	GENERATED_BODY()

public:
	/** 효과값 = CurrentSpent × ValuePerPoint → 퍼센트 ("+30%" · Cooldown "−8%") */
	UFUNCTION(BlueprintCallable, Category = "Core Upgrade")
	void SetAttributeData(const FCoreUpgradeAttribute& InAttribute, int32 CurrentSpent, int32 BankAvailable);

	UFUNCTION(BlueprintPure, Category = "Core Upgrade")
	ECoreUpgradeAttribute GetAttributeType() const { return AttributeType; }

	/** bIsRefund = false면 배분(+), true면 회수(−) 요청 */
	UPROPERTY(BlueprintAssignable, Category = "Core Upgrade")
	FOnCoreAttributePointRequested OnPointRequested;

protected:
	virtual void RequestPoint(bool bIsRefund) override;

private:
	ECoreUpgradeAttribute AttributeType = ECoreUpgradeAttribute::Damage;
};