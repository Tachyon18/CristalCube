// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_ElementalApplyAddon.h"
#include "../../Gameplay/CC_ElementalStatusComponent.h"

void UCC_ElementalApplyAddon::OnHit_Implementation(UCC_SkillSystem* SkillSystem, const FSkillDefinition& Skill, FSkillExecutionContext& Context, AActor* HitTarget, FVector HitLocation)
{
    if (!IsValidHitTarget(HitTarget)) return;
    if (Skill.ElementType == ESkillElementType::None) return;

    UCC_ElementalStatusComponent* ElementalComp = HitTarget->FindComponentByClass<UCC_ElementalStatusComponent>();
    if (!ElementalComp) return;

    ElementalComp->ApplyElement(Skill.ElementType, Data.StackAmount, Data.MaxStacks, Data.Duration, Context.Caster, Data.ApplyEffect);

}
