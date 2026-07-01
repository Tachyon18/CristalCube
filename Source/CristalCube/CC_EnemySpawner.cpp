// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_EnemySpawner.h"
#include "Characters/CC_EnemyCharacter.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Gameplay/CC_Cube.h"
#include "Gameplay/CC_EnemyAIInterface.h"
#include "CC_GameModeBase.h"
#include "CC_LogHelper.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
ACC_EnemySpawner::ACC_EnemySpawner()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;

    // Default spawn settings
    SpawnInterval = 3.0f;
    CurrentSpawnInterval = SpawnInterval;
    EnemiesPerSpawn = 3;
    MinSpawnDistance = 800.0f;
    MaxSpawnDistance = 1200.0f;
    MaxEnemies = 50;

    // Difficulty scaling
    bIncreaseSpawnRate = true;
    SpawnIntervalDecreasePerMinute = 0.1f;
    MinSpawnInterval = 0.5f;

    // State
    bAutoStart = true;
    bIsSpawning = false;
    GameTime = 0.0f;
    CachedPlayer = nullptr;

	OwnerCube = nullptr;
	bIsFrozen = false;
}

// Called when the game starts or when spawned
void ACC_EnemySpawner::BeginPlay()
{
    Super::BeginPlay();

    FindPlayer();

    if (!EnemyClass)
    {
        CC_LOG_SPAWNER(Error, TEXT("No enemy class set! Spawner disabled."));
        return;
    }

    // 더 이상 "내가 먼저 Cube를 찾는" 쪽이 아니라 "CubeWorldManager::SetupCubeContent()가
    // Cube 생성 시점에 SetOwnerCube()를 호출해 나를 찾아와주는" 쪽으로 역할이 바뀜 (3.3절 참고).
    if (OwnerCube)
    {
        // Cube가 이미 BeginPlay되어 SetOwnerCube()를 먼저 호출해둔 경우
        // (엔진 BeginPlay 순서상 Cube 쪽이 먼저 실행됐을 때 — 흔하게 일어날 수 있음)
        OwnerCube->RegisterActor(this);
        CC_LOG_SPAWNER(Log, TEXT("[Spawner] Registered with Cube (%d,%d)"),
            OwnerCube->CubeCoordinate.X, OwnerCube->CubeCoordinate.Y);

        if (bAutoStart && !OwnerCube->IsFrozen())
        {  
            RequestSpawningStart();
        }
        else
        {
            CC_LOG_SPAWNER(Log, TEXT("[Spawner] Cube is Frozen — waiting for Unfreeze"));
        }
    }
    else
    {
        // [신규] OwnerCube 연결 대기 — SetOwnerCube()가 외부(CubeWorldManager)에서 호출되면
        // 그 안에서 자동으로 스폰이 시작됨 (아래 SetOwnerCube() 참고).
        // 안전장치: 일정 시간 안에 연결이 안 되면 경고를 남기고 Freeze 연동 없이 fallback 스폰.
        GetWorld()->GetTimerManager().SetTimer(
            OwnerCubeTimeoutHandle,
            this,
            &ACC_EnemySpawner::OnOwnerCubeTimeout,
            OwnerCubeWaitTimeout,
            false);

        CC_LOG_SPAWNER(Log,
            TEXT("[Spawner] Waiting for OwnerCube link (timeout: %.1fs)"),
            OwnerCubeWaitTimeout);
    }

    CC_LOG_SPAWNER(Warning,
        TEXT("EnemySpawner initialized (Interval: %.1fs, Enemies/Spawn: %d, Max: %d)"),
        SpawnInterval, EnemiesPerSpawn, MaxEnemies);
}

// Called every frame
void ACC_EnemySpawner::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    GameTime += DeltaTime;

    // Update spawn interval based on difficulty scaling
    if (bIncreaseSpawnRate && bIsSpawning)
    {
        UpdateSpawnInterval();
    }

    // Periodically clean up dead enemies
    static float CleanupTimer = 0.0f;
    CleanupTimer += DeltaTime;

    if (CleanupTimer >= 5.0f)  // Every 5 seconds
    {
        CleanupDeadEnemies();
        CleanupTimer = 0.0f;
    }
}

