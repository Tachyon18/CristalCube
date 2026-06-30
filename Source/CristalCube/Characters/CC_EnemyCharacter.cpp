// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_EnemyCharacter.h"
#include "Engine/DamageEvents.h"
#include "Engine/OverlapResult.h"
#include "Kismet/GameplayStatics.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "TimerManager.h"
#include "CC_PlayerCharacter.h"
#include "../CC_LogHelper.h"
#include "../CC_EnemyManager.h"
#include "../CC_AIManager.h"
#include "../CC_EnemyAIController.h"
#include "../CC_CubeWorldManager.h"
#include "../Gameplay/CC_ExperienceGem.h"
#include "../Gameplay/CC_Cube.h"


ACC_EnemyCharacter::ACC_EnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;


	// Enemy defaults
	MaxHealth = 50.0f;
	CurrentHealth = MaxHealth;
	MoveSpeed = 300.0f;  // Slower than player

	if(UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Overlap);
		Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		Capsule->SetGenerateOverlapEvents(true);
	}

	// ─── RVO Avoidance: Enemy 간 소프트 분산 ──────────────────────────────
	if (UCharacterMovementComponent* CMC = GetCharacterMovement())
	{
		CMC->bUseRVOAvoidance = true;
		CMC->AvoidanceWeight = 0.5f;   // 0=회피 무시 / 1=완전 회피
		CMC->AvoidanceConsiderationRadius =
			GetCapsuleComponent()->GetScaledCapsuleRadius() * 3.0f;
	}

	AttackHitData.HitType = EAttackHitType::Line;
	AttackHitData.Range = 200.0f;
	AttackHitData.bPenetrate = false;

	// AI settings
	DetectionRange = 2000.0f;  // 20 meters
	TargetPlayer = nullptr;

	// Reward settings
	ExperienceDrop = 10.0f;    // Give 10 XP when killed

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// Enemy type
	EnemyType = TEXT("Basic");
	bIsBoss = false;
}

void ACC_EnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (!GetController())
	{
		CC_LOG_ENEMY(Warning, TEXT("No controller! Spawning AI Controller..."));
	}

	// Add "Enemy" tag for weapon auto-aim
	Tags.AddUnique(FName("Enemy"));

	// Find player
	FindPlayer();

	// ─── 목표 오프셋 초기화 (플레이어 주변 원형 배치) ────────────────────────
	if (TargetOffsetRadius > 0.f)
	{
		const float Angle = FMath::FRandRange(0.f, 360.f);
		const float Radius = FMath::FRandRange(TargetOffsetRadius * 0.5f, TargetOffsetRadius);
		TargetOffset = FVector(
			FMath::Cos(FMath::DegreesToRadians(Angle)) * Radius,
			FMath::Sin(FMath::DegreesToRadians(Angle)) * Radius,
			0.f
		);
		UE_LOG(LogTemp, VeryVerbose,
			TEXT("[Enemy %s] TargetOffset = (%.0f, %.0f)  Radius=%.0f"),
			*GetName(), TargetOffset.X, TargetOffset.Y, Radius);
	}

	if (USkeletalMeshComponent* SkeletalMesh = GetMesh())
	{
		if (UAnimInstance* AnimInstance = SkeletalMesh->GetAnimInstance())
		{
			AnimInstance->OnMontageEnded.AddDynamic(
				this, &ACC_EnemyCharacter::OnAttackMontageEnded
			);


			UE_LOG(LogTemp, Warning, TEXT("[ENEMY] %s - Montage ended event bound"),
				*GetName());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[ENEMY] %s - No AnimInstance found!"),
				*GetName());
		}

		SkeletalMesh->bEnableUpdateRateOptimizations = true;
		SkeletalMesh->VisibilityBasedAnimTickOption =
			EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;

		// URO 거리별 프레임 스킵 명시 (BP 설정 덮어쓰기 방지)
		if (SkeletalMesh->AnimUpdateRateParams)
		{
			SkeletalMesh->AnimUpdateRateParams->bShouldUseLodMap = true;
		}
	}

	GetWorldTimerManager().SetTimerForNextTick(this,
		&ACC_EnemyCharacter::RegisterToManagers);
}

