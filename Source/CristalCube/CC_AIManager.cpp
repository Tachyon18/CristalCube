// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_AIManager.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Characters/CC_EnemyCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "CC_EnemyManager.h"


void UCC_AIManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogTemp, Log, TEXT("CristalCubeAIManager initialized"));
}

void UCC_AIManager::Deinitialize()
{
	// Clean up timer
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AIBatchUpdateTimer);
	}

	// Clear enemy arrays
	ActiveEnemies.Empty();
	ActiveAIEnemies.Empty();
	SleepingEnemies.Empty();

	UE_LOG(LogTemp, Log, TEXT("CristalCubeAIManager deinitialized"));

	Super::Deinitialize();
}

void UCC_AIManager::RegisterEnemy(ACC_EnemyCharacter* Enemy)
{
	if (!Enemy)
	{
		UE_LOG(LogTemp, Warning, TEXT("Attempted to register null enemy"));
		return;
	}

	if (ActiveEnemies.Contains(Enemy))
	{
		UE_LOG(LogTemp, Warning, TEXT("Enemy already registered: %s"), *Enemy->GetName());
		return;
	}

	ActiveEnemies.Add(Enemy);

	// Start the batch timer if this is the first enemy
	if (ActiveEnemies.Num() == 1)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				AIBatchUpdateTimer,
				this,
				&UCC_AIManager::BatchUpdateAI,
				AIUpdateFrequency,
				true  // Loop
			);

			UE_LOG(LogTemp, Log, TEXT("AI Manager started batch processing (%.2fs interval)"), AIUpdateFrequency);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Enemy registered: %s (Total: %d)"), *Enemy->GetName(), ActiveEnemies.Num());

}

void UCC_AIManager::UnregisterEnemy(ACC_EnemyCharacter* Enemy)
{
	if (!Enemy)
	{
		return;
	}

	// Remove from all arrays
	ActiveEnemies.Remove(Enemy);
	ActiveAIEnemies.Remove(Enemy);
	SleepingEnemies.Remove(Enemy);

	// Stop timer if no enemies left
	if (ActiveEnemies.Num() == 0)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(AIBatchUpdateTimer);
			UE_LOG(LogTemp, Log, TEXT("AI Manager stopped - no enemies remaining"));
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Enemy unregistered: %s (Remaining: %d)"), *Enemy->GetName(), ActiveEnemies.Num());

}

void UCC_AIManager::SetUpdateFrequency(float NewFrequency)
{
	AIUpdateFrequency = FMath::Clamp(NewFrequency, 0.05f, 1.0f);

	// Restart timer with new frequency if active
	if (ActiveEnemies.Num() > 0)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(AIBatchUpdateTimer);
			World->GetTimerManager().SetTimer(
				AIBatchUpdateTimer,
				this,
				&UCC_AIManager::BatchUpdateAI,
				AIUpdateFrequency,
				true
			);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("AI Manager frequency updated to %.2fs"), AIUpdateFrequency);

}

void UCC_AIManager::BatchUpdateAI()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UCristalCubeAIManager::BatchUpdateAI);

	float StartTime = FPlatformTime::Seconds();

	// Get player reference once
	ACC_PlayerCharacter* Player = GetPlayerCharacter();
	if (!Player)
	{
		return;
	}

	FVector PlayerLocation = Player->GetActorLocation();

	// Clear previous frame's AI arrays
	ActiveAIEnemies.Empty();
	SleepingEnemies.Empty();

	// Process all enemies in batch
	int32 ProcessedCount = 0;
	for (ACC_EnemyCharacter* Enemy : ActiveEnemies)
	{
		if (Enemy && Enemy->IsAlive())
		{
			UpdateEnemyAIState(Enemy, PlayerLocation);
			ProcessedCount++;
		}
	}

	// Performance tracking
	LastUpdateTime = FPlatformTime::Seconds() - StartTime;
	LastProcessedCount = ProcessedCount;

	// Debug output (remove in shipping build)
	if (ProcessedCount > 0)
	{
		UE_LOG(LogTemp, VeryVerbose, TEXT("AI Batch: %d enemies, %.4fms, Active: %d, Sleeping: %d"),
			ProcessedCount, LastUpdateTime * 1000.0f, ActiveAIEnemies.Num(), SleepingEnemies.Num());
	}
}

ACC_PlayerCharacter* UCC_AIManager::GetPlayerCharacter() const
{
	if (UWorld* World = GetWorld())
	{
		return Cast<ACC_PlayerCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0));
	}
	return nullptr;
}

void UCC_AIManager::UpdateEnemyAIState(ACC_EnemyCharacter* Enemy, const FVector& PlayerLocation)
{
	if (!Enemy) return;

	if (Enemy->GetIsFrozen()) return;

	FVector EnemyLocation = Enemy->GetActorLocation();

	// PERFORMANCE: Use DistSquared to avoid sqrt calculation
	float DistanceSquared = FVector::DistSquared(EnemyLocation, PlayerLocation);

	// Use individual enemy's DetectionRange (respects existing design)
	float DetectionRange = Enemy->GetDetectionRange();
	float DetectionRangeSquared = DetectionRange * DetectionRange;

	// Check if enemy should chase based on its own detection range
	if (DistanceSquared <= DetectionRangeSquared)
	{
		// Enemy should chase - set the existing bChasePlayer flag
		Enemy->SetChasePlayer(true);
		ActiveAIEnemies.Add(Enemy);
	}
	else
	{
		// Enemy too far or outside its detection range
		Enemy->SetChasePlayer(false);

		// Additional check: if way too far, put to sleep for performance
		float MaxAIRangeSquared = MaxAIRange * MaxAIRange;
		if (DistanceSquared > MaxAIRangeSquared)
		{
			SleepingEnemies.Add(Enemy);
		}
	}
}

bool UCC_AIManager::IsEnemyInAIRange(const FVector& EnemyLocation, const FVector& PlayerLocation) const
{
	return false;
}

UCC_AIManager* UCC_AIManager::Get(const UObject* WorldContextObject)
{
	if (UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject))
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			return GameInstance->GetSubsystem<UCC_AIManager>();
		}
	}
	return nullptr;
}

