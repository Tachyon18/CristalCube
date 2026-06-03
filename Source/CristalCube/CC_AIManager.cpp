// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_AIManager.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Characters/CC_EnemyCharacter.h"
#include "Gameplay/CC_EnemyAIInterface.h"
#include "Gameplay/CC_EnemyBase.h"
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

	//UE_LOG(LogTemp, Log, TEXT("CristalCubeAIManager deinitialized"));

	Super::Deinitialize();
}

void UCC_AIManager::RegisterEnemy(AActor* Enemy)
{
	if (!ImplementsEnemyAI(Enemy))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[AIManager] RegisterEnemy 실패: %s 가 ICC_EnemyAIInterface를 구현하지 않음"),
			*GetNameSafe(Enemy));
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

void UCC_AIManager::UnregisterEnemy(AActor* Enemy)
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
			//UE_LOG(LogTemp, Log, TEXT("AI Manager stopped - no enemies remaining"));
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Enemy unregistered: %s (Remaining: %d)"), *GetNameSafe(Enemy), ActiveEnemies.Num());

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

	//float StartTime = FPlatformTime::Seconds();

	// Get player reference once
	ACC_PlayerCharacter* Player = GetPlayerCharacter();
	if (!Player) return;

	const FVector PlayerLocation = Player->GetActorLocation();
	const float MaxAIRangeSq = MaxAIRange * MaxAIRange;

	// Clear previous frame's AI arrays
	ActiveAIEnemies.Empty();
	SleepingEnemies.Empty();

	//// Process all enemies in batch
	//int32 ProcessedCount = 0;
	//for (ACC_EnemyCharacter* Enemy : ActiveEnemies)
	//{
	//	if (Enemy && Enemy->IsAlive())
	//	{
	//		UpdateEnemyAIState(Enemy, PlayerLocation);
	//		ProcessedCount++;
	//	}
	//}

	//// Performance tracking
	//LastUpdateTime = FPlatformTime::Seconds() - StartTime;
	//LastProcessedCount = ProcessedCount;

	//// Debug output (remove in shipping build)
	//if (ProcessedCount > 0)
	//{
	//	UE_LOG(LogTemp, VeryVerbose, TEXT("AI Batch: %d enemies, %.4fms, Active: %d, Sleeping: %d"),
	//		ProcessedCount, LastUpdateTime * 1000.0f, ActiveAIEnemies.Num(), SleepingEnemies.Num());
	//}

	// ─── 1패스: 유효하지 않은 적 먼저 제거 ──────────────────────────────
	ActiveEnemies.RemoveAll([this](const AActor* E)
		{
			if (!IsValid(E)) return true;
			return !ICC_EnemyAIInterface::Execute_IsEnemyAlive(E);
		});

	// ─── 2패스: 거리 기반 상태 분류 (DistSquared, sqrt 없음) ─────────────
	for (AActor* Enemy : ActiveEnemies)
	{
		// 동결 상태면 스킵
		if (ICC_EnemyAIInterface::Execute_GetIsFrozen(Enemy)) continue;

		const float DistSq = FVector::DistSquared(Enemy->GetActorLocation(), PlayerLocation);
		const float DetectRange = ICC_EnemyAIInterface::Execute_GetDetectionRange(Enemy);
		const float DetectSq = DetectRange * DetectRange;

		if (DistSq <= DetectSq)
		{
			ICC_EnemyAIInterface::Execute_SetChasePlayer(Enemy, true);
			ActiveAIEnemies.Add(Enemy);
		}
		else
		{
			ICC_EnemyAIInterface::Execute_SetChasePlayer(Enemy, false);
			if (DistSq > MaxAIRangeSq)
				SleepingEnemies.Add(Enemy);
		}
	}

	// 3패스: 이동 + 공격 체크 (Active 적만)
	for (AActor* Enemy : ActiveAIEnemies)
	{
		if (ICC_EnemyAIInterface::Execute_GetIsFrozen(Enemy)) continue;

		// EnemyBase — 직접 호출
		if (ACC_EnemyBase* Base = Cast<ACC_EnemyBase>(Enemy))
		{
			Base->PerformMove();
			Base->CheckAndPerformAttack();
		}
		// EnemyCharacter — 기존 Tick이 이동 처리. 공격은 자체 Overlap 유지 (Phase 3에서 통일)
	}

	LastProcessedCount = ActiveAIEnemies.Num();

}

ACC_PlayerCharacter* UCC_AIManager::GetPlayerCharacter() const
{
	if (UWorld* World = GetWorld())
	{
		return Cast<ACC_PlayerCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0));
	}
	return nullptr;
}

bool UCC_AIManager::ImplementsEnemyAI(AActor* Enemy) const
{
	return IsValid(Enemy) &&
		Enemy->GetClass()->ImplementsInterface(UCC_EnemyAIInterface::StaticClass());
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

