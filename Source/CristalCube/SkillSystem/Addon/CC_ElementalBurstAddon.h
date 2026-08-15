// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../CC_SkillAddonBase.h"
#include "../../CristalCubeStruct.h"
#include "CC_ElementalBurstAddon.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, meta = (DisplayName = "Elemental Burst"))
class CRISTALCUBE_API UCC_ElementalBurstAddon : public UCC_SkillAddonBase
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Addon|ElementalBurst")
	FElementalBurstAddonData Data;

	virtual void OnHit_Implementation(UCC_SkillSystem* SkillSystem, const FSkillDefinition& Skill,
		FSkillExecutionContext& Context, AActor* HitTarget, FVector HitLocation) override;

	
};
