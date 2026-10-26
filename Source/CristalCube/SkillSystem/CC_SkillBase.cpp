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

void UCC_SkillBase::SpendAddonAttributePoint(ESkillAddonType AddonType, FName AttributeID, float ValuePerPoint)
{
    bool bFound = false;

    for (UCC_SkillAddonBase* Addon : SkillDef.Addons)
    {
        if (Addon && Addon->AddonType == AddonType)
        {
            Addon->ApplyModifier(AttributeID, ValuePerPoint);
            bFound = true;
        }
    }

    if (!bFound)
    {
        UE_LOG(LogTemp, Warning, TEXT("[%s] SpendAddonAttributePoint: addon %s not present, ignored"),
            *SkillDef.SkillID.ToString(), *UEnum::GetValueAsString(AddonType));
    }
}
