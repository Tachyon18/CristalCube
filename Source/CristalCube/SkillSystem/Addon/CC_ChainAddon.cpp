// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_ChainAddon.h"
#include "../CC_SkillSystem.h"

void UCC_ChainAddon::OnHit_Implementation(UCC_SkillSystem* SkillSystem, const FSkillDefinition& Skill, FSkillExecutionContext& Context, AActor* HitTarget, FVector HitLocation)
{
    if (!SkillSystem) return;

    // 체인 전용 로컬 상태 — 원본 Context(Context 매개변수)는 절대 변경하지 않는다.
    // 이렇게 해야 이 체인 시퀀스에서 쌓인 ChainCount/데미지 감쇠가
    // 같은 원본 히트에서 파생되는 다른 형제 Addon(예: 원본 히트 자체의 Explosion)에 새어나가지 않는다.
    FSkillExecutionContext ChainState = Context;
    AActor* CurrentTarget = HitTarget;

    while (ChainState.CurrentChainCount < Data.ChainCount)
    {
        FVector SearchOrigin = CurrentTarget ? CurrentTarget->GetActorLocation() : ChainState.StartLocation;
        AActor* NextTarget = SkillSystem->FindNearestEnemy(SearchOrigin, Data.SearchRadius, ChainState.HitActors);
        if (!NextTarget || !IsValidHitTarget(NextTarget)) break;

        ChainState.CurrentChainCount++;
        ChainState.HitActors.Add(NextTarget);
        SkillSystem->ApplyDamage(NextTarget, ChainState.CurrentDamage, ChainState.Caster);

        UE_LOG(LogTemp, Warning, TEXT("[ChainDebug] Hop %d -> %s | ChainEffect valid: %s"),
            ChainState.CurrentChainCount, *NextTarget->GetName(), Data.ChainEffect ? TEXT("YES") : TEXT("NO"));

        if (Data.ChainEffect)
        {
            SkillSystem->SpawnChainEffect(Data.ChainEffect, SearchOrigin, NextTarget->GetActorLocation());
        }

        FHitResult ChainHit;
        ChainHit.ImpactPoint = NextTarget->GetActorLocation();
        ChainHit.HitObjectHandle = FActorInstanceHandle(NextTarget);

        // 이번 홉 자체가 '개별 타격' — 이 홉 전용 사본으로 하위 Addon(Explosion 등)에 전달.
        // BranchContext는 여기서만 쓰고 버림 (ChainState나 원본 Context로 병합하지 않음)
        FSkillExecutionContext BranchContext = ChainState;
        SkillSystem->ProcessAddons(Skill, BranchContext, ChainHit, AddonIndex + 1);

        // 다음 홉을 위해 데미지 감쇠 — ChainState(로컬)에만 반영
        ChainState.CurrentDamage *= Data.DamageDecay;
        CurrentTarget = NextTarget;
    }
}

void UCC_ChainAddon::ApplyModifier_Implementation(FName AttributeID, float ValuePerPoint)
{
    if (AttributeID == TEXT("ChainCount"))
    {
        Data.ChainCount += FMath::RoundToInt(ValuePerPoint);
    }
    else if (AttributeID == TEXT("ChainDamageDecay"))
    {
        Data.DamageDecay = FMath::Clamp(Data.DamageDecay + ValuePerPoint, 0.f, 1.f);
    }
    else if (AttributeID == TEXT("ChainSearchRadius"))
    {
        Data.SearchRadius += ValuePerPoint;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[ChainAddon] ApplyModifier: unknown AttributeID '%s'"), *AttributeID.ToString());
    }
}
