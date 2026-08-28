// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_ElementalApplyAddon.h"
#include "../CC_SkillSystem.h"
#include "../../Gameplay/CC_ElementalStatusComponent.h"

void UCC_ElementalApplyAddon::OnHit_Implementation(UCC_SkillSystem* SkillSystem, const FSkillDefinition& Skill, FSkillExecutionContext& Context, AActor* HitTarget, FVector HitLocation)
{
    if (!IsValidHitTarget(HitTarget)) return;
    if (Skill.ElementType == ESkillElementType::None) return;

    UCC_ElementalStatusComponent* ElementalComp = HitTarget->FindComponentByClass<UCC_ElementalStatusComponent>();
    if (!ElementalComp) return;

    ElementalComp->ApplyElement(Skill.ElementType, Data.StackAmount, Data.MaxStacks, Data.Duration, Context.Caster, Data.ApplyEffect);

    // Red 전용 — 속성 부여 시점에 추가 데미지 1회 적용
    if (Skill.ElementType == ESkillElementType::Red && SkillSystem)
    {   
        const float ProcDamage = Data.RedData.FlatDamage + Data.RedData.SkillDamageRatio * Skill.BaseDamage;
        SkillSystem->ApplyDamage(HitTarget, ProcDamage, Context.Caster);
    }
}

void UCC_ElementalApplyAddon::ApplyModifier_Implementation(FName AttributeID, float ValuePerPoint)
{
    if (AttributeID == TEXT("ElementalApplyStackAmount"))
    {
        Data.StackAmount += FMath::RoundToInt(ValuePerPoint);
    }
    else if (AttributeID == TEXT("ElementalApplyDuration"))
    {
        Data.Duration += ValuePerPoint;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[ElementalApplyAddon] ApplyModifier: unknown AttributeID '%s'"), *AttributeID.ToString());
    }
}
