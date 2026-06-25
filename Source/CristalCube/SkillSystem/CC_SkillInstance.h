// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CC_SkillInstance.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UCC_SkillInstance : public UInterface
{
	GENERATED_BODY()
};

/**
 * 지속성을 가지는(한 프레임 이상 살아있는) 스킬 인스턴스 액터가 구현하는 인터페이스.
 * Projectile/Rainfall Effector뿐 아니라, 향후 추가될 Channeling/지속형 Area 등
 * 새로운 스킬 인스턴스도 이것만 구현하면 SkillSystem의 큐브 전환 정리 로직에
 * 자동으로 편입됨 — SkillSystem은 구체 타입을 몰라도 됨.
 */
class CRISTALCUBE_API ICC_SkillInstance
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	/** true면 큐브 전환 시에도 소멸하지 않고 유지됨 ('추가적인 특징') */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Skill|Instance")
	bool ShouldPersistThroughCubeTransition() const;
	virtual bool ShouldPersistThroughCubeTransition_Implementation() const = 0;

	/** 큐브 전환으로 제거될 때 호출. 기본은 Destroy()이겠지만,
 *  추후 페이드아웃 등 다른 정리가 필요한 인스턴스는 이 부분만 다르게 구현하면 됨. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Skill|Instance")
	void OnRemovedByCubeTransition();
	virtual void OnRemovedByCubeTransition_Implementation() = 0;
};
