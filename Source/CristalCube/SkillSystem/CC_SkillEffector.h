// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../CristalCubeStruct.h"
#include "CC_SkillInstance.h"
#include "CC_SkillEffector.generated.h"

class UCC_EffectorPoolSubsystem;
class UCC_SkillSystem;

// SkillSystem으로 충돌 이벤트를 위임하는 델리게이트
// Effector는 충돌 및 판정 감지만, 판단·처리는 SkillSystem이 담당
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnEffectorHit,
	ACC_SkillEffector*, Effector,
	AActor*, HitActor,
	FVector, HitLocation
);

UCLASS()
class CRISTALCUBE_API ACC_SkillEffector : public AActor, public ICC_SkillInstance
{

	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACC_SkillEffector();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;


public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision")
	class USphereComponent* CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VFX")
	USceneComponent* VFXRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UProjectileMovementComponent* ProjectileMovement;

	UPROPERTY()
	TArray<class UNiagaraComponent*> VFXStack;

	UFUNCTION(BlueprintCallable, Category = "VFX")
	UNiagaraComponent* AddVFX(UNiagaraSystem* VFXTemplate);

	UFUNCTION(BlueprintCallable, Category = "VFX")
	void SetVFXColor(FLinearColor PrimaryColor, FLinearColor SecondaryColor);

	//==========================================================================
	// SKILL DATA (SkillSystem이 주입)
	//==========================================================================

	// SkillSystem이 Spawn 후 직접 설정
	UPROPERTY(BlueprintReadWrite, Category = "Skill Effector")
	FSkillExecutionContext SkillContext;

	// SkillSystem의 OnProjectileHit 핸들러에서 접근
	UPROPERTY(BlueprintReadOnly, Category = "Skill Effector")
	FSkillDefinition SkillDef;

	// 이 Effector의 히트가 ProcessAddons()를 재실행할 때 시작할 인덱스.
	// 일반 투사체는 0(전체 재처리). Homing Addon이 스폰한 2차 발사체는
	// AddonIndex + 1로 설정되어 "자신을 스폰한 Addon보다 뒤"만 연쇄된다.
	UPROPERTY(BlueprintReadWrite, Category = "Skill Effector")
	int32 AddonStartIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
	float EffectDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
	float LifeTime = 5.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Skill Effector")
	AActor* SkillOwner = nullptr;

	UPROPERTY(BlueprintAssignable, Category = "Skill Effector")
	FOnEffectorHit OnEffectorHit;

protected:

	UPROPERTY(BlueprintReadOnly, Category = "Skill Effector")
	ESkillCoreType SkillCoreType = ESkillCoreType::None;


public:

	UFUNCTION(BlueprintCallable, Category = "Skill Effector")
	void Initialize(ESkillCoreType InCoreType, const FSkillDefinition& InSkillDef);

	/** Destroy() 직접 호출 대신 항상 이걸 사용 — VFX가 Death Event/잔여 파티클을
	*  끊기지 않고 정상 종료할 수 있도록 Detach 후 파괴함. */
	UFUNCTION(BlueprintCallable, Category = "Skill Effector")
	void DeactivateAndDestroy();

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void SetSkillOwner(AActor* NewOwner) { SkillOwner = NewOwner; }

	UFUNCTION()
	void ApplyDamageToActor(AActor* TargetActor);

	void SetupAsProjectile();
	void SetupAsRainfall();

	//==========================================================================
	// POOLING
	//==========================================================================

	// 풀이 신규 스폰 직후 1회 호출. 풀 없이 스폰된 경우(레벨 직접 배치 등)엔 안 불리며,
	// 그 경우 DeactivateAndDestroy()는 기존처럼 Destroy()로 폴백.
	void SetOwningPool(UCC_EffectorPoolSubsystem* InPool);

	// 이 Effector를 ActiveSkillInstances에 등록한 SkillSystem 참조 — 반납 시 그쪽에서도
	// 스스로 등록 해제하기 위함. TWeakObjectPtr — SkillSystem 생명주기가 갈릴 수 있어서
	// (플레이어가 먼저 사라지는 등) 약한 참조로 보관.
	void SetOwningSkillSystem(UCC_SkillSystem* InSkillSystem);

	// 재사용 시 위치/충돌/VFX/수명/Addon 인덱스 등 이전 사용 흔적을 전부 리셋하고
	// NewTransform에 재배치. 풀이 신규 스폰 시/재사용 시 둘 다 호출.
	void ActivateAtTransform(const FTransform& NewTransform);

	// ICC_SkillInstance 구현
	virtual bool ShouldPersistThroughCubeTransition_Implementation() const override
	{
		return SkillDef.bPersistsThroughCubeTransition;
	}

	virtual void OnRemovedByCubeTransition_Implementation() override
	{
		DeactivateAndDestroy();
	}
protected:

	// LifeSpan 타임아웃(아무것도 못 맞추고 EffectDuration 지남)도 같은 경로를 타도록 오버라이드
	virtual void LifeSpanExpired() override;

	// 풀 반납/파괴 처리가 이미 끝났는지 — 중복 호출(예: 큐브 전환 정리와 LifeSpan 타이머가
	// 겹치는 경우) 방지용 가드.
	bool bIsPooledInactive = false;

	UPROPERTY()
	UCC_EffectorPoolSubsystem* OwningPool = nullptr;

	TWeakObjectPtr<UCC_SkillSystem> OwningSkillSystem;

};