void ACC_EnemySpawner::CheckWaveClearCondition()
{
    if (bWaveCleared) return;
    if (TotalSpawnedThisWave < WaveSize) return;   // 아직 다 못 내보냄
    if (GetAliveEnemyCount() > 0) return;          // 아직 생존자 있음

    bWaveCleared = true;

    CC_LOG_SPAWNER(Warning, TEXT("[Spawner] WAVE CLEARED — Cube (%s)"),
        OwnerCube ? *OwnerCube->CubeCoordinate.ToString() : TEXT("None"));

    OnWaveCleared.Broadcast(this);
}

void ACC_EnemySpawner::StartSpawning()
{
    if (bWaveCleared)
    {
        CC_LOG_SPAWNER(Log, TEXT("[Spawner] Wave already cleared for this cube — not restarting."));
        return;
    }

    if (bIsSpawning)
    {
        CC_LOG_SPAWNER(Warning, TEXT("Already spawning!"));
        return;
    }

    if (!EnemyClass)
    {
        CC_LOG_SPAWNER(Error, TEXT("Cannot start spawning - no enemy class set!"));
        return;
    }

    if (!bHasWaveStarted)
    {
        bHasWaveStarted = true;
        TotalSpawnedThisWave = 0;
        CC_LOG_SPAWNER(Log, TEXT("[Spawner] Wave START (WaveSize=%d)"), WaveSize);
    }
    else
    {
        CC_LOG_SPAWNER(Log, TEXT("[Spawner] Wave RESUME (%d/%d already spawned)"),
            TotalSpawnedThisWave, WaveSize);
    }

    bIsSpawning = true;
    CurrentSpawnInterval = SpawnInterval;

    // Set up repeating timer
    GetWorld()->GetTimerManager().SetTimer(
        SpawnTimerHandle,
        this,
        &ACC_EnemySpawner::SpawnEnemies,
        CurrentSpawnInterval,
        true  // Repeating
    );

    CC_LOG_SPAWNER(Log, TEXT("Started spawning enemies"));

    // Spawn immediately
    SpawnEnemies();
}

void ACC_EnemySpawner::StopSpawning()
{
    if (!bIsSpawning)
    {
        return;
    }

    bIsSpawning = false;
    GetWorld()->GetTimerManager().ClearTimer(SpawnTimerHandle);

    CC_LOG_SPAWNER(Log, TEXT("Stopped spawning enemies"));
}

void ACC_EnemySpawner::SpawnEnemies()
{
    if (bWaveCleared)
    {
        return;
    }

    int32 RemainingInWave = WaveSize - TotalSpawnedThisWave;
    if (RemainingInWave <= 0)
    {
        // 이미 다 내보냈음 — 타이머만 도는 상태였다면 정리
        StopSpawning();
        return;
    }

    if (!CanSpawnMore())
    {
        CC_LOG_SPAWNER(VeryVerbose, TEXT("Max enemies reached (%d/%d), skipping spawn"),
            GetAliveEnemyCount(), MaxEnemies);
        return;
    }

    if (!CachedPlayer)
    {
        FindPlayer();
        if (!CachedPlayer)
        {
            CC_LOG_SPAWNER(Warning, TEXT("No player found, cannot spawn enemies"));
            return;
        }
    }

    // Calculate how many enemies we can spawn
    int32 EnemiesToSpawn = FMath::Min3(
        EnemiesPerSpawn,
        RemainingInWave,
        MaxEnemies - GetAliveEnemyCount());

    int32 SuccessfulSpawns = 0;

    for (int32 i = 0; i < EnemiesToSpawn; ++i)
    {
        FVector SpawnLocation = GetRandomSpawnLocation();

        APawn* NewEnemy = SpawnSingleEnemy(SpawnLocation);

        if (NewEnemy)
        {
            SpawnedEnemies.Add(NewEnemy);
            SuccessfulSpawns++;
        }
    }

	TotalSpawnedThisWave += SuccessfulSpawns;

    if (SuccessfulSpawns > 0)
    {
        CC_LOG_SPAWNER(Log, TEXT("Spawned %d enemies (Wave: %d/%d, Alive: %d/%d, Interval: %.2fs)"),
            SuccessfulSpawns, TotalSpawnedThisWave, WaveSize,
            GetAliveEnemyCount(), MaxEnemies, CurrentSpawnInterval);
    }

    if (TotalSpawnedThisWave >= WaveSize)
    {
        // 더 이상 내보낼 게 없음 — 타이머 정지. "전멸 판정"은 CheckWaveClearCondition()이 담당.
        StopSpawning();
        CC_LOG_SPAWNER(Log, TEXT("[Spawner] Wave spawn quota reached (%d/%d) — timer stopped, waiting for clear."),
            TotalSpawnedThisWave, WaveSize);
    }
}

