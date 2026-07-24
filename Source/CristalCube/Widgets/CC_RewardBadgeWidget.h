// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CC_RewardBadgeWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRewardBadgeClicked);

/**
 * GameHUD 코너에 상시 떠있는 CubeClear 보상 알림 인디케이터.
 * PendingRewardSlots가 0이면 숨김, 1 이상이면 숫자와 함께 표시.
 * 클릭하면 OnRewardBadgeClicked를 broadcast — 실제 카드 패널 오픈은
 * 이 델리게이트를 구독하는 쪽(GameHUD → GameMode)이 담당한다.
 */
UCLASS()
class CRISTALCUBE_API UCC_RewardBadgeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    /** 대기 중인 보상 슬롯 수 갱신. 0이면 자동으로 숨김. */
    UFUNCTION(BlueprintCallable, Category = "Reward Badge")
    void SetPendingCount(int32 Count);

    UPROPERTY(BlueprintAssignable, Category = "Reward Badge")
    FOnRewardBadgeClicked OnRewardBadgeClicked;

protected:
    UPROPERTY(meta = (BindWidget))
    class UButton* BadgeButton;

    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* CountText;

    UFUNCTION()
    void HandleBadgeClicked();
};
