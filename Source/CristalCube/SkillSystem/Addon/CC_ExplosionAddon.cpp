// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_ExplosionAddon.h"
#include "../CC_SkillSystem.h"

void UCC_ExplosionAddon::OnHit_Implementation(UCC_SkillSystem* SkillSystem, const FSkillDefinition& Skill, FSkillExecutionContext& Context, AActor* HitTarget, FVector HitLocation)
{
    if (!SkillSystem) return;

    for (AActor* Enemy : SkillSystem->FindEnemiesInRadius(HitLocation, Data.Radius))
    {
        if (!IsValidHitTarget(Enemy) || Context.HitActors.Contains(Enemy)) continue;

        float DistanceRatio = FMath::Clamp(FVector::Dist(HitLocation, Enemy->GetActorLocation()) / Data.Radius, 0.0f, 1.0f);
        float DamageMultiplier = FMath::Lerp(1.0f, Data.MinDamageRatio, DistanceRatio);

        SkillSystem->ApplyDamage(Enemy, Context.CurrentDamage * DamageMultiplier, Context.Caster);
        Context.HitActors.Add(Enemy);
    }

    if (Data.ExplosionEffect)
    {
        SkillSystem->SpawnEffect(Data.ExplosionEffect, HitLocation);
    }
}
