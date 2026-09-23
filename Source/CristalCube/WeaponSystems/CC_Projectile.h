// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraSystem.h"
#include "../CristalCubeStruct.h"
#include "CC_Projectile.generated.h"

UCLASS()
class CRISTALCUBE_API ACC_Projectile : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACC_Projectile();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	// Collision sphere
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USphereComponent* CollisionSphere;

	// Projectile movement
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UProjectileMovementComponent* ProjectileMovement;

	// Visual mesh (optional, can use particle effect instead)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* ProjectileMesh;

protected:

	//==========================================================================
	// Skill System Reference
	//==========================================================================

	UPROPERTY()
	class UCC_SkillSystem* SkillSystem;

	// Current skill definition (for Addon checks)
	FSkillDefinition CurrentSkill;

	// Execution context (for Addon processing)
	FSkillExecutionContext ExecutionContext;

public:

	// Set skill data for Addon processing
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void SetSkillData(UCC_SkillSystem* InSkillSystem,
		const FSkillDefinition& InSkill,
		const FSkillExecutionContext& InContext);

protected:

	//==========================================================================
	// PROJECTILE PROPERTIES
	//==========================================================================


	// Base damage dealt on hit
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Stats")
	float Damage;

	// Projectile speed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Stats")
	float Speed;

	// Max lifetime before auto-destroy (seconds)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Stats")
	float Lifetime;

	// Should destroy on hit?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Behavior")
	bool bDestroyOnHit;

public:

	// Can pierce through enemies?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Behavior")
	bool bCanPierce;

	// Number of enemies to pierce through (if bCanPierce is true)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Behavior")
	int32 PierceCount;

protected:

	// Current pierce count
	UPROPERTY()
	int32 CurrentPierceCount;



protected:

	//==========================================================================
	// EFFECTS
	//==========================================================================

	// Hit effect (spawned on impact)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Effects")
	UNiagaraSystem* HitEffect;

	// Hit sound
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Effects")
	USoundBase* HitSound;

protected:
	//==========================================================================
	// OWNERSHIP & TARGETING
	//==========================================================================


	// Who fired this projectile
	UPROPERTY()
	AActor* ProjectileOwner;

public:
	// Set the owner of this projectile
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void SetProjectileOwner(AActor* NewOwner);

	// Initialize projectile with custom damage
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void InitializeProjectile(float InDamage, float InSpeed = 0.0f);

protected:
	//==========================================================================
	// COLLISION & DAMAGE
	//==========================================================================

	UFUNCTION(BlueprintCallable)
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	// Apply damage to hit actor
	UFUNCTION(BlueprintCallable)
	void ApplyDamageToActor(AActor* HitActor);

	// Play hit effects
	void PlayHitEffects(const FVector& HitLocation);

	// Handle pierce behavior
	bool ShouldPierceThrough(AActor* HitActor);

};

