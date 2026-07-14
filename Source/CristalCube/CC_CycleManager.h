// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CristalCubeStruct.h"
#include "CC_CycleManager.generated.h"

class UCurveFloat;

/** Cube Clear(킬 카운트) 단계별 난이도 설정 — 기존 FCycleConfig를 대체 (TimeLimit 필드 제거) */
USTRUCT(BlueprintType)
struct FCubeClearStageConfig
{
    GENERATED_BODY()

    /** 이 단계에서 Cube Clear로 인정되는 킬 수 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage")
    int32 KillsRequired = 20;

    /** 적 최대 동시 수 — 추후 CubeWorldManager가 Spawner에 배포 (10.2, 다음 작업 예정) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty")
    int32 MaxEnemies = 10;

    /** 스폰 간격 (초) — 추후 CubeWorldManager가 Spawner에 배포 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty")
    float SpawnInterval = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty")
    float EnemyDamageMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty")
    float EnemySpeedMultiplier = 1.0f;
};

/**
 * [자리만 예약 — 2026-07-13] BossProgress 구간별 이벤트 훅.
 * TriggerRatio/EventID 둘 다 디자이너가 배열로 채워 넣는 데이터이며 C++엔 하드코딩하지 않는다.
 * EventID가 무엇을 의미하는지(보상/특수 웨이브/미니보스/필드 이벤트 등)는 아직 미정 —
 * 리스너 쪽(어디서 EventID를 분기할지)도 다음 세션에 결정. 지금은 구조만 존재, 트리거 로직 없음.
 */
USTRUCT(BlueprintType)
struct FBossProgressMilestone
{
    GENERATED_BODY()

    /** BossProgressThreshold 대비 비율 (0.0 ~ 1.0) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Milestone")
    float TriggerRatio = 0.5f;

    /** 이벤트 식별자 — enum 아님. 실제 의미/처리는 리스너 쪽에서 해석 (미정) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Milestone")
    FName EventID = NAME_None;

    /** 런타임 상태 — 세션마다 리셋, 한 Milestone당 1회만 발화 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Milestone")
    bool bTriggered = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCubeClearAchieved, int32, StageNumber);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBossSpawned, AActor*, Boss);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameCleared);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnKillCountUpdated, int32, Current, int32, Required);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProgressMilestoneReached, FName, EventID, float, ProgressRatio);

/**
 * ACC_CycleManager
 *
 * [2026-07-13 재작성 — Cube_Cycle_Loop_Redesign.md 10절 기준, "Cycle 반복" 개념 폐기]
 *
 *   1) 누적 킬 수가 단계별 임계치(StageConfigs)에 도달할 때마다 "Cube Clear" 이벤트 —
 *      반복 발생, Cube Energy 보상 + (추후) Spawner 난이도 갱신 트리거. GameMode가 구독.
 *   2) 그와 별개로 BossProgress 게이지(일반/Persistent 처치를 다른 가중치로 누적)를 채워
 *      임계치 도달 시 Boss를 직접 스폰/생명주기 관리.
 *   3) Boss 처치(AActor::OnDestroyed) 시 OnGameCleared Broadcast — 이게 진짜 게임 클리어. 딱 1회.
 *
 * EnemyManager 연동: EnemyManager::OnEnemyKilled → OnEnemyKilled() 자동 호출 (기존과 동일)
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

    // ====================================================================
	// Cube Clear Stage Configurations
	// ====================================================================

    // [2026-07-13] 하드코딩 배열(StageConfigs) 제거 — 커브 에셋으로 대체.
    // X = CurrentStage, Y = 해당 값. 커브 에셋이 비어있으면 아래 Fallback 선형 공식을 씀
    // (디자이너가 커브를 아직 안 만들었어도 게임이 굴러가게 하기 위한 안전장치).

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube Clear|Curves")
    UCurveFloat* KillsRequiredCurve = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube Clear|Curves")
    UCurveFloat* MaxEnemiesCurve = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube Clear|Curves")
    UCurveFloat* SpawnIntervalCurve = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube Clear|Curves")
    UCurveFloat* EnemyDamageMultiplierCurve = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube Clear|Curves")
    UCurveFloat* EnemySpeedMultiplierCurve = nullptr;

    // ---- Fallback (커브 미할당 시 사용되는 선형 공식 기본값) ----

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube Clear|Fallback")
    int32 FallbackBaseKillsRequired = 20;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube Clear|Fallback")
    int32 FallbackKillsIncreasePerStage = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube Clear|Fallback")
    int32 FallbackBaseMaxEnemies = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube Clear|Fallback")
    int32 FallbackMaxEnemiesIncreasePerStage = 2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube Clear|Fallback")
    float FallbackBaseSpawnInterval = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube Clear|Fallback")
    float FallbackSpawnIntervalDecreasePerStage = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube Clear|Fallback")
    float FallbackBaseDamageMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube Clear|Fallback")
    float FallbackDamageMultiplierIncreasePerStage = 0.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube Clear|Fallback")
    float FallbackBaseSpeedMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube Clear|Fallback")
    float FallbackSpeedMultiplierIncreasePerStage = 0.05f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube Clear|Fallback")
    float FallbackMaxSpeedMultiplier = 2.0f;

	// ====================================================================
    //  Boss / Game Clear 설정 
	// ====================================================================

    /** BossProgress 게이지 임계치 — [임시값, 밸런싱 대상. 10.6절] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss")
    float BossProgressThreshold = 150.0f;

    /** 일반 처치 기여량 — [임시값] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss")
    float NormalKillWeight = 1.0f;

    /** Persistent 처치 기여량 — 일반보다 크게 ("세게 잡을수록 이득") [임시값] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss")
    float PersistentKillWeight = 4.0f;

    /** 스폰할 Boss 클래스 — [미설정, 10.6절 "Boss 액터 미설계"]. 비어있으면 SpawnBoss()가 경고만 남기고 스폰 안 함 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss")
    TSubclassOf<AActor> BossClass;

    /** Boss 스폰 위치 — ActiveCube 중심 기준 Z 오프셋 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss")
    float BossSpawnHeightOffset = 200.0f;

    /**
     * [자리만 예약] Progress 구간별 이벤트 지점들. 디자이너가 자유롭게 추가/삭제.
     * 지금은 값을 채워도 아무 동작 안 함 — 트리거 체크 로직 자체가 다음 세션 작업.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Milestones")
    TArray<FBossProgressMilestone> ProgressMilestones;

	// ====================================================================
    // 런타임 상태
	// ====================================================================

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
    int32 CurrentStage = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
    int32 KillsThisStage = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
    int32 TotalKills = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
    bool bTrackingActive = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
    float BossProgress = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
    bool bBossActive = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
    AActor* CurrentBoss = nullptr;

	// ====================================================================
    // 델리게이트
	// ====================================================================

    /** 킬 카운트가 현재 단계 임계치에 도달할 때마다 Broadcast (반복 발생) */
    UPROPERTY(BlueprintAssignable, Category = "Cube Clear|Events")
    FOnCubeClearAchieved OnCubeClearAchieved;

