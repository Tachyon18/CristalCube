// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../CC_SkillAddonBase.h"
#include "../../CristalCubeStruct.h"
#include "CC_SigilAddon.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, meta = (DisplayName = "Sigil"))
class CRISTALCUBE_API UCC_SigilAddon : public UCC_SkillAddonBase
{
	GENERATED_BODY()
	
public:

	UCC_SigilAddon() { AddonType = ESkillAddonType::Sigil;  }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Addon|Sigil")
	FSigilAddonData Data;

	virtual void OnHit_Implementation(UCC_SkillSystem* SkillSystem, const FSkillDefinition& Skill,
		FSkillExecutionContext& Context, AActor* HitTarget, FVector HitLocation) override;

	virtual void ApplyModifier_Implementation(FName AttributeID, float ValuePerPoint) override;
};
