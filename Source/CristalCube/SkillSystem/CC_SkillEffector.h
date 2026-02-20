// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../CristalCubeStruct.h"
#include "CC_SkillEffector.generated.h"

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
	float EffectDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
	float LifeTime = 5.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Skill Effector")
	AActor* SkillOwner = nullptr;

protected:

	UPROPERTY(BlueprintReadOnly, Category = "Skill Effector")
	ESkillCoreType SkillCoreType = ESkillCoreType::None;

	UPROPERTY(BlueprintReadOnly, Category = "Skill Effector")
	FSkillDefinition SkillDef;

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

	//==========================================================================
	// PENETRATE LOGIC
	//==========================================================================

	UPROPERTY(BlueprintReadOnly, Category = "Skill Effector|Penetrate")
	bool bCanPenetrate = false;

	/** Maximum number of pierce */
	UPROPERTY(BlueprintReadOnly, Category = "Skill Effector|Penetrate")
	int32 MaxPenetrateCount = 0;

	/** Current pierce count */
	UPROPERTY(BlueprintReadOnly, Category = "Skill Effector|Penetrate")
	int32 CurrentPenetrateCount = 0;

	/** Actors that have been hit (to prevent duplicate hits) */
	UPROPERTY()
	TArray<AActor*> HitActors;

};
