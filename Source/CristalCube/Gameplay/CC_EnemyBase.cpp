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
#include "../CC_CollisionHelper.h"
#include "../Characters/CC_PlayerCharacter.h"
#include "CC_ExperienceGem.h"
#include "CC_Cube.h"
#include "CC_VisualComponent.h"
#include "CC_StatusEffectComponent.h" 
#include "CC_ElementalStatusComponent.h"
#include "CC_DamageNumberActor.h"
#include "CC_EnemyAreaAttack.h"
#include "../WeaponSystems/CC_Projectile.h"
#include "../SkillSystem/CC_EffectorPoolSubsystem.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"


// Sets default values
ACC_EnemyBase::ACC_EnemyBase()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

    CapsuleComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComp"));
    SetRootComponent(CapsuleComp);
    CapsuleComp->InitCapsuleSize(34.f, 34.f);
    CapsuleComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);

    FCC_CollisionHelper::ConfigureAsEnemyHitbox(CapsuleComp);

    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
    MeshComp->SetupAttachment(CapsuleComp);
    MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    EnemyMovement = CreateDefaultSubobject<UCC_EnemyMovementComponent>(TEXT("EnemyMovement"));
    EnemyMovement->MaxSpeed = MoveSpeed;

    VisualComponent = CreateDefaultSubobject<UCC_VisualComponent>(TEXT("VisualComponent"));
	StatusEffectComponent = CreateDefaultSubobject<UCC_StatusEffectComponent>(TEXT("StatusEffectComponent"));
	ElementalStatusComponent = CreateDefaultSubobject<UCC_ElementalStatusComponent>(TEXT("ElementalStatusComponent"));  

    CurrentHealth = MaxHealth;
    AreaAttackClass = ACC_EnemyAreaAttack::StaticClass();

}

// Called when the game starts or when spawned
void ACC_EnemyBase::BeginPlay()
{
	Super::BeginPlay();

    // BP Collision Presets가 생성자 설정을 덮어써도 런타임에 역할 콜리전 보장
    FCC_CollisionHelper::ConfigureAsEnemyHitbox(CapsuleComp);

    CurrentHealth = MaxHealth;
    BaseMaxHealth = MaxHealth;
    BaseAttackDamage = EnemyStats.AttackDamage;
    BaseExpGemAmount = ExpGemAmount;
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

    // 캡슐 디버그 — CVar 꺼져있으면 DrawEnemyCapsuleDebug 내부에서 즉시 return
    GetWorldTimerManager().SetTimer(
        CapsuleDebugTimer, this, &ACC_EnemyBase::DrawCapsuleDebug, 0.1f, true);

}

void ACC_EnemyBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{

    GetWorldTimerManager().ClearTimer(AttackCooldownTimer);
    GetWorldTimerManager().ClearTimer(CapsuleDebugTimer);

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
    // 이동 상태(EnemyState)는 건드리지 않음 — 발사 후에도 계속 이동.
    // (이전 구현은 Attacking으로 바꾼 뒤 되돌리지 않아, 첫 공격 이후 이동/공격이 영구 정지됐음)
    bCanAttack = false;

    ActiveAttackActors.RemoveAll([this](const TWeakObjectPtr<AActor>& Weak)
        {
            const AActor* Attack = Weak.Get();
            if (const ACC_Projectile* P = Cast<ACC_Projectile>(Attack)) return P->GetProjectileOwner() != this;
            if (const ACC_EnemyAreaAttack* A = Cast<ACC_EnemyAreaAttack>(Attack)) return A->GetAttackInstigator() != this;
            return true;
        });

    if (TargetPlayer && TargetPlayer->IsAlive())
    {
        const float AttackDamage = GetEffectiveAttackDamage();

        if (AttackProjectileClass)
        {
            FireProjectileAttack(AttackDamage);
        }
        else if (AreaAttackClass)
        {
            LaunchAreaAttack(AttackDamage);
        }
        else
        {
            // 최종 폴백: 기존 즉발 데미지
            TargetPlayer->TakeDamage(AttackDamage, FDamageEvent(), nullptr, this);
        }
    }

    GetWorldTimerManager().SetTimer(
        AttackCooldownTimer,
        this,
        &ACC_EnemyBase::ResetAttackCooldown,
        GetEffectiveAttackCooldown(),
        false
    );
}

void ACC_EnemyBase::ResetAttackCooldown()
{
    bCanAttack = true;

    // 방어: 과거 상태가 남아 있어도 다음 공격/이동이 막히지 않도록
    bIsAttacking = false;
    if (EnemyState == EEnemyState::Attacking)
    {
        EnemyState = EEnemyState::Moving;
    }
}

