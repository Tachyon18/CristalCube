// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_MainGameMode.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "CC_PlayerController.h"
#include "CC_PlayerState.h"
#include "SkillSystem/CC_SkillBase.h"
#include "SkillSystem/CC_SkillLibrarySubsystem.h"
#include "Widgets/CC_GameHUD.h"
#include "Widgets/CC_CubeClearRewardWidget.h"
#include "Widgets/CC_RunResultWidget.h"
#include "TimerManager.h"

ACC_MainGameMode::ACC_MainGameMode()
{
    CycleManagerClass = ACC_CycleManager::StaticClass();
}

void ACC_MainGameMode::BeginPlay()
{
    Super::BeginPlay(); // Cube 시스템 준비 신호 대기 후 Playing 전환 (Startup_Pipeline_Design 8절)

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
        CycleManager->OnCubeClearAchieved.AddDynamic(this, &ACC_MainGameMode::OnCubeClearAchieved);
        CycleManager->OnGameCleared.AddDynamic(this, &ACC_MainGameMode::OnGameCleared);

        CycleManager->StartCubeClearTracking();

        UE_LOG(LogTemp, Log, TEXT("[GameMode] CycleManager spawned and Cube Clear tracking started."));
    }
}

void ACC_MainGameMode::ApplyCubeClearReward(const FCubeClearReward& SelectedReward)
{
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    ACC_PlayerCharacter* Player = PC ? Cast<ACC_PlayerCharacter>(PC->GetPawn()) : nullptr;
    ACC_PlayerState* PS = PC ? PC->GetPlayerState<ACC_PlayerState>() : nullptr;

    switch (SelectedReward.RewardType)
    {
    case ECubeClearRewardType::StatBoost:
        if (Player && SelectedReward.StatUpgradeType != EUpgradeType::None)
        {
            Player->ApplyStatUpgrade(SelectedReward.StatUpgradeType, SelectedReward.StatValue);
        }
        break;

    case ECubeClearRewardType::AddonGrant:
        if (PS)
        {
            TArray<UCC_SkillBase*> Candidates;
            for (UCC_SkillBase* Skill : PS->GetAllSkills())
            {
                if (Skill && !Skill->HasAddon(SelectedReward.TargetAddonType))
                {
                    Candidates.Add(Skill);
                }
            }
            if (Candidates.Num() > 0)
            {
                PS->GrantAddonWithCatchUp(Candidates[FMath::RandRange(0, Candidates.Num() - 1)], SelectedReward.TargetAddonType);
            }
        }
        break;

    case ECubeClearRewardType::AddonUpgrade:
        if (PS)
        {
            PS->AddAddonPoints(SelectedReward.TargetAddonType, SelectedReward.AddonPointAmount);
        }
        break;

    default:
        break;
    }

    UE_LOG(LogTemp, Log, TEXT("[MainGameMode] Reward applied: %s"),
        *SelectedReward.DisplayName.ToString());
}

void ACC_MainGameMode::HandleCubeClearRewardPicked(FCubeClearReward SelectedReward)
{
    ApplyCubeClearReward(SelectedReward);

    PendingRewardSlots = FMath::Max(0, PendingRewardSlots - 1);
    RefreshCubeClearRewardBar();

    if (PendingRewardSlots > 0 && CurrentCubeClearRewardWidget)
    {
        // 패널 안 닫고 다음 3장으로 즉시 이어감
        CurrentCubeClearRewardWidget->SetRewardChoices(GetRandomCubeClearRewards(3));
    }
    else
    {
        CloseCubeClearRewardPanel();
    }
}

void ACC_MainGameMode::CloseCubeClearRewardPanel()
{
    if (CurrentCubeClearRewardWidget)
    {
        CurrentCubeClearRewardWidget->RemoveFromParent();
        CurrentCubeClearRewardWidget = nullptr;
    }

    UGameplayStatics::SetGamePaused(GetWorld(), false);

    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (PC)
    {
        PC->bShowMouseCursor = false;
        PC->SetInputMode(FInputModeGameOnly());
    }
}

void ACC_MainGameMode::OnCubeClearAchieved(int32 StageNumber)
{
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    ACC_PlayerState* PS = PC ? PC->GetPlayerState<ACC_PlayerState>() : nullptr;
    if (!PS) return;

    PS->AddCubeEnergy(CubeEnergyPerCubeClear);

    // HealFull 대체 — 픽 카드가 아니라 CubeClear마다 자동으로 붙는 부분회복
    if (ACC_PlayerCharacter* Player = Cast<ACC_PlayerCharacter>(PC->GetPawn()))
    {
        Player->Heal(Player->GetMaxHealth() * CubeClearAutoHealPercent);
    }

    const int32 NewPicks = PS->ConsumeCubeClearPicks();
    PendingRewardSlots += NewPicks;

    UE_LOG(LogTemp, Log,
        TEXT("[MainGameMode] Cube Clear (stage %d) — +%.1f energy, +%d reward slot(s), pending: %d"),
        StageNumber, CubeEnergyPerCubeClear, NewPicks, PendingRewardSlots);

    RefreshCubeClearRewardBar();
}

void ACC_MainGameMode::OnGameCleared()
{
    // 사망 처리 중(결과 화면 대기)이면 클리어로 뒤집지 않음
    if (CurrentGameState == EGameState::GameOver) return;

    UE_LOG(LogTemp, Warning, TEXT("[MainGameMode] === GAME CLEARED (Boss defeated) ==="));

    SetGameState(EGameState::GameClear);
    ShowRunResult(true);
}