void ACC_EnemyCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Unregister from AI Manager
	if (UCC_AIManager* AIManager = UCC_AIManager::Get(this))
	{
		AIManager->UnregisterEnemy(this);
	}

	if (ACC_EnemyManager* Manager = ACC_EnemyManager::Get(this))
	{
		Manager->UnregisterEnemy(this);
	}

	Super::EndPlay(EndPlayReason);
}


void ACC_EnemyCharacter::ReportActualDeathToEnemyManager()
{
	if (bReportedActualDeath)
	{
		return;
	}

	bReportedActualDeath = true;

	if (ACC_EnemyManager* Manager = ACC_EnemyManager::Get(this))
	{
		Manager->ReportEnemyKilled(this);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[ENEMY] Failed to report actual death for %s - EnemyManager missing"),
			*GetName());
	}
}

void ACC_EnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Chase player if enabled and alive
	if (!IsAlive()|| EnemyState == EEnemyState::Attacking)
	{
		return;	
	}

	if (!bMovementEnabled)
	{
		return;
	}

	// AILogicInterval마다 MoveTarget 갱신
	AILogicTimer += DeltaTime;
	if (AILogicTimer >= AILogicInterval)
	{
		AILogicTimer = 0.f;
		UpdateMoveTarget();
	}
	
	PerformMove(DeltaTime);
}

void ACC_EnemyCharacter::PerformMove(float DeltaTime)
{
	switch (MovementBehavior)
	{
	case EMovementBehavior::Direct:
		PerformMove_Direct(DeltaTime);
		break;

	case EMovementBehavior::Step:
		// Phase 3 구현 예정
		PerformMove_Step(DeltaTime);
		break;

	case EMovementBehavior::Teleport:
		// Phase 3 구현 예정
		PerformMove_Teleport(DeltaTime);
		break;

	case EMovementBehavior::Waypoint:
		// 후순위 구현 예정
		PerformMove_Direct(DeltaTime);
		break;
	}
}

void ACC_EnemyCharacter::PerformMove_Direct(float DeltaTime)
{
	if (CachedMoveDirection.IsNearlyZero()) return;

	AddMovementInput(CachedMoveDirection, 1.0f);

	FRotator LookAt = CachedMoveDirection.Rotation();
	LookAt.Pitch = 0.f;
	LookAt.Roll = 0.f;
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), LookAt, DeltaTime, 5.0f));
}

void ACC_EnemyCharacter::PerformMove_Step(float DeltaTime)
{
	const FVector CurrentLoc = GetActorLocation();

	switch (StepPhase)
	{
	case EStepPhase::Moving:
	{
		if (StepTarget.IsZero())
			StepTarget = ComputeNextStepTarget();

		const FVector ToTarget = StepTarget - CurrentLoc;
		const float DistSq = ToTarget.SizeSquared2D();

		if (DistSq <= StepArrivalThreshold * StepArrivalThreshold)
		{
			StepPhase = EStepPhase::Waiting;
			StepWaitElapsed = 0.f;
			break;
		}

		const FVector Direction = ToTarget.GetSafeNormal2D();
		AddMovementInput(Direction, 1.0f);

		FRotator LookAt = Direction.Rotation();
		LookAt.Pitch = 0.f;
		LookAt.Roll = 0.f;
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), LookAt, DeltaTime, 5.0f));
		break;
	}

	case EStepPhase::Waiting:
	{
		StepWaitElapsed += DeltaTime;
		if (StepWaitElapsed >= StepWaitDuration)
		{
			StepTarget = FVector::ZeroVector;
			StepPhase = EStepPhase::Moving;
		}
		break;
	}
	}
}

void ACC_EnemyCharacter::PerformMove_Teleport(float DeltaTime)
{
	TeleportElapsed += DeltaTime;
	if (TeleportElapsed < TeleportInterval) return;

	TeleportElapsed = 0.f;

	if (MoveTarget.IsZero()) return;

	const float Angle = FMath::FRandRange(0.f, 360.f);
	const float Radius = FMath::FRandRange(TeleportRadius * 0.5f, TeleportRadius);

	const FVector Destination = MoveTarget + FVector(
		FMath::Cos(FMath::DegreesToRadians(Angle)) * Radius,
		FMath::Sin(FMath::DegreesToRadians(Angle)) * Radius,
		0.f
	);

	const FVector PreviousLoc = GetActorLocation();

	SetActorLocation(Destination, false, nullptr, ETeleportType::TeleportPhysics);

	OnTeleportPerformed(PreviousLoc, GetActorLocation());
}

