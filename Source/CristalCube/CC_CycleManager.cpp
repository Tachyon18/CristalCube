// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_CycleManager.h"
#include "CC_EnemyManager.h"
#include "CC_CubeWorldManager.h"
#include "Gameplay/CC_Cube.h"
#include "Gameplay/CC_EnemyAIInterface.h"
#include "Curves/CurveFloat.h"
#include "TimerManager.h"

// Sets default values
ACC_CycleManager::ACC_CycleManager()
{
    PrimaryActorTick.bCanEverTick = false;

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

    EM->OnEnemyKilled.AddDynamic(this, &ACC_CycleManager::OnEnemyKilled);
    UE_LOG(LogTemp, Log, TEXT("[CycleManager] Bound to EnemyManager::OnEnemyKilled."));
}

void ACC_CycleManager::OnEnemyKilled(AActor* KilledEnemy)
{
    if (!bTrackingActive) return;

    ++KillsThisStage;
    ++TotalKills;

    bool bWasPersistent = false;
    if (KilledEnemy && KilledEnemy->GetClass()->ImplementsInterface(UCC_EnemyAIInterface::StaticClass()))
    {
        bWasPersistent = ICC_EnemyAIInterface::Execute_IsPersistentEnemy(KilledEnemy);
    }

    BossProgress += bWasPersistent ? PersistentKillWeight : NormalKillWeight;

    OnKillCountUpdated.Broadcast(KillsThisStage, GetKillsRequired());

    UE_LOG(LogTemp, Log,
        TEXT("[CycleManager] Kill counted (%s, Persistent=%s) — %d/%d this stage, BossProgress %.1f/%.1f"),
        *GetNameSafe(KilledEnemy), bWasPersistent ? TEXT("true") : TEXT("false"),
        KillsThisStage, GetKillsRequired(), BossProgress, BossProgressThreshold);

    CheckCubeClearThreshold();
    CheckProgressMilestones();
    CheckBossSpawnCondition();
}

void ACC_CycleManager::OnBossDestroyed(AActor* DestroyedActor)
{
    bBossActive = false;
    CurrentBoss = nullptr;
    bTrackingActive = false; // 게임 클리어 후 킬 카운트 의미 없음

    UE_LOG(LogTemp, Warning, TEXT("[CycleManager] === Boss destroyed — GAME CLEAR ==="));

    OnGameCleared.Broadcast();
}

void ACC_CycleManager::StartCubeClearTracking()
{
    CurrentStage = 1;
    KillsThisStage = 0;
    TotalKills = 0;
    BossProgress = 0.0f;
    bBossActive = false;
    bTrackingActive = true;

    for (FBossProgressMilestone& Milestone : ProgressMilestones)
    {
        Milestone.bTriggered = false;
    }

    OnKillCountUpdated.Broadcast(0, GetKillsRequired());

    UE_LOG(LogTemp, Warning,
        TEXT("[CycleManager] Cube Clear tracking started — Stage %d, KillsRequired: %d"),
        CurrentStage, GetKillsRequired());
}

FCubeClearStageConfig ACC_CycleManager::GetCurrentStageConfig() const
{
    const int32 Stage = FMath::Max(CurrentStage, 1);
    const float X = static_cast<float>(Stage);
    const int32 StageOffset = Stage - 1; // Fallback 선형 공식용

    FCubeClearStageConfig Config;

    Config.KillsRequired = KillsRequiredCurve
        ? FMath::RoundToInt(KillsRequiredCurve->GetFloatValue(X))
        : FallbackBaseKillsRequired + FallbackKillsIncreasePerStage * StageOffset;

    Config.MaxEnemies = MaxEnemiesCurve
        ? FMath::RoundToInt(MaxEnemiesCurve->GetFloatValue(X))
        : FallbackBaseMaxEnemies + FallbackMaxEnemiesIncreasePerStage * StageOffset;

    Config.SpawnInterval = SpawnIntervalCurve
        ? SpawnIntervalCurve->GetFloatValue(X)
        : FMath::Max(0.2f, FallbackBaseSpawnInterval - FallbackSpawnIntervalDecreasePerStage * StageOffset);

    Config.EnemyDamageMultiplier = EnemyDamageMultiplierCurve
        ? EnemyDamageMultiplierCurve->GetFloatValue(X)
        : FallbackBaseDamageMultiplier + FallbackDamageMultiplierIncreasePerStage * StageOffset;

    Config.EnemySpeedMultiplier = EnemySpeedMultiplierCurve
        ? EnemySpeedMultiplierCurve->GetFloatValue(X)
        : FMath::Min(FallbackBaseSpeedMultiplier + FallbackSpeedMultiplierIncreasePerStage * StageOffset,
            FallbackMaxSpeedMultiplier);

    return Config;
}

