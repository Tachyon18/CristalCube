// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/World.h"
#include "CC_EffectorPoolSubsystem.generated.h"

USTRUCT()
struct FCC_ActorPoolFreeList
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<AActor>> Actors;
};

/**
 * 서로 상속 관계가 없는 여러 종류의 "풀링 가능한" 이펙터 액터(ACC_SkillEffector,
 * ACC_ShockwaveEffector, ACC_SigilEffector 등)를 클래스별로 한꺼번에 관리하는 제네릭 풀.
 * 풀 자체는 "빈 인스턴스를 정확한 클래스별로 대기열에 보관/꺼내기"만 하는 타입 무관
 * 로직이라 AActor* 기준으로 충분함 — 각 Effector의 리셋/정리 로직(Deactivate 등)은
 * 여전히 각 클래스 자신이 갖고 있고, 반납 시 이 서브시스템의 ReleaseEffector()만 호출한다.
 * 호출부는 반환값을 원하는 타입으로 Cast<>해서 사용.
 */
UCLASS()
class CRISTALCUBE_API UCC_EffectorPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	// EffectorClass의 재사용 가능한 인스턴스가 있으면 그걸 반환(ActivateAtTransform 호출),
	// 없으면 새로 스폰(이후 이 풀에 등록되어 다음부터는 재사용됨).
	AActor* AcquireEffector(TSubclassOf<AActor> EffectorClass);

	// 인스턴스를 파괴하는 대신 반납. ACC_SkillEffector::DeactivateAndDestroy()가 호출함 —
	// 직접 호출하지 않음.
	void ReleaseEffector(AActor* Effector);

protected:
	// 재사용 대기 중인(숨김+비활성) 인스턴스, 클래스별로 분리 보관.
	UPROPERTY()
	TMap<TSubclassOf<AActor>, FCC_ActorPoolFreeList> FreeActors;

};

// Shockwave/Sigil처럼 스폰 시점에 Transform이 필요 없고(Initialize()가 직접
// SetActorLocation하는) 이펙터용 공용 헬퍼 — 풀에서 획득 + SetOwningPool()까지 마치고
// 반환, 풀이 없으면 새로 스폰. TEffector는 SetOwningPool(UCC_EffectorPoolSubsystem*)를
// 갖고 있어야 함(세 이펙터 클래스 모두 상속 없이 동일 시그니처로 각자 구현하고 있음).
template<typename TEffector>
TEffector* CC_AcquirePooledEffector(UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}

	UCC_EffectorPoolSubsystem* Pool = World->GetSubsystem<UCC_EffectorPoolSubsystem>();
	if (!Pool)
	{
		return World->SpawnActor<TEffector>();
	}

	TEffector* Effector = Cast<TEffector>(Pool->AcquireEffector(TEffector::StaticClass()));
	if (Effector)
	{
		Effector->SetOwningPool(Pool);
	}

	return Effector;
}