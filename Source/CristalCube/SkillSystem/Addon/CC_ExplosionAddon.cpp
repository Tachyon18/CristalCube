// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_ExplosionAddon.h"
#include "../CC_SkillSystem.h"

void UCC_ExplosionAddon::OnHit_Implementation(UCC_SkillSystem* SkillSystem, const FSkillDefinition& Skill, FSkillExecutionContext& Context, AActor* HitTarget, FVector HitLocation)
{
    if (!SkillSystem) return;

    const int32 NextIndex = AddonIndex + 1;
    // 원본 Context는 건드리지 않고, 이번 폭발 범위 내 중복 판정만 로컬로 관리
    TArray<AActor*> AlreadyExploded = Context.HitActors;

    for (AActor* Enemy : SkillSystem->FindEnemiesInRadius(HitLocation, Data.Radius))
    {
        if (!IsValidHitTarget(Enemy) || AlreadyExploded.Contains(Enemy)) continue;

        // Max damage inside Radius * MaxDamageRadiusRatio, then linear falloff to MinDamageRatio at Radius
        const float Dist = FVector::Dist(HitLocation, Enemy->GetActorLocation());
        const float MaxRadius = Data.Radius * FMath::Clamp(Data.MaxDamageRadiusRatio, 0.0f, 1.0f);
        const float FalloffRange = Data.Radius - MaxRadius;
        const float DistanceRatio = (FalloffRange > KINDA_SMALL_NUMBER)
            ? FMath::Clamp((Dist - MaxRadius) / FalloffRange, 0.0f, 1.0f)
            : 0.0f;
        const float DamageMultiplier = FMath::Lerp(1.0f, Data.MinDamageRatio, DistanceRatio);
        const float ActualDamage = Context.CurrentDamage * DamageMultiplier;

        SkillSystem->ApplyDamage(Enemy, ActualDamage, Context.Caster);
        AlreadyExploded.Add(Enemy);

        // 이 폭발로 새로 맞은 적 하나하나가 '개별 타격' — 독립된 Context로 하위 Addon에 전달
        FSkillExecutionContext BranchContext = Context;
        BranchContext.HitActors = AlreadyExploded;
        BranchContext.CurrentChainCount = 0;
        BranchContext.CurrentDamage = ActualDamage;

        FHitResult ExplosionHit;
        ExplosionHit.ImpactPoint = Enemy->GetActorLocation();
        ExplosionHit.HitObjectHandle = FActorInstanceHandle(Enemy);
        SkillSystem->ProcessAddons(Skill, BranchContext, ExplosionHit, NextIndex);
    }

    if (Data.ExplosionEffect)
    {
        SkillSystem->SpawnEffect(Data.ExplosionEffect, HitLocation, FRotator::ZeroRotator, Skill.ElementType);
    }
}

void UCC_ExplosionAddon::ApplyModifier_Implementation(FName AttributeID, float ValuePerPoint)
{
    if (AttributeID == TEXT("ExplosionRadius"))
    {
        Data.Radius = FMath::Max(Data.Radius + ValuePerPoint, 0.0f);
    }
    else if (AttributeID == TEXT("ExplosionMinDamageRatio"))
    {
        Data.MinDamageRatio = FMath::Clamp(Data.MinDamageRatio + ValuePerPoint, 0.f, 1.f);
    }
    else if (AttributeID == TEXT("ExplosionMaxDamageRadiusRatio"))
    {
        Data.MaxDamageRadiusRatio = FMath::Clamp(Data.MaxDamageRadiusRatio + ValuePerPoint, 0.0f, 1.0f);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[ExplosionAddon] ApplyModifier: unknown AttributeID '%s'"), *AttributeID.ToString());
    }
}
