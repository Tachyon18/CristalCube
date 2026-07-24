// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CC_VisualComponent.generated.h"

/**
 * Enemy의 부가적인 시각 상태(현재는 Persistent 전환 시 아우라 Niagara + 머티리얼 색/발광 Blend)를 담당.
 * ACC_EnemyCharacter(Skeletal)와 ACC_EnemyBase(Static Mesh) 양쪽에서 공유.
 * 이후 다른 시각 상태(피격 플래시, 버프 표시 등)가 필요해지면 이 컴포넌트에 메서드를 추가해 확장.
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CRISTALCUBE_API UCC_VisualComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCC_VisualComponent();

	/** Persistent 전환 시 붙는 아우라 이펙트 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual|Persistent")
	class UNiagaraSystem* PersistentAuraEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual|Persistent")
	class UMaterialInterface* PersistentBaseMaterialOverride = nullptr;

	/** PersistentBlend 파라미터를 목표치로 보간하는 시간(초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual|Persistent")
	float PersistentBlendTransitionSeconds = 0.5f;

	/** Persistent 상태 변경 — 대상 Mesh 컴포넌트(Skeletal/Static 무관)를 받아 Aura/MID/Blend 전부 처리 */
	UFUNCTION(BlueprintCallable, Category = "Visual|Persistent")
	void SetPersistentVisualState(bool bPersistentState, class UMeshComponent* TargetMesh);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:		

	UPROPERTY()
	class UNiagaraComponent* PersistentAuraComponent = nullptr;

	UPROPERTY()
	class UMaterialInstanceDynamic* PersistentMID = nullptr;

	FTimerHandle PersistentBlendTimerHandle;
	float PersistentBlendCurrent = 0.f;
	float PersistentBlendTarget = 0.f;

	void UpdatePersistentBlend();
};
