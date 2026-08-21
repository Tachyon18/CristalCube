// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../CC_SkillAddonBase.h"
#include "../../CristalCubeStruct.h"
#include "CC_MagicMissileAddon.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, meta = (DisplayName = "Magic Missile"))
class CRISTALCUBE_API UCC_MagicMissileAddon : public UCC_SkillAddonBase
{
	GENERATED_BODY()

public:

	UCC_MagicMissileAddon() { AddonType = ESkillAddonType::MagicMissile; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Addon|Homing")
	FMagicMissileAddonData Data;

	virtual void OnHit_Implementation(UCC_SkillSystem* SkillSystem, const FSkillDefinition& Skill,
		FSkillExecutionContext& Context, AActor* HitTarget, FVector HitLocation) override;

	virtual void ApplyModifier_Implementation(FName AttributeID, float ValuePerPoint) override;
};
