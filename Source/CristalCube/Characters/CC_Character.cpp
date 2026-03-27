// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_Character.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values
ACC_Character::ACC_Character()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MaxHealth = 100.f;
	BaseMaxHealth = MaxHealth;
	CurrentHealth = MaxHealth;
	MoveSpeed = 600.0f;
	BaseMoveSpeed = MoveSpeed;

	// Apply core movement speed
	GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
}

// Called when the game starts or when spawned
void ACC_Character::BeginPlay()
{
	Super::BeginPlay();
	
	UE_LOG(LogTemp, Warning, TEXT("%s spawned with %.1f health"), *GetName(), CurrentHealth);
}

// Called every frame
void ACC_Character::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ACC_Character::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

float ACC_Character::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// Don't take damage if already dead or damage is negative/zero
	if (DamageAmount <= 0.0f || CurrentHealth <= 0.0f)
	{
		return 0.0f;
	}

	// Calculate actual damage taken
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	// Apply damage to current health
	CurrentHealth -= ActualDamage;
	CurrentHealth = FMath::Max(0.0f, CurrentHealth);

	// Log damage for debugging
	UE_LOG(LogTemp, Log, TEXT("%s took %.1f damage, Health: %.1f/%.1f"),
		*GetName(), ActualDamage, CurrentHealth, MaxHealth);

	// Check if character should die
	if (CurrentHealth <= 0.0f)
	{
		Die();
	}

	return ActualDamage;
}

void ACC_Character::Die()
{
	// Log death
	UE_LOG(LogTemp, Warning, TEXT("%s has died"), *GetName());

	// Disable collision
	SetActorEnableCollision(false);

	// Stop movement
	GetCharacterMovement()->SetMovementMode(MOVE_None);

	// Destroy actor after a short delay to allow for death effects
	FTimerHandle DestroyTimer;
	GetWorld()->GetTimerManager().SetTimer(DestroyTimer, [this]()
		{
			Destroy();
		}, 0.5f, false);
}

float ACC_Character::Heal(float HealAmount)
{
	// Input validation
	if (HealAmount < 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("Negative heal amount rejected: %.1f"), HealAmount);
		return 0.0f;
	}

	// Can't heal the dead
	if (!IsAlive())
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot heal dead character: %s"), *GetName());
		return 0.0f;
	}

	// Calculate actual heal amount (prevent overheal)
	float ActualHealAmount = FMath::Min(HealAmount, MaxHealth - CurrentHealth);

	if (ActualHealAmount > 0.0f)
	{
		CurrentHealth += ActualHealAmount;
		UE_LOG(LogTemp, Log, TEXT("%s healed for %.1f (%.1f/%.1f)"),
			*GetName(), ActualHealAmount, CurrentHealth, MaxHealth);
	}

	return ActualHealAmount;
}

void ACC_Character::ApplyStats()
{
}

