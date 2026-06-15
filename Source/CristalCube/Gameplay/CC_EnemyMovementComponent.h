// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/MovementComponent.h"
#include "CC_EnemyMovementComponent.generated.h"

/**
 * CristalCube 경량 Enemy 이동 컴포넌트
 * - UMovementComponent 직접 상속
 * - Controller 체크 없음 / 중력 없음
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
// SETTINGS
//==========================================================================

/** 최대 이동 속도 (cm/s) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float MaxSpeed = 250.f;

    /** 속도 보간 강도 — 클수록 즉각 반응, 작을수록 부드럽게 가속 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float VelocityInterpSpeed = 12.f;

    //==========================================================================
    // INTERFACE
    //==========================================================================

    /** AIManager에서 호출 — 이동 방향과 속도를 세팅 */
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void SetMovementVelocity(const FVector& NewVelocity);

    /** 즉시 정지 */
    virtual void StopMovementImmediately() override;

    /** 현재 이동 중인지 여부 */
    UFUNCTION(BlueprintPure, Category = "Movement")
    bool IsMoving() const { return !Velocity.IsNearlyZero(1.f); }

protected:
    /** 목표 속도 (AIManager가 세팅, 매 틱 Velocity가 보간하며 추적) */
    FVector DesiredVelocity = FVector::ZeroVector;
	
};
