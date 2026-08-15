// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_SkillEmpowerComponent.h"

// Sets default values for this component's properties
UCC_SkillEmpowerComponent::UCC_SkillEmpowerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	// ...
}


// Called when the game starts
void UCC_SkillEmpowerComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UCC_SkillEmpowerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	for (int32 i = ActiveStacks.Num() - 1; i >= 0; --i)
	{
		ActiveStacks[i].RemainingDuration -= DeltaTime;
		if (ActiveStacks[i].RemainingDuration <= 0.0f)
		{
			ActiveStacks.RemoveAt(i);
		}
	}

	if (ActiveStacks.Num() == 0)
	{
		SetComponentTickEnabled(false);
	}
}

float UCC_SkillEmpowerComponent::AddStackAndGetMultiplier(FName SkillID, float DamagePerStack, int32 MaxStacks, float Duration)
{
	for (FActiveEmpowerStack& Existing : ActiveStacks)
	{
		if (Existing.SkillID == SkillID)
		{
			Existing.StackCount = FMath::Min(Existing.StackCount + 1, MaxStacks);
			Existing.RemainingDuration = Duration;
			return 1.0f + DamagePerStack * Existing.StackCount;
		}
	}

	FActiveEmpowerStack NewStack;
	NewStack.SkillID = SkillID;
	NewStack.StackCount = 1;
	NewStack.RemainingDuration = Duration;
	ActiveStacks.Add(NewStack);
	SetComponentTickEnabled(true);
	return 1.0f + DamagePerStack;
}

