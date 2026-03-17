// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_GameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "Characters/CC_PlayerCharacter.h"
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
	SetGameState(EGameState::Playing);
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