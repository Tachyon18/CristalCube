// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_PlayerCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "../CC_LogHelper.h"
#include "../CC_GameModeBase.h"
#include "../CC_MainGameMode.h"
#include "../CC_PlayerController.h"
#include "../Widgets/CC_GameHUD.h"
#include "../Widgets/CC_LevelUpWidget.h"
#include "../Widgets/CC_SkillInventoryWidget.h"
#include "../CC_TileTrackerComponent.h"
#include "../SkillSystem/CC_SkillSystem.h"
#include "../CC_CollisionHelper.h"
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

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		FCC_CollisionHelper::ConfigureAsPlayerHitbox(GetCapsuleComponent());
	}

	// Initialize player stats (all start at 1.0 = no bonus)
	PlayerStats = FCristalCubePlayerStats();

	TileTrackerComponent = CreateDefaultSubobject<UCC_TileTrackerComponent>(TEXT("TileTrackerComponent"));

	SkillSystem = CreateDefaultSubobject<UCC_SkillSystem>(TEXT("SkillSys"));

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

}

void ACC_PlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// BP Collision Presets가 생성자 설정을 덮어써도 런타임에 역할 콜리전 보장
	FCC_CollisionHelper::ConfigureAsPlayerHitbox(GetCapsuleComponent());

	CC_LOG_PLAYER(Warning, "PlayerCharacter BeginPlay completed - Input handled by PlayerController");
	CC_LOG_PLAYER(Warning, "Camera Distance: %.1f, Angle: %.1f",
		CameraBoom->TargetArmLength,
		CameraBoom->GetRelativeRotation().Pitch);

	// Apply initial stats
	ApplyPlayerStats();

	if (SkillInventoryWidgetClass)
	{
		CurrentSkillInventoryWidget = CreateWidget<UCC_SkillInventoryWidget>(GetWorld(), SkillInventoryWidgetClass);
		if (CurrentSkillInventoryWidget)
		{
			//UE_LOG(LogTemp, Log, TEXT("[Player] Skill Inventory Widget created"));
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

	// AttackSpeed / Damage multipliers: no timer to refresh here (weapon system removed).

	UE_LOG(LogTemp, Log,
		TEXT("[Stats] Refreshed ? HP:%.0f/%.0f | Speed:%.0f | DmgMul:%.2f | AtkSpd:%.2f"),
		CurrentHealth, MaxHealth,
		GetCharacterMovement()->MaxWalkSpeed,
		PlayerStats.BasicStats.DamageMultiplier,
		PlayerStats.BasicStats.AttackSpeedMultiplier);
}

void ACC_PlayerCharacter::PerformAttack()
{
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
		VectorLaser.bCanPenetrate = true;
		VectorLaser.Passives.PierceData.PierceCount = 10;

		// 원소: 번개
		VectorLaser.ElementType = ESkillElementType::Red;

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

	++PendingLevelUpCount;
	ShowLevelUpUI();
}

void ACC_PlayerCharacter::OnPlayerTileChanged(int32 OldTileIndex, int32 NewTileIndex, FVector NewPosition)
{
	UE_LOG(LogTemp, Log, TEXT("[Player] Tile changed: %d -> %d"), OldTileIndex, NewTileIndex);
}

void ACC_PlayerCharacter::UpdateGameHUD()
{
	ACC_PlayerController* PC = Cast<ACC_PlayerController>(GetController());
	UCC_GameHUD* HUD = PC ? PC->GetGameHUD() : nullptr;
	if (!HUD) return;

	HUD->UpdateHealth(CurrentHealth, MaxHealth);
	HUD->UpdateExp(Experience, ExperienceToNextLevel, Level);
	HUD->UpdateTimer(GameTime);
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

	if (ACC_GameModeBase* GM = Cast<ACC_GameModeBase>(UGameplayStatics::GetGameMode(this)))
	{
		TArray<FLevelUpCandidate> Candidates = GM->GetRandomLevelUpCandidates(3);
		CurrentLevelUpUI->SetLevelUpChoices(Candidates);

		CurrentLevelUpUI->OnLevelUpCandidateSelected.AddDynamic(this, &ACC_PlayerCharacter::OnLevelUpCandidateSelected);
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

void ACC_PlayerCharacter::OnLevelUpCandidateSelected(FLevelUpCandidate SelectedCandidate)
{
	UE_LOG(LogTemp, Warning, TEXT("[Player] LevelUp candidate selected: %s"), *SelectedCandidate.DisplayName.ToString());

	if (ACC_GameModeBase* GM = Cast<ACC_GameModeBase>(UGameplayStatics::GetGameMode(this)))
	{
		GM->ApplyLevelUpCandidate(SelectedCandidate);
	}

	PendingLevelUpCount = FMath::Max(0, PendingLevelUpCount - 1);

	// 현재 위젯을 먼저 정리 ? ShowLevelUpUI()의 CurrentLevelUpUI 가드 때문에
	// 지우지 않으면 다음 레벨업 카드가 안 뜬다.
	if (CurrentLevelUpUI)
	{
		CurrentLevelUpUI->RemoveFromParent();
		CurrentLevelUpUI = nullptr;
	}

	if (PendingLevelUpCount > 0)
	{
		// 아직 처리 안 된 레벨업이 남아있음 ? 게임은 계속 멈춘 채로 다음 카드 바로 표시
		ShowLevelUpUI();
	}
	else
	{
		HideLevelUpUI();
	}
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
