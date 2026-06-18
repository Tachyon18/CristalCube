// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_EnemyMovementComponent.h"

UCC_EnemyMovementComponent::UCC_EnemyMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// UMovementComponent 기본 설정
	bConstrainToPlane = true;
	SetPlaneConstraintAxisSetting(EPlaneConstraintAxisSetting::Z); // XY 평면만 이동
}

void UCC_EnemyMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!UpdatedComponent || !IsValid(GetOwner())) return;

	switch (MovementBehavior)
	{
	case EMovementBehavior::Direct:
		TickDirect(DeltaTime);
		break;

	case EMovementBehavior::Step:
		TickStep(DeltaTime);
		break;

	case EMovementBehavior::Teleport:
		TickTeleport(DeltaTime);
		break;

	case EMovementBehavior::Waypoint:
		// 후순위 — 현재는 Direct와 동일하게 동작
		TickDirect(DeltaTime);
		break;
	}
}

void UCC_EnemyMovementComponent::SetMovementBehavior(EMovementBehavior NewBehavior)
{
	if (MovementBehavior == NewBehavior) return;

	MovementBehavior = NewBehavior;

	// 내부 상태 초기화 — 이전 Behavior의 잔여 상태가 새 Behavior에 영향 주지 않도록
	StepPhase = EStepPhase::Moving;
	StepTarget = FVector::ZeroVector;
	StepWaitElapsed = 0.f;
	TeleportElapsed = 0.f;
	DesiredVelocity = FVector::ZeroVector;
}

void UCC_EnemyMovementComponent::SetMoveTarget(const FVector& NewTarget)
{
	MoveTarget = NewTarget;
}

void UCC_EnemyMovementComponent::SetMovementVelocity(const FVector& NewVelocity)
{
    DesiredVelocity = NewVelocity.GetClampedToMaxSize(MaxSpeed);
}

void UCC_EnemyMovementComponent::StopMovementImmediately()
{
    DesiredVelocity = FVector::ZeroVector;
    Velocity = FVector::ZeroVector;
    Super::StopMovementImmediately();
}

void UCC_EnemyMovementComponent::UpdateFacingRotation(const FVector& Direction, float DeltaTime)
{
	if (Direction.IsNearlyZero() || !UpdatedComponent) return;

	FRotator TargetRot = Direction.Rotation();
	TargetRot.Pitch = 0.f;
	TargetRot.Roll = 0.f;

	if (RotationInterpSpeed <= 0.f)
	{
		UpdatedComponent->SetWorldRotation(TargetRot);
		return;
	}

	const FRotator CurrentRot = UpdatedComponent->GetComponentRotation();
	const FRotator NewRot = FMath::RInterpConstantTo(CurrentRot, TargetRot, DeltaTime, RotationInterpSpeed);
	UpdatedComponent->MoveComponent(FVector::ZeroVector, NewRot, false);
}

void UCC_EnemyMovementComponent::TickDirect(float DeltaTime)
{
	FVector Direction = FVector::ZeroVector;

	if (!MoveTarget.IsZero())
	{
		Direction = (MoveTarget - UpdatedComponent->GetComponentLocation()).GetSafeNormal2D();
		DesiredVelocity = Direction * MaxSpeed;
	}
	else
	{
		DesiredVelocity = FVector::ZeroVector;
	}

	// 속도 보간
	Velocity = FMath::VInterpTo(Velocity, DesiredVelocity, DeltaTime, VelocityInterpSpeed);
	if (Velocity.SizeSquared() > MaxSpeed * MaxSpeed)
		Velocity = Velocity.GetSafeNormal() * MaxSpeed;

	ApplyVelocityMove(DeltaTime);

	if (!Velocity.IsNearlyZero(1.f))
		UpdateFacingRotation(Velocity.GetSafeNormal2D(), DeltaTime);
}

