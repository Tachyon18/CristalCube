// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CC_AIManager.generated.h"


/**
 * Centralized AI Management System for Enemy Characters
 * Handles batch processing of AI updates for optimal performance
 * Supports spatial optimization and LOD systems
 */
UCLASS()
class CRISTALCUBE_API UCC_AIManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// --- 등록/해제: AActor* 기반으로 통일 ---
	UFUNCTION(BlueprintCallable, Category = "AI Manager")
	void RegisterEnemy(AActor* Enemy);

	UFUNCTION(BlueprintCallable, Category = "AI Manager")
	void UnregisterEnemy(AActor* Enemy);

	// AI Update Settings
	UFUNCTION(BlueprintCallable, Category = "AI Manager")
	void SetUpdateFrequency(float NewFrequency);

	// Debug Information
	UFUNCTION(BlueprintPure, Category = "AI Manager")
	int32 GetActiveEnemyCount() const { return ActiveEnemies.Num(); }

	UFUNCTION(BlueprintPure, Category = "AI Manager")
	int32 GetActiveAICount() const { return ActiveAIEnemies.Num(); }	

protected:
	// Core AI Processing
	UFUNCTION()
	void BatchUpdateAI();

	// Helper Functions
	class ACC_PlayerCharacter* GetPlayerCharacter() const;

	bool ImplementsEnemyAI(AActor* Enemy) const;

	bool IsEnemyInAIRange(const FVector& EnemyLocation, const FVector& PlayerLocation) const;

protected:

	// Enemy Collections
	UPROPERTY()
	TArray<AActor*> ActiveEnemies;

	UPROPERTY()
	TArray<AActor*> ActiveAIEnemies;

	UPROPERTY()
	TArray<AActor*> SleepingEnemies;

	// Timer Management
	UPROPERTY()
	FTimerHandle AIBatchUpdateTimer;

	// Configuration
	UPROPERTY(EditAnywhere, Category = "AI Settings", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float AIUpdateFrequency = 0.16f;  // 10 updates per second

	UPROPERTY(EditAnywhere, Category = "AI Settings")
	float MaxAIRange = 3000.0f;  // 30m - beyond this, enemies sleep

	UPROPERTY(EditAnywhere, Category = "AI Settings")
	float HighPriorityRange = 1500.0f;  // 15m - high priority enemies

	// Performance Tracking
	UPROPERTY(VisibleAnywhere, Category = "Debug")
	float LastUpdateTime = 0.0f;

	UPROPERTY(VisibleAnywhere, Category = "Debug")
	int32 LastProcessedCount = 0;

public:
	// Static Access (for convenience)
	UFUNCTION(BlueprintPure, Category = "AI Manager", meta = (WorldContext = "WorldContextObject"))
	static UCC_AIManager* Get(const UObject* WorldContextObject);

};
