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

	// 스킬 슬롯 1~6 — MaxSkillSlots(기본 6)와 대응
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Skills")
	UInputAction* SkillAction1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Skills")
	UInputAction* SkillAction2;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Skills")
	UInputAction* SkillAction3;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Skills")
	UInputAction* SkillAction4;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Skills")
	UInputAction* SkillAction5;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Skills")
	UInputAction* SkillAction6;

protected:

	void HandleMove(const struct FInputActionValue& Value);

	void HandlePrimaryAttack(const FInputActionValue& Value);
	void HandleSecondaryAttack(const FInputActionValue& Value);

	void HandleDash(const FInputActionValue& Value);

	void HandleCastSkill1(const FInputActionValue& Value);
	void HandleCastSkill2(const FInputActionValue& Value);
	void HandleCastSkill3(const FInputActionValue& Value);
	void HandleCastSkill4(const FInputActionValue& Value);
	void HandleCastSkill5(const FInputActionValue& Value);
	void HandleCastSkill6(const FInputActionValue& Value);

protected:

	UFUNCTION(BlueprintPure, Category = "Input")
	FVector GetMouseWorldPosition() const;

	void UpdateCharacterRotation(float DeltaTime);

	UFUNCTION(BlueprintPure, Category = "Input")
	FVector GetMouseDirection() const;

	/** 실제 발동 공통 로직. 마우스 월드 좌표를 TargetLocation으로 사용. */
	void CastSkillAtSlot(int32 SlotIndex);

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

	//==============================================================================
	// DEBUG / TEST CONSOLE COMMANDS
	//==============================================================================

	/** 테스트용 — 콘솔(~)에서 "GrantExp 300" 형태로 호출. 인자 생략 시 100 지급.
	 *  AddExperience() 내부에 이미 while 루프로 다중 레벨업 처리가 있어서
	 *  큰 값을 한 번에 줘도 정상적으로 여러 번 LevelUp이 발생함. */
	UFUNCTION(Exec, Category = "Debug")
	void GrantExp(float Amount = 100.0f);

protected:

	UPROPERTY()
	bool bIsChargingBeam = false;

	UPROPERTY()
	FVector BeamStartLocation;

	UFUNCTION()
	void StartBeamCharge();

	UFUNCTION()
	void ReleaseBeam();

public:

	//==============================================================================
	// Game HUD
	//==============================================================================

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UCC_GameHUD> GameHUDClass;

	UPROPERTY()
	class UCC_GameHUD* CurrentGameHUD;

	UFUNCTION(BlueprintPure, Category = "UI")
	UCC_GameHUD* GetGameHUD() const { return CurrentGameHUD; }

protected:
	void CreateGameHUD();

};
