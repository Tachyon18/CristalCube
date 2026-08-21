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
	// 원소별 기본 수치 — 축마다 실제로 다루는 값의 종류가 달라서 필드는 다르지만,
	// 이름은 FElement[색]Data로 통일해서 어디서든 예측 가능하게 둠
	//==========================================================================

	// Red는 이 컴포넌트에 데이터를 안 둠 — FElementRedData는 CC_ElementalApplyAddon 쪽
	// FElementalApplyAddonData.RedData에 있음(일회성 프록이라 스킬 데이터가 있는 곳에서 계산).
	// ActiveElements엔 지속시간/스택 추적용으로만 등록됨(Burst 소비 대상 조회 등).

	UPROPERTY(EditDefaultsOnly, Category = "ElementalStatus|Blue")
	FElementBlueData BlueData;

	UPROPERTY(EditDefaultsOnly, Category = "ElementalStatus|Green")
	FElementGreenData GreenData;

	// 현재 Blue로 감속 중인지 + 복구용 원본 속도
	bool bSpeedSlowed = false;
	float SavedOriginalSpeed = -1.0f;

	void ApplyBlueSlow();
	void RemoveBlueSlow();

public:
	// 대상이 Green으로 "받는 피해"가 늘어나 있는지 조회 — UCC_SkillSystem::ApplyDamage()에서 사용
	float GetIncomingDamageMultiplier() const;

	// 자기 자신이 Green으로 "가하는 공격력"이 줄어 있는지 조회 — Enemy 공격 판정에서 사용
	float GetOutgoingDamageMultiplier() const;

protected:
	UPROPERTY()
	TArray<FActiveElementalStatus> ActiveElements;

	// 원소 하나 스폰 헬퍼 (DoT의 SpawnAttachedTickEffect와 동일 패턴)
	class UNiagaraComponent* SpawnAttachedElementEffect(UNiagaraSystem* Effect) const;

};