float ACC_CycleManager::GetKillProgress() const
{
    int32 Required = GetKillsRequired();
    if (Required <= 0) return 1.0f;
    return FMath::Clamp((float)KillsThisStage / Required, 0.f, 1.f);
}

int32 ACC_CycleManager::GetKillsRequired() const
{
    return GetCurrentStageConfig().KillsRequired;
}

float ACC_CycleManager::GetBossProgressRatio() const
{
    if (BossProgressThreshold <= 0.f) return 1.0f;
    return FMath::Clamp(BossProgress / BossProgressThreshold, 0.f, 1.f);
}

void ACC_CycleManager::CheckCubeClearThreshold()
{
    if (KillsThisStage < GetKillsRequired()) return;

    KillsThisStage = 0;
    ++CurrentStage;

    UE_LOG(LogTemp, Warning,
        TEXT("[CycleManager] === Cube Clear achieved — advancing to stage %d ==="), CurrentStage);

    OnCubeClearAchieved.Broadcast(CurrentStage);
    OnKillCountUpdated.Broadcast(0, GetKillsRequired());
}

void ACC_CycleManager::CheckBossSpawnCondition()
{
    if (bBossActive) return;
    if (BossProgress < BossProgressThreshold) return;

    SpawnBoss();
}

void ACC_CycleManager::CheckProgressMilestones()
{
    if (ProgressMilestones.Num() == 0 || BossProgressThreshold <= 0.f) return;

    const float CurrentRatio = BossProgress / BossProgressThreshold;

    for (FBossProgressMilestone& Milestone : ProgressMilestones)
    {
        if (Milestone.bTriggered) continue;
        if (CurrentRatio < Milestone.TriggerRatio) continue;

        Milestone.bTriggered = true;

        UE_LOG(LogTemp, Log,
            TEXT("[CycleManager] Progress milestone reached: %s at %.0f%% BossProgress"),
            *Milestone.EventID.ToString(), CurrentRatio * 100.f);

        OnProgressMilestoneReached.Broadcast(Milestone.EventID, CurrentRatio);
    }
}

void ACC_CycleManager::SpawnBoss()
{
    if (!BossClass)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CycleManager] BossProgress threshold reached but BossClass is not set — spawn skipped."));
        return;
    }

    FVector SpawnLocation = GetActorLocation();
    if (ACC_CubeWorldManager* WorldManager = ACC_CubeWorldManager::Get(this))
    {
        if (ACC_Cube* ActiveCube = WorldManager->GetActiveCube())
        {
            SpawnLocation = ActiveCube->GetCubeCenter() + FVector(0.f, 0.f, BossSpawnHeightOffset);
        }
    }

    FActorSpawnParameters Params;
    Params.Owner = this;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    CurrentBoss = GetWorld()->SpawnActor<AActor>(BossClass, SpawnLocation, FRotator::ZeroRotator, Params);
    if (!CurrentBoss)
    {
        UE_LOG(LogTemp, Error, TEXT("[CycleManager] Failed to spawn Boss!"));
        return;
    }

    bBossActive = true;
    CurrentBoss->OnDestroyed.AddDynamic(this, &ACC_CycleManager::OnBossDestroyed);

    UE_LOG(LogTemp, Warning, TEXT("[CycleManager] Boss spawned at %s"), *SpawnLocation.ToString());

    OnBossSpawned.Broadcast(CurrentBoss);
}
