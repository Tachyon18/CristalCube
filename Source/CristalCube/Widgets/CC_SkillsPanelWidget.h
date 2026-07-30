// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CC_SkillsPanelWidget.generated.h"

class UCC_SkillSlotWidget;
class ACC_PlayerState;

/**
 * GameHUD 상시 노출 액션바. 정사각형 GlassPanel 슬롯 6개로 구성.
 * PlayerState::OnSkillsChanged를 구독해 Grant/Remove/Swap 시 자동 갱신,
 * 매 프레임 쿨다운 진행률을 슬롯에 밀어준다.
 * Ctrl 홀드 드래그 재배치는 CC_SkillSlotWidget 자체 기능이라 별도 구현 불필요 —
 * 여기선 OnSlotDropped를 받아서 SwapSlots()만 호출.
 *
 * [Blueprint 설정]
 *  1. Parent Class = UCC_SkillsPanelWidget, 이름 = WBP_SkillsPanel
 *  2. Slot1~Slot6을 WBP_SkillSlot 인스턴스로 배치, 이름 정확히 Slot1~Slot6
 *  3. WBP_GameHUD 안에 자식 위젯으로 배치 (원하는 위치에 앵커)
 */
UCLASS()
class CRISTALCUBE_API UCC_SkillsPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    UFUNCTION(BlueprintCallable, Category = "Skills Panel")
    void RefreshPanel();

protected:

    UPROPERTY(meta = (BindWidget))
    UCC_SkillSlotWidget* Slot1;
    UPROPERTY(meta = (BindWidget))
    UCC_SkillSlotWidget* Slot2;
    UPROPERTY(meta = (BindWidget))
    UCC_SkillSlotWidget* Slot3;
    UPROPERTY(meta = (BindWidget))
    UCC_SkillSlotWidget* Slot4;
    UPROPERTY(meta = (BindWidget))
    UCC_SkillSlotWidget* Slot5;
    UPROPERTY(meta = (BindWidget))
    UCC_SkillSlotWidget* Slot6;

    UFUNCTION()
    void HandleSlotDropped(int32 SourceSlotIndex, int32 TargetSlotIndex);

private:
    UPROPERTY()
    TArray<UCC_SkillSlotWidget*> Slots;

    UPROPERTY()
    ACC_PlayerState* BoundPlayerState = nullptr;

    UFUNCTION()
    void HandleSkillsChanged();
	
};
