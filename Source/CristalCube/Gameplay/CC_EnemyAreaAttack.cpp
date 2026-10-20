// Fill out your copyright notice in the Description page of Project Settings.

#include "CC_EnemyAreaAttack.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "DrawDebugHelpers.h"
#include "../Characters/CC_PlayerCharacter.h"
#include "../SkillSystem/CC_EffectorPoolSubsystem.h"

static TAutoConsoleVariable<int32> CVarShowEnemyAttackArea(
	TEXT("CC.ShowEnemyAttackArea"),
	0,
	TEXT("0 = off, 1 = draw enemy area-attack rings (orange = expanding, red = hit)."),
	ECVF_Default
);

ACC_EnemyAreaAttack::ACC_EnemyAreaAttack()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);
}

ACC_EnemyAreaAttack* ACC_EnemyAreaAttack::AcquirePooled(UWorld* World, TSubclassOf<ACC_EnemyAreaAttack> AttackClass)
{
	return CC_AcquirePooledEffectorOfClass<ACC_EnemyAreaAttack>(World, AttackClass);
}

void ACC_EnemyAreaAttack::Launch(const FVector& InOrigin, float InMaxRadius, float InDamage, AActor* InInstigator)
{
	Origin = InOrigin;
	MaxRadius = FMath::Max(InMaxRadius, 0.0f);
	Damage = InDamage;
	DamageInstigator = InInstigator;
	Target = Cast<ACC_PlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));

	// Reset progress for pooled reuse
	ElapsedTime = 0.0f;
	PreviousRadius = 0.0f;
	CurrentRadius = 0.0f;
	bHasHit = false;
	bIsActive = true;

	SetActorLocation(Origin);
	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);

	if (RingEffect)
	{
		RingVFX = UNiagaraFunctionLibrary::SpawnSystemAttached(
			RingEffect, Root, NAME_None, FVector::ZeroVector, FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset, false);

		if (RingVFX)
		{
			RingVFX->SetFloatParameter(FName("User.Radius"), 0.0f);
		}
	}
}

void ACC_EnemyAreaAttack::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsActive)
	{
		return;
	}

	if (bFollowInstigator)
	{
		if (const AActor* Inst = DamageInstigator.Get())
		{
			Origin = Inst->GetActorLocation();
			SetActorLocation(Origin);
		}
	}

	ElapsedTime += DeltaTime;
	PreviousRadius = CurrentRadius;

	const float Alpha = (ExpandDuration > 0.0f)
		? FMath::Clamp(ElapsedTime / ExpandDuration, 0.0f, 1.0f)
		: 1.0f;
	CurrentRadius = MaxRadius * Alpha;

	if (RingVFX)
	{
		RingVFX->SetFloatParameter(FName("User.Radius"), CurrentRadius);
	}

#if !UE_BUILD_SHIPPING
	if (CVarShowEnemyAttackArea.GetValueOnGameThread() > 0)
	{
		DrawDebugCircle(GetWorld(), Origin, FMath::Max(CurrentRadius, 1.0f), 32,
			bHasHit ? FColor::Red : FColor::Orange, false, -1.0f, 0, 2.0f,
			FVector(1.0f, 0.0f, 0.0f), FVector(0.0f, 1.0f, 0.0f), false);
	}
#endif

	if (!bHasHit)
	{
		// Band swept this frame: contiguous across frames, so the front never skips a target
		TryHitTarget(PreviousRadius, CurrentRadius + RingThickness);
	}

	if (Alpha >= 1.0f)
	{
		Deactivate();
	}
}

void ACC_EnemyAreaAttack::TryHitTarget(float BandInner, float BandOuter)
{
	ACC_PlayerCharacter* Player = Target.Get();
	if (!Player || !Player->IsAlive())
	{
		return;
	}

	const FVector PlayerLoc = Player->GetActorLocation();

	// Planar test: vertical band first, then 2D ring band widened by capsule radius
	if (FMath::Abs(PlayerLoc.Z - Origin.Z) > HeightTolerance)
	{
		return;
	}

	float TargetRadius = 0.0f;
	if (const UCapsuleComponent* Capsule = Player->GetCapsuleComponent())
	{
		TargetRadius = Capsule->GetScaledCapsuleRadius();
	}

	const float Dist2D = FVector::Dist2D(Origin, PlayerLoc);
	if (Dist2D + TargetRadius < BandInner || Dist2D - TargetRadius > BandOuter)
	{
		return;
	}

	FHitResult Hit;
	Hit.ImpactPoint = PlayerLoc;
	Hit.Location = PlayerLoc;

	const FVector Dir = (PlayerLoc - Origin).GetSafeNormal2D();
	UGameplayStatics::ApplyPointDamage(Player, Damage, Dir, Hit, nullptr, DamageInstigator.Get(), UDamageType::StaticClass());

	bHasHit = true;
}


void ACC_EnemyAreaAttack::Deactivate()
{
	if (!bIsActive)
	{
		return;
	}

	bIsActive = false;
	SetActorTickEnabled(false);
	SetActorHiddenInGame(true);

	if (RingVFX)
	{
		RingVFX->DestroyComponent();
		RingVFX = nullptr;
	}

	DamageInstigator.Reset();
	Target.Reset();

	if (OwningPool)
	{
		OwningPool->ReleaseEffector(this);
	}
	else
	{
		Destroy();
	}
}