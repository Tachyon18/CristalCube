// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../CristalCubeStruct.h"
#include "CC_SkillEffector.generated.h"

// SkillSystem으로 충돌 이벤트를 위임하는 델리게이트
// Effector는 충돌 및 판정 감지만, 판단·처리는 SkillSystem이 담당
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnEffectorHit,
	ACC_SkillEffector*, Effector,
	AActor*, HitActor
);

UCLASS()
class CRISTALCUBE_API ACC_SkillEffector : public AActor
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

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void SetSkillOwner(AActor* NewOwner) { SkillOwner = NewOwner; }

	UFUNCTION()
	void ApplyDamageToActor(AActor* TargetActor);

	void SetupAsProjectile();

protected:

};
