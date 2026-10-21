// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_StatusEffectComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

// Sets default values for this component's properties
UCC_StatusEffectComponent::UCC_StatusEffectComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;

	// ...
}


// Called when the game starts
void UCC_StatusEffectComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UCC_StatusEffectComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (ActiveEffects.Num() == 0)
    {
        return;
    }

    TArray<int32> ExpiredIndices;

    for (int32 i = 0; i < ActiveEffects.Num(); ++i)
    {
        FActiveStatusEffect& Effect = ActiveEffects[i];
        Effect.RemainingDuration -= DeltaTime;

        Effect.TimeSinceLastTick += DeltaTime;
        if (Effect.TimeSinceLastTick >= Effect.TickInterval)
        {
            Effect.TimeSinceLastTick -= Effect.TickInterval;
            ApplyTickDamage(Effect);
        }

        if (Effect.RemainingDuration <= 0.0f)
        {
            ExpiredIndices.Add(i);
        }
    }

    for (int32 i = ExpiredIndices.Num() - 1; i >= 0; --i)
    {
        const int32 Index = ExpiredIndices[i];

        if (UNiagaraComponent* VFXComp = ActiveEffects[Index].ActiveVFXComponent.Get())
        {
            VFXComp->DestroyComponent();
        }

        ActiveEffects.RemoveAt(Index);
    }

    if (ActiveEffects.Num() == 0)
    {
        SetComponentTickEnabled(false);
    }
}

void UCC_StatusEffectComponent::ApplyDamageOverTime(FName EffectID, float TickDamage, float TotalDuration, float TickInterval, AActor* Instigator, bool bStackable, UNiagaraSystem* TickEffect)
{
    if (!bStackable)
    {
        for (FActiveStatusEffect& Existing : ActiveEffects)
        {
            if (Existing.EffectID == EffectID)
            {
                // 갱신(Refresh) — 지속시간만 연장. VFX는 이미 재생 중이면 그대로 둠(재스폰 안 함).
                Existing.RemainingDuration = TotalDuration;

                if (!Existing.ActiveVFXComponent.IsValid() && TickEffect)
                {
                    Existing.ActiveVFXComponent = SpawnAttachedTickEffect(TickEffect);
                }
                return;
            }
        }
    }

    FActiveStatusEffect NewEffect;
    NewEffect.EffectID = EffectID;
    NewEffect.TickDamage = TickDamage;
    NewEffect.TickInterval = TickInterval;
    NewEffect.RemainingDuration = TotalDuration;
    NewEffect.bStackable = bStackable;
    NewEffect.Instigator = Instigator;
    NewEffect.ActiveVFXComponent = SpawnAttachedTickEffect(TickEffect);
    ActiveEffects.Add(NewEffect);
    SetComponentTickEnabled(true);
}

bool UCC_StatusEffectComponent::RemoveEffect(FName EffectID)
{
    bool bRemoved = false;

    for (int32 i = ActiveEffects.Num() - 1; i >= 0; --i)
    {
        if (ActiveEffects[i].EffectID == EffectID)
        {
            if (UNiagaraComponent* VFXComp = ActiveEffects[i].ActiveVFXComponent.Get())
            {
                VFXComp->DestroyComponent();
            }

            ActiveEffects.RemoveAt(i);
            bRemoved = true;
        }
    }

    if (ActiveEffects.Num() == 0)
    {
        SetComponentTickEnabled(false);
    }


    return bRemoved;
}

bool UCC_StatusEffectComponent::HasEffect(FName EffectID) const
{
    return ActiveEffects.ContainsByPredicate([EffectID](const FActiveStatusEffect& E)
        {
            return E.EffectID == EffectID;
        });
}

void UCC_StatusEffectComponent::ApplyTickDamage(const FActiveStatusEffect& Effect)
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }

    UGameplayStatics::ApplyDamage(Owner, Effect.TickDamage, nullptr, Effect.Instigator.Get(), nullptr);

}

UNiagaraComponent* UCC_StatusEffectComponent::SpawnAttachedTickEffect(UNiagaraSystem* Effect) const
{
    AActor* Owner = GetOwner();
    if (!Effect || !Owner || !Owner->GetRootComponent())
    {
        return nullptr;
    }

    return UNiagaraFunctionLibrary::SpawnSystemAttached(
        Effect,
        Owner->GetRootComponent(),
        NAME_None,
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        EAttachLocation::SnapToTarget,
        false,  // bAutoDestroy — 지속 이펙트라 수명은 위에서 직접 관리
        true,   // bAutoActivate
        ENCPoolMethod::None,
        true    // bPreCullCheck
    );
}

