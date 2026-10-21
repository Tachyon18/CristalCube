// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../CristalCubeStruct.h"
#include "CC_AddonUpgradeRowWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnAddonAttributePointRequested,
    ESkillAddonType, AddonType, FName, AttributeID, bool, bIsRefund);

/**
 * Addon 포인트 배분 카드(UCC_AddonUpgradeCardWidget) 내부에서 동적으로 생성되는
 * 속성 1개짜리 행. 표시 전용 + 버튼 입력만 처리 — 실제 배분(PlayerState::SpendAddonPoint)은
 * 상위 컨테이너가 담당 (다른 위젯들과 동일한 "컨테이너가 데이터/로직 소유" 원칙 유지).
 */
UCLASS()
class CRISTALCUBE_API UCC_AddonUpgradeRowWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
    /** ESkillAddonType은 호출부가 이미 알고 있어서 여기 다시 안 들고, AttributeDef/현재 배분량/은행 잔액만 받음 */
    UFUNCTION(BlueprintCallable, Category = "Addon Upgrade")
    void SetAttributeData(ESkillAddonType InAddonType, const FAddonUpgradeAttribute& InAttribute,
        int32 CurrentSpent, int32 BankAvailable);

    /** bIsRefund = false면 배분(+), true면 회수(-) 요청 */
    UPROPERTY(BlueprintAssignable, Category = "Addon Upgrade")
    FOnAddonAttributePointRequested OnPointRequested;

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
    ESkillAddonType AddonType = ESkillAddonType::None;
    FName AttributeID = NAME_None;
};
