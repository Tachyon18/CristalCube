// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../SkillSystem/CC_SkillLibrarySubsystem.h"
#include "CC_SkillInventorySlotWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSlotDropped, int32, SourceSlotIndex, int32, TargetSlotIndex);

/**
 * WBP_SkillInventory의 고정 슬롯 1개.
 * 순수 표시 전용 — 서브시스템이나 PlayerState를 직접 참조하지 않는다.
 * 컨테이너(UCC_SkillInventoryWidget)가 데이터를 밀어넣거나 비운다.
 */
UCLASS()
class CRISTALCUBE_API UCC_SkillInventorySlotWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
    UFUNCTION(BlueprintCallable, Category = "Skill Inventory")
    void SetSkillData(const FSkillDisplayData& InData);

    UFUNCTION(BlueprintCallable, Category = "Skill Inventory")
    void ClearSlot();

    /** 컨테이너가 생성 시 주입 — EquippedSkills 인덱스와 일치시킴 */
    UFUNCTION(BlueprintCallable, Category = "Skill Inventory")
    void SetSlotIndex(int32 InIndex) { SlotIndex = InIndex; }

    UFUNCTION(BlueprintPure, Category = "Skill Inventory")
    int32 GetSlotIndex() const { return SlotIndex; }

    /** 컨테이너가 구독해서 실제 SwapSlots() 호출을 담당 */
    UPROPERTY(BlueprintAssignable, Category = "Skill Inventory")
    FOnSlotDropped OnSlotDropped;

protected:
    UPROPERTY(meta = (BindWidget))
    class UImage* SkillIcon;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* SkillName;

    /** 빈 슬롯일 때 표시할 오버레이 (예: 반투명 "+" 아이콘). 없으면 그냥 스킵. */
    UPROPERTY(meta = (BindWidgetOptional))
    class UWidget* EmptySlotOverlay;

    UPROPERTY(BlueprintReadOnly)
    int32 SlotIndex = -1;

    UPROPERTY(BlueprintReadOnly)
    bool bIsOccupied = false;

    //==========================================================================
    // DRAG & DROP — Ctrl을 누른 상태에서만 활성화 (느슨한 "스킬 조정 모드")
    //==========================================================================

    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, class UDragDropOperation*& OutOperation) override;
    virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, class UDragDropOperation* InOperation) override;

};
