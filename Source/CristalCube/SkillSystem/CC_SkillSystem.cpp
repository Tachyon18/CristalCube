// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_SkillSystem.h"
#include "../CC_LogHelper.h"
#include "CC_SkillEffector.h"
#include "../WeaponSystems/CC_Projectile.h"
#include "../Characters/CC_Character.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DamageEvents.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "DrawDebugHelpers.h"


// Sets default values for this component's properties
UCC_SkillSystem::UCC_SkillSystem()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;  // 필요할 때만 Tick

	// ...
}


// Called when the game starts
void UCC_SkillSystem::BeginPlay()
{
	Super::BeginPlay();

	// ...
	UE_LOG(LogTemp, Log, TEXT("SkillSystem initialized for %s"), *GetOwner()->GetName());
}


// Called every frame
void UCC_SkillSystem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

//==============================================================================
// MAIN INTERFACE
//==============================================================================

void UCC_SkillSystem::ExecuteSkill(const FSkillDefinition& Skill, FVector TargetLocation)
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		UE_LOG(LogTemp, Error, TEXT("ExecuteSkill: No owner!"));
		return;
	}

	// 컨텍스트 초기화
	FSkillExecutionContext Context;
	Context.Caster = Owner;
	Context.StartLocation = Owner->GetActorLocation();
	Context.TargetLocation = TargetLocation;
	Context.Direction = (TargetLocation - Context.StartLocation).GetSafeNormal();
	Context.CurrentDamage = Skill.BaseDamage * Skill.Passives.DamageMultiplier;

	UE_LOG(LogTemp, Log, TEXT("Executing Skill : %s, Core : %d"), *Skill.SkillID.ToString(),
		(int32)Skill.CoreType);

	//// Cast VFX
	//if (Skill.CastEffect)
	//{
	//	SpawnEffect(Skill.CastEffect, Context.StartLocation);
	//}

	// Cast Sound
	//if (Skill.CastSound)
	//{
	//	PlaySound(Skill.CastSound, Context.StartLocation);
	//}

	// Core 실행
	switch (Skill.CoreType)
	{
	case ESkillCoreType::Projectile:
		ExecuteProjectile(Skill, Context);
		break;

	case ESkillCoreType::Instant:
		ExecuteInstant(Skill, Context);
		break;

	case ESkillCoreType::Area:
		ExecuteArea(Skill, Context);
		break;

	case ESkillCoreType::Beam:
		ExecuteBeam(Skill, Context);
		break;

	default:
		UE_LOG(LogTemp, Warning, TEXT("ExecuteSkill: Unknown Core Type: %d"), (int32)Skill.CoreType);
		break;
	}
}

void UCC_SkillSystem::ExecuteSkillOnTarget(const FSkillDefinition& Skill, AActor* TargetActor)
{
	if (!TargetActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("ExecuteSkillOnTarget: No target!"));
		return;
	}

	ExecuteSkill(Skill, TargetActor->GetActorLocation());
}

//==============================================================================
// CORE EXECUTION - STUBS (다음 단계에서 구현)
//==============================================================================

void UCC_SkillSystem::ExecuteProjectile(const FSkillDefinition& Skill, FSkillExecutionContext& Context)
{
	if(!SkillEffectorClass)
	{
		UE_LOG(LogTemp, Error, TEXT("ExecuteProjectile: SkillEffector not set!"));
		return;
	}

	UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	int32 ProjectileCount = GetProjectileCount(Skill);

	for (int32 i = 0; i < ProjectileCount; ++i)
	{
		// 2-1. 스폰 위치 (약간 앞쪽)
		FVector SpawnLocation = Context.StartLocation + Context.Direction * 50.0f;

		// 2-2. 스폰 방향 (여러 발사 시 각도 분산)
		FVector SpawnDirection = Context.Direction;
		if (ProjectileCount > 1)
		{
			// MultiShot: 부채꼴 형태로 분산 (중앙 기준 ±30도)
			float SpreadAngle = Skill.Passives.MultiShotData.SpreadAngle;
			float AngleStep = SpreadAngle / (ProjectileCount - 1); // 총 60도 범위
			float Angle = -(SpreadAngle * 0.5f) + (i * AngleStep);

			// 방향 벡터 회전
			SpawnDirection = Context.Direction.RotateAngleAxis(Angle, FVector::UpVector);
		}

		// 2-3. 투사체 스폰
		FTransform SpawnTransform;
		SpawnTransform.SetLocation(SpawnLocation);
		SpawnTransform.SetRotation(SpawnDirection.ToOrientationQuat());

		ACC_SkillEffector* SkillEffectorProjectile = World->SpawnActor<ACC_SkillEffector>(
			SkillEffectorClass,
			SpawnTransform
		);

		if (SkillEffectorProjectile)
		{
			// 3. 투사체 초기화
			SkillEffectorProjectile->SetSkillOwner(Context.Caster);
			SkillEffectorProjectile->SkillContext = Context;
			SkillEffectorProjectile->Initialize(Skill.CoreType, Skill);

			SkillEffectorProjectile->OnEffectorHit.AddDynamic(this, &UCC_SkillSystem::OnProjectileHit);

			//// 4. Penetrate Addon 설정
			//if (Skill.Addons.Contains(ESkillAddonType::Penetrate))
			//{
			//	SkillEffectorProjectile->bCanPierce = true;
			//	SkillEffectorProjectile->PierceCount = Skill.Passives.PierceCount;
			//}

			//SkillEffectorProjectile->SetSkillData(this, Skill, Context);

			// 5. 디버그 표시
			if (bShowDebugShapes)
			{
				DrawDebugLine(
					World,
					SpawnLocation,
					SpawnLocation + SpawnDirection * 1000.0f,
					FColor::Red,
					false,
					DebugDrawDuration,
					0,
					2.0f
				);
			}

			UE_LOG(LogTemp, Log, TEXT("Spawned SkillEffect %d/%d - Damage: %.1f"),
				i + 1, ProjectileCount, Context.CurrentDamage);
		}
	}
	
}

