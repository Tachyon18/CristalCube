// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_ExperienceGem.h"
#include "Components/SphereComponent.h"
#include "../Characters/CC_PlayerCharacter.h"
#include "CC_Cube.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
ACC_ExperienceGem::ACC_ExperienceGem()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));

	RootComponent = CollisionSphere;

	CollisionSphere->InitSphereRadius(30.f);

	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);

	CollisionSphere->SetGenerateOverlapEvents(true);

	GemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GemMesh"));
	GemMesh->SetupAttachment(CollisionSphere);
	GemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ExpAmount = 10.f;
	MagnetRadius = 300.f;
	MoveSpeed = 500.f;
	AccelerationMultiplier = 2.f;
	bIsBeingAttracted = false;
}

// Called when the game starts or when spawned
void ACC_ExperienceGem::BeginPlay()
{
	Super::BeginPlay();

	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ACC_ExperienceGem::OnOverlapBegin);

	TargetPlayer = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);

	if (!TargetPlayer)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ExpGem] No player found!"));
	}	

	FTimerHandle InitialCheckTimer;
	GetWorld()->GetTimerManager().SetTimer(
		InitialCheckTimer,
		[this]() {
			if (!IsValid(this) || !TargetPlayer) return;

			float Distance = FVector::Dist(GetActorLocation(), TargetPlayer->GetActorLocation());

			if (Distance <= CollisionSphere->GetScaledSphereRadius())
			{
				if (ACC_PlayerCharacter* Player = Cast<ACC_PlayerCharacter>(TargetPlayer))
				{
					UE_LOG(LogTemp, Log, TEXT("[ExpGem] Auto-collected (was overlapping)"));
					Player->AddExperience(ExpAmount);
					Destroy();
					return;
				}
			}
		},
		0.1f,
		false
	);

	GetWorld()->GetTimerManager().SetTimer(
		DistanceCheckTimer,
		this,
		&ACC_ExperienceGem::CheckDistanceToPlayer,
		0.1f,
		true
	);

	FTimerHandle MergeTimer;
	GetWorld()->GetTimerManager().SetTimer(
		MergeTimer,
		this,
		&ACC_ExperienceGem::TryMergeNearbyGems,
		0.5f,
		false
	);

	GetWorld()->GetTimerManager().SetTimer(
		LifetimeTimer,
		[this]() {
			if (IsValid(this))
			{
				UE_LOG(LogTemp, Log, TEXT("[ExpGem] Expired"));
				Destroy();
			}
		},
		Lifetime,
		false
	);

}

void ACC_ExperienceGem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (OwnerCube.IsValid())
	{
		OwnerCube->UnregisterActor(this);
	}

	Super::EndPlay(EndPlayReason);
}

// Called every frame
void ACC_ExperienceGem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);


	if (bIsBeingAttracted && TargetPlayer)
	{
		MoveTowardsPlayer(DeltaTime);
	}
}

void ACC_ExperienceGem::SetOwnerCube(ACC_Cube* Cube)
{
	OwnerCube = Cube;
}

void ACC_ExperienceGem::Freeze_Implementation()
{
	if (bIsFrozen) return;
	bIsFrozen = true;

	// Tick 기반 흡수 이동(MoveTowardsPlayer)은 Cube::Freeze()의
	// SetActorTickEnabled(false)가 이미 멈춰줌 — 여기선 타이머만 일시정지
	GetWorldTimerManager().PauseTimer(LifetimeTimer);
	GetWorldTimerManager().PauseTimer(DistanceCheckTimer);

	UE_LOG(LogTemp, Log, TEXT("[ExpGem] Frozen with cube"));
}

void ACC_ExperienceGem::Unfreeze_Implementation()
{
	if (!bIsFrozen) return;
	bIsFrozen = false;

	GetWorldTimerManager().UnPauseTimer(LifetimeTimer);
	GetWorldTimerManager().UnPauseTimer(DistanceCheckTimer);

	UE_LOG(LogTemp, Log, TEXT("[ExpGem] Unfrozen with cube"));
}

void ACC_ExperienceGem::CheckDistanceToPlayer()
{
	if (!TargetPlayer)
	{
		return;
	}

	float Distance = FVector::Dist(GetActorLocation(), TargetPlayer->GetActorLocation());

	if (Distance <= MagnetRadius)
	{
		bIsBeingAttracted = true;

		GetWorld()->GetTimerManager().ClearTimer(DistanceCheckTimer);
		SetActorTickEnabled(true);
	}

}

void ACC_ExperienceGem::MoveTowardsPlayer(float DeltaTime)
{
	if (!TargetPlayer)
	{
		return;
	}

	FVector CurrentLocation = GetActorLocation();
	FVector PlayerLocation = TargetPlayer->GetActorLocation();

	FVector Direction = (PlayerLocation - CurrentLocation).GetSafeNormal();

	float Distance = FVector::Dist(CurrentLocation, PlayerLocation);
	float SpeedMultiplier = FMath::Lerp(1.0f, AccelerationMultiplier, 1.0f - (Distance / MagnetRadius));

	FVector NewLocation = CurrentLocation + Direction * MoveSpeed * SpeedMultiplier * DeltaTime;
	SetActorLocation(NewLocation);
}

void ACC_ExperienceGem::TryMergeNearbyGems()
{
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACC_ExperienceGem::StaticClass(), FoundActors);

	for (AActor* Actor : FoundActors)
	{
		if (Actor == this) continue;

		ACC_ExperienceGem* OtherGem = Cast<ACC_ExperienceGem>(Actor);
		if (!OtherGem) continue;

		float Distance = FVector::Dist(GetActorLocation(), OtherGem->GetActorLocation());

		if (Distance <= MergeRadius)
		{
			ExpAmount += OtherGem->ExpAmount;

			float Scale = FMath::Clamp(ExpAmount / 10.0f, 0.3f, 0.6f);
			GemMesh->SetWorldScale3D(FVector(Scale));

			OtherGem->Destroy();

			UE_LOG(LogTemp, Log, TEXT("[ExpGem] Merged! New value: %f EXP"), ExpAmount);
		}
	}
}

void ACC_ExperienceGem::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
	const FHitResult& SweepResult)
{
	UE_LOG(LogTemp, Log, TEXT("[ExpGem] OverlapBegin is Called"));

	if (ACC_PlayerCharacter* Player = Cast<ACC_PlayerCharacter>(OtherActor))
	{
		Player->AddExperience(ExpAmount);

		UE_LOG(LogTemp, Log, TEXT("[ExpGem] Player collected %f EXP"), ExpAmount);

		Destroy();
	}
}

void ACC_ExperienceGem::SetExpAmount(float NewAmount)
{
	ExpAmount = NewAmount;
}
