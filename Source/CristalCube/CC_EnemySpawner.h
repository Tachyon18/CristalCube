// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GamePlay/CC_Freezable.h"
#include "CC_EnemySpawner.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWaveCleared, ACC_EnemySpawner*, Spawner);

class ACC_PlayerCharacter;
class ACC_GameModeBase;

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
    TSubclassOf<APawn> EnemyClass;

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

    /** OwnerCube가 이 시간(초) 안에 연결되지 않으면 경고 로그 후 fallback 스폰 (Freeze 연동 없이) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Settings")
    float OwnerCubeWaitTimeout = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Wave")
    int32 WaveSize = 20;

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

    /** OwnerCube 연결 대기 타임아웃 안전장치 핸들 */
    FTimerHandle OwnerCubeTimeoutHandle;

    /** Currently spawned enemies */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawner|State")
    TArray<APawn*> SpawnedEnemies;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawner|Wave")
	int32 TotalSpawnedThisWave = 0;

    /** Whether spawner is active */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawner|State")
    bool bIsSpawning;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawner|Wave")
    bool bWaveCleared = false;

    UPROPERTY()
    bool bHasWaveStarted = false;

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

    /** 게임이 진짜 시작(Playing)될 때까지 스폰 개시를 미루고 있는 중인지 */
    UPROPERTY()
    bool bPendingAutoStart = false;

    /** GameState 신호 조회/구독용 캐시 */
    UPROPERTY()
    class ACC_GameModeBase* CachedGameMode = nullptr;

public:
    
    UPROPERTY(BlueprintAssignable, Category = "Spawner|Wave")
    FOnWaveCleared OnWaveCleared;

    UFUNCTION(BlueprintPure, Category = "Spawner|Wave")
    bool IsWaveFinishedSpawning() const { return TotalSpawnedThisWave >= WaveSize; }

    UFUNCTION(BlueprintPure, Category = "Spawner|Wave")
    bool IsWaveCleared() const { return bWaveCleared; }

protected:
    void CheckWaveClearCondition();

public:

    //==========================================================================
    // SPAWNING
    //==========================================================================

    /** Start spawning enemies */
    UFUNCTION(BlueprintCallable, Category = "Spawner")
    void StartSpawning();

    /** Stop spawning enemies */
    UFUNCTION(BlueprintCallable, Category = "Spawner")
    void StopSpawning();

    /** Spawn enemies immediately */
    UFUNCTION(BlueprintCallable, Category = "Spawner")
    void SpawnEnemies();

    /** Spawn a single enemy at location */
    UFUNCTION(BlueprintCallable, Category = "Spawner")
    APawn* SpawnSingleEnemy(const FVector& Location);

    /** Get random spawn location around player */
    UFUNCTION(BlueprintCallable, Category = "Spawner")
    FVector GetRandomSpawnLocation() const;

    /** Clean up dead/null enemies from array */
    UFUNCTION(BlueprintCallable, Category = "Spawner")
    void CleanupDeadEnemies();

    /** Update spawn interval based on game time */
    void UpdateSpawnInterval();

public:

    //==========================================================================
    // GETTERS
    //==========================================================================

    UFUNCTION(BlueprintPure, Category = "Spawner")
    int32 GetAliveEnemyCount() const;

    UFUNCTION(BlueprintPure, Category = "Spawner")
    bool CanSpawnMore() const;

    /** Set owner cube for spawn location */
    UFUNCTION(BlueprintCallable, Category = "Spawner")
    void SetOwnerCube(class ACC_Cube* Cube);

    UFUNCTION(BlueprintPure, Category = "Spawner")
    ACC_PlayerCharacter* GetPlayer() const { return CachedPlayer; }

    /** Get owner cube */
    UFUNCTION(BlueprintPure, Category = "Spawner")
    class ACC_Cube* GetOwnerCube() const { return OwnerCube; }

	UFUNCTION(BlueprintPure, Category = "Spawner")
	bool GetIsAutoStart() const { return bAutoStart; }

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

    /** OwnerCube 대기 타임아웃 콜백 — 그 시점에도 OwnerCube가 없으면 fallback 스폰 */
    void OnOwnerCubeTimeout();

    /** 실제 스폰 개시 요청 — GameState가 Playing이 아니면 지금 시작하지 않고
    Playing 전환 신호를 받을 때까지 미룬다. */
    void RequestSpawningStart();

    /** GameModeBase::OnGameStateChanged 수신 콜백 */
    UFUNCTION()
    void OnGameStateChangedHandler(EGameState NewState);
};
