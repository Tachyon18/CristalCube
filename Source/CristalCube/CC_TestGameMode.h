// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CC_GameModeBase.h"
#include "CC_TestGameMode.generated.h"

UENUM(BlueprintType)
enum class ESpawnMode : uint8
{
    None        UMETA(DisplayName = "No Enemies"),
    Dummy       UMETA(DisplayName = "Dummy (Infinite HP, No Attack)"),
    WeakWave    UMETA(DisplayName = "Weak Wave")
};

/**
 * ACC_CC_TestRoomGameMode
 *
 * L_TestRoom에서 사용하는 샌드박스 게임 모드.
 * 게임 오버 없음, 스폰 모드 실시간 전환, 치트 지원.
 */
UCLASS()
class CRISTALCUBE_API ACC_TestGameMode : public ACC_GameModeBase
{
	GENERATED_BODY()
	
public:
    ACC_TestGameMode();

protected:
    virtual void BeginPlay() override;

public:

    // ========== 스폰 모드 ==========

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TestRoom")
    ESpawnMode CurrentSpawnMode = ESpawnMode::None;

    /** 더미 적 클래스 (무한 체력, 반격 없음) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TestRoom")
    TSubclassOf<AActor> DummyEnemyClass;

    /** 약한 웨이브 적 최대 수 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TestRoom")
    int32 WeakWaveMaxEnemies = 10;

    /** 약한 웨이브 스폰 간격 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TestRoom")
    float WeakWaveSpawnInterval = 2.0f;

    /** 더미 배치 수 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TestRoom")
    int32 DummyCount = 10;

    UFUNCTION(BlueprintCallable, Category = "TestRoom")
    void SetSpawnMode(ESpawnMode NewMode);

    // ========== 치트 / 빠른 설정 ==========

    /** 플레이어를 즉시 특정 레벨로 설정 */
    UFUNCTION(BlueprintCallable, Category = "TestRoom|Cheats")
    void SetPlayerLevel(int32 TargetLevel);

    /** 플레이어 체력 완전 회복 */
    UFUNCTION(BlueprintCallable, Category = "TestRoom|Cheats")
    void RestorePlayerHealth();

    /** 현재 스폰된 적 전부 제거 */
    UFUNCTION(BlueprintCallable, Category = "TestRoom|Cheats")
    void ClearAllEnemies();

    // ========== 게임 오버 없음 ==========

    /** TestRoom은 게임 오버 없이 체력 회복 후 재개 */
    virtual void TriggerGameOver() override;

private:
    TArray<AActor*> SpawnedDummies;
    FTimerHandle WeakWaveTimerHandle;

    void SpawnDummies();
    void ClearDummies();
    void StartWeakWave();
    void StopWeakWave();
    void SpawnWeakEnemy();
};
