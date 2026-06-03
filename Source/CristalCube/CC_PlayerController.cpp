// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "Kismet/GameplayStatics.h"
#include "SkillSystem/CC_SkillSystem.h"
#include "Characters/CC_PlayerCharacter.h"


ACC_PlayerController::ACC_PlayerController()
{
    bShowMouseCursor = true; 
    bEnableClickEvents = true;
    bEnableMouseOverEvents = false;

    PrimaryActorTick.bCanEverTick = true;

    CC_LOG_PLAYER(Warning, "PlayerController initialized");
}

void ACC_PlayerController::BeginPlay()
{
	Super::BeginPlay();
	
    if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
        ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        if (DefaultMappingContext)
        {
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
            CC_LOG_PLAYER(Log, "Input mapping context added");
        }
        else
        {
            CC_LOG_PLAYER(Warning, "DefaultMappingContext is null!");
        }
    }

    ControlledCharacter = Cast<ACC_PlayerCharacter>(GetPawn());
    if (ControlledCharacter)
    {
        CC_LOG_PLAYER(Log, "PlayerCharacter reference cached: %s", CC_ACTOR_NAME(ControlledCharacter));
    }
    else
    {
        CC_LOG_PLAYER(Warning, "Failed to cast pawn to CC_PlayerCharacter");
    }
}

void ACC_PlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void ACC_PlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    ControlledCharacter = Cast<ACC_PlayerCharacter>(InPawn);

    if (!ControlledCharacter)
    {
        CC_LOG_PLAYER(Warning, "OnPossess: Pawn is not ACC_PlayerCharacter (%s)", InPawn ? CC_ACTOR_NAME(InPawn) : TEXT("null"));
        return;
    }

    CC_LOG_PLAYER(Log, "OnPossess: ControlledCharacter cached ? %s",
        CC_ACTOR_NAME(ControlledCharacter));

    SetInputMode(FInputModeGameAndUI());
    bShowMouseCursor = true;

    CC_LOG_PLAYER(Log, "OnPossess: InputMode reset to GameOnly");
}

void ACC_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
    {
		CC_LOG_PLAYER(Warning, "Enhanced Input Component found");

        // Movement
        if (MoveAction)
        {
            EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ACC_PlayerController::HandleMove);
            EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &ACC_PlayerController::HandleMove);

			CC_LOG_PLAYER(Warning, "Move action bound successfully");
        }

        // Primary Attack
        if (PrimaryAttackAction)
        {
            EnhancedInputComponent->BindAction(PrimaryAttackAction, ETriggerEvent::Started, this, &ACC_PlayerController::HandlePrimaryAttack);
            EnhancedInputComponent->BindAction(PrimaryAttackAction, ETriggerEvent::Completed, this, &ACC_PlayerController::HandlePrimaryAttack);
        }

        // Secondary Attack
        if (SecondaryAttackAction)
        {
            EnhancedInputComponent->BindAction(SecondaryAttackAction, ETriggerEvent::Started, this, &ACC_PlayerController::HandleSecondaryAttack);
            EnhancedInputComponent->BindAction(SecondaryAttackAction, ETriggerEvent::Completed, this, &ACC_PlayerController::HandleSecondaryAttack);
        }

        // Dash
        if (DashAction)
        {
            EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Started, this, &ACC_PlayerController::HandleDash);
        }

		InputComponent->BindAction("BeamCharge", IE_Pressed, this, &ACC_PlayerController::StartBeamCharge);
		InputComponent->BindAction("BeamCharge", IE_Released, this, &ACC_PlayerController::ReleaseBeam);

        CC_LOG_PLAYER(Log, "Enhanced Input actions bound successfully");
    }
    else
    {
        CC_LOG_PLAYER(Error, "Failed to cast InputComponent to EnhancedInputComponent");
    }
}

void ACC_PlayerController::HandleMove(const FInputActionValue& Value)
{
    //UE_LOG(LogTemp, Warning, TEXT("[DEBUG] HandleMove called, CC: %s"), ControlledCharacter ? TEXT("VALID") : TEXT("NULL"));

	CurrentMoveInput = Value.Get<FVector2D>();


    if (ControlledCharacter && !CurrentMoveInput.IsZero())
    {
		const FVector ForwardDirection = FVector(0.0f, 1.0f, 0.0f);
		const FVector RightDirection = FVector(1.0f, 0.0f, 0.0f);

        ControlledCharacter->AddMovementInput(ForwardDirection, CurrentMoveInput.Y);  // W/S
        ControlledCharacter->AddMovementInput(RightDirection, CurrentMoveInput.X);    // A/D

        CC_LOG_PLAYER(VeryVerbose, "Movement input: X=%.2f Y=%.2f",
            CurrentMoveInput.X, CurrentMoveInput.Y);
    }
}

void ACC_PlayerController::HandlePrimaryAttack(const FInputActionValue& Value)
{
    bool bIsPressed = Value.Get<bool>();
    bIsPrimaryAttacking = bIsPressed;

    if (ControlledCharacter)
    {
        if (bIsPressed)
        {
            ControlledCharacter->PerformAttack();
            CC_LOG_PLAYER(VeryVerbose, "Primary attack triggered");
        }
    }
}

