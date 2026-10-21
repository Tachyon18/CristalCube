// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CC_DamageNumberActor.generated.h"


class UTextRenderComponent;
class UCC_EffectorPoolSubsystem;

/**
 * 데미지 발생 지점에서 숫자를 띄웠다가 위로 떠오르면서 사라지는 연출용 액터.
 * 오늘 Effector 풀링에 쓴 UCC_EffectorPoolSubsystem을 그대로 재사용함 —
 * AActor* 기반 제네릭 풀이라 이 클래스도 SetOwningPool()만 구현하면 바로 풀링 대상이 됨.
 */
UCLASS()
class CRISTALCUBE_API ACC_DamageNumberActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACC_DamageNumberActor();

	// 풀에서 꺼내졌을 때(또는 새로 스폰됐을 때) 공통으로 호출 — 위치/텍스트/색상 리셋
	void Initialize(float DamageAmount, const FVector& SpawnLocation, bool bIsCritical = false);

	void SetOwningPool(UCC_EffectorPoolSubsystem* InPool);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	void Deactivate();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> TextRender;

	UPROPERTY()
	TObjectPtr<UCC_EffectorPoolSubsystem> OwningPool;

	UPROPERTY()
	TObjectPtr<APlayerCameraManager> CachedCameraManager = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "DamageNumber")
	float RiseSpeed = 80.0f;

	UPROPERTY(EditDefaultsOnly, Category = "DamageNumber")
	float LifeTime = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "DamageNumber")
	FColor NormalColor = FColor(0, 0, 0);

	UPROPERTY(EditDefaultsOnly, Category = "DamageNumber")
	FColor CriticalColor = FColor(255, 60, 60);

	float ElapsedTime = 0.0f;
};