void UCC_EnemyMovementComponent::TickStep(float DeltaTime)
{
	const FVector CurrentLoc = UpdatedComponent->GetComponentLocation();

	switch (StepPhase)
	{
	case EStepPhase::Moving:
	{
		// 최초 진입 또는 목표 미설정 시 새 Step 목표 계산
		if (StepTarget.IsZero())
			StepTarget = ComputeNextStepTarget();

		const FVector ToTarget = StepTarget - CurrentLoc;
		const float DistSq = ToTarget.SizeSquared2D();

		if (DistSq <= StepArrivalThreshold * StepArrivalThreshold)
		{
			// 도착 — 대기 상태로 전환
			Velocity = FVector::ZeroVector;
			StepPhase = EStepPhase::Waiting;
			StepWaitElapsed = 0.f;
			break;
		}

		const FVector Direction = ToTarget.GetSafeNormal2D();
		DesiredVelocity = Direction * MaxSpeed;

		Velocity = FMath::VInterpTo(Velocity, DesiredVelocity, DeltaTime, VelocityInterpSpeed);
		if (Velocity.SizeSquared() > MaxSpeed * MaxSpeed)
			Velocity = Velocity.GetSafeNormal() * MaxSpeed;

		ApplyVelocityMove(DeltaTime);
		UpdateFacingRotation(Direction, DeltaTime);
		break;
	}

	case EStepPhase::Waiting:
	{
		Velocity = FVector::ZeroVector;
		StepWaitElapsed += DeltaTime;

		if (StepWaitElapsed >= StepWaitDuration)
		{
			// 대기 종료 — 다음 Step 목표 재계산 후 이동 재개
			StepTarget = FVector::ZeroVector; // 다음 Moving 진입 시 재계산되도록 리셋
			StepPhase = EStepPhase::Moving;
		}
		break;
	}
	}
}

void UCC_EnemyMovementComponent::TickTeleport(float DeltaTime)
{
	// Teleport 방식은 자체 이동(Velocity)이 없음 — 정지 상태 유지
	Velocity = FVector::ZeroVector;

	TeleportElapsed += DeltaTime;
	if (TeleportElapsed < TeleportInterval) return;

	TeleportElapsed = 0.f;

	if (MoveTarget.IsZero()) return;

	const float Angle = FMath::FRandRange(0.f, 360.f);
	const float Radius = FMath::FRandRange(TeleportRadius * 0.5f, TeleportRadius);

	const FVector Destination = MoveTarget + FVector(
		FMath::Cos(FMath::DegreesToRadians(Angle)) * Radius,
		FMath::Sin(FMath::DegreesToRadians(Angle)) * Radius,
		0.f
	);

	const FVector PreviousLoc = UpdatedComponent->GetComponentLocation();

	FHitResult Hit;
	SafeMoveUpdatedComponent(Destination - PreviousLoc, UpdatedComponent->GetComponentQuat(), false, Hit);

	OnTeleportPerformed(PreviousLoc, UpdatedComponent->GetComponentLocation());
}

FVector UCC_EnemyMovementComponent::ComputeNextStepTarget() const
{
	const FVector CurrentLoc = UpdatedComponent->GetComponentLocation();

	switch (StepPattern)
	{
	case EStepPattern::Orbit:
		// 후순위 stub — 현재는 TrackPlayer와 동일하게 동작
		// (적 위치 기준 일정 반경 회전 이동은 추후 구현)
		// fallthrough

	case EStepPattern::TrackPlayer:
	default:
	{
		if (MoveTarget.IsZero())
			return CurrentLoc;

		// Step 시작 시점의 플레이어(목표) 방향을 기준으로 스냅샷
		const FVector BaseDirection = (MoveTarget - CurrentLoc).GetSafeNormal2D();
		if (BaseDirection.IsNearlyZero())
			return CurrentLoc;

		// 각도 랜덤 편차 적용
		const float AngleOffset = FMath::FRandRange(-StepAngleVariance, StepAngleVariance);
		const FVector VariedDirection = BaseDirection.RotateAngleAxis(AngleOffset, FVector::UpVector);

		// 거리 랜덤 편차 적용
		const float VariedDistance = StepDistance * FMath::FRandRange(
			1.f - StepDistanceVariance, 1.f + StepDistanceVariance);

		return CurrentLoc + VariedDirection * VariedDistance;
	}
	}
}

void UCC_EnemyMovementComponent::ApplyVelocityMove(float DeltaTime)
{
	if (Velocity.IsNearlyZero(0.1f))
	{
		Velocity = FVector::ZeroVector;
		return;
	}

	const FVector Delta = Velocity * DeltaTime;
	FHitResult Hit;
	SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentQuat(), true, Hit);

	if (Hit.IsValidBlockingHit())
	{
		SlideAlongSurface(Delta, 1.f - Hit.Time, Hit.Normal, Hit);
	}
}
