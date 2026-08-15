// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_SkillAddonBase.h"
#include "../Gameplay/CC_EnemyAIInterface.h"
#include "../Gameplay/CC_StatusEffectComponent.h"

bool UCC_SkillAddonBase::IsValidHitTarget(AActor* Target) const
{
    if (!Target || !IsValid(Target))
    {
        return false;
    }

    if (!Target->ActorHasTag(FName("Enemy")))
    {
        return false;
    }

    if (Target->GetClass()->ImplementsInterface(UCC_EnemyAIInterface::StaticClass())
        && ICC_EnemyAIInterface::Execute_GetIsFrozen(Target))
    {
        return false;
    }

    return true;
}

UCC_StatusEffectComponent* UCC_SkillAddonBase::GetStatusComponent(AActor* Target) const
{
    return Target ? Target->FindComponentByClass<UCC_StatusEffectComponent>() : nullptr;
}
