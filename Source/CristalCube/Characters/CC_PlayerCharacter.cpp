// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_PlayerCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "../CC_LogHelper.h"
#include "../CC_WeaponManagerSubsystem.h"
#include "../CC_SearchingComponent.h"
#include "../CC_GameModeBase.h"
#include "../Widgets/CC_GameHUD.h"
#include "../Widgets/CC_LevelUpWidget.h"
#include "../CC_TileTrackerComponent.h"
#include "../SkillSystem/CC_SkillSystem.h"
#include "CC_EnemyCharacter.h"


ACC_PlayerCharacter::ACC_PlayerCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = false; // Character doesn't rotate in direction of movement
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 640.0f, 0.0f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;


	// Initialize player stats (all start at 1.0 = no bonus)
	PlayerStats = FCristalCubePlayerStats();

	SearchingComponent = CreateDefaultSubobject<UCC_SearchingComponent>(TEXT("SearchingComponent"));

	TileTrackerComponent = CreateDefaultSubobject<UCC_TileTrackerComponent>(TEXT("TileTrackerComponent"));

	SkillSystem = CreateDefaultSubobject<UCC_SkillSystem>(TEXT("SkillSystem"));

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 800.0f;           // Camera distance
	CameraBoom->SetRelativeRotation(FRotator(-60.0f, 0.0f, 0.0f));  // Top-down angle
	CameraBoom->bDoCollisionTest = false;

	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;  // Camera does not rotate relative to arm

	// Initialize leveling system
	Level = 1;
	Experience = 0.0f;
	BaseExperienceRequirement = 100.f;
	ExperienceScaling = 1.15f;
	ExperienceToNextLevel = BaseExperienceRequirement;

	// Initialize weapon pointers
	CurrentWeapon = nullptr;

	MaxWeapons = 6;
	StartingWeaponClass = nullptr;

}

void ACC_PlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	CC_LOG_PLAYER(Warning, "PlayerCharacter BeginPlay completed - Input handled by PlayerController");
	CC_LOG_PLAYER(Warning, "Camera Distance: %.1f, Angle: %.1f",
		CameraBoom->TargetArmLength,
		CameraBoom->GetRelativeRotation().Pitch);

	// Apply initial stats
	ApplyPlayerStats();

	if (StartingWeaponClass)
	{
		CreateAndEquipWeapon(StartingWeaponClass);
	}
	else
	{
		CC_LOG_PLAYER(Warning, "No StartingWeaponClass set");

		InitializeWeaponSystem();
		EquipStartingWeapons();
	}

	if (GameHUDClass)
	{
		CurrentGameHUD = CreateWidget<UCC_GameHUD>(GetWorld(), GameHUDClass);
		if (CurrentGameHUD)
		{
			CurrentGameHUD->AddToViewport();
			UpdateGameHUD();
			UE_LOG(LogTemp, Log, TEXT("[Player] Game HUD created"));
		}
		
	}

	if (TileTrackerComponent)
	{
		TileTrackerComponent->OnCurrentTileChanged.AddDynamic(
			this,
			&ACC_PlayerCharacter::OnPlayerTileChanged
		);
	}
}

void ACC_PlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	GameTime += DeltaTime;

	// HUD Update
	static float HUDUpdateTimer = 0.0f;
	HUDUpdateTimer += DeltaTime;
	if (HUDUpdateTimer >= 0.1f)
	{
		UpdateGameHUD();
		HUDUpdateTimer = 0.0f;
	}
}

void ACC_PlayerCharacter::ApplyPlayerStats()
{
	ApplyStats();
}