    UPROPERTY(BlueprintAssignable, Category = "Boss|Events")
    FOnBossSpawned OnBossSpawned;

    /** Boss 처치 = 진짜 게임 클리어. 딱 한 번 Broadcast */
    UPROPERTY(BlueprintAssignable, Category = "Boss|Events")
    FOnGameCleared OnGameCleared;

    UPROPERTY(BlueprintAssignable, Category = "Cube Clear|Events")
    FOnKillCountUpdated OnKillCountUpdated;

    UPROPERTY(BlueprintAssignable, Category = "Boss|Events")
    FOnProgressMilestoneReached OnProgressMilestoneReached;

    // ========== 공개 함수 ==========

    /** 킬 카운트/BossProgress 추적 시작. GameMode BeginPlay에서 1회 호출 */
    UFUNCTION(BlueprintCallable, Category = "Cube Clear")
    void StartCubeClearTracking();

    UFUNCTION(BlueprintPure, Category = "Cube Clear")
    FCubeClearStageConfig GetCurrentStageConfig() const;

    UFUNCTION(BlueprintPure, Category = "Cube Clear")
    float GetKillProgress() const;

    UFUNCTION(BlueprintPure, Category = "Cube Clear")
    int32 GetCurrentStage() const { return CurrentStage; }

    UFUNCTION(BlueprintPure, Category = "Cube Clear")
    int32 GetKillsRequired() const;

    UFUNCTION(BlueprintPure, Category = "Boss")
    float GetBossProgressRatio() const;

private:

    void CheckCubeClearThreshold();
    void CheckBossSpawnCondition();
    void CheckProgressMilestones();
    void SpawnBoss();

    UFUNCTION()
    void OnEnemyKilled(AActor* KilledEnemy);

    UFUNCTION()
    void OnBossDestroyed(AActor* DestroyedActor);

    void BindToEnemyManager();
};
