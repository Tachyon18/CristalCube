// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_SkillBase.h"
#include "CC_SkillSystem.h"
#include "CC_AddonPresetAsset.h"
#include "Addon/CC_ExplosionAddon.h"
#include "Addon/CC_ChainAddon.h"
#include "Addon/CC_ShockwaveAddon.h"
#include "Addon/CC_MagicMissileAddon.h"
#include "Addon/CC_ElementalApplyAddon.h"
#include "Addon/CC_ElementalBurstAddon.h"
#include "Addon/CC_DamageOverTimeAddon.h"
#include "Addon/CC_SigilAddon.h"
#include "Addon/CC_EchoAddon.h"
#include "Addon/CC_SelfEmpowerAddon.h"
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

    // 쿨다운 시작 — CooldownMultiplier 반영 (1.0 미만이면 쿨다운 감소 = 강화)
    const float EffectiveCooldown = FMath::Max(SkillDef.Cooldown * SkillDef.Passives.CooldownMultiplier, 0.0f);
    CooldownRemaining = EffectiveCooldown;

    UE_LOG(LogTemp, Log, TEXT("[%s] Cast! Cooldown: %.1fs"), *SkillDef.SkillID.ToString(), EffectiveCooldown);
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
    const float EffectiveCooldown = SkillDef.Cooldown * SkillDef.Passives.CooldownMultiplier;

    if (EffectiveCooldown <= 0.0f)
    {
        return 1.0f; // 쿨다운 없는 스킬은 항상 준비된 상태
    }

    return 1.0f - (CooldownRemaining / EffectiveCooldown);
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

bool UCC_SkillBase::GrantAddon(ESkillAddonType AddonType, UCC_AddonPresetAsset* Preset)
{
    if (AddonType == ESkillAddonType::None) return false;
    if (HasAddon(AddonType)) return false;

    UCC_SkillAddonBase* NewAddon = nullptr;

    // 프리셋이 있고 타입이 맞으면 그 Template을 복제 — Radius/Effect 등 전부 그대로 승계.
    if (Preset && Preset->AddonType == AddonType && Preset->Template)
    {
        NewAddon = DuplicateObject<UCC_SkillAddonBase>(Preset->Template, this);
    }
    
    if(!NewAddon)
    {
        switch (AddonType)
        {
        case ESkillAddonType::Explosion:      NewAddon = NewObject<UCC_ExplosionAddon>(this);      break;
        case ESkillAddonType::Chain:          NewAddon = NewObject<UCC_ChainAddon>(this);          break;
        case ESkillAddonType::Shockwave:      NewAddon = NewObject<UCC_ShockwaveAddon>(this);      break;
        case ESkillAddonType::MagicMissile:   NewAddon = NewObject<UCC_MagicMissileAddon>(this);   break;
        case ESkillAddonType::ElementalApply: NewAddon = NewObject<UCC_ElementalApplyAddon>(this); break;
        case ESkillAddonType::ElementalBurst: NewAddon = NewObject<UCC_ElementalBurstAddon>(this); break;
        case ESkillAddonType::DamageOverTime: NewAddon = NewObject<UCC_DamageOverTimeAddon>(this); break;
        case ESkillAddonType::Sigil:          NewAddon = NewObject<UCC_SigilAddon>(this);          break;
        case ESkillAddonType::Echo:           NewAddon = NewObject<UCC_EchoAddon>(this);           break;
        case ESkillAddonType::SelfEmpower:    NewAddon = NewObject<UCC_SelfEmpowerAddon>(this);    break;
        default:
            // Penetrate / MultiShot은 Projectile Core 고유 강화 — 포인트제 Addon 카탈로그 대상 아님(STATUS.md 참고)
            UE_LOG(LogTemp, Warning, TEXT("[%s] GrantAddon: %s is not a point-allocatable addon"),
                *SkillDef.SkillID.ToString(), *UEnum::GetValueAsString(AddonType));
            return false;
        }
    }

    if (!NewAddon) return false;

    SkillDef.Addons.Add(NewAddon);

    UE_LOG(LogTemp, Log, TEXT("[%s] Addon granted: %s"),
        *SkillDef.SkillID.ToString(), *UEnum::GetValueAsString(AddonType));
    return true;
}

void UCC_SkillBase::ResolveAddons()
{
    for (UCC_SkillAddonBase*& Addon : SkillDef.Addons)
    {
        if (!Addon) continue;

        for (UCC_AddonPresetAsset* Preset : SkillDef.AddonPresets)
        {
            if (Preset && Preset->AddonType == Addon->AddonType && Preset->Template)
            {
                Addon = DuplicateObject<UCC_SkillAddonBase>(Preset->Template, this);
                break;
            }
        }
    }
}

