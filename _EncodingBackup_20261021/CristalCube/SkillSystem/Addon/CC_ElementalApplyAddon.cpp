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

    ElementalComp->ApplyElementWithData(Skill.ElementType, Data.StackAmount, Data.MaxStacks, Data.Duration, Context.Caster,
        Data.ApplyEffect, Data.BlueData, Data.GreenData);

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
    else if (AttributeID == TEXT("ElementalApplyBlueSlow"))
    {
        // Blue = tempo reduction: one point moves move/attack tempo together (negative value = stronger)
        Data.BlueData.SlowMultiplier = FMath::Clamp(Data.BlueData.SlowMultiplier + ValuePerPoint,
            FCCElementalLimits::BlueMultiplierMin, FCCElementalLimits::BlueMultiplierMax);
        Data.BlueData.AttackSpeedMultiplier = FMath::Clamp(Data.BlueData.AttackSpeedMultiplier + ValuePerPoint,
            FCCElementalLimits::BlueMultiplierMin, FCCElementalLimits::BlueMultiplierMax);
    }
    else if (AttributeID == TEXT("ElementalApplyGreenIncoming"))
    {
        Data.GreenData.IncomingDamageMultiplier = FMath::Clamp(Data.GreenData.IncomingDamageMultiplier + ValuePerPoint,
            FCCElementalLimits::GreenIncomingMin, FCCElementalLimits::GreenIncomingMax);
    }
    else if (AttributeID == TEXT("ElementalApplyGreenOutgoing"))
    {
        Data.GreenData.OutgoingDamageMultiplier = FMath::Clamp(Data.GreenData.OutgoingDamageMultiplier + ValuePerPoint,
            FCCElementalLimits::GreenReductionMin, FCCElementalLimits::GreenReductionMax);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[ElementalApplyAddon] ApplyModifier: unknown AttributeID '%s'"), *AttributeID.ToString());
    }
}
