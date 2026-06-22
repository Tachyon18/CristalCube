// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_EnemyManager.h"
#include "Kismet/GameplayStatics.h"
#include "Gameplay/CC_EnemyAIInterface.h"
#include "EngineUtils.h"

TWeakObjectPtr<ACC_EnemyManager> ACC_EnemyManager::WeakInstance = nullptr;

// Sets default values
ACC_EnemyManager::ACC_EnemyManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = CacheUpdateInterval;
}

// Called when the game starts or when spawned
void ACC_EnemyManager::BeginPlay()
{
	Super::BeginPlay();

	WeakInstance = this;

	UE_LOG(LogTemp, Log, TEXT("[ENEMY MANAGER] Initialized"));
	
}

void ACC_EnemyManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{

    if (WeakInstance == this)
    {
        WeakInstance = nullptr;
    }

    ActiveEnemies.Empty();
    NearestEnemyCache.Empty();
    PendingRemoval.Reset();

    UE_LOG(LogTemp, Log, TEXT("[ENEMY MANAGER] EndPlay — Instance cleared, arrays emptied"));

    Super::EndPlay(EndPlayReason);
}

// Called every frame
void ACC_EnemyManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TimeSinceLastUpdate += DeltaTime;

	if (TimeSinceLastUpdate >= CacheUpdateInterval)
	{
		UpdateNearestEnemyCache();
		TimeSinceLastUpdate = 0.0f;
	}

}

ACC_EnemyManager* ACC_EnemyManager::Get(const UObject* WorldContextObject)
{
    if (!WorldContextObject) return nullptr;

    UWorld* CallerWorld = WorldContextObject->GetWorld();
    if (!CallerWorld) return nullptr;

    // WeakObjectPtr — GC·PIE 종료 시 자동 null, GetWorld() 호출 없이 안전
    ACC_EnemyManager* Pinned = WeakInstance.Get();
    if (Pinned && !CallerWorld->bIsTearingDown
        && Pinned->GetWorld() == CallerWorld)
    {
        return Pinned;
    }

    // 다른 세션의 오염된 포인터 제거
    WeakInstance = nullptr;

    // 레벨에 배치된 EnemyManager 탐색
    for (TActorIterator<ACC_EnemyManager> It(CallerWorld); It; ++It)
    {
        WeakInstance = *It;
        UE_LOG(LogTemp, Log, TEXT("[EnemyManager] Instance found via iterator."));
        return WeakInstance.Get();
    }

    // 자동 생성 제거 — 레벨에 BP_EnemyManager가 없으면 null 반환
    UE_LOG(LogTemp, Error,
        TEXT("[EnemyManager] No instance in level! "
            "Place BP_EnemyManager in the level before PIE."));
    return nullptr;
}

void ACC_EnemyManager::RegisterEnemy(AActor* Enemy)
{
    if (!Enemy || ActiveEnemies.Contains(Enemy))
    {
        return;
    }

    ActiveEnemies.Add(Enemy);

    UE_LOG(LogTemp, VeryVerbose, TEXT("[ENEMY MANAGER] Registered enemy (Total: %d)"), ActiveEnemies.Num());

}

void ACC_EnemyManager::UnregisterEnemy(AActor* Enemy)
{
    if (!Enemy)
    {
        return;
    }

    PendingRemoval.Add(Enemy);

    // CycleManager에 킬 알림 (직접 참조 없이 델리게이트로)
    OnEnemyUnregistered.Broadcast(Enemy);

    UE_LOG(LogTemp, VeryVerbose, TEXT("[ENEMY MANAGER] Unregistered enemy for cleanup: %s (Tracked: %d)"), *GetNameSafe(Enemy), ActiveEnemies.Num());
}

void ACC_EnemyManager::ReportEnemyKilled(AActor* Enemy)
{
    if (!Enemy)
    {
        return;
    }

    if (!ActiveEnemies.Contains(Enemy))
    {
        UE_LOG(LogTemp, Warning, TEXT("[ENEMY MANAGER] Ignored kill report for untracked enemy: %s"),
            *GetNameSafe(Enemy));
        return;
    }

    OnEnemyKilled.Broadcast(Enemy);

    UE_LOG(LogTemp, Log, TEXT("[ENEMY MANAGER] Confirmed enemy death: %s"), *GetNameSafe(Enemy));

}

AActor* ACC_EnemyManager::GetNearestEnemy(const FVector& Location, float MaxRadius)
{
    if (ActiveEnemies.Num() == 0)
    {
        return nullptr;
    }

    AActor* NearestEnemy = nullptr;
    float NearestDistSq = MaxRadius * MaxRadius;

    for (AActor* Enemy : ActiveEnemies)
    {
        if (!Enemy || !IsValid(Enemy))
        {
            continue;
        }

        if (ICC_EnemyAIInterface::Execute_GetIsFrozen(Enemy)) continue;

        float DistSq = FVector::DistSquared(Location, Enemy->GetActorLocation());

        if (DistSq < NearestDistSq)
        {
            NearestDistSq = DistSq;
            NearestEnemy = Enemy;
        }
    }

    return NearestEnemy;
}

TArray<AActor*> ACC_EnemyManager::GetEnemiesInRadius(const FVector& Location, float Radius)
{
    TArray<AActor*> Result;
    float RadiusSq = Radius * Radius;

    for (AActor* Enemy : ActiveEnemies)
    {
        if (!Enemy || !IsValid(Enemy))
        {
            continue;
        }

        if (ICC_EnemyAIInterface::Execute_GetIsFrozen(Enemy)) continue;

        float DistSq = FVector::DistSquared(Location, Enemy->GetActorLocation());

        if (DistSq <= RadiusSq)
        {
            Result.Add(Enemy);
        }
    }

    return Result;
}

void ACC_EnemyManager::UpdateNearestEnemyCache()
{
    if (PendingRemoval.Num() > 0)
    {
        for (AActor* Enemy : PendingRemoval)
        {
            ActiveEnemies.RemoveSingleSwap(Enemy, EAllowShrinking::No); // O(1) 제거
            NearestEnemyCache.Remove(Enemy);
        }
        PendingRemoval.Reset(); // 내용만 비움 (메모리 유지)
        ActiveEnemies.Shrink(); // 제거 후 메모리 정리
    }

    // Remove invalid enemies
    ActiveEnemies.RemoveAll([](AActor* Enemy) {
        return !IsValid(Enemy);
        });

	// Update cache for all valid enemies
    for (auto It = NearestEnemyCache.CreateIterator(); It; ++It)
    {
        if (!IsValid(It.Key()) || !IsValid(It.Value()))
        {
            It.RemoveCurrent();
        }
    }
}
