// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/MovementComponent.h"
#include "../CristalCubeStruct.h"
#include "CC_EnemyMovementComponent.generated.h"

/**
 * CristalCube 경량 Enemy 이동 컴포넌트
 * - UMovementComponent 직접 상속
 * - Controller 체크 없음 / 중력 없음
 * - Phase 3: MovementBehavior(Direct/Step/Teleport) 패턴 자체를 컴포넌트가 책임짐
 * - AIManager가 Velocity를 세팅하면 매 틱 위치 갱신
 */
UCLASS(ClassGroup = "CristalCube", meta = (BlueprintSpawnableComponent))
class CRISTALCUBE_API UCC_EnemyMovementComponent : public UMovementComponent
{
	GENERATED_BODY()

public:
	UCC_EnemyMovementComponent();

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    //==========================================================================
    // SETTINGS - Common
    //==========================================================================

    /** 최대 이동 속도 (cm/s) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float MaxSpeed = 250.f;

    /** 속도 보간 강도 — 클수록 즉각 반응, 작을수록 부드럽게 가속 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float VelocityInterpSpeed = 12.f;

    /** 회전 보간 속도 (deg/s) — 0이면 즉시 회전(스냅) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Rotation")
    float RotationInterpSpeed = 180.f;

	//==========================================================================
	// SETTINGS — Step
	//==========================================================================

	/** Step 이동 패턴 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Step")
	EStepPattern StepPattern = EStepPattern::TrackPlayer;

	/** 1회 Step 이동 거리 (기준값, 랜덤 편차 적용됨) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Step")
	float StepDistance = 300.f;

	/** Step 도착 후 정지 대기 시간 (초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Step")
	float StepWaitDuration = 0.8f;

	/** Step 목표 도달 판정 거리 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Step")
	float StepArrivalThreshold = 15.f;

	/** Step 거리 랜덤 편차 비율 (0.2 = ±20%) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Step",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StepDistanceVariance = 0.2f;

	/** Step 방향 랜덤 편차 (도, 추적 방향 기준 좌우 흔들림) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Step",
		meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float StepAngleVariance = 15.f;

	//==========================================================================
	// SETTINGS — Teleport
	//==========================================================================

	/** Teleport 간격 (초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Teleport")
	float TeleportInterval = 2.f;

	/** Teleport 목적지 랜덤 반경 (MoveTarget 기준) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Teleport")
	float TeleportRadius = 200.f;


    //==========================================================================
    // INTERFACE
    //==========================================================================

	/** 현재 이동 방식 설정 — 변경 시 내부 상태 초기화 */
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void SetMovementBehavior(EMovementBehavior NewBehavior);

	UFUNCTION(BlueprintPure, Category = "Movement")
	EMovementBehavior GetMovementBehavior() const { return MovementBehavior; }

	/** EnemyBase가 매 배치 틱마다 호출 — "어디로 갈지"만 전달 */
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void SetMoveTarget(const FVector& NewTarget);

	/** (레거시 호환) AIManager/EnemyBase에서 직접 속도 세팅하던 경로 — Direct 전용 */
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void SetMovementVelocity(const FVector& NewVelocity);

    /** 즉시 정지 */
    virtual void StopMovementImmediately() override;

    /** 현재 이동 중인지 여부 */
    UFUNCTION(BlueprintPure, Category = "Movement")
    bool IsMoving() const { return !Velocity.IsNearlyZero(1.f); }

	/** Teleport 발생 시 BP에서 VFX 등 시각 효과를 구현할 자리 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Movement|Teleport")
	void OnTeleportPerformed(const FVector& FromLocation, const FVector& ToLocation);

protected:
    /** 목표 속도 (Direct/공통 — 매 틱 Velocity가 보간하며 추적) */
    FVector DesiredVelocity = FVector::ZeroVector;

	/** 외부(EnemyBase)에서 전달된 최종 목표 지점 (플레이어 위치 + 오프셋 등) */
	FVector MoveTarget = FVector::ZeroVector;

	/** 현재 활성화된 이동 방식 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	EMovementBehavior MovementBehavior = EMovementBehavior::Direct;

	//--- Step 내부 상태 ---
	enum class EStepPhase : uint8 { Moving, Waiting };
	EStepPhase StepPhase = EStepPhase::Moving;
	FVector StepTarget = FVector::ZeroVector;
	float StepWaitElapsed = 0.f;

	//--- Teleport 내부 상태 ---
	float TeleportElapsed = 0.f;

	//--- 공통 회전 처리 ---
	void UpdateFacingRotation(const FVector& Direction, float DeltaTime);

	//--- Behavior별 Tick 처리 ---
	void TickDirect(float DeltaTime);
	void TickStep(float DeltaTime);
	void TickTeleport(float DeltaTime);

	/** Step 새 목표 계산 (StepPattern 분기 + 랜덤 편차) */
	FVector ComputeNextStepTarget() const;

	/** 현재 Velocity/위치를 이용해 충돌 슬라이딩까지 포함한 실제 위치 이동 적용 */
	void ApplyVelocityMove(float DeltaTime);
	
};