void ACC_PlayerCharacter::ApplyStatUpgrade(EUpgradeType Type, float Value)
{
	switch (Type)
	{
		// ── BasicStats ───────────────────────────────────────
	case EUpgradeType::Damage:
		PlayerStats.BasicStats.DamageMultiplier += Value;
		break;

	case EUpgradeType::AttackSpeed:
		PlayerStats.BasicStats.AttackSpeedMultiplier += Value;
		break;

	case EUpgradeType::MoveSpeed:
		PlayerStats.BasicStats.MoveSpeedMultiplier += Value;
		break;

	case EUpgradeType::Health:
		PlayerStats.BasicStats.HealthMultiplier += Value;
		break;

		// ── AdvancedStats (EUpgradeType 확장 후 활성화) ───────
		// case EUpgradeType::CritChance:
		//     PlayerStats.AdvancedStats.CriticalChance += Value;
		//     break;

	default:
		UE_LOG(LogTemp, Warning,
			TEXT("[Stats] ApplyStatUpgrade: Unhandled EUpgradeType(%d)"),
			static_cast<int32>(Type));
		return;  // 알 수 없는 타입 → 적용 없이 리턴
	}

	UE_LOG(LogTemp, Log, TEXT("[Stats] Upgraded %d by %.3f"), static_cast<int32>(Type), Value);

	// 변경된 스탯만 UE 시스템에 반영
	RefreshStats();
}

void ACC_PlayerCharacter::ApplyStatOverride(const FCristalCubePlayerStats& InStats)
{
	PlayerStats = InStats;

	UE_LOG(LogTemp, Log, TEXT("[Stats] StatOverride applied ? full refresh"));

	RefreshStats();
}

void ACC_PlayerCharacter::RefreshStats()
{
	// ── 이동속도 ─────────────────────────────────────────────
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		float FinalSpeed = BaseMoveSpeed * PlayerStats.BasicStats.MoveSpeedMultiplier;
		Movement->MaxWalkSpeed = FinalSpeed;
	}

	// ── 체력 상한 ─────────────────────────────────────────────
	float NewMaxHealth = BaseMaxHealth * PlayerStats.BasicStats.HealthMultiplier;

	if (FMath::IsNearlyEqual(CurrentHealth, MaxHealth))
	{
		CurrentHealth = NewMaxHealth;
	}
	else
	{
		float Ratio = MaxHealth > 0.f ? (CurrentHealth / MaxHealth) : 1.f;
		CurrentHealth = FMath::Clamp(NewMaxHealth * Ratio, 0.f, NewMaxHealth);
	}
	MaxHealth = NewMaxHealth;

	// ── 공격속도 (무기 타이머 재설정) ────────────────────────
	// AttackSpeedMultiplier는 Weapon->GetAttackSpeed()에서 직접 참조하므로
	// 타이머만 재시작하면 자동 반영
	for (ACC_Weapon* Weapon : EquippedWeapons)
	{
		if (!IsValid(Weapon)) continue;
		StopWeaponAutoAttack(Weapon);
		StartWeaponAutoAttack(Weapon);
	}

	// ── DamageMultiplier, AdvancedStats 등 ────────────────────
	// 무기 공격 시 GetFinalDamageMultiplier()로 조회되므로 별도 처리 불필요

	UE_LOG(LogTemp, Log,
		TEXT("[Stats] Refreshed ? HP:%.0f/%.0f | Speed:%.0f | DmgMul:%.2f | AtkSpd:%.2f"),
		CurrentHealth, MaxHealth,
		GetCharacterMovement()->MaxWalkSpeed,
		PlayerStats.BasicStats.DamageMultiplier,
		PlayerStats.BasicStats.AttackSpeedMultiplier);
}

void ACC_PlayerCharacter::InitializeWeaponSystem()
{
	UGameInstance* GameInstance = GetGameInstance();
	if(!GameInstance)
	{
		CC_LOG_PLAYER(Error, "Cannot initialize Weapon System - GameInstance is null");
		return;
	}

	WeaponManager = GameInstance->GetSubsystem<UCC_WeaponManagerSubsystem>();

	if (!WeaponManager)
	{
		CC_LOG_PLAYER(Error, "Cannot initialize Weapon System - WeaponManagerSubsystem is null");
		return;
	}

	if (WeaponDataTable)
	{
		WeaponManager->LoadWeaponDataTable(WeaponDataTable);
		CC_LOG_PLAYER(Log, "Weapon System Initialized");
	}
	else
	{
		CC_LOG_PLAYER(Warning, "No WeaponDataTable set - cannot load weapon data");
	}
}

