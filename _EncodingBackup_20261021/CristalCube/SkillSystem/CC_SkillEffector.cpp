// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_SkillEffector.h"
#include "CC_EffectorPoolSubsystem.h"
#include "CC_SkillSystem.h"
#include "CC_SkillLibrarySubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "../CC_LogHelper.h"
#include "../CC_CollisionHelper.h"
#include "Components/SphereComponent.h"
#include "../Gameplay/CC_EnemyAIInterface.h"
#include "TimerManager.h"
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
	
	FCC_CollisionHelper::ConfigureAsSkillHittable(CollisionSphere);

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
	TArray<UNiagaraComponent*> AllNiagaraComponents;
	GetComponents<UNiagaraComponent>(AllNiagaraComponents);

	for (UNiagaraComponent* VFX : AllNiagaraComponents)
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

	switch (SkillCoreType)
	{
		case ESkillCoreType::Projectile:
			SetupAsProjectile();
			break;
		case ESkillCoreType::Rainfall:
			SetupAsRainfall();
			break;
		default:
			break;
	}

	const FLinearColor ElementColor = UCC_SkillLibrarySubsystem::ResolveElementColor(this, SkillDef.ElementType);
	SetVFXColor(ElementColor, ElementColor);

	CC_LOG_SKILL(Log, "[SkillEffector] Initialized - Type: %d, Damage: %.1f", (int32)SkillCoreType, SkillDef.BaseDamage);
}

void ACC_SkillEffector::DeactivateAndDestroy()
{
	if (EndGameplay())
	{
		BeginDrain();
	}
}

void ACC_SkillEffector::DeactivateImmediately()
{
	if (EndGameplay() || bIsDraining)
	{
		// Fresh end, or a drain already in progress (hit just before the transition) - release now
		FinishRelease();
	}
}

void ACC_SkillEffector::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == this || OtherActor == SkillOwner)
	{
		return;
	}

	if (!OtherActor->ActorHasTag(FName("Enemy"))) return;


	// Frozen 상태(다른 Cube에 있어 숨겨진 적)는 타겟에서 제외
	if (OtherActor->GetClass()->ImplementsInterface(UCC_EnemyAIInterface::StaticClass())
		&& ICC_EnemyAIInterface::Execute_GetIsFrozen(OtherActor))
	{
		return;
	}

	if (SkillContext.HitActors.Contains(OtherActor))
	{
		return;
	}

	SkillContext.HitActors.Add(OtherActor);

	// 실제 충돌 지점 — 스윕 정보가 유효하면 그대로 사용(적 콜리전 표면의 실제 피격 높이),
	// 스윕이 아니거나 값이 비어있으면 기존처럼 액터 위치로 폴백
	FVector ImpactLocation = OtherActor->GetActorLocation();
	if (bFromSweep && !SweepResult.ImpactPoint.IsNearlyZero())
	{
		ImpactLocation = SweepResult.ImpactPoint;
	}

	OnEffectorHit.Broadcast(this, OtherActor, ImpactLocation);
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

void ACC_SkillEffector::SetupAsRainfall()
{
	// 낙하체는 작게
	CollisionSphere->SetSphereRadius(20.0f);

	// 아래 방향으로 강한 초기 속도 + 중력 강화
	ProjectileMovement->SetActive(true);
	ProjectileMovement->InitialSpeed = 600.0f;
	ProjectileMovement->MaxSpeed = 2000.0f;
	ProjectileMovement->ProjectileGravityScale = 2.5f;
	ProjectileMovement->bRotationFollowsVelocity = true;

	CC_LOG_SKILL(Log, "[SkillEffector] Setup as Rainfall");

}

void ACC_SkillEffector::SetOwningPool(UCC_EffectorPoolSubsystem* InPool)
{
	OwningPool = InPool;
}

void ACC_SkillEffector::SetOwningSkillSystem(UCC_SkillSystem* InSkillSystem)
{
	OwningSkillSystem = InSkillSystem;
}

void ACC_SkillEffector::ActivateAtTransform(const FTransform& NewTransform)
{
	bIsPooledInactive = false;
	bIsDraining = false;
	bReleasedToPool = false;
	PendingDrainVFXCount = 0;
	GetWorldTimerManager().ClearTimer(DrainTimeoutHandle);

	// Restore visuals hidden by the previous drain
	for (const TWeakObjectPtr<UPrimitiveComponent>& Prim : DrainHiddenComponents)
	{
		if (Prim.IsValid())
		{
			Prim->SetVisibility(true);
		}
	}
	DrainHiddenComponents.Reset();

	SetActorTransform(NewTransform);
	SetActorHiddenInGame(false);

	if (CollisionSphere)
	{
		CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		CollisionSphere->SetGenerateOverlapEvents(true);
	}

	if (ProjectileMovement)
	{
		ProjectileMovement->Velocity = FVector::ZeroVector;
		// SetActive(true)와 실제 속도는 곧바로 뒤따르는 Initialize()의
		// SetupAsProjectile()/SetupAsRainfall()이 CoreType에 맞게 세팅함.
		ProjectileMovement->SetActive(false);
	}

	// Restart resident Niagara only (detached one-shots from the previous use are left alone)
	TArray<UNiagaraComponent*> ResidentVFX;
	GetResidentNiagara(ResidentVFX);
	for (UNiagaraComponent* VFX : ResidentVFX)
	{
		VFX->Activate(true);
	}

	SkillContext = FSkillExecutionContext();
	AddonStartIndex = 0;

	if (EffectDuration > 0.0f)
	{
		SetLifeSpan(EffectDuration);
	}
}

