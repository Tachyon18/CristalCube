// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "CC_RosterCardHeaderButton.generated.h"

class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHeaderSlotDropped, int32, SourceSlotIndex, int32, TargetSlotIndex);

/**
 * WBP_SkillRosterEntry의 CardHeaderButton 전용 서브클래스.
 * 기본 클릭(아코디언 펼침/접힘)은 UButton 그대로 두고, Ctrl+좌클릭일 때만
 * CC_SkillSlotWidget과 동일한 패턴(느슨한 "스킬 조정 모드")으로 드래그를 가로챈다.
 * Ctrl을 안 누르면 Super로 흘려보내 기존 OnClicked 흐름(펼침/접힘)을 그대로 유지.
 * 빈 슬롯(bIsOccupied=false)에서는 드래그 "시작"만 막고, 드롭 수신은 항상 허용한다.
 */
UCLASS()
class CRISTALCUBE_API UCC_RosterCardHeaderButton : public UButton
{
	GENERATED_BODY()
	
public:
	/** 이 카드의 슬롯 인덱스/아이콘/점유 여부. Owner가 SetSkillData()에서 매번 갱신. */
	UFUNCTION(BlueprintCallable, Category = "Skill Roster")
	void SetDragPayload(int32 InSlotIndex, UTexture2D* InIconForDrag, bool bInIsOccupied);

	/** 부모(CC_SkillRosterEntryWidget)가 구독 — 실제 SwapSlots 호출은 그 위(Inventory)가 담당 */
	UPROPERTY(BlueprintAssignable, Category = "Skill Roster")
	FOnHeaderSlotDropped OnHeaderSlotDropped;

protected:


private:
	int32 SlotIndex = INDEX_NONE;
	bool bIsOccupied = false;

	UPROPERTY()
	UTexture2D* IconForDrag = nullptr;
};
