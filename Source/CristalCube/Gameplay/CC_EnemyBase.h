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

    /** 경량 이동 컴포넌트 — 매 틱 부드러운 이동 처리 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UCC_EnemyMovementComponent* EnemyMovement;

    /** 이 Enemy의 도형 타입 — BP 드롭다운으로 선택 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Shape")
    EEnemyShapeType ShapeType = EEnemyShapeType::Cube;

    /** ShapeType == Custom일 때 직접 지정하는 메시 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Shape",
        meta = (EditCondition = "ShapeType == EEnemyShapeType::Custom"))
    TSoftObjectPtr<UStaticMesh> CustomMesh;

    /** 메시를 캡슐 기준으로 얼마나 띄울지 — 도형별 개성에 맞게 디자이너가 직접 입력 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Shape")
    float MeshZOffset = 0.f;

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

    /** 플레이어 목표 오프셋 (RVO 보조 — BeginPlay에서 랜덤 결정) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Movement")
    FVector TargetOffset = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Movement")
    float TargetOffsetRadius = 150.0f;


    // --- 이동 내부 ---
    UPROPERTY()
    class ACC_PlayerCharacter* TargetPlayer;

public:

    void FindPlayer();
    void UpdateMoveTarget();
    void PerformMove();

    void CheckAndPerformAttack();

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

    /** 이동 방식 변경 — EnemyMovement 컴포넌트에 동기화 */
    UFUNCTION(BlueprintCallable, Category = "Enemy|Movement")
    void SetMovementBehavior(EMovementBehavior NewBehavior);

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

    /** ShapeType에 따라 MeshComp 메시를 자동 할당 */
    UFUNCTION(BlueprintCallable, Category = "Enemy|Shape")
    void InitShape();

    /** MeshComp 바운딩 박스 기반으로 CapsuleComp 크기 자동 조정 */
    UFUNCTION(BlueprintCallable, Category = "Enemy|Collision")
    void AutoFitCapsuleToMesh();

    /** MeshZOffset 값으로 MeshComp의 상대 위치 적용 (디자이너가 직접 세팅한 값 사용) */
    UFUNCTION(BlueprintCallable, Category = "Enemy|Shape")
    void ApplyMeshZOffset();

    /** 현재 ShapeType 반환 (Phase 4 HISM 분류용) */
    UFUNCTION(BlueprintPure, Category = "Enemy|Shape")
    EEnemyShapeType GetShapeType() const { return ShapeType; }
};
