// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_SkillEffector.h"
#include "Kismet/GameplayStatics.h"
#include "../CC_LogHelper.h"
#include "Components/SphereComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "GameFramework/ProjectileMovementComponent.h"

// Sets default values
ACC_SkillEffector::ACC_SkillEffector()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	RootComponent = CollisionSphere;
	CollisionSphere->SetSphereRadius(50.0f);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	CollisionSphere->SetNotifyRigidBodyCollision(true);
	CollisionSphere->SetGenerateOverlapEvents(true);

	VFXRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VFXRoot"));
	VFXRoot->SetupAttachment(RootComponent);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->SetUpdatedComponent(CollisionSphere);
	ProjectileMovement->InitialSpeed = 0.0f;
	ProjectileMovement->MaxSpeed = 0.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->SetActive(false);  // 기본 비활성

}

// Called when the game starts or when spawned
void ACC_SkillEffector::BeginPlay()
{
	Super::BeginPlay();
	
	if(CollisionSphere)
	{
		CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ACC_SkillEffector::OnOverlapBegin);
	}

	if (EffectDuration > 0.0f)
	{
		SetLifeSpan(EffectDuration);
	}

	CC_LOG_SKILL(Log, "Spawned - CoreType: %d", (int32)SkillCoreType);
}

// Called every frame
void ACC_SkillEffector::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

UNiagaraComponent* ACC_SkillEffector::AddVFX(UNiagaraSystem* VFXTemplate)
{
	if (!VFXTemplate) 
	{
		CC_LOG_SKILL(Warning, "[SkillEffector] AddVFX : VFXTemplate is null");
		return nullptr;
	}

	UNiagaraComponent* VFXComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
		VFXTemplate,
		VFXRoot,
		NAME_None,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		EAttachLocation::KeepRelativeOffset,
		true  // Auto Destroy
	);

	if (VFXComponent)
	{
		VFXStack.Add(VFXComponent);
		CC_LOG_SKILL(Log, "[SkillEffector] Added VFX: %s", *VFXTemplate->GetName());

	}

	return VFXComponent;
}

void ACC_SkillEffector::SetVFXColor(FLinearColor PrimaryColor, FLinearColor SecondaryColor)
{
	for (UNiagaraComponent* VFX : VFXStack)
	{
		if (VFX)
		{
			VFX->SetVariableLinearColor(FName("User.PrimaryColor"), PrimaryColor);
			VFX->SetVariableLinearColor(FName("User.SecondaryColor"), SecondaryColor);
		}
	}
}

void ACC_SkillEffector::Initialize(ESkillCoreType InCoreType, const FSkillDefinition& InSkillDef)
{
	SkillCoreType = InCoreType;
	SkillDef = InSkillDef;
	
	if (SkillDef.Addons.Contains(ESkillAddonType::Penetrate))
	{
		bCanPenetrate = true;
		MaxPenetrateCount = SkillDef.Passives.PierceCount;
		CurrentPenetrateCount = 0;
	}

	switch (SkillCoreType)
	{
		case ESkillCoreType::Projectile:
			SetupAsProjectile();
			break;
		default:
			break;
	}

	CC_LOG_SKILL(Log, "[SkillEffector] Initialized - Type: %d, Damage: %.1f", (int32)SkillCoreType, SkillDef.BaseDamage);
}

void ACC_SkillEffector::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == this || OtherActor == SkillOwner)
	{
		return;
	}

	if (!OtherActor->ActorHasTag(FName("Enemy")))
	{
		UE_LOG(LogTemp, Warning, TEXT("[SkillEffector] Not an enemy, ignoring: %s"),
			*OtherActor->GetName());
		return;
	}

	if (HitActors.Contains(OtherActor))
	{
		return;
	}

	HitActors.Add(OtherActor);

	UE_LOG(LogTemp, Warning, TEXT("[SkillEffector] Hit: %s"), *OtherActor->GetName());

	ApplyDamageToActor(OtherActor);

	FVector HitLocation = SweepResult.ImpactPoint.IsZero() ?
		OtherActor->GetActorLocation() : FVector(SweepResult.ImpactPoint);


	if (bCanPenetrate)
	{
		CurrentPenetrateCount++;
		
		if (CurrentPenetrateCount >= MaxPenetrateCount)
		{
			UE_LOG(LogTemp, Log, TEXT("[SkillEffector] Penetrate limit reached (%d/%d), destroying."), CurrentPenetrateCount, MaxPenetrateCount);
			Destroy();
		}
	}
	else
	{
		Destroy();
	}

	//PlayHitEffects(HitLocation);

	//if (SkillSystem)
	//{
	//	FHitResult HitResult = SweepResult;
	//	HitResult.ImpactPoint = HitLocation;
	//	HitResult.GetActor() ? HitResult.GetActor() : OtherActor;

	//	SkillSystem->ProcessAddons(CurrentSkill, ExecutionContext, HitResult);
	//}

	//if (bCanPierce && ShouldPierceThrough(OtherActor))
	//{
	//	CurrentPierceCount++;
	//	if (PierceCount > 0 && CurrentPierceCount >= PierceCount)
	//	{
	//		Destroy();
	//	}
	//}
	//else if (bDestroyOnHit)
	//{
	//	Destroy();
	//}
}

void ACC_SkillEffector::ApplyDamageToActor(AActor* TargetActor)
{
	if (!TargetActor)
	{
		return;
	}

	// Apply damage using Unreal's damage system
	UGameplayStatics::ApplyDamage(
		TargetActor,
		SkillDef.BaseDamage,
		GetInstigatorController(),
		this,
		UDamageType::StaticClass()
	);

	UE_LOG(LogTemp, Log, TEXT("Projectile hit %s for %.1f damage"), *TargetActor->GetName(), SkillDef.BaseDamage);
}

void ACC_SkillEffector::SetupAsProjectile()
{
	CollisionSphere->SetSphereRadius(25.0f);

	ProjectileMovement->SetActive(true);
	ProjectileMovement->InitialSpeed = 1000.0f;
	ProjectileMovement->MaxSpeed = 1000.0f;

	CC_LOG_SKILL(Log, "[SkillEffector] Setup as Projectile");
}
