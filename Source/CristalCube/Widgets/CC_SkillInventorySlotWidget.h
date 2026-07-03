// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../SkillSystem/CC_SkillLibrarySubsystem.h"
#include "CC_SkillInventorySlotWidget.generated.h"

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

protected:
    UPROPERTY(meta = (BindWidget))
    class UImage* SkillIcon;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* SkillName;

    /** 빈 슬롯일 때 표시할 오버레이 (예: 반투명 "+" 아이콘). 없으면 그냥 스킵. */
    UPROPERTY(meta = (BindWidgetOptional))
    class UWidget* EmptySlotOverlay;
};
