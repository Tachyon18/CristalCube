// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_BasicSword.h"
#include "DrawDebugHelpers.h"
#include "../../CC_LogHelper.h"
#include "Engine/DamageEvents.h"
#include "Engine/OverlapResult.h"

ACC_BasicSword::ACC_BasicSword()
{
	// Weapon info
	BaseStats.WeaponCategory = EWeaponCategory::Melee;
	BaseStats.BaseDamage = 25.0f;
	BaseStats.AttackSpeed = 1.5f;  // 1.5 attacks per second

	MeleeStats.AttackRange = 150.0f;  // 1.5 meters
	MeleeStats.SwingAngle = 90.0f; // 90 degrees
}

void ACC_BasicSword::Attack()
{
	if(!CanAttack())
	{
		return;
	}

	LastAttackTime = GetWorld()->GetTimeSeconds();
	bCanAttack = false;
	PlayAttackEffects();

	ExecuteSwordAttack();

	float CooldownDuration = 1.0f / BaseStats.AttackSpeed;
	GetWorld()->GetTimerManager().SetTimer(
		AttackCooldownTimer,
		this,
		&ACC_BasicSword::ResetCooldown,
		CooldownDuration,
		false
	);
}

void ACC_BasicSword::ExecuteSwordAttack()
{
	if(!WeaponOwner)
	{
		CC_LOG_WEAPON(Warning, "No Weapon Owner");
		return;
	}

	// Perform Melee attack
	PerformMeleeAttack();

	// Setup attack data (Line = horizontal slash)
	FAttackHitData HitData;
	HitData.HitType = EAttackHitType::Line;
	HitData.Range = MeleeStats.AttackRange;
	HitData.Width = MeleeStats.SwingAngle * 2.0f; // 90 degrees wide
	HitData.Thickness = 50.0f;  // 50 cm thick
	HitData.Height = 150.0f;
	HitData.bPenetrate = true;

	// Calculate attack position
	FVector OwnerLocation = WeaponOwner->GetActorLocation();
	FRotator OwnerRotation = WeaponOwner->GetActorRotation();
	FVector Forward = OwnerRotation.Vector();

	// Box center (forward by half range)
	FVector BoxCenter = OwnerLocation + (Forward * HitData.Range * 0.5f);

    // Box extent
    FVector BoxExtent(
        HitData.Thickness * 0.5f,  // 
        HitData.Width * 0.5f,      // Y 
        HitData.Height * 0.5f      // Z 
    );

    // Perform overlap
    TArray<FOverlapResult> OverlapResults;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(WeaponOwner);
    QueryParams.AddIgnoredActor(this);

    bool bHit = GetWorld()->OverlapMultiByChannel(
        OverlapResults,
        BoxCenter,
        OwnerRotation.Quaternion(),
        ECC_Pawn,
        FCollisionShape::MakeBox(BoxExtent),
        QueryParams
    );

    // Apply damage
    if (bHit)
    {
        float FinalDamage = CalculateFinalDamage();

        for (const FOverlapResult& Result : OverlapResults)
        {
            AActor* HitActor = Result.GetActor();
            if (HitActor && HitActor->ActorHasTag("Enemy"))
            {
                FDamageEvent DamageEvent;
                HitActor->TakeDamage(
                    FinalDamage,
                    DamageEvent,
                    WeaponOwner->GetInstigatorController(),
                    WeaponOwner
                );

                UE_LOG(LogTemp, Log, TEXT("[SWORD] Hit %s for %.1f damage"),
                    *HitActor->GetName(), FinalDamage);
            }
        }
    }

    // Debug visualization
#if !UE_BUILD_SHIPPING
    DrawDebugBox(
        GetWorld(),
        BoxCenter,
        BoxExtent,
        OwnerRotation.Quaternion(),
        bHit ? FColor::Green : FColor::Red,
        false,
        0.3f,
        0,
        2.0f
    );
#endif
}
