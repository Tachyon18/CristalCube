// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_SkillLibrarySubsystem.h"
#include "CC_SkillBase.h"

void UCC_SkillLibrarySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UCC_SkillLibrarySubsystem::Deinitialize()
{
    Super::Deinitialize();
}

void UCC_SkillLibrarySubsystem::LoadSkillDataTable(UDataTable* DataTable)
{
    SkillDataTable = DataTable;
}

bool UCC_SkillLibrarySubsystem::GetSkillDisplayData(FName SkillRowName, FSkillDisplayData& OutData)
{
    FSkillTableRow* Row = GetSkillRowPtr(SkillRowName);
    if (!Row)
    {
        return false;
    }

    if (!Row->SkillClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SkillLibrary] Row '%s' has no SkillClass set"), *SkillRowName.ToString());
        return false;
    }

    const UCC_SkillBase* CDO = Row->SkillClass->GetDefaultObject<UCC_SkillBase>();
    if (!CDO)
    {
        return false;
    }

    FillFromDefinition(CDO->GetDefinition(), OutData);

    // DataTable 쪽 UI 메타로 덮어씀
    OutData.Icon = Row->Icon;
    OutData.Description = Row->Description;
    OutData.SkillClass = Row->SkillClass;

    return true;
}

TArray<FSkillDisplayData> UCC_SkillLibrarySubsystem::GetAllSkillDisplayData()
{
    TArray<FSkillDisplayData> Result;

    if (!SkillDataTable)
    {
        return Result;
    }

    TArray<FName> RowNames = SkillDataTable->GetRowNames();
    Result.Reserve(RowNames.Num());

    for (const FName& RowName : RowNames)
    {
        FSkillDisplayData Data;
        if (GetSkillDisplayData(RowName, Data))
        {
            Result.Add(Data);
        }
    }

    return Result;
}

TArray<FName> UCC_SkillLibrarySubsystem::GetAllSkillRowNames()
{
    if (!SkillDataTable)
    {
        return {};
    }
    return SkillDataTable->GetRowNames();
}

TArray<FName> UCC_SkillLibrarySubsystem::GetStartingSkillRowNames()
{
    TArray<FName> Result;

    if (!SkillDataTable)
    {
        return Result;
    }

    TArray<FSkillTableRow*> AllRows;
    SkillDataTable->GetAllRows<FSkillTableRow>(TEXT("GetStartingSkillRowNames"), AllRows);
    TArray<FName> AllNames = SkillDataTable->GetRowNames();

    for (int32 i = 0; i < AllRows.Num(); ++i)
    {
        if (AllRows.IsValidIndex(i) && AllRows[i]->bIsStartingSkill)
        {
            Result.Add(AllNames[i]);
        }
    }

    return Result;
}

TArray<FName> UCC_SkillLibrarySubsystem::GetRandomUnownedSkillNames(const TArray<FName>& OwnedSkillIDs, int32 Count)
{
    TArray<FName> Result;
    if (!SkillDataTable || Count <= 0) return Result;

    TArray<FSkillTableRow*> AllRows;
    SkillDataTable->GetAllRows<FSkillTableRow>(TEXT("GetRandomUnownedSkillNames"), AllRows);
    TArray<FName> AllNames = SkillDataTable->GetRowNames();

    // 미보유 스킬만 후보 풀로 추림
    TArray<FName> PoolNames;
    TArray<FSkillTableRow*> PoolRows;
    for (int32 i = 0; i < AllRows.Num(); ++i)
    {
        if (!AllRows.IsValidIndex(i)) continue;
        if (OwnedSkillIDs.Contains(AllNames[i])) continue;

        PoolNames.Add(AllNames[i]);
        PoolRows.Add(AllRows[i]);
    }

    Count = FMath::Min(Count, PoolNames.Num());

    for (int32 Picked = 0; Picked < Count; ++Picked)
    {
        float TotalWeight = 0.0f;
        for (FSkillTableRow* Row : PoolRows)
        {
            if (Row) TotalWeight += Row->DropWeight;
        }
        if (TotalWeight <= 0.0f) break;

        float RandomValue = FMath::FRandRange(0.0f, TotalWeight);
        float CurrentWeight = 0.0f;
        int32 SelectedIndex = 0;

        for (int32 i = 0; i < PoolRows.Num(); ++i)
        {
            if (!PoolRows[i]) continue;
            CurrentWeight += PoolRows[i]->DropWeight;
            if (RandomValue <= CurrentWeight)
            {
                SelectedIndex = i;
                break;
            }
        }

        Result.Add(PoolNames[SelectedIndex]);
        PoolNames.RemoveAt(SelectedIndex);
        PoolRows.RemoveAt(SelectedIndex);
    }

    return Result;
}

