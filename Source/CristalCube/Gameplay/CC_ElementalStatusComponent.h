// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../CristalCubeStruct.h"
#include "CC_ElementalStatusComponent.generated.h"


/**
 * 대상에게 걸린 "원소 상태"를 원소 종류(ElementType) 단위로 관리.
 * UCC_StatusEffectComponent(SkillID 키, DoT 전용)와는 별개 —
 * 어느 스킬이 걸었든 "지금 이 원소가 걸려있는가"만 신경 쓰는 크로스-스킬 조회가 목적.
 *
 * 원소별 기본 효과(TickComponent 내부)는 baseline 수준만 구현 — 반응(Fire+Ice 등)
 * 같은 정교한 원소 엔진은 MVP/출시 이후 별도 작업.
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CRISTALCUBE_API UCC_ElementalStatusComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCC_ElementalStatusComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	// 원소 부여/갱신 — 이미 걸려있으면 스택 추가(MaxStacks 제한) + Duration Refresh
	UFUNCTION(BlueprintCallable, Category = "ElementalStatus")
	void ApplyElement(ESkillElementType ElementType, int32 StackAmount, int32 MaxStacks, float Duration, AActor* Instigator, UNiagaraSystem* ApplyEffect = nullptr);

	// 특정 원소가 걸려있는지 + 현재 스택 수 (Burst가 조회용으로 사용)
	UFUNCTION(BlueprintCallable, Category = "ElementalStatus")
	bool GetActiveElement(ESkillElementType ElementType, int32& OutStackCount) const;

	// Burst 소비 — 해당 원소 상태 제거 (VFX도 함께 정리)
	UFUNCTION(BlueprintCallable, Category = "ElementalStatus")
	void ConsumeElement(ESkillElementType ElementType);

protected:

	//==========================================================================
	// 원소별 기본 수치 (baseline — 반응/조합 없는 단순 적용)
	//==========================================================================

	UPROPERTY(EditDefaultsOnly, Category = "ElementalStatus|Fire")
	float FireTickDamage = 4.0f;

	UPROPERTY(EditDefaultsOnly, Category = "ElementalStatus|Fire")
	float FireTickInterval = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "ElementalStatus|Poison")
	float PoisonTickDamagePerStack = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category = "ElementalStatus|Poison")
	float PoisonTickInterval = 1.0f;

	// TODO(추후 확장): 감전 확산/기절 등 정교한 Lightning 반응은 여기 대신
	// 별도 원소 반응 시스템에서 처리 — 지금은 약한 틱뎀만
	UPROPERTY(EditDefaultsOnly, Category = "ElementalStatus|Lightning")
	float LightningTickDamage = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category = "ElementalStatus|Lightning")
	float LightningTickInterval = 0.4f;

	UPROPERTY(EditDefaultsOnly, Category = "ElementalStatus|Ice")
	float IceSlowMultiplier = 0.5f;

	// 현재 Ice로 감속 중인지 + 복구용 원본 속도
	bool bSpeedSlowed = false;
	float SavedOriginalSpeed = -1.0f;

	void ApplyIceSlow();
	void RemoveIceSlow();

	UPROPERTY()
	TArray<FActiveElementalStatus> ActiveElements;

	// 원소 하나 스폰 헬퍼 (DoT의 SpawnAttachedTickEffect와 동일 패턴)
	class UNiagaraComponent* SpawnAttachedElementEffect(UNiagaraSystem* Effect) const;

};
