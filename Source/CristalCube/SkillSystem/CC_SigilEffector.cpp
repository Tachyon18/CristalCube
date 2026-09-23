// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_SigilEffector.h"
#include "CC_EffectorPoolSubsystem.h"
#include "../CC_EnemyManager.h"
#include "../Gameplay/CC_EnemyAIInterface.h"
#include "CC_SkillSystem.h"
#include "CC_SkillLibrarySubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DamageEvents.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "TimerManager.h"

// Sets default values
ACC_SigilEffector::ACC_SigilEffector()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	SetActorEnableCollision(false);
}

void ACC_SigilEffector::Initialize(FVector InOrigin, AActor* InInstigator, const FSigilAddonData& InData, UCC_SkillSystem* InSkillSystem, const FSkillDefinition& InSkill, const FSkillExecutionContext& InContext, int32 InStartIndex)
{
	Origin = InOrigin;
	DamageInstigator = InInstigator;
	Data = InData;

	SkillSystemRef = InSkillSystem;
	SkillDef = InSkill;
	BaseContext = InContext;
	BaseContext.HitActors.Reset();
	BaseContext.CurrentChainCount = 0;
	AddonStartIndex = InStartIndex;

	SetActorLocation(Origin);
	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);

	if (Data.SigilEffect)
	{
		SigilVFX = UCC_SkillSystem::SpawnPersistentAttachedVFX(Data.SigilEffect, Root, InSkill.ElementType);

		if (SigilVFX)
		{
			SigilVFX->SetFloatParameter(FName("User.Radius"), Data.Radius);
		}
	}

	if (Data.TickInterval > 0.0f)
	{
		GetWorldTimerManager().SetTimer(TickTimer, this, &ACC_SigilEffector::ApplyTick, Data.TickInterval, true);
	}

	SetLifeSpan(Data.Duration);  // 종료 시 자동 파괴 (Attach된 VFX도 함께 정리)
}

void ACC_SigilEffector::SetOwningPool(UCC_EffectorPoolSubsystem* InPool)
{
	OwningPool = InPool;
}

// Called when the game starts or when spawned
void ACC_SigilEffector::BeginPlay()
{
	Super::BeginPlay();
	
}

void ACC_SigilEffector::LifeSpanExpired()
{
	Deactivate();
}

// Called every frame
void ACC_SigilEffector::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ACC_SigilEffector::Deactivate()
{
	SetLifeSpan(0.0f); // 대기 중이던 LifeSpan 타이머 취소(조기 반납 경로 대비)
	GetWorldTimerManager().ClearTimer(TickTimer); // 반복 데미지 타이머 정지 — 안 지우면
	// 풀에서 대기하는 동안에도 ApplyTick()이
	// 계속 불려서 엉뚱한 상태로 데미지를 줌

	SetActorTickEnabled(false);
	SetActorHiddenInGame(true);

	if (SigilVFX)
	{
		// Data.SigilEffect는 스킬 설정마다 다른 에셋일 수 있어서 살려서 재활용하지 않고
		// 통째로 정리 — 다음 Initialize()가 필요한 에셋으로 새로 스폰함.
		SigilVFX->DestroyComponent();
		SigilVFX = nullptr;
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

void ACC_SigilEffector::ApplyTick()
{
	TArray<AActor*> FoundEnemies;
	if (ACC_EnemyManager* EnemyManager = ACC_EnemyManager::Get(this))
	{
		FoundEnemies = EnemyManager->GetEnemiesInRadius(Origin, Data.Radius);
	}

	UCC_SkillSystem* SkillSystem = SkillSystemRef.Get();

	for (AActor* Enemy : FoundEnemies)
	{
		if (!Enemy || !IsValid(Enemy)) continue;

		Enemy->TakeDamage(Data.TickDamage, FDamageEvent(), nullptr, DamageInstigator.Get());

		if (SkillSystem)
		{
			// 이번 틱에 이 적이 맞은 것 자체가 '개별 타격' — 독립된 Context 사본으로 하위 Addon에 전달
			FSkillExecutionContext TickContext = BaseContext;
			TickContext.CurrentChainCount = 0;
			TickContext.CurrentDamage = Data.TickDamage;
			TickContext.HitActors.Add(Enemy);

			FHitResult TickHit;
			TickHit.ImpactPoint = Enemy->GetActorLocation();
			TickHit.HitObjectHandle = FActorInstanceHandle(Enemy);
			SkillSystem->ProcessAddons(SkillDef, TickContext, TickHit, AddonStartIndex);
		}
	}
}

