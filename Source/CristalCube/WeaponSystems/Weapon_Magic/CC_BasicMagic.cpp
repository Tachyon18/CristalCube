// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_BasicMagic.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"
#include "../../CC_LogHelper.h"
#include "NiagaraFunctionLibrary.h"
#include "Engine/DamageEvents.h"


ACC_BasicMagic::ACC_BasicMagic()
{
	// Weapon info
	BaseStats.WeaponCategory = EWeaponCategory::Magic;
	BaseStats.BaseDamage = 25.0f;
	BaseStats.AttackSpeed = 0.8f;

    TargetingMode = ETargetingMode::Single;
	SearchRadius = 800.0f; // 8 meters
	MaxTargets = 3;
	MagicStats.EffectRadius = 400.0f; // 4 meters
	MagicStats.CastTime = 0.3f; // 1 second cast time
	bRequireTarget = true;
}

void ACC_BasicMagic::Attack()
{
    if (!CanAttack())
    {
        return;
    }

    LastAttackTime = GetWorld()->GetTimeSeconds();
    bCanAttack = false;
    PlayAttackEffects();

    ExecuteMagicAttack();

    const float EffectiveAttackSpeed = FMath::Max(GetAttackSpeed(), KINDA_SMALL_NUMBER);
    float CooldownDuration = 1.0f / EffectiveAttackSpeed;
    GetWorld()->GetTimerManager().SetTimer(
        AttackCooldownTimer,
        this,
        &ACC_BasicMagic::ResetCooldown,
        CooldownDuration,
        false
    );
}

void ACC_BasicMagic::ExecuteMagicAttack()
{

    if (!WeaponOwner)
    {
        UE_LOG(LogTemp, Error, TEXT("[Magic] No WeaponOwner!"));
        return;
    }

    switch (TargetingMode)
    {
    case ETargetingMode::Single:
        ExecuteSingleTarget();
        break;

    case ETargetingMode::Multi:
        ExecuteMultiTarget();
        break;

    case ETargetingMode::Area:
        ExecuteAreaTarget();
        break;

    default:
        UE_LOG(LogTemp, Warning, TEXT("[Magic] No targeting mode set"));
        break;
    }

}

void ACC_BasicMagic::ExecuteSingleTarget()
{
	AActor* Target = FindNearestEnemy(SearchRadius);

    if (!Target)
    {
        if (bRequireTarget)
        {
            CC_LOG_WEAPON(Warning, "[Magic] No target (Single Mode)");
            return;
        }

		Target = WeaponOwner;

    }

    FVector TargetLocation = Target->GetActorLocation();
	ApplyMagicDamage(TargetLocation);

	CC_LOG_WEAPON(Log, "[Magic] Single Target at %s", *Target->GetName());
}

void ACC_BasicMagic::ExecuteMultiTarget()
{
	TArray<AActor*> Targets = FindRandomEnemies(SearchRadius, MaxTargets);

    if (Targets.Num() == 0)
    {
        if (bRequireTarget)
        {
            CC_LOG_WEAPON(Warning, "[Magic] No targets (Multi Mode)");
            return;
        }
		ApplyMagicDamage(WeaponOwner->GetActorLocation());
        return;
    }

    for(AActor* Target : Targets)
    {
        if (Target)
        {
			ApplyMagicDamage(Target->GetActorLocation());
        }
	}
}

void ACC_BasicMagic::ExecuteAreaTarget()
{
    FVector PlayerLocation = WeaponOwner->GetActorLocation();
    CC_LOG_WEAPON(Log, "[Magic] Area Attack");
	ApplyMagicDamage(PlayerLocation);

}

void ACC_BasicMagic::ApplyMagicDamage(const FVector& Location)
{
    //// Sphere overlap
    //TArray<FOverlapResult> OverlapResults;
    //FCollisionQueryParams QueryParams;
    //QueryParams.AddIgnoredActor(WeaponOwner);
    //QueryParams.AddIgnoredActor(this);

    //bool bHit = GetWorld()->OverlapMultiByChannel(
    //    OverlapResults,
    //    Location,
    //    FQuat::Identity,
    //    ECC_Pawn,
    //    FCollisionShape::MakeSphere(MagicStats.EffectRadius),
    //    QueryParams
    //);

    //if (!bHit)
    //{
    //    return;
    //}

    //// Apply damage
    //float FinalDamage = CalculateFinalDamage();

    //for (const FOverlapResult& Result : OverlapResults)
    //{
    //    AActor* HitActor = Result.GetActor();
    //    if (HitActor && HitActor->ActorHasTag("Enemy"))
    //    {
    //        FDamageEvent DamageEvent;
    //        HitActor->TakeDamage(
    //            FinalDamage,
    //            DamageEvent,
    //            WeaponOwner->GetInstigatorController(),
    //            WeaponOwner
    //        );

    //        CC_LOG_WEAPON(Log, TEXT("Zapped %s for %.1f damage"),
    //            *HitActor->GetName(), FinalDamage);
    //    }
    //}

    if (MagicEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            GetWorld(),
            MagicEffect,
            Location,
            FRotator::ZeroRotator,
            FVector(4.0f),
            true,
            true,
            ENCPoolMethod::AutoRelease
        );
    }

    FTimerHandle DamageDelayTimer;
    GetWorld()->GetTimerManager().SetTimer(
        DamageDelayTimer,
        [this, Location]()
        {
            TArray<AActor*> HitEnemies = GetEnemiesInRadius(MagicStats.EffectRadius);

            float FinalDamage = CalculateFinalDamage();

            for (AActor* Enemy : HitEnemies)
            {
                if (Enemy && Enemy->ActorHasTag("Enemy"))
                {
                    FDamageEvent DamageEvent;
                    Enemy->TakeDamage(
                        FinalDamage,
                        DamageEvent,
                        WeaponOwner->GetInstigatorController(),
                        WeaponOwner
                    );
                }

                CC_LOG_WEAPON(Log, TEXT("Magic hit %s for %.1f damage"),
                    *Enemy->GetName(), FinalDamage);
            }
        },
        0.2f,
        false
    );




    // Debug visualization
#if !UE_BUILD_SHIPPING
    DrawDebugSphere(
        GetWorld(),
        Location,
        MagicStats.EffectRadius,
        16,
        FColor::Yellow,
        false,
        0.3f,
        0,
        2.0f
    );
#endif
}
