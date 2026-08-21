// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "../CristalCubeStruct.h"
#include "CC_AddonUpgradeEntryData.generated.h"

/**
 * 
 */
UCLASS()
class CRISTALCUBE_API UCC_AddonUpgradeEntryData : public UObject
{
	GENERATED_BODY()
	
public:
    UPROPERTY(BlueprintReadOnly, Category = "Addon Upgrade")
    ESkillAddonType AddonType = ESkillAddonType::None;

    UPROPERTY(BlueprintReadOnly, Category = "Addon Upgrade")
    FAddonTableRow AddonRow;   // DisplayName/Icon/UpgradeAttributes 전부 포함

    UPROPERTY(BlueprintReadOnly, Category = "Addon Upgrade")
    int32 UnspentPoints = 0;   // 이 Addon 타입의 현재 은행 잔액

};
