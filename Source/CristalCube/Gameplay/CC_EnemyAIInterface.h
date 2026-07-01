// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CC_EnemyAIInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UCC_EnemyAIInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Enemy AI 공통 인터페이스
 * ACC_EnemyCharacter, ACC_EnemyBase 양쪽에서 구현
 * UCC_AIManager가 이 인터페이스만 바라보고 두 계층을 통합 관리
 */
class CRISTALCUBE_API ICC_EnemyAIInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

    /** 이동 활성화/비활성화 (AIManager 배치 처리에서 호출) */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Enemy|AI")
    void SetChasePlayer(bool bChase);
    virtual void SetChasePlayer_Implementation(bool bChase) = 0;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Enemy|AI")
    bool GetChasePlayer() const;
    virtual bool GetChasePlayer_Implementation() const = 0;

    /** 감지 범위 — AIManager 거리 분류에 사용 */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Enemy|AI")
    float GetDetectionRange() const;
    virtual float GetDetectionRange_Implementation() const = 0;

    /** 생존 여부 — AIManager 유효성 체크에 사용 */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Enemy|AI")
    bool IsEnemyAlive() const;
    virtual bool IsEnemyAlive_Implementation() const = 0;

    /** 동결 여부 — AIManager 배치 루프에서 스킵 여부 판단 */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Enemy|AI")
    bool GetIsFrozen() const;
    virtual bool GetIsFrozen_Implementation() const = 0;

    /** Persistent(Lock 추적 대상) 여부 조회 */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Enemy|Persistent")
    bool IsPersistentEnemy() const;
    virtual bool IsPersistentEnemy_Implementation() const = 0;

    /** Persistent 상태 전환 — 등록/해제 */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Enemy|Persistent")
    void SetPersistentEnemy(bool bPersistentState);
    virtual void SetPersistentEnemy_Implementation(bool bPersistentState) = 0;
};
