// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../CristalCubeStruct.h"
#include "CC_SkillInstance.h"
#include "CC_SkillEffector.generated.h"

class UCC_EffectorPoolSubsystem;
class UCC_SkillSystem;
class UNiagaraComponent;
class UPrimitiveComponent;

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

	// 판정 최대 수명(초) - 아무것도 못 맞추면 이 시간 뒤 Drain(LifeSpan). MagicMissile 등은 SetLifeSpan으로 따로 지정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
	float EffectDuration = 3.0f;

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

	/** 판정을 지금 끝내고(콜리전·이동·히트 델리게이트·인스턴스 등록 해제), 상주 나이아가라는
	 *  스스로 끝날 때까지 둔 뒤(Drain) 숨기고 풀에 반납. Destroy() 대신 항상 이것을 사용. */
	UFUNCTION(BlueprintCallable, Category = "Skill Effector")
	void DeactivateAndDestroy();

	/** DeactivateAndDestroy와 같지만 Drain 없이 즉시 반납 — 큐브 전환 등 연출까지 바로 지워야 할 때.
	 *  이미 Drain 중이면 대기를 끊고 바로 반납. */
	UFUNCTION(BlueprintCallable, Category = "Skill Effector")
	void DeactivateImmediately();

	// 안전장치: 상주 나이아가라가 끝났다고 알려오지 않을 때 강제 반납까지 최대 대기 시간(초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	float MaxDrainTime = 2.0f;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void SetSkillOwner(AActor* NewOwner) { SkillOwner = NewOwner; }

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

	// 소유자/컨텍스트/Initialize/히트 바인딩이 모두 끝난 뒤 마지막에 호출 — 이때 처음으로 판정과 이동이 켜짐.
	// (판정을 먼저 켜면 스폰 직후 오버랩이 SkillOwner 미설정 상태로 처리되어 시전자 자신이 걸림)
	void Launch();

	// ICC_SkillInstance 구현
	virtual bool ShouldPersistThroughCubeTransition_Implementation() const override
	{
		return SkillDef.bPersistsThroughCubeTransition;
	}

	virtual void OnRemovedByCubeTransition_Implementation() override
	{
		DeactivateImmediately();
	}

protected:

	// LifeSpan 타임아웃(아무것도 못 맞추고 EffectDuration 지남)도 같은 경로를 타도록 오버라이드
	virtual void LifeSpanExpired() override;

	// 판정 종료 여부(Drain 중 포함) — 중복 종료(예: 큐브 전환 정리와 LifeSpan 타이머가 겹치는 경우) 방지용 가드.
	// 실제 풀 반납 완료 여부는 bReleasedToPool.
	bool bIsPooledInactive = false;

	// Drain 상태: 판정은 끝났고 상주 나이아가라 종료 대기 중
	bool bIsDraining = false;
	bool bReleasedToPool = false;
	int32 PendingDrainVFXCount = 0;
	FTimerHandle DrainTimeoutHandle;

	// Drain 시작 때 숨긴 나이아가라 외 보이는 것(메시 코어 등) — 재사용 시 복구
	TArray<TWeakObjectPtr<UPrimitiveComponent>> DrainHiddenComponents;

	// 두 종료 경로(Drain / 즉시)의 공통 앞부분 — 이미 판정이 끝난 상태면 false
	bool EndGameplay();
	void BeginDrain();
	void FinishRelease();
	void OnDrainTimeout();

	// 콜리전 디버그 (CC.ShowSkillCollision 1일 때만 타이머 가동)
	FTimerHandle CollisionDebugTimer;
	void StartCollisionDebugIfEnabled();
	void DrawCollisionDebug();
	void DrawOverlapDebug(const AActor* OtherActor, const UPrimitiveComponent* OtherComp, const FVector& Point, const FColor& Color) const;

	// 지형(WorldStatic/WorldDynamic)에 막혀 ProjectileMovement가 멈췄을 때 — 감지만 하고 처리는 SkillSystem에 위임
	UFUNCTION()
	void OnProjectileStopped(const FHitResult& ImpactResult);

	// 디버그: 발사 시점의 실제 콜리전 값 로그 + 이동 스윕과 같은 조건으로 아래 방향 차단 대상 검사
	void DebugReportCollision() const;

	// 상주 = 이 액터에 붙어 있는 나이아가라(BP 컴포넌트). AddVFX로 붙인 1회성은
	// EndGameplay에서 떼어내 스스로 끝나므로 제외.
	void GetResidentNiagara(TArray<UNiagaraComponent*>& OutVFX) const;

	UFUNCTION()
	void OnResidentVFXFinished(UNiagaraComponent* FinishedComponent);

	UPROPERTY()
	UCC_EffectorPoolSubsystem* OwningPool = nullptr;

	TWeakObjectPtr<UCC_SkillSystem> OwningSkillSystem;

};
