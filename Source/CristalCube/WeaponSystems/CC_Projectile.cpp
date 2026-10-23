// Fill out your copyright notice in the Description page of Project Settings.

#include "CC_Projectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "TimerManager.h"
#include "../CC_CollisionHelper.h"
#include "../Characters/CC_PlayerCharacter.h"
#include "../SkillSystem/CC_EffectorPoolSubsystem.h"
#include "DrawDebugHelpers.h"
#include "Engine/CollisionProfile.h"

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

namespace
{
	// 디버그: 적 투사체 발사 시점의 실제 콜리전 값 + 진행 방향 차단 대상 (CC.ShowSkillCollision 1일 때만)
	void DebugReportEnemyProjectileCollision(const AActor* Self, const USphereComponent* Sphere,
		const UProjectileMovementComponent* Movement, const FVector& Dir)
	{
		if (!FCC_CollisionHelper::IsSkillCollisionDebugEnabled() || !Self || !Sphere || !Self->GetWorld())
		{
			return;
		}

		auto ChannelName = [](ECollisionChannel Channel)
			{
				return UCollisionProfile::Get()->ReturnChannelNameFromContainerIndex(Channel).ToString();
			};
		auto ResponseName = [](ECollisionResponse Response)
			{
				return Response == ECR_Block ? TEXT("Block") : (Response == ECR_Overlap ? TEXT("Overlap") : TEXT("Ignore"));
			};

		UWorld* World = Self->GetWorld();
		const ECollisionChannel MyType = Sphere->GetCollisionObjectType();

		// 1) 실제 런타임 값 — 빌드 반영 여부(WorldStatic/WorldDynamic=Block인지)와 BP의 Sweep 설정이 여기서 보임
		UE_LOG(LogTemp, Warning,
			TEXT("[EnemyProjCollision] %s | Profile=%s Obj=%s Enabled=%d | WorldStatic=%s WorldDynamic=%s Pawn=%s | Sweep=%d Updated=%s Active=%d Radius=%.0f"),
			*Self->GetName(),
			*Sphere->GetCollisionProfileName().ToString(),
			*ChannelName(MyType),
			(int32)Sphere->GetCollisionEnabled(),
			ResponseName(Sphere->GetCollisionResponseToChannel(ECC_WorldStatic)),
			ResponseName(Sphere->GetCollisionResponseToChannel(ECC_WorldDynamic)),
			ResponseName(Sphere->GetCollisionResponseToChannel(ECC_Pawn)),
			Movement ? (int32)Movement->bSweepCollision : -1,
			*GetNameSafe(Movement ? Movement->UpdatedComponent.Get() : nullptr),
			Movement ? (int32)Movement->IsActive() : -1,
			Sphere->GetScaledSphereRadius());

		const FVector Start = Sphere->GetComponentLocation();
		const FVector End = Start + Dir * 3000.0f;
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(EnemyProjCollisionDebug), false, Self);

