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
	//==========================================================================
	// 오브젝트 채널 — DefaultEngine.ini에 이미 정의된 커스텀 채널 재사용
	//   GameTraceChannel1 = "Enemy"      (기본 응답 Overlap)
	//   GameTraceChannel2 = "Projectile" (기본 응답 Ignore → 지형으로 "지정"된 것만 투사체를 막음)
	// Project Settings에서 채널 순서를 바꾸면 이 매핑도 같이 바꿔야 함.
	// 생성자 값은 BP Collision Presets에 덮이므로, 각 호출부는 런타임(BeginPlay/발사/초기화)에도 다시 적용함.
	//==========================================================================
	static constexpr ECollisionChannel ECC_CC_Enemy = ECC_GameTraceChannel1;
	static constexpr ECollisionChannel ECC_CC_Projectile = ECC_GameTraceChannel2;

	// 적 피격 판정체(캡슐) — 오브젝트 타입 Enemy, 투사체에 Overlap, 플레이어(Pawn)와는 몸으로 막힘.
	// (Beam은 오브젝트 타입 쿼리로 Enemy를 찾으므로 Pawn=Block이어도 관통 유지)
	static void ConfigureAsEnemyHitbox(UPrimitiveComponent* Comp)
	{
		if (!Comp) return;
		Comp->SetCollisionObjectType(ECC_CC_Enemy);
		Comp->SetCollisionResponseToChannel(ECC_CC_Projectile, ECR_Overlap);
		Comp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		Comp->SetGenerateOverlapEvents(true);
	}

	// 플레이어 피격 판정체(캡슐) — 오브젝트 타입 Pawn, 적 투사체에 Overlap.
	static void ConfigureAsPlayerHitbox(UPrimitiveComponent* Comp)
	{
		if (!Comp) return;
		Comp->SetCollisionObjectType(ECC_Pawn);
		Comp->SetCollisionResponseToChannel(ECC_CC_Projectile, ECR_Overlap);
		Comp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		Comp->SetCollisionResponseToChannel(ECC_CC_Enemy, ECR_Block);    // 적과 몸으로 막힘
		Comp->SetGenerateOverlapEvents(true);
	}

	// 플레이어 스킬 투사체 판정 구체 — 적만 Overlap, 지형(Static/Dynamic 모두)에는 Block, 나머지 무시.
	// 실제로 막히는 건 상대가 Projectile 채널을 Block하는 경우뿐(= ConfigureAsSkillTerrain으로 지정된 지형).
	static void ConfigureAsSkillProjectile(UPrimitiveComponent* Comp)
	{
		if (!Comp) return;
		Comp->SetCollisionObjectType(ECC_CC_Projectile);
		Comp->SetCollisionResponseToAllChannels(ECR_Ignore);
		Comp->SetCollisionResponseToChannel(ECC_CC_Enemy, ECR_Overlap);
		Comp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		Comp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
		Comp->SetGenerateOverlapEvents(true);
	}

	// 적 투사체 판정 구체 — 스킬 투사체와 같은 문법: 대상(플레이어 Pawn)만 Overlap, 지형(Static/Dynamic)에는 Block, 나머지 무시.
	// 실제로 막히는 건 상대가 Projectile 채널을 Block하는 경우뿐(= 지정 지형, bBlocksMovement 장식물). 막히면 ACC_Projectile::OnProjectileStopped.
	static void ConfigureAsEnemyProjectile(UPrimitiveComponent* Comp)
	{
		if (!Comp) return;
		Comp->SetCollisionObjectType(ECC_CC_Projectile);
		Comp->SetCollisionResponseToAllChannels(ECR_Ignore);
		Comp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		Comp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		Comp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
		Comp->SetGenerateOverlapEvents(true);
	}

	// 스킬 착탄 지형으로 "지정" — 기존 프리셋/오브젝트 타입/다른 응답은 그대로 두고 투사체 채널 Block만 추가.
	// (바닥 높이를 찾는 기존 트레이스(ECC_WorldStatic), 캐릭터 이동 등 다른 판정에는 영향 없음)
	static void ConfigureAsSkillTerrain(UPrimitiveComponent* Comp)
	{
		if (!Comp) return;
		const ECollisionEnabled::Type Enabled = Comp->GetCollisionEnabled();
		if (Enabled == ECollisionEnabled::NoCollision || Enabled == ECollisionEnabled::PhysicsOnly)
		{
			Comp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);   // 쿼리가 없으면 스윕에 안 잡힘
		}
		Comp->SetCollisionResponseToChannel(ECC_CC_Projectile, ECR_Block);
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

	/**
	 * 스킬 Effector 콜리전 디버그 — 콘솔: "CC.ShowSkillCollision 1" (0=끄기)
	 * 꺼져 있으면 즉시 return하므로 평상시 비용은 거의 없음.
	 */
	static bool IsSkillCollisionDebugEnabled();

	// 실제 콜리전 구체 — 노랑=판정 중, 회색=판정 종료(Drain 중)
	static void DrawSkillSphereDebug(const UWorld* World, const class USphereComponent* Sphere, bool bGameplayActive);

	// 오버랩 발생 지점 + 라벨 — 빨강=적중, 하늘색=Frozen 제외, 흰색=적 아님(무시)
	static void DrawSkillOverlapDebug(const UWorld* World, const FVector& Point, const FColor& Color, const FString& Label);
};
