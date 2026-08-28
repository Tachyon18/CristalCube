// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_PlayerState.h"
#include "SkillSystem/CC_SkillBase.h"
#include "SkillSystem/CC_SkillSystem.h"
#include "SkillSystem/CC_SkillLibrarySubsystem.h"
#include "SkillSystem/CC_AddonPresetAsset.h"
#include "CristalCubeStruct.h"
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

	UE_LOG(LogTemp, Log, TEXT("GrantSkill: Granting skill %s to slot %d"), *SkillClass->GetName(), EmptySlot);

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

	NewSkill->ResolveAddons();

    // 이 스킬에 지정된 프리셋들을 캐치업까지 포함해서 resolve — 스킬 인스턴스가 처음 태어나는
    // 이 시점에 한 번만 실행됨(재장착 시엔 애초에 새 인스턴스가 만들어지므로 다시 여기로 옴).

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

    EquipStartingSkills();
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

void ACC_PlayerState::EquipStartingSkills()
{
    if (!SkillLibrary)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayerState] EquipStartingSkills: SkillLibrary not initialized"));
        return;
    }

    TArray<FName> StartingNames = SkillLibrary->GetStartingSkillRowNames();
    if (StartingNames.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayerState] EquipStartingSkills: no bIsStartingSkill rows found in DT_Skill"));
        return;
    }

    for (const FName& RowName : StartingNames)
    {
        GrantSkillByRowName(RowName);
    }

    UE_LOG(LogTemp, Log, TEXT("[PlayerState] EquipStartingSkills: %d starting skill(s) granted"), StartingNames.Num());

}

bool ACC_PlayerState::SwapSlots(int32 SlotA, int32 SlotB)
{
    if (SlotA == SlotB || !EquippedSkills.IsValidIndex(SlotA) || !EquippedSkills.IsValidIndex(SlotB))
    {
        return false;
    }

    EquippedSkills.Swap(SlotA, SlotB);   // 둘 다 비어있어도 안전 (nullptr swap)
    OnSkillsChanged.Broadcast();
    return true;
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
        if (IsValid(Skill) && Skill->IsReady() && Skill->GetDefinition().bAutoCast)
        {
            Skill->TryCast(SkillSystem, TargetLocation);
        }
    }
}

void ACC_PlayerState::AddCubeEnergy(float Amount)
{
    if (Amount <= 0.f || CubeEnergyGaugeMax <= 0.f) return;

    CubeEnergyCurrent += Amount;

    while (CubeEnergyCurrent >= CubeEnergyGaugeMax)
    {
        CubeEnergyCurrent -= CubeEnergyGaugeMax;
        ++PendingBonusPicks;
    }

    UE_LOG(LogTemp, Log, TEXT("[PlayerState] CubeEnergy +%.1f (Current: %.1f/%.1f, PendingPicks: %d)"),
        Amount, CubeEnergyCurrent, CubeEnergyGaugeMax, PendingBonusPicks);
}

int32 ACC_PlayerState::ConsumeCubeClearPicks()
{
    const int32 Total = 1 + PendingBonusPicks;  // Cube Clear당 최소 1장 보장
    PendingBonusPicks = 0;
    return Total;
}

void ACC_PlayerState::AddAddonPoints(ESkillAddonType AddonType, int32 Amount)
{
    if (AddonType == ESkillAddonType::None || Amount <= 0) return;

    int32& Points = AddonUnspentPoints.FindOrAdd(AddonType);
    Points += Amount;

    UE_LOG(LogTemp, Log, TEXT("[PlayerState] Addon points +%d for %s (total: %d)"),
        Amount, *UEnum::GetValueAsString(AddonType), Points);
}

int32 ACC_PlayerState::GetAddonUnspentPoints(ESkillAddonType AddonType) const
{
    const int32* Points = AddonUnspentPoints.Find(AddonType);
    return Points ? *Points : 0;
}