void ACC_SkillEffector::LifeSpanExpired()
{
	// 기본 AActor::LifeSpanExpired()는 그냥 Destroy()를 호출함 —
	// 여기서도 VFX가 끊기지 않도록 같은 경로로 통일
	DeactivateAndDestroy();
}

bool ACC_SkillEffector::EndGameplay()
{
	// Guard: already ended (hit + LifeSpan, cube transition + LifeSpan, ...)
	if (bIsPooledInactive)
	{
		return false;
	}
	bIsPooledInactive = true;

	// Cancel a pending LifeSpan timer so it cannot fire while pooled (double release)
	SetLifeSpan(0.0f);

	// Gameplay ends here: no more hits, no more movement
	if (CollisionSphere)
	{
		CollisionSphere->SetGenerateOverlapEvents(false);
		CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (ProjectileMovement)
	{
		ProjectileMovement->SetActive(false);
	}

	// One-shot VFX added via AddVFX (AutoDestroy) are detached and finish on their own
	for (UNiagaraComponent* VFX : VFXStack)
	{
		if (!VFX) continue;
		VFX->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		VFX->Deactivate();
	}
	VFXStack.Empty();

	// Must be cleared before reuse, otherwise OnProjectileHit is bound again on every reuse.
	// Safe even when called from inside Broadcast (dynamic multicast iterates over a copy).
	OnEffectorHit.Clear();

	if (UCC_SkillSystem* SkillSystem = OwningSkillSystem.Get())
	{
		SkillSystem->UnregisterActiveSkillInstance(this);
	}
	return true;
}

void ACC_SkillEffector::BeginDrain()
{
	bIsDraining = true;

	// Non-Niagara visuals (mesh cores etc.) have no fade-out of their own - hide them now
	DrainHiddenComponents.Reset();
	TArray<UPrimitiveComponent*> Primitives;
	GetComponents<UPrimitiveComponent>(Primitives);
	for (UPrimitiveComponent* Prim : Primitives)
	{
		if (!Prim || Prim == CollisionSphere || Prim->IsA<UNiagaraComponent>() || !Prim->GetVisibleFlag()) continue;
		Prim->SetVisibility(false);
		DrainHiddenComponents.Add(Prim);
	}

	// Resident Niagara: stop spawning, let live particles (trail tail etc.) finish naturally.
	// Count first, then deactivate - Deactivate() may report finished synchronously.
	TArray<UNiagaraComponent*> ResidentVFX;
	GetResidentNiagara(ResidentVFX);
	ResidentVFX.RemoveAll([](const UNiagaraComponent* VFX) { return !VFX->IsActive(); });

	PendingDrainVFXCount = ResidentVFX.Num();
	for (UNiagaraComponent* VFX : ResidentVFX)
	{
		VFX->OnSystemFinished.AddUniqueDynamic(this, &ACC_SkillEffector::OnResidentVFXFinished);
	}
	for (UNiagaraComponent* VFX : ResidentVFX)
	{
		VFX->Deactivate();
	}

	if (!bIsDraining)
	{
		return; // everything finished synchronously and FinishRelease already ran
	}
	if (PendingDrainVFXCount <= 0)
	{
		FinishRelease();
		return;
	}

	// Safety net for systems that never report finished (infinite lifetime, Inactive Response = Continue, ...)
	GetWorldTimerManager().SetTimer(DrainTimeoutHandle, this, &ACC_SkillEffector::OnDrainTimeout,
		FMath::Max(MaxDrainTime, 0.01f), false);
}

void ACC_SkillEffector::FinishRelease()
{
	if (bReleasedToPool)
	{
		return;
	}
	bReleasedToPool = true;
	bIsDraining = false;
	PendingDrainVFXCount = 0;
	GetWorldTimerManager().ClearTimer(DrainTimeoutHandle);

	// Kill whatever is left (timeout / cut-short drain) so the next Activate(true) starts clean
	TArray<UNiagaraComponent*> ResidentVFX;
	GetResidentNiagara(ResidentVFX);
	for (UNiagaraComponent* VFX : ResidentVFX)
	{
		VFX->DeactivateImmediate();
	}

	SetActorHiddenInGame(true);

	if (OwningPool)
	{
		OwningPool->ReleaseEffector(this);
	}
	else
	{
		// Spawned without a pool (placed in level, pool not ready yet) - destroy as before
		Destroy();
	}
}

void ACC_SkillEffector::OnDrainTimeout()
{
	CC_LOG_SKILL(Warning, "[SkillEffector] Drain timeout (%.1fs) on %s - check that resident Niagara finishes on Deactivate",
		MaxDrainTime, *GetName());
	FinishRelease();
}

void ACC_SkillEffector::GetResidentNiagara(TArray<UNiagaraComponent*>& OutVFX) const
{
	GetComponents<UNiagaraComponent>(OutVFX);
	OutVFX.RemoveAll([](const UNiagaraComponent* VFX) { return !VFX || !VFX->GetAttachParent(); });
}

void ACC_SkillEffector::OnResidentVFXFinished(UNiagaraComponent* FinishedComponent)
{
	if (!bIsDraining)
	{
		return;
	}
	if (--PendingDrainVFXCount <= 0)
	{
		FinishRelease();
	}
}
