// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_CollisionHelper.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"

static TAutoConsoleVariable<int32> CVarShowEnemyCapsules(
	TEXT("CC.ShowEnemyCapsules"),
	0,
	TEXT("0 = 끄기, 1 = 켜기. Enemy CapsuleComponent 실제 콜리전 실루엣을 매 프레임 디버그로 그림.\n")
	TEXT("색상: 초록=평상시, 하늘색=Frozen, 보라=Persistent"),
	ECVF_Default
);

void FCC_CollisionHelper::DrawEnemyCapsuleDebug(const UWorld* World, const UCapsuleComponent* Capsule, bool bIsFrozen, bool bIsPersistent)
{
	if (!World || !Capsule) return;
	if (CVarShowEnemyCapsules.GetValueOnGameThread() <= 0) return;

	const FColor Color = bIsFrozen ? FColor::Cyan
		: bIsPersistent ? FColor::Magenta
		: FColor::Green;

	DrawDebugCapsule(
		World,
		Capsule->GetComponentLocation(),
		Capsule->GetScaledCapsuleHalfHeight(),
		Capsule->GetScaledCapsuleRadius(),
		Capsule->GetComponentQuat(),
		Color,
		false,   // bPersistentLines
		0.0f,    // LifeTime — 매 프레임/타이머 재호출로 리프레시되므로 짧게
		0,
		1.5f
	);
}
