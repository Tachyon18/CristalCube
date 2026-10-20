// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CC_EnemyAreaAttack.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;
class UCC_EffectorPoolSubsystem;
class ACC_PlayerCharacter;

/**
 * Enemy omni-directional basic attack (used when no projectile class is set).
 * A flat ring expands on the attacker's plane from 0 to MaxRadius.
 * Hit test is planar: 2D (XY) distance against the ring band, widened by the
 * target capsule radius, plus a vertical band (HeightTolerance). Hits the player once.
 * Debug: console "CC.ShowEnemyAttackArea 1".
 */
UCLASS()
class CRISTALCUBE_API ACC_EnemyAreaAttack : public AActor
{
	GENERATED_BODY()

public:
	ACC_EnemyAreaAttack();

	static ACC_EnemyAreaAttack* AcquirePooled(UWorld* World, TSubclassOf<ACC_EnemyAreaAttack> AttackClass);

	void Launch(const FVector& InOrigin, float InMaxRadius, float InDamage, AActor* InInstigator);

	void SetOwningPool(UCC_EffectorPoolSubsystem* InPool) { OwningPool = InPool; }

	// Public so the attacker can cancel it (e.g. when frozen). Safe to call on an inactive instance.
	void Deactivate();

	// Current attacker (null once deactivated)
	AActor* GetAttackInstigator() const { return DamageInstigator.Get(); }

	virtual void Tick(float DeltaTime) override;

protected:
	// Seconds for the ring to reach MaxRadius (0 = instant)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AreaAttack", meta = (ClampMin = "0.0"))
	float ExpandDuration = 0.35f;

	// Thickness of the ring front (cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AreaAttack", meta = (ClampMin = "0.0"))
	float RingThickness = 30.0f;

	// Max vertical gap between the ring plane and the target center that still hits (cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AreaAttack", meta = (ClampMin = "0.0"))
	float HeightTolerance = 120.0f;

	// true: ring center follows the attacker every tick (aura style). false: fixed at cast location.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AreaAttack")
	bool bFollowInstigator = false;

	// Optional ring VFX. Receives "User.Radius" every tick.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AreaAttack")
	UNiagaraSystem* RingEffect = nullptr;

private:
	void TryHitTarget(float BandInner, float BandOuter);

	UPROPERTY(VisibleAnywhere)
	USceneComponent* Root = nullptr;

	UPROPERTY()
	UNiagaraComponent* RingVFX = nullptr;

	UPROPERTY()
	UCC_EffectorPoolSubsystem* OwningPool = nullptr;

	TWeakObjectPtr<AActor> DamageInstigator;
	TWeakObjectPtr<ACC_PlayerCharacter> Target;

	FVector Origin = FVector::ZeroVector;
	float MaxRadius = 0.0f;
	float Damage = 0.0f;
	float ElapsedTime = 0.0f;
	float PreviousRadius = 0.0f;
	float CurrentRadius = 0.0f;
	bool bHasHit = false;
	bool bIsActive = false;
};