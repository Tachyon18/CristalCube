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
	const int32 SelfIndex = AddonIndex;  // 자기 자신부터 재처리 — MaxEchoes 반복과 순서 기반 재적용을 동시에 만족

    FTimerHandle EchoTimer;
    SkillSystem->GetWorld()->GetTimerManager().SetTimer(EchoTimer,
        FTimerDelegate::CreateLambda([WeakSystem, SkillCopy, ContextCopy, EchoTargetLocation, SelfIndex]()
            {
                if (WeakSystem.IsValid())
                {
                    WeakSystem->ExecuteSkillWithContext(SkillCopy, ContextCopy, EchoTargetLocation, SelfIndex);
                }
            }),
        Delay, false);
}

void UCC_EchoAddon::ApplyModifier_Implementation(FName AttributeID, float ValuePerPoint)
{
    if (AttributeID == TEXT("EchoDamageRatio"))
    {
        Data.DamageRatio = FMath::Max(Data.DamageRatio + ValuePerPoint, 0.f);
    }
    else if (AttributeID == TEXT("EchoMaxEchoes"))
    {
        Data.MaxEchoes += FMath::RoundToInt(ValuePerPoint);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[EchoAddon] ApplyModifier: unknown AttributeID '%s'"), *AttributeID.ToString());
    }
}
