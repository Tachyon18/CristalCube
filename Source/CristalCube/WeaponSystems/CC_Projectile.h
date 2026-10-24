// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CC_Projectile.generated.h"

class UPrimitiveComponent;
class USphereComponent;
class UProjectileMovementComponent;
class UStaticMeshComponent;
class UNiagaraSystem;
class UNiagaraComponent;
class USoundBase;
class UCC_EffectorPoolSubsystem;

/**
 * 적 기본 공격 투사체 — 유효한 대상은 플레이어뿐. 스킬 투사체(ACC_SkillEffector)와 같은 문법:
 *   지정 지형(Projectile=Block)에 막히면 착탄 연출 후 종료, 판정은 즉시 끝내고 연출은 스스로 끝날 때까지 둠(Drain).
 * UCC_EffectorPoolSubsystem으로 풀링: Fire() = 활성화·발사, EndAndDrain() = Drain 후 반납, Deactivate() = 즉시 반납.
 * 수명은 수동 타이머(InitialLifeSpan 미사용) — 풀 대기 중인 인스턴스에 이전 LifeSpan 콜백이 오지 않게.
 */
UCLASS()
class CRISTALCUBE_API ACC_Projectile : public AActor
{
	GENERATED_BODY()

public:
	ACC_Projectile();

	// 풀에서 획득 (풀이 없으면 일반 SpawnActor로 폴백)
	static ACC_Projectile* AcquirePooled(UWorld* World, TSubclassOf<ACC_Projectile> ProjectileClass);

	// 활성화(재사용 포함) 후 발사. InSpeed <= 0이면 BP 기본 Speed 유지
	void Fire(const FVector& SpawnLocation, const FVector& Direction, float InDamage, float InSpeed, AActor* InOwner);

	// 즉시 반납 — 판정·이동·연출을 바로 끄고 숨긴 뒤 풀에 반납(풀 없으면 Destroy). Drain 중이면 대기를 끊음. 중복 호출 안전
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void Deactivate();

	// 판정·이동은 지금 끝내고, 상주 나이아가라는 스스로 끝날 때까지 둔 뒤 반납 (명중/지형 착탄/수명 만료)
	// Deactivate()는 즉시 반납 — 공격 취소(Freeze/텔레포트) 등 연출까지 바로 지워야 할 때
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void EndAndDrain();

	void SetOwningPool(UCC_EffectorPoolSubsystem* InPool) { OwningPool = InPool; }

	// 현재 발사자(반납되면 null, Drain 중에는 유지) — 쏜 적이 자기 탄만 취소할 수 있게
	AActor* GetProjectileOwner() const { return ProjectileOwner.Get(); }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// 지형에 막혀 ProjectileMovement가 멈췄을 때 — 착탄 연출 후 종료(스킬 투사체 OnProjectileStopped와 같은 문법)
	UFUNCTION()
	void OnProjectileStopped(const FHitResult& ImpactResult);

	void PlayHitEffects(const FVector& HitLocation) const;

protected:
	//==========================================================================
	// COMPONENTS
	//==========================================================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UProjectileMovementComponent* ProjectileMovement;

	// 선택 비주얼(대신 BP에서 나이아가라 컴포넌트를 붙여도 됨) — Drain 시작 때 숨김
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* ProjectileMesh;

	//==========================================================================
	// PROPERTIES
	//==========================================================================

	// Fire()가 덮어씀
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Stats")
	float Damage = 10.0f;

	// 발사자가 0을 넘기면 쓰는 기본 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Stats")
	float Speed = 1000.0f;

	// 수명(초) — 만료 시 EndAndDrain(Drain 후 반납)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Stats")
	float Lifetime = 5.0f;

	// 명중 시 종료(EndAndDrain) — false면 명중 후에도 계속 비행
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Behavior")
	bool bDestroyOnHit = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Behavior")
	bool bCanPierce = false;

	// bCanPierce일 때 추가로 통과할 수 있는 대상 수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Behavior")
	int32 PierceCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Effects")
	UNiagaraSystem* HitEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Effects")
	USoundBase* HitSound = nullptr;

	// 안전장치: 상주 나이아가라가 끝났다고 알려오지 않을 때 반납까지 최대 대기 시간(초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Effects")
	float MaxDrainTime = 2.0f;

private:
	UPROPERTY()
	UCC_EffectorPoolSubsystem* OwningPool = nullptr;

	// 약한 참조: 탄이 날아가거나 Drain 중일 때 발사자가 죽을 수 있음
	UPROPERTY()
	TWeakObjectPtr<AActor> ProjectileOwner;

	TArray<TWeakObjectPtr<AActor>> HitActors;

	int32 CurrentPierceCount = 0;

	// 판정 종료 상태(Drain 중·풀 대기·첫 Fire 전) — 실제 반납 완료는 bReleasedToPool
	bool bIsPooledInactive = true;

	FTimerHandle LifetimeTimerHandle;

	// 판정 종료 공통 처리 — 이미 끝난 상태면 false
	bool EndGameplay();

	// 실제 반납(숨김 + 풀 반납). 중복 호출 안전
	void FinishRelease();

	// 액터에 붙은 나이아가라(BP 컴포넌트)만 — 외부에 스폰한 HitEffect는 제외
	void GetResidentNiagara(TArray<UNiagaraComponent*>& OutVFX) const;

	UFUNCTION()
	void OnResidentVFXFinished(UNiagaraComponent* FinishedComponent);

	// Drain 상태: 판정은 끝났고 상주 나이아가라 종료 대기 중
	bool bIsDraining = false;

	// 풀 반납 완료 여부 (처음 Fire 전에도 true)
	bool bReleasedToPool = true;

	int32 PendingDrainVFXCount = 0;

	FTimerHandle DrainTimeoutHandle;

	// Drain 시작 때 숨긴 나이아가라 외 보이는 것(메시 등) — 재사용 시 복구
	TArray<TWeakObjectPtr<UPrimitiveComponent>> DrainHiddenComponents;
};