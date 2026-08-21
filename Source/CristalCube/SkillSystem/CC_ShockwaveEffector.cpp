// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_ShockwaveEffector.h"
#include "../Gameplay/CC_EnemyAIInterface.h"
#include "CC_SkillSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DamageEvents.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

// Sets default values
ACC_ShockwaveEffector::ACC_ShockwaveEffector()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	SetActorEnableCollision(false);
}

void ACC_ShockwaveEffector::Initialize(FVector InOrigin, float InDamage, AActor* InInstigator, const FShockwaveAddonData& InData, AActor* InExcludedTarget, UCC_SkillSystem* InSkillSystem, const FSkillDefinition& InSkill, const FSkillExecutionContext& InContext, int32 InStartIndex)
{
	Origin = InOrigin;
	Damage = InDamage;
	DamageInstigator = InInstigator;
	Data = InData;
	ExcludedTarget = Data.bExcludeOriginTarget ? InExcludedTarget : nullptr;

	SkillSystemRef = InSkillSystem;
	SkillDef = InSkill;
	BaseContext = InContext;
	BaseContext.HitActors.Reset();
	BaseContext.CurrentChainCount = 0;
	AddonStartIndex = InStartIndex;

	SetActorLocation(Origin);

	if (Data.ShockwaveEffect)
	{
		ShockwaveVFX = UNiagaraFunctionLibrary::SpawnSystemAttached(
			Data.ShockwaveEffect,
			Root,
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			false,  // bAutoDestroy — 이 액터 수명(ExpandDuration)에 종속시킴
			true,   // bAutoActivate
			ENCPoolMethod::None,  // 반경을 매틱 직접 갱신하므로 풀링 미사용
			true    // bPreCullCheck
		);

		if (ShockwaveVFX)
		{
			ShockwaveVFX->SetFloatParameter(FName("Radius"), 0.0f);
		}
	}

	if (Data.ExpandDuration <= 0.0f)
	{
		// 방어: 확장시간 0이면 즉시 최대 반경으로 취급 (다음 Tick에서 바로 종료)
		CurrentRadius = Data.MaxRadius;
	}
}

// Called when the game starts or when spawned
void ACC_ShockwaveEffector::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ACC_ShockwaveEffector::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ElapsedTime += DeltaTime;

	PreviousRadius = CurrentRadius;
	const float Alpha = (Data.ExpandDuration > 0.0f)
		? FMath::Clamp(ElapsedTime / Data.ExpandDuration, 0.0f, 1.0f)
		: 1.0f;
	CurrentRadius = FMath::Lerp(0.0f, Data.MaxRadius*2, Alpha);

	if (ShockwaveVFX && IsValid(ShockwaveVFX))
	{
		ShockwaveVFX->SetFloatParameter(FName("Radius"), CurrentRadius);
	}

	// 이번 프레임에 파동 전선이 지나간 밴드
	const float BandInner = PreviousRadius;
	const float BandOuter = CurrentRadius + Data.RingThickness;

	TArray<AActor*> FoundEnemies;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("Enemy"), FoundEnemies);

	UCC_SkillSystem* SkillSystem = SkillSystemRef.Get();

	for (AActor* Enemy : FoundEnemies)
	{
		if (!Enemy || !IsValid(Enemy) || Enemy == ExcludedTarget.Get()) continue;
		if (AlreadyHit.Contains(Enemy)) continue;

		if (Enemy->GetClass()->ImplementsInterface(UCC_EnemyAIInterface::StaticClass())
			&& ICC_EnemyAIInterface::Execute_GetIsFrozen(Enemy))
		{
			continue;
		}

		const float Dist = FVector::Dist(Origin, Enemy->GetActorLocation());
		if (Dist >= BandInner && Dist <= BandOuter)
		{
			Enemy->TakeDamage(Damage, FDamageEvent(), nullptr, DamageInstigator.Get());
			AlreadyHit.Add(Enemy);

			if (SkillSystem)
			{
				// 이번에 파동 전선에 새로 맞은 것 자체가 '개별 타격' — 독립된 Context 사본으로 하위 Addon에 전달
				FSkillExecutionContext TickContext = BaseContext;
				TickContext.CurrentChainCount = 0;
				TickContext.CurrentDamage = Damage;
				TickContext.HitActors.Add(Enemy);

				FHitResult TickHit;
				TickHit.ImpactPoint = Enemy->GetActorLocation();
				TickHit.HitObjectHandle = FActorInstanceHandle(Enemy);
				SkillSystem->ProcessAddons(SkillDef, TickContext, TickHit, AddonStartIndex);
			}
		}
	}

	if (Alpha >= 1.0f)
	{
		Destroy();  // Root에 Attach된 ShockwaveVFX도 함께 정리됨
	}

}