void ACC_PlayerController::HandleSecondaryAttack(const FInputActionValue& Value)
{
    bool bIsPressed = Value.Get<bool>();
    bIsSecondaryAttacking = bIsPressed;

    if (ControlledCharacter && bIsPressed)
    {
        CC_LOG_PLAYER(VeryVerbose, "Secondary attack triggered");
    }
}

void ACC_PlayerController::HandleDash(const FInputActionValue& Value)
{
    if (ControlledCharacter)
    {
        CC_LOG_PLAYER(Log, "Dash triggered (not implemented yet)");
    }
}

FVector ACC_PlayerController::GetMouseWorldPosition() const
{
    FVector WorldLocation, WorldDirection;

    if (DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
    {
        FVector CharacterLocation = GetPawn() ? GetPawn()->GetActorLocation() : FVector::ZeroVector;
        FPlane GroundPlane(CharacterLocation, FVector::UpVector);

        FVector MouseWorldPos = FMath::LinePlaneIntersection(
            WorldLocation,
            WorldLocation + WorldDirection * 10000.0f,
            GroundPlane
        );

        return MouseWorldPos;
    }

    return GetPawn() ? GetPawn()->GetActorLocation() : FVector::ZeroVector;
}

void ACC_PlayerController::UpdateCharacterRotation(float DeltaTime)
{
	if (!ControlledCharacter) return;

	float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - LastMouseUpdateTime < MouseUpdateInterval) return; // Limit to 50 FPS
    LastMouseUpdateTime = CurrentTime;
	
    FVector MouseWorldPos = GetMouseWorldPosition();
	FVector CharacterLocation = ControlledCharacter->GetActorLocation();

    FVector DirectionToMouse = MouseWorldPos - CharacterLocation;
    DirectionToMouse.Z = 0.0f; // Ignore vertical difference

    float DistanceToMouse = DirectionToMouse.Size();
	if (DistanceToMouse < MinMouseDistance) return; // Ignore if too close

    FRotator TargetRotation = DirectionToMouse.Rotation();

    FRotator CurrentRotation = ControlledCharacter->GetActorRotation();
    FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, MouseRotationSpeed);

    ControlledCharacter->SetActorRotation(NewRotation);

    LastMouseWorldPosition = MouseWorldPos;
}

FVector ACC_PlayerController::GetMouseDirection() const
{
    if (ControlledCharacter)
    {
        FVector MousePos = GetMouseWorldPosition();
        FVector CharacterPos = ControlledCharacter->GetActorLocation();
        FVector Direction = MousePos - CharacterPos;
        Direction.Z = 0.0f;
        return Direction.GetSafeNormal();
    }

    return FVector::ForwardVector;
}

void ACC_PlayerController::SetMouseRotationSpeed(float NewSpeed)
{
      MouseRotationSpeed = FMath::Clamp(NewSpeed, 1.0f, 30.0f);
	  CC_LOG_PLAYER(Log, "Mouse rotation speed set to: %.2f", MouseRotationSpeed);
}

void ACC_PlayerController::SetMinMouseDistance(float NewDistance)
{
    MinMouseDistance = FMath::Clamp(NewDistance, 10.0f, 200.0f);
	CC_LOG_PLAYER(Log, "Min mouse distance set to: %.2f", MinMouseDistance);
}

void ACC_PlayerController::StartBeamCharge()
{
    FHitResult HitResult;
    GetHitResultUnderCursor(ECC_Visibility, false, HitResult);

    if (HitResult.bBlockingHit)
    {
        BeamStartLocation = HitResult.ImpactPoint;
        bIsChargingBeam = true;

        UE_LOG(LogTemp, Log, TEXT("Beam charge started at: %s"), *BeamStartLocation.ToString());

        // 시각적 피드백 (선택 사항: 데칼, 파티클 등)
        // SpawnDecalAtLocation(...);
    }
}

void ACC_PlayerController::ReleaseBeam()
{
    if (!bIsChargingBeam)
    {
        return;
    }

    FHitResult HitResult;
    GetHitResultUnderCursor(ECC_Visibility, false, HitResult);

    if (HitResult.bBlockingHit)
    {
        FVector BeamEndLocation = HitResult.ImpactPoint;

        // SkillSystem 가져오기
        APawn* ControlledPawn = GetPawn();
        if (ControlledPawn)
        {
            UCC_SkillSystem* SkillSystem = ControlledPawn->FindComponentByClass<UCC_SkillSystem>();
            if (SkillSystem)
            {
                // Beam 스킬 정의 (DataTable에서 가져오거나 임시 생성)
                FSkillDefinition BeamSkill;
                BeamSkill.SkillID = FName("VectorLaser");
                BeamSkill.CoreType = ESkillCoreType::Beam;
                BeamSkill.BaseDamage = 50.0f;
                BeamSkill.Range = 2000.0f;

                // Context 설정
                FSkillExecutionContext Context;
                Context.Caster = ControlledPawn;
                Context.StartLocation = BeamStartLocation;
                Context.TargetLocation = BeamEndLocation;
                Context.Direction = (BeamEndLocation - BeamStartLocation).GetSafeNormal();
                Context.CurrentDamage = BeamSkill.BaseDamage;

                // 실행!
                SkillSystem->ExecuteSkill(BeamSkill, BeamEndLocation);

                UE_LOG(LogTemp, Log, TEXT("Beam fired from %s to %s"),
                    *BeamStartLocation.ToString(), *BeamEndLocation.ToString());
            }
        }
    }

    bIsChargingBeam = false;
}
