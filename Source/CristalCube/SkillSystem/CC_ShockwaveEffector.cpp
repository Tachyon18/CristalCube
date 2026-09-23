// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_ShockwaveEffector.h"
#include "CC_EffectorPoolSubsystem.h"
#include "../CC_EnemyManager.h"
#include "../Gameplay/CC_EnemyAIInterface.h"
#include "CC_SkillSystem.h"
#include "CC_SkillLibrarySubsystem.h"
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

	// 풀에서 재사용되는 경우를 위한 리셋 — 안 하면 이전 사용의 진행도(만료 직전 상태:
	// ElapsedTime이 이미 ExpandDuration에 도달해있는 등)를 그대로 물려받아, 재사용 즉시
	// 다음 Tick에서 Alpha>=1.0f로 판정돼 확장을 시작도 못 해보고 바로 다시 반납돼버림.
	ElapsedTime = 0.0f;
	CurrentRadius = 0.0f;
	PreviousRadius = 0.0f;
	AlreadyHit.Reset();

	SetActorLocation(Origin);
	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);

	if (Data.ShockwaveEffect)
	{
		ShockwaveVFX = UCC_SkillSystem::SpawnPersistentAttachedVFX(Data.ShockwaveEffect, Root, InSkill.ElementType);

		if (ShockwaveVFX)
		{
			ShockwaveVFX->SetFloatParameter(FName("User.Radius"), 0.0f);
		}
	}

	if (Data.ExpandDuration <= 0.0f)
	{
		// 방어: 확장시간 0이면 즉시 최대 반경으로 취급 (다음 Tick에서 바로 종료)
		CurrentRadius = Data.MaxRadius;
	}
}

void ACC_ShockwaveEffector::SetOwningPool(UCC_EffectorPoolSubsystem* InPool)
{
	OwningPool = InPool;
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
		ShockwaveVFX->SetFloatParameter(FName("User.Radius"), CurrentRadius);
	}

	// 이번 프레임에 파동 전선이 지나간 밴드
	const float BandInner = PreviousRadius;
	const float BandOuter = CurrentRadius + Data.RingThickness;

	TArray<AActor*> FoundEnemies;
	if (ACC_EnemyManager* EnemyManager = ACC_EnemyManager::Get(this))
	{
		FoundEnemies = EnemyManager->GetEnemiesInRadius(Origin, BandOuter);
	}

	UCC_SkillSystem* SkillSystem = SkillSystemRef.Get();

	for (AActor* Enemy : FoundEnemies)
	{
		if (!Enemy || !IsValid(Enemy) || Enemy == ExcludedTarget.Get()) continue;
		if (AlreadyHit.Contains(Enemy)) continue;

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
		Deactivate();
	}

}

void ACC_ShockwaveEffector::Deactivate()
{
	SetActorTickEnabled(false);
	SetActorHiddenInGame(true);

	if (ShockwaveVFX)
	{
		// Data.ShockwaveEffect는 스킬 설정마다 다른 에셋일 수 있어서(같은 풀링된 액터라도
		// 다음 사용 때 다른 나이아가라 시스템이 필요할 수 있음) 살려서 재활용하지 않고
		// 통째로 정리 — 다음 Initialize()가 필요한 에셋으로 새로 스폰함.
		ShockwaveVFX->DestroyComponent();
		ShockwaveVFX = nullptr;
	}

	if (OwningPool)
	{
		OwningPool->ReleaseEffector(this);
	}
	else
	{
		Destroy();
	}
}

