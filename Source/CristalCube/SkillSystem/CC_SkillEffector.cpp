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
	if (bIsPooledInactive)
	{
		// 이미 반납/파괴 처리된 인스턴스 — 중복 호출 방지(큐브 전환 정리와 LifeSpan 타이머가
		// 겹치는 경우 등).
		return;
	}
	bIsPooledInactive = true;

	// 대기 중이던 LifeSpan 타이머가 있으면 취소 — 이 함수가 큐브 전환 등으로 "조기 호출"된
	// 경우, 취소 안 하면 풀에서 대기하는 도중 타이머가 불시에 터져서 같은 액터가 중복으로
	// 반납(풀 오염)될 수 있음.
	SetLifeSpan(0.0f);

	// 콜리전/이동 즉시 정지 — 더 이상 새로운 히트나 이동을 만들지 않음
	if (CollisionSphere)
	{
		CollisionSphere->SetGenerateOverlapEvents(false);
		CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	
	if (ProjectileMovement)
	{
		ProjectileMovement->SetActive(false);
	}

	// AddVFX()로 붙인 일회성 VFX(AutoDestroy=true)만 Detach 후 Deactivate — 액터가 풀에
	// 반납되거나 파괴돼도 잔여 파티클/Death Event를 스스로 끝까지 재생하고 알아서 정리됨.
	// (액터에 계속 붙여두면 풀에서 재사용될 때 위치가 꼬이므로 반드시 떼어냄.)
	for (UNiagaraComponent* VFX : VFXStack)
	{
		if (!VFX) continue;

		VFX->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		VFX->Deactivate();
	}
	VFXStack.Empty();

	// 그 외(BP에서 VFXRoot 하위에 상주로 붙여둔) 나이아가라 컴포넌트는 액터에 그대로 붙여둔
	// 채 Deactivate만 — 위치를 다시 계산할 필요 없이, 재사용될 때 Activate(true)만 하면
	// 그대로 재생됨. Destroy() 폴백 경로(풀 없을 때)에선 액터가 통째로 사라지니 이대로 둬도
	// 문제 없음.
	TArray<UNiagaraComponent*> PersistentVFX;
	GetComponents<UNiagaraComponent>(PersistentVFX);
	for (UNiagaraComponent* VFX : PersistentVFX)
	{
		if (VFX)
		{
			VFX->Deactivate();
		}
	}

	// 델리게이트 초기화 — 안 하면 재사용될 때마다 OnProjectileHit이 중복 바인딩되어,
	// 한 번 맞았는데 데미지가 여러 번 들어가는 버그가 됨(풀링 전엔 매번 새 액터라
	// 문제된 적 없었음).
	OnEffectorHit.Clear();

	if (UCC_SkillSystem* SkillSystem = OwningSkillSystem.Get())
	{
		SkillSystem->UnregisterActiveSkillInstance(this);
	}

	SetActorHiddenInGame(true);

	if (OwningPool)
	{
		OwningPool->ReleaseEffector(this);
	}
	else
	{
		// 풀 없이 스폰된 경우(레벨에 직접 배치했거나, 풀 서브시스템이 아직 없던 시점에
		// 스폰됐거나) — 예전처럼 그냥 파괴.
		Destroy();
	}
}

void ACC_SkillEffector::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UE_LOG(LogTemp, Log, TEXT("[SkillEffector] Overlap Begin with: %s"), *OtherActor->GetName());

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

	UE_LOG(LogTemp, Warning, TEXT("[SkillEffector] Hit: %s"), *OtherActor->GetName());

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

	// BP에 상주하는(위 DeactivateAndDestroy에서 Deactivate만 되고 안 떼어졌던) 나이아가라
	// 컴포넌트 재생 재시작 — 위치는 VFXRoot에 계속 붙어있으니 따로 안 건드림.
	TArray<UNiagaraComponent*> PersistentVFX;
	GetComponents<UNiagaraComponent>(PersistentVFX);
	for (UNiagaraComponent* VFX : PersistentVFX)
	{
		if (VFX)
		{
			VFX->Activate(true);
		}
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
