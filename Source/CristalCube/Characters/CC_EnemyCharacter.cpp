// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_EnemyCharacter.h"
#include "Engine/DamageEvents.h"
#include "Kismet/GameplayStatics.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "TimerManager.h"
#include "CC_PlayerCharacter.h"
#include "../CC_LogHelper.h"
#include "../CC_EnemyManager.h"
#include "../CC_AIManager.h"
#include "../CC_EnemyAIController.h"
#include "../Gameplay/CC_ExperienceGem.h"

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
		Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		Capsule->SetGenerateOverlapEvents(true);
	}

	AttackRangeSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AttackRangeSphere"));
	AttackRangeSphere->SetupAttachment(RootComponent);
	AttackRangeSphere->InitSphereRadius(EnemyStats.AttackRange * 0.75f); // 75% of attack range

	AttackRangeSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	AttackRangeSphere->SetCollisionObjectType(ECC_WorldDynamic);
	AttackRangeSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	AttackRangeSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	AttackRangeSphere->SetGenerateOverlapEvents(true);

	AttackRangeSphere->SetHiddenInGame(false);
	AttackRangeSphere->ShapeColor = FColor::Red;

	AttackHitData.HitType = EAttackHitType::Line;
	AttackHitData.Range = 200.0f;
	AttackHitData.bPenetrate = false;

	// AI settings
	DetectionRange = 2000.0f;  // 20 meters
	bChasePlayer = true;
	TargetPlayer = nullptr;

	// Combat settings
	ContactDamage = 10.0f;     // 10 damage per hit
	DamageCooldown = 1.0f;     // 1 second between hits
	LastDamageTime = 0.0f;

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

	// Setup overlap events for contact damage
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->OnComponentBeginOverlap.AddDynamic(this, &ACC_EnemyCharacter::OnOverlapBegin);
	}

	CC_LOG_ENEMY(Warning, TEXT("Spawned enemy: %s (HP: %.0f, Damage: %.0f)"),
		*EnemyType, MaxHealth, ContactDamage);

	if (AttackRangeSphere)
	{
		AttackRangeSphere->OnComponentBeginOverlap.AddDynamic(
			this, &ACC_EnemyCharacter::OnAttackRangeBeginOverlap
		);

		AttackRangeSphere->OnComponentEndOverlap.AddDynamic(
			this, &ACC_EnemyCharacter::OnAttackRangeEndOverlap
		);

		CC_LOG_ENEMY(Warning, TEXT("%s - Attack range sphere initialized (Radius: %.1f)"), *GetName(), AttackRangeSphere->GetScaledSphereRadius());
	}

	if (UCC_AIManager* AIManager = UCC_AIManager::Get(this))
	{
		AIManager->RegisterEnemy(this);
		UE_LOG(LogTemp, Log, TEXT("Enemy registered with AI Manager: %s"), *GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AI Manager not found, using fallback AI for: %s"), *GetName());
		// Fallback: 기존 방식으로 동작 (AI Manager 없어도 게임 진행 가능)
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
	}

	if (ACC_EnemyManager* Manager = ACC_EnemyManager::Get(this))
	{
		Manager->RegisterEnemy(this);
	}
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
	if (bChasePlayer && IsAlive() && !bIsAttacking)
	{
		ChasePlayer(DeltaTime);
	}
}

void ACC_EnemyCharacter::ChasePlayer(float DeltaTime)
{
	if (!TargetPlayer)
	{
		// Try to find player again
		FindPlayer();
		return;
	}

	// Check if player is within detection range
	float DistanceToPlayer = FVector::Dist(GetActorLocation(), TargetPlayer->GetActorLocation());

	if (DistanceToPlayer > DetectionRange)
	{
		// Player too far, don't chase
		return;
	}

	// Calculate direction to player
	FVector DirectionToPlayer = (TargetPlayer->GetActorLocation() - GetActorLocation()).GetSafeNormal();

	// Move towards player
	AddMovementInput(DirectionToPlayer, 1.0f);

	// Rotate to face player
	FRotator LookAtRotation = DirectionToPlayer.Rotation();
	LookAtRotation.Pitch = 0.0f;  // Keep on ground
	LookAtRotation.Roll = 0.0f;

	SetActorRotation(FMath::RInterpTo(GetActorRotation(), LookAtRotation, DeltaTime, 5.0f));

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

bool ACC_EnemyCharacter::CanDealDamage() const
{
	float CurrentTime = GetWorld()->GetTimeSeconds();
	return (CurrentTime - LastDamageTime) >= DamageCooldown;
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

void ACC_EnemyCharacter::DealContactDamage(AActor* OtherActor)
{
	if (!OtherActor || !CanDealDamage())
	{
		return;
	}

	// Check if it's the player
	ACC_PlayerCharacter* Player = Cast<ACC_PlayerCharacter>(OtherActor);
	if (!Player)
	{
		return;
	}

	// Deal damage to player
	UGameplayStatics::ApplyDamage(
		Player,
		ContactDamage,
		GetController(),
		this,
		UDamageType::StaticClass()
	);

	// Update cooldown
	LastDamageTime = GetWorld()->GetTimeSeconds();

	UE_LOG(LogTemp, Log, TEXT("Enemy dealt %.0f contact damage to player"), ContactDamage);

}

void ACC_EnemyCharacter::Die()
{
	ReportActualDeathToEnemyManager();


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

	// Call base class Die() to handle death animation, etc.
	Super::Die();

	// TODO: Spawn death effect, drop items, etc.

	// Destroy enemy after short delay
	SetLifeSpan(1.0f);
}

void ACC_EnemyCharacter::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	//DealContactDamage(OtherActor);

	//UE_LOG(LogTemp, Warning, TEXT("[ENEMY] %s - OnOverlapBegin with: %s") *GetName(), *OtherActor->GetName());

	if(ACC_PlayerCharacter* Player = Cast<ACC_PlayerCharacter>(OtherActor))
	{
		//UE_LOG(LogTemp, Warning, TEXT("[ENEMY] %s - Detected PLAYER overlap!"), *GetName());

		TargetPlayer = Player;


		//UE_LOG(LogTemp, Warning, TEXT("[ENEMY] %s - Calling TryAttack..."), *GetName());


		TryAttack(Player);


	}
	else
	{
		//UE_LOG(LogTemp, Log, TEXT("[ENEMY] %s - Overlap with non-player: %s"), *GetName(), *OtherActor->GetClass()->GetName());
	}
}

void ACC_EnemyCharacter::PerformAttack()
{
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

	UE_LOG(LogTemp, Log, TEXT("[ENEMY] %s attack ready!"), *GetName());

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

void ACC_EnemyCharacter::OnAttackRangeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	//CC_LOG_ENEMY(Warning, TEXT("%s - OnAttackRangeBeginOverlap with: %s"), *GetName(), *OtherActor->GetName());

	if(ACC_PlayerCharacter* Player = Cast<ACC_PlayerCharacter>(OtherActor))
	{
		if (!bPlayerInRange)
		{
			bPlayerInRange = true;
			TargetPlayer = Player;

			CC_LOG_ENEMY(Warning, TEXT("%s - Player entered ATTACK RANGE"),
				*GetName());

			TryAttack(Player);
		}
	}

}

void ACC_EnemyCharacter::OnAttackRangeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor == TargetPlayer)
	{
		bPlayerInRange = false;
		TargetPlayer = nullptr;

		//CC_LOG_ENEMY(Log, TEXT("%s - Player left ATTACK RANGE"), *GetName());
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
