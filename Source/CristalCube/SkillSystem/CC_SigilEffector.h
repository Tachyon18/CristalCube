// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../CristalCubeStruct.h"
#include "CC_SigilEffector.generated.h"


class UNiagaraComponent;

/**
 * Sigil Addon 전용 실행 액터. OnHit 시점에 1회 스폰되어 고정 반경 안의
 * 적에게 TickInterval마다 데미지를 주다가 Duration 종료 시 자멸.
 * Shockwave와 달리 반경이 확장되지 않아 매 프레임 Tick이 필요 없음 —
 * 반복 타이머로만 동작해 비용을 최소화.
 */
UCLASS()
class CRISTALCUBE_API ACC_SigilEffector : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACC_SigilEffector();

	void Initialize(FVector InOrigin, AActor* InInstigator, const FSigilAddonData& InData);


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

	FTimerHandle TickTimer;

	void ApplyTick();

};
