// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../CristalCubeStruct.h"
#include "CC_StatusEffectComponent.generated.h"

/**
 * 경량 MVP — DoT만 지원. StatModifier(슬로우/취약 등)는
 * 실제 원소 로직이 붙을 때 같이 확장 예정.
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CRISTALCUBE_API UCC_StatusEffectComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCC_StatusEffectComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "StatusEffect")
	void ApplyDamageOverTime(FName EffectID, float TickDamage, float TotalDuration, float TickInterval, AActor* Instigator, bool bStackable = false, class UNiagaraSystem* TickEffect = nullptr);

	UFUNCTION(BlueprintCallable, Category = "StatusEffect")
	bool RemoveEffect(FName EffectID);

	UFUNCTION(BlueprintPure, Category = "StatusEffect")
	bool HasEffect(FName EffectID) const;

protected:
	UPROPERTY()
	TArray<FActiveStatusEffect> ActiveEffects;

	void ApplyTickDamage(const FActiveStatusEffect& Effect);

	class UNiagaraComponent* SpawnAttachedTickEffect(class UNiagaraSystem* Effect) const;
};
