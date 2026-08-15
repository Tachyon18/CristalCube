// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CC_SkillEmpowerComponent.generated.h"


USTRUCT()
struct FActiveEmpowerStack
{
	GENERATED_BODY()

	UPROPERTY()
	FName SkillID = NAME_None;

	UPROPERTY()
	int32 StackCount = 0;

	UPROPERTY()
	float RemainingDuration = 0.0f;
};

/**
 * 시전자가 스킬별로 쌓은 "연속 사용 스택"을 관리. SkillID 키.
 * UCC_ElementalStatusComponent(대상 쪽 상태)와 대칭 구조 — 이쪽은 시전자 쪽 상태.
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CRISTALCUBE_API UCC_SkillEmpowerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCC_SkillEmpowerComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 스택 1 추가(또는 만료됐으면 1로 리셋) + Duration Refresh, 결과 데미지 배율 반환
	UFUNCTION(BlueprintCallable, Category = "SkillEmpower")
	float AddStackAndGetMultiplier(FName SkillID, float DamagePerStack, int32 MaxStacks, float Duration);

private:
	UPROPERTY()
	TArray<FActiveEmpowerStack> ActiveStacks;
};
