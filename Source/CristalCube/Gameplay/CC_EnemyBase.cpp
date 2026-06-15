// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_EnemyBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "CC_EnemyMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Engine/DamageEvents.h"
#include "../CC_EnemyManager.h"
#include "../CC_AIManager.h"
#include "../CC_CubeWorldManager.h"
#include "../Characters/CC_PlayerCharacter.h"
#include "../Gameplay/CC_ExperienceGem.h"

// Sets default values
ACC_EnemyBase::ACC_EnemyBase()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

    CapsuleComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComp"));
    SetRootComponent(CapsuleComp);
    CapsuleComp->InitCapsuleSize(34.f, 34.f);
    CapsuleComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
    CapsuleComp->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Overlap);

    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
    MeshComp->SetupAttachment(CapsuleComp);
    MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    EnemyMovement = CreateDefaultSubobject<UCC_EnemyMovementComponent>(TEXT("EnemyMovement"));
    EnemyMovement->MaxSpeed = MoveSpeed;

    CurrentHealth = MaxHealth;

}

// Called when the game starts or when spawned
void ACC_EnemyBase::BeginPlay()
{
	Super::BeginPlay();
	
    CurrentHealth = MaxHealth;
    Tags.AddUnique(FName("Enemy"));

    FindPlayer();

    InitShape();

    // TargetOffset 랜덤 초기화
    if (TargetOffsetRadius > 0.f)
    {
        const float Angle = FMath::FRandRange(0.f, 360.f);
        const float Radius = FMath::FRandRange(TargetOffsetRadius * 0.5f, TargetOffsetRadius);
        TargetOffset = FVector(
            FMath::Cos(FMath::DegreesToRadians(Angle)) * Radius,
            FMath::Sin(FMath::DegreesToRadians(Angle)) * Radius,
            0.f
        );
    }

    GetWorldTimerManager().SetTimerForNextTick(this, &ACC_EnemyBase::RegisterToManagers);

}

void ACC_EnemyBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{

    GetWorldTimerManager().ClearTimer(AttackCooldownTimer);

    if (UCC_AIManager* AIManager = UCC_AIManager::Get(this))
       AIManager->UnregisterEnemy(this);

    if (ACC_EnemyManager* Manager = ACC_EnemyManager::Get(this))
       Manager->UnregisterEnemy(this);

	Super::EndPlay(EndPlayReason);
}

// Called every frame
void ACC_EnemyBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);


}