void UCC_SkillSystem::ExecuteInstant(const FSkillDefinition& Skill, FSkillExecutionContext& Context)
{
	AActor* Target = FindNearestEnemy(Context.StartLocation, Skill.Range, Context.HitActors);

	if (!Target)
	{
		UE_LOG(LogTemp, Log, TEXT("ExecuteInstant [%s]: No target found within range %.0f"),
			*Skill.SkillID.ToString(), Skill.Range);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("ExecuteInstant [%s]: Targeting %s"),
		*Skill.SkillID.ToString(), *Target->GetName());

	ApplyDamage(Target, Context.CurrentDamage, Context.Caster);
	Context.HitActors.Add(Target);

	FVector HitLocation = Target->GetActorLocation(); // 간단히 타겟 위치로 히트 위치 설정 (향후 개선 가능)

	if(Skill.HitEffect)
	{
		SpawnEffect(Skill.HitEffect, HitLocation);
	}

	if (bShowDebugShapes)
	{
		DrawDebugLine(
			GetWorld(),
			Context.StartLocation,
			HitLocation,
			FColor::Yellow,
			false,
			DebugDrawDuration,
			0,
			2.0f
		);
		DrawDebugSphere(
			GetWorld(),
			HitLocation,
			30.0f,
			12,
			FColor::Yellow,
			false,
			DebugDrawDuration
		);
	}

	FHitResult InstantHit;
	InstantHit.ImpactPoint = HitLocation;
	InstantHit.HitObjectHandle = FActorInstanceHandle(Target);

	ProcessAddons(Skill, Context, InstantHit);

	UE_LOG(LogTemp, Log, TEXT("ExecuteInstant [%s]: Hit %s for %.1f damage"),
		*Skill.SkillID.ToString(), *Target->GetName(), Context.CurrentDamage);
}

void UCC_SkillSystem::ExecuteArea(const FSkillDefinition& Skill, FSkillExecutionContext& Context)
{

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	//==========================================================================
	// 1. 범위 내 적 탐색 (Skill.Range를 Area 반경으로 사용)
	//==========================================================================
	TArray<AActor*> EnemiesInRange = FindEnemiesInRadius(Context.StartLocation, Skill.Range);

	UE_LOG(LogTemp, Log, TEXT("ExecuteArea [%s]: %d enemies in range %.0f"),
		*Skill.SkillID.ToString(), EnemiesInRange.Num(), Skill.Range);

	//==========================================================================
	// 2. 범위 내 모든 적에게 데미지
	//==========================================================================
	for (AActor* Enemy : EnemiesInRange)
	{
		if (!IsValid(Enemy) || Context.HitActors.Contains(Enemy))
		{
			continue;
		}

		ApplyDamage(Enemy, Context.CurrentDamage, Context.Caster);
		Context.HitActors.Add(Enemy);

		// 개별 Hit VFX
		if (Skill.HitEffect)
		{
			SpawnEffect(Skill.HitEffect, Enemy->GetActorLocation());
		}

		// 개별 Addon 처리 (Area도 Chain/Explosion 조합 가능)
		FHitResult Hit;
		Hit.HitObjectHandle = FActorInstanceHandle(Enemy);
		Hit.ImpactPoint = Enemy->GetActorLocation();
		ProcessAddons(Skill, Context, Hit);

		UE_LOG(LogTemp, Log, TEXT("ExecuteArea: Hit %s for %.1f dmg"),
			*Enemy->GetName(), Context.CurrentDamage);
	}

	//==========================================================================
	// 3. Area Cast VFX (시전자 위치 기준)
	//==========================================================================
	if (Skill.CastEffect)
	{
		SpawnEffect(Skill.CastEffect, Context.StartLocation);
	}

	//==========================================================================
	// 4. 디버그
	//==========================================================================
	if (bShowDebugShapes)
	{
		DrawDebugSphere(
			World,
			Context.StartLocation,
			Skill.Range,
			24,
			FColor::Green,
			false,
			DebugDrawDuration,
			0,
			3.0f
		);
	}
}