void ACC_PlayerCharacter::EquipStartingWeapons()
{
	if (!WeaponManager)
	{
		CC_LOG_PLAYER(Error, TEXT("WeaponManager Not Initialized!"));
		return;
	}

	TArray<FName> StartingWeaponNames = WeaponManager->GetStartingWeaponNames();

	if (StartingWeaponNames.Num() == 0)
	{
		CC_LOG_PLAYER(Error, TEXT("No Starting weapons available"));
		return;
	}

	TArray<FName> SelectedWeaponNames =
		WeaponManager->GetRandomWeaponNames(StartingWeaponNames, StartingWeaponCount, false);

	for (FName WeaponName : SelectedWeaponNames)
	{
		TSubclassOf<ACC_Weapon> WeaponClass = WeaponManager->GetWeaponClass(WeaponName);

		if (WeaponClass)
		{
			CreateAndEquipWeapon(WeaponClass);

			FWeaponData WeaponData;
			if (WeaponManager->GetWeaponData(WeaponName, WeaponData))
			{
				CC_LOG_PLAYER(Log, TEXT("Equipped: %s"), *WeaponData.WeaponName.ToString());
			}
		}
	}
}

void ACC_PlayerCharacter::EquipWeaponByName(FName WeaponRowName)
{
	if (!WeaponManager)
	{
		CC_LOG_PLAYER(Error, TEXT("WeaponManager not initialized!"));
		return;
	}

	TSubclassOf<ACC_Weapon> WeaponClass = WeaponManager->GetWeaponClass(WeaponRowName);

	if (WeaponClass)
	{
		CreateAndEquipWeapon(WeaponClass);

		FWeaponData WeaponData;
		if (WeaponManager->GetWeaponData(WeaponRowName, WeaponData))
		{
			CC_LOG_PLAYER(Log, TEXT("Equipped weapon: %s"), *WeaponData.WeaponName.ToString());
		}
	}
}

bool ACC_PlayerCharacter::EquipWeapon(ACC_Weapon* Weapon)
{
	// Validation
	if (!Weapon)
	{
		CC_LOG_PLAYER(Warning, "Cannot equip weapon - weapon is null");
		return false;
	}

	if (!CanEquipMoreWeapons())
	{
		CC_LOG_PLAYER(Warning, "Cannot equip more weapons (Max: %d)", MaxWeapons);
		return false;
	}

	if (HasWeapon(Weapon))
	{
		CC_LOG_PLAYER(Warning, "Weapon already equipped");
		return false;
	}

	// Setup weapon
	Weapon->OnEquipped(this);

	// Add to equipped weapons list
	EquippedWeapons.Add(Weapon);

	// Start auto-attack
	StartWeaponAutoAttack(Weapon);

	CC_LOG_PLAYER(Log, "Equipped weapon (Total: %d/%d)",
		EquippedWeapons.Num(), MaxWeapons);

	return true;
}

ACC_Weapon* ACC_PlayerCharacter::CreateAndEquipWeapon(TSubclassOf<ACC_Weapon> WeaponClass)
{
	if (!WeaponClass)
	{
		CC_LOG_PLAYER(Warning, "Cannot create weapon - invalid class");
		return nullptr;
	}

	if (!CanEquipMoreWeapons())
	{
		CC_LOG_PLAYER(Warning, "Cannot equip more weapons (Max: %d)", MaxWeapons);
		return nullptr;
	}

	// Create weapon
	ACC_Weapon* NewWeapon = CreateWeapon(WeaponClass);

	if (!NewWeapon)
	{
		CC_LOG_PLAYER(Error, "Failed to create weapon!");
		return nullptr;
	}

	// Equip it
	if (EquipWeapon(NewWeapon))
	{
		return NewWeapon;
	}
	else
	{
		// Failed to equip, destroy it
		NewWeapon->Destroy();
		return nullptr;
	}
}

void ACC_PlayerCharacter::UnequipWeapon(ACC_Weapon* Weapon)
{
	if (!Weapon)
	{
		CC_LOG_PLAYER(Warning, "Cannot unequip - weapon is null");
		return;
	}

	if (!HasWeapon(Weapon))
	{
		CC_LOG_PLAYER(Warning, "Weapon is not equipped");
		return;
	}

	// Stop auto-attack
	StopWeaponAutoAttack(Weapon);

	// Remove from list
	EquippedWeapons.Remove(Weapon);

	// Cleanup weapon
	Weapon->OnUnequipped();
	Weapon->Destroy();

	CC_LOG_PLAYER(Log, "Unequipped weapon (Remaining: %d)", EquippedWeapons.Num());

}

