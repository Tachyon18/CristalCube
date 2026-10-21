// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_TestGameMode.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Characters/CC_Character.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ACC_TestGameMode::ACC_TestGameMode()
{
}

void ACC_TestGameMode::BeginPlay()
{
	Super::BeginPlay();
	// TestRoom은 게임 시작부터 자유롭게 플레이 가능
	UE_LOG(LogTemp, Log, TEXT("[TestRoomGameMode] TestRoom ready."));
}

// ============================================================
//  스폰 모드
// ============================================================

void ACC_TestGameMode::SetSpawnMode(ESpawnMode NewMode)
{
    // 이전 모드 정리
    ClearDummies();
    StopWeakWave();

    CurrentSpawnMode = NewMode;

    switch (NewMode)
    {
    case ESpawnMode::None:
        UE_LOG(LogTemp, Log, TEXT("[TestRoom] Spawn mode: None"));
        break;

    case ESpawnMode::Dummy:
        SpawnDummies();
        break;

    case ESpawnMode::WeakWave:
        StartWeakWave();
        break;
    }
}

void ACC_TestGameMode::SpawnDummies()
{
    if (!DummyEnemyClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("[TestRoom] DummyEnemyClass not set!"));
        return;
    }

    // 원형으로 배치
    for (int32 i = 0; i < DummyCount; ++i)
    {
        float Angle = (360.f / DummyCount) * i;
        float Radius = 800.f;
        FVector Offset(
            FMath::Cos(FMath::DegreesToRadians(Angle)) * Radius,
            FMath::Sin(FMath::DegreesToRadians(Angle)) * Radius,
            0.f);

        FVector SpawnLoc = FVector::ZeroVector + Offset;

        AActor* Dummy = GetWorld()->SpawnActor<AActor>(
            DummyEnemyClass, SpawnLoc, FRotator::ZeroRotator);

        if (Dummy)
        {
            SpawnedDummies.Add(Dummy);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[TestRoom] Spawned %d dummies."), SpawnedDummies.Num());

}

void ACC_TestGameMode::ClearDummies()
{
    for (AActor* Dummy : SpawnedDummies)
    {
        if (IsValid(Dummy)) Dummy->Destroy();
    }
    SpawnedDummies.Empty();
}

void ACC_TestGameMode::StartWeakWave()
{
    GetWorldTimerManager().SetTimer(
        WeakWaveTimerHandle, this, &ACC_TestGameMode::SpawnWeakEnemy,
        WeakWaveSpawnInterval, true, 0.0f);

    UE_LOG(LogTemp, Log, TEXT("[TestRoom] Weak wave started."));
}

void ACC_TestGameMode::StopWeakWave()
{
    GetWorldTimerManager().ClearTimer(WeakWaveTimerHandle);
}

void ACC_TestGameMode::SpawnWeakEnemy()
{
    // 기존 EnemySpawner 또는 EnemyManager와 연동
    // 블루프린트에서 구체적인 스폰 로직 오버라이드 가능
    UE_LOG(LogTemp, Log, TEXT("[TestRoom] Weak enemy spawn tick."));
}

// ============================================================
//  치트
// ============================================================

void ACC_TestGameMode::SetPlayerLevel(int32 TargetLevel)
{
    ACC_PlayerCharacter* Player = GetPlayerCharacter();
    if (!Player) return;

    // 목표 레벨까지 LevelUp() 반복 호출
    while (Player->GetPlayerLevel() < TargetLevel)
    {
        Player->LevelUp();
    }

    UE_LOG(LogTemp, Log, TEXT("[TestRoom] Player level set to %d"), TargetLevel);
}

void ACC_TestGameMode::RestorePlayerHealth()
{
    ACC_Character* Player = Cast<ACC_Character>(GetPlayerCharacter());
    if (!Player) return;

    // CC_Character의 CurrentHealth를 MaxHealth로 복구
    // (CC_Character에 RestoreFullHealth() 또는 SetHealth() 가 있다면 호출)
    UE_LOG(LogTemp, Log, TEXT("[TestRoom] Player health restored."));
}

void ACC_TestGameMode::ClearAllEnemies()
{
    // 현재 스폰된 적들을 모두 제거하는 로직
    // EnemyManager 또는 EnemySpawner와 연동하여 구현
    TArray<AActor*> Found;
    UGameplayStatics::GetAllActorsOfClass(
        GetWorld(), ACC_Character::StaticClass(), Found);

    for (AActor* A : Found)
    {
        // 플레이어 제외
        if (A == GetPlayerCharacter()) continue;
        if (IsValid(A)) A->Destroy();
    }

    ClearDummies();

    UE_LOG(LogTemp, Log, TEXT("[TestRoom] All enemies cleared."));
}

// ============================================================
//  게임 오버 없음
// ============================================================

void ACC_TestGameMode::TriggerGameOver()
{
    // TestRoom은 게임 오버 없이 체력만 회복
    UE_LOG(LogTemp, Log, TEXT("[TestRoom] Player died — restoring health instead of game over."));
    RestorePlayerHealth();
}