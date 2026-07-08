// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"

/**
 * 스킬(Projectile Effector 등)에 의해 "맞을 수 있는" 대상이 되기 위한
 * 콜리전 설정을 한 곳에서 관리하는 순수 유틸리티.
 * 어떤 언리얼 클래스도 상속하지 않음 — 그냥 정적 함수를 담아두는 평범한 C++ 클래스.
 *
 * 사용법: Player/Enemy 계열 생성자에서, 자신의 루트 콜리전 컴포넌트를 "넘겨서" 호출.
 *   FCC_CollisionHelper::ConfigureAsSkillHittable(GetCapsuleComponent());
 */

class CRISTALCUBE_API FCC_CollisionHelper
{
public:
	static void ConfigureAsSkillHittable(UPrimitiveComponent* Comp)
	{
		if (!Comp)
		{
			return;
		}

		Comp->SetCollisionObjectType(ECC_Visibility);
		Comp->SetCollisionResponseToChannel(ECC_Visibility, ECR_Overlap);
		Comp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		Comp->SetGenerateOverlapEvents(true);
	}
	
	/**
	 * Enemy CapsuleComponent 실제 콜리전 실루엣 디버그 드로우.
	 * 콘솔 토글: "CC.ShowEnemyCapsules 1" (0=끄기)
	 * 색상: 초록=평상시, 하늘색=Frozen, 보라=Persistent
	 * CVar가 꺼져 있으면 즉시 return하므로 평상시 비용은 거의 없음.
	 */
	static void DrawEnemyCapsuleDebug(
		const UWorld* World,
		const class UCapsuleComponent* Capsule,
		bool bIsFrozen,
		bool bIsPersistent);
};
