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
#include "DrawDebugHelpers.h"
#include "Engine/CollisionProfile.h"
#include "GameFramework/ProjectileMovementComponent.h"

// Sets default values
ACC_SkillEffector::ACC_SkillEffector()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	RootComponent = CollisionSphere;
	CollisionSphere->SetSphereRadius(50.0f);
	//CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	//CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	
	FCC_CollisionHelper::ConfigureAsSkillProjectile(CollisionSphere);

	CollisionSphere->SetNotifyRigidBodyCollision(true);
	CollisionSphere->SetGenerateOverlapEvents(true);

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


	if (ProjectileMovement)
	{
		// 지형에 막혀 멈추면 호출 (bShouldBounce=false라 첫 Block에서 정지)
		ProjectileMovement->OnProjectileStop.AddDynamic(this, &ACC_SkillEffector::OnProjectileStopped);
	}

	CC_LOG_SKILL(Log, "Spawned - CoreType: %d", (int32)SkillCoreType);

	StartCollisionDebugIfEnabled();
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
		RootComponent,
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

	// 투사체 속도를 나이아가라에 전달 — 에셋이 User.ProjectileSpeed를 선언한 경우에만 의미 있음.
	// 연출 보정용(트레일 길이/스폰률 등)이며, 헤드 이동은 액터(ProjectileMovement)가 담당.
	if ((SkillCoreType == ESkillCoreType::Projectile || SkillCoreType == ESkillCoreType::Rainfall) && ProjectileMovement)
	{
		TArray<UNiagaraComponent*> NiagaraComps;
		GetComponents<UNiagaraComponent>(NiagaraComps);
		for (UNiagaraComponent* VFX : NiagaraComps)
		{
			if (VFX)
			{
				VFX->SetVariableFloat(FName("User.ProjectileSpeed"), ProjectileMovement->InitialSpeed);
			}
		}
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

	if (!OtherActor->ActorHasTag(FName("Enemy")))
	{
		// 디버그: 적이 아닌 무엇과 겹쳤는지(이름 + 오브젝트 타입) — 채널 문제 확인용
		DrawOverlapDebug(OtherActor, OtherComp, GetActorLocation(), FColor::White);
		return;
	}

	// Frozen 상태(다른 Cube에 있어 숨겨진 적)는 타겟에서 제외
	if (OtherActor->GetClass()->ImplementsInterface(UCC_EnemyAIInterface::StaticClass())
		&& ICC_EnemyAIInterface::Execute_GetIsFrozen(OtherActor))
	{
		DrawOverlapDebug(OtherActor, OtherComp, GetActorLocation(), FColor::Cyan);
		return;
	}

	if (SkillContext.HitActors.Contains(OtherActor))
	{
		return;
	}

	SkillContext.HitActors.Add(OtherActor);

	// 실제 충돌 지점 — 스윕 정보가 유효하면 그대로 사용(적 콜리전 표면의 실제 피격 높이),
	// 스윕이 아니거나 값이 비어있으면 액터 위치로 폴백
	FVector ImpactLocation = OtherActor->GetActorLocation();
	if (bFromSweep && !SweepResult.ImpactPoint.IsNearlyZero())
	{
		ImpactLocation = SweepResult.ImpactPoint;
	}

	DrawOverlapDebug(OtherActor, OtherComp, ImpactLocation, FColor::Red);

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
	// 판정 반경/속도는 스킬 데이터에서 — 코드 고정값 대신 스킬마다 조절
	CollisionSphere->SetSphereRadius(FMath::Max(SkillDef.ProjectileCollisionRadius, 1.0f));

	const float Speed = FMath::Max(SkillDef.ProjectileSpeed * SkillDef.Passives.SpeedMultiplier, 0.0f);

	ProjectileMovement->SetActive(true);
	ProjectileMovement->InitialSpeed = Speed;
	ProjectileMovement->MaxSpeed = Speed;
	ProjectileMovement->ProjectileGravityScale = 0.0f;   // 같은 BP가 Rainfall로 쓰였던 흔적(중력 2.5) 제거
	// InitialSpeed는 컴포넌트 초기화 때만 반영됨 — 풀 재사용을 위해 Velocity를 직접 지정
	ProjectileMovement->Velocity = GetActorForwardVector() * Speed;

	CC_LOG_SKILL(Log, "[SkillEffector] Setup as Projectile - Speed %.0f, Radius %.0f", Speed, CollisionSphere->GetUnscaledSphereRadius());
}

