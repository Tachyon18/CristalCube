// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "../CristalCubeStruct.h"
#include "CC_SkillLibrarySubsystem.generated.h"

/**
 * 조회 전용 스킬 카탈로그.
 * - DT_Skill(FSkillTableRow)에서 UI 메타데이터(Icon/Description/획득 정보)를 읽고
 * - 스킬 클래스의 CDO에서 게임플레이 스탯(FSkillDefinition)을 읽어
 * 하나로 합쳐서 UI에 바로 꽂을 수 있는 형태로 제공한다.
 * 실행(ExecuteSkill 등)은 절대 다루지 않음 — 그건 UCC_SkillSystem의 영역.
 */

 // UI에서 바로 쓰기 좋은 합성 데이터 — DataTable 메타 + CDO 스탯
USTRUCT(BlueprintType)
struct FSkillDisplayData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Skill|Display")
    FName SkillID = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category = "Skill|Display")
    FText DisplayName;

    UPROPERTY(BlueprintReadOnly, Category = "Skill|Display")
    FText Description;

    UPROPERTY(BlueprintReadOnly, Category = "Skill|Display")
    UTexture2D* Icon = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Skill|Display")
    float BaseDamage = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Skill|Display")
    float Cooldown = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Skill|Display")
    ESkillCoreType CoreType = ESkillCoreType::None;

    UPROPERTY(BlueprintReadOnly, Category = "Skill|Display")
    ESkillElementType ElementType = ESkillElementType::None;

    UPROPERTY(BlueprintReadOnly, Category = "Skill|Display")
    TArray<ESkillAddonType> Addons;

    UPROPERTY(BlueprintReadOnly, Category = "Skill|Display")
    TSubclassOf<class UCC_SkillBase> SkillClass;
};

UCLASS(Blueprintable)
class CRISTALCUBE_API UCC_SkillLibrarySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    UDataTable* SkillDataTable = nullptr;

public:
    UFUNCTION(BlueprintCallable, Category = "Skill Library")
    void LoadSkillDataTable(UDataTable* DataTable);

    /** RowName(=SkillID 권장)로 UI용 합성 데이터 조회. 실패 시 false. */
    UFUNCTION(BlueprintCallable, Category = "Skill Library")
    bool GetSkillDisplayData(FName SkillRowName, FSkillDisplayData& OutData);

    /** DataTable에 등록된 전체 스킬을 UI용 데이터로 한 번에 조회 (백과사전용) */
    UFUNCTION(BlueprintCallable, Category = "Skill Library")
    TArray<FSkillDisplayData> GetAllSkillDisplayData();

    UFUNCTION(BlueprintCallable, Category = "Skill Library")
    TArray<FName> GetAllSkillRowNames();

    UFUNCTION(BlueprintCallable, Category = "Skill Library")
    TArray<FName> GetStartingSkillRowNames();

    /** 지정한 SkillID 목록(보유 스킬)을 제외한 나머지에서 DropWeight 가중 랜덤 N개(중복 없이) 추출.
     *  LevelUp SkillGrant 후보 생성용. */
    UFUNCTION(BlueprintCallable, Category = "Skill Library")
    TArray<FName> GetRandomUnownedSkillNames(const TArray<FName>& OwnedSkillIDs, int32 Count);

    FSkillTableRow* GetSkillRowPtr(FName SkillRowName);

    /** SkillID의 DT_Skill 로우에서 CoreUpgradeAttributes 전체 조회. 로우 없으면 빈 배열. */
    UFUNCTION(BlueprintCallable, Category = "Skill Library")
    TArray<FCoreUpgradeAttribute> GetSkillCoreUpgradeAttributes(FName SkillID);

    /** 위 배열에서 AttributeID 하나만 찾음. 못 찾으면 false. */
    UFUNCTION(BlueprintCallable, Category = "Skill Library")
    bool GetSkillCoreUpgradeAttribute(FName SkillID, ECoreUpgradeAttribute AttributeType, FCoreUpgradeAttribute& OutAttribute);

protected:
    /** SkillClass의 CDO에서 FSkillDefinition을 읽어 OutData에 채움 (Icon/Description 제외) */
    void FillFromDefinition(const FSkillDefinition& Def, FSkillDisplayData& OutData) const;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    UDataTable* AddonDataTable = nullptr;

public:
    UFUNCTION(BlueprintCallable, Category = "Skill Library")
    void LoadAddonDataTable(UDataTable* DataTable);

    /** RowName으로 조회 (백과사전에서 전체 나열할 때) */
    UFUNCTION(BlueprintCallable, Category = "Skill Library")
    bool GetAddonDisplayData(FName AddonRowName, FAddonTableRow& OutData);

    /** 실행 중인 스킬의 Addons 배열에서 실제 표시 데이터를 끌어올 때 (AddonType으로 역matching) */
    UFUNCTION(BlueprintCallable, Category = "Skill Library")
    bool GetAddonDisplayDataByType(ESkillAddonType AddonType, FAddonTableRow& OutData);

    UFUNCTION(BlueprintCallable, Category = "Skill Library")
    TArray<FAddonTableRow> GetAllAddonDisplayData();

    /** bImplemented=true인 것만 (게임플레이 중 실제로 나올 수 있는 Addon만) */
    UFUNCTION(BlueprintCallable, Category = "Skill Library")
    TArray<FAddonTableRow> GetImplementedAddonDisplayData();

public:

    /** 스킬 하나의 Addons 배열을 통째로 넘겨서, 구현된 것만 골라 표시 데이터로 변환.
     *  위젯은 이 함수 하나만 호출하면 되고, Addons 배열을 직접 순회하지 않는다.
     *  이렇게 하면 ESkillAddonType이 나중에 클래스 기반으로 바뀌어도
     *  이 함수 내부 구현만 바뀌고, 위젯 쪽 호출부는 그대로 유지된다. */
    UFUNCTION(BlueprintCallable, Category = "Skill Library")
    TArray<FAddonTableRow> GetAddonBadgesForSkill(const FSkillDisplayData& SkillData);

    /** 원소별 고정 색상 조회 — UI(뱃지/카드/게이지 등)에서 색 기준으로 바로 쓸 수 있게.
    *  값은 여기 한 곳에서만 관리 — 나중에 조합색(2차색) 추가 시에도 이 함수만 확장하면 됨. */
    UFUNCTION(BlueprintCallable, Category = "Skill Library")
    FElementColorData GetElementColor(ESkillElementType ElementType) const;
};