FVector ACC_EnemyCharacter::ComputeNextStepTarget()
{
	const FVector CurrentLoc = GetActorLocation();

	if (MoveTarget.IsZero())
		return CurrentLoc;

	const FVector BaseDirection = (MoveTarget - CurrentLoc).GetSafeNormal2D();
	if (BaseDirection.IsNearlyZero())
		return CurrentLoc;

	const float AngleOffset = FMath::FRandRange(-StepAngleVariance, StepAngleVariance);
	const FVector VariedDirection = BaseDirection.RotateAngleAxis(AngleOffset, FVector::UpVector);

	const float VariedDistance = StepDistance * FMath::FRandRange(
		1.f - StepDistanceVariance, 1.f + StepDistanceVariance);

	return CurrentLoc + VariedDirection * VariedDistance;
}

void ACC_EnemyCharacter::UpdateMoveTarget()
{
	if (!TargetPlayer) FindPlayer();
	if (!TargetPlayer) return;

	const float DistSq = FVector::DistSquared(GetActorLocation(), TargetPlayer->GetActorLocation());

	if (DistSq <= DetectionRange * DetectionRange)
	{
		// TargetOffset 포함한 목적지 설정
		MoveTarget = TargetPlayer->GetActorLocation() + TargetOffset;

		// 이동 방향 캐시
		CachedMoveDirection = (MoveTarget - GetActorLocation()).GetSafeNormal();

		// 공격 범위 체크 (AttackRangeSphere Overlap이 없을 경우 폴백)
		const float AttackRangeSq = EnemyStats.AttackRange * EnemyStats.AttackRange;
		if (DistSq <= AttackRangeSq && !bPlayerInRange)
		{
			bPlayerInRange = true;
			TryAttack(TargetPlayer);
		}
		else if (DistSq > AttackRangeSq && bPlayerInRange)
		{
			bPlayerInRange = false;
		}
	}
	else
	{
		MoveTarget = FVector::ZeroVector;
		CachedMoveDirection = FVector::ZeroVector;
	}
}

