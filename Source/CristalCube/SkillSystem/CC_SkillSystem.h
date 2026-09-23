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
class USceneComponent;

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

	void ExecuteSkillWithContext(const FSkillDefinition& Skill, FSkillExecutionContext Context, FVector TargetLocation, int32 StartIndex = 0);

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
	void ProcessAddons(const FSkillDefinition& Skill, FSkillExecutionContext& Context, const FHitResult& Hit, int32 StartIndex = 0);

	void ProcessCastAddons(const FSkillDefinition& Skill, FSkillExecutionContext& Context, int32 StartIndex = 0);

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

	void SpawnChainEffect(UNiagaraSystem* Effect, FVector StartLocation, FVector TargetLocation, ESkillElementType ElementType = ESkillElementType::None);

	/**
	 * 궤적형(시작점→끝점) VFX 스폰 ? Beam Core와 Chain Addon이 공유하는 로직.
	 * User.BeamEnd 세팅 → (Width>0이면 User.BeamWidth도 세팅) → 색상 주입 → 수동 Activate.
	 * @param Width - 0 이하면 세팅 스킵(에셋 자체 기본값 사용)
	 */
	void SpawnTrajectoryEffect(UNiagaraSystem* Effect, FVector StartLocation, FVector TargetLocation, float Width = 0.0f, ESkillElementType ElementType = ESkillElementType::None);

public:

	//==========================================================================
	// UTILITY FUNCTIONS
	//==========================================================================

	UFUNCTION()
	void OnProjectileHit(class ACC_SkillEffector* Effector, AActor* HitActor, FVector HitLocation);

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
	 * ActiveSkillInstances에서 명시적으로 제거. 풀에 반납된(재사용 대기 중인) Effector가
	 * 큐브 전환 정리 로직에 "여전히 활성"으로 잘못 걸리는 걸 막기 위해 반납 시점에 호출됨.
	 */
	UFUNCTION(BlueprintCallable, Category = "Skill System")
	void UnregisterActiveSkillInstance(AActor* Instance);

	// SkillEffector를 풀에서 획득(없으면 새로 스폰)하고 Transform까지 재배치해서 반환.
	// ExecuteProjectile()/SpawnRainfallProjectile() 공용 헬퍼.
	ACC_SkillEffector* AcquireSkillEffector(TSubclassOf<ACC_SkillEffector> EffectorClass, const FTransform& SpawnTransform);

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
	 * @param ElementType - None이면 색 주입 스킵(경고 텔레그래프 등 원소색 안 태우고 싶을 때).
	 *                      그 외엔 GetElementColor() 기준으로 User.PrimaryColor/SecondaryColor 자동 주입.
	 */
	void SpawnEffect(UNiagaraSystem* Effect, FVector Location, FRotator Rotation = FRotator::ZeroRotator, ESkillElementType ElementType = ESkillElementType::None);

	/**
	 * 지속형 Attach VFX 스폰 ? Shockwave/Sigil/ElementalStatus 등 "호출자가 수명을 직접
	 * 관리하는 지속 이펙트"가 공유하는 스폰 패턴(AutoDestroy=false, SnapToTarget, 풀링 없음,
	 * PreCullCheck=true) + 색상 주입까지 한 번에 처리하는 정적 헬퍼.
	 */
	static UNiagaraComponent* SpawnPersistentAttachedVFX(UNiagaraSystem* Effect, USceneComponent* AttachToComponent, ESkillElementType ElementType = ESkillElementType::None);

	/**
	 * 사운드 재생
	 */
	void PlaySound(USoundBase* Sound, FVector Location);
	
protected:

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
