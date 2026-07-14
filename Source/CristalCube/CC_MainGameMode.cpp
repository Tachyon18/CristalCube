// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_MainGameMode.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "CC_PlayerState.h"
#include "SkillSystem/CC_SkillBase.h"
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

    case ECubeClearRewardType::HealFull:
        if (Player)
        {
            
        }
        break;

    case ECubeClearRewardType::SkillGrant:
        if (PS)
        {
            PS->GrantSkillByRowName(SelectedReward.DataRowName);
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
                Candidates[FMath::RandRange(0, Candidates.Num() - 1)]->GrantAddon(SelectedReward.TargetAddonType);
            }
        }
        break;

    case ECubeClearRewardType::AddonUpgrade:
        if (PS)
        {
            TArray<UCC_SkillBase*> Candidates;
            for (UCC_SkillBase* Skill : PS->GetAllSkills())
            {
                if (Skill && Skill->HasAddon(SelectedReward.TargetAddonType))
                {
                    Candidates.Add(Skill);
                }
            }
            if (Candidates.Num() > 0)
            {
                Candidates[FMath::RandRange(0, Candidates.Num() - 1)]->ApplyAddonModifier(
                    SelectedReward.TargetAddonType, SelectedReward.AddonUpgradeDelta);
            }
        }
        break;

    default:
        break;
    }

    UE_LOG(LogTemp, Log, TEXT("[MainGameMode] Reward applied: %s"),
        *SelectedReward.DisplayName.ToString());
}

void ACC_MainGameMode::OnCubeClearAchieved(int32 StageNumber)
{
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    ACC_PlayerState* PS = PC ? PC->GetPlayerState<ACC_PlayerState>() : nullptr;
    if (!PS) return;

    PS->AddCubeEnergy(CubeEnergyPerCubeClear);

    const int32 NewPicks = PS->ConsumeCubeClearPicks();
    PendingRewardSlots += NewPicks;

    UE_LOG(LogTemp, Log,
        TEXT("[MainGameMode] Cube Clear (stage %d) — +%.1f energy, +%d reward slot(s), pending: %d"),
        StageNumber, CubeEnergyPerCubeClear, NewPicks, PendingRewardSlots);

    RefreshCubeClearRewardBar();
}

void ACC_MainGameMode::OnGameCleared()
{
    UE_LOG(LogTemp, Warning, TEXT("[MainGameMode] === GAME CLEARED (Boss defeated) ==="));

    SetGameState(EGameState::GameClear);
    ShowGameClearUI();
}

// ============================================================
//  Cycle Clear
// ============================================================

void ACC_MainGameMode::RefreshCubeClearRewardBar()
{
    // 실제 하단 코너 UI 갱신은 WBP 쪽에서 PendingRewardSlots를 읽어 구현 예정 (Track 5).
    // 예전 버전의 무한재귀 + 미정의 SelectedReward 참조 버그 제거 — 지금은 알림 역할만.
}

void ACC_MainGameMode::OnCubeClearRewardSelected(FCubeClearReward SelectedReward)
{
    ApplyCubeClearReward(SelectedReward);

    PendingRewardSlots = FMath::Max(0, PendingRewardSlots - 1);
    RefreshCubeClearRewardBar();
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
//  게임 오버
// ============================================================

void ACC_MainGameMode::ShowGameClearUI()
{
    UGameplayStatics::SetGamePaused(GetWorld(), true);

    if (!GameClearWidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MainGameMode] GameClearWidgetClass not set!"));
        return;
    }

    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (!PC) return;

    CurrentGameClearWidget = CreateWidget<UUserWidget>(PC, GameClearWidgetClass);
    if (CurrentGameClearWidget)
    {
        CurrentGameClearWidget->AddToViewport();
        PC->SetInputMode(FInputModeUIOnly());
        PC->SetShowMouseCursor(true);
    }
}

void ACC_MainGameMode::TriggerGameOver()
{
    Super::TriggerGameOver();

    SetGameState(EGameState::GameOver);
    if (GameOverDelay > 0.f)
    {
        GetWorldTimerManager().SetTimer(
            GameOverTimerHandle, this, &ACC_MainGameMode::RestartCurrentLevel, GameOverDelay, false);
    }
}