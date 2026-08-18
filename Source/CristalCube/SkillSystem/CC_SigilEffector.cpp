// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_SigilEffector.h"
#include "../Gameplay/CC_EnemyAIInterface.h"
#include "CC_SkillSystem.h"
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

	if (Data.SigilEffect)
	{
		SigilVFX = UNiagaraFunctionLibrary::SpawnSystemAttached(
			Data.SigilEffect,
			Root,
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			false,  // bAutoDestroy — 액터 수명(Duration)에 종속
			true,   // bAutoActivate
			ENCPoolMethod::None,
			true    // bPreCullCheck
		);
	}

	if (Data.TickInterval > 0.0f)
	{
		GetWorldTimerManager().SetTimer(TickTimer, this, &ACC_SigilEffector::ApplyTick, Data.TickInterval, true);
	}

	SetLifeSpan(Data.Duration);  // 종료 시 자동 파괴 (Attach된 VFX도 함께 정리)
}

// Called when the game starts or when spawned
void ACC_SigilEffector::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ACC_SigilEffector::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ACC_SigilEffector::ApplyTick()
{
	TArray<AActor*> FoundEnemies;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("Enemy"), FoundEnemies);

	UCC_SkillSystem* SkillSystem = SkillSystemRef.Get();

	for (AActor* Enemy : FoundEnemies)
	{
		if (!Enemy || !IsValid(Enemy)) continue;

		if (Enemy->GetClass()->ImplementsInterface(UCC_EnemyAIInterface::StaticClass())
			&& ICC_EnemyAIInterface::Execute_GetIsFrozen(Enemy))
		{
			continue;
		}

		if (FVector::Dist(Origin, Enemy->GetActorLocation()) > Data.Radius) continue;

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