// Called to bind functionality to input
void ACC_EnemyBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void ACC_EnemyBase::FindPlayer()
{
    TargetPlayer = Cast<ACC_PlayerCharacter>(
        UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
}

void ACC_EnemyBase::UpdateMoveTarget()
{
    if (!TargetPlayer) FindPlayer();
    if (!TargetPlayer) return;

    const float DistSq = FVector::DistSquared(GetActorLocation(), TargetPlayer->GetActorLocation());

    if (DistSq <= DetectionRange * DetectionRange)
    {
        MoveTarget = TargetPlayer->GetActorLocation() + TargetOffset;
    }
    else
    {
        MoveTarget = FVector::ZeroVector;
    }
}

void ACC_EnemyBase::PerformMove()
{
    if (!bMovementEnabled || EnemyState == EEnemyState::Attacking) return;

    UpdateMoveTarget();

    switch (MovementBehavior)
    {
    case EMovementBehavior::Direct:
        PerformMove_Direct();
        break;

    case EMovementBehavior::Step:
        // Phase 3 구현 예정
        PerformMove_Direct();
        break;

    case EMovementBehavior::Teleport:
        // 별도 TeleportTimerHandle로 처리 — 여기선 아무것도 안 함
        break;

    case EMovementBehavior::Waypoint:
        // 후순위
        PerformMove_Direct();
        break;
    }
}

void ACC_EnemyBase::PerformMove_Direct()
{
    if (MoveTarget.IsZero() || !EnemyMovement) return;

    const FVector CurrentLoc = GetActorLocation();
    const FVector Direction = (MoveTarget - CurrentLoc).GetSafeNormal2D();

    if (Direction.IsNearlyZero())
    {
        EnemyMovement->StopMovementImmediately();
        return;
    }

    // Controller 의존 없이 직접 속도 세팅
    EnemyMovement->Velocity = Direction * MoveSpeed;

    // 이동 방향으로 회전
    FRotator LookAt = Direction.Rotation();
    LookAt.Pitch = 0.f;
    LookAt.Roll = 0.f;
    SetActorRotation(LookAt);
}

void ACC_EnemyBase::PerformMove_Teleport()
{
    if (!bMovementEnabled || !TargetPlayer) return;

    UpdateMoveTarget();
    if (MoveTarget.IsZero()) return;

    // 플레이어 근방 랜덤 위치로 순간이동
    const float Angle = FMath::FRandRange(0.f, 360.f);
    const float Radius = FMath::FRandRange(TeleportRadius * 0.5f, TeleportRadius);

    FVector TeleportDest = MoveTarget + FVector(
        FMath::Cos(FMath::DegreesToRadians(Angle)) * Radius,
        FMath::Sin(FMath::DegreesToRadians(Angle)) * Radius,
        0.f
    );

    SetActorLocation(TeleportDest, false);
}

void ACC_EnemyBase::CheckAndPerformAttack()
{
    if (!bCanAttack || bIsAttacking || !IsAlive()) return;
    if (!TargetPlayer || !TargetPlayer->IsAlive()) return;
    if (EnemyState != EEnemyState::Moving) return;

    const float DistSq = FVector::DistSquared(
        GetActorLocation(), TargetPlayer->GetActorLocation());
    const float RangeSq = EnemyStats.AttackRange * EnemyStats.AttackRange;

    if (DistSq <= RangeSq)
        PerformAttack();
}


void ACC_EnemyBase::TryAttack(ACC_PlayerCharacter* Target)
{
    if (!Target || !IsAlive() || !bCanAttack || bIsAttacking) return;

    const float DistSq = FVector::DistSquared(GetActorLocation(), Target->GetActorLocation());
    if (DistSq > EnemyStats.AttackRange * EnemyStats.AttackRange) return;

    TargetPlayer = Target;
    PerformAttack();
}

void ACC_EnemyBase::PerformAttack()
{
    EnemyState = EEnemyState::Attacking;
    bIsAttacking = true;
    bCanAttack = false;

    // 즉발 데미지 (애니메이션 없는 경량 Enemy)
    if (TargetPlayer && TargetPlayer->IsAlive())
    {
        TargetPlayer->TakeDamage(
            EnemyStats.AttackDamage,
            FDamageEvent(),
            nullptr,
            this
        );
    }

    bIsAttacking = false;
    EnemyState = EEnemyState::Moving;

    // 쿨다운 후 이동 재개
    GetWorldTimerManager().SetTimer(
        AttackCooldownTimer,
        this,
        &ACC_EnemyBase::ResetAttackCooldown,
        EnemyStats.AttackCooldown,
        false
    );
}

void ACC_EnemyBase::ResetAttackCooldown()
{
    bCanAttack = true;
}


float ACC_EnemyBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    if (!IsAlive()) return 0.f;

    const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    CurrentHealth = FMath::Max(0.f, CurrentHealth - Applied);

    if (CurrentHealth <= 0.f)
        Die();

    return Applied;
}

void ACC_EnemyBase::Die()
{
    if (bIsDead) return;
    bIsDead = true;

    GetWorldTimerManager().ClearTimer(AttackCooldownTimer);

    // EXP 젬 드롭
    if (ExpGemClass)
    {
        const float Angle = FMath::FRandRange(0.f, 360.f);
        const float Radius = FMath::FRandRange(50.f, 150.f);
        FVector SpawnLoc = GetActorLocation() + FVector(
            FMath::Cos(FMath::DegreesToRadians(Angle)) * Radius,
            FMath::Sin(FMath::DegreesToRadians(Angle)) * Radius,
            50.f
        );

        if (ACC_ExperienceGem* Gem = GetWorld()->SpawnActor<ACC_ExperienceGem>(
            ExpGemClass, SpawnLoc, FRotator::ZeroRotator))
        {
            Gem->SetExpAmount(ExpGemAmount);
        }
    }

    if (UCC_AIManager* AIManager = UCC_AIManager::Get(this))
        AIManager->UnregisterEnemy(this);

    if (ACC_EnemyManager* Manager = ACC_EnemyManager::Get(this))
        Manager->ReportEnemyKilled(this);

    SetLifeSpan(0.5f);
}

void ACC_EnemyBase::Freeze_Implementation()
{
    if (bIsFrozen) return;
    bIsFrozen = true;
    CustomTimeDilation = 0.0f;

    if (EnemyMovement)
        EnemyMovement->StopMovementImmediately();

    GetWorldTimerManager().PauseTimer(AttackCooldownTimer);
}

void ACC_EnemyBase::Unfreeze_Implementation()
{
    if (!bIsFrozen) return;
    bIsFrozen = false;  
    CustomTimeDilation = 1.0f;
    GetWorldTimerManager().UnPauseTimer(AttackCooldownTimer);
    // SetMovementEnabled는 AIManager가 SetChasePlayer로 제어
}

void ACC_EnemyBase::SetChasePlayer_Implementation(bool bChase)
{  
    bMovementEnabled = bChase;
}

void ACC_EnemyBase::InitShape()
{
    if (!MeshComp) return;

    // UE5 기본 제공 4종 (MVP)
    // Tetrahedron / Octahedron 은 Blender 에셋 임포트 후 경로 채울 것
    static const TMap<EEnemyShapeType, FString> ShapeAssetPaths =
    {
        { EEnemyShapeType::Cube,     TEXT("/Engine/BasicShapes/Cube.Cube")           },
        { EEnemyShapeType::Sphere,   TEXT("/Engine/BasicShapes/Sphere.Sphere")       },
        { EEnemyShapeType::Cylinder, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")   },
        { EEnemyShapeType::Cone,     TEXT("/Engine/BasicShapes/Cone.Cone")           },
    };

    if (ShapeType == EEnemyShapeType::Custom)
    {
        if (UStaticMesh* Mesh = CustomMesh.LoadSynchronous())
            MeshComp->SetStaticMesh(Mesh);
        return;
    }

    if (const FString* PathPtr = ShapeAssetPaths.Find(ShapeType))
    {
        if (UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, **PathPtr))
        {
            MeshComp->SetStaticMesh(Mesh);
            AutoFitCapsuleToMesh();
        }
    }
}

void ACC_EnemyBase::AutoFitCapsuleToMesh()
{
    if (!MeshComp || !CapsuleComp) return;

    const FBoxSphereBounds Bounds = MeshComp->CalcBounds(MeshComp->GetComponentTransform());
    const float Radius = FMath::Max(Bounds.BoxExtent.X, Bounds.BoxExtent.Y) * 0.75f;
    const float HalfHeight = FMath::Max(Bounds.BoxExtent.Z, Radius);

    CapsuleComp->SetCapsuleSize(Radius, HalfHeight);
}

void ACC_EnemyBase::SetMovementEnabled(bool bEnabled)
{
    bMovementEnabled = bEnabled;

    if (EnemyMovement)
    {
        EnemyMovement->MaxSpeed = bEnabled ? MoveSpeed : 0.f;

        if (!bEnabled)
        {
            EnemyMovement->StopMovementImmediately();
        }
    }
}

void ACC_EnemyBase::RegisterToManagers()
{
    if (UCC_AIManager* AIManager = UCC_AIManager::Get(this))
    {
        AIManager->RegisterEnemy(this);
    }

    if (ACC_EnemyManager* Manager = ACC_EnemyManager::Get(this))
    {
        Manager->RegisterEnemy(this);
    }
}
