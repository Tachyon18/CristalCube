// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../CristalCubeStruct.h"
#include "CC_SkillInstance.h"
#include "CC_SkillSystem.generated.h"


// Forward declarations
class ACC_Projectile;
class UNiagaraSystem;

/**
 * 모듈형 스킬 시스템 컴포넌트
 *
 * Phase 1 (Week 9-10): 프로토타입
 * - Core 3개 (Projectile, Instant, Area)
 * - Addon 4개 (Explosion, Chain, Penetrate, MultiShot)
 *
 * Phase 2+: 확장
 * - Core 추가 (Slash, Beam, Channeling, etc.)
 * - Addon 추가 (Homing, Echo, Split, etc.)
 * - Temporal/Spatial 변조
 *
 * 이름은 확장해도 변경하지 않음!
 */

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CRISTALCUBE_API UCC_SkillSystem : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCC_SkillSystem();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


public:

	//==========================================================================
	// MAIN INTERFACE
	//==========================================================================

	/**
	 * 스킬 실행 (메인 함수)
	 * @param Skill - 실행할 스킬 정의
	 * @param TargetLocation - 목표 위치
	 */
	UFUNCTION(BlueprintCallable, Category = "Skill System")
	void ExecuteSkill(const FSkillDefinition& Skill, FVector TargetLocation);

	/**
	 * 스킬 실행 (타겟 Actor 지정)
	 * @param Skill - 실행할 스킬 정의
	 * @param TargetActor - 목표 Actor
	 */
	UFUNCTION(BlueprintCallable, Category = "Skill System")
	void ExecuteSkillOnTarget(const FSkillDefinition& Skill, AActor* TargetActor);

protected:

	//==========================================================================
	// CORE EXECUTION (Phase 1: 3개, 향후 확장 가능)
	//==========================================================================

	/**
	 * Projectile Core - 투사체 발사
	 */
	void ExecuteProjectile(const FSkillDefinition& Skill, FSkillExecutionContext& Context);

	/**
	 * Instant Core - 즉발 공격 (히트스캔)
	 */
	void ExecuteInstant(const FSkillDefinition& Skill, FSkillExecutionContext& Context);

	/**
	 * Area Core - 범위 공격
	 */
	void ExecuteArea(const FSkillDefinition& Skill, FSkillExecutionContext& Context);

	/**
	* Beam Core - 빔 공격 (향후 확장)
	*/
	void ExecuteBeam(const FSkillDefinition& Skill, FSkillExecutionContext& Context);

	/**
	 * Rainfall Core ? 공중에서 복수 투사체 낙하
	 */
	void ExecuteRainfall(const FSkillDefinition& Skill, FSkillExecutionContext& Context);

	/**
	 * Rainfall 개별 투사체 스폰 (타이머 콜백에서 호출)
	 */
	void SpawnRainfallProjectile(const FSkillDefinition& Skill,
		const FSkillExecutionContext& Context,
		FVector TargetFloorLocation);

	/**
	 * 낙하 위치 배열 계산 (패턴별)
	 */
	TArray<FVector> CalculateDropLocations(FVector Center, float Radius,
		int32 Count, EDropPattern Pattern) const;

public:

	//==========================================================================
	// ADDON PROCESSING (Phase 1: 4개, 향후 확장 가능)
	//==========================================================================

	/**
	 * Addon 처리 (충돌 시 호출)
	 * @param Skill - 스킬 정의
	 * @param Context - 실행 컨텍스트
	 * @param Hit - 충돌 정보
	 */
	void ProcessAddons(const FSkillDefinition& Skill, FSkillExecutionContext& Context, const FHitResult& Hit);

	/**
	 * Explosion Addon - 폭발 범위 피해
	 */
	void ApplyExplosion(const FSkillDefinition& Skill, FSkillExecutionContext& Context, FVector Location);

	/**
	 * Chain Addon - 연쇄 공격
	 */
	void ApplyChain(const FSkillDefinition& Skill, FSkillExecutionContext& Context, AActor* HitTarget);

	/**
	 * Penetrate Addon - 관통 (이미 적용됨, 카운터만 체크)
	 */
	bool CanPenetrate(const FSkillDefinition& Skill, FSkillExecutionContext& Context) const;

	/**
	 * MultiShot Addon - 다중 발사 (ExecuteProjectile에서 처리)
	 */
	int32 GetProjectileCount(const FSkillDefinition& Skill) const;

	/**
	* Chain Addon - 체인 이펙트에 목표 지점 User Parameter로 전달
	*/

	void SpawnChainEffect(UNiagaraSystem* Effect, FVector StartLocation, FVector TargetLocation);

protected:

	//==========================================================================
	// UTILITY FUNCTIONS
	//==========================================================================

	UFUNCTION()
	void OnProjectileHit(class ACC_SkillEffector* Effector, AActor* HitActor);

	UFUNCTION()
	void OnCubeTransitioned(FIntPoint NewCoordinate);

	/**
	 * 지속되는 스킬 인스턴스 액터를 추적 목록에 등록.
	 * Projectile/Rainfall Effector뿐 아니라, 향후 ICC_SkillInstance를 구현한
	 * 어떤 스킬 인스턴스 액터든(Channeling 등) 스폰 시 이 함수만 호출하면
	 * 별도 코드 수정 없이 큐브 전환 정리 대상에 포함됨.
	 */
	UFUNCTION(BlueprintCallable, Category = "Skill System")
	void RegisterActiveSkillInstance(AActor* Instance);

	/**
	 * 가장 가까운 적 찾기
	 */
	UFUNCTION(BlueprintCallable, Category = "Skill System|Utility")
	AActor* FindNearestEnemy(FVector Origin, float Radius, const TArray<AActor*>& ExcludeActors) const;

	/**
	 * 반경 내 모든 적 찾기
	 */
	UFUNCTION(BlueprintCallable, Category = "Skill System|Utility")
	TArray<AActor*> FindEnemiesInRadius(FVector Origin, float Radius) const;

	/**
	 * 피해 적용
	 */
	void ApplyDamage(AActor* Target, float Damage, AActor* DamageCauser);

	/**
	 * VFX 스폰
	 */
	void SpawnEffect(UNiagaraSystem* Effect, FVector Location, FRotator Rotation = FRotator::ZeroRotator);

	/**
	 * 사운드 재생
	 */
	void PlaySound(USoundBase* Sound, FVector Location);

	//==========================================================================
	// PROPERTIES
	//==========================================================================

	// Projectile 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Skill System|Setup")
	TSubclassOf<ACC_Projectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "Skill System|Setup")
	TSubclassOf <class ACC_SkillEffector> SkillEffectorClass;

	// 적 태그
	UPROPERTY(EditDefaultsOnly, Category = "Skill System|Setup")
	FName EnemyTag = TEXT("Enemy");

	// 디버그 드로우
	UPROPERTY(EditAnywhere, Category = "Skill System|Debug")
	bool bShowDebugShapes = false;

	UPROPERTY(EditAnywhere, Category = "Skill System|Debug")
	float DebugDrawDuration = 2.0f;


	//==========================================================================
	// RUNTIME DATA
	//==========================================================================

	// 현재 실행 중인 스킬들 (Phase 2+ 확장용)
	UPROPERTY()
	TArray<FSkillExecutionContext> ActiveSkills;

	/** 현재 진행 중인 스킬 인스턴스 액터들. 큐브 전환 시 ICC_SkillInstance 기준으로 정리됨. */
	UPROPERTY()
	TArray<AActor*> ActiveSkillInstances;

};
