// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_ElementalBurstAddon.h"
#include "../CC_SkillSystem.h"
#include "../../Gameplay/CC_ElementalStatusComponent.h"

void UCC_ElementalBurstAddon::OnHit_Implementation(UCC_SkillSystem* SkillSystem, const FSkillDefinition& Skill, FSkillExecutionContext& Context, AActor* HitTarget, FVector HitLocation)
{
    if (!SkillSystem || !IsValidHitTarget(HitTarget)) return;

    UCC_ElementalStatusComponent* ElementalComp = HitTarget->FindComponentByClass<UCC_ElementalStatusComponent>();

    int32 StackCount = 1;
    bool bFoundElement = ElementalComp && ElementalComp->GetActiveElement(Skill.ElementType, StackCount);

    if (!bFoundElement && Data.bRequireExistingElement)
    {
        return;  // 조건 없음 = "걸려있어야만" 발동. 지금은 이 모드만 사용.
    }

    const float BurstDamage = Data.DamagePerStack * StackCount;
    SkillSystem->ApplyDamage(HitTarget, BurstDamage, Context.Caster);

    if (Data.BurstEffect)
    {
        SkillSystem->SpawnEffect(Data.BurstEffect, HitLocation);
    }

    if (Data.bConsumeOnBurst && bFoundElement && ElementalComp)
    {
        ElementalComp->ConsumeElement(Skill.ElementType);
    }
}

void UCC_ElementalBurstAddon::ApplyModifier_Implementation(FName AttributeID, float ValuePerPoint)
{
    if (AttributeID == TEXT("ElementalBurstDamagePerStack"))
    {
        Data.DamagePerStack += ValuePerPoint;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[ElementalBurstAddon] ApplyModifier: unknown AttributeID '%s'"), *AttributeID.ToString());
    }
}
