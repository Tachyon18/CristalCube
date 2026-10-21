// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../CristalCubeStruct.h"
#include "CC_CubeClearRewardWidget.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCubeClearRewardSelected, FCubeClearReward, SelectedReward);

/**
 * CubeClear 보상 카드 패널. UCC_LevelUpWidget과 동일한 "고정 3슬롯" 구조를 재사용한다.
 * RewardBadge 클릭으로만 열리는 논블로킹 흐름의 일부(자동 팝업 아님).
 * PendingRewardSlots가 남아있으면 GameMode가 위젯을 닫지 않고 SetRewardChoices()를
 * 다시 호출해 다음 3장으로 이어간다(연속 픽).
 */
UCLASS()
class CRISTALCUBE_API UCC_CubeClearRewardWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:

    virtual void NativeConstruct() override;

    /** 카드 3장 채우기. 이미 열려있는 상태에서 다시 호출해도 됨 (연속 픽 갱신용). */
    UFUNCTION(BlueprintCallable, Category = "Cube Clear Reward")
    void SetRewardChoices(const TArray<FCubeClearReward>& InRewards);

    UPROPERTY(BlueprintAssignable, Category = "Cube Clear Reward")
    FOnCubeClearRewardSelected OnCubeClearRewardSelected;

protected:

    UPROPERTY(meta = (BindWidget)) class UButton* Choice1Button;
    UPROPERTY(meta = (BindWidget)) UButton* Choice2Button;
    UPROPERTY(meta = (BindWidget)) UButton* Choice3Button;
    UPROPERTY(meta = (BindWidget)) class UTextBlock* Choice1Name;
    UPROPERTY(meta = (BindWidget)) UTextBlock* Choice2Name;
    UPROPERTY(meta = (BindWidget)) UTextBlock* Choice3Name;
    UPROPERTY(meta = (BindWidget)) UTextBlock* Choice1Description;
    UPROPERTY(meta = (BindWidget)) UTextBlock* Choice2Description;
    UPROPERTY(meta = (BindWidget)) UTextBlock* Choice3Description;
    UPROPERTY(meta = (BindWidget)) class UImage* Choice1Icon;
    UPROPERTY(meta = (BindWidget)) UImage* Choice2Icon;
    UPROPERTY(meta = (BindWidget)) UImage* Choice3Icon;
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) class UCC_IntroGlassWidget* CardPanel1;
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) UCC_IntroGlassWidget* CardPanel2;
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) UCC_IntroGlassWidget* CardPanel3;

    UFUNCTION() void OnChoice1Clicked();
    UFUNCTION() void OnChoice2Clicked();
    UFUNCTION() void OnChoice3Clicked();

private:
    TArray<FCubeClearReward> CurrentChoices;
    void SelectReward(int32 ChoiceIndex);
    void ApplyCardVisual(int32 Index, class UTextBlock* NameText, class UTextBlock* DescText, class UImage* IconImage);

};