void ACC_MainGameMode::ShowPendingRunResult()
{
    if (CurrentRunResultWidget) return; // 이미 표시 중

    if (!RunResultWidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MainGameMode] RunResultWidgetClass not set — restarting level instead."));
        RestartCurrentLevel();
        return;
    }

    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (!PC) return;

    CurrentRunResultWidget = CreateWidget<UCC_RunResultWidget>(PC, RunResultWidgetClass);
    if (!CurrentRunResultWidget) return;

    CurrentRunResultWidget->SetRunResult(PendingRunResult);
    CurrentRunResultWidget->AddToViewport(100); // LevelUp(10)/CubeClearReward(10)보다 위

    UGameplayStatics::SetGamePaused(GetWorld(), true);

    FInputModeUIOnly InputMode;
    InputMode.SetWidgetToFocus(CurrentRunResultWidget->TakeWidget());
    PC->SetInputMode(InputMode);
    PC->SetShowMouseCursor(true);

    UE_LOG(LogTemp, Log, TEXT("[MainGameMode] Run result shown — %s, %.0fs, %d kills, %d cube clears, Lv %d"),
        PendingRunResult.bCleared ? TEXT("CLEAR") : TEXT("GAME OVER"),
        PendingRunResult.SurvivalTime, PendingRunResult.TotalKills,
        PendingRunResult.CubeClearCount, PendingRunResult.ReachedLevel);
}

// ============================================================
//  Cycle Clear
// ============================================================

void ACC_MainGameMode::RefreshCubeClearRewardBar()
{
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (ACC_PlayerController* CCPC = Cast<ACC_PlayerController>(PC))
    {
        if (UCC_GameHUD* HUD = CCPC->GetGameHUD())
        {
            HUD->UpdatePendingRewardSlots(PendingRewardSlots);
        }
    }
}

void ACC_MainGameMode::OnRewardBadgeClicked()
{
    if (PendingRewardSlots <= 0) return;
    if (CurrentCubeClearRewardWidget) return; // 이미 열려있으면 중복 오픈 방지

    if (!CubeClearRewardWidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MainGameMode] CubeClearRewardWidgetClass not set!"));
        return;
    }

    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (!PC) return;

    CurrentCubeClearRewardWidget = CreateWidget<UCC_CubeClearRewardWidget>(PC, CubeClearRewardWidgetClass);
    if (!CurrentCubeClearRewardWidget) return;

    CurrentCubeClearRewardWidget->SetRewardChoices(GetRandomCubeClearRewards(3));
    CurrentCubeClearRewardWidget->OnCubeClearRewardSelected.AddDynamic(this, &ACC_MainGameMode::HandleCubeClearRewardPicked);
    CurrentCubeClearRewardWidget->AddToViewport(10);

    UGameplayStatics::SetGamePaused(GetWorld(), true);
    PC->bShowMouseCursor = true;
    PC->SetInputMode(FInputModeUIOnly());
}

TArray<FCubeClearReward> ACC_MainGameMode::GetRandomCubeClearRewards(int32 Count)
{
    TArray<FCubeClearReward> Result;
    if (CubeClearRewardPool.Num() == 0) return Result;

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
//  Run Result (Game Over / Game Clear)
// ============================================================

FCCRunResult ACC_MainGameMode::BuildRunResult(bool bCleared) const
{
    FCCRunResult Result;
    Result.bCleared = bCleared;

    if (const ACC_PlayerCharacter* Player = GetPlayerCharacter())
    {
        Result.SurvivalTime = Player->GetGameTime();
        Result.ReachedLevel = Player->GetPlayerLevel();
    }

    if (CycleManager)
    {
        Result.TotalKills = CycleManager->TotalKills;
        Result.CubeClearCount = FMath::Max(CycleManager->GetCurrentStage() - 1, 0);
        Result.BossProgressRatio = bCleared ? 1.0f : CycleManager->GetBossProgressRatio();
    }

    return Result;
}

void ACC_MainGameMode::ShowRunResult(bool bCleared)
{
    PendingRunResult = BuildRunResult(bCleared);
    ShowPendingRunResult();
}

void ACC_MainGameMode::TriggerGameOver()
{
    if (CurrentGameState == EGameState::GameOver || CurrentGameState == EGameState::GameClear) return;

    // 폰 정리·사망 연출 전에 기록부터 확정
    PendingRunResult = BuildRunResult(false);

    // 사망 후 처치는 기록에 넣지 않음 (Boss 처치 → 클리어 판정도 막힘)
    if (CycleManager)
    {
        CycleManager->bTrackingActive = false;
    }

    Super::TriggerGameOver(); // GameOver 상태 + OnGameOver Broadcast

    if (GameOverDelay > 0.f)
    {
        GetWorldTimerManager().SetTimer(
            GameOverTimerHandle, this, &ACC_MainGameMode::ShowPendingRunResult, GameOverDelay, false);
    }
    else
    {
        ShowPendingRunResult();
    }
}

void ACC_MainGameMode::DebugShowRunResult(bool bCleared)
{
    SetGameState(bCleared ? EGameState::GameClear : EGameState::GameOver);
    ShowRunResult(bCleared);
}