void ACC_SkillEffector::SetupAsRainfall()
{
	const FRainfallCoreData& Data = SkillDef.RainfallData;

	CollisionSphere->SetSphereRadius(FMath::Max(Data.CollisionRadius, 1.0f));

	const float StartSpeed = FMath::Max(Data.FallInitialSpeed, 0.0f);
	// 0 = 제한 없음(ProjectileMovement 규칙). 값이 있으면 시작 속도보다 작아지지 않게
	const float MaxSpeed = Data.FallMaxSpeed > 0.0f ? FMath::Max(Data.FallMaxSpeed, StartSpeed) : 0.0f;

	ProjectileMovement->SetActive(true);
	ProjectileMovement->InitialSpeed = StartSpeed;
	ProjectileMovement->MaxSpeed = MaxSpeed;
	ProjectileMovement->ProjectileGravityScale = FMath::Max(Data.FallGravityScale, 0.0f);
	ProjectileMovement->bRotationFollowsVelocity = true;
	// 스폰 회전이 수직 아래이므로 Forward = 낙하 방향. 풀 재사용 대비 Velocity 직접 지정
	ProjectileMovement->Velocity = GetActorForwardVector() * StartSpeed;

	CC_LOG_SKILL(Log, "[SkillEffector] Setup as Rainfall - Start %.0f, Max %.0f, Gravity x%.1f, Radius %.0f",
		StartSpeed, MaxSpeed, ProjectileMovement->ProjectileGravityScale, CollisionSphere->GetUnscaledSphereRadius());
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

	// 판정은 아직 켜지 않음 — 이동(텔레포트) 중 엉뚱한 오버랩 방지. 실제 ON은 Launch()에서.
	if (CollisionSphere)
	{
		CollisionSphere->SetGenerateOverlapEvents(false);
		CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// 이전 사용의 소유자가 남아 있으면 안 됨 — 호출부가 Launch() 전에 다시 세팅
	SkillOwner = nullptr;

	SetActorTransform(NewTransform);
	SetActorHiddenInGame(false);

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

	// 재사용될 때마다 다시 확인 — 토글을 켠 뒤 새로 나가는 투사체부터 그려짐
	StartCollisionDebugIfEnabled();
}

void ACC_SkillEffector::Launch()
{
	if (bIsPooledInactive)
	{
		return;
	}

	if (CollisionSphere)
	{
		// 역할 콜리전(적 Overlap + 지정 지형 Block)을 발사 시점에 보장 — BP 프리셋이 덮어써도 유지
		FCC_CollisionHelper::ConfigureAsSkillProjectile(CollisionSphere);
		CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		CollisionSphere->SetGenerateOverlapEvents(true);
	}

	if (ProjectileMovement && ProjectileMovement->IsActive())
	{
		// InitialSpeed는 컴포넌트 초기화 시점에만 Velocity로 바뀜 — 풀 재사용/Setup 이후엔 직접 넣어야 실제로 날아감.
		// 호출부가 이미 Velocity를 정했으면(MagicMissile 등) 그대로 둠.
		if (ProjectileMovement->Velocity.IsNearlyZero())
		{
			ProjectileMovement->Velocity = GetActorForwardVector() * ProjectileMovement->InitialSpeed;
		}
		ProjectileMovement->SetUpdatedComponent(CollisionSphere);
		ProjectileMovement->UpdateComponentVelocity();
	}

	// 켜는 순간 이미 겹쳐 있는 대상도 BeginOverlap을 받도록 즉시 갱신
	UpdateOverlaps();

	DebugReportCollision();
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
	GetWorldTimerManager().ClearTimer(CollisionDebugTimer);

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


void ACC_SkillEffector::StartCollisionDebugIfEnabled()
{
	// 토글은 활성화 시점에 한 번만 확인 — 꺼져 있으면 타이머 자체를 만들지 않음
	if (!FCC_CollisionHelper::IsSkillCollisionDebugEnabled())
	{
		return;
	}
	GetWorldTimerManager().SetTimer(CollisionDebugTimer, this, &ACC_SkillEffector::DrawCollisionDebug, 0.05f, true);
	DrawCollisionDebug();   // 첫 프레임 즉시 1회
}

void ACC_SkillEffector::DrawCollisionDebug()
{
	// 판정 중이면 노랑, Drain 중(판정 종료)이면 회색
	FCC_CollisionHelper::DrawSkillSphereDebug(GetWorld(), CollisionSphere, !bIsPooledInactive);
}

void ACC_SkillEffector::DrawOverlapDebug(const AActor* OtherActor, const UPrimitiveComponent* OtherComp, const FVector& Point, const FColor& Color) const
{
	if (!FCC_CollisionHelper::IsSkillCollisionDebugEnabled())
	{
		return;
	}

	const FString ObjectType = OtherComp
		? UCollisionProfile::Get()->ReturnChannelNameFromContainerIndex(OtherComp->GetCollisionObjectType()).ToString()
		: TEXT("None");

	FCC_CollisionHelper::DrawSkillOverlapDebug(GetWorld(), Point, Color,
		FString::Printf(TEXT("%s [%s]"), *GetNameSafe(OtherActor), *ObjectType));
}

void ACC_SkillEffector::OnProjectileStopped(const FHitResult& ImpactResult)
{
	// 이미 적중 등으로 판정이 끝났으면 무시 (같은 프레임 적중 + 지형 충돌)
	if (bIsPooledInactive)
	{
		return;
	}

	// 디버그: 주황 = 지형 충돌 (상대 이름 + 오브젝트 타입)
	DrawOverlapDebug(ImpactResult.GetActor(), ImpactResult.GetComponent(), ImpactResult.ImpactPoint, FColor::Orange);

	if (UCC_SkillSystem* SkillSystem = OwningSkillSystem.Get())
	{
		SkillSystem->OnEffectorTerrainHit(this, ImpactResult);
	}
	else
	{
		// SkillSystem 없이 스폰된 경우의 안전 경로 — 판정 종료 후 연출은 스스로 마무리
		DeactivateAndDestroy();
	}
}

void ACC_SkillEffector::DebugReportCollision() const
{
	if (!FCC_CollisionHelper::IsSkillCollisionDebugEnabled() || !CollisionSphere || !GetWorld())
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

	const ECollisionChannel MyType = CollisionSphere->GetCollisionObjectType();

	// 1) 실제 런타임 값 — BP 프리셋이 덮어썼는지 여기서 바로 보임
	UE_LOG(LogTemp, Warning,
		TEXT("[SkillCollision] %s | Profile=%s Obj=%s Enabled=%d | WorldStatic=%s WorldDynamic=%s Pawn=%s | Sweep=%d Updated=%s Radius=%.0f"),
		*GetName(),
		*CollisionSphere->GetCollisionProfileName().ToString(),
		*ChannelName(MyType),
		(int32)CollisionSphere->GetCollisionEnabled(),
		ResponseName(CollisionSphere->GetCollisionResponseToChannel(ECC_WorldStatic)),
		ResponseName(CollisionSphere->GetCollisionResponseToChannel(ECC_WorldDynamic)),
		ResponseName(CollisionSphere->GetCollisionResponseToChannel(ECC_Pawn)),
		ProjectileMovement ? (int32)ProjectileMovement->bSweepCollision : -1,
		*GetNameSafe(ProjectileMovement ? ProjectileMovement->UpdatedComponent.Get() : nullptr),
		CollisionSphere->GetScaledSphereRadius());

	// 2) 이동 스윕과 같은 조건(내 오브젝트 타입 + 내 응답)으로 아래 5000cm 검사 — "바닥이 날 막아주는가"
	const FVector Start = CollisionSphere->GetComponentLocation();
	const FVector End = Start - FVector(0.0f, 0.0f, 5000.0f);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SkillCollisionDebug), false, this);
	const FCollisionResponseParams ResponseParams(CollisionSphere->GetCollisionResponseToChannels());

	FHitResult Hit;
	const bool bBlocked = GetWorld()->SweepSingleByChannel(
		Hit, Start, End, FQuat::Identity, MyType,
		FCollisionShape::MakeSphere(CollisionSphere->GetScaledSphereRadius()),
		QueryParams, ResponseParams);

	if (bBlocked)
	{
		DrawDebugLine(GetWorld(), Start, Hit.ImpactPoint, FColor::Green, false, 3.0f, 0, 1.5f);
		UE_LOG(LogTemp, Warning, TEXT("[SkillCollision]   아래 차단 대상: %s / %s (Obj=%s)"),
			*GetNameSafe(Hit.GetActor()), *GetNameSafe(Hit.GetComponent()),
			Hit.GetComponent() ? *ChannelName(Hit.GetComponent()->GetCollisionObjectType()) : TEXT("None"));
	}
	else
	{
		DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 3.0f, 0, 1.5f);
		UE_LOG(LogTemp, Warning, TEXT("[SkillCollision]   아래 5000cm 안에 나를 막는 것 없음"));
	}

	// 3) 응답과 상관없이 아래에 실제로 무엇이 있는지 — 바닥 쪽 설정 확인용
	FHitResult AnyHit;
	const FCollisionObjectQueryParams AllObjects(FCollisionObjectQueryParams::AllObjects);
	if (GetWorld()->LineTraceSingleByObjectType(AnyHit, Start, End, AllObjects, QueryParams) && AnyHit.GetComponent())
	{
		const UPrimitiveComponent* Below = AnyHit.GetComponent();
		UE_LOG(LogTemp, Warning,
			TEXT("[SkillCollision]   아래 첫 물체: %s / %s | Profile=%s Obj=%s Enabled=%d | 나(Obj=%s)에 대한 응답=%s"),
			*GetNameSafe(AnyHit.GetActor()), *GetNameSafe(Below),
			*Below->GetCollisionProfileName().ToString(),
			*ChannelName(Below->GetCollisionObjectType()),
			(int32)Below->GetCollisionEnabled(),
			*ChannelName(MyType),
			ResponseName(Below->GetCollisionResponseToChannel(MyType)));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[SkillCollision]   아래에 콜리전을 가진 물체 자체가 없음 (바닥 NoCollision 또는 메시 콜리전 형상 없음)"));
	}
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