APawn* ACC_EnemySpawner::SpawnSingleEnemy(const FVector& Location)
{
    if (!EnemyClass)
    {
        return nullptr;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    APawn* NewEnemy = GetWorld()->SpawnActor<APawn>(
        EnemyClass,
        Location,
        FRotator::ZeroRotator,
        SpawnParams
    );

    if (NewEnemy)
    {
        bool bIsPersistent = false;
        if (NewEnemy->GetClass()->ImplementsInterface(UCC_EnemyAIInterface::StaticClass()))
        {
            bIsPersistent = ICC_EnemyAIInterface::Execute_IsPersistentEnemy(NewEnemy);
        }

        if (bIsPersistent)
        {
            CC_LOG_SPAWNER(Log, TEXT("[Spawner] NewEnemy is Persistent — skipping Cube ownership entirely"));

        }
        else if (OwnerCube)
        {
            OwnerCube->RegisterActor(NewEnemy);

            if (OwnerCube->IsFrozen() && !bIsPersistent)
            {
                NewEnemy->SetActorHiddenInGame(true);
                NewEnemy->SetActorTickEnabled(false);
                
                if(NewEnemy->Implements<UCC_Freezable>())
                {
                    ICC_Freezable::Execute_Freeze(NewEnemy);
                }
                
                CC_LOG_SPAWNER(Log,
                    TEXT("[Spawner] NewEnemy Frozen immediately (Cube was already Frozen)"));
            }

            CC_LOG_SPAWNER(VeryVerbose, TEXT("Registered enemy with Cube (%d, %d)"),
                OwnerCube->CubeCoordinate.X, OwnerCube->CubeCoordinate.Y);
        }

        CC_LOG_SPAWNER(VeryVerbose, TEXT("Spawned enemy at (%.0f, %.0f, %.0f)"),
            Location.X, Location.Y, Location.Z);
    }
    else
    {
        CC_LOG_SPAWNER(Warning, TEXT("Failed to spawn enemy at location"));
    }

    return NewEnemy;
}

FVector ACC_EnemySpawner::GetRandomSpawnLocation() const
{
    FVector SpawnCenter;
	float SpawnRadius;    
    
    if (OwnerCube)
    {
        SpawnCenter = OwnerCube->GetCubeCenter();
        // Spawn within cube bounds (slightly smaller than cube size)
        SpawnRadius = OwnerCube->CubeSize * 0.35f; 

        CC_LOG_SPAWNER(VeryVerbose, TEXT("Using Cube center for spawn location"));
    }
    // Priority 2: Use Player center as fallback
    else if (CachedPlayer)
    {
        SpawnCenter = CachedPlayer->GetActorLocation();
        SpawnRadius = MaxSpawnDistance;

        CC_LOG_SPAWNER(VeryVerbose, TEXT("Using Player center for spawn location"));
    }
    // Priority 3: World origin
    else
    {
        CC_LOG_SPAWNER(Warning, TEXT("No player for spawn location, using world origin"));
        return FVector::ZeroVector;
    }

    // Random angle (360 degrees)
    float Angle = FMath::FRandRange(0.0f, 360.0f);
    float RadAngle = FMath::DegreesToRadians(Angle);

    // Random distance between min and max
    float MinDist = OwnerCube ? 0.0f : MinSpawnDistance;
    float Distance = FMath::FRandRange(MinSpawnDistance, SpawnRadius);

    // Calculate offset from player
    FVector Offset;
    Offset.X = FMath::Cos(RadAngle) * Distance;
    Offset.Y = FMath::Sin(RadAngle) * Distance;
    Offset.Z = 0.0f;  // Keep on ground level

    FVector SpawnLocation = SpawnCenter + Offset;

    CC_LOG_SPAWNER(VeryVerbose, TEXT("Spawn location: (%.0f, %.0f, %.0f)"),
        SpawnLocation.X, SpawnLocation.Y, SpawnLocation.Z);

    return SpawnLocation + Offset;
}

void ACC_EnemySpawner::CleanupDeadEnemies()
{
    int32 RemovedCount = 0;

    // Remove null and dead enemies from tracking array
    for (int32 i = SpawnedEnemies.Num() - 1; i >= 0; --i)
    {
        APawn* Enemy = SpawnedEnemies[i];
        bool bDead = !IsValid(Enemy);
        if (!bDead && Enemy->Implements<UCC_EnemyAIInterface>())
        {
            bDead = !ICC_EnemyAIInterface::Execute_IsEnemyAlive(Enemy);
        }

        if (bDead)
        {
            SpawnedEnemies.RemoveAt(i);
            RemovedCount++;
        }
    }

    if (RemovedCount > 0)
    {
        CC_LOG_SPAWNER(VeryVerbose, TEXT("Cleaned up %d dead enemies (Alive: %d)"),
            RemovedCount, GetAliveEnemyCount());
    }

    // 정리 직후 웨이브 클리어 여부 체크 — 별도 폴링 타이머 안 만들고 기존 5초 주기에 편승
    CheckWaveClearCondition();
}

void ACC_EnemySpawner::UpdateSpawnInterval()
{
    if (!bIncreaseSpawnRate)
    {
        return;
    }

    // Calculate new interval based on game time
    float MinutesPlayed = GameTime / 60.0f;
    float IntervalDecrease = MinutesPlayed * SpawnIntervalDecreasePerMinute;

    float NewInterval = FMath::Max(SpawnInterval - IntervalDecrease, MinSpawnInterval);

    // Only update if changed significantly
    if (FMath::Abs(NewInterval - CurrentSpawnInterval) > 0.05f)
    {
        CurrentSpawnInterval = NewInterval;

        // Restart timer with new interval
        if (bIsSpawning)
        {
            GetWorld()->GetTimerManager().ClearTimer(SpawnTimerHandle);
            GetWorld()->GetTimerManager().SetTimer(
                SpawnTimerHandle,
                this,
                &ACC_EnemySpawner::SpawnEnemies,
                CurrentSpawnInterval,
                true
            );

            CC_LOG_SPAWNER(VeryVerbose, TEXT("Spawn interval updated to %.2fs (%.1f min elapsed)"),
                CurrentSpawnInterval, MinutesPlayed);
        }
    }
}

int32 ACC_EnemySpawner::GetAliveEnemyCount() const
{
    int32 Count = 0;

    for (APawn* Enemy : SpawnedEnemies)
    {
        bool bAlive = IsValid(Enemy);
        if (bAlive && Enemy->Implements<UCC_EnemyAIInterface>())
        {
            bAlive = ICC_EnemyAIInterface::Execute_IsEnemyAlive(Enemy);
        }

        if (bAlive)
        {
            Count++;
        }
    }

    return Count;
}

bool ACC_EnemySpawner::CanSpawnMore() const
{
    return GetAliveEnemyCount() < MaxEnemies;
}

void ACC_EnemySpawner::SetOwnerCube(ACC_Cube* Cube)
{
    OwnerCube = Cube;

    // [신규] 연결되었으니 대기 타임아웃은 더 이상 필요 없음
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(OwnerCubeTimeoutHandle);
    }

    // [버그 수정 — 기존 유지] OwnerCube가 늦게 연결되는 경우, 그 사이 태어난
    // 미등록 Enemy들을 한꺼번에 등록.
    if (OwnerCube)
    {
        for (APawn* Enemy : SpawnedEnemies)
        {
            if (IsValid(Enemy))
            {
                OwnerCube->RegisterActor(Enemy);
            }
        }

		OnWaveCleared.AddUniqueDynamic(OwnerCube, &ACC_Cube::HandleSpawnerWaveCleared);

        // (CubeWorldManager::LinkSpawnersToNearestCube()에서 중복 호출하던 StartSpawning()은
        //  제거하고 이 경로 하나로 단일화)
        if (bAutoStart && !bIsSpawning && EnemyClass && !OwnerCube->IsFrozen())
        {
            RequestSpawningStart();
            CC_LOG_SPAWNER(Log, TEXT("[Spawner] OwnerCube linked — spawn requested (Cube (%d,%d))"),
                OwnerCube->CubeCoordinate.X, OwnerCube->CubeCoordinate.Y);
        }
    }
}

