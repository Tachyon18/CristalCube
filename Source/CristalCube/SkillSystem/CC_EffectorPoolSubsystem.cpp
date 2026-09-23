// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_EffectorPoolSubsystem.h"

AActor* UCC_EffectorPoolSubsystem::AcquireEffector(TSubclassOf<AActor> EffectorClass)
{
	if (!EffectorClass)
	{
		return nullptr;
	}

	TArray<TObjectPtr<AActor>>& FreeList = FreeActors.FindOrAdd(EffectorClass).Actors;

	while (FreeList.Num() > 0)
	{
		AActor* Effector = FreeList.Pop(EAllowShrinking::No);
		if (IsValid(Effector))
		{
			return Effector;
		}
	}

	return GetWorld()->SpawnActor<AActor>(EffectorClass);
}

void UCC_EffectorPoolSubsystem::ReleaseEffector(AActor* Effector)
{
	if (!IsValid(Effector))
	{
		return;
	}

	FreeActors.FindOrAdd(Effector->GetClass()).Actors.Add(Effector);
}