		// 2) 이동 스윕과 같은 조건(내 오브젝트 타입 + 내 응답)으로 진행 방향 3000cm — "저 벽이 날 막아주는가"
		const FCollisionResponseParams ResponseParams(Sphere->GetCollisionResponseToChannels());
		FHitResult Hit;
		if (World->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, MyType,
			FCollisionShape::MakeSphere(Sphere->GetScaledSphereRadius()), QueryParams, ResponseParams))
		{
			DrawDebugLine(World, Start, Hit.ImpactPoint, FColor::Green, false, 3.0f, 0, 1.5f);
			UE_LOG(LogTemp, Warning, TEXT("[EnemyProjCollision]   진행 방향 차단 대상: %s / %s (Obj=%s)"),
				*GetNameSafe(Hit.GetActor()), *GetNameSafe(Hit.GetComponent()),
				Hit.GetComponent() ? *ChannelName(Hit.GetComponent()->GetCollisionObjectType()) : TEXT("None"));
		}
		else
		{
			DrawDebugLine(World, Start, End, FColor::Red, false, 3.0f, 0, 1.5f);
			UE_LOG(LogTemp, Warning, TEXT("[EnemyProjCollision]   진행 방향 3000cm 안에 나를 막는 것 없음"));
		}

		// 3) 응답과 무관하게 진행 방향 첫 물체 — 상대(벽)가 나에게 실제로 어떻게 응답하는지
		FHitResult AnyHit;
		const FCollisionObjectQueryParams AllObjects(FCollisionObjectQueryParams::AllObjects);
		if (World->LineTraceSingleByObjectType(AnyHit, Start, End, AllObjects, QueryParams) && AnyHit.GetComponent())
		{
			const UPrimitiveComponent* Other = AnyHit.GetComponent();
			UE_LOG(LogTemp, Warning,
				TEXT("[EnemyProjCollision]   진행 방향 첫 물체: %s / %s | Profile=%s Obj=%s Enabled=%d | 나(Obj=%s)에 대한 응답=%s"),
				*GetNameSafe(AnyHit.GetActor()), *GetNameSafe(Other),
				*Other->GetCollisionProfileName().ToString(),
				*ChannelName(Other->GetCollisionObjectType()),
				(int32)Other->GetCollisionEnabled(),
				*ChannelName(MyType),
				ResponseName(Other->GetCollisionResponseToChannel(MyType)));
		}
	}
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

	if (ProjectileMovement)
	{
		// 지형에 막히면 호출 (bShouldBounce=false라 첫 Block에서 정지)
		ProjectileMovement->OnProjectileStop.AddDynamic(this, &ACC_Projectile::OnProjectileStopped);
	}
}

void ACC_Projectile::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LifetimeTimerHandle);
		World->GetTimerManager().ClearTimer(DrainTimeoutHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void ACC_Projectile::Fire(const FVector& SpawnLocation, const FVector& Direction, float InDamage, float InSpeed, AActor* InOwner)
{
	bIsPooledInactive = false;
	bIsDraining = false;
	bReleasedToPool = false;
	PendingDrainVFXCount = 0;
	GetWorldTimerManager().ClearTimer(DrainTimeoutHandle);

	// 이전 Drain에서 숨긴 메시 등 복구
	for (const TWeakObjectPtr<UPrimitiveComponent>& Prim : DrainHiddenComponents)
	{
		if (Prim.IsValid())
		{
			Prim->SetVisibility(true);
		}
	}
	DrainHiddenComponents.Reset();

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

	// 풀 재사용 — 반납 때 꺼둔 상주 나이아가라(BP 컴포넌트)를 처음부터 다시 재생
	// (시스템이 한 번 끝나면 숨김 해제만으로는 다시 보이지 않음 — 스킬 Effector ActivateAtTransform과 같은 처리)
	TArray<UNiagaraComponent*> ResidentVFX;
	GetResidentNiagara(ResidentVFX);
	for (UNiagaraComponent* VFX : ResidentVFX)
	{
		VFX->Activate(true);
	}

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
		GetWorldTimerManager().SetTimer(LifetimeTimerHandle, this, &ACC_Projectile::EndAndDrain, Lifetime, false);
	}

	DebugReportEnemyProjectileCollision(this, CollisionSphere, ProjectileMovement, Dir);
}

void ACC_Projectile::Deactivate()
{
	// 즉시 반납: 진행 중이면 판정 종료 후 바로, Drain 중이면 대기를 끊고 바로
	if (EndGameplay() || bIsDraining)
	{
		FinishRelease();
	}
}

