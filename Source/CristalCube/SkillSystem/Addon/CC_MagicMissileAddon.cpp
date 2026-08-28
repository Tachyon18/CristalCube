// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_MagicMissileAddon.h"
#include "../CC_SkillSystem.h"
#include "../CC_SkillEffector.h"
#include "GameFramework/ProjectileMovementComponent.h"

void UCC_MagicMissileAddon::OnHit_Implementation(UCC_SkillSystem* SkillSystem, const FSkillDefinition& Skill, FSkillExecutionContext& Context, AActor* HitTarget, FVector HitLocation)
{
	if (!SkillSystem || !SkillSystem->GetWorld() || !Context.Caster) return;

    TSubclassOf<ACC_SkillEffector> EffectorClass = Data.MagicMissileEffectorClass ? Data.MagicMissileEffectorClass : Skill.ProjectileClass;

    if (!EffectorClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MagicMissileAddon] No effector class available"));
        return;
    }

    // 발사 지점: 캐스터 정수리 위
    AActor* Caster = Context.Caster;
    const FVector SpawnLocation = Caster->GetActorLocation() + FVector(0.f, 0.f, Caster->GetSimpleCollisionHalfHeight() + Data.SpawnHeightOffset);

    // 목표 지점 미리 확보 — HitLocation 중심 X/Y 산개, Z는 HitLocation 고정
    TArray<FVector> TargetPoints;
    for (int32 i = 0; i < Data.LaunchCount; ++i)
    {
        const FVector2D Offset = FMath::RandPointInCircle(Data.ScatterRadius);
        TargetPoints.Add(HitLocation + FVector(Offset.X, Offset.Y, 0.f));
    }

    TWeakObjectPtr<UCC_SkillSystem> WeakSystem = SkillSystem;
    FSkillDefinition SkillCopy = Skill;
    FSkillExecutionContext ContextCopy = Context;
    ContextCopy.CurrentDamage = Context.CurrentDamage * Data.DamageRatio;
    FMagicMissileAddonData DataCopy = Data;
    const int32 StartIndex = AddonIndex + 1;
    AActor* CasterActor = Context.Caster;

    TSharedPtr<int32> LaunchedCount = MakeShared<int32>(0);
    TSharedPtr<FTimerHandle> TimerHandlePtr = MakeShared<FTimerHandle>();

    FTimerDelegate LaunchDelegate = FTimerDelegate::CreateLambda(
        [WeakSystem, TargetPoints, SkillCopy, ContextCopy, DataCopy, SpawnLocation, CasterActor, StartIndex, EffectorClass, LaunchedCount, TimerHandlePtr]() mutable
        {
            if (!WeakSystem.IsValid() || *LaunchedCount >= TargetPoints.Num())
            {
                if (WeakSystem.IsValid())
                {
                    WeakSystem->GetWorld()->GetTimerManager().ClearTimer(*TimerHandlePtr);
                }
                return;
            }

            const FVector TargetPoint = TargetPoints[*LaunchedCount];
            (*LaunchedCount)++;

            const FVector Direction = (TargetPoint - SpawnLocation).GetSafeNormal();
            const FTransform SpawnTransform(Direction.ToOrientationQuat(), SpawnLocation);

            ACC_SkillEffector* Missile = WeakSystem->GetWorld()->SpawnActor<ACC_SkillEffector>(EffectorClass, SpawnTransform);
            if (Missile)
            {
                Missile->SetSkillOwner(CasterActor);
                Missile->Initialize(ESkillCoreType::Projectile, SkillCopy);

                if (UProjectileMovementComponent* Movement = Missile->ProjectileMovement)
                {
                    Movement->bIsHomingProjectile = false;   // 명시적 — 이제 고정 지점 직선 사격
                    Movement->ProjectileGravityScale = 0.0f; // 직선 비행
                    Movement->InitialSpeed = DataCopy.ProjectileSpeed;
                    Movement->MaxSpeed = DataCopy.ProjectileSpeed;
                    Movement->Velocity = Direction * DataCopy.ProjectileSpeed;
                }

                Missile->SetLifeSpan(DataCopy.LifeTime);
                Missile->AddonStartIndex = StartIndex;
                Missile->SkillContext = ContextCopy;

                WeakSystem->RegisterActiveSkillInstance(Missile);
                Missile->OnEffectorHit.AddDynamic(WeakSystem.Get(), &UCC_SkillSystem::OnProjectileHit);
            }

            if (DataCopy.LaunchEffect)
            {
                WeakSystem->SpawnEffect(DataCopy.LaunchEffect, SpawnLocation);
            }

            if (*LaunchedCount >= TargetPoints.Num())
            {
                WeakSystem->GetWorld()->GetTimerManager().ClearTimer(*TimerHandlePtr);
            }
        });

    LaunchDelegate.ExecuteIfBound();  // 1발째 즉시 발사

    if (TargetPoints.Num() > 1)
    {
        SkillSystem->GetWorld()->GetTimerManager().SetTimer(*TimerHandlePtr, LaunchDelegate, DataCopy.LaunchInterval, true);
    }
}

void UCC_MagicMissileAddon::ApplyModifier_Implementation(FName AttributeID, float ValuePerPoint)
{
    if (AttributeID == TEXT("MagicMissileLaunchCount"))
    {
        Data.LaunchCount += FMath::RoundToInt(ValuePerPoint);
    }
    else if (AttributeID == TEXT("MagicMissileDamageRatio"))
    {
        Data.DamageRatio = FMath::Max(Data.DamageRatio + ValuePerPoint, 0.f);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[MagicMissileAddon] ApplyModifier: unknown AttributeID '%s'"), *AttributeID.ToString());
    }
}
