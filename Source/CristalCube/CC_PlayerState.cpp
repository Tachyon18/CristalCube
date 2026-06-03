// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_PlayerState.h"
#include "SkillSystem/CC_SkillBase.h"
#include "SkillSystem/CC_SkillSystem.h"
#include "CC_LogHelper.h"

ACC_PlayerState::ACC_PlayerState()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ACC_PlayerState::BeginPlay()
{
	Super::BeginPlay();
}

void ACC_PlayerState::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 모든 스킬의 쿨다운을 매 프레임 감소
	for (UCC_SkillBase* Skill : EquippedSkills)
	{
		if (IsValid(Skill))
		{
			Skill->TickCooldown(DeltaTime);
		}
	}
}

UCC_SkillBase* ACC_PlayerState::GrantSkill(TSubclassOf<UCC_SkillBase> SkillClass)
{
    if (!SkillClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("GrantSkill: SkillClass is null"));
        return nullptr;
    }

    if (!CanGrantMoreSkills())
    {
        UE_LOG(LogTemp, Warning, TEXT("GrantSkill: Skill slots full (%d/%d)"),
            EquippedSkills.Num(), MaxSkillSlots);
        return nullptr;
    }

    // 중복 체크 (SkillID 기준)
    UCC_SkillBase* TempDefault = SkillClass->GetDefaultObject<UCC_SkillBase>();
    if (TempDefault && HasSkill(TempDefault->GetSkillID()))
    {
        UE_LOG(LogTemp, Warning, TEXT("GrantSkill: Skill [%s] already equipped"),
            *TempDefault->GetSkillID().ToString());
        return nullptr;
    }

    // Outer = this → GC가 PlayerState 수명에 귀속
    UCC_SkillBase* NewSkill = NewObject<UCC_SkillBase>(this, SkillClass);
    if (!NewSkill)
    {
        return nullptr;
    }

    EquippedSkills.Add(NewSkill);
    NewSkill->OnEquipped(GetPawn());

    UE_LOG(LogTemp, Log, TEXT("GrantSkill: [%s] added. Total: %d"),
        *NewSkill->GetSkillID().ToString(), EquippedSkills.Num());

    return NewSkill;
}

bool ACC_PlayerState::RemoveSkill(FName SkillID)
{
    for (int32 i = 0; i < EquippedSkills.Num(); ++i)
    {
        if (IsValid(EquippedSkills[i]) && EquippedSkills[i]->GetSkillID() == SkillID)
        {
            EquippedSkills[i]->OnUnequipped();
            EquippedSkills.RemoveAt(i);
            UE_LOG(LogTemp, Log, TEXT("RemoveSkill: [%s] removed"), *SkillID.ToString());
            return true;
        }
    }
    return false;
}

void ACC_PlayerState::RemoveAllSkills()
{
    for (UCC_SkillBase* Skill : EquippedSkills)
    {
        if (IsValid(Skill))
        {
            Skill->OnUnequipped();
        }
    }
    EquippedSkills.Empty();
}

UCC_SkillBase* ACC_PlayerState::FindSkill(FName SkillID) const
{
    for (UCC_SkillBase* Skill : EquippedSkills)
    {
        if (IsValid(Skill) && Skill->GetSkillID() == SkillID)
        {
            return Skill;
        }
    }
    return nullptr;
}

bool ACC_PlayerState::HasSkill(FName SkillID) const
{
    return FindSkill(SkillID) != nullptr;
}

bool ACC_PlayerState::TryCastSkill(FName SkillID, UCC_SkillSystem* SkillSystem, FVector TargetLocation)
{
    UCC_SkillBase* Skill = FindSkill(SkillID);
    if (!Skill)
    {
        return false;
    }
    return Skill->TryCast(SkillSystem, TargetLocation);
}

void ACC_PlayerState::CastAllReadySkills(UCC_SkillSystem* SkillSystem, FVector TargetLocation)
{
    if (!SkillSystem)
    {
        return;
    }

    for (UCC_SkillBase* Skill : EquippedSkills)
    {
        if (IsValid(Skill) && Skill->IsReady())
        {
            Skill->TryCast(SkillSystem, TargetLocation);
        }
    }
}
