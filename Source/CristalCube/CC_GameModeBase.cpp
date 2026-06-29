// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_GameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "Characters/CC_PlayerCharacter.h"
#include "CC_CubeWorldManager.h"
#include "CC_PlayerController.h"

ACC_GameModeBase::ACC_GameModeBase()
{
	DefaultPawnClass = ACC_PlayerCharacter::StaticClass();
	PlayerControllerClass = ACC_PlayerController::StaticClass();
}

void ACC_GameModeBase::BeginPlay()
{
	Super::BeginPlay();
	// Get reference to player controller

	ACC_CubeWorldManager* CubeManager = ACC_CubeWorldManager::Get(this);

	if (!CubeManager)
	{
		// CubeWorldManager가 없는 레벨(예: L_TestRoom) — 대기할 대상이 없으니 즉시 진행
		UE_LOG(LogTemp, Log, TEXT("[GameMode] No CubeWorldManager in this level — proceeding immediately."));
		SetGameState(EGameState::Playing);
		return;
	}
	
	if (CubeManager->IsCubeSystemReady())
	{
		// BeginPlay 순서 경합으로 CubeWorldManager가 이미 초기화를 끝내버린 경우 —
		// 신호를 기다려봐야 이미 지나간 신호라 못 받으므로, 상태를 직접 확인해서 즉시 전환.
		UE_LOG(LogTemp, Log, TEXT("[GameMode] CubeWorldManager already ready — proceeding immediately."));
		StartMinimumDelayThenPlaying();
		return;
	}

	CubeManager->OnCubeSystemReady.AddDynamic(this, &ACC_GameModeBase::OnCubeSystemReadyHandler);

	GetWorldTimerManager().SetTimer(
		WaitingToStartTimeoutHandle,
		this,
		&ACC_GameModeBase::OnWaitingToStartTimeout,
		WaitingToStartTimeout,
		false);

	UE_LOG(LogTemp, Log, TEXT("[GameMode] Waiting for CubeWorldManager ready signal (timeout: %.1fs)..."),
		WaitingToStartTimeout);
}

// ============================================================
//  게임 상태
// ============================================================

void ACC_GameModeBase::SetGameState(EGameState NewState)
{
	if (CurrentGameState == NewState) return;

	CurrentGameState = NewState;
	OnGameStateChanged.Broadcast(NewState);

	UE_LOG(LogTemp, Log, TEXT("[GameMode] State -> %s"),
		*UEnum::GetValueAsString(NewState));
}

void ACC_GameModeBase::TriggerGameOver()
{
	if (CurrentGameState == EGameState::GameOver) return;
	
	SetGameState(EGameState::GameOver);
	OnGameOver.Broadcast();

	UE_LOG(LogTemp, Warning, TEXT("[GameMode] GAME OVER"));
}

// ============================================================
// 레벨 전환
// ============================================================

void ACC_GameModeBase::GoToGameMode()
{
	UGameplayStatics::OpenLevel(this, GameLevelName);
}

void ACC_GameModeBase::GoToTestRoom()
{
	UGameplayStatics::OpenLevel(this, TestRoomLevelName);
}

void ACC_GameModeBase::GoToMainMenu()
{
	UGameplayStatics::OpenLevel(this, MainMenuLevelName);
}

void ACC_GameModeBase::RestartCurrentLevel()
{
	UGameplayStatics::OpenLevel(this, FName(*GetWorld()->GetName()));
}

// =============================================================
// 편의 접근자
// =============================================================

ACC_PlayerCharacter* ACC_GameModeBase::GetPlayerCharacter() const
{
	return Cast<ACC_PlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
}

ACC_PlayerController* ACC_GameModeBase::GetPlayerControllerRef() const
{
	return Cast<ACC_PlayerController>(UGameplayStatics::GetPlayerController(this, 0));
}

void ACC_GameModeBase::OnCubeSystemReadyHandler()
{
	if (CurrentGameState != EGameState::WaitingToStart) return; // 이미 전환됨(타임아웃 등) — 중복 방지

	GetWorldTimerManager().ClearTimer(WaitingToStartTimeoutHandle);
	UE_LOG(LogTemp, Log, TEXT("[GameMode] CubeWorldManager ready signal received."));
	StartMinimumDelayThenPlaying();
}

void ACC_GameModeBase::OnWaitingToStartTimeout()
{
	if (CurrentGameState != EGameState::WaitingToStart) return;

	UE_LOG(LogTemp, Warning,
		TEXT("[GameMode] CubeWorldManager ready signal NOT received within %.1fs — falling back to Playing."),
		WaitingToStartTimeout);
	SetGameState(EGameState::Playing);
}

void ACC_GameModeBase::StartMinimumDelayThenPlaying()
{
	if (MinimumStartDelay <= 0.f)
	{
		SetGameState(EGameState::Playing);
		return;
	}

	GetWorldTimerManager().SetTimer(
		MinimumStartDelayHandle,
		this,
		&ACC_GameModeBase::OnMinimumStartDelayElapsed,
		MinimumStartDelay,
		false);

	UE_LOG(LogTemp, Log,
		TEXT("[GameMode] Cube system ready — holding %.1fs more before Playing (load buffer)."),
		MinimumStartDelay);
}

void ACC_GameModeBase::OnMinimumStartDelayElapsed()
{
	if (CurrentGameState != EGameState::WaitingToStart) return;

	SetGameState(EGameState::Playing);
}
