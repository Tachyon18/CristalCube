// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_BasicGun.h"
#include "../CC_Projectile.h"
#include "../../CC_SearchingComponent.h"
#include "../../CC_LogHelper.h"


ACC_BasicGun::ACC_BasicGun()
{
    // Weapon info
    BaseStats.WeaponCategory = EWeaponCategory::Ranged;
    BaseStats.BaseDamage = 15.0f;
    BaseStats.AttackSpeed = 2.0f;  // 2 attacks per second

    // Ranged settings
    RangedStats.ProjectileCount = 1;
    RangedStats.ProjectileSpeed = 1200.0f;
    RangedStats.Accuracy = 0.95f;  // 95% accuracy (5% spread)

    // Auto-aim settings
    bAutoAim = true;
    AutoAimRadius = 1500.0f;  // 15 meters

    CC_LOG_WEAPON(Log, "BasicGun created (Damage: %.1f, Speed: %.1f/s)",
        BaseStats.BaseDamage, BaseStats.AttackSpeed);
}

void ACC_BasicGun::Attack()
{
	Super::Attack();

    // Update attack state
    LastAttackTime = GetWorld()->GetTimeSeconds();
    bCanAttack = false;

    // Play effects
    PlayAttackEffects();

    ExecuteGunAttack();

    // Schedule cooldown reset
    const float EffectiveAttackSpeed = FMath::Max(GetAttackSpeed(), KINDA_SMALL_NUMBER);
    float CooldownDuration = 1.0f / EffectiveAttackSpeed;
    GetWorld()->GetTimerManager().SetTimer(
        AttackCooldownTimer,
        this,
        &ACC_BasicGun::ResetCooldown,
        CooldownDuration,
        false
    );

    CC_LOG_WEAPON(VeryVerbose, "BasicGun fired");
}

void ACC_BasicGun::ExecuteGunAttack()
{
    AActor* Target = FindNearestEnemy(AutoAimRadius);
    CC_LOG_WEAPON(Log, TEXT("[GUN] ExecuteGunAttack is Called"));

    if (!Target) return;
 
    FVector TargetLocation = Target->GetActorLocation();
    FVector OwnerLocation = WeaponOwner->GetActorLocation();
    FRotator AimRotation = (TargetLocation - OwnerLocation).Rotation();

    // Get final projectile count (base + upgrades)
    int32 FinalCount = GetFinalProjectileCount();

    // Calculate spread angle (inverse of accuracy)
    float SpreadAngle = (1.0f - RangedStats.Accuracy) * 30.0f;  // Max 30 degree spread

    UCC_SearchingComponent* Searching = GetSearchingComponent();
    if (!Searching)
    {
        CC_LOG_WEAPON(Error, TEXT("[GUN] No SearchingComponent!"));
        return;
    }

    TArray<FRotator> SpreadRotations = Searching->CalculateSpreadRotations(
        AimRotation,
        FinalCount,
        SpreadAngle
    );

    FVector SpawnLocation = OwnerLocation + AimRotation.Vector() * 75.0f;

    for (const FRotator& Rotation : SpreadRotations)
    {
        if (ProjectileClass)
        {
            FActorSpawnParameters SpawnParams;
            SpawnParams.Owner = WeaponOwner;
            SpawnParams.Instigator = Cast<APawn>(WeaponOwner);
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

            ACC_Projectile* Projectile = GetWorld()->SpawnActor<ACC_Projectile>(
                ProjectileClass,
                SpawnLocation,
                Rotation,
                SpawnParams
            );

            if (Projectile)
            {
                float FinalDamage = CalculateFinalDamage();
                Projectile->InitializeProjectile(FinalDamage, RangedStats.ProjectileSpeed);
                Projectile->SetProjectileOwner(WeaponOwner);

                UE_LOG(LogTemp, Log, TEXT("Spawned projectile: Damage=%.1f, Speed=%.1f"),
                    FinalDamage, RangedStats.ProjectileSpeed);
            }
        }
    }
}