void ACC_PlayerCharacter::UnequipAllWeapons()
{
	// Create copy of array to iterate (since we're modifying the original)
	TArray<ACC_Weapon*> WeaponsCopy = EquippedWeapons;

	for (ACC_Weapon* Weapon : WeaponsCopy)
	{
		UnequipWeapon(Weapon);
	}

	CC_LOG_PLAYER(Log, "Unequipped all weapons");
}

void ACC_PlayerCharacter::SwitchWeapon()
{

}

void ACC_PlayerCharacter::PerformAttack()
{
	if (CurrentWeapon && CurrentWeapon->CanAttack())
	{
		CurrentWeapon->Attack();
	}
}

ACC_Weapon* ACC_PlayerCharacter::FindWeaponByClass(TSubclassOf<ACC_Weapon> WeaponClass) const
{
	if (!WeaponClass)
	{
		return nullptr;
	}

	for (ACC_Weapon* Weapon : EquippedWeapons)
	{
		if (Weapon && Weapon->GetClass() == WeaponClass)
		{
			return Weapon;
		}
	}

	return nullptr;
}

ACC_Weapon* ACC_PlayerCharacter::CreateWeapon(TSubclassOf<ACC_Weapon> WeaponClass)
{
	if (!WeaponClass)
	{
		return nullptr;
	}

	// Spawn parameters
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Spawn weapon
	ACC_Weapon* NewWeapon = GetWorld()->SpawnActor<ACC_Weapon>(
		WeaponClass,
		GetActorLocation(),
		FRotator::ZeroRotator,
		SpawnParams
	);

	if (NewWeapon)
	{
		CC_LOG_PLAYER(VeryVerbose, "Created weapon from class");
	}
	else
	{
		CC_LOG_PLAYER(Error, "Failed to spawn weapon!");
	}

	return NewWeapon;
}

void ACC_PlayerCharacter::StartWeaponAutoAttack(ACC_Weapon* Weapon)
{
	if (!Weapon)
	{
		CC_LOG_PLAYER(Warning, "Cannot start auto-attack - weapon is null");
		return;
	}

	// Get attack speed
	float AttackSpeed = Weapon->GetAttackSpeed();
	if (AttackSpeed <= 0.0f)
	{
		CC_LOG_PLAYER(Warning, "Invalid attack speed: %.2f", AttackSpeed);
		return;
	}

	// Calculate interval
	float AttackInterval = 1.0f / AttackSpeed;

	// Create timer handle
	FTimerHandle AttackTimer;

	// Set up repeating timer
	GetWorld()->GetTimerManager().SetTimer(
		AttackTimer,
		[Weapon]()
		{
			if (IsValid(Weapon))
			{
				Weapon->Attack();
			}
		},
		AttackInterval,
		true  // Repeating
	);

	// Store timer handle
	WeaponAttackTimers.Add(Weapon, AttackTimer);

	CC_LOG_PLAYER(Log, "Started auto-attack (Interval: %.2fs, Rate: %.1f/s)",
		AttackInterval, AttackSpeed);
}

void ACC_PlayerCharacter::StopWeaponAutoAttack(ACC_Weapon* Weapon)
{
	if (!Weapon)
	{
		return;
	}

	// Find and clear timer
	FTimerHandle* TimerHandle = WeaponAttackTimers.Find(Weapon);
	if (TimerHandle && TimerHandle->IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(*TimerHandle);
		WeaponAttackTimers.Remove(Weapon);

		CC_LOG_PLAYER(VeryVerbose, "Stopped auto-attack for weapon");
	}
}

void ACC_PlayerCharacter::TestBasicSkill()
{
}

void ACC_PlayerCharacter::TestProjectileSkill()
{
	UCC_SkillSystem* TestSkillSystem = FindComponentByClass<UCC_SkillSystem>();
	if (!TestSkillSystem)
	{
		return;
	}

	// 간단한 테스트 스킬 (VFX 없음)
	FSkillDefinition TestSkill;
	TestSkill.SkillID = FName("TestProj");
	TestSkill.CoreType = ESkillCoreType::Projectile;
	TestSkill.BaseDamage = 50.0f;
	TestSkill.Range = 2000.0f;
	TestSkill.Passives.ProjectileCount = 1;

	// Addon 테스트
	TestSkill.Addons.Add(ESkillAddonType::MultiShot);
	TestSkill.Addons.Add(ESkillAddonType::Penetrate);
	TestSkill.Passives.PierceData.PierceCount = 3;

	// VFX는 nullptr로 (안전)
	TestSkill.CastEffect = nullptr;
	TestSkill.HitEffect = nullptr;

	// 마우스 방향으로 발사
	FVector TargetLocation = GetActorLocation() + GetActorForwardVector() * 1000.0f;
	SkillSystem->ExecuteSkill(TestSkill, TargetLocation);

	UE_LOG(LogTemp, Warning, TEXT("Test skill fired!"));
}

void ACC_PlayerCharacter::CastFireball()
{
	if (!SkillSystem)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Test] Casting Fireball!"));

	// 화염구: 투사체 + 폭발
	FSkillDefinition Fireball;
	Fireball.SkillID = FName("Fireball");
	Fireball.DisplayName = FText::FromString("Fireball");
	Fireball.CoreType = ESkillCoreType::Projectile;
	Fireball.BaseDamage = 40.0f;
	Fireball.Range = 2000.0f;
	Fireball.Cooldown = 1.5f;

	// 폭발 효과
	Fireball.Addons.Add(ESkillAddonType::Explosion);
	Fireball.Passives.SizeMultiplier = 1.5f;  // 폭발 범위 증가
	Fireball.Passives.ProjectileCount = 1;

	// 원소: 화염
	Fireball.ElementType = ESkillElementType::Fire;

	FVector TargetLocation = GetActorLocation() + GetActorForwardVector() * 1000.0f;
	SkillSystem->ExecuteSkill(Fireball, TargetLocation);

	
}

void ACC_PlayerCharacter::CastLightningBolt()
{
	if (!SkillSystem)
	{
		return;
	}

	// 번개: 즉발 + 연쇄
	FSkillDefinition Lightning;
	Lightning.SkillID = FName("Lightning");
	Lightning.DisplayName = FText::FromString("Lightning Bolt");
	Lightning.CoreType = ESkillCoreType::Instant;
	Lightning.BaseDamage = 35.0f;
	Lightning.Range = 1500.0f;
	Lightning.Cooldown = 0.8f;

	if (TestLightningEffect)
	{
		Lightning.HitEffect = TestLightningEffect;
	}

	// 연쇄 효과 (최대 4번)
	Lightning.Addons.Add(ESkillAddonType::Chain);
	Lightning.Passives.ChainData.ChainCount = 4;

	// 원소: 번개
	Lightning.ElementType = ESkillElementType::Lightning;

	FVector TargetLocation = GetActorLocation() + GetActorForwardVector() * 1000.0f;
	SkillSystem->ExecuteSkill(Lightning, TargetLocation);

	UE_LOG(LogTemp, Warning, TEXT("Lightning Bolt cast!"));
}

void ACC_PlayerCharacter::CastIceShard()
{
	if (!SkillSystem)
	{
		return;
	}

	// 얼음 파편: 투사체 + 멀티샷
	FSkillDefinition IceShard;
	IceShard.SkillID = FName("IceShard");
	IceShard.DisplayName = FText::FromString("Ice Shard");
	IceShard.CoreType = ESkillCoreType::Projectile;
	IceShard.BaseDamage = 20.0f;
	IceShard.Range = 1800.0f;
	IceShard.Cooldown = 0.5f;

	// 멀티샷 (3발 → 5발)
	IceShard.Addons.Add(ESkillAddonType::MultiShot);
	IceShard.Passives.ProjectileCount = 3;
	IceShard.Passives.SpeedMultiplier = 1.2f;

	// 원소: 얼음
	IceShard.ElementType = ESkillElementType::Ice;

	FVector TargetLocation = GetActorLocation() + GetActorForwardVector() * 1000.0f;
	SkillSystem->ExecuteSkill(IceShard, TargetLocation);

	UE_LOG(LogTemp, Warning, TEXT("Ice Shard cast!"));
}

