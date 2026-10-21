// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_VisualComponent.h"
#include "Components/MeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "TimerManager.h"
#include "GameFramework/Actor.h"

// Sets default values for this component's properties
UCC_VisualComponent::UCC_VisualComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


void UCC_VisualComponent::SetPersistentVisualState(bool bPersistentState, UMeshComponent* TargetMesh)
{
	if (!TargetMesh) return;

	// 아우라
	if (bPersistentState)
	{
		if (PersistentAuraEffect && !PersistentAuraComponent)
		{
			PersistentAuraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
				PersistentAuraEffect, TargetMesh, NAME_None,
				FVector(0, 0, 50.f), FRotator::ZeroRotator,
				EAttachLocation::SnapToTarget, true);
		}
	}
	else
	{
		if (PersistentAuraComponent)
		{
			PersistentAuraComponent->DestroyComponent();
			PersistentAuraComponent = nullptr;
		}
	}

	// 색/발광 — MID 생성은 최초 1회만
	if (!PersistentMID)
	{
		UMaterialInterface* BaseMat = PersistentBaseMaterialOverride
			? PersistentBaseMaterialOverride
			: TargetMesh->GetMaterial(0);

		if (BaseMat)
		{
			PersistentMID = UMaterialInstanceDynamic::Create(BaseMat, this);
			TargetMesh->SetMaterial(0, PersistentMID);
		}
	}

	if (PersistentMID)
	{
		PersistentBlendTarget = bPersistentState ? 1.0f : 0.0f;

		AActor* Owner = GetOwner();
		UWorld* World = Owner ? Owner->GetWorld() : nullptr;

		if (World)
		{
			World->GetTimerManager().ClearTimer(PersistentBlendTimerHandle);
			World->GetTimerManager().SetTimer(
				PersistentBlendTimerHandle, this,
				&UCC_VisualComponent::UpdatePersistentBlend,
				0.05f, true);
		}
	}
}

// Called when the game starts
void UCC_VisualComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UCC_VisualComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UCC_VisualComponent::UpdatePersistentBlend()
{
	if (!PersistentMID) return;

	const float Step = (PersistentBlendTransitionSeconds > 0.f)
		? (0.05f / PersistentBlendTransitionSeconds)
		: 1.0f;

	PersistentBlendCurrent = FMath::FInterpConstantTo(
		PersistentBlendCurrent, PersistentBlendTarget, Step, 1.0f);

	PersistentMID->SetScalarParameterValue(TEXT("PersistentBlend"), PersistentBlendCurrent);

	if (FMath::IsNearlyEqual(PersistentBlendCurrent, PersistentBlendTarget, 0.01f))
	{
		PersistentBlendCurrent = PersistentBlendTarget;
		PersistentMID->SetScalarParameterValue(TEXT("PersistentBlend"), PersistentBlendCurrent);

		if (AActor* Owner = GetOwner())
		{
			Owner->GetWorldTimerManager().ClearTimer(PersistentBlendTimerHandle);
		}
	}
}

