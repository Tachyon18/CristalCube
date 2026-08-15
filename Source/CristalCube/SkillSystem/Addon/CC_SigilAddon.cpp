// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_SigilAddon.h"
#include "../CC_SkillSystem.h"
#include "../CC_SigilEffector.h"

void UCC_SigilAddon::OnHit_Implementation(UCC_SkillSystem* SkillSystem, const FSkillDefinition& Skill, FSkillExecutionContext& Context, AActor* HitTarget, FVector HitLocation)
{
    if (!SkillSystem || !SkillSystem->GetWorld()) return;

    ACC_SigilEffector* Sigil = SkillSystem->GetWorld()->SpawnActor<ACC_SigilEffector>(HitLocation, FRotator::ZeroRotator);
    if (Sigil)
    {
        Sigil->Initialize(HitLocation, Context.Caster, Data);
    }
}