void ACC_PlayerCharacter::CastPoisonNova()
{
	if (!SkillSystem)
	{
		return;
	}

	// 독 폭발: 범위 공격
	FSkillDefinition PoisonNova;
	PoisonNova.SkillID = FName("PoisonNova");
	PoisonNova.DisplayName = FText::FromString("Poison Nova");
	PoisonNova.CoreType = ESkillCoreType::Area;
	PoisonNova.BaseDamage = 30.0f;
	PoisonNova.Range = 400.0f;  // 반경
	PoisonNova.Cooldown = 3.0f;

	// 범위 증가
	PoisonNova.Passives.SizeMultiplier = 2.0f;

	// 원소: 독
	PoisonNova.ElementType = ESkillElementType::Poison;

	// 플레이어 위치에서 발동
	FVector TargetLocation = GetActorLocation();
	SkillSystem->ExecuteSkill(PoisonNova, TargetLocation);

	UE_LOG(LogTemp, Warning, TEXT("Poison Nova cast!"));
}

void ACC_PlayerCharacter::CastPiercingArrow()
{
	if (!SkillSystem)
	{
		return;
	}

	// 관통 화살: 투사체 + 관통
	FSkillDefinition Arrow;
	Arrow.SkillID = FName("PiercingArrow");
	Arrow.DisplayName = FText::FromString("Piercing Arrow");
	Arrow.CoreType = ESkillCoreType::Projectile;
	Arrow.BaseDamage = 45.0f;
	Arrow.Range = 2500.0f;
	Arrow.Cooldown = 1.0f;

	// 관통 (5명)
	Arrow.Addons.Add(ESkillAddonType::Penetrate);
	Arrow.Passives.PierceData.PierceCount = 5;
	Arrow.Passives.SpeedMultiplier = 1.5f;

	// 원소: 물리
	Arrow.ElementType = ESkillElementType::Physical;

	FVector TargetLocation = GetActorLocation() + GetActorForwardVector() * 1000.0f;
	SkillSystem->ExecuteSkill(Arrow, TargetLocation);

	UE_LOG(LogTemp, Warning, TEXT("Piercing Arrow cast!"));
}

void ACC_PlayerCharacter::CastThunderStrike()
{
	if (!SkillSystem)
	{
		return;
	}

	FSkillDefinition ThunderStrike;
	ThunderStrike.SkillID = FName("ThunderStrike");
	ThunderStrike.DisplayName = FText::FromString("Thunder Strike");
	ThunderStrike.CoreType = ESkillCoreType::Instant;
	ThunderStrike.BaseDamage = 50.0f;
	ThunderStrike.Range = 1200.0f;
	ThunderStrike.Cooldown = 2.0f;

	if (TestLightningEffect)
	{
		ThunderStrike.HitEffect = TestLightningEffect;
	}

	// Explosion Addon
	ThunderStrike.Addons.Add(ESkillAddonType::Explosion);
	ThunderStrike.Passives.ExplosionData.Radius = 350.0f;
	ThunderStrike.Passives.ExplosionData.MinDamageRatio = 0.5f;

	// 속성: 번개
	ThunderStrike.ElementType = ESkillElementType::Lightning;

	FVector TargetLocation = GetActorLocation() + GetActorForwardVector() * 1000.0f;
	SkillSystem->ExecuteSkill(ThunderStrike, TargetLocation);

	UE_LOG(LogTemp, Warning, TEXT("[Test] Thunder Strike cast!"));
}

void ACC_PlayerCharacter::StartVectorLaser()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	// 마우스 커서 위치를 월드 좌표로 변환
	FHitResult HitResult;
	PC->GetHitResultUnderCursor(ECC_Visibility, false, HitResult);

	if (HitResult.bBlockingHit)
	{
		LaserStartPoint = HitResult.ImpactPoint;
		bIsChargingLaser = true;

		// 시작점 시각 피드백
		if (GetWorld())
		{
			DrawDebugSphere(
				GetWorld(),
				LaserStartPoint,
				50.0f,
				16,
				FColor::Cyan,
				false,
				5.0f,
				0,
				3.0f
			);
		}

		UE_LOG(LogTemp, Log, TEXT("Laser charging at: %s"), *LaserStartPoint.ToString());
	}
}

