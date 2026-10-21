// Fill out your copyright notice in the Description page of Project Settings.

#include "CC_Projectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"
#include "../CC_CollisionHelper.h"
#include "../Characters/CC_PlayerCharacter.h"
#include "../SkillSystem/CC_EffectorPoolSubsystem.h"

ACC_Projectile::ACC_Projectile()
{
	// ProjectileMovementComponent ticks by itself; the actor needs no Tick.
	PrimaryActorTick.bCanEverTick = false;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(15.0f);
	RootComponent = CollisionSphere;

	// 적 투사체 — 플레이어만 Overlap
	FCC_CollisionHelper::ConfigureAsEnemyProjectile(CollisionSphere);

	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision); // Fire() enables it

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(CollisionSphere);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->SetUpdatedComponent(CollisionSphere);
	ProjectileMovement->InitialSpeed = Speed;
	ProjectileMovement->MaxSpeed = Speed;
	ProjectileMovement->ProjectileGravityScale = 0.0f; // top-down straight shot
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->bAutoActivate = false;

	SetActorHiddenInGame(true);
}

ACC_Projectile* ACC_Projectile::AcquirePooled(UWorld* World, TSubclassOf<ACC_Projectile> ProjectileClass)
{
	return CC_AcquirePooledEffectorOfClass<ACC_Projectile>(World, ProjectileClass);
}

void ACC_Projectile::BeginPlay()
{
	Super::BeginPlay();

	if (CollisionSphere)
	{
		CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ACC_Projectile::OnOverlapBegin);
	}
}

void ACC_Projectile::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LifetimeTimerHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void ACC_Projectile::Fire(const FVector& SpawnLocation, const FVector& Direction, float InDamage, float InSpeed, AActor* InOwner)
{
	bIsPooledInactive = false;

	const FVector Dir = Direction.GetSafeNormal();
	if (Dir.IsNearlyZero())
	{
		Deactivate();
		return;
	}

	Damage = InDamage;
	if (InSpeed > 0.0f)
	{
		Speed = InSpeed;
	}

	CurrentPierceCount = 0;
	HitActors.Reset();
	ProjectileOwner = InOwner;
	SetInstigator(Cast<APawn>(InOwner));

	SetActorLocationAndRotation(SpawnLocation, Dir.Rotation(), false, nullptr, ETeleportType::TeleportPhysics);
	SetActorHiddenInGame(false);

	if (ProjectileMovement)
	{
		// StopSimulating() can clear UpdatedComponent, so always re-bind
		ProjectileMovement->SetUpdatedComponent(CollisionSphere);
		ProjectileMovement->InitialSpeed = Speed;
		ProjectileMovement->MaxSpeed = Speed;
		ProjectileMovement->Velocity = Dir * Speed;
		ProjectileMovement->Activate(true);
		ProjectileMovement->UpdateComponentVelocity();
	}

	if (CollisionSphere)
	{
		FCC_CollisionHelper::ConfigureAsEnemyProjectile(CollisionSphere);
		CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}

	if (Lifetime > 0.0f)
	{
		GetWorldTimerManager().SetTimer(LifetimeTimerHandle, this, &ACC_Projectile::Deactivate, Lifetime, false);
	}
}

void ACC_Projectile::Deactivate()
{
	if (bIsPooledInactive)
	{
		return;
	}
	bIsPooledInactive = true;

	GetWorldTimerManager().ClearTimer(LifetimeTimerHandle);

	if (CollisionSphere)
	{
		CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->Deactivate();
	}

	SetActorHiddenInGame(true);
	HitActors.Reset();
	ProjectileOwner.Reset();
	SetInstigator(nullptr);

	if (OwningPool)
	{
		OwningPool->ReleaseEffector(this);
	}
	else
	{
		Destroy();
	}
}

void ACC_Projectile::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bIsPooledInactive || !OtherActor || OtherActor == this || OtherActor == ProjectileOwner.Get())
	{
		return;
	}

	// Enemies, skill effectors and other projectiles also overlap (same channel) - ignore silently
	ACC_PlayerCharacter* Player = Cast<ACC_PlayerCharacter>(OtherActor);
	if (!Player || !Player->IsAlive() || HitActors.Contains(Player))
	{
		return;
	}
	HitActors.Add(Player);

	const FVector HitLocation = (bFromSweep && !SweepResult.ImpactPoint.IsNearlyZero())
		? FVector(SweepResult.ImpactPoint)
		: Player->GetActorLocation();

	const FVector ShotDir = ProjectileMovement
		? ProjectileMovement->Velocity.GetSafeNormal()
		: GetActorForwardVector();

	FHitResult Hit = SweepResult;
	Hit.ImpactPoint = HitLocation;
	Hit.Location = HitLocation;

	UGameplayStatics::ApplyPointDamage(Player, Damage, ShotDir, Hit, GetInstigatorController(), this, UDamageType::StaticClass());

	PlayHitEffects(HitLocation);

	if (bCanPierce && CurrentPierceCount < PierceCount)
	{
		++CurrentPierceCount;
		return; // keep flying
	}

	if (bDestroyOnHit)
	{
		Deactivate();
	}
}

void ACC_Projectile::PlayHitEffects(const FVector& HitLocation) const
{
	if (HitEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), HitEffect, HitLocation);
	}

	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, HitSound, HitLocation);
	}
}