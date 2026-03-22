// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_EnemyManager.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

ACC_EnemyManager* ACC_EnemyManager::Instance = nullptr;

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

	Instance = this;

	UE_LOG(LogTemp, Log, TEXT("[ENEMY MANAGER] Initialized"));
	
}

void ACC_EnemyManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{

    if (Instance == this)
    {
        Instance = nullptr;
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
    if (IsValid(Instance))
    {
        return Instance;
    }

	Instance = nullptr; // Clear invalid instance

    if (!WorldContextObject)
    {
        return nullptr;
    }

    UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

    if (!World)
    {
        return nullptr;
    }


    // Try to find existing instance
    for (TActorIterator<ACC_EnemyManager> It(World); It; ++It)
    {
        Instance = *It;
        break;
    }

    // Create if not found
    if (!Instance)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.Name = FName("EnemyManager");
        Instance = World->SpawnActor<ACC_EnemyManager>(ACC_EnemyManager::StaticClass(), SpawnParams);
        UE_LOG(LogTemp, Warning, TEXT("[ENEMY MANAGER] Auto-created instance"));
     }

    return Instance;
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
    OnEnemyUnregistered.Broadcast(Enemy);

    UE_LOG(LogTemp, VeryVerbose, TEXT("[ENEMY MANAGER] Unregistered enemy (Total: %d)"), ActiveEnemies.Num());

}

void ACC_EnemyManager::NotifyEnemyKilled(AActor* Enemy)
{
    if (!Enemy || PendingRemoval.Contains(Enemy))
    {
        return;
    }

    if (!ActiveEnemies.Contains(Enemy))
    {
        return;
    }

    PendingRemoval.Add(Enemy);
    OnEnemyKilled.Broadcast(Enemy);
    OnEnemyUnregistered.Broadcast(Enemy);

    UE_LOG(LogTemp, Log, TEXT("[ENEMY MANAGER] Enemy killed: %s"), *Enemy->GetName());
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