void ACC_PlayerCharacter::ReleaseVectorLaser()
{
	if (!bIsChargingLaser || !SkillSystem)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	// 마우스 릴리즈 위치
	FHitResult HitResult;
	PC->GetHitResultUnderCursor(ECC_Visibility, false, HitResult);

	if (HitResult.bBlockingHit)
	{
		FVector LaserEndPoint = HitResult.ImpactPoint;

		// 벡터 레이저: Beam Core
		FSkillDefinition VectorLaser;
		VectorLaser.SkillID = FName("VectorLaser");
		VectorLaser.DisplayName = FText::FromString("Vector Laser");
		VectorLaser.CoreType = ESkillCoreType::Beam;
		VectorLaser.BaseDamage = 60.0f;
		VectorLaser.Range = 3000.0f;
		VectorLaser.Cooldown = 2.0f;

		// 관통 추가
		VectorLaser.Addons.Add(ESkillAddonType::Penetrate);
		VectorLaser.Passives.PierceData.PierceCount = 10;

		// 원소: 번개
		VectorLaser.ElementType = ESkillElementType::Lightning;

		// Context 설정 (시작점 → 끝점)
		FSkillExecutionContext Context;
		Context.Caster = this;
		Context.StartLocation = LaserStartPoint;
		Context.TargetLocation = LaserEndPoint;
		Context.Direction = (LaserEndPoint - LaserStartPoint).GetSafeNormal();
		Context.CurrentDamage = VectorLaser.BaseDamage;

		// 실행!
		SkillSystem->ExecuteSkill(VectorLaser, LaserEndPoint);

		UE_LOG(LogTemp, Warning, TEXT("Vector Laser fired from %s to %s"),
			*LaserStartPoint.ToString(), *LaserEndPoint.ToString());
	}

	bIsChargingLaser = false;
}


void ACC_PlayerCharacter::LevelUp()
{
	Level++;
	Experience -= ExperienceToNextLevel;

	// Increase XP requirement for next level (simple scaling / Request to change to refined variables in the future)
	ExperienceToNextLevel *= ExperienceScaling;

	// Heal to full on level up
	RefreshStats();
	Heal(MaxHealth);

	UE_LOG(LogTemp, Log, TEXT("Level Up! Now level %d"), Level);

	UpdateGameHUD();
	ShowLevelUpUI();
}

void ACC_PlayerCharacter::OnPlayerTileChanged(int32 OldTileIndex, int32 NewTileIndex, FVector NewPosition)
{
	UE_LOG(LogTemp, Log, TEXT("[Player] Tile changed: %d -> %d"), OldTileIndex, NewTileIndex);
}

void ACC_PlayerCharacter::UpdateGameHUD()
{
	if (!CurrentGameHUD) return;

	CurrentGameHUD->UpdateHealth(CurrentHealth, MaxHealth);
	CurrentGameHUD->UpdateExp(Experience, ExperienceToNextLevel, Level);
	CurrentGameHUD->UpdateTimer(GameTime);

}

float ACC_PlayerCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float PreviousHealth = CurrentHealth;
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	// Base character damage flow owns health reduction and death checks.
	// The player override only syncs the HUD after health actually changes.
	if (ActualDamage > 0.0f && !FMath::IsNearlyEqual(CurrentHealth, PreviousHealth))
	{
		UpdateGameHUD();
	}

	return ActualDamage;
}

void ACC_PlayerCharacter::AddExperience(float ExpAmount)
{
	Experience += ExpAmount;

	UpdateGameHUD();

	// Check for level up
	while (Experience >= ExperienceToNextLevel)
	{
		LevelUp();
	}
}

float ACC_PlayerCharacter::GetExperiencePercentage() const
{
	if (ExperienceToNextLevel <= 0.0f)
	{
		return 0.0f;
	}
	return Experience / ExperienceToNextLevel;
}