void ACC_Projectile::EndAndDrain()
{
	if (!EndGameplay())
	{
		return;
	}
	bIsDraining = true;

	// 나이아가라가 아닌 보이는 것(메시 등)은 스스로 사라지는 연출이 없으므로 지금 숨김
	DrainHiddenComponents.Reset();
	TArray<UPrimitiveComponent*> Primitives;
	GetComponents<UPrimitiveComponent>(Primitives);
	for (UPrimitiveComponent* Prim : Primitives)
	{
		if (!Prim || Prim == CollisionSphere || Prim->IsA<UNiagaraComponent>() || !Prim->GetVisibleFlag()) continue;
		Prim->SetVisibility(false);
		DrainHiddenComponents.Add(Prim);
	}

	// 상주 나이아가라: 새 생성만 멈추고 남은 파티클(트레일 꼬리 등)은 자연 종료.
	// 먼저 세고 나서 Deactivate — Deactivate가 동기적으로 종료를 알릴 수 있음
	TArray<UNiagaraComponent*> ResidentVFX;
	GetResidentNiagara(ResidentVFX);
	ResidentVFX.RemoveAll([](const UNiagaraComponent* VFX) { return !VFX->IsActive(); });

	PendingDrainVFXCount = ResidentVFX.Num();
	for (UNiagaraComponent* VFX : ResidentVFX)
	{
		VFX->OnSystemFinished.AddUniqueDynamic(this, &ACC_Projectile::OnResidentVFXFinished);
	}
	for (UNiagaraComponent* VFX : ResidentVFX)
	{
		VFX->Deactivate();
	}

	if (!bIsDraining)
	{
		return; // 전부 동기 종료되어 이미 반납됨
	}
	if (PendingDrainVFXCount <= 0)
	{
		FinishRelease();
		return;
	}

	// 끝났다고 알려오지 않는 시스템(무한 수명, Inactive Response = Continue 등) 대비
	GetWorldTimerManager().SetTimer(DrainTimeoutHandle, this, &ACC_Projectile::FinishRelease,
		FMath::Max(MaxDrainTime, 0.01f), false);
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
		// 판정은 지금 끝내고 연출(트레일 등)은 스스로 마무리
		EndAndDrain();
	}
}

void ACC_Projectile::OnProjectileStopped(const FHitResult& ImpactResult)
{
	// 이미 명중/수명 만료로 반납됐으면 무시 (같은 프레임 명중 + 지형 충돌)
	if (bIsPooledInactive)
	{
		return;
	}

	if (FCC_CollisionHelper::IsSkillCollisionDebugEnabled())
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyProjCollision] %s 지형 착탄: %s"), *GetName(), *GetNameSafe(ImpactResult.GetActor()));
	}
	// 실제 충돌 지점에서 착탄 연출 — 플레이어 명중과 같은 HitEffect 슬롯
	PlayHitEffects(ImpactResult.ImpactPoint);
	EndAndDrain();
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

bool ACC_Projectile::EndGameplay()
{
	if (bIsPooledInactive)
	{
		return false;
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

	HitActors.Reset();
	// ProjectileOwner는 반납 때까지 유지 — Drain 중에도 쏜 적의 CancelActiveAttacks가 즉시 정리할 수 있게
	return true;
}

void ACC_Projectile::FinishRelease()
{
	if (bReleasedToPool)
	{
		return;
	}
	bReleasedToPool = true;
	bIsDraining = false;
	PendingDrainVFXCount = 0;
	GetWorldTimerManager().ClearTimer(DrainTimeoutHandle);

	// 남은 것(타임아웃/중간 취소) 정리 — 다음 Fire의 Activate(true)가 깨끗하게 시작하도록
	TArray<UNiagaraComponent*> ResidentVFX;
	GetResidentNiagara(ResidentVFX);
	for (UNiagaraComponent* VFX : ResidentVFX)
	{
		VFX->DeactivateImmediate();
	}

	SetActorHiddenInGame(true);
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

void ACC_Projectile::GetResidentNiagara(TArray<UNiagaraComponent*>& OutVFX) const
{
	GetComponents<UNiagaraComponent>(OutVFX);
	OutVFX.RemoveAll([](const UNiagaraComponent* VFX) { return !VFX || !VFX->GetAttachParent(); });
}

void ACC_Projectile::OnResidentVFXFinished(UNiagaraComponent* FinishedComponent)
{
	// 진행 중 스스로 끝난 시스템(1회 재생 등)이나 반납 정리 중 알림은 무시
	if (!bIsDraining)
	{
		return;
	}
	if (--PendingDrainVFXCount <= 0)
	{
		FinishRelease();
	}
}
