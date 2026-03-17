// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CristalCubeStruct.h"
#include "CC_CycleManager.generated.h"

/** 사이클별 난이도 설정 */
USTRUCT(BlueprintType)
struct FCycleConfig
{
    GENERATED_BODY()

    /** 사이클 종료 조건 — 킬 수 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cycle")
    int32 KillsRequired = 20;

    /** 사이클 종료 조건 — 제한 시간 (0이면 킬 수만 사용) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cycle")
    float TimeLimit = 0.f;

    /** 적 최대 동시 수 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty")
    int32 MaxEnemies = 10;

    /** 스폰 간격 (초) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty")
    float SpawnInterval = 3.0f;

    /** 적 공격력 배율 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty")
    float EnemyDamageMultiplier = 1.0f;

    /** 적 이동속도 배율 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty")
    float EnemySpeedMultiplier = 1.0f;
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCubeCleared, int32, CycleNumber);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCycleStarted, int32, CycleNumber);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnKillCountUpdated, int32, Current, int32, Required);

/**
 * ACC_CycleManager
 *
 * 사이클(= Cube Clear 단위) 진행을 전담하는 액터.
 * CC_MainGameMode에서 Spawn하여 참조.
 *
 * EnemyManager 연동:
 *   EnemyManager::OnEnemyUnregistered → OnEnemyKilled() 자동 호출
 *   EnemyCharacter/CycleManager 간 직접 의존 없음
 *
 * 사이클 흐름:
 *   StartFirstCycle()
 *   → 킬 카운트 집계 (EnemyManager 델리게이트)
 *   → KillsRequired 달성 → OnCubeCleared Broadcast
 *   → GameMode가 Cube Clear 보상 UI 표시
 *   → 선택 완료 → StartNextCycle()
 */
UCLASS()
class CRISTALCUBE_API ACC_CycleManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACC_CycleManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

    // ========== 사이클 설정 ==========

    /**
     * 사이클별 설정 배열.
     * 인덱스 = 사이클 번호 - 1.
     * 배열 끝을 초과하면 마지막 항목에 배율을 누적 적용.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cycle Config")
    TArray<FCycleConfig> CycleConfigs;

    /** 배열 초과 시 사이클마다 누적되는 킬 목표 증가량 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cycle Config")
    int32 KillsIncreasePerCycle = 5;

    /** 배열 초과 시 사이클마다 누적되는 적 공격력 배율 증가 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cycle Config")
    float DamageMultiplierIncreasePerCycle = 0.2f;

    // ========== 런타임 상태 ==========

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
    int32 CurrentCycle = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
    int32 KillsThisCycle = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
    int32 TotalKills = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
    bool bCycleActive = false;

    // ========== 델리게이트 ==========
        
    UPROPERTY(BlueprintAssignable, Category = "Cycle Events")
    FOnCubeCleared OnCubeCleared;

    UPROPERTY(BlueprintAssignable, Category = "Cycle Events")
    FOnCycleStarted OnCycleStarted;

    UPROPERTY(BlueprintAssignable, Category = "Cycle Events")
    FOnKillCountUpdated OnKillCountUpdated;

    // ========== 공개 함수 ==========

    /** 첫 번째 사이클 시작 */
    UFUNCTION(BlueprintCallable, Category = "Cycle")
    void StartFirstCycle();

    /** 다음 사이클로 진행 (레벨업 선택 완료 후 호출) */
    UFUNCTION(BlueprintCallable, Category = "Cycle")
    void StartNextCycle();

    /** 현재 사이클 Config 반환 */
    UFUNCTION(BlueprintPure, Category = "Cycle")
    FCycleConfig GetCurrentCycleConfig() const;

    /** 킬 진행도 (0.0 ~ 1.0) */
    UFUNCTION(BlueprintPure, Category = "Cycle")
    float GetKillProgress() const;

    UFUNCTION(BlueprintPure, Category = "Cycle")
    int32 GetCurrentCycle() const { return CurrentCycle; }

    UFUNCTION(BlueprintPure, Category = "Cycle")
    int32 GetKillsRequired() const;

    UFUNCTION(BlueprintPure, Category = "Cycle")
    int32 GetKillsThisCycle() const { return KillsThisCycle; }

private:

    void StartCycle(int32 CycleNumber);
    void CheckCycleCompletion();

    /** 적 처치 시 호출 (EnemyCharacter 사망 시 연결) */
    UFUNCTION(BlueprintCallable, Category = "Cycle")
    void OnEnemyKilled(AActor* KilledEnemy);

    void BindToEnemyManager();

    FTimerHandle TimeLimitHandle;
    void OnTimeLimitReached();

};
