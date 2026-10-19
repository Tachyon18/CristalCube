// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_SigilAddon.h"
#include "../CC_SkillSystem.h"
#include "../CC_SigilEffector.h"
#include "../CC_EffectorPoolSubsystem.h"

void UCC_SigilAddon::OnHit_Implementation(UCC_SkillSystem* SkillSystem, const FSkillDefinition& Skill, FSkillExecutionContext& Context, AActor* HitTarget, FVector HitLocation)
{
    if (!SkillSystem || !SkillSystem->GetWorld()) return;

    UWorld* World = SkillSystem->GetWorld();

    ACC_SigilEffector* Sigil = CC_AcquirePooledEffector<ACC_SigilEffector>(World);

    if (Sigil)
    {
        Sigil->Initialize(HitLocation, Context.Caster, Data, SkillSystem, Skill, Context, AddonIndex + 1);
    }
}

void UCC_SigilAddon::ApplyModifier_Implementation(FName AttributeID, float ValuePerPoint)
{
    if (AttributeID == TEXT("SigilRadius"))
    {
        Data.Radius += ValuePerPoint;
    }
    else if (AttributeID == TEXT("SigilTickDamage"))
    {
        Data.TickDamage += ValuePerPoint;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[SigilAddon] ApplyModifier: unknown AttributeID '%s'"), *AttributeID.ToString());
    }
}