void UCC_SkillSystem::ExecuteBeam(const FSkillDefinition& Skill, FSkillExecutionContext& Context)
{
	UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	FVector Start = Context.StartLocation;
	FVector End = Context.TargetLocation;

	FVector Direction = (End - Start).GetSafeNormal();
	float Distance = FVector::Dist(Start, End);
	if (Distance > Skill.Range)
	{
		End = Start + Direction * Skill.Range;
	}

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Context.Caster);
	QueryParams.bTraceComplex = false;

	TArray<FHitResult> HitResults;
	bool bHit = World->LineTraceMultiByChannel(
		HitResults,
		Start,
		End,
		ECC_Pawn,
		QueryParams
	);

	TArray<AActor*> DamagedActors;
	for (const FHitResult& Hit : HitResults)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || DamagedActors.Contains(HitActor))
		{
			continue;
		}

		if (HitActor->ActorHasTag(EnemyTag))
		{
			// 데미지 적용
			ApplyDamage(HitActor, Context.CurrentDamage, Context.Caster);
			DamagedActors.Add(HitActor);

			// Hit VFX
			if (Skill.HitEffect)
			{
				SpawnEffect(Skill.HitEffect, Hit.ImpactPoint);
			}

			// Addon 처리 (Explosion, Chain 등)
			ProcessAddons(Skill, Context, Hit);

			UE_LOG(LogTemp, Log, TEXT("Beam hit: %s"), *HitActor->GetName());
		}
	}

	// 4. Beam VFX 스폰 (Niagara Beam)
	if (Skill.CastEffect)
	{
		UNiagaraComponent* BeamEffect = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World,
			Skill.CastEffect,
			Start,
			FRotator::ZeroRotator,
			FVector(1.0f),
			true,
			true,
			ENCPoolMethod::AutoRelease
		);

		if (BeamEffect)
		{
			// Beam 끝점 설정 (Niagara Parameter)
			BeamEffect->SetVectorParameter(FName("BeamEnd"), End);
			BeamEffect->SetFloatParameter(FName("BeamWidth"), 10.0f);
		}
	}

	// 5. 디버그 표시
	if (bShowDebugShapes)
	{
		DrawDebugLine(
			World,
			Start,
			End,
			FColor::Cyan,
			false,
			DebugDrawDuration,
			0,
			5.0f
		);

		for (const FHitResult& Hit : HitResults)
		{
			DrawDebugSphere(
				World,
				Hit.ImpactPoint,
				20.0f,
				12,
				FColor::Yellow,
				false,
				DebugDrawDuration
			);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Beam fired: %d enemies hit"), DamagedActors.Num());

}

//==============================================================================
// ADDON PROCESSING - STUBS
//==============================================================================

void UCC_SkillSystem::ProcessAddons(const FSkillDefinition& Skill, FSkillExecutionContext& Context, const FHitResult& Hit)
{
	AActor* HitTarget = Hit.GetActor();
	FVector HitLocation = Hit.ImpactPoint;

	if (HitTarget && !IsValid(HitTarget))
	{
		HitTarget = nullptr;
	}

	for (ESkillAddonType Addon : Skill.Addons)
	{
		switch (Addon)
		{
		case ESkillAddonType::Explosion:
			ApplyExplosion(Skill, Context, HitLocation);
			break;

		case ESkillAddonType::Chain:
			if (HitTarget)
			{
				ApplyChain(Skill, Context, HitTarget);
			}
			break;

			// Penetrate / MultiShot은 SkillEffector / ExecuteProjectile에서 처리
		case ESkillAddonType::Penetrate:
		case ESkillAddonType::MultiShot:
		default:
			break;
		}
	}
}

void UCC_SkillSystem::ApplyExplosion(const FSkillDefinition& Skill, FSkillExecutionContext& Context, FVector Location)
{
	const FExplosionAddonData& ExplosionData = Skill.Passives.ExplosionData;

	TArray<AActor*> EnemiesInRange = FindEnemiesInRadius(Location, ExplosionData.Radius);

	UE_LOG(LogTemp, Log, TEXT("ApplyExplosion [%s]: %d enemies in radius %.0f"),
		*Skill.SkillID.ToString(), EnemiesInRange.Num(), ExplosionData.Radius);

	for (AActor* Enemy : EnemiesInRange)
	{
		if (!Enemy || Context.HitActors.Contains(Enemy))
		{
			continue;  // 이미 맞은 적 스킵
		}

		float Distance = FVector::Dist(Location, Enemy->GetActorLocation());
		float DistanceRatio = FMath::Clamp(Distance / ExplosionData.Radius, 0.0f, 1.0f);

		// 중심: 100%, 외곽: 50% ? 선형 감소 적용 (거리 별 데미지 비율은 추후 전용 데이터 사용)
		float DamageMultiplier = FMath::Lerp(1.0f, ExplosionData.MinDamageRatio, DistanceRatio);
		float ExplosionDamage = Context.CurrentDamage * DamageMultiplier;

		ApplyDamage(Enemy, ExplosionDamage, Context.Caster);
		Context.HitActors.Add(Enemy);

		UE_LOG(LogTemp, Log, TEXT("ApplyExplosion: Hit %s ? %.0fcm → %.1f dmg (x%.2f)"),
			*Enemy->GetName(), Distance, ExplosionDamage, DamageMultiplier);
	}

	if (Skill.ExplosionEffect)
	{
		SpawnEffect(Skill.ExplosionEffect, Location);
	}

	// 디버그 구체
	if (bShowDebugShapes)
	{
		DrawDebugSphere(
			GetWorld(),
			Location,
			ExplosionData.Radius,
			24,
			FColor::Orange,
			false,
			DebugDrawDuration,
			0,
			3.0f
		);
	}

}

void UCC_SkillSystem::ApplyChain(const FSkillDefinition& Skill, FSkillExecutionContext& Context, AActor* HitTarget)
{
	//
	const FChainAddonData& ChainData = Skill.Passives.ChainData;

	if(Context.CurrentChainCount >= ChainData.ChainCount)
	{
		UE_LOG(LogTemp, Log, TEXT("ApplyChain [%s]: Chain limit reached (%d/%d)"),
			*Skill.SkillID.ToString(), Context.CurrentChainCount, ChainData.ChainCount);
		return;
	}

	FVector SearchOrigin = HitTarget ? HitTarget->GetActorLocation() : Context.StartLocation;

	AActor* NextTarget = FindNearestEnemy(SearchOrigin, ChainData.SearchRadius, Context.HitActors);

	if (!NextTarget)
	{
		UE_LOG(LogTemp, Log, TEXT("ApplyChain [%s]: No next target for chain %d"),
			*Skill.SkillID.ToString(), Context.CurrentChainCount + 1);
		return;
	}


	Context.CurrentChainCount++;
	Context.HitActors.Add(NextTarget);

	ApplyDamage(NextTarget, Context.CurrentDamage, Context.Caster);

	FVector NextHitLocation = NextTarget->GetActorLocation();

	UE_LOG(LogTemp, Log, TEXT("ApplyChain [%s]: Chain %d → %s (%.1f dmg)"),
		*Skill.SkillID.ToString(), Context.CurrentChainCount,
		*NextTarget->GetName(), Context.CurrentDamage);



	if (Skill.HitEffect)
	{
		SpawnEffect(Skill.HitEffect, NextHitLocation);
	}

	// 디버그
	if (bShowDebugShapes && HitTarget)
	{
		DrawDebugLine(
			GetWorld(),
			SearchOrigin,
			NextHitLocation,
			FColor::Cyan,
			false,
			DebugDrawDuration,
			0,
			2.0f
		);
		DrawDebugSphere(
			GetWorld(),
			NextHitLocation,
			25.0f,
			12,
			FColor::Cyan,
			false,
			DebugDrawDuration
		);
	}

	FHitResult ChainHit;
	ChainHit.ImpactPoint = NextHitLocation;
	ChainHit.HitObjectHandle = FActorInstanceHandle(NextTarget);

	ProcessAddons(Skill, Context, ChainHit);
}

bool UCC_SkillSystem::CanPenetrate(const FSkillDefinition& Skill, FSkillExecutionContext& Context) const
{
	if (!Skill.Addons.Contains(ESkillAddonType::Penetrate))
	{
		return false;
	}

	return Context.CurrentPierceCount < Skill.Passives.PierceData.PierceCount;
}

int32 UCC_SkillSystem::GetProjectileCount(const FSkillDefinition& Skill) const
{
	int32 Count = Skill.Passives.ProjectileCount;

	if (Skill.Addons.Contains(ESkillAddonType::MultiShot))
	{
		Count += Skill.Passives.MultiShotData.AdditionalCount;  // AdditionalCount만큼 발사 수 증가
	}

	return FMath::Max(1, Count);
}

//==============================================================================
// UTILITY FUNCTIONS
//==============================================================================

void UCC_SkillSystem::OnProjectileHit(ACC_SkillEffector* Effector, AActor* HitActor)
{
	if (!Effector || !IsValid(Effector) || !IsValid(HitActor))
	{
		return;
	}

	FSkillExecutionContext& Context = Effector->SkillContext;
	const FSkillDefinition& Skill = Effector->SkillDef;

	ApplyDamage(HitActor, Context.CurrentDamage, Context.Caster);
	Context.HitActors.Add(HitActor);
	
	if (Skill.HitEffect)
	{
		SpawnEffect(Skill.HitEffect, HitActor->GetActorLocation());
	}

	FHitResult Hit;
	Hit.HitObjectHandle = FActorInstanceHandle(HitActor);
	Hit.ImpactPoint = HitActor->GetActorLocation();
	ProcessAddons(Skill, Context, Hit);

	bool bHasPenetrate = Skill.Addons.Contains(ESkillAddonType::Penetrate);

	if (bHasPenetrate)
	{
		Context.CurrentPierceCount++;
		UE_LOG(LogTemp, Log, TEXT("[SkillSystem] Penetrate %d/%d ? continuing"),
			Context.CurrentPierceCount, Skill.Passives.PierceData.PierceCount);

		if (!CanPenetrate(Skill, Context))
		{
			Effector->Destroy();
		}
	}
	else
	{
		Effector->Destroy();
	}

}

AActor* UCC_SkillSystem::FindNearestEnemy(FVector Origin, float Radius, const TArray<AActor*>& ExcludeActors) const
{
	TArray<AActor*> FoundEnemies;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), EnemyTag, FoundEnemies);

	AActor* NearestEnemy = nullptr;
	float NearestDistance = Radius;

	for (AActor* Enemy : FoundEnemies)
	{
		if (!Enemy || !IsValid(Enemy) || ExcludeActors.Contains(Enemy))
		{
			continue;
		}

		float Distance = FVector::Dist(Origin, Enemy->GetActorLocation());
		if (Distance < NearestDistance)
		{
			NearestDistance = Distance;
			NearestEnemy = Enemy;
		}
	}

	return NearestEnemy;
}

