// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_ShockwaveAddon.h"
#include "../CC_SkillSystem.h"
#include "../CC_ShockwaveEffector.h"
#include "../CC_EffectorPoolSubsystem.h"

void UCC_ShockwaveAddon::OnHit_Implementation(UCC_SkillSystem* SkillSystem, const FSkillDefinition& Skill, FSkillExecutionContext& Context, AActor* HitTarget, FVector HitLocation)
{
    if (!SkillSystem || !SkillSystem->GetWorld()) return;

    UWorld* World = SkillSystem->GetWorld();
    ACC_ShockwaveEffector* Wave = CC_AcquirePooledEffector<ACC_ShockwaveEffector>(World);

    if (Wave)
    {
        Wave->Initialize(HitLocation, Context.CurrentDamage, Context.Caster, Data, HitTarget,
            SkillSystem, Skill, Context, AddonIndex + 1);
    }
}

void UCC_ShockwaveAddon::ApplyModifier_Implementation(FName AttributeID, float ValuePerPoint)
{
    if (AttributeID == TEXT("ShockwaveMaxRadius"))
    {
        Data.MaxRadius += ValuePerPoint;
    }
    else if (AttributeID == TEXT("ShockwaveRingThickness"))
    {
        Data.RingThickness += ValuePerPoint;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[ShockwaveAddon] ApplyModifier: unknown AttributeID '%s'"), *AttributeID.ToString());
    }
}