void ACC_EnemySpawner::Freeze_Implementation()
{
    if (bIsFrozen)
    {
        CC_LOG_SPAWNER(VeryVerbose, TEXT("Already frozen, ignoring"));
        return;
    }

    // Stop spawning
    StopSpawning();

    bIsFrozen = true;

    CC_LOG_SPAWNER(Log, TEXT("Spawner FROZEN (Alive enemies: %d)"), GetAliveEnemyCount());

}

void ACC_EnemySpawner::Unfreeze_Implementation()
{
    if (!bIsFrozen)
    {
        CC_LOG_SPAWNER(VeryVerbose, TEXT("Not frozen, ignoring"));
        return;
    }

    bIsFrozen = false;

    // Resume spawning
    if (EnemyClass)
    {
        RequestSpawningStart();
        CC_LOG_SPAWNER(Log, TEXT("Spawner UNFROZEN (Resuming spawning)"));
    }
    else
    {
        CC_LOG_SPAWNER(Warning, TEXT("Cannot unfreeze - no enemy class set"));
    }
}

void ACC_EnemySpawner::FindPlayer()
{
    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    CachedPlayer = Cast<ACC_PlayerCharacter>(PlayerPawn);

    if (CachedPlayer)
    {
        CC_LOG_SPAWNER(Log, TEXT("Found player: %s"), *CachedPlayer->GetName());
    }
    else
    {
        CC_LOG_SPAWNER(Warning, TEXT("Could not find player"));
    }
}

