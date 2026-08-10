// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_ChainAddon.h"
#include "../CC_SkillSystem.h"

void UCC_ChainAddon::OnHit_Implementation(UCC_SkillSystem* SkillSystem, const FSkillDefinition& Skill, FSkillExecutionContext& Context, AActor* HitTarget, FVector HitLocation)
{
    if (!SkillSystem || Context.CurrentChainCount >= Data.ChainCount) return;

    FVector SearchOrigin = HitTarget ? HitTarget->GetActorLocation() : Context.StartLocation;
    AActor* NextTarget = SkillSystem->FindNearestEnemy(SearchOrigin, Data.SearchRadius, Context.HitActors);
    if (!NextTarget || !IsValidHitTarget(NextTarget)) return;

    Context.CurrentChainCount++;
    Context.HitActors.Add(NextTarget);
    SkillSystem->ApplyDamage(NextTarget, Context.CurrentDamage, Context.Caster);

    UE_LOG(LogTemp, Warning, TEXT("[ChainDebug] Hop %d -> %s | ChainEffect valid: %s"),
        Context.CurrentChainCount, *NextTarget->GetName(), Data.ChainEffect ? TEXT("YES") : TEXT("NO"));

    if (Data.ChainEffect)
    {
        SkillSystem->SpawnChainEffect(Data.ChainEffect, SearchOrigin, NextTarget->GetActorLocation());
        UE_LOG(LogTemp, Warning, TEXT("[ChainDebug] SpawnChainEffect called"));
    }

    FHitResult ChainHit;
    ChainHit.ImpactPoint = NextTarget->GetActorLocation();
    ChainHit.HitObjectHandle = FActorInstanceHandle(NextTarget);
    SkillSystem->ProcessAddons(Skill, Context, ChainHit);
}
