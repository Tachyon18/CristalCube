// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CC_SkillInventoryWidget.generated.h"

class UCC_SkillInventorySlotWidget;
class ACC_PlayerState;

/**
 * 플레이 중 장착 스킬을 고정 6슬롯으로 보여주는 인벤토리 위젯.
 * PlayerState::OnSkillsChanged를 구독해서 Grant/Remove 시 자동 갱신된다.
 *
 * [Blueprint 설정]
 *  1. Parent Class = UCC_SkillInventoryWidget, 이름 = WBP_SkillInventory
 *  2. Slot1~Slot6을 각각 WBP_SkillInventorySlot(Parent=UCC_SkillInventorySlotWidget) 인스턴스로 배치,
 *     이름을 정확히 Slot1, Slot2, ... Slot6으로 맞출 것
 */
UCLASS()
class CRISTALCUBE_API UCC_SkillInventoryWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UFUNCTION(BlueprintCallable, Category = "Skill Inventory")
    void RefreshInventory();

protected:
    UPROPERTY(meta = (BindWidget))
    UCC_SkillInventorySlotWidget* Slot1;

    UPROPERTY(meta = (BindWidget))
    UCC_SkillInventorySlotWidget* Slot2;

    UPROPERTY(meta = (BindWidget))
    UCC_SkillInventorySlotWidget* Slot3;

    UPROPERTY(meta = (BindWidget))
    UCC_SkillInventorySlotWidget* Slot4;

    UPROPERTY(meta = (BindWidget))
    UCC_SkillInventorySlotWidget* Slot5;

    UPROPERTY(meta = (BindWidget))
    UCC_SkillInventorySlotWidget* Slot6;

private:
    /** Slot1~6을 순회용 배열로 캐싱 (NativeConstruct에서 1회 구성) */
    UPROPERTY()
    TArray<UCC_SkillInventorySlotWidget*> Slots;

    UPROPERTY()
    ACC_PlayerState* BoundPlayerState = nullptr;

    UFUNCTION()
    void HandleSkillsChanged();
};
