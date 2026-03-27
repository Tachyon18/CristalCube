// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CC_Character.generated.h"

UCLASS()
class CRISTALCUBE_API ACC_Character : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ACC_Character();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float MaxHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float BaseMaxHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float CurrentHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float MoveSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float BaseMoveSpeed;

public:
	// Damage System
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	// Death System
	UFUNCTION(BlueprintCallable, Category = "Character")
	virtual void Die();

	UFUNCTION(BlueprintCallable, Category = "Character")
	float Heal(float HealAmount);

	UFUNCTION(BlueprintCallable, Category = "Character")
	bool IsAlive() const { return CurrentHealth > 0.0f; }

	// Getters
	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetHealthPercentage() const { return MaxHealth > 0.0f ? (CurrentHealth / MaxHealth) : 0.0f; }



protected:

	virtual void ApplyStats();
};
