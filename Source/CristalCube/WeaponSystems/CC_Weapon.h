// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraSystem.h"
#include "../CristalCubeStruct.h"
#include "CC_Weapon.generated.h"

UCLASS()
class CRISTALCUBE_API ACC_Weapon : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACC_Weapon();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	//==============================================================================
// WEAPON PROPERTIES
//==============================================================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* WeaponMesh;	

	// Base weapon stats (damage, attack speed, category)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Stats")
	FCristalCubeWeaponStats BaseStats;

	// Ranged weapon settings (only used if WeaponCategory == Ranged)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Ranged", meta = (EditCondition))
	FCristalCubeRangedStats RangedStats;

	// Melee weapon settings (only used if WeaponCategory == Melee)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Melee", meta = (EditCondition))
	FCristalCubeMeleeStats MeleeStats;

	// Magic weapon settings (only used if WeaponCategory == Magic)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Magic", meta = (EditCondition))
	FCristalCubeMagicStats MagicStats;

	// Current state
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon State")
	bool bCanAttack;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon State")
	float LastAttackTime;

	// Owner reference
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon State")
	AActor* WeaponOwner;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Upgrade")
	int32 UpgradeLevel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Upgrade")
	float DamageMultiplierPerUpgrade;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Upgrade")
	float AttackSpeedMultiplierPerUpgrade;

	// Projectile class to spawn (for Ranged weapons)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Ranged", meta = (EditCondition = "BaseStats.WeaponCategory == EWeaponCategory::Ranged", EditConditionHides))
	TSubclassOf<class ACC_Projectile> ProjectileClass;

	// Enable auto-aim to nearest enemy
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Ranged", meta = (EditCondition = "BaseStats.WeaponCategory == EWeaponCategory::Ranged", EditConditionHides))
	bool bAutoAim;

	// Auto-aim detection radius
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Ranged", meta = (EditCondition = "BaseStats.WeaponCategory == EWeaponCategory::Ranged && bAutoAim", EditConditionHides))
	float AutoAimRadius;

	//==============================================================================
	// VISUAL/AUDIO COMPONENTS
	//==============================================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	class UNiagaraSystem* AttackEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	class USoundBase* AttackSound;

public:
	//==============================================================================
	// WEAPON INTERFACE
	//==============================================================================

	// Main attack function (to be overridden by child classes)
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void Attack();

	// Check if weapon can attack
	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool CanAttack() const;

	// Weapon equip/unequip
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void OnEquipped(AActor* NewOwner);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void OnUnequipped();

	// Get weapon stats
	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetBaseDamage() const { return BaseStats.BaseDamage; }

	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetAttackSpeed() const;

	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetUpgradeLevel() const { return UpgradeLevel; }

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool ApplyUpgrade();

	//==========================================================================
// TYPE-SPECIFIC ATTACK FUNCTIONS
//==========================================================================

	// Ranged weapon attack
	void PerformRangedAttack();

	// Melee weapon attack
	void PerformMeleeAttack();

	// Magic weapon attack
	void PerformMagicAttack();


protected:
	//==============================================================================
	// HELPER FUNCTIONS
	//==============================================================================

	// Attack cooldown timer
	FTimerHandle AttackCooldownTimer;

	// Reset attack cooldown
	UFUNCTION()
	void ResetCooldown();

	// Play attack effects
	virtual void PlayAttackEffects();

	// Calculate final damage (base damage + owner stats)
	virtual float CalculateFinalDamage() const;

	// Get number of projectiles to spawn (base + upgrades)
	int32 GetFinalProjectileCount() const;

	//==========================================================================
// RANGED HELPER FUNCTIONS
//==========================================================================

	// Spawn a single projectile
	void SpawnProjectile(const FVector& SpawnLocation, const FRotator& SpawnRotation);

	// Spawn multiple projectiles with spread
	void SpawnMultipleProjectiles(const FVector& SpawnLocation, const FRotator& BaseRotation, int32 Count);

	UFUNCTION(BlueprintCallable, Category = "Weapon|Searching")
	class UCC_SearchingComponent* GetSearchingComponent() const;

	// Get firing direction (auto-aim or forward)
	FVector GetFiringDirection() const;

	// Find nearest enemy for auto-aim
	UFUNCTION(BlueprintCallable, Category = "Weapon|Targeting")
	AActor* FindNearestEnemy(float SearchRadius = 0.0f) const;

	UFUNCTION(BlueprintCallable, Category = "Weapon|Targeting")
	TArray<AActor*> FindRandomEnemies(float SearchRadius, int32 Count);

	UFUNCTION(BlueprintCallable, Category = "Weapon|Targeting")
	TArray<AActor*> GetEnemiesInRadius(float Radius);

	UFUNCTION(BlueprintCallable, Category = "Weapon|Targeting")
	TArray<AActor*> GetEnemiesInCone(FVector Direction, float Range, float Angle);

	// Calculate spread rotations
	TArray<FRotator> CalculateSpreadRotations(const FRotator& BaseRotation, int32 Count, float Spread) const;

};
