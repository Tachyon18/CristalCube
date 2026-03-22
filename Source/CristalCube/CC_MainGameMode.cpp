// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_MainGameMode.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ACC_MainGameMode::ACC_MainGameMode()
{
    CycleManagerClass = ACC_CycleManager::StaticClass();
}

void ACC_MainGameMode::BeginPlay()
{
    Super::BeginPlay(); // SetGameState(Playing) 호출됨

    SpawnCycleManager();
}

// ============================================================
//  사이클 매니저
// ============================================================

void ACC_MainGameMode::SpawnCycleManager()
{
    if (!CycleManagerClass) return;

	FActorSpawnParameters Params;
	Params.Owner = this;

    CycleManager = GetWorld()->SpawnActor<ACC_CycleManager>(
        CycleManagerClass, FVector::ZeroVector, FRotator::ZeroRotator, Params);

    if (CycleManager)
    {
        // 사이클 완료 이벤트 바인딩
        CycleManager->OnCubeCleared.AddDynamic(
            this, &ACC_MainGameMode::OnCubeCleared);

        CycleManager->StartFirstCycle();

        UE_LOG(LogTemp, Log, TEXT("[GameMode] CycleManager spawned and first cycle started."));
    }
}

// ============================================================
//  Cube Clear
// ============================================================

void ACC_MainGameMode::OnCubeCleared(int32 ClearedCycle)
{
    UE_LOG(LogTemp, Warning,
        TEXT("[MainGameMode] Cube %d cleared — showing reward UI"), ClearedCycle);

    SetGameState(EGameState::LevelUp); // LevelUp 상태 재활용 (입력 차단 목적)
    ShowCubeClearUI(ClearedCycle);
}

void ACC_MainGameMode::ShowCubeClearUI(int32 ClearedCycle)
{
    if (!CubeClearWidgetClass)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[MainGameMode] CubeClearWidgetClass not set! Auto-proceeding."));

        HideCubeClearUI();
        SetGameState(EGameState::Playing);

        if (CycleManager)
        {
            CycleManager->StartNextCycle();
        }

        return;
    }

    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (!PC) return;

    CurrentCubeClearWidget = CreateWidget<UUserWidget>(PC, CubeClearWidgetClass);
    if (CurrentCubeClearWidget)
    {
        CurrentCubeClearWidget->AddToViewport();
        UGameplayStatics::SetGamePaused(GetWorld(), true);
        PC->SetInputMode(FInputModeUIOnly());
        PC->SetShowMouseCursor(true);
    }
}

void ACC_MainGameMode::HideCubeClearUI()
{
    if (CurrentCubeClearWidget)
    {
        CurrentCubeClearWidget->RemoveFromParent();
        CurrentCubeClearWidget = nullptr;
    }

    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (PC)
    {
        UGameplayStatics::SetGamePaused(GetWorld(), false);
        PC->SetInputMode(FInputModeGameOnly());
        PC->SetShowMouseCursor(false);
    }
}

void ACC_MainGameMode::OnCubeClearRewardSelected(FCubeClearReward SelectedReward)
{
    HideCubeClearUI();
    SetGameState(EGameState::Playing);

    const bool bRewardApplied = ApplyCubeClearRewardToPlayer(SelectedReward);

    UE_LOG(LogTemp, Log,
        TEXT("[MainGameMode] Reward selected: %s (%s). Applied=%s. Starting next cycle."),
        *SelectedReward.DisplayName.ToString(),
        *UEnum::GetValueAsString(SelectedReward.RewardType),
        bRewardApplied ? TEXT("true") : TEXT("false"));

    if (CycleManager)
    {
        CycleManager->StartNextCycle();
    }
}

TArray<FCubeClearReward> ACC_MainGameMode::GetRandomCubeClearRewards(int32 Count)
{
    TArray<FCubeClearReward> Result;
    if (CubeClearRewardPool.Num() == 0) return Result;

    // 풀 복사 후 셔플
    TArray<FCubeClearReward> Pool = CubeClearRewardPool;
    for (int32 i = Pool.Num() - 1; i > 0; --i)
    {
        int32 j = FMath::RandRange(0, i);
        Pool.Swap(i, j);
    }

    int32 Take = FMath::Min(Count, Pool.Num());
    for (int32 i = 0; i < Take; ++i)
    {
        Result.Add(Pool[i]);
    }

    return Result;
}


// ============================================================
//  게임 오버
// ============================================================

void ACC_MainGameMode::TriggerGameOver()
{
    Super::TriggerGameOver();

    SetGameState(EGameState::GameOver);
    if (GameOverDelay > 0.f)
    {
        GetWorldTimerManager().SetTimer(
            GameOverTimerHandle, this, &ACC_MainGameMode::RestartCurrentLevel, GameOverDelay, false);
    }
    // GameOverDelay == 0 이면 블루프린트(WBP_GameOver)에서 수동 처리
}

bool ACC_MainGameMode::ApplyCubeClearRewardToPlayer(const FCubeClearReward& SelectedReward)
{
    ACC_PlayerCharacter* PlayerCharacter = GetPlayerCharacter();
    if (!PlayerCharacter)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[MainGameMode] Failed to apply Cube Clear reward: player character not found."));
        return false;
    }

    return PlayerCharacter->ApplyCubeClearReward(SelectedReward);
}
