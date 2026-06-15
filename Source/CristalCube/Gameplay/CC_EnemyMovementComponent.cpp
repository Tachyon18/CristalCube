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

    // DesiredVelocity를 향해 부드럽게 보간
    Velocity = FMath::VInterpTo(Velocity, DesiredVelocity, DeltaTime, VelocityInterpSpeed);

    // 최대 속도 제한
    if (Velocity.SizeSquared() > MaxSpeed * MaxSpeed)
        Velocity = Velocity.GetSafeNormal() * MaxSpeed;

    if (Velocity.IsNearlyZero(0.1f))
    {
        Velocity = FVector::ZeroVector;
        return;
    }

    // 위치 갱신 — Controller 체크 없음
    FVector Delta = Velocity * DeltaTime;
    FHitResult Hit;
    SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentQuat(), true, Hit);

    // 충돌 시 슬라이딩
    if (Hit.IsValidBlockingHit())
    {
        SlideAlongSurface(Delta, 1.f - Hit.Time, Hit.Normal, Hit);
    }
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