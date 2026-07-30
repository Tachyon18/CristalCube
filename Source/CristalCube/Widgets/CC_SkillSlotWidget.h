// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../SkillSystem/CC_SkillLibrarySubsystem.h"
#include "CC_SkillSlotWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSlotDropped, int32, SourceSlotIndex, int32, TargetSlotIndex);

/**
 * 스킬 아이콘 표시 슬롯 1개 (범용).
 * 순수 표시 전용 — 서브시스템이나 PlayerState를 직접 참조하지 않는다.
 * 컨테이너가 데이터를 밀어넣거나 비운다. WBP_SkillsPanel(상시 액션바),
 * WBP_SkillInventory(관리 화면) 양쪽에서 재사용됨.
 * SkillName/EmptySlotOverlay/CooldownOverlay/HotkeyLabel은 전부 Optional이라,
 * 컨테이너 성격에 따라 필요한 것만 골라 배치하면 됨
 * (예: SkillsPanel엔 이름 텍스트 없이 아이콘+쿨다운+단축키만).
 */
UCLASS()
class CRISTALCUBE_API UCC_SkillSlotWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
    UFUNCTION(BlueprintCallable, Category = "Skill Inventory")
    void SetSkillData(const FSkillDisplayData& InData);

    UFUNCTION(BlueprintCallable, Category = "Skill Inventory")
    void ClearSlot();

    /** Progress: 0=막 사용(꽉 덮임), 1=사용 가능(안 덮임) */
    UFUNCTION(BlueprintCallable, Category = "Skill Inventory")
    void SetCooldownProgress(float Progress01);

    /** 컨테이너가 생성 시 주입 — EquippedSkills 인덱스와 일치시킴 */
    UFUNCTION(BlueprintCallable, Category = "Skill Inventory")
    void SetSlotIndex(int32 InIndex);

    UFUNCTION(BlueprintPure, Category = "Skill Inventory")
    int32 GetSlotIndex() const { return SlotIndex; }

    /** 컨테이너가 구독해서 실제 SwapSlots() 호출을 담당 */
    UPROPERTY(BlueprintAssignable, Category = "Skill Inventory")
    FOnSlotDropped OnSlotDropped;

protected:
    UPROPERTY(meta = (BindWidget))
    class UImage* SkillIcon;

    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* SkillName;

    /** 빈 슬롯일 때 표시할 오버레이 (예: 반투명 "+" 아이콘). 없으면 그냥 스킵. */
    UPROPERTY(meta = (BindWidgetOptional))
    class UWidget* EmptySlotOverlay;

    /** 쿨다운 중 어둡게 덮는 진행률 오버레이. 없으면 쿨다운 표시 없이 그냥 스킵됨. */
    UPROPERTY(meta = (BindWidgetOptional))
    class UProgressBar* CooldownOverlay;

    /** 단축키 표시("1"~"6"). 없으면 스킵. */
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* HotkeyLabel;

    /** 쿨다운 완료 순간 재생할 반짝임 연출. WBP에서 HotkeyLabel 대상으로 짧게(0.2~0.3초) 제작.
    *  없으면 그냥 스킵 — 반짝임 없이 조용히 넘어감. */
    UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
    class UWidgetAnimation* ReadyFlashAnim;

    UPROPERTY(BlueprintReadOnly)
    int32 SlotIndex = -1;

    UPROPERTY(BlueprintReadOnly)
    bool bIsOccupied = false;

protected:

    //==========================================================================
    // DRAG & DROP — Ctrl을 누른 상태에서만 활성화 (느슨한 "스킬 조정 모드")
    //==========================================================================

    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, class UDragDropOperation*& OutOperation) override;
    virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, class UDragDropOperation* InOperation) override;

    /** 쿨다운 완료 순간(0→1 전이) 감지용. 드래그 가드에도 같이 사용. */
    bool bIsReady = true;
};