void ACC_EnemyCharacter::FindPlayer()
{
	// Find player character in the world
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	TargetPlayer = Cast<ACC_PlayerCharacter>(PlayerPawn);

	if (TargetPlayer)
	{
		UE_LOG(LogTemp, Log, TEXT("Enemy found player: %s"), *TargetPlayer->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Enemy could not find player"));
	}
}

bool ACC_EnemyCharacter::PerformAttackHit(const FAttackHitData& HitData, TArray<AActor*>& OutHitTargets)
{
	OutHitTargets.Empty();

	FVector Start = GetActorLocation();
	FVector Forward = GetActorForwardVector();

	switch (HitData.HitType)
	{
		case EAttackHitType::Point:
		{
			// 단일 타겟만
			if (!TargetPlayer) return false;

			float Distance = FVector::Dist(Start, TargetPlayer->GetActorLocation());
			if (Distance <= HitData.Range)
			{
				OutHitTargets.Add(TargetPlayer);
			}
			break;
		}

		case EAttackHitType::Sphere:
		{
			// 360도 원형 범위
			TArray<FOverlapResult> Overlaps;
			FCollisionQueryParams Params;
			Params.AddIgnoredActor(this);

			GetWorld()->OverlapMultiByChannel(
				Overlaps,
				Start,
				FQuat::Identity,
				ECC_Pawn,
				FCollisionShape::MakeSphere(HitData.Range),
				Params
			);

			for (const FOverlapResult& Overlap : Overlaps)
			{
				if (ACC_PlayerCharacter* Target = Cast<ACC_PlayerCharacter>(Overlap.GetActor()))
				{
					OutHitTargets.Add(Target);
					if (!HitData.bPenetrate) break;
				}
			}
			break;
		}

		case EAttackHitType::Line:
		{
			// 횡베기: 좌우로 넓고 전방으로 얇은 Box
			FVector HitStart = Start;
			FVector HitEnd = Start + (Forward * HitData.Range);
			FVector HitCenter = (HitStart + HitEnd) * 0.5f;

			// Box 크기: X=얇게, Y=넓게!
			FVector BoxExtent(
				HitData.Thickness * 0.5f,  // 전방 두께 (얇게)
				HitData.Width * 1.f,      // 좌우 폭 (넓게!)
				HitData.Height * 0.5f      // 상하 높이
			);

			TArray<FHitResult> HitResults;
			FCollisionQueryParams Params;
			Params.AddIgnoredActor(this);

			GetWorld()->SweepMultiByChannel(
				HitResults,
				HitStart,
				HitEnd,
				GetActorQuat(),
				ECC_Pawn,
				FCollisionShape::MakeBox(BoxExtent),
				Params
			);

			for (const FHitResult& Hit : HitResults)
			{
				if (ACC_PlayerCharacter* Target = Cast<ACC_PlayerCharacter>(Hit.GetActor()))
				{
					OutHitTargets.Add(Target);
					if (!HitData.bPenetrate) break;
				}
			}
			break;
		}

		case EAttackHitType::Box:
		{
			// 전방 사각형
			FVector HitStart = Start;
			FVector HitEnd = Start + (Forward * HitData.Range);

			FVector BoxExtent(
				HitData.Range * 0.5f,
				HitData.Width * 0.5f,
				HitData.Height * 0.5f
			);

			TArray<FHitResult> HitResults;
			FCollisionQueryParams Params;
			Params.AddIgnoredActor(this);

			GetWorld()->SweepMultiByChannel(
				HitResults,
				HitStart,
				HitEnd,
				GetActorQuat(),
				ECC_Pawn,
				FCollisionShape::MakeBox(BoxExtent),
				Params
			);

			for (const FHitResult& Hit : HitResults)
			{
				if (ACC_PlayerCharacter* Target = Cast<ACC_PlayerCharacter>(Hit.GetActor()))
				{
					OutHitTargets.Add(Target);
					if (!HitData.bPenetrate) break;
				}
			}
			break;
		}

		case EAttackHitType::Cone:
		{
			// 부채꼴
			TArray<FOverlapResult> Overlaps;
			FCollisionQueryParams Params;
			Params.AddIgnoredActor(this);

			// 일단 구체로 후보 찾기
			GetWorld()->OverlapMultiByChannel(
				Overlaps,
				Start,
				FQuat::Identity,
				ECC_Pawn,
				FCollisionShape::MakeSphere(HitData.Range),
				Params
			);

			// 각도 필터링
			for (const FOverlapResult& Overlap : Overlaps)
			{
				if (ACC_PlayerCharacter* Target = Cast<ACC_PlayerCharacter>(Overlap.GetActor()))
				{
					FVector ToTarget = (Target->GetActorLocation() - Start).GetSafeNormal();
					float Dot = FVector::DotProduct(Forward, ToTarget);
					float AngleRad = FMath::Acos(Dot);
					float AngleDeg = FMath::RadiansToDegrees(AngleRad);

					if (AngleDeg <= HitData.Angle / 2.0f)
					{
						OutHitTargets.Add(Target);
						if (!HitData.bPenetrate) break;
					}
				}
			}
			break;
		}

		case EAttackHitType::Capsule:
		{
			// 긴 원통
			FVector HitStart = Start;
			FVector HitEnd = Start + (Forward * HitData.Range);

			TArray<FHitResult> HitResults;
			FCollisionQueryParams Params;
			Params.AddIgnoredActor(this);

			GetWorld()->SweepMultiByChannel(
				HitResults,
				HitStart,
				HitEnd,
				FQuat::Identity,
				ECC_Pawn,
				FCollisionShape::MakeCapsule(HitData.Radius, HitData.Range * 0.5f),
				Params
			);

			for (const FHitResult& Hit : HitResults)
			{
				if (ACC_PlayerCharacter* Target = Cast<ACC_PlayerCharacter>(Hit.GetActor()))
				{
					OutHitTargets.Add(Target);
					if (!HitData.bPenetrate) break;
				}
			}
			break;
		}
	}

	// 디버그 시각화
	if (bShowAttackDebug)
	{
		DrawAttackDebug(HitData, OutHitTargets.Num() > 0);
	}

	return OutHitTargets.Num() > 0;
}

void ACC_EnemyCharacter::Die()
{

	// Drop experience for player
	//if (TargetPlayer && ExperienceDrop > 0.0f)
	//{
	//	TargetPlayer->AddExperience(ExperienceDrop);
	//	UE_LOG(LogTemp, Log, TEXT("Enemy dropped %.0f experience"), ExperienceDrop);
	//}

	// Drop experience to Gem
	if (ExpGemClass)
	{
		FVector BaseLocation = GetActorLocation();

		float RandomAngle = FMath::RandRange(0.0f, 360.0f);
		float RandomRadius = FMath::RandRange(50.0f, 150.0f);

		FVector Offset(
			FMath::Cos(FMath::DegreesToRadians(RandomAngle)) * RandomRadius,
			FMath::Sin(FMath::DegreesToRadians(RandomAngle)) * RandomRadius,
			50.f
		);

		FVector SpawnLocation = BaseLocation + Offset;
		FRotator SpawnRotation = FRotator::ZeroRotator;

		ACC_ExperienceGem* Gem = GetWorld()->SpawnActor<ACC_ExperienceGem>(
			ExpGemClass,
			SpawnLocation,
			SpawnRotation
		);

		if (Gem)
		{
			Gem->SetExpAmount(ExpGemAmount);

			// 현재 Active Cube에 등록 (Freeze와 함께 멈추도록)
			if (ACC_CubeWorldManager* CubeManager = ACC_CubeWorldManager::Get(this))
			{
				if (ACC_Cube* ActiveCubeRef = CubeManager->GetActiveCube())
				{
					ActiveCubeRef->RegisterActor(Gem);
					Gem->SetOwnerCube(ActiveCubeRef);
				}
			}

			CC_LOG_ENEMY(Log, TEXT("[Enemy] Spawned EXP Gem (%f EXP)"), ExpGemAmount);
		}
	}
	else
	{
		CC_LOG_ENEMY(Warning, TEXT("[Enemy] No ExpGemClass set!"));
	}

	if (UCC_AIManager* AIManager = UCC_AIManager::Get(this))
	{
		AIManager->UnregisterEnemy(this);
	}

	if (bPersistent)
	{
		if (ACC_CubeWorldManager* CubeManager = ACC_CubeWorldManager::Get(this))
			CubeManager->UnregisterPersistentEnemy(this);
	}

	ReportActualDeathToEnemyManager();

	// Call base class Die() to handle death animation, etc.
	Super::Die();

	// TODO: Spawn death effect, drop items, etc.

	// Destroy enemy after short delay
	SetLifeSpan(1.0f);
}

void ACC_EnemyCharacter::PerformAttack()
{
	EnemyState = EEnemyState::Attacking;
	bIsAttacking = true;
	bCanAttack = false;

	CC_LOG_ENEMY(Warning, TEXT("%s - Performing attack on player!"), *GetName());

	// Stop movement while attacking
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
	}

	// Ply attack animation if available
	if (AttackMontage && GetMesh())
	{
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			AnimInstance->Montage_Play(AttackMontage, 1.0f);

			//UE_LOG(LogTemp, Log, TEXT("[ENEMY] %s started attack animation"), *GetName());
		}
	}
	else
	{
		// if no animation, immediately deal damage and reset
		DealDamageToTarget();
		bIsAttacking = false;
		EnemyState = EEnemyState::Moving;
		StartAttackCooldown();
	}
}