void UCC_SkillBase::SpendCoreAttributePoint(ECoreUpgradeAttribute AttributeType, float ValuePerPoint)
{
    switch (AttributeType)
    {
    case ECoreUpgradeAttribute::Damage:
        SkillDef.Passives.DamageMultiplier += ValuePerPoint;
        break;
    case ECoreUpgradeAttribute::Cooldown:
        SkillDef.Passives.CooldownMultiplier += ValuePerPoint;
        break;
    case ECoreUpgradeAttribute::Range:
        SkillDef.Passives.RangeMultiplier += ValuePerPoint;
        break;
    case ECoreUpgradeAttribute::Area:
        SkillDef.Passives.AreaMultiplier += ValuePerPoint;
        break;
    }
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

void UCC_SkillBase::SpendAddonAttributePoint(ESkillAddonType AddonType, FName AttributeID, float ValuePerPoint)
{
    // Modifier의 Multiplier류 기본값(1.0)은 ApplyAddonModifier가 안 건드리는 필드라 안전함 —
    // Addon Data 필드만 델타로 채워서 넘긴다.
    //FSkillPassiveProperties Modifier;
    //bool bMatched = true;
    //
    //switch (AddonType)
    //{
    //case ESkillAddonType::Explosion:
    //    if (AttributeID == TEXT("ExplosionRadius"))            Modifier.ExplosionData.Radius = ValuePerPoint;
    //    else if (AttributeID == TEXT("ExplosionMinDamageRatio")) Modifier.ExplosionData.MinDamageRatio = ValuePerPoint;
    //    else bMatched = false;
    //    break;
    //case ESkillAddonType::Chain:
    //    if (AttributeID == TEXT("ChainCount"))            Modifier.ChainData.ChainCount = FMath::RoundToInt(ValuePerPoint);
    //    else if (AttributeID == TEXT("ChainDamageDecay"))   Modifier.ChainData.DamageDecay = ValuePerPoint;
    //    else if (AttributeID == TEXT("ChainSearchRadius"))  Modifier.ChainData.SearchRadius = ValuePerPoint;
    //    else bMatched = false;
    //    break;
    //case ESkillAddonType::Shockwave:
    //    if (AttributeID == TEXT("ShockwaveMaxRadius"))        Modifier.ShockwaveData.MaxRadius = ValuePerPoint;
    //    else if (AttributeID == TEXT("ShockwaveRingThickness")) Modifier.ShockwaveData.RingThickness = ValuePerPoint;
    //    else bMatched = false;
    //    break;
    //case ESkillAddonType::MagicMissile:
    //    if (AttributeID == TEXT("MagicMissileLaunchCount"))   Modifier.MagicMissileData.LaunchCount = FMath::RoundToInt(ValuePerPoint);
    //    else if (AttributeID == TEXT("MagicMissileDamageRatio")) Modifier.MagicMissileData.DamageRatio = ValuePerPoint;
    //    else bMatched = false;
    //    break;
    //case ESkillAddonType::ElementalApply:
    //    if (AttributeID == TEXT("ElementalApplyStackAmount")) Modifier.ElementalApplyData.StackAmount = FMath::RoundToInt(ValuePerPoint);
    //    else if (AttributeID == TEXT("ElementalApplyDuration")) Modifier.ElementalApplyData.Duration = ValuePerPoint;
    //    else bMatched = false;
    //    break;
    //case ESkillAddonType::ElementalBurst:
    //    if (AttributeID == TEXT("ElementalBurstDamagePerStack")) Modifier.ElementalBurstData.DamagePerStack = ValuePerPoint;
    //    else bMatched = false;
    //    break;
    //case ESkillAddonType::DamageOverTime:
    //    if (AttributeID == TEXT("DamageOverTimeTickDamage"))     Modifier.DamageOverTimeData.TickDamage = ValuePerPoint;
    //    else if (AttributeID == TEXT("DamageOverTimeTotalDuration")) Modifier.DamageOverTimeData.TotalDuration = ValuePerPoint;
    //    else bMatched = false;
    //    break;
    //case ESkillAddonType::Sigil:
    //    if (AttributeID == TEXT("SigilRadius"))      Modifier.SigilData.Radius = ValuePerPoint;
    //    else if (AttributeID == TEXT("SigilTickDamage")) Modifier.SigilData.TickDamage = ValuePerPoint;
    //    else bMatched = false;
    //    break;
    //case ESkillAddonType::Echo:
    //    if (AttributeID == TEXT("EchoDamageRatio"))  Modifier.EchoData.DamageRatio = ValuePerPoint;
    //    else if (AttributeID == TEXT("EchoMaxEchoes")) Modifier.EchoData.MaxEchoes = FMath::RoundToInt(ValuePerPoint);
    //    else bMatched = false;
    //    break;
    //case ESkillAddonType::SelfEmpower:
    //    if (AttributeID == TEXT("SelfEmpowerDamagePerStack")) Modifier.SelfEmpowerData.DamagePerStack = ValuePerPoint;
    //    else if (AttributeID == TEXT("SelfEmpowerMaxStacks"))   Modifier.SelfEmpowerData.MaxStacks = FMath::RoundToInt(ValuePerPoint);
    //    else bMatched = false;
    //    break;
    //default:
    //    bMatched = false;
    //    break;
    //}
    //
    //if (!bMatched)
    //{
    //    UE_LOG(LogTemp, Warning, TEXT("[%s] SpendAddonAttributePoint: unknown AttributeID '%s' for addon %s"),
    //        *SkillDef.SkillID.ToString(), *AttributeID.ToString(), *UEnum::GetValueAsString(AddonType));
    //    return;
    //}
    //
    //ApplyAddonModifier(AddonType, Modifier);

    bool bFound = false;

    for (UCC_SkillAddonBase* Addon : SkillDef.Addons)
    {
        if (Addon && Addon->AddonType == AddonType)
        {
            Addon->ApplyModifier(AttributeID, ValuePerPoint);
            bFound = true;
        }
    }
}
