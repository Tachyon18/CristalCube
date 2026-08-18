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
    //
    // [일반 원칙] 한 타격(Core 히트든, Addon이 만들어낸 새 타격이든)이 발생하면
    // 그 타격 전용 Context 사본을 만들어 SkillSystem->ProcessAddons(..., AddonIndex + 1)로
    // "나보다 배열상 뒤에 있는 Addon"에게 위임한다(Chain의 매 홉, Explosion의 각 피격 대상,
    // Sigil의 매 틱당 각 피격 대상 등 — 개별 타격 하나하나가 하위 Addon으로의 델리게이트).
    // 이 Context 사본은 아래로만 흐르며, 원본/형제 타격 계열의 Context로 다시 병합(merge-back)
    // 하지 않는다 — Context가 막는 건 "같은 타격 계열 내 같은 적 중복 판정"뿐, 전역 중복 방지가 아니다.
    // 자기 자신을 반복 호출하는 Addon(Chain 등)은 재귀/루프 동안 원본 Context를 직접 변경하지 말고
    // 로컬 사본에서만 상태(카운트, 데미지 감쇠)를 누적해 부모 프레임으로 새어나가지 않게 한다.

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