void ACC_EnemyBase::FireProjectileAttack(float AttackDamage)
{
    const FVector MyLoc = GetActorLocation();
    const FVector TargetLoc = TargetPlayer->GetActorLocation();

    FVector AimDir2D = (TargetLoc - MyLoc).GetSafeNormal2D();
    if (AimDir2D.IsNearlyZero())
    {
        AimDir2D = GetActorForwardVector();
    }

    const FVector SpawnLoc = MyLoc + AimDir2D * ProjectileForwardOffset + FVector(0.f, 0.f, ProjectileHeightOffset);
    const FVector ShotDir = (TargetLoc - SpawnLoc).GetSafeNormal();

    if (ACC_Projectile* Projectile = ACC_Projectile::AcquirePooled(GetWorld(), AttackProjectileClass))
    {
        Projectile->Fire(SpawnLoc, ShotDir, AttackDamage, ProjectileSpeed, this);
        ActiveAttackActors.Add(Projectile);
    }
}

void ACC_EnemyBase::LaunchAreaAttack(float AttackDamage)
{
    if (ACC_EnemyAreaAttack* Area = ACC_EnemyAreaAttack::AcquirePooled(GetWorld(), AreaAttackClass))
    {
        Area->Launch(GetActorLocation(), EnemyStats.AttackRange, AttackDamage, this);
        ActiveAttackActors.Add(Area);
    }
}

void ACC_EnemyBase::CancelActiveAttacks()
{
    for (const TWeakObjectPtr<AActor>& Weak : ActiveAttackActors)
    {
        AActor* Attack = Weak.Get();

        // 풀에서 다른 적에게 재사용된 액터는 건드리지 않음 — 현재 주인이 나일 때만 취소
        if (ACC_Projectile* P = Cast<ACC_Projectile>(Attack))
        {
            if (P->GetProjectileOwner() == this) P->Deactivate();
        }
        else if (ACC_EnemyAreaAttack* A = Cast<ACC_EnemyAreaAttack>(Attack))
        {
            if (A->GetAttackInstigator() == this) A->Deactivate();
        }
    }
    ActiveAttackActors.Reset();
}

float ACC_EnemyBase::GetEffectiveAttackCooldown() const
{
    float AttackSpeedMult = 1.f;
    if (ElementalStatusComponent)
    {
        AttackSpeedMult = ElementalStatusComponent->GetAttackSpeedMultiplier();
    }

    // 0초 타이머는 SetTimer가 즉시 해제해 bCanAttack이 영영 안 돌아오므로 하한을 둠
    return FMath::Max(EnemyStats.AttackCooldown / FMath::Max(AttackSpeedMult, 0.05f), 0.05f);
}

float ACC_EnemyBase::GetEffectiveAttackDamage() const
{
    float AttackDamage = EnemyStats.AttackDamage;
    if (ElementalStatusComponent)
    {
        AttackDamage *= ElementalStatusComponent->GetOutgoingDamageMultiplier();
    }
    return AttackDamage;
}

float ACC_EnemyBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    if (!IsAlive()) return 0.f;

    // Defender-side modifier: every damage path (skills, Sigil, Shockwave, ...) goes through here
    if (ElementalStatusComponent)
    {
        DamageAmount *= ElementalStatusComponent->GetIncomingDamageMultiplier();
    }

    const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    CurrentHealth = FMath::Max(0.f, CurrentHealth - Applied);

    if (Applied > 0.f)
    {
        if (ACC_DamageNumberActor* DmgNum = CC_AcquirePooledEffector<ACC_DamageNumberActor>(GetWorld()))
        {
            FVector PopupLocation;
            if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
            {
                const FPointDamageEvent& PointEvent = static_cast<const FPointDamageEvent&>(DamageEvent);
                PopupLocation = PointEvent.HitInfo.ImpactPoint + FVector(0.f, 0.f, 20.f);
            }
            else
            {
                // Fallback: colliding components only (excludes attached Niagara bounds)
                FVector Origin, BoxExtent;
                GetActorBounds(true, Origin, BoxExtent);
                PopupLocation = Origin + FVector(0.f, 0.f, BoxExtent.Z + 20.f);
            }
            DmgNum->Initialize(Applied, PopupLocation);
        }
    }

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

    if (bPersistent)
    {
        if (ACC_CubeWorldManager* CubeManager = ACC_CubeWorldManager::Get(this))
            CubeManager->UnregisterPersistentEnemy(this);
    }

    SetLifeSpan(0.5f);
}