void ACC_EnemySpawner::OnOwnerCubeTimeout()
{
    if (OwnerCube)
    {
        // 타임아웃 전에 이미 정상 연결됨 — SetOwnerCube()에서 타이머가 Clear됐어야 하지만
        // 혹시 모를 경합에 대한 방어적 처리
        return;
    }

    CC_LOG_SPAWNER(Warning,
        TEXT("[Spawner] OwnerCube link timed out after %.1fs — "
            "Freeze integration disabled. Falling back to auto-start."),
        OwnerCubeWaitTimeout);

    if (bAutoStart && EnemyClass)
    {
        RequestSpawningStart();
    }
}

void ACC_EnemySpawner::RequestSpawningStart()
{
    if (bIsSpawning)
    {
        return;
    }

    if (!CachedGameMode)
    {
        CachedGameMode = Cast<ACC_GameModeBase>(UGameplayStatics::GetGameMode(this));
    }

    if (CachedGameMode && !CachedGameMode->IsPlaying())
    {
        if (!bPendingAutoStart)
        {
            bPendingAutoStart = true;
            CachedGameMode->OnGameStateChanged.AddDynamic(this, &ACC_EnemySpawner::OnGameStateChangedHandler);
            CC_LOG_SPAWNER(Log,
                TEXT("[Spawner] Spawn condition met, but game hasn't truly started yet — "
                    "deferring until GameState -> Playing."));
        }
        return;
    }

    StartSpawning();
}

void ACC_EnemySpawner::OnGameStateChangedHandler(EGameState NewState)
{
    if (NewState != EGameState::Playing)
    {
        return;
    }

    if (CachedGameMode)
    {
        CachedGameMode->OnGameStateChanged.RemoveDynamic(this, &ACC_EnemySpawner::OnGameStateChangedHandler);
    }
    bPendingAutoStart = false;

    // Playing 전환까지 기다리는 동안 조건이 바뀌었을 수 있음
    // (예: 그 사이 다시 Frozen) — 실제 시작 전에 한 번 더 확인.
    if (!bIsSpawning && EnemyClass && (!OwnerCube || !OwnerCube->IsFrozen()))
    {
        StartSpawning();
    }
}
