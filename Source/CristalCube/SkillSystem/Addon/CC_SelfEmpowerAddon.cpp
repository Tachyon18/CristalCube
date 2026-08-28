// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_SelfEmpowerAddon.h"
#include "../../Gameplay/CC_SkillEmpowerComponent.h"

void UCC_SelfEmpowerAddon::OnCast_Implementation(UCC_SkillSystem* SkillSystem, const FSkillDefinition& Skill, FSkillExecutionContext& Context)
{
    if (!Context.Caster) return;

    UCC_SkillEmpowerComponent* EmpowerComp = Context.Caster->FindComponentByClass<UCC_SkillEmpowerComponent>();
    if (!EmpowerComp) return;

    const float Multiplier = EmpowerComp->AddStackAndGetMultiplier(Skill.SkillID, Data.DamagePerStack, Data.MaxStacks, Data.StackDuration);
    Context.CurrentDamage *= Multiplier;
}

void UCC_SelfEmpowerAddon::ApplyModifier_Implementation(FName AttributeID, float ValuePerPoint)
{
    if (AttributeID == TEXT("SelfEmpowerDamagePerStack"))
    {
        Data.DamagePerStack += ValuePerPoint;
    }
    else if (AttributeID == TEXT("SelfEmpowerMaxStacks"))
    {
        Data.MaxStacks += FMath::RoundToInt(ValuePerPoint);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[SelfEmpowerAddon] ApplyModifier: unknown AttributeID '%s'"), *AttributeID.ToString());
    }
}
