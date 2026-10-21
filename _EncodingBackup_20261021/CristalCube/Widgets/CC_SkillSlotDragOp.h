// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "CC_SkillSlotDragOp.generated.h"

/**
 * Skill Inventory 슬롯 간 드래그 앤 드롭 시 오가는 페이로드.
 * 원본 슬롯 인덱스만 들고 있으면 됨 — 실제 교체는 SwapSlots()가 처리.
 */
UCLASS()
class CRISTALCUBE_API UCC_SkillSlotDragOp : public UDragDropOperation
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "Skill Inventory")
	int32 SourceSlotIndex = -1;
	
};
