// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_ElementalStatusComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CC_EnemyMovementComponent.h"
#include "Engine/DamageEvents.h"

// Sets default values for this component's properties
UCC_ElementalStatusComponent::UCC_ElementalStatusComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	// ...
}


// Called when the game starts
void UCC_ElementalStatusComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UCC_ElementalStatusComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);


	if (ActiveElements.Num() == 0) return;

	AActor* Owner = GetOwner();
	TArray<int32> ExpiredIndices;

	for (int32 i = 0; i < ActiveElements.Num(); ++i)
	{
		FActiveElementalStatus& Status = ActiveElements[i];
		Status.RemainingDuration -= DeltaTime;
		Status.TimeSinceLastTick += DeltaTime;

		// 원소별 기본(baseline) 효과 — 반응/조합 없는 단순 적용
		switch (Status.ElementType)
		{
		case ESkillElementType::Red:
			// 데미지 없음 — 추가 데미지는 부여 시점에(CC_ElementalApplyAddon에서) 1회 적용됨
			break;

		case ESkillElementType::Green:
			// 데미지/회복 없음 — 받는피해·공격력 배율은 GetIncomingDamageMultiplier()/
			// GetOutgoingDamageMultiplier()로 그때그때 조회됨
			break;

		case ESkillElementType::Blue:
			// 데미지 없음 — 슬로우는 ApplyElement()에서 부여 시점에 1회 적용됨
			break;
		default:
			break;
		}

		if (Status.RemainingDuration <= 0.0f)
		{
			ExpiredIndices.Add(i);
		}
	}

	for (int32 i = ExpiredIndices.Num() - 1; i >= 0; --i)
	{
		const int32 Index = ExpiredIndices[i];

		if (ActiveElements[Index].ElementType == ESkillElementType::Blue)
		{
			RemoveBlueSlow();
		}

		if (UNiagaraComponent* VFXComp = ActiveElements[Index].ActiveVFXComponent.Get())
		{
			VFXComp->DestroyComponent();
		}

		ActiveElements.RemoveAt(Index);
	}

	if (ActiveElements.Num() == 0)
	{
		SetComponentTickEnabled(false);
	}
}

void UCC_ElementalStatusComponent::ApplyElement(ESkillElementType ElementType, int32 StackAmount, int32 MaxStacks, float Duration, AActor* Instigator, UNiagaraSystem* ApplyEffect)
{
	if (ElementType == ESkillElementType::None) return;

	for (FActiveElementalStatus& Existing : ActiveElements)
	{
		if (Existing.ElementType == ElementType)
		{
			Existing.StackCount = FMath::Min(Existing.StackCount + StackAmount, MaxStacks);
			Existing.RemainingDuration = Duration;
			Existing.Instigator = Instigator;

			if (!Existing.ActiveVFXComponent.IsValid() && ApplyEffect)
			{
				Existing.ActiveVFXComponent = SpawnAttachedElementEffect(ApplyEffect);
			}
			return;
		}
	}

	FActiveElementalStatus NewStatus;
	NewStatus.ElementType = ElementType;
	NewStatus.StackCount = FMath::Min(StackAmount, MaxStacks);
	NewStatus.RemainingDuration = Duration;
	NewStatus.Instigator = Instigator;
	NewStatus.ActiveVFXComponent = SpawnAttachedElementEffect(ApplyEffect);
	ActiveElements.Add(NewStatus);
	SetComponentTickEnabled(true);

	if (ElementType == ESkillElementType::Blue)
	{
		ApplyBlueSlow();
	}
}

bool UCC_ElementalStatusComponent::GetActiveElement(ESkillElementType ElementType, int32& OutStackCount) const
{
	for (const FActiveElementalStatus& Status : ActiveElements)
	{
		if (Status.ElementType == ElementType)
		{
			OutStackCount = Status.StackCount;
			return true;
		}
	}
	OutStackCount = 0;
	return false;
}

void UCC_ElementalStatusComponent::ConsumeElement(ESkillElementType ElementType)
{
	for (int32 i = ActiveElements.Num() - 1; i >= 0; --i)
	{
		if (ActiveElements[i].ElementType == ElementType)
		{
			if (ElementType == ESkillElementType::Blue)
			{
				RemoveBlueSlow();
			}

			if (UNiagaraComponent* VFXComp = ActiveElements[i].ActiveVFXComponent.Get())
			{
				VFXComp->DestroyComponent();
			}
			ActiveElements.RemoveAt(i);

			if (ActiveElements.Num() == 0)
			{
				SetComponentTickEnabled(false);
			}
			return;
		}
	}
}

void UCC_ElementalStatusComponent::ApplyBlueSlow()
{
	AActor* Owner = GetOwner();
	if (!Owner || bSpeedSlowed) return;

	if (ACharacter* Character = Cast<ACharacter>(Owner))
	{
		if (UCharacterMovementComponent* CMC = Character->GetCharacterMovement())
		{
			SavedOriginalSpeed = CMC->MaxWalkSpeed;
			CMC->MaxWalkSpeed *= BlueData.SlowMultiplier;
			bSpeedSlowed = true;
		}
	}
	else if (UCC_EnemyMovementComponent* EnemyMovement = Owner->FindComponentByClass<UCC_EnemyMovementComponent>())
	{
		SavedOriginalSpeed = EnemyMovement->MaxSpeed;
		EnemyMovement->MaxSpeed *= BlueData.SlowMultiplier;
		bSpeedSlowed = true;
	}
}

void UCC_ElementalStatusComponent::RemoveBlueSlow()
{
	AActor* Owner = GetOwner();
	if (!Owner || !bSpeedSlowed || SavedOriginalSpeed < 0.0f) return;

	if (ACharacter* Character = Cast<ACharacter>(Owner))
	{
		if (UCharacterMovementComponent* CMC = Character->GetCharacterMovement())
		{
			CMC->MaxWalkSpeed = SavedOriginalSpeed;
		}
	}
	else if (UCC_EnemyMovementComponent* EnemyMovement = Owner->FindComponentByClass<UCC_EnemyMovementComponent>())
	{
		EnemyMovement->MaxSpeed = SavedOriginalSpeed;
	}

	bSpeedSlowed = false;
	SavedOriginalSpeed = -1.0f;
}

float UCC_ElementalStatusComponent::GetIncomingDamageMultiplier() const
{
	int32 StackCount = 0;
	if (GetActiveElement(ESkillElementType::Green, StackCount))
	{
		return GreenData.IncomingDamageMultiplier;
	}
	return 1.0f;
}

float UCC_ElementalStatusComponent::GetOutgoingDamageMultiplier() const
{
	int32 StackCount = 0;
	if (GetActiveElement(ESkillElementType::Green, StackCount))
	{
		return GreenData.OutgoingDamageMultiplier;
	}
	return 1.0f;
}

UNiagaraComponent* UCC_ElementalStatusComponent::SpawnAttachedElementEffect(UNiagaraSystem* Effect) const
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
		false,  // bAutoDestroy — 수명은 컴포넌트가 직접 관리
		true,   // bAutoActivate
		ENCPoolMethod::None,  // 수명 직접 관리하므로 풀링 비사용 (DoT 때 겪은 문제 재발 방지)
		true    // bPreCullCheck
	);
}

