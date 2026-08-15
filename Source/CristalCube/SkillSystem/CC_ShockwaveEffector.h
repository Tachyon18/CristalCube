// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../CristalCubeStruct.h"
#include "CC_ShockwaveEffector.generated.h"

class UNiagaraComponent;

/**
 * Shockwave Addon 전용 실행 액터.
 * OnHit 시점에 1회 스폰되어 반경을 시간에 따라 확장시키며, 확장 전선이
 * "지나가는 순간"에만 각 적을 1회 판정한다. ExpandDuration 종료 시 자멸.
 */
UCLASS()
class CRISTALCUBE_API ACC_ShockwaveEffector : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACC_ShockwaveEffector();

	/** 스폰 직후 UCC_ShockwaveAddon이 1회 호출 */
	void Initialize(FVector InOrigin, float InDamage, AActor* InInstigator,
		const FShockwaveAddonData& InData, AActor* InExcludedTarget);

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

	float CurrentRadius = 0.0f;
	float PreviousRadius = 0.0f;
	float ElapsedTime = 0.0f;

	TArray<TWeakObjectPtr<AActor>> AlreadyHit;

};
