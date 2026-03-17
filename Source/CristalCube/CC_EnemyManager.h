// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CC_EnemyManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyUnregistered, AActor*, Enemy);
/**
 * Central manager for all enemies
 * Provides optimized enemy queries for weapons
 */
UCLASS()
class CRISTALCUBE_API ACC_EnemyManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACC_EnemyManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
    // Singleton instance
    static ACC_EnemyManager* Instance;

    //==========================================================================
    // ENEMY TRACKING
    //==========================================================================

    // All active enemies
    UPROPERTY()
    TArray<AActor*> ActiveEnemies;

    // Cache for nearest enemy queries
    UPROPERTY()
    TMap<AActor*, AActor*> NearestEnemyCache;

    UPROPERTY()
    TSet<AActor*> PendingRemoval;

    // Update interval for cache (0.1s = 10 FPS)
    UPROPERTY(EditAnywhere, Category = "Performance")
    float CacheUpdateInterval = 0.1f;

    float TimeSinceLastUpdate = 0.0f;

public:
    //==========================================================================
    // PUBLIC INTERFACE
    //==========================================================================

    // Get singleton instance
    UFUNCTION(BlueprintPure, Category = "Enemy Manager", meta = (WorldContext = "WorldContextObject"))
    static ACC_EnemyManager* Get(const UObject* WorldContextObject);

    // Register enemy (called by enemy on spawn)
    UFUNCTION(BlueprintCallable, Category = "Enemy Manager")
    void RegisterEnemy(AActor* Enemy);

    // Unregister enemy (called by enemy on death/destroy)
    UFUNCTION(BlueprintCallable, Category = "Enemy Manager")
    void UnregisterEnemy(AActor* Enemy);

    // Get nearest enemy to location (FAST!)
    UFUNCTION(BlueprintPure, Category = "Enemy Manager")
    AActor* GetNearestEnemy(const FVector& Location, float MaxRadius = 10000.0f);

    // Get all enemies in radius
    UFUNCTION(BlueprintPure, Category = "Enemy Manager")
    TArray<AActor*> GetEnemiesInRadius(const FVector& Location, float Radius);

    // Get enemy count
    UFUNCTION(BlueprintPure, Category = "Enemy Manager")
    int32 GetEnemyCount() const { return ActiveEnemies.Num(); }

    // Get all enemies
    UFUNCTION(BlueprintPure, Category = "Enemy Manager")
    const TArray<AActor*>& GetAllEnemies() const { return ActiveEnemies; }

    UPROPERTY(BlueprintAssignable, Category = "Enemy Manager")
    FOnEnemyUnregistered OnEnemyUnregistered;

protected:
    // Update cache
    void UpdateNearestEnemyCache();
};
