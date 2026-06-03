// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "CC_PlayerState.generated.h"

class UCC_SkillBase;
class UCC_SkillSystem;

/**
 * 플레이어의 스킬 로스터를 관리하는 PlayerState.
 * - 스킬 인스턴스의 소유자 (GC Outer)
 * - 쿨다운 Tick 관리
 * - 스킬 부여 / 제거 / 조회 API 제공
 * - 실제 실행은 UCC_SkillSystem을 반드시 경유
 */

UCLASS()
class CRISTALCUBE_API ACC_PlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	ACC_PlayerState();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:

	//==========================================================================
	// SKILL MANAGEMENT
	//==========================================================================

    /**
     * 스킬 부여. 중복 SkillID 및 슬롯 초과 시 null 반환.
     * Outer를 this로 설정하므로 GC에 안전.
     */
    UFUNCTION(BlueprintCallable, Category = "Skills")
    UCC_SkillBase* GrantSkill(TSubclassOf<UCC_SkillBase> SkillClass);

    /** SkillID로 스킬 제거. 성공 여부 반환. */
    UFUNCTION(BlueprintCallable, Category = "Skills")
    bool RemoveSkill(FName SkillID);

    /** 모든 스킬 해제 */
    UFUNCTION(BlueprintCallable, Category = "Skills")
    void RemoveAllSkills();

    //==========================================================================
    // QUERIES
    //==========================================================================

    UFUNCTION(BlueprintPure, Category = "Skills")
    UCC_SkillBase* FindSkill(FName SkillID) const;

    UFUNCTION(BlueprintPure, Category = "Skills")
    bool HasSkill(FName SkillID) const;

    UFUNCTION(BlueprintPure, Category = "Skills")
    bool CanGrantMoreSkills() const { return EquippedSkills.Num() < MaxSkillSlots; }

    UFUNCTION(BlueprintPure, Category = "Skills")
    int32 GetSkillCount() const { return EquippedSkills.Num(); }

    UFUNCTION(BlueprintPure, Category = "Skills")
    const TArray<UCC_SkillBase*>& GetAllSkills() const { return EquippedSkills; }

    //==========================================================================
    // CAST INTERFACE
    //==========================================================================

    /**
     * 특정 스킬 발동. PlayerCharacter에서 SkillSystem과 연결해서 호출.
     */
    UFUNCTION(BlueprintCallable, Category = "Skills")
    bool TryCastSkill(FName SkillID, UCC_SkillSystem* SkillSystem, FVector TargetLocation);

    /**
     * Ready 상태인 모든 스킬을 일괄 발동.
     * Vampire Survivors 스타일의 자동 공격 루프에서 사용.
     */
    UFUNCTION(BlueprintCallable, Category = "Skills")
    void CastAllReadySkills(UCC_SkillSystem* SkillSystem, FVector TargetLocation);

protected:

    /** 장착된 스킬 인스턴스 목록. Outer = this이므로 GC 보장. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skills")
    TArray<UCC_SkillBase*> EquippedSkills;

    /** 최대 스킬 슬롯 수 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skills")
    int32 MaxSkillSlots = 6;

};
