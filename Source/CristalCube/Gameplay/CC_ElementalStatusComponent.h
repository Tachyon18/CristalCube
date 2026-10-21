// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../CristalCubeStruct.h"
#include "CC_ElementalStatusComponent.generated.h"

// Safety bounds for elemental effect values - worst-case guards only, NOT balance.
// Balance lives in DA presets (FElementalApplyAddonData) and DT ValuePerPoint.
// Shared by UCC_ElementalStatusComponent (apply time) and UCC_ElementalApplyAddon (upgrade time).
struct FCCElementalLimits
{
	// Blue: 0 would freeze the target completely, > 1 would speed it up
	static constexpr float BlueMultiplierMin = 0.05f;
	static constexpr float BlueMultiplierMax = 1.0f;

	// Green incoming: < 1 would turn "takes more damage" into damage reduction; upper bound stops runaway stacking
	static constexpr float GreenIncomingMin = 1.0f;
	static constexpr float GreenIncomingMax = 10.0f;

	// Green outgoing / heal received: < 0 would invert the effect, > 1 would buff the target
	static constexpr float GreenReductionMin = 0.0f;
	static constexpr float GreenReductionMax = 1.0f;
};

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
	// Uses this component's BlueData/GreenData as default values.
	// 원소 부여/갱신 — 이미 걸려있으면 스택 추가(MaxStacks 제한) + Duration Refresh
	UFUNCTION(BlueprintCallable, Category = "ElementalStatus")
	void ApplyElement(ESkillElementType ElementType, int32 StackAmount, int32 MaxStacks, float Duration, AActor* Instigator, UNiagaraSystem* ApplyEffect = nullptr);

	// Same as ApplyElement, with explicit per-application values (CC_ElementalApplyAddon path).
	// Re-apply keeps the stronger value: Blue = lower multipliers, Green = higher incoming / lower outgoing.
	UFUNCTION(BlueprintCallable, Category = "ElementalStatus")
	void ApplyElementWithData(ESkillElementType ElementType, int32 StackAmount, int32 MaxStacks, float Duration, AActor* Instigator,
		UNiagaraSystem* ApplyEffect, const FElementBlueData& InBlueData, const FElementGreenData& InGreenData);

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

	// Default values - used only when ApplyElement() is called without explicit data
	UPROPERTY(EditDefaultsOnly, Category = "ElementalStatus|Blue")
	FElementBlueData BlueData;

	UPROPERTY(EditDefaultsOnly, Category = "ElementalStatus|Green")
	FElementGreenData GreenData;

	// 현재 Blue로 감속 중인지 + 복구용 원본 속도
	bool bSpeedSlowed = false;
	float SavedOriginalSpeed = -1.0f;

	void ApplyBlueSlow(float SlowMultiplier);
	void RemoveBlueSlow();

public:
	// Blue attack speed multiplier (1.0 if none) - read by ACC_EnemyBase::GetEffectiveAttackCooldown()
	float GetAttackSpeedMultiplier() const;

	// Green incoming damage multiplier (1.0 if none) - read by ACC_EnemyBase::TakeDamage()
	float GetIncomingDamageMultiplier() const;

	// Green outgoing damage multiplier (1.0 if none) - read by ACC_EnemyBase::GetEffectiveAttackDamage()
	float GetOutgoingDamageMultiplier() const;

protected:
	UPROPERTY()
	TArray<FActiveElementalStatus> ActiveElements;

	const FActiveElementalStatus* FindActiveElement(ESkillElementType ElementType) const;

	// 원소 하나 스폰 헬퍼 (DoT의 SpawnAttachedTickEffect와 동일 패턴)
	class UNiagaraComponent* SpawnAttachedElementEffect(UNiagaraSystem* Effect, ESkillElementType ElementType) const;

};
