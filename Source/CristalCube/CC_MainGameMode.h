// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CC_GameModeBase.h"
#include "CC_CycleManager.h"
#include "CristalCubeStruct.h"
#include "CC_MainGameMode.generated.h"

/**
 * ACC_MainGameMode
 *
 * L_GameMode 전용 메인 게임 모드.
 *
 * 보상 체계:
 *   - LevelUp 보상: 기존 시스템 유지 (경험치 기반, 언제든 발생)
 *   - Cube Clear 보상: 사이클 클리어 시 별도 보상 UI (신규).
 */
UCLASS()
class CRISTALCUBE_API ACC_MainGameMode : public ACC_GameModeBase
{
	GENERATED_BODY()
	
public:
    ACC_MainGameMode();

protected:
    virtual void BeginPlay() override;

public:

    // ========== 사이클 매니저 ==========

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cycle")
    ACC_CycleManager* CycleManager = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cycle")
    TSubclassOf<ACC_CycleManager> CycleManagerClass;

    // ========== Cube Clear 보상 ==========

    /**
     * Cube Clear 시 제공할 보상 후보 풀.
     * 이 중 랜덤 3개를 선택해서 플레이어에게 제시.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube Clear")
    TArray<FCubeClearReward> CubeClearRewardPool;

    /** Cube Clear 보상 UI 위젯 클래스 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube Clear")
    TSubclassOf<UUserWidget> CubeClearWidgetClass;

    /** 현재 표시 중인 Cube Clear 위젯 */
    UPROPERTY()
    UUserWidget* CurrentCubeClearWidget = nullptr;

    /** Cube Clear 보상 UI 표시 */
    UFUNCTION(BlueprintCallable, Category = "Cube Clear")
    void ShowCubeClearUI(int32 ClearedCycle);

    /** Cube Clear 보상 UI 닫기 */
    UFUNCTION(BlueprintCallable, Category = "Cube Clear")
    void HideCubeClearUI();

    /**
     * 보상 선택 완료 후 호출.
     * WBP_CubeClear 위젯에서 바인딩.
     * 다음 사이클을 시작.
     */
    UFUNCTION(BlueprintCallable, Category = "Cube Clear")
    void OnCubeClearRewardSelected(FCubeClearReward SelectedReward);

    /**
     * 보상 풀에서 랜덤 N개 선택.
     * UI에서 선택지를 구성할 때 호출.
     */
    UFUNCTION(BlueprintCallable, Category = "Cube Clear")
    TArray<FCubeClearReward> GetRandomCubeClearRewards(int32 Count = 3);

    // ========== 게임 오버 ==========

    /** 게임 오버 후 자동 재시작까지 대기 시간 (0이면 수동) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Over")
    float GameOverDelay = 0.0f;

    virtual void TriggerGameOver() override;

    // ========== 편의 조회 ==========

    UFUNCTION(BlueprintPure, Category = "Cycle")
    ACC_CycleManager* GetCycleManager() const { return CycleManager; }

private:
    void SpawnCycleManager();

    UFUNCTION()
    void OnCubeCleared(int32 ClearedCycle);

    FTimerHandle GameOverTimerHandle;
};
