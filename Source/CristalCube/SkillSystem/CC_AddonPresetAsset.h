// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "../CristalCubeStruct.h"
#include "CC_AddonPresetAsset.generated.h"

/**
 * Addon 배리에이션 하나 = 애셋 하나 (예: DA_Explosion_Fire, DA_Explosion_Thunder).
 * Content Browser에서 우클릭 > Miscellaneous > Data Asset > UCC_AddonPresetAsset으로 생성.
 * AddonType은 Template의 실제 클래스와 일치해야 함(예: Template이 UCC_ExplosionAddon이면
 * AddonType도 Explosion) — GrantAddon()/ResolveAddons()가 이 값으로 매칭하므로 어긋나면 무시됨.
 */
UCLASS()
class CRISTALCUBE_API UCC_AddonPresetAsset : public UDataAsset
{
	GENERATED_BODY()

public:
    // 이 프리셋이 어떤 Addon 타입용인지
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Addon Preset")
    ESkillAddonType AddonType = ESkillAddonType::None;

    // 에디터 표시용 라벨(로직에는 안 쓰임)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Addon Preset")
    FText PresetLabel;

    // 실제 복제 원본 — Radius/Effect 등 여기서 인라인 편집. UDataAsset은 UCLASS라
    // Details 패널에서 Instanced 서브오브젝트 편집이 정상 동작함.
    UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Addon Preset")
    UCC_SkillAddonBase* Template = nullptr;
};