void ACC_EnemyBase::Freeze_Implementation()
{
    if (bPersistent) return;
    if (bIsFrozen) return;
    bIsFrozen = true;
    CustomTimeDilation = 0.0f;

    if (EnemyMovement)
        EnemyMovement->StopMovementImmediately();

    GetWorldTimerManager().PauseTimer(AttackCooldownTimer);
    
    // 임시: 얼려진 적의 진행 중 공격 제거 (Track 6 C 전환 정리에서 일괄 처리로 이관 예정)
    CancelActiveAttacks();

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

void ACC_EnemyBase::SetPersistentEnemy_Implementation(bool bPersistentState)
{

    bPersistent = bPersistentState;

    if (ACC_CubeWorldManager* CubeManager = ACC_CubeWorldManager::Get(this))
    {
        if (bPersistent)
        {
            CubeManager->RegisterPersistentEnemy(this);
        }
        else
        {
            CubeManager->UnregisterPersistentEnemy(this);
        }
    }

    if (VisualComponent)
    {
        VisualComponent->SetPersistentVisualState(bPersistent, MeshComp);
    }

    OnPersistentStateChanged(bPersistent);
}

void ACC_EnemyBase::ResetMovementState_Implementation()
{
    if (EnemyMovement)
    {
        EnemyMovement->StopMovementImmediately();
    }

    // 텔레포트된 새 위치를 기준으로 다시 판단하도록 목표/상태 리셋
    MoveTarget = GetActorLocation();
    EnemyState = EEnemyState::Moving;
    bIsAttacking = false;

    // 텔레포트(Persistent 큐브 이동)로 위치가 바뀌면 이전 위치에서 쏜 공격은 무효 — Freeze를 안 타는 Persistent 적 대응
    CancelActiveAttacks();
}

void ACC_EnemyBase::ApplyStatMultiplier_Implementation(float Multiplier)
{
    if (Multiplier <= 0.f || BaseMaxHealth <= 0.f) return;

    const float Ratio = (CurrentStatMultiplier > 0.f) ? (Multiplier / CurrentStatMultiplier) : Multiplier;

    MaxHealth = BaseMaxHealth * Multiplier;
    CurrentHealth = FMath::Clamp(CurrentHealth * Ratio, 1.f, MaxHealth);

    EnemyStats.AttackDamage = BaseAttackDamage * Multiplier;
    ExpGemAmount = BaseExpGemAmount * Multiplier;

    CurrentStatMultiplier = Multiplier;

    UE_LOG(LogTemp, Log, TEXT("[%s] Stat multiplier applied: x%.2f (HP %.0f/%.0f, DMG %.1f, EXP %.1f)"),
        *GetName(), Multiplier, CurrentHealth, MaxHealth, EnemyStats.AttackDamage, ExpGemAmount);

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
        { EEnemyShapeType::Octahedron, TEXT("/Game/Develop/Mesh/SM_Octahedron.SM_Octahedron") },
		{ EEnemyShapeType::Tetrahedron, TEXT("/Game/Develop/Mesh/SM_Tetrahedron.SM_Tetrahedron") }

    };

    MeshComp->SetRelativeScale3D(ShapeScale);

    if (ShapeType == EEnemyShapeType::Custom)
    {
        if (UStaticMesh* Mesh = CustomMesh.LoadSynchronous())
            MeshComp->SetStaticMesh(Mesh);

        AutoFitCapsuleToMesh(); // ← 변경: 기존엔 여기서 그냥 return, Custom은 재단 안 됐음
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
    //const float OldHalfHeight = CapsuleComp->GetUnscaledCapsuleHalfHeight();

    const FBoxSphereBounds Bounds = MeshComp->CalcBounds(MeshComp->GetComponentTransform());
    float Radius = FMath::Max(Bounds.BoxExtent.X, Bounds.BoxExtent.Y) * 0.75f;
    float HalfHeight = FMath::Max(Bounds.BoxExtent.Z, Radius);

    Radius *= CapsuleFitMultiplier;
    HalfHeight *= CapsuleFitMultiplier;

    CapsuleComp->SetCapsuleSize(Radius, HalfHeight);

    // 스폰 시점 위치가 "지면"이라고 가정하지 않고, 실제로 라인 트레이스를 쏴서
    // 진짜 지면 Z를 직접 측정. 스폰 경로든 에디터 수동 배치든, Scale이 뭐든 전부 대응됨.
    const FVector CurrentLoc = GetActorLocation();
    const FVector TraceStart = CurrentLoc + FVector(0.f, 0.f, 500.f);
    const FVector TraceEnd = CurrentLoc - FVector(0.f, 0.f, 2000.f);

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    FHitResult Hit;
    float FloorZ = CurrentLoc.Z; // 트레이스 실패 시 폴백 — 현재 위치 유지

    if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic, Params))
    {
        FloorZ = Hit.Location.Z;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[%s] AutoFitCapsuleToMesh: Trace failed."),
            *GetName(), FloorZ);
    }

    LastMeasuredFloorZ = FloorZ;

    FVector NewLoc = CurrentLoc;
    NewLoc.Z = FloorZ + HalfHeight;
}

void ACC_EnemyBase::ApplyMeshZOffset()
{
    if (!MeshComp) return;

    MeshComp->SetRelativeLocation(FVector(0.f, 0.f, MeshZOffset));
}

void ACC_EnemyBase::DrawCapsuleDebug()
{
    FCC_CollisionHelper::DrawEnemyCapsuleDebug(GetWorld(), CapsuleComp, bIsFrozen, bPersistent);
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
    if (UCC_AIManager* AIManager = UCC_AIManager::Get(this))
    {
        AIManager->RegisterEnemy(this);
    }

    if (ACC_EnemyManager* Manager = ACC_EnemyManager::Get(this))
    {
        Manager->RegisterEnemy(this);
    }

    UE_LOG(LogTemp, Warning, TEXT("[%s] bPersistent value check: %s"),
        *GetName(), bPersistent ? TEXT("true") : TEXT("false"));

    if (bPersistent)
    {
        ICC_EnemyAIInterface::Execute_SetPersistentEnemy(this, true);
    }
}
