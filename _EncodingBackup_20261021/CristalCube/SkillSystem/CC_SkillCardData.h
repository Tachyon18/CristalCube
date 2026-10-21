// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "../CristalCubeStruct.h"
#include "CC_SkillLibrarySubsystem.h"
#include "CC_SkillCardData.generated.h"

/**
 * 
 */
UCLASS()
class CRISTALCUBE_API UCC_SkillCardData : public UObject
{
	GENERATED_BODY()
	
public:
    UPROPERTY(BlueprintReadOnly, Category = "Skill Card")
    FSkillDisplayData SkillData;

    /** GetAddonBadgesForSkill() 결과를 미리 캐싱 — 카드 위젯은 서브시스템을 몰라도 됨 */
    UPROPERTY(BlueprintReadOnly, Category = "Skill Card")
    TArray<FAddonTableRow> AddonBadges;
};
