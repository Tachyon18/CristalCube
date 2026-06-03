// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CC_LogHelper.h"
#include "CC_PlayerController.generated.h"


/**
 * 
 */
UCLASS()
class CRISTALCUBE_API ACC_PlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	ACC_PlayerController();

protected:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	virtual void OnPossess(APawn* InPawn) override;
	virtual void SetupInputComponent() override;
	

protected:

	// Input Mapping Context
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputMappingContext* DefaultMappingContext;

	// Movement Action
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* MoveAction;

	// Attack Actions
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* PrimaryAttackAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* SecondaryAttackAction;

	// Special Actions
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* DashAction;

protected:

	void HandleMove(const struct FInputActionValue& Value);

	void HandlePrimaryAttack(const FInputActionValue& Value);
	void HandleSecondaryAttack(const FInputActionValue& Value);

	void HandleDash(const FInputActionValue& Value);

protected:

	UFUNCTION(BlueprintPure, Category = "Input")
	FVector GetMouseWorldPosition() const;

	void UpdateCharacterRotation(float DeltaTime);

	UFUNCTION(BlueprintPure, Category = "Input")
	FVector GetMouseDirection() const;

protected:

	UPROPERTY()
	class ACC_PlayerCharacter* ControlledCharacter;

	UPROPERTY(EditAnywhere, Category = "Input", meta = (ClampMin = "1.0", ClampMax = "30.0"))
	float MouseRotationSpeed = 12.0f;

	UPROPERTY(EditAnywhere, Category = "Input", meta = (ClampMin = "10.0", ClampMax = "200.0"))
	float MinMouseDistance = 50.0f;

protected:

	FVector2D CurrentMoveInput;

	bool bIsPrimaryAttacking = false;
	bool bIsSecondaryAttacking = false;

	FVector LastMouseWorldPosition;
	float LastMouseUpdateTime = 0.0f;
	float MouseUpdateInterval = 0.016f; // 60 FPS

public:

	UFUNCTION(BlueprintPure, Category = "Input")
	FVector2D GetCurrentMoveInput() const { return CurrentMoveInput; }

	UFUNCTION(BlueprintPure, Category = "Input")
	bool IsPrimaryAttacking() const { return bIsPrimaryAttacking; }

	UFUNCTION(BlueprintPure, Category = "Input")
	bool IsSecondaryAttacking() const { return bIsSecondaryAttacking; }

	UFUNCTION(BlueprintCallable, Category = "Input")
	void SetMouseRotationSpeed(float NewSpeed);

	UFUNCTION(BlueprintCallable, Category = "Input")
	void SetMinMouseDistance(float NewDistance);

protected:

	UPROPERTY()
	bool bIsChargingBeam = false;

	UPROPERTY()
	FVector BeamStartLocation;

	UFUNCTION()
	void StartBeamCharge();

	UFUNCTION()
	void ReleaseBeam();

};
