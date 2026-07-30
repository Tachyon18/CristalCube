// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "Widgets/CC_GameHUD.h"
#include "Kismet/GameplayStatics.h"
#include "SkillSystem/CC_SkillBase.h"
#include "SkillSystem/CC_SkillSystem.h"
#include "Characters/CC_PlayerCharacter.h"
#include "CC_PlayerState.h"



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

    FInputModeGameAndUI InputMode;
    InputMode.SetHideCursorDuringCapture(false);
    SetInputMode(InputMode);
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

        if (SkillAction1) EnhancedInputComponent->BindAction(SkillAction1, ETriggerEvent::Started, this, &ACC_PlayerController::HandleCastSkill1);
        if (SkillAction2) EnhancedInputComponent->BindAction(SkillAction2, ETriggerEvent::Started, this, &ACC_PlayerController::HandleCastSkill2);
        if (SkillAction3) EnhancedInputComponent->BindAction(SkillAction3, ETriggerEvent::Started, this, &ACC_PlayerController::HandleCastSkill3);
        if (SkillAction4) EnhancedInputComponent->BindAction(SkillAction4, ETriggerEvent::Started, this, &ACC_PlayerController::HandleCastSkill4);
        if (SkillAction5) EnhancedInputComponent->BindAction(SkillAction5, ETriggerEvent::Started, this, &ACC_PlayerController::HandleCastSkill5);
        if (SkillAction6) EnhancedInputComponent->BindAction(SkillAction6, ETriggerEvent::Started, this, &ACC_PlayerController::HandleCastSkill6);

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

void ACC_PlayerController::HandleCastSkill1(const FInputActionValue& Value) { CastSkillAtSlot(0); }
void ACC_PlayerController::HandleCastSkill2(const FInputActionValue& Value) { CastSkillAtSlot(1); }
void ACC_PlayerController::HandleCastSkill3(const FInputActionValue& Value) { CastSkillAtSlot(2); }
void ACC_PlayerController::HandleCastSkill4(const FInputActionValue& Value) { CastSkillAtSlot(3); }
void ACC_PlayerController::HandleCastSkill5(const FInputActionValue& Value) { CastSkillAtSlot(4); }
void ACC_PlayerController::HandleCastSkill6(const FInputActionValue& Value) { CastSkillAtSlot(5); }

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

void ACC_PlayerController::CastSkillAtSlot(int32 SlotIndex)
{
    if (!ControlledCharacter)
    {
        ControlledCharacter = Cast<ACC_PlayerCharacter>(GetPawn());
    }
    if (!ControlledCharacter) return;

    ACC_PlayerState* PS = GetPlayerState<ACC_PlayerState>();
    UCC_SkillSystem* SS = ControlledCharacter->GetSkillSystem();
    if (!PS || !SS) return;

    UCC_SkillBase* Skill = PS->GetSkillAtSlot(SlotIndex);
    if (!Skill) return;   // 빈 슬롯 ? 조용히 무시

    const FVector TargetLocation = GetMouseWorldPosition();
    const bool bCast = Skill->TryCast(SS, TargetLocation);

    if (!bCast)
    {
        // 쿨다운 중이거나 발동 실패 ? 나중에 GameHUD 쿨다운 링/사운드 피드백 여기 연결
        CC_LOG_PLAYER(VeryVerbose, "Skill slot %d not ready", SlotIndex);
    }
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

void ACC_PlayerController::GrantExp(float Amount)
{
    // PIE 시작 직후 등 캐싱 타이밍을 놓쳤을 경우를 대비한 방어적 재조회
    if (!ControlledCharacter)
    {
        ControlledCharacter = Cast<ACC_PlayerCharacter>(GetPawn());
    }

    if (!ControlledCharacter)
    {
        CC_LOG_PLAYER(Warning, "GrantExp failed - no ControlledCharacter to grant experience to");
        return;
    }

    ControlledCharacter->AddExperience(Amount);
    CC_LOG_PLAYER(Log, "GrantExp: +%.1f XP granted via console", Amount);
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

void ACC_PlayerController::CreateGameHUD()
{
    if (CurrentGameHUD || !GameHUDClass) return;

    CurrentGameHUD = CreateWidget<UCC_GameHUD>(this, GameHUDClass);
    if (CurrentGameHUD)
    {
        CurrentGameHUD->AddToViewport();
        CC_LOG_PLAYER(Log, "Game HUD created (owned by PlayerController)");
    }
}
