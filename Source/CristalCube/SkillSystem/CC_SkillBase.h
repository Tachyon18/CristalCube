// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "../CristalCubeStruct.h"
#include "CC_SkillBase.generated.h"

class UCC_SkillSystem;

/**
 * 모든 스킬의 베이스 클래스.
 * - 자신의 FSkillDefinition을 소유하고 초기화
 * - 쿨다운 상태를 내부적으로 관리
 * - 실제 실행은 반드시 UCC_SkillSystem을 경유
 * - PlayerState의 EquippedSkills 배열에서 관리됨
 */

UCLASS(Blueprintable, BlueprintType, Abstract)
class CRISTALCUBE_API UCC_SkillBase : public UObject
{
	GENERATED_BODY()
	
public:
	UCC_SkillBase();

public:

    //==========================================================================
    // MAIN INTERFACE
    //==========================================================================

    /**
     * 스킬 발동 시도. 쿨다운 체크 후 SkillSystem에 위임.
     * @param SkillSystem - 실행 엔진 (필수)
     * @param TargetLocation - 발동 목표 위치
     * @return 실제로 발동되었으면 true
     */
    UFUNCTION(BlueprintCallable, Category = "Skill")
    bool TryCast(UCC_SkillSystem* SkillSystem, FVector TargetLocation);

    /**
     * 쿨다운 틱 업데이트. PlayerState의 Tick에서 호출.
     */

    void TickCooldown(float DeltaTime);

    //==========================================================================
    // QUERIES
    //==========================================================================

    UFUNCTION(BlueprintPure, Category = "Skill")
    bool IsReady() const { return CooldownRemaining <= 0.0f; }

    /** 쿨다운 진행률 (0.0 = 막 사용, 1.0 = 사용 가능) */
    UFUNCTION(BlueprintPure, Category = "Skill")
    float GetCooldownProgress() const;

    UFUNCTION(BlueprintPure, Category = "Skill")
    float GetRemainingCooldown() const { return CooldownRemaining; }

    UFUNCTION(BlueprintPure, Category = "Skill")
    FName GetSkillID() const { return SkillDef.SkillID; }

    UFUNCTION(BlueprintPure, Category = "Skill")
    const FSkillDefinition& GetDefinition() const { return SkillDef; }

    //==========================================================================
    // LIFECYCLE EVENTS
    //==========================================================================

    /** PlayerState에 장착될 때 호출 */
    UFUNCTION(BlueprintCallable, Category = "Skill")
    virtual void OnEquipped(AActor* InOwner);

    /** PlayerState에서 해제될 때 호출 */
    UFUNCTION(BlueprintCallable, Category = "Skill")
    virtual void OnUnequipped();

    //==========================================================================
    // UPGRADE INTERFACE (Phase 2+)
    //==========================================================================

    /**
     * 패시브 수치 누적 적용. (레벨업/아이템 강화 연동)
     * Multiplier 방식이므로 기본값 1.0f 이상/이하로 배율 조정.
     */
    UFUNCTION(BlueprintCallable, Category = "Skill")
    virtual void ApplyPassiveModifier(const FSkillPassiveProperties& Modifier);

protected:

    //==========================================================================
    // PROPERTIES (서브클래스에서 생성자로 초기화)
    //==========================================================================

    /** 이 스킬의 완전한 정의. 서브클래스 생성자에서 설정. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Definition")
    FSkillDefinition SkillDef;

private:

    /** 런타임 쿨다운. GC 안전, UPROPERTY 불필요 (float) */
    float CooldownRemaining = 0.0f;

    /** 장착된 오너의 약참조 (GC 순환 방지) */
    UPROPERTY()
    TWeakObjectPtr<AActor> SkillOwner;
};
