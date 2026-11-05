// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CC_UpgradeRowWidget.h"
#include "../CristalCubeStruct.h"
#include "CC_AddonUpgradeRowWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnAddonAttributePointRequested,
	ESkillAddonType, AddonType, FName, AttributeID, bool, bIsRefund);

UCLASS()
class CRISTALCUBE_API UCC_AddonUpgradeRowWidget : public UCC_UpgradeRowWidget
{
	GENERATED_BODY()

public:
	/** 효과값 = CurrentSpent × ValuePerPoint → 부호 숫자 ("+20" · 단위는 속성마다 달라 B6에서 표 정리) */
	UFUNCTION(BlueprintCallable, Category = "Addon Upgrade")
	void SetAttributeData(ESkillAddonType InAddonType, const FAddonUpgradeAttribute& InAttribute,
		int32 CurrentSpent, int32 BankAvailable);

	UFUNCTION(BlueprintPure, Category = "Addon Upgrade")
	FName GetAttributeID() const { return AttributeID; }

	/** bIsRefund = false면 배분(+), true면 회수(−) 요청 */
	UPROPERTY(BlueprintAssignable, Category = "Addon Upgrade")
	FOnAddonAttributePointRequested OnPointRequested;

protected:
	virtual void RequestPoint(bool bIsRefund) override;

private:
	ESkillAddonType AddonType = ESkillAddonType::None;
	FName AttributeID = NAME_None;
};