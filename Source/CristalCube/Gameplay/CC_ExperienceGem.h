// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CC_Freezable.h"        
#include "CC_ExperienceGem.generated.h"

class ACC_Cube;

UCLASS()
class CRISTALCUBE_API ACC_ExperienceGem : public AActor, public ICC_Freezable
{

	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACC_ExperienceGem();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	void SetOwnerCube(ACC_Cube* Cube);

	virtual void Freeze_Implementation() override;
	virtual void Unfreeze_Implementation() override;
	virtual bool IsFrozen_Implementation() const override { return bIsFrozen; }

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class USphereComponent* CollisionSphere;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* GemMesh;

	UPROPERTY()
	FTimerHandle DistanceCheckTimer;

	UPROPERTY()
	FTimerHandle LifetimeTimer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MergeRadius = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ExpAmount = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MagnetRadius = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MoveSpeed = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AccelerationMultiplier = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Lifetime = 30.f;

	UPROPERTY()
	bool bIsBeingAttracted = false;

	UPROPERTY()
	TWeakObjectPtr<ACC_Cube> OwnerCube;     

	UPROPERTY()
	bool bIsFrozen = false;

	UPROPERTY()
	class ACharacter* TargetPlayer = nullptr;

	void CheckDistanceToPlayer();

	void MoveTowardsPlayer(float DeltaTime);

	void TryMergeNearbyGems();

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);

public:

	UFUNCTION(BlueprintCallable)
	void SetExpAmount(float NewAmount);
};