void ACC_EnemyCharacter::StartAttackCooldown()
{
	if (!GetWorld())
	{
		UE_LOG(LogTemp, Error, TEXT("[ENEMY] Cannot start cooldown - no valid world"));
		return;
	}

	// Start cooldown timer
	GetWorld()->GetTimerManager().SetTimer(
		AttackCooldownTimer,
		this,
		&ACC_EnemyCharacter::ResetAttackCooldown,
		EnemyStats.AttackCooldown,
		false  // Loop = false (Only once)
	);

	//UE_LOG(LogTemp, Log, TEXT("[ENEMY] %s cooldown started (%.1fs)"), *GetName(), EnemyStats.AttackCooldown);
}

void ACC_EnemyCharacter::ResetAttackCooldown()
{
	bCanAttack = true;
	EnemyState = EEnemyState::Moving;

	//UE_LOG(LogTemp, Log, TEXT("[ENEMY] %s attack ready!"), *GetName());

	// if in range, try to attack again
	if (TargetPlayer)
	{
		float Distance = FVector::Dist(GetActorLocation(), TargetPlayer->GetActorLocation());
		if (Distance <= EnemyStats.AttackRange)
		{
			TryAttack(TargetPlayer);
		}
	}
}

void ACC_EnemyCharacter::DealDamageToTarget()
{
	// AnimNofity "AttackHit" from animation
	if (!IsAlive())
	{
		return;
	}

	TArray<AActor*> HitTargets;

	if (PerformAttackHit(AttackHitData, HitTargets))
	{
		for (AActor* Target : HitTargets)
		{
			if (HitActorsThisAttack.Contains(Target))
			{
				continue;
			}

			if (ACC_PlayerCharacter* Player = Cast<ACC_PlayerCharacter>(TargetPlayer))
			{
				if (Player->IsAlive())
				{
					Player->TakeDamage(EnemyStats.AttackDamage, FDamageEvent(), nullptr, this);

					UE_LOG(LogTemp, Warning, TEXT("[ENEMY] %s dealt %.1f damage to player"),
						*GetName(), EnemyStats.AttackDamage);
				}
			}
		}
	}

}

