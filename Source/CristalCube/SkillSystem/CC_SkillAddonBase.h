// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CC_SkillAddonBase.generated.h"

class UCC_SkillSystem;
/**
 * 
 */
UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced)
class CRISTALCUBE_API UCC_SkillAddonBase : public UObject
{
	GENERATED_BODY()
	
public:

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Addon")
    FName AddonID = NAME_None;

    // ProcessAddons()가 이 Addon 실행 직전에 채워줌 — Skill.Addons 배열 내 자기 인덱스.
    // 스스로 새 히트를 만들어 후속 Addon을 재귀 처리하고 싶을 때(Chain의 다음 타격,
    // Homing의 신규 발사체 명중 등) SkillSystem->ProcessAddons(..., AddonIndex + 1)로
    // "나보다 배열상 뒤에 있는 Addon만" 연쇄시켜 순환(무한 재귀)을 원천 차단한다.
    UPROPERTY()
    int32 AddonIndex = 0;

    // 피격 시점 발동 — Explosion/Chain/DamageOverTime류
    UFUNCTION(BlueprintNativeEvent, Category = "Addon")
    void OnHit(UCC_SkillSystem* SkillSystem, const FSkillDefinition& Skill,
        FSkillExecutionContext& Context, AActor* HitTarget, FVector HitLocation);
    virtual void OnHit_Implementation(UCC_SkillSystem* SkillSystem, const FSkillDefinition& Skill,
        FSkillExecutionContext& Context, AActor* HitTarget, FVector HitLocation) {
    }

    // 시전 시점 발동 — Echo/SelfEmpower/Homing(추가 발사체 스폰)류
    UFUNCTION(BlueprintNativeEvent, Category = "Addon")
    void OnCast(UCC_SkillSystem* SkillSystem, const FSkillDefinition& Skill, FSkillExecutionContext& Context);
    virtual void OnCast_Implementation(UCC_SkillSystem* SkillSystem, const FSkillDefinition& Skill, FSkillExecutionContext& Context) {}

    // 강화 포인트 배분 시 수치 누적
    UFUNCTION(BlueprintNativeEvent, Category = "Addon")
    void ApplyModifier(const UCC_SkillAddonBase* Modifier);
    virtual void ApplyModifier_Implementation(const UCC_SkillAddonBase* Modifier) {}

protected:

    // 검증 — Explosion이든 DamageOverTime이든, "이 대상에게 효과를 가해도 되는가"는 같은 기준
    // (기존 FindEnemiesInRadius/OnOverlapBegin이 쓰던 것과 동일한 필터를 여기 하나로 통일)
    UFUNCTION(BlueprintCallable, Category = "Addon")
    bool IsValidHitTarget(AActor* Target) const;

    // 적용 — 상태 부여형 Addon(DamageOverTime, 향후 ElementalApply 등)이 공용으로 쓰는 접근 지점
    UFUNCTION(BlueprintCallable, Category = "Addon")
    class UCC_StatusEffectComponent* GetStatusComponent(AActor* Target) const;

};
