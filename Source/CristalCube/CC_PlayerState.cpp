// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_PlayerState.h"
#include "SkillSystem/CC_SkillBase.h"
#include "SkillSystem/CC_SkillSystem.h"
#include "SkillSystem/CC_SkillLibrarySubsystem.h"
#include "CC_LogHelper.h"

ACC_PlayerState::ACC_PlayerState()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ACC_PlayerState::BeginPlay()
{
	Super::BeginPlay();

    EquippedSkills.Init(nullptr, MaxSkillSlots);

    InitializeSkillLibrary();
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
    const int32 EmptySlot = FindFirstEmptySlot();
    if (EmptySlot == -1)
    {
        UE_LOG(LogTemp, Warning, TEXT("GrantSkill: Skill slots full (Max %d)"), MaxSkillSlots);
        return nullptr;
    }

    return GrantSkillToSlot(SkillClass, EmptySlot);
}

bool ACC_PlayerState::RemoveSkill(FName SkillID)
{
    for (int32 i = 0; i < EquippedSkills.Num(); ++i)
    {
        if (IsValid(EquippedSkills[i]) && EquippedSkills[i]->GetSkillID() == SkillID)
        {
            return RemoveSkillAtSlot(i);   // 실제 로직은 슬롯 버전에 위임 (Broadcast 중복 호출 없음)
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

    // Empty() 대신 크기를 유지한 채 전부 nullptr로 — 슬롯 배열 크기 불변 원칙
    EquippedSkills.Init(nullptr, MaxSkillSlots);

    OnSkillsChanged.Broadcast();
}

UCC_SkillBase* ACC_PlayerState::GrantSkillToSlot(TSubclassOf<UCC_SkillBase> SkillClass, int32 SlotIndex)
{
    if (!SkillClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("GrantSkillToSlot: SkillClass is null"));
        return nullptr;
    }

    if (!EquippedSkills.IsValidIndex(SlotIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("GrantSkillToSlot: SlotIndex %d out of range (Max %d)"),
            SlotIndex, MaxSkillSlots);
        return nullptr;
    }

    if (IsValid(EquippedSkills[SlotIndex]))
    {
        UE_LOG(LogTemp, Warning, TEXT("GrantSkillToSlot: Slot %d already occupied by [%s]"),
            SlotIndex, *EquippedSkills[SlotIndex]->GetSkillID().ToString());
        return nullptr;
    }

    // 중복 체크 (SkillID 기준) — 기존 GrantSkill과 동일한 정책 유지
    UCC_SkillBase* TempDefault = SkillClass->GetDefaultObject<UCC_SkillBase>();
    if (TempDefault && HasSkill(TempDefault->GetSkillID()))
    {
        UE_LOG(LogTemp, Warning, TEXT("GrantSkillToSlot: Skill [%s] already equipped"),
            *TempDefault->GetSkillID().ToString());
        return nullptr;
    }

    UCC_SkillBase* NewSkill = NewObject<UCC_SkillBase>(this, SkillClass);
    if (!NewSkill)
    {
        return nullptr;
    }

    EquippedSkills[SlotIndex] = NewSkill;
    NewSkill->OnEquipped(GetPawn());

    UE_LOG(LogTemp, Log, TEXT("GrantSkillToSlot: [%s] added at slot %d"),
        *NewSkill->GetSkillID().ToString(), SlotIndex);

    OnSkillsChanged.Broadcast();

    return NewSkill;
}

bool ACC_PlayerState::RemoveSkillAtSlot(int32 SlotIndex)
{
    if (!EquippedSkills.IsValidIndex(SlotIndex) || !IsValid(EquippedSkills[SlotIndex]))
    {
        return false;
    }

    EquippedSkills[SlotIndex]->OnUnequipped();
    EquippedSkills[SlotIndex] = nullptr;   // RemoveAt 대신 자리만 비움 — 다른 슬롯 인덱스 유지

    UE_LOG(LogTemp, Log, TEXT("RemoveSkillAtSlot: slot %d cleared"), SlotIndex);

    OnSkillsChanged.Broadcast();
    return true;
}

UCC_SkillBase* ACC_PlayerState::GetSkillAtSlot(int32 SlotIndex) const
{
    return EquippedSkills.IsValidIndex(SlotIndex) ? EquippedSkills[SlotIndex] : nullptr;
}

int32 ACC_PlayerState::FindFirstEmptySlot() const
{
    for (int32 i = 0; i < EquippedSkills.Num(); ++i)
    {
        if (!IsValid(EquippedSkills[i]))
        {
            return i;
        }
    }
    return -1;
}

void ACC_PlayerState::InitializeSkillLibrary()
{
    UGameInstance* GameInstance = GetGameInstance();
    if (!GameInstance)
    {
        UE_LOG(LogTemp, Error, TEXT("[PlayerState] Cannot init SkillLibrary - GameInstance null"));
        return;
    }

    SkillLibrary = GameInstance->GetSubsystem<UCC_SkillLibrarySubsystem>();
    if (!SkillLibrary)
    {
        UE_LOG(LogTemp, Error, TEXT("[PlayerState] Cannot init SkillLibrary - Subsystem null"));
        return;
    }

    if (SkillDataTable)
    {
        SkillLibrary->LoadSkillDataTable(SkillDataTable);
        UE_LOG(LogTemp, Log, TEXT("[PlayerState] Skill Library Initialized"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayerState] No SkillDataTable set"));
    }

    if (AddonDataTable)
    {
        SkillLibrary->LoadAddonDataTable(AddonDataTable);
    }
}

UCC_SkillBase* ACC_PlayerState::GrantSkillByRowName(FName SkillRowName)
{
    if (!SkillLibrary)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayerState] GrantSkillByRowName: SkillLibrary not initialized"));
        return nullptr;
    }

    FSkillTableRow* Row = SkillLibrary->GetSkillRowPtr(SkillRowName);
    if (!Row || !Row->SkillClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayerState] GrantSkillByRowName: '%s' not found or SkillClass unset"),
            *SkillRowName.ToString());
        return nullptr;
    }

    return GrantSkill(Row->SkillClass);
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

bool ACC_PlayerState::CanGrantMoreSkills() const
{
    return FindFirstEmptySlot() != -1;
}

int32 ACC_PlayerState::GetSkillCount() const
{
    int32 Count = 0;
    for (const UCC_SkillBase* Skill : EquippedSkills)
    {
        if (IsValid(Skill))
        {
            ++Count;
        }
    }
    return Count;
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
