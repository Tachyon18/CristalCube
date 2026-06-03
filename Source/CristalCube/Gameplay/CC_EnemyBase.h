// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "../CristalCubeStruct.h"
#include "CC_EnemyAIInterface.h"
#include "CC_Freezable.h"
#include "CC_EnemyBase.generated.h"


/**
 * 경량 Enemy 베이스 클래스
 * - CMC 없음, APawn 기반
 * - 이동: 타이머 + SetActorLocation
 * - 다면체 StaticMesh 조합 Enemy용
 * - EMovementBehavior::Direct / Teleport 지원
 */
UCLASS()
class CRISTALCUBE_API ACC_EnemyBase : public APawn, public ICC_Freezable, public ICC_EnemyAIInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ACC_EnemyBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

    //==========================================================================
    // COMPONENTS
    //==========================================================================

    /** 메인 충돌체 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UCapsuleComponent* CapsuleComp;

    /** 메인 메시 (다면체 조합의 루트 메시) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* MeshComp;

    // Phase 4: HISM 교체 예정 자리
    // UPROPERTY() UHierarchicalInstancedStaticMeshComponent* HISM;

    //==========================================================================
    // STATS
    //==========================================================================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Stats")
    float MaxHealth = 30.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Stats")
    float CurrentHealth;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Stats")
    FCristalCubeEnemyStats EnemyStats;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Reward")
    float ExpGemAmount = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Reward")
    TSubclassOf<class ACC_ExperienceGem> ExpGemClass;

    //==========================================================================
    // MOVEMENT
    //==========================================================================

    /** 현재 이동 목적지 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Movement")
    FVector MoveTarget = FVector::ZeroVector;

    /** 이동 방식 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Movement")
    EMovementBehavior MovementBehavior = EMovementBehavior::Direct;

    /** 이동 활성화 여부 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Movement")
    bool bMovementEnabled = true;

    /** 현재 상태 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|State")
    EEnemyState EnemyState = EEnemyState::Moving;

    /** 감지 범위 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Movement")
    float DetectionRange = 2000.0f;

    /** 이동 속도 (cm/s) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Movement")
    float MoveSpeed = 250.0f;

    /** 타이머 틱 간격 — Direct 이동 갱신 주기 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Movement")
    float MoveTickInterval = 0.05f;

    /** 플레이어 목표 오프셋 (RVO 보조 — BeginPlay에서 랜덤 결정) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Movement")
    FVector TargetOffset = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Movement")
    float TargetOffsetRadius = 150.0f;

    // Step 파라미터 (Phase 3)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Movement|Step")
    float StepDistance = 300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Movement|Step")
    float StepWaitDuration = 0.8f;


    // --- Teleport 파라미터 ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Movement|Teleport")
    float TeleportInterval = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Movement|Teleport")
    float TeleportRadius = 200.0f;

    // --- 타이머 핸들 ---
    FTimerHandle MoveTimerHandle;
    FTimerHandle TeleportTimerHandle;

    // --- 이동 내부 ---
    UPROPERTY()
    class ACC_PlayerCharacter* TargetPlayer;

public:

    void FindPlayer();
    void UpdateMoveTarget();
    void PerformMove();

    /** Direct 이동 — MoveTarget 방향으로 SetActorLocation */
    void PerformMove_Direct();

    /** Teleport 이동 — TeleportInterval마다 플레이어 근방으로 순간이동 */
    void PerformMove_Teleport();

    void CheckAndPerformAttack();

    void StartMoveTimer();
    void StopMoveTimer();

protected:

    //==========================================================================
    // COMBAT
    //==========================================================================

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Combat")
    bool bCanAttack = true;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Combat")
    bool bIsAttacking = false;

    FTimerHandle AttackCooldownTimer;

    void TryAttack(ACC_PlayerCharacter* Target);

    void PerformAttack();
    void ResetAttackCooldown();


    //==========================================================================
    // HEALTH
    //==========================================================================

    bool bIsDead = false;

    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
        AController* EventInstigator, AActor* DamageCauser) override;

    virtual void Die();

    //==========================================================================
    // FREEZE (ICC_Freezable)
    //==========================================================================

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    bool bIsFrozen = false;


public:

    //==========================================================================
    // PUBLIC INTERFACE (AIManager 호환)
    //==========================================================================

    UFUNCTION(BlueprintCallable, Category = "Enemy|Movement")
    void SetMovementEnabled(bool bEnabled);

protected:
    UFUNCTION()
    void RegisterToManagers();

public:

	//==========================================================================
	// ICC_Freezable 구현
	//==========================================================================

    virtual void Freeze_Implementation() override;
    virtual void Unfreeze_Implementation() override;
    virtual bool IsFrozen_Implementation() const override { return bIsFrozen; }

    //==========================================================================
    // ICC_EnemyAIInterface 구현
    //==========================================================================

    virtual void SetChasePlayer_Implementation(bool bChase) override;
    virtual bool GetChasePlayer_Implementation() const override { return bMovementEnabled; }
    virtual float GetDetectionRange_Implementation() const override { return DetectionRange; }
    virtual bool IsEnemyAlive_Implementation() const override { return IsAlive(); }
    virtual bool GetIsFrozen_Implementation() const override { return bIsFrozen; }

    //==========================================================================
    // 공용
    //==========================================================================

    UFUNCTION(BlueprintPure, Category = "Enemy|Stats")
    bool IsAlive() const { return !bIsDead && CurrentHealth > 0.f; }
};
