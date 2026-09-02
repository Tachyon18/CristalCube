// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../CristalCubeStruct.h"
#include "CC_CoreUpgradeRowWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCoreAttributePointRequested,
	ECoreUpgradeAttribute, AttributeType, bool, bIsRefund);

/**
 * Core 강화(Damage/Cooldown/Range/Area) 카탈로그 1개짜리 행.
 * CC_AddonUpgradeRowWidget과 같은 역할이지만, Core는 고정된 4종 enum 집합이라
 * FName 대신 ECoreUpgradeAttribute로 타입을 고정 — Addon 위젯과는 완전히 별개 클래스.
 * 표시 전용 + 버튼 입력만 처리, 실제 배분은 CC_SkillUpgradeDetailWidget이 담당.
 */
UCLASS()
class CRISTALCUBE_API UCC_CoreUpgradeRowWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
    UFUNCTION(BlueprintCallable, Category = "Core Upgrade")
    void SetAttributeData(const FCoreUpgradeAttribute& InAttribute, int32 CurrentSpent, int32 BankAvailable);

    /** bIsRefund = false면 배분(+), true면 회수(-) 요청 */
    UPROPERTY(BlueprintAssignable, Category = "Core Upgrade")
    FOnCoreAttributePointRequested OnPointRequested;

protected:
    UPROPERTY(meta = (BindWidget))
    class UTextBlock* AttributeName;

    /** "3 / 10" 또는 MaxPoints=0이면 "3" 형태로 표시 */
    UPROPERTY(meta = (BindWidget))
    UTextBlock* PointsText;

    UPROPERTY(meta = (BindWidget))
    class UButton* SpendButton;

    /** Optional — 배치 안 해도 안전. 있으면 배분한 포인트를 1개씩 회수 가능. */
    UPROPERTY(meta = (BindWidgetOptional))
    class UButton* RefundButton;

    UFUNCTION()
    void HandleSpendClicked();

    UFUNCTION()
    void HandleRefundClicked();

private:
    ECoreUpgradeAttribute AttributeType = ECoreUpgradeAttribute::Damage;
};
