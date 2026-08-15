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

bool UCC_SkillBase::GrantAddon(ESkillAddonType AddonType)
{
    if (AddonType == ESkillAddonType::None) return false;
    if (SkillDef.Addons.Contains(AddonType)) return false;

    SkillDef.Addons.Add(AddonType);

    UE_LOG(LogTemp, Log, TEXT("[%s] Addon granted: %s"),
        *SkillDef.SkillID.ToString(), *UEnum::GetValueAsString(AddonType));
    return true;
}

void UCC_SkillBase::ApplyAddonModifier(ESkillAddonType AddonType, const FSkillPassiveProperties& Modifier)
{
    if (!HasAddon(AddonType))
    {
        UE_LOG(LogTemp, Warning, TEXT("[%s] ApplyAddonModifier: addon %s not present, ignored"),
            *SkillDef.SkillID.ToString(), *UEnum::GetValueAsString(AddonType));
        return;
    }

    switch (AddonType)
    {
    case ESkillAddonType::Explosion:
        SkillDef.Passives.ExplosionData.Radius += Modifier.ExplosionData.Radius;
        SkillDef.Passives.ExplosionData.MinDamageRatio = FMath::Clamp(
            SkillDef.Passives.ExplosionData.MinDamageRatio + Modifier.ExplosionData.MinDamageRatio, 0.f, 1.f);
        break;
    case ESkillAddonType::Chain:
        SkillDef.Passives.ChainData.ChainCount += Modifier.ChainData.ChainCount;
        SkillDef.Passives.ChainData.DamageDecay = FMath::Clamp(
            SkillDef.Passives.ChainData.DamageDecay + Modifier.ChainData.DamageDecay, 0.f, 1.f);
        SkillDef.Passives.ChainData.SearchRadius += Modifier.ChainData.SearchRadius;
        break;
    case ESkillAddonType::Penetrate:
        SkillDef.Passives.PierceData.PierceCount += Modifier.PierceData.PierceCount;
        break;
    case ESkillAddonType::MultiShot:
        SkillDef.Passives.MultiShotData.AdditionalCount += Modifier.MultiShotData.AdditionalCount;
        SkillDef.Passives.MultiShotData.SpreadAngle += Modifier.MultiShotData.SpreadAngle;
        break;
    case ESkillAddonType::Shockwave:
        SkillDef.Passives.ShockwaveData.MaxRadius += Modifier.ShockwaveData.MaxRadius;
        SkillDef.Passives.ShockwaveData.RingThickness += Modifier.ShockwaveData.RingThickness;
        break;
    case ESkillAddonType::MagicMissile:
		SkillDef.Passives.MagicMissileData.LaunchCount += Modifier.MagicMissileData.LaunchCount;
		SkillDef.Passives.MagicMissileData.DamageRatio += Modifier.MagicMissileData.DamageRatio;
        break;
    case ESkillAddonType::ElementalApply:
        SkillDef.Passives.ElementalApplyData.StackAmount += Modifier.ElementalApplyData.StackAmount;
        SkillDef.Passives.ElementalApplyData.Duration += Modifier.ElementalApplyData.Duration;
        break;
    case ESkillAddonType::ElementalBurst:
        SkillDef.Passives.ElementalBurstData.DamagePerStack += Modifier.ElementalBurstData.DamagePerStack;
        break;
    case ESkillAddonType::DamageOverTime:
        SkillDef.Passives.DamageOverTimeData.TickDamage += Modifier.DamageOverTimeData.TickDamage;
        SkillDef.Passives.DamageOverTimeData.TotalDuration += Modifier.DamageOverTimeData.TotalDuration;
        break;
	case ESkillAddonType::Sigil:
        SkillDef.Passives.SigilData.Radius += Modifier.SigilData.Radius;
        SkillDef.Passives.SigilData.TickDamage += Modifier.SigilData.TickDamage;
        break;
    case ESkillAddonType::Echo:
        SkillDef.Passives.EchoData.DamageRatio += Modifier.EchoData.DamageRatio;
        SkillDef.Passives.EchoData.MaxEchoes += Modifier.EchoData.MaxEchoes;
        break;
    case ESkillAddonType::SelfEmpower:
        SkillDef.Passives.SelfEmpowerData.DamagePerStack += Modifier.SelfEmpowerData.DamagePerStack;
        SkillDef.Passives.SelfEmpowerData.MaxStacks += Modifier.SelfEmpowerData.MaxStacks;
        break;
    default:
        break;
    }

    UE_LOG(LogTemp, Log, TEXT("[%s] Addon modifier applied: %s"),
        *SkillDef.SkillID.ToString(), *UEnum::GetValueAsString(AddonType));
}
