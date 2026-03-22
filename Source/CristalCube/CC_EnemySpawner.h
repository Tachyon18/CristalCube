// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GamePlay/CC_Freezable.h"
#include "CC_EnemySpawner.generated.h"

class ACC_EnemyCharacter;
class ACC_PlayerCharacter;
class ACC_CycleManager;
struct FCycleConfig;

UCLASS()
class CRISTALCUBE_API ACC_EnemySpawner : public AActor, public ICC_Freezable
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACC_EnemySpawner();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
    //==========================================================================
    // SPAWN SETTINGS
    //==========================================================================

    /** Enemy class to spawn */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Setup")
    TSubclassOf<ACC_EnemyCharacter> EnemyClass;

    /** Time between spawns */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Settings")
    float SpawnInterval = 3.0f;

    /** How many enemies to spawn each time */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Settings")
    int32 EnemiesPerSpawn = 3;

    /** Minimum distance from player to spawn */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Settings")
    float MinSpawnDistance = 800.0f;

    /** Maximum distance from player to spawn */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Settings")
    float MaxSpawnDistance = 1200.0f;

    /** Maximum number of enemies alive at once */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Settings")
    int32 MaxEnemies = 50;

    /** Start spawning automatically on BeginPlay */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Settings")
    bool bAutoStart = true;

    /** Should increase spawn rate over time? */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Difficulty")
    bool bIncreaseSpawnRate = true;

    /** Spawn interval decrease per minute (makes spawns faster) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Difficulty")
    float SpawnIntervalDecreasePerMinute = 0.1f;

    /** Minimum spawn interval (won't go lower) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Difficulty")
    float MinSpawnInterval = 0.5f;

    //==========================================================================
    // STATE
    //==========================================================================

    /** Timer handle for spawning */
    FTimerHandle SpawnTimerHandle;

    /** Currently spawned enemies */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawner|State")
    TArray<ACC_EnemyCharacter*> SpawnedEnemies;

    /** Whether spawner is active */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawner|State")
    bool bIsSpawning;

    /** Current effective spawn interval */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawner|State")
    float CurrentSpawnInterval;

    /** Time since game started */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawner|State")
    float GameTime;

    /** Cached player reference */
    UPROPERTY()
    ACC_PlayerCharacter* CachedPlayer;

    /** Owner Cube reference */
    UPROPERTY()
    class ACC_Cube* OwnerCube;

    UPROPERTY()
    bool bIsFrozen = false;

    //==========================================================================
// SPAWNING
//==========================================================================

public:

/** Start spawning enemies */
    UFUNCTION(BlueprintCallable, Category = "Spawner")
    void StartSpawning();

    /** Stop spawning enemies */
    UFUNCTION(BlueprintCallable, Category = "Spawner")
    void StopSpawning();

    /** Spawn enemies immediately */
    UFUNCTION(BlueprintCallable, Category = "Spawner")
    void SpawnEnemies();

    /** Apply the current cycle's difficulty values to spawning behavior */
    void ApplyCycleConfig(const FCycleConfig& CycleConfig);

    /** Spawn a single enemy at location */
    UFUNCTION(BlueprintCallable, Category = "Spawner")
    ACC_EnemyCharacter* SpawnSingleEnemy(const FVector& Location);

    /** Get random spawn location around player */
    UFUNCTION(BlueprintCallable, Category = "Spawner")
    FVector GetRandomSpawnLocation() const;

    /** Clean up dead/null enemies from array */
    UFUNCTION(BlueprintCallable, Category = "Spawner")
    void CleanupDeadEnemies();

    /** Update spawn interval based on game time */
    void UpdateSpawnInterval();

    //==========================================================================
    // GETTERS
    //==========================================================================

    UFUNCTION(BlueprintPure, Category = "Spawner")
    int32 GetAliveEnemyCount() const;

    UFUNCTION(BlueprintPure, Category = "Spawner")
    bool CanSpawnMore() const;

    UFUNCTION(BlueprintPure, Category = "Spawner")
    ACC_PlayerCharacter* GetPlayer() const { return CachedPlayer; }

    /** Set owner cube for spawn location */
    UFUNCTION(BlueprintCallable, Category = "Spawner")
    void SetOwnerCube(class ACC_Cube* Cube) { OwnerCube = Cube; }

    /** Get owner cube */
    UFUNCTION(BlueprintPure, Category = "Spawner")
    class ACC_Cube* GetOwnerCube() const { return OwnerCube; }

    //==========================================================================
    // FREEZABLE INTERFACE
    //==========================================================================

    virtual void Freeze_Implementation() override;
    virtual void Unfreeze_Implementation() override;
    virtual bool IsFrozen_Implementation() const override { return bIsFrozen; }

protected:
    //==========================================================================
    // INTERNAL
    //==========================================================================

    /** Find and cache player reference */
    void FindPlayer();

    /** Pull the active cycle config if a cycle is already running */
    void SyncWithActiveCycle();

    /** Find the current cycle manager in the world */
    ACC_CycleManager* FindCycleManager() const;

    /** Rebuild the active spawn timer with the current interval */
    void RefreshSpawnTimer();
};
