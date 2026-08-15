// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_DamageOverTimeAddon.h"
#include "../CC_SkillSystem.h"
#include "../../Gameplay/CC_StatusEffectComponent.h"

void UCC_DamageOverTimeAddon::OnHit_Implementation(UCC_SkillSystem* SkillSystem, const FSkillDefinition& Skill, FSkillExecutionContext& Context, AActor* HitTarget, FVector HitLocation)
{
    if (!IsValidHitTarget(HitTarget)) return;

    UCC_StatusEffectComponent* StatusComp = GetStatusComponent(HitTarget);
    if (!StatusComp) return;

    StatusComp->ApplyDamageOverTime(Skill.SkillID, Data.TickDamage, Data.TotalDuration,
        Data.TickInterval, Context.Caster, Data.bStackable, Data.TickEffect);
}