void ACC_PlayerCharacter::ShowLevelUpUI()
{
	if (!LevelUpUIClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[Player] LevelUpUIClass not set!"));
		return;
	}

	if (CurrentLevelUpUI)
	{
		return;
	}

	CurrentLevelUpUI = CreateWidget<UCC_LevelUpWidget>(GetWorld(), LevelUpUIClass);

	if (!CurrentLevelUpUI)
	{
		UE_LOG(LogTemp, Error, TEXT("[Player] Failed to create LevelUpUI"));
		return;
	}

	if(WeaponManager)
	{
		TArray<FName> Options = WeaponManager->GetRandomWeaponNames(WeaponManager->GetAllWeaponNames(), 3);
		CurrentLevelUpUI->SetWeaponChoices(Options);

		CurrentLevelUpUI->OnWeaponSelected.AddDynamic(this, &ACC_PlayerCharacter::OnWeaponSelected);
	}

	CurrentLevelUpUI->AddToViewport(10);

	UGameplayStatics::SetGamePaused(GetWorld(), true);

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC)
	{
		PC->bShowMouseCursor = true;
		PC->SetInputMode(FInputModeUIOnly());
	}

	UE_LOG(LogTemp, Log, TEXT("[Player] Level Up UI shown"));
	//if (LevelUpWidgetClass)
	//{
	//	UUserWidget* Widget = CreateWidget<UUserWidget>(
	//		GetWorld(),
	//		LevelUpWidgetClass
	//	);
	//	Widget->AddToViewport();
	//}

}

void ACC_PlayerCharacter::HideLevelUpUI()
{
	if (CurrentLevelUpUI)
	{
		CurrentLevelUpUI->RemoveFromParent();
		CurrentLevelUpUI = nullptr;
	}

	UGameplayStatics::SetGamePaused(GetWorld(), false);

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC)
	{
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
	}

	UE_LOG(LogTemp, Log, TEXT("[Player] Level Up UI hidden"));
}

void ACC_PlayerCharacter::OnWeaponSelected(FName WeaponName)
{
	UE_LOG(LogTemp, Warning, TEXT("[Player] Weapon Selected: %s"), *WeaponName.ToString());

	if (WeaponManager)
	{
		FWeaponData* WeaponData = WeaponManager->GetWeaponDataPtr(WeaponName);
		if (WeaponData && WeaponData->WeaponClass)
		{
			CreateAndEquipWeapon(WeaponData->WeaponClass);
		}
	}

	HideLevelUpUI();
}

void ACC_PlayerCharacter::ApplyWeaponUpgrade(FName WeaponID)
{
	UCC_WeaponManagerSubsystem* WeaponMgr = GetGameInstance()
		->GetSubsystem<UCC_WeaponManagerSubsystem>();

	FWeaponData* Data = WeaponMgr->GetWeaponDataPtr(WeaponID);

	if (Data && Data->WeaponClass)
	{
		CreateAndEquipWeapon(Data->WeaponClass);
	}

	UGameplayStatics::SetGamePaused(GetWorld(), false);
}

void ACC_PlayerCharacter::ApplyStats()
{
	Super::ApplyStats();

	//if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	//{
	//	float AddMoveSpeed = BaseMoveSpeed * PlayerStats.BasicStats.MoveSpeedMultiplier;
	//	float FinalMoveSpeed = MoveSpeed + AddMoveSpeed;
	//	Movement->MaxWalkSpeed = FinalMoveSpeed;
	//}

	//// Apply health multiplier
	//float AddMaxHealth = BaseMaxHealth * PlayerStats.BasicStats.HealthMultiplier;
	//float FinalMaxHealth = MaxHealth + AddMaxHealth;

	//// If health was at max, keep it at max after stat change
	//if (FMath::IsNearlyEqual(CurrentHealth, MaxHealth))
	//{
	//	CurrentHealth = FinalMaxHealth;
	//}
	//else
	//{
	//	// Otherwise, adjust current health proportionally
	//	float HealthPercentage = CurrentHealth / MaxHealth;
	//	CurrentHealth = FinalMaxHealth * HealthPercentage;
	//}

	//MaxHealth = FinalMaxHealth;

	//UE_LOG(LogTemp, Log, TEXT("Stats Applied - Health: %.0f/%.0f, Speed: %.0f"),
	//	CurrentHealth, MaxHealth, GetCharacterMovement()->MaxWalkSpeed);

	ApplyStatOverride(PlayerStats);
}

void ACC_PlayerCharacter::Die()
{
	Super::Die();

	if (ACC_GameModeBase* GM = Cast<ACC_GameModeBase>(UGameplayStatics::GetGameMode(this)))
	{
		GM->TriggerGameOver();
	}
}