void ACC_EnemyCharacter::TryAttack(ACC_PlayerCharacter* Target)
{
	if (!CanAttack() || !Target)
	{
		return;
	}

	CC_LOG_ENEMY(Warning, TEXT("%s - Trying to attack player..."), *GetName());

	// Check range
	float Distance = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
	if (Distance > EnemyStats.AttackRange)
	{
		CC_LOG_ENEMY(Warning, TEXT("%s - Player out of range (%.1f > %.1f)"),
			*GetName(), Distance, EnemyStats.AttackRange);
		return;
	}

	// Try to attack
	TargetPlayer = Target;
	HitActorsThisAttack.Empty();

	PerformAttack();
}

float ACC_EnemyCharacter::GetAttackCooldownPercent() const
{
	if (bCanAttack)
	{
		return 1.0f;
	}

	if (!GetWorld())
	{
		return 0.0f;
	}

	float RemainingTime = GetWorld()->GetTimerManager().GetTimerRemaining(AttackCooldownTimer);

	if (RemainingTime <= 0.0f)
	{
		return 1.0f;
	}

	return 1.0f - (RemainingTime / EnemyStats.AttackCooldown);
}

void ACC_EnemyCharacter::PlayAttackAnimation()
{
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_Play(AttackMontage, 1.0f);
	}
}

void ACC_EnemyCharacter::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage == AttackMontage)
	{
		bIsAttacking = false;
		EnemyState = EEnemyState::Moving;   // 애니메이션 종료 → 이동 재개

		// Start cooldown Timer
		GetWorld()->GetTimerManager().SetTimer(
			AttackCooldownTimer,
			this,
			&ACC_EnemyCharacter::ResetAttackCooldown,
			EnemyStats.AttackCooldown,
			false
		);

		UE_LOG(LogTemp, Log, TEXT("[ENEMY] %s attack ended, cooldown started (%.1fs)"),
			*GetName(), EnemyStats.AttackCooldown);
	}

}

