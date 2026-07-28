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
 *   - LevelUp 보상: 기존 시스템 유지 (경험치 기반)
 *   - Cube Clear 보상: 빈번, 논블로킹 — PendingRewardSlots 코너 바 (Track 5, 하단 UI는 추후 구현)
 *   - Game Clear (Boss 처치): 드묾, 완전 일시정지 팝업
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

	// ====================================================================
    // Cycle Manager
	// ====================================================================

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cycle")
    ACC_CycleManager* CycleManager = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cycle")
    TSubclassOf<ACC_CycleManager> CycleManagerClass;

	// ====================================================================
	// Cube Clear Reward (Non-blocking, Pending)
	// ====================================================================
    
    /** Cube Clear 보상 후보 풀. GetRandomCubeClearRewards()로 N개 랜덤 추출 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube Clear")
    TArray<FCubeClearReward> CubeClearRewardPool;

    /** Cube Clear마다 지급할 기본 Cube Energy 양 — [임시값, 밸런싱 대상] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube Clear")
    float CubeEnergyPerCubeClear = 15.0f;

    /** Cube Clear마다 자동으로 회복되는 비율 (0.0~1.0) — HealFull을 대체하는 부수 효과.
     *  픽 카드가 아니라 매 CubeClear마다 무조건 적용됨. [임시값, 밸런싱 대상] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube Clear", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float CubeClearAutoHealPercent = 0.15f;

    /** 아직 선택 안 된 보상 슬롯 수 — 하단 코너 UI(WBP, 추후 작업)가 이 값을 읽어 표시 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cube Clear")
    int32 PendingRewardSlots = 0;

    /** 하단 코너 UI 갱신 알림 — 실제 표시는 WBP 쪽에서 PendingRewardSlots를 읽어 구현 (지금은 알림 훅만) */
    UFUNCTION(BlueprintCallable, Category = "Cube Clear")
    void RefreshCubeClearRewardBar();

    /** RewardBadge 클릭 시 호출 — 카드 패널 오픈 (Phase C에서 실제 구현 예정, 지금은 로그만) */
    UFUNCTION(BlueprintCallable, Category = "Cube Clear")
    void OnRewardBadgeClicked();

    /** 카드 패널 위젯 클래스 — WBP_CubeClearReward 지정 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube Clear")
    TSubclassOf<class UCC_CubeClearRewardWidget> CubeClearRewardWidgetClass;

    UPROPERTY()
    class UCC_CubeClearRewardWidget* CurrentCubeClearRewardWidget = nullptr;

    /** 보상 풀에서 랜덤 N개 선택 */
    UFUNCTION(BlueprintCallable, Category = "Cube Clear")
    TArray<FCubeClearReward> GetRandomCubeClearRewards(int32 Count = 3);



	// ====================================================================
	// Game Clear (Boss Clear, Blocking)
	// ====================================================================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Clear")
    TSubclassOf<UUserWidget> GameClearWidgetClass;

    UPROPERTY()
    UUserWidget* CurrentGameClearWidget = nullptr;

    UFUNCTION(BlueprintCallable, Category = "Game Clear")
    void ShowGameClearUI();

	// ====================================================================
    // Game Over
	// ====================================================================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Over")
    float GameOverDelay = 0.0f;

    virtual void TriggerGameOver() override;

	// ====================================================================
	// Convenience
	// ====================================================================

    UFUNCTION(BlueprintPure, Category = "Cycle")
    ACC_CycleManager* GetCycleManager() const { return CycleManager; }

private:
    void SpawnCycleManager();
    void ApplyCubeClearReward(const FCubeClearReward& SelectedReward);

    UFUNCTION()
    void HandleCubeClearRewardPicked(FCubeClearReward SelectedReward);

    void CloseCubeClearRewardPanel();

    UFUNCTION()
    void OnCubeClearAchieved(int32 StageNumber);

    UFUNCTION()
    void OnGameCleared();

    FTimerHandle GameOverTimerHandle;
};
