// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_EchoAddon.h"
#include "../CC_SkillSystem.h"

void UCC_EchoAddon::OnCast_Implementation(UCC_SkillSystem* SkillSystem, const FSkillDefinition& Skill, FSkillExecutionContext& Context)
{
    if (!SkillSystem || !SkillSystem->GetWorld()) return;
    if (Context.CurrentEchoCount >= Data.MaxEchoes) return;

    FSkillDefinition SkillCopy = Skill;
    FSkillExecutionContext ContextCopy = Context;
    ContextCopy.CurrentEchoCount++;
    ContextCopy.CurrentDamage = Context.CurrentDamage * Data.DamageRatio;
    ContextCopy.HitActors.Reset();  // 새 시전이니 이전 명중 목록은 비움 (같은 적 재타격 허용)

    TWeakObjectPtr<UCC_SkillSystem> WeakSystem = SkillSystem;
    FVector EchoTargetLocation = Context.TargetLocation;
    float Delay = Data.EchoDelay;

    FTimerHandle EchoTimer;
    SkillSystem->GetWorld()->GetTimerManager().SetTimer(EchoTimer,
        FTimerDelegate::CreateLambda([WeakSystem, SkillCopy, ContextCopy, EchoTargetLocation]()
            {
                if (WeakSystem.IsValid())
                {
                    WeakSystem->ExecuteSkillWithContext(SkillCopy, ContextCopy, EchoTargetLocation);
                }
            }),
        Delay, false);
}