bool ACC_PlayerState::SpendAddonPoint(ESkillAddonType AddonType, FName AttributeID, float ValuePerPoint, int32 MaxPoints)
{
    if (AddonType == ESkillAddonType::None || AttributeID.IsNone()) return false;

    const int32 Bank = GetAddonUnspentPoints(AddonType);
    if (Bank <= 0) return false;

    const int32 AlreadySpent = GetAddonAttributeSpentPoints(AddonType, AttributeID);
    if (MaxPoints > 0 && AlreadySpent >= MaxPoints) return false;

    // 은행 차감
    AddonUnspentPoints[AddonType] = Bank - 1;

    // 스펜트 기록 누적 (따라잡기용)
    FAddonAttributeSpentPoints& SpentEntry = AddonSpentPoints.FindOrAdd(AddonType);
    int32& SpentCount = SpentEntry.Points.FindOrAdd(AttributeID);
    ++SpentCount;

    // 현재 이 Addon을 보유 중인 모든 장착 스킬에 즉시 반영
    for (UCC_SkillBase* Skill : EquippedSkills)
    {
        if (IsValid(Skill) && Skill->HasAddon(AddonType))
        {
            Skill->SpendAddonAttributePoint(AddonType, AttributeID, ValuePerPoint);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[PlayerState] Addon point spent: %s / %s (now %d, bank %d left)"),
        *UEnum::GetValueAsString(AddonType), *AttributeID.ToString(), SpentCount, AddonUnspentPoints[AddonType]);

    return true;
}

bool ACC_PlayerState::RefundAddonPoint(ESkillAddonType AddonType, FName AttributeID, float ValuePerPoint)
{
    if (AddonType == ESkillAddonType::None || AttributeID.IsNone()) return false;

    FAddonAttributeSpentPoints* SpentEntry = AddonSpentPoints.Find(AddonType);
    if (!SpentEntry) return false;

    int32* SpentCount = SpentEntry->Points.Find(AttributeID);
    if (!SpentCount || *SpentCount <= 0) return false;

    // 스펜트 기록 차감
    --(*SpentCount);

    // 은행으로 반환
    int32& Points = AddonUnspentPoints.FindOrAdd(AddonType);
    ++Points;

    // 현재 이 Addon을 보유 중인 모든 장착 스킬에 역산 반영
    for (UCC_SkillBase* Skill : EquippedSkills)
    {
        if (IsValid(Skill) && Skill->HasAddon(AddonType))
        {
            Skill->SpendAddonAttributePoint(AddonType, AttributeID, -ValuePerPoint);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[PlayerState] Addon point refunded: %s / %s (now %d, bank %d)"),
        *UEnum::GetValueAsString(AddonType), *AttributeID.ToString(), *SpentCount, Points);

    return true;
}

int32 ACC_PlayerState::GetAddonAttributeSpentPoints(ESkillAddonType AddonType, FName AttributeID) const
{
    const FAddonAttributeSpentPoints* Entry = AddonSpentPoints.Find(AddonType);
    if (!Entry) return 0;

    const int32* Count = Entry->Points.Find(AttributeID);
    return Count ? *Count : 0;
}

bool ACC_PlayerState::GrantAddonWithCatchUp(UCC_SkillBase* Skill, ESkillAddonType AddonType)
{
    if (!Skill) return false;

    // 1) 이 스킬 고유의 카탈로그에서 같은 타입 검색 — "지정한 Addon"
    UCC_AddonPresetAsset* ResolvedPreset = nullptr;
    for (UCC_AddonPresetAsset* Candidate : Skill->GetDefinition().AddonPresets)
    {
        if (Candidate && Candidate->AddonType == AddonType)
        {
            ResolvedPreset = Candidate;
            break;
        }
    }

    // 2) 없으면 DT_Addon의 DefaultPreset — "Default Addon" (스킬 무관 전역)
    FAddonTableRow AddonRow;
    const bool bHasRow = SkillLibrary && SkillLibrary->GetAddonDisplayDataByType(AddonType, AddonRow);

    if (!ResolvedPreset && bHasRow)
    {
        ResolvedPreset = AddonRow.DefaultPreset;
    }

    // 3) 둘 다 없으면 ResolvedPreset은 nullptr로 남고, GrantAddon 내부의 순정 NewObject 폴백으로 감
    if (!Skill->GrantAddon(AddonType, ResolvedPreset))
    {
        return false;
    }

    const FAddonAttributeSpentPoints* SpentEntry = AddonSpentPoints.Find(AddonType);
    if (!SpentEntry || SpentEntry->Points.Num() == 0)
    {
        return true;
    }

    if (!bHasRow)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayerState] GrantAddonWithCatchUp: addon row not found for %s, catch-up skipped"),
            *UEnum::GetValueAsString(AddonType));
        return true;
    }

    for (const TPair<FName, int32>& Pair : SpentEntry->Points)
    {
        const FAddonUpgradeAttribute* AttrDef = AddonRow.UpgradeAttributes.FindByPredicate(
            [&Pair](const FAddonUpgradeAttribute& A) { return A.AttributeID == Pair.Key; });

        if (AttrDef)
        {
            Skill->SpendAddonAttributePoint(AddonType, Pair.Key, AttrDef->ValuePerPoint * Pair.Value);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[PlayerState] GrantAddonWithCatchUp: %s caught up on %s (%d attribute(s))"),
        *Skill->GetSkillID().ToString(), *UEnum::GetValueAsString(AddonType), SpentEntry->Points.Num());

    return true;
}

void ACC_PlayerState::AddSkillCorePoints(FName SkillID, int32 Amount)
{
    if (SkillID.IsNone() || Amount <= 0) return;

    int32& Points = SkillCoreUnspentPoints.FindOrAdd(SkillID);
    Points += Amount;

    UE_LOG(LogTemp, Log, TEXT("[PlayerState] Skill core points +%d for %s (total: %d)"),
        Amount, *SkillID.ToString(), Points);
}

int32 ACC_PlayerState::GetSkillCoreUnspentPoints(FName SkillID) const
{
    const int32* Points = SkillCoreUnspentPoints.Find(SkillID);
    return Points ? *Points : 0;
}
