// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_SkillBase.h"
#include "CC_SkillSystem.h"
#include "../CC_LogHelper.h"

UCC_SkillBase::UCC_SkillBase()
{
}

bool UCC_SkillBase::TryCast(UCC_SkillSystem* SkillSystem, FVector TargetLocation)
{
    if (!SkillSystem)
    {
        UE_LOG(LogTemp, Warning, TEXT("[%s] TryCast: SkillSystem is null"), *SkillDef.SkillID.ToString());
        return false;
    }

    if (!IsReady())
    {
        UE_LOG(LogTemp, Log, TEXT("[%s] TryCast: On cooldown (%.1fs remaining)"),
            *SkillDef.SkillID.ToString(), CooldownRemaining);
        return false;
    }

    SkillSystem->ExecuteSkill(SkillDef, TargetLocation);

    // 쿨다운 시작
    CooldownRemaining = SkillDef.Cooldown;

    UE_LOG(LogTemp, Log, TEXT("[%s] Cast! Cooldown: %.1fs"), *SkillDef.SkillID.ToString(), SkillDef.Cooldown);
    return true;
}

void UCC_SkillBase::TickCooldown(float DeltaTime)
{
    if (CooldownRemaining > 0.0f)
    {
        CooldownRemaining = FMath::Max(0.0f, CooldownRemaining - DeltaTime);
    }
}

float UCC_SkillBase::GetCooldownProgress() const
{
    if (SkillDef.Cooldown <= 0.0f)
    {
        return 1.0f; // 쿨다운 없는 스킬은 항상 준비된 상태
    }

    return 1.0f - (CooldownRemaining / SkillDef.Cooldown);
}

void UCC_SkillBase::OnEquipped(AActor* InOwner)
{
    SkillOwner = InOwner;
    CooldownRemaining = 0.0f;  // 장착 시 즉시 사용 가능
    UE_LOG(LogTemp, Log, TEXT("[%s] Equipped on %s"),
        *SkillDef.SkillID.ToString(), InOwner ? *InOwner->GetName() : TEXT("None"));

}

void UCC_SkillBase::OnUnequipped()
{
    UE_LOG(LogTemp, Log, TEXT("[%s] Unequipped"), *SkillDef.SkillID.ToString());
    SkillOwner = nullptr;
}

void UCC_SkillBase::ApplyPassiveModifier(const FSkillPassiveProperties& Modifier)
{
    // 배율 누적 (곱셈)
    SkillDef.Passives.DamageMultiplier *= Modifier.DamageMultiplier;
    SkillDef.Passives.SizeMultiplier *= Modifier.SizeMultiplier;
    SkillDef.Passives.SpeedMultiplier *= Modifier.SpeedMultiplier;
    SkillDef.Passives.ProjectileCount += Modifier.ProjectileCount - 1;  // 1이 기본이므로 초과분만 누적

    UE_LOG(LogTemp, Log, TEXT("[%s] Passive modifier applied. DmgMul: %.2f"),
        *SkillDef.SkillID.ToString(), SkillDef.Passives.DamageMultiplier);
}