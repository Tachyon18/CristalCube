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
#include "../Gameplay/CC_Cube.h"

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
    CapsuleComp->SetGenerateOverlapEvents(true);

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

    if (EnemyMovement)
    {
        EnemyMovement->SetMovementBehavior(MovementBehavior);
        EnemyMovement->MaxSpeed = MoveSpeed;
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

    // Behavior 분기 책임은 EnemyMovement 컴포넌트로 이전됨.
    // EnemyBase는 "어디로 갈지"만 전달.
    EnemyMovement->SetMoveTarget(MoveTarget);
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
        
            if (ACC_CubeWorldManager* CubeManager = ACC_CubeWorldManager::Get(this))
            {
                if (ACC_Cube* ActiveCubeRef = CubeManager->GetActiveCube())
                {
                    ActiveCubeRef->RegisterActor(Gem);
                    Gem->SetOwnerCube(ActiveCubeRef);
                }
            }
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

    if (UCC_AIManager* AIManager = UCC_AIManager::Get(this))
    {
        AIManager->UnregisterEnemy(this);
    }
}

void ACC_EnemyBase::Unfreeze_Implementation()
{
    if (!bIsFrozen) return;
    bIsFrozen = false;  
    CustomTimeDilation = 1.0f;
    GetWorldTimerManager().UnPauseTimer(AttackCooldownTimer);
    // SetMovementEnabled는 AIManager가 SetChasePlayer로 제어

    if (UCC_AIManager* AIManager = UCC_AIManager::Get(this))
    {
        AIManager->RegisterEnemy(this);
    }
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

        ApplyMeshZOffset();
        return;
    }

    if (const FString* PathPtr = ShapeAssetPaths.Find(ShapeType))
    {
        if (UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, **PathPtr))
        {
            MeshComp->SetStaticMesh(Mesh);
            AutoFitCapsuleToMesh();
            ApplyMeshZOffset();
        }
    }
}

void ACC_EnemyBase::AutoFitCapsuleToMesh()
{
    if (!MeshComp || !CapsuleComp) return;

    // 캡슐 "크기"만 메시 바운드 기준으로 자동 계산.
    // 위치(Z) 보정은 더 이상 여기서 하지 않음 — MeshZOffset으로 디자이너가 직접 지정.
    const FBoxSphereBounds Bounds = MeshComp->CalcBounds(MeshComp->GetComponentTransform());
    const float Radius = FMath::Max(Bounds.BoxExtent.X, Bounds.BoxExtent.Y) * 0.75f;
    const float HalfHeight = FMath::Max(Bounds.BoxExtent.Z, Radius);

    CapsuleComp->SetCapsuleSize(Radius, HalfHeight);
}

void ACC_EnemyBase::ApplyMeshZOffset()
{
    if (!MeshComp) return;

    MeshComp->SetRelativeLocation(FVector(0.f, 0.f, MeshZOffset));
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

void ACC_EnemyBase::SetMovementBehavior(EMovementBehavior NewBehavior)
{
    MovementBehavior = NewBehavior;

    if (EnemyMovement)
    {
        EnemyMovement->SetMovementBehavior(MovementBehavior);
    }
}

void ACC_EnemyBase::RegisterToManagers()
{
    if (bIsFrozen)
    {
        return;
    }

    if (UCC_AIManager* AIManager = UCC_AIManager::Get(this))
    {
        AIManager->RegisterEnemy(this);
    }

    if (ACC_EnemyManager* Manager = ACC_EnemyManager::Get(this))
    {
        Manager->RegisterEnemy(this);
    }
}
