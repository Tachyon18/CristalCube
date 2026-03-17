// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CC_GameModeBase.generated.h"

UENUM(BlueprintType)
enum class EGameState : uint8
{
    WaitingToStart  UMETA(DisplayName = "Waiting To Start"),
    Playing         UMETA(DisplayName = "Playing"),
    Paused          UMETA(DisplayName = "Paused"),
    GameOver        UMETA(DisplayName = "Game Over"),
    LevelUp         UMETA(DisplayName = "Level Up")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameStateChanged, EGameState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameOver);

/**
 * ACC_GameModeBase
 *
 * L_GameMode / L_TestRoom 양쪽 모드가 공유하는 공통 베이스.
 * - 게임 상태 (Playing / Paused / GameOver / LevelUp) 관리
 * - 레벨 간 전환
 * - PlayerController / PlayerCharacter 레퍼런스 제공
 */
UCLASS()
class CRISTALCUBE_API ACC_GameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
    ACC_GameModeBase();

protected:
	virtual void BeginPlay() override;

public:
		
    // ========== 게임 상태 ==========

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game State")
    EGameState CurrentGameState = EGameState::WaitingToStart;

    UPROPERTY(BlueprintAssignable, Category = "Game State")
    FOnGameStateChanged OnGameStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "Game State")
    FOnGameOver OnGameOver;

    UFUNCTION(BlueprintCallable, Category = "Game State")
    void SetGameState(EGameState NewState);

    UFUNCTION(BlueprintPure, Category = "Game State")
    EGameState GetCurrentGameState() const { return CurrentGameState; }

    UFUNCTION(BlueprintPure, Category = "Game State")
    bool IsPlaying() const { return CurrentGameState == EGameState::Playing; }

    // ========== 레벨 전환 ==========

/** 메인 게임 레벨 이름 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Levels")
    FName GameLevelName = FName("L_GameMode");

    /** 테스트룸 레벨 이름 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Levels")
    FName TestRoomLevelName = FName("L_TestRoom");

    /** 메인 메뉴 레벨 이름 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Levels")
    FName MainMenuLevelName = FName("L_MainMenu");

    UFUNCTION(BlueprintCallable, Category = "Levels")
    void GoToGameMode();

    UFUNCTION(BlueprintCallable, Category = "Levels")
    void GoToTestRoom();

    UFUNCTION(BlueprintCallable, Category = "Levels")
    void GoToMainMenu();

    UFUNCTION(BlueprintCallable, Category = "Levels")
    void RestartCurrentLevel();

    // ========== 게임 오버 ==========

    UFUNCTION(BlueprintCallable, Category = "Game State")
    virtual void TriggerGameOver();

    // ========== 편의 접근자 ==========

    UFUNCTION(BlueprintPure, Category = "References")
    class ACC_PlayerCharacter* GetPlayerCharacter() const;

    UFUNCTION(BlueprintPure, Category = "References")
    class ACC_PlayerController* GetPlayerControllerRef() const;
};
