// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CC_Character.h"
#include "../CristalCubeStruct.h"
#include "NiagaraSystem.h"
#include "CC_PlayerCharacter.generated.h"

/**
 * 
 */
UCLASS()
class CRISTALCUBE_API ACC_PlayerCharacter : public ACC_Character
{
	GENERATED_BODY()
	
public:
	ACC_PlayerCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	class UCameraComponent* FollowCamera;


protected:

	//==============================================================================
	// CHARACTER STATS SYSTEM
	//==============================================================================

	// Player's stats
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	FCristalCubePlayerStats PlayerStats;

	// Apply stats to character (called when stats change)
	UFUNCTION(BlueprintCallable, Category = "Stats")
	void ApplyPlayerStats();

	// Getters
	UFUNCTION(BlueprintCallable, Category = "Stats")
	FCristalCubePlayerStats GetPlayerStats() const { return PlayerStats; }

	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetFinalDamageMultiplier() const { return PlayerStats.BasicStats.DamageMultiplier; }

	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetFinalAttackSpeedMultiplier() const { return PlayerStats.BasicStats.AttackSpeedMultiplier; }

public:

	/**
	 * 단일 스탯 수정 + 즉시 적용.
	 * 업그레이드/스킬 보너스/큐브 보상 등 런타임 스탯 변경에 사용.
	 * Value는 배율 기준 델타값 (예: MoveSpeed +10% → Value = 0.1f)
	 */
	UFUNCTION(BlueprintCallable, Category = "Stats")
	void ApplyStatUpgrade(EUpgradeType Type, float Value);

	/**
	 * 전체 구조체 덮어쓰기 + 일괄 적용.
	 * BeginPlay 초기화, 세이브 로드, 완전 리셋 등 일괄 설정 시 사용.
	 */
	UFUNCTION(BlueprintCallable, Category = "Stats")
	void ApplyStatOverride(const FCristalCubePlayerStats& InStats);

	/**
	 * PlayerStats 구조체는 건드리지 않고 UE 시스템에만 반영.
	 * Blueprint에서 PlayerStats를 직접 수정한 뒤 호출.
	 */
	UFUNCTION(BlueprintCallable, Category = "Stats")
	void RefreshStats();


protected:

	//==============================================================================
	// COMBAT - basic attack entry point
	//==============================================================================

public:

	// Called by ACC_PlayerController::HandlePrimaryAttack.
	// Weapon system removed (2026-10-20). Kept as a no-op entry point for the
	// future skill-based weapon object (Gun variants, etc.).
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void PerformAttack();

	UFUNCTION(BlueprintPure, Category = "Skills")
	UCC_SkillSystem* GetSkillSystem() const { return SkillSystem; }

protected:
	
	//==============================================================================
	// Skill System Component
	//==============================================================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UCC_SkillSystem* SkillSystem;

public:

	// (2026-10-26) 테스트 캐스트 함수(Cast*/Test*)는 제거 - 실제 스킬은 SkillTableRow + AddonPreset 데이터 기반.
	// 벡터 레이저는 Track 4 Skill Path 이후 Laser(Beam)로 흡수 예정 (Track 5 A8)

	// === 벡터 레이저 (마우스 드래그) ===
	UFUNCTION(BlueprintCallable, Category = "Skills")
	void StartVectorLaser();       // 레이저 시작점 기록

	UFUNCTION(BlueprintCallable, Category = "Skills")
	void ReleaseVectorLaser();     // 레이저 발사

protected:
	
	bool bIsChargingLaser;
	FVector LaserStartPoint;

protected:

	//==============================================================================
	// LEVEL & EXPERIENCE SYSTEM
	//==============================================================================
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level")
	int32 Level;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level")
	float Experience = 0.0f;

	/** while(AddExperience) 루프에서 한 번에 여러 레벨이 오른 경우,
	 *  LevelUp 카드 선택 UI를 한 번에 하나씩 순차로 띄우기 위한 대기 카운트. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level")
	int32 PendingLevelUpCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level")
	float ExperienceToNextLevel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level")
	TSubclassOf<UUserWidget> LevelUpWidgetClass;

	/** Base experience for level 2 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Experience")
	float BaseExperienceRequirement;

	/** Experience scaling per level */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Experience")
	float ExperienceScaling;

	//==============================================================================
	// Tile
	//==============================================================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UCC_TileTrackerComponent* TileTrackerComponent;

	UFUNCTION()
	void OnPlayerTileChanged(int32 OldTileIndex, int32 NewTileIndex, FVector NewPosition);

	//==============================================================================
	// Widgets
	//==============================================================================

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UCC_GameHUD> GameHUDClass;

	UPROPERTY()
	UCC_GameHUD* CurrentGameHUD;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UCC_SkillInventoryWidget> SkillInventoryWidgetClass;

	UPROPERTY()
	class UCC_SkillInventoryWidget* CurrentSkillInventoryWidget;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UCC_LevelUpWidget> LevelUpUIClass;

	UPROPERTY()
	UCC_LevelUpWidget* CurrentLevelUpUI;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float GameTime = 0.0f;

	void UpdateGameHUD();



protected:



public:

	//==============================================================================
	// PUBLIC FUNCTIONS
	//==============================================================================

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	// Add experience (called when enemy dies)
	UFUNCTION(BlueprintCallable, Category = "Level")
	void AddExperience(float ExpAmount);

	UFUNCTION(BlueprintCallable, Category = "Level")
	void LevelUp();

	UFUNCTION(BlueprintCallable, Category = "Level")
	int32 GetPlayerLevel() const { return Level; }

	UFUNCTION(BlueprintCallable, Category = "Level")
	float GetExperience() const { return Experience; }

	UFUNCTION(BlueprintPure, Category = "Level")
	float GetExperiencePercentage() const;

	/** 이번 판 경과 시간(초) — 결과 화면용 */
	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetGameTime() const { return GameTime; }

	UFUNCTION(BlueprintCallable)
	void ShowLevelUpUI();

	UFUNCTION(BlueprintCallable)
	void HideLevelUpUI();

	UFUNCTION(BlueprintCallable)
	void OnLevelUpCandidateSelected(FLevelUpCandidate SelectedCandidate);

	/** Returns CameraBoom subobject */
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject */
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }


	//==============================================================================
	// OVERRIDE FROM BASE CHARACTER
	//==============================================================================

protected:
	// Override ApplyStats to include player-specific stat calculations

	virtual void ApplyStats() override;

	virtual void Die() override;

};
