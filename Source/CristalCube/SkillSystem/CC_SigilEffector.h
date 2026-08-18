// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../CristalCubeStruct.h"
#include "CC_SigilEffector.generated.h"


class UNiagaraComponent;
class UCC_SkillSystem;

/**
 * Sigil Addon 전용 실행 액터. OnHit 시점에 1회 스폰되어 고정 반경 안의
 * 적에게 TickInterval마다 데미지를 주다가 Duration 종료 시 자멸.
 * 각 틱에 실제로 맞은 적 하나하나가 '개별 타격'으로 취급되어,
 * 자신만의 Context 사본으로 하위 Addon(예: Chain)을 다시 발동시킨다.
 */
UCLASS()
class CRISTALCUBE_API ACC_SigilEffector : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACC_SigilEffector();

	void Initialize(FVector InOrigin, AActor* InInstigator, const FSigilAddonData& InData, UCC_SkillSystem* InSkillSystem, const FSkillDefinition& InSkill, const FSkillExecutionContext& InContext, int32 InStartIndex);


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:

	UPROPERTY(VisibleAnywhere)
	class USceneComponent* Root = nullptr;

	UPROPERTY()
	UNiagaraComponent* SigilVFX = nullptr;

	FVector Origin = FVector::ZeroVector;
	FSigilAddonData Data;
	TWeakObjectPtr<AActor> DamageInstigator;

	// 하위 Addon 전파용 — 스킬 시전 시점의 Context/SkillSystem 참조를 틱마다 재사용
	TWeakObjectPtr<UCC_SkillSystem> SkillSystemRef;
	FSkillDefinition SkillDef;
	FSkillExecutionContext BaseContext;
	int32 AddonStartIndex = 0;

	FTimerHandle TickTimer;

	void ApplyTick();

};