void ACC_EnemyCharacter::DrawAttackDebug(const FAttackHitData& HitData, bool bHit)
{
	if (!GetWorld()) return;

	CC_LOG_ENEMY(Log, TEXT("Drawing attack debug for %s"), *GetName());

	FColor Color = bHit ? FColor::Green : FColor::Red;
	FVector Start = GetActorLocation();
	FVector Forward = GetActorForwardVector();
	float Duration = 1.0f;

	switch (HitData.HitType)
	{
	case EAttackHitType::Point:
	{
		if (TargetPlayer)
		{
			DrawDebugLine(GetWorld(), Start, TargetPlayer->GetActorLocation(),
				Color, false, Duration, 0, 3.0f);
		}
		break;
	}

	case EAttackHitType::Sphere:
	{
		DrawDebugSphere(GetWorld(), Start, HitData.Range, 12, Color, false, Duration);
		break;
	}

	case EAttackHitType::Line:
	{
		FVector Center = Start + (Forward * HitData.Range * 0.5f);
		FVector Extent(HitData.Thickness * 0.5f, HitData.Width * 1.f, HitData.Height * 0.5f);
		DrawDebugBox(GetWorld(), Center, Extent, GetActorQuat(), Color, false, Duration, 0, 3.0f);
		break;
	}

	case EAttackHitType::Box:
	{
		FVector Center = Start + (Forward * HitData.Range * 0.5f);
		FVector Extent(HitData.Range * 0.5f, HitData.Width * 0.5f, HitData.Height * 0.5f);
		DrawDebugBox(GetWorld(), Center, Extent, GetActorQuat(), Color, false, Duration, 0, 3.0f);
		break;
	}

	case EAttackHitType::Cone:
	{
		DrawDebugCone(GetWorld(), Start, Forward, HitData.Range,
			FMath::DegreesToRadians(HitData.Angle / 2.0f),
			FMath::DegreesToRadians(HitData.Angle / 2.0f),
			12, Color, false, Duration);
		break;
	}

	case EAttackHitType::Capsule:
	{
		FVector End = Start + (Forward * HitData.Range);
		DrawDebugCapsule(GetWorld(), (Start + End) * 0.5f, HitData.Range * 0.5f,
			HitData.Radius, GetActorQuat(), Color, false, Duration);
		break;
	}
	}
}

void ACC_EnemyCharacter::Freeze_Implementation()
{
	if (bPersistent) return;
	if (bIsFrozen) return;

	bIsFrozen = true;

	// 추적 즉시 중단 ? AIManager가 다음 Tick에 덮어쓰지 못하도록 먼저 끊음
	// BlueprintNativeEvent는 직접 호출 금지 ? Execute 함수 경유 필수
	ICC_EnemyAIInterface::Execute_SetChasePlayer(this, false);

	// 시간 정지 (이동/물리/애니 모두 영향)
	CustomTimeDilation = 0.0f;

	// 애니메이션 정지
	if (USkeletalMeshComponent* SM = GetMesh())
	{
		SM->bPauseAnims = true;
	}

	// 공격 타이머 정지
	GetWorldTimerManager().PauseTimer(AttackCooldownTimer);

	UE_LOG(LogTemp, Log, TEXT("[Enemy %s] FROZEN"), *GetName());
}

void ACC_EnemyCharacter::Unfreeze_Implementation()
{
	if (!bIsFrozen) return;

	bIsFrozen = false;

	CustomTimeDilation = 1.0f;

	if (USkeletalMeshComponent* SM = GetMesh())
	{
		SM->bPauseAnims = false;
	}

	GetWorldTimerManager().UnPauseTimer(AttackCooldownTimer);

	// SetChasePlayer 복원은 AIManager 다음 Tick에 자동 처리됨

	UE_LOG(LogTemp, Log, TEXT("[Enemy %s] UNFROZEN"), *GetName());
}

void ACC_EnemyCharacter::RegisterToManagers()
{
	if (UCC_AIManager* AIManager = UCC_AIManager::Get(this))
	{
		AIManager->RegisterEnemy(this);
	}

	if (ACC_EnemyManager* Manager = ACC_EnemyManager::Get(this))
	{
		Manager->RegisterEnemy(this);
	}

	if (bPersistent)
	{
		if (ACC_CubeWorldManager* CubeManager = ACC_CubeWorldManager::Get(this))
		{
			CubeManager->RegisterPersistentEnemy(this);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[Enemy %s] Registered to managers."), *GetName());
}