TArray<AActor*> UCC_SkillSystem::FindEnemiesInRadius(FVector Origin, float Radius) const
{
	TArray<AActor*> FoundEnemies;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), EnemyTag, FoundEnemies);

	TArray<AActor*> EnemiesInRadius;
	for (AActor* Enemy : FoundEnemies)
	{
		if (!Enemy || !IsValid(Enemy))
		{
			continue;
		}

		float Distance = FVector::Dist(Origin, Enemy->GetActorLocation());
		if (Distance <= Radius)
		{
			EnemiesInRadius.Add(Enemy);
		}
	}

	return EnemiesInRadius;
}

void UCC_SkillSystem::ApplyDamage(AActor* Target, float Damage, AActor* DamageCauser)
{
	if (!Target || Damage <= 0.0f)
	{
		return;
	}

	// Character 타입이면 직접 피해 적용
	if (ACC_Character* Character = Cast<ACC_Character>(Target))
	{
		Character->TakeDamage(Damage, FDamageEvent(), nullptr, DamageCauser);
		UE_LOG(LogTemp, Log, TEXT("Applied %.1f damage to %s"), Damage, *Target->GetName());
	}
}

void UCC_SkillSystem::SpawnEffect(UNiagaraSystem* Effect, FVector Location, FRotator Rotation)
{
	if (!Effect || !GetWorld())
	{
		return;
	}

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		GetWorld(),
		Effect,
		Location,
		Rotation,
		FVector(1.0f),
		true,  // Auto Destroy
		true,  // Auto Activate
		ENCPoolMethod::AutoRelease  // 풀링!
	);
}

void UCC_SkillSystem::PlaySound(USoundBase* Sound, FVector Location)
{
	if (!Sound || !GetWorld())
	{
		return;
	}

	UGameplayStatics::PlaySoundAtLocation(GetWorld(), Sound, Location);
}
