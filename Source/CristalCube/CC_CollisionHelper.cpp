// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_CollisionHelper.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
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
		0.1f,    // LifeTime — 매 프레임/타이머 재호출로 리프레시되므로 짧게
		0,
		1.5f
	);
}

static TAutoConsoleVariable<int32> CVarShowSkillCollision(
	TEXT("CC.ShowSkillCollision"),
	0,
	TEXT("0 = 끄기, 1 = 켜기. 스킬 Effector의 실제 콜리전 구체와 오버랩 지점을 디버그로 그림.\n")
	TEXT("구체: 노랑=판정 중, 회색=판정 종료(Drain). 지점: 빨강=적중, 하늘색=Frozen 제외, 흰색=적 아님"),
	ECVF_Default
);

bool FCC_CollisionHelper::IsSkillCollisionDebugEnabled()
{
	return CVarShowSkillCollision.GetValueOnGameThread() > 0;
}

void FCC_CollisionHelper::DrawSkillSphereDebug(const UWorld* World, const USphereComponent* Sphere, bool bGameplayActive)
{
	if (!World || !Sphere || !IsSkillCollisionDebugEnabled()) return;

	DrawDebugSphere(
		World,
		Sphere->GetComponentLocation(),
		Sphere->GetScaledSphereRadius(),   // BP 값이 아니라 런타임 실제 반경
		12,
		bGameplayActive ? FColor::Yellow : FColor(128, 128, 128),
		false,
		0.06f,   // 0.05초 타이머로 다시 그리므로 살짝 길게
		0,
		1.0f
	);
}

void FCC_CollisionHelper::DrawSkillOverlapDebug(const UWorld* World, const FVector& Point, const FColor& Color, const FString& Label)
{
	if (!World || !IsSkillCollisionDebugEnabled()) return;

	DrawDebugPoint(World, Point, 12.0f, Color, false, 2.0f);

	if (!Label.IsEmpty())
	{
		DrawDebugString(World, Point + FVector(0.0f, 0.0f, 30.0f), Label, nullptr, Color, 2.0f, false, 1.0f);
	}
}