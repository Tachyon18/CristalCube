// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_CycleManager.h"
#include "CC_EnemyManager.h"
#include "TimerManager.h"

// Sets default values
ACC_CycleManager::ACC_CycleManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

    // 기본 사이클 설정 6개 (블루프린트에서 조정 가능)
    auto Make = [](int32 Kills, int32 Max, float Interval, float Dmg, float Spd) {
        FCycleConfig C;
        C.KillsRequired = Kills;
        C.MaxEnemies = Max;
        C.SpawnInterval = Interval;
        C.EnemyDamageMultiplier = Dmg;
        C.EnemySpeedMultiplier = Spd;
        return C;
        };

    CycleConfigs.Add(Make(20, 10, 3.0f, 1.0f, 1.0f));
    CycleConfigs.Add(Make(25, 15, 2.5f, 1.1f, 1.05f));
    CycleConfigs.Add(Make(30, 20, 2.0f, 1.2f, 1.1f));
    CycleConfigs.Add(Make(35, 25, 1.8f, 1.4f, 1.15f));
    CycleConfigs.Add(Make(40, 30, 1.5f, 1.6f, 1.2f));
    CycleConfigs.Add(Make(45, 30, 1.5f, 1.8f, 1.25f));

}

// Called when the game starts or when spawned
void ACC_CycleManager::BeginPlay()
{
	Super::BeginPlay();

    // EnemyManager 델리게이트 구독 (약간의 딜레이로 EnemyManager 초기화 보장)
    FTimerHandle BindHandle;
    GetWorldTimerManager().SetTimer(
        BindHandle, this,
        &ACC_CycleManager::BindToEnemyManager,
        0.1f, false);
}

// Called every frame
void ACC_CycleManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ACC_CycleManager::BindToEnemyManager()
{
    ACC_EnemyManager* EM = ACC_EnemyManager::Get(this);
    if (!EM)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CycleManager] EnemyManager not found — kill count disabled."));
        return;
    }

    EM->OnEnemyUnregistered.AddDynamic(this, &ACC_CycleManager::OnEnemyKilled);
    UE_LOG(LogTemp, Log, TEXT("[CycleManager] Bound to EnemyManager::OnEnemyUnregistered."));
}

void ACC_CycleManager::OnEnemyKilled(AActor* KilledEnemy)
{
    if (!bCycleActive) return;

    ++KillsThisCycle;
    ++TotalKills;

    OnKillCountUpdated.Broadcast(KillsThisCycle, GetKillsRequired());

    UE_LOG(LogTemp, Log, TEXT("[CycleManager] Kill %d / %d"),
        KillsThisCycle, GetKillsRequired());

    CheckCycleCompletion();
}

// ============================================================
//  사이클 제어
// ============================================================

void ACC_CycleManager::StartFirstCycle()
{
    StartCycle(1);
}

void ACC_CycleManager::StartNextCycle()
{
    StartCycle(CurrentCycle + 1);
}

void ACC_CycleManager::StartCycle(int32 CycleNumber)
{
    CurrentCycle = CycleNumber;
    KillsThisCycle = 0;
    bCycleActive = true;

    FCycleConfig Config = GetCurrentCycleConfig();

    // 제한 시간 설정 (0이면 킬 수 기반)
    if (Config.TimeLimit > 0.f)
    {
        GetWorldTimerManager().SetTimer(
            TimeLimitHandle, this,
            &ACC_CycleManager::OnTimeLimitReached,
            Config.TimeLimit, false);
    }

    OnCycleStarted.Broadcast(CurrentCycle);
    OnKillCountUpdated.Broadcast(0, GetKillsRequired());

    UE_LOG(LogTemp, Warning, TEXT("[CycleManager] Cycle %d started — KillsRequired: %d, MaxEnemies: %d"),
        CurrentCycle, GetKillsRequired(), Config.MaxEnemies);
}

void ACC_CycleManager::CheckCycleCompletion()
{
    if (KillsThisCycle < GetKillsRequired()) return;

    bCycleActive = false;
    GetWorldTimerManager().ClearTimer(TimeLimitHandle);

    UE_LOG(LogTemp, Warning,
        TEXT("[CycleManager] === Cube %d CLEARED ==="), CurrentCycle);

    OnCubeCleared.Broadcast(CurrentCycle);
}

void ACC_CycleManager::OnTimeLimitReached()
{
    if (!bCycleActive) return;

    bCycleActive = false;

    UE_LOG(LogTemp, Warning, TEXT("[CycleManager] Cycle %d — Time limit reached"), CurrentCycle);

    OnCubeCleared.Broadcast(CurrentCycle);
}

// ============================================================
//  조회
// ============================================================

FCycleConfig ACC_CycleManager::GetCurrentCycleConfig() const
{
    int32 Index = CurrentCycle - 1;

    if (CycleConfigs.IsValidIndex(Index))
    {
        return CycleConfigs[Index];
    }

    // 배열 초과 시 마지막 항목에 누적 배율 적용
    FCycleConfig Last = CycleConfigs.Last();
    int32 Overflow = Index - (CycleConfigs.Num() - 1);

    Last.KillsRequired += KillsIncreasePerCycle * Overflow;
    Last.EnemyDamageMultiplier += DamageMultiplierIncreasePerCycle * Overflow;
    Last.EnemySpeedMultiplier = FMath::Min(Last.EnemySpeedMultiplier + 0.05f * Overflow, 2.0f);

    return Last;
}

float ACC_CycleManager::GetKillProgress() const
{
    int32 Required = GetKillsRequired();
    if (Required <= 0) return 1.0f;
    return FMath::Clamp((float)KillsThisCycle / Required, 0.f, 1.f);
}

int32 ACC_CycleManager::GetKillsRequired() const
{
    return GetCurrentCycleConfig().KillsRequired;
}
