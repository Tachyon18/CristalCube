// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CC_Character.h"
#include "../CristalCubeStruct.h"
#include "../Gameplay/CC_Freezable.h"
#include "../Gameplay/CC_EnemyAIInterface.h"
#include "CC_EnemyCharacter.generated.h"

/**
 * 
 */
UCLASS()
class CRISTALCUBE_API ACC_EnemyCharacter : public ACC_Character, public ICC_Freezable, public ICC_EnemyAIInterface
{
	GENERATED_BODY()

public:
	ACC_EnemyCharacter();

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
public:

	virtual void Tick(float DeltaTime) override;

protected:

	bool bReportedActualDeath = false;

	// Target player reference
	UPROPERTY()
	class ACC_PlayerCharacter* TargetPlayer;

	TArray<AActor*> HitActorsThisAttack;

	// Chase the target player
	void ChasePlayer(float DeltaTime);

protected:

	//==========================================================================
	// MOVEMENT
	//==========================================================================
	
	/** 현재 이동 목적지. 플레이어 위치 또는 경유점 등 외부에서 주입 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Movement")
	FVector MoveTarget = FVector::ZeroVector;

	/** 이동 방식 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Movement")
	EMovementBehavior MovementBehavior = EMovementBehavior::Direct;

	/** 이동 활성화 여부 (Attack 상태 진입 시 false) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Movement")
	bool bMovementEnabled = true;

	/** 현재 Enemy 상태 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|State")
	EEnemyState EnemyState = EEnemyState::Moving;

	/** 감지 범위 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Movement")
	float DetectionRange = 2000.0f;

	/** MoveTarget을 향해 현재 MovementBehavior 방식으로 이동 */
	void PerformMove(float DeltaTime);

	/** Direct 이동 구현 (Phase 1) */
	void PerformMove_Direct(float DeltaTime);

	// Phase 3에서 추가될 것들 (선언만)
	// void PerformMove_Step(float DeltaTime);
	// void PerformMove_Teleport();

	/** MoveTarget 갱신 (매 AILogicInterval마다 호출) */
	void UpdateMoveTarget();

	/** 플레이어 위치 탐색 (FindPlayer 역할 통합) */
	void FindPlayer();

	// --- Direct 이동 내부 ---
	FVector CachedMoveDirection = FVector::ZeroVector;

	float AILogicTimer = 0.f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Movement")
	float AILogicInterval = 0.1f;

	// --- Step 이동 내부 (Phase 3 구현, 현재 stub) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Movement|Step")
	float StepDistance = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Movement|Step")
	float StepWaitDuration = 0.8f;

	// --- Teleport 이동 내부 (Phase 3 구현, 현재 stub) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Movement|Teleport")
	float TeleportInterval = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Movement|Teleport")
	float TeleportRadius = 200.0f;

public:

	UFUNCTION(BlueprintCallable, Category = "Enemy|Movement")
	void SetMovementEnabled(bool bEnabled) { bMovementEnabled = bEnabled; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Movement")
	bool GetMovementEnabled() const { return bMovementEnabled; }


protected:
	//==========================================================================
	// COMBAT
	//==========================================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Combat")
	FAttackHitData AttackHitData;

	// Contact damage dealt to player on collision
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Combat")
	float ContactDamage;

	// Cooldown between damage ticks
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Combat")
	float DamageCooldown;

	// Last time damage was dealt
	UPROPERTY()
	float LastDamageTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Combat|Debug")
	bool bShowAttackDebug = true;

	// Can deal damage right now?
	bool CanDealDamage() const;

	UFUNCTION(BlueprintCallable, Category = "Enemy|Combat")
	bool PerformAttackHit(const FAttackHitData& HitData, TArray<AActor*>& OutHitTargets);

	// Deal contact damage to player
	void DealContactDamage(AActor* OtherActor);

	
protected:
	
	//==========================================================================
	// EXPERIENCE DROP
	//==========================================================================


	// Experience points dropped when killed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Reward")
	float ExperienceDrop;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Reward")
	TSubclassOf<class ACC_ExperienceGem> ExpGemClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Reward")
	float ExpGemAmount = 10.0f;

	// Override Die() to drop experience
	virtual void Die() override;

	void ReportActualDeathToEnemyManager();

protected:

	//==========================================================================
	// COLLISION
	//==========================================================================


	// Called when enemy overlaps with something
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);


protected:

	//==========================================================================
	// ENEMY TYPE (Future expansion)
	//==========================================================================


	// Enemy type for future variety
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Type")
	FString EnemyType;

	// Is this a boss enemy?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Type")
	bool bIsBoss;

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Type")
	bool bPersistent = false;

public:


protected:
	
protected:

	//==========================================================================
	// TARGET OFFSET (RVO 보조 — 플레이어 주변 원형 배치)
	//==========================================================================

	/** 플레이어 목표 위치 오프셋 (BeginPlay에서 랜덤 결정) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|AI")
	FVector TargetOffset = FVector::ZeroVector;

	/** 목표 오프셋 반경 (cm). 0이면 오프셋 없음 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|AI")
	float TargetOffsetRadius = 200.0f;

protected:

	//==========================================================================
	// Attack 
	//==========================================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	FCristalCubeEnemyStats EnemyStats;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USphereComponent* AttackRangeSphere;

	// Attack cooldown timer
	FTimerHandle AttackCooldownTimer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	bool bPlayerInRange = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	bool bCanAttack = true;

	void PerformAttack();
	void StartAttackCooldown();
	void ResetAttackCooldown();

	UFUNCTION()
	void OnAttackRangeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnAttackRangeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

public:

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void DealDamageToTarget();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void TryAttack(ACC_PlayerCharacter* Target);

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool CanAttack() const { return bCanAttack && !bIsAttacking && ACC_Character::IsAlive(); }

	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetAttackCooldownPercent() const;

protected:
	
	// Animation instance reference

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	class UAnimMontage* AttackMontage;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
	bool bIsAttacking = false;

	void PlayAttackAnimation();

	UFUNCTION()
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);


	// Debug

	void DrawAttackDebug(const FAttackHitData& HitData, bool bHit);

protected:
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	bool bIsFrozen = false;

public:
	
	// ICC_Freezable 구현
	virtual void Freeze_Implementation() override;
	virtual void Unfreeze_Implementation() override;
	virtual bool IsFrozen_Implementation() const override { return bIsFrozen; }

	UFUNCTION()
	void RegisterToManagers();

	// ICC_EnemyAIInterface 구현
	virtual void SetChasePlayer_Implementation(bool bChase) override
	{
		SetMovementEnabled(bChase);
	}

	virtual bool GetChasePlayer_Implementation() const override
	{
		return bMovementEnabled;
	}

	virtual float GetDetectionRange_Implementation() const override
	{
		return DetectionRange;
	}

	virtual bool IsEnemyAlive_Implementation() const override
	{
		return ACC_Character::IsAlive();
	}   // ACC_Character::IsAlive()

	virtual bool GetIsFrozen_Implementation() const override
	{
		return bIsFrozen;
	}
};
