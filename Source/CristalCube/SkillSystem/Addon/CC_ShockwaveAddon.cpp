// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_ShockwaveAddon.h"
#include "../CC_SkillSystem.h"
#include "../CC_ShockwaveEffector.h"

void UCC_ShockwaveAddon::OnHit_Implementation(UCC_SkillSystem* SkillSystem, const FSkillDefinition& Skill, FSkillExecutionContext& Context, AActor* HitTarget, FVector HitLocation)
{
    if (!SkillSystem || !SkillSystem->GetWorld()) return;

    ACC_ShockwaveEffector* Wave = SkillSystem->GetWorld()->SpawnActor<ACC_ShockwaveEffector>(HitLocation, FRotator::ZeroRotator);
    if (Wave)
    {
        Wave->Initialize(HitLocation, Context.CurrentDamage, Context.Caster, Data, HitTarget);
    }
}
