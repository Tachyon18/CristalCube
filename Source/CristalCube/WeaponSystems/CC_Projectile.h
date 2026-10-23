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
 * Enemy basic-attack projectile. The player is the only valid target.
 * Pooled through UCC_EffectorPoolSubsystem: Fire() activates, Deactivate() returns it.
 * Lifetime uses a manual timer (no InitialLifeSpan), so a pooled instance never
 * receives a stale LifeSpan callback while it waits in the free list.
 */
UCLASS()
class CRISTALCUBE_API ACC_Projectile : public AActor
{
	GENERATED_BODY()

public:
	ACC_Projectile();

	// Pooled spawn. Falls back to a plain SpawnActor when no pool exists.
	static ACC_Projectile* AcquirePooled(UWorld* World, TSubclassOf<ACC_Projectile> ProjectileClass);

	// Activate (or re-activate) and launch. InSpeed <= 0 keeps the Blueprint default Speed.
	void Fire(const FVector& SpawnLocation, const FVector& Direction, float InDamage, float InSpeed, AActor* InOwner);

	// Stop, hide and return to the pool (Destroy when not pooled). Safe to call twice.
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void Deactivate();

	// 판정·이동은 지금 끝내고, 상주 나이아가라는 스스로 끝날 때까지 둔 뒤 반납 (명중/지형 착탄/수명 만료)
	// Deactivate()는 즉시 반납 — 공격 취소(Freeze/텔레포트) 등 연출까지 바로 지워야 할 때
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void EndAndDrain();

	void SetOwningPool(UCC_EffectorPoolSubsystem* InPool) { OwningPool = InPool; }

	// Current shooter (null once deactivated) - lets the shooter cancel only its own live shots
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

	// Optional visual (a Niagara component can be added in the Blueprint instead)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* ProjectileMesh;

	//==========================================================================
	// PROPERTIES
	//==========================================================================

	// Overwritten by Fire()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Stats")
	float Damage = 10.0f;

	// Default speed when the shooter passes 0
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Stats")
	float Speed = 1000.0f;

	// Seconds before auto-deactivate
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Stats")
	float Lifetime = 5.0f;

	// Deactivate (return to pool) on hit
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Behavior")
	bool bDestroyOnHit = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Behavior")
	bool bCanPierce = false;

	// Number of extra targets it can pass through when bCanPierce is true
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

	// Weak: the shooter can die while the projectile is still flying or pooled
	UPROPERTY()
	TWeakObjectPtr<AActor> ProjectileOwner;

	TArray<TWeakObjectPtr<AActor>> HitActors;

	int32 CurrentPierceCount = 0;

	// True while waiting in the pool (and before the first Fire)
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