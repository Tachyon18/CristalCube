// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../CristalCubeStruct.h"
#include "CC_ShockwaveEffector.generated.h"

class UNiagaraComponent;
class UCC_SkillSystem;

/**
 * Shockwave Addon 전용 실행 액터.
 * OnHit 시점에 1회 스폰되어 반경을 시간에 따라 확장시키며, 확장 전선이
 * "지나가는 순간"에만 각 적을 1회 판정한다. ExpandDuration 종료 시 자멸.
 * 각 틱에 새로 맞춘 적 하나하나가 '개별 타격'으로 취급되어,
 * 자신만의 Context 사본으로 하위 Addon을 다시 발동시킨다.
 */
UCLASS()
class CRISTALCUBE_API ACC_ShockwaveEffector : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACC_ShockwaveEffector();

	/** 스폰 직후 UCC_ShockwaveAddon이 1회 호출 
	 *  InSkillSystem/InSkill/InContext/InStartIndex — 링이 새로 맞춘 적마다 뒤쪽 Addon을
	 *  연쇄시키기 위함. MagicMissile과 동일한 패턴(자기 자신은 제외, AddonIndex + 1부터). */

	void Initialize(FVector InOrigin, float InDamage, AActor* InInstigator, const FShockwaveAddonData& InData, AActor* InExcludedTarget, UCC_SkillSystem* InSkillSystem, const FSkillDefinition& InSkill, const FSkillExecutionContext& InContext, int32 InStartIndex);

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
	UNiagaraComponent* ShockwaveVFX = nullptr;

	FVector Origin = FVector::ZeroVector;
	float Damage = 0.0f;
	FShockwaveAddonData Data;

	TWeakObjectPtr<AActor> DamageInstigator;
	TWeakObjectPtr<AActor> ExcludedTarget;

	// 하위 Addon 전파용 — 스킬 시전 시점의 Context/SkillSystem 참조를 매 판정마다 재사용
	TWeakObjectPtr<UCC_SkillSystem> SkillSystemRef;
	FSkillDefinition SkillDef;
	FSkillExecutionContext BaseContext;
	int32 AddonStartIndex = 0;

	float CurrentRadius = 0.0f;
	float PreviousRadius = 0.0f;
	float ElapsedTime = 0.0f;

	TArray<TWeakObjectPtr<AActor>> AlreadyHit;

};