FSkillTableRow* UCC_SkillLibrarySubsystem::GetSkillRowPtr(FName SkillRowName)
{
    if (!SkillDataTable)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SkillLibrary] SkillDataTable not set"));
        return nullptr;
    }

    return SkillDataTable->FindRow<FSkillTableRow>(SkillRowName, TEXT("GetSkillRowPtr"));

}

void UCC_SkillLibrarySubsystem::FillFromDefinition(const FSkillDefinition& Def, FSkillDisplayData& OutData) const
{
    OutData.SkillID = Def.SkillID;
    OutData.DisplayName = Def.DisplayName;
    OutData.BaseDamage = Def.BaseDamage;
    OutData.Cooldown = Def.Cooldown;
    OutData.CoreType = Def.CoreType;
    OutData.ElementType = Def.ElementType;
    OutData.Addons = Def.Addons;
}

void UCC_SkillLibrarySubsystem::LoadAddonDataTable(UDataTable* DataTable)
{
    AddonDataTable = DataTable;
}

bool UCC_SkillLibrarySubsystem::GetAddonDisplayData(FName AddonRowName, FAddonTableRow& OutData)
{
    if (!AddonDataTable) return false;

    if (FAddonTableRow* Row = AddonDataTable->FindRow<FAddonTableRow>(AddonRowName, TEXT("GetAddonDisplayData")))
    {
        OutData = *Row;
        return true;
    }
    return false;
}

bool UCC_SkillLibrarySubsystem::GetAddonDisplayDataByType(ESkillAddonType AddonType, FAddonTableRow& OutData)
{
    if (!AddonDataTable || AddonType == ESkillAddonType::None) return false;

    TArray<FAddonTableRow*> AllRows;
    AddonDataTable->GetAllRows<FAddonTableRow>(TEXT("GetAddonDisplayDataByType"), AllRows);

    for (FAddonTableRow* Row : AllRows)
    {
        if (Row && Row->AddonType == AddonType)
        {
            OutData = *Row;
            return true;
        }
    }
    return false;
}

TArray<FAddonTableRow> UCC_SkillLibrarySubsystem::GetAllAddonDisplayData()
{
    TArray<FAddonTableRow> Result;
    if (!AddonDataTable) return Result;

    TArray<FAddonTableRow*> AllRows;
    AddonDataTable->GetAllRows<FAddonTableRow>(TEXT("GetAllAddonDisplayData"), AllRows);
    for (FAddonTableRow* Row : AllRows)
    {
        if (Row) Result.Add(*Row);
    }
    return Result;
}

TArray<FAddonTableRow> UCC_SkillLibrarySubsystem::GetImplementedAddonDisplayData()
{
    TArray<FAddonTableRow> Result = GetAllAddonDisplayData();
    Result.RemoveAll([](const FAddonTableRow& Row) { return !Row.bImplemented; });
    return Result;
}

TArray<FAddonTableRow> UCC_SkillLibrarySubsystem::GetAddonBadgesForSkill(const FSkillDisplayData& SkillData)
{
    TArray<FAddonTableRow> Result;

    for (ESkillAddonType Addon : SkillData.Addons)
    {
        FAddonTableRow Row;
        if (GetAddonDisplayDataByType(Addon, Row) && Row.bImplemented)
        {
            Result.Add(Row);
        }
    }

    return Result;
}

