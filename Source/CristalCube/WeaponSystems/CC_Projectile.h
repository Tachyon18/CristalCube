// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CC_Projectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UStaticMeshComponent;
class UNiagaraSystem;
class USoundBase;
class UCC_EffectorPoolSubsystem;

/**
 * Enemy basic-attack projectile. The player is the only valid target.
 * Pooled through UCC_EffectorPoolSubsystem: Fire() activates, Deactivate() returns it.
 * Lifetime uses a manual timer (no InitialLifeSpan), so a pooled instance never
 * receives a stale LifeSpan callback while it waits in the free list.
 */
UCLASS()
class CRISTALCUBE_API ACC_Projectile : public AActor
{
	GENERATED_BODY()

public:
	ACC_Projectile();

	// Pooled spawn. Falls back to a plain SpawnActor when no pool exists.
	static ACC_Projectile* AcquirePooled(UWorld* World, TSubclassOf<ACC_Projectile> ProjectileClass);

	// Activate (or re-activate) and launch. InSpeed <= 0 keeps the Blueprint default Speed.
	void Fire(const FVector& SpawnLocation, const FVector& Direction, float InDamage, float InSpeed, AActor* InOwner);

	// Stop, hide and return to the pool (Destroy when not pooled). Safe to call twice.
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void Deactivate();

	void SetOwningPool(UCC_EffectorPoolSubsystem* InPool) { OwningPool = InPool; }

	// Current shooter (null once deactivated) - lets the shooter cancel only its own live shots
	AActor* GetProjectileOwner() const { return ProjectileOwner.Get(); }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void PlayHitEffects(const FVector& HitLocation) const;

protected:
	//==========================================================================
	// COMPONENTS
	//==========================================================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UProjectileMovementComponent* ProjectileMovement;

	// Optional visual (a Niagara component can be added in the Blueprint instead)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* ProjectileMesh;

	//==========================================================================
	// PROPERTIES
	//==========================================================================

	// Overwritten by Fire()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Stats")
	float Damage = 10.0f;

	// Default speed when the shooter passes 0
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Stats")
	float Speed = 1000.0f;

	// Seconds before auto-deactivate
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Stats")
	float Lifetime = 5.0f;

	// Deactivate (return to pool) on hit
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Behavior")
	bool bDestroyOnHit = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Behavior")
	bool bCanPierce = false;

	// Number of extra targets it can pass through when bCanPierce is true
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Behavior")
	int32 PierceCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Effects")
	UNiagaraSystem* HitEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Effects")
	USoundBase* HitSound = nullptr;

private:
	UPROPERTY()
	UCC_EffectorPoolSubsystem* OwningPool = nullptr;

	// Weak: the shooter can die while the projectile is still flying or pooled
	UPROPERTY()
	TWeakObjectPtr<AActor> ProjectileOwner;

	TArray<TWeakObjectPtr<AActor>> HitActors;

	int32 CurrentPierceCount = 0;

	// True while waiting in the pool (and before the first Fire)
	bool bIsPooledInactive = true;

	FTimerHandle LifetimeTimerHandle;
};