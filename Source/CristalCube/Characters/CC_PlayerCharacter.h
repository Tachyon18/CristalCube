// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CC_Character.h"
#include "../CristalCubeStruct.h"
#include "../WeaponSystems/CC_Weapon.h"
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

	void CaptureBaseStatsIfNeeded();

	// Getters
	UFUNCTION(BlueprintCallable, Category = "Stats")
	FCristalCubePlayerStats GetPlayerStats() const { return PlayerStats; }

	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetFinalDamageMultiplier() const { return PlayerStats.BasicStats.DamageMultiplier; }

	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetFinalAttackSpeedMultiplier() const { return PlayerStats.BasicStats.AttackSpeedMultiplier; }

protected:

	//==============================================================================
	// WEAPON SYSTEM
	//==============================================================================

	// Current equipped weapons

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons|Data")
	UDataTable* WeaponDataTable;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	class UCC_SearchingComponent* SearchingComponent;

	UPROPERTY()
	class UCC_WeaponManagerSubsystem* WeaponManager;

	void InitializeWeaponSystem();

	void EquipStartingWeapons();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	int32 StartingWeaponCount = 2;

	// Current active weapon
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	class ACC_Weapon* CurrentWeapon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Weapons")
	TArray<ACC_Weapon*> EquippedWeapons;

	/** Starting weapon class (equipped on BeginPlay) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Weapons")
	TSubclassOf<ACC_Weapon> StartingWeaponClass;

	/** Maximum number of weapons player can equip */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Weapons")
	int32 MaxWeapons;

	UPROPERTY()
	TMap<ACC_Weapon*, FTimerHandle> WeaponAttackTimers;

public:

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void EquipWeaponByName(FName WeaponRowName);

	// Weapon Management
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool EquipWeapon(ACC_Weapon* Weapon);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	ACC_Weapon* CreateAndEquipWeapon(TSubclassOf<ACC_Weapon> WeaponClass);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void UnequipWeapon(ACC_Weapon* Weapon);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void UnequipAllWeapons();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SwitchWeapon();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void PerformAttack();

	// Getters
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	UCC_SearchingComponent* GetSearchingComponent() const { return SearchingComponent; }
	
	UFUNCTION(BlueprintPure, Category = "Weapon")
	ACC_Weapon* GetCurrentWeapon() const { return CurrentWeapon; }

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	ACC_Weapon* FindWeaponByClass(TSubclassOf<ACC_Weapon> WeaponClass) const;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	ACC_Weapon* CreateWeapon(TSubclassOf<ACC_Weapon> WeaponClass);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void StartWeaponAutoAttack(ACC_Weapon* Weapon);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void StopWeaponAutoAttack(ACC_Weapon* Weapon);

	/** Get all equipped weapons */
	UFUNCTION(BlueprintPure, Category = "Player|Weapons")
	TArray<ACC_Weapon*> GetEquippedWeapons() const { return EquippedWeapons; }

	/** Check if can equip more weapons */
	UFUNCTION(BlueprintPure, Category = "Player|Weapons")
	bool CanEquipMoreWeapons() const { return EquippedWeapons.Num() < MaxWeapons; }

	/** Get weapon count */
	UFUNCTION(BlueprintPure, Category = "Player|Weapons")
	int32 GetEquippedWeaponCount() const { return EquippedWeapons.Num(); }

	/** Check if specific weapon is equipped */
	UFUNCTION(BlueprintPure, Category = "Player|Weapons")
	bool HasWeapon(ACC_Weapon* Weapon) const { return EquippedWeapons.Contains(Weapon); }

protected:
	
	//==============================================================================
	// Skill System Component
	//==============================================================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UCC_SkillSystem* SkillSystem;

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test")
	UNiagaraSystem* TestIceEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test")
	UNiagaraSystem* TestLightningEffect = nullptr;

	UFUNCTION(BlueprintCallable, Category = "Test")
	void TestBasicSkill();

	UFUNCTION(BlueprintCallable, Category = "Test")
	void TestProjectileSkill();

public:

	//==============================================================================
	// Test Skill System - 테스트용이므로 추후 제거
	//==============================================================================

	UFUNCTION(BlueprintCallable, Category = "Skills")
	void CastFireball();           // 화염구 - 투사체 + 폭발

	UFUNCTION(BlueprintCallable, Category = "Skills")
	void CastLightningBolt();      // 번개 - 즉발 + 연쇄

	UFUNCTION(BlueprintCallable, Category = "Skills")
	void CastIceShard();           // 얼음 파편 - 투사체 + 멀티샷

	UFUNCTION(BlueprintCallable, Category = "Skills")
	void CastPoisonNova();         // 독 폭발 - 범위 공격

	UFUNCTION(BlueprintCallable, Category = "Skills")
	void CastPiercingArrow();      // 관통 화살 - 투사체 + 관통

	UFUNCTION(BlueprintCallable, Category = "Skills")
	void CastThunderStrike();      // 천둥 벼락 - 즉발 + 폭발

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

	UFUNCTION(BlueprintCallable)
	void ShowLevelUpUI();

	UFUNCTION(BlueprintCallable)
	void HideLevelUpUI();

	UFUNCTION(BlueprintCallable)
	void OnWeaponSelected(FName WeaponName);

	UFUNCTION(BlueprintCallable)
	void ApplyWeaponUpgrade(FName WeaponID);

	UFUNCTION(BlueprintCallable, Category = "Cube Clear")
	bool ApplyCubeClearReward(const FCubeClearReward& Reward);

	UFUNCTION(BlueprintCallable, Category = "Cube Clear")
	bool ApplyStatUpgrade(EUpgradeType UpgradeType, float UpgradeValue);

	/** Returns CameraBoom subobject */
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject */
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }


	//==============================================================================
	// OVERRIDE FROM BASE CHARACTER
	//==============================================================================

protected:
	// Override ApplyStats to include player-specific stat calculations

	UPROPERTY(Transient)
	bool bHasCapturedBaseStats = false;

	UPROPERTY(Transient)
	float BaseMaxHealthStat = 0.0f;

	UPROPERTY(Transient)
	float BaseMoveSpeedStat = 0.0f;

	virtual void ApplyStats() override;


};
