// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_GameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "SkillSystem/CC_SkillBase.h"
#include "SkillSystem/CC_SkillLibrarySubsystem.h"
#include "Characters/CC_PlayerCharacter.h"
#include "CC_PlayerState.h"
#include "CC_CubeWorldManager.h"
#include "CC_PlayerController.h"

ACC_GameModeBase::ACC_GameModeBase()
{
	DefaultPawnClass = ACC_PlayerCharacter::StaticClass();
	PlayerControllerClass = ACC_PlayerController::StaticClass();
}

void ACC_GameModeBase::BeginPlay()
{
	Super::BeginPlay();
	// Get reference to player controller

	ACC_CubeWorldManager* CubeManager = ACC_CubeWorldManager::Get(this);

	if (!CubeManager)
	{
		// CubeWorldManager가 없는 레벨(예: L_TestRoom) — 대기할 대상이 없으니 즉시 진행
		UE_LOG(LogTemp, Log, TEXT("[GameMode] No CubeWorldManager in this level — proceeding immediately."));
		SetGameState(EGameState::Playing);
		return;
	}
	
	if (CubeManager->IsCubeSystemReady())
	{
		// BeginPlay 순서 경합으로 CubeWorldManager가 이미 초기화를 끝내버린 경우 —
		// 신호를 기다려봐야 이미 지나간 신호라 못 받으므로, 상태를 직접 확인해서 즉시 전환.
		UE_LOG(LogTemp, Log, TEXT("[GameMode] CubeWorldManager already ready — proceeding immediately."));
		StartMinimumDelayThenPlaying();
		return;
	}

	CubeManager->OnCubeSystemReady.AddDynamic(this, &ACC_GameModeBase::OnCubeSystemReadyHandler);

	GetWorldTimerManager().SetTimer(
		WaitingToStartTimeoutHandle,
		this,
		&ACC_GameModeBase::OnWaitingToStartTimeout,
		WaitingToStartTimeout,
		false);

	UE_LOG(LogTemp, Log, TEXT("[GameMode] Waiting for CubeWorldManager ready signal (timeout: %.1fs)..."),
		WaitingToStartTimeout);
}

// ============================================================
//  게임 상태
// ============================================================

void ACC_GameModeBase::SetGameState(EGameState NewState)
{
	if (CurrentGameState == NewState) return;

	CurrentGameState = NewState;
	OnGameStateChanged.Broadcast(NewState);

	UE_LOG(LogTemp, Log, TEXT("[GameMode] State -> %s"),
		*UEnum::GetValueAsString(NewState));
}

void ACC_GameModeBase::TriggerGameOver()
{
	if (CurrentGameState == EGameState::GameOver) return;
	
	SetGameState(EGameState::GameOver);
	OnGameOver.Broadcast();

	UE_LOG(LogTemp, Warning, TEXT("[GameMode] GAME OVER"));
}

// ============================================================
// 레벨 전환
// ============================================================

void ACC_GameModeBase::GoToGameMode()
{
	UGameplayStatics::OpenLevel(this, GameLevelName);
}

void ACC_GameModeBase::GoToTestRoom()
{
	UGameplayStatics::OpenLevel(this, TestRoomLevelName);
}

void ACC_GameModeBase::GoToMainMenu()
{
	UGameplayStatics::OpenLevel(this, MainMenuLevelName);
}

void ACC_GameModeBase::RestartCurrentLevel()
{
	UGameplayStatics::OpenLevel(this, FName(*GetWorld()->GetName()));
}

TArray<FLevelUpCandidate> ACC_GameModeBase::GetRandomLevelUpCandidates(int32 Count)
{
    TArray<FLevelUpCandidate> Result;

    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    ACC_PlayerState* PS = PC ? PC->GetPlayerState<ACC_PlayerState>() : nullptr;
    UCC_SkillLibrarySubsystem* SkillLibrary = GetGameInstance()
        ? GetGameInstance()->GetSubsystem<UCC_SkillLibrarySubsystem>() : nullptr;

    if (!PS || !SkillLibrary) return Result;

    TArray<FName> OwnedSkillIDs;
    for (UCC_SkillBase* Skill : PS->GetAllSkills())
    {
        if (Skill) OwnedSkillIDs.Add(Skill->GetSkillID());
    }

    TArray<FName> UsedSkillGrantNames;
    TArray<ESkillAddonType> UsedAddonTypes;
    TArray<FName> UsedCoreSkillIDs;

    for (int32 i = 0; i < Count; ++i)
    {
        TArray<ELevelUpCandidateType> ValidCategories;

        const bool bSkillGrantValid = PS->CanGrantMoreSkills()
            && (SkillLibrary->GetAllSkillRowNames().Num() - OwnedSkillIDs.Num() - UsedSkillGrantNames.Num() > 0);
        if (bSkillGrantValid) ValidCategories.Add(ELevelUpCandidateType::SkillGrant);

        const bool bAddonPointValid = UsedAddonTypes.Num() < 4;
        if (bAddonPointValid) ValidCategories.Add(ELevelUpCandidateType::AddonPoint);

        const bool bCorePointValid = (OwnedSkillIDs.Num() - UsedCoreSkillIDs.Num()) > 0;
        if (bCorePointValid) ValidCategories.Add(ELevelUpCandidateType::CorePoint);

        if (ValidCategories.Num() == 0) break;

        // 안전장치: 보유 스킬이 하나도 없는데 이번 뽑기에서 아직 SkillGrant가
        // 한 장도 안 나왔다면 이번 슬롯은 강제로 SkillGrant.
        // (시작 스킬이 정상 배선됐다면 사실상 발동 안 함 — 데이터테이블 세팅 실수 등
        // 예외 상황에서 "공격 스킬 없이 포인트 카드만 뜨는" 사태 방지용 방어선.)
        const bool bForceSkillGrant =
            OwnedSkillIDs.Num() == 0 && UsedSkillGrantNames.Num() == 0 && bSkillGrantValid;

        const ELevelUpCandidateType Category = bForceSkillGrant
            ? ELevelUpCandidateType::SkillGrant
            : ValidCategories[FMath::RandRange(0, ValidCategories.Num() - 1)];

        FLevelUpCandidate Candidate;
        Candidate.CandidateType = Category;

        switch (Category)
        {
        case ELevelUpCandidateType::SkillGrant:
        {
            TArray<FName> ExcludeList = OwnedSkillIDs;
            ExcludeList.Append(UsedSkillGrantNames);
            TArray<FName> Picked = SkillLibrary->GetRandomUnownedSkillNames(ExcludeList, 1);
            if (Picked.Num() == 0) continue;

            Candidate.SkillRowName = Picked[0];
            UsedSkillGrantNames.Add(Picked[0]);

            FSkillDisplayData DisplayData;
            if (SkillLibrary->GetSkillDisplayData(Picked[0], DisplayData))
            {
                Candidate.DisplayName = DisplayData.DisplayName;
                Candidate.Icon = DisplayData.Icon;
            }
            break;
        }
        case ELevelUpCandidateType::AddonPoint:
        {
            TArray<ESkillAddonType> AllAddonTypes = {
                ESkillAddonType::Explosion, ESkillAddonType::Chain,
                ESkillAddonType::Penetrate, ESkillAddonType::MultiShot
            };
            AllAddonTypes.RemoveAll([&](ESkillAddonType T) { return UsedAddonTypes.Contains(T); });
            if (AllAddonTypes.Num() == 0) continue;

            const ESkillAddonType Picked = AllAddonTypes[FMath::RandRange(0, AllAddonTypes.Num() - 1)];
            Candidate.TargetAddonType = Picked;
            Candidate.PointAmount = 1;
            UsedAddonTypes.Add(Picked);

            FAddonTableRow AddonRow;
            if (SkillLibrary->GetAddonDisplayDataByType(Picked, AddonRow))
            {
                Candidate.DisplayName = AddonRow.DisplayName;
                Candidate.Description = AddonRow.Description;
                Candidate.Icon = AddonRow.Icon;
            }
            break;
        }
        case ELevelUpCandidateType::CorePoint:
        {
            TArray<FName> AvailableOwned = OwnedSkillIDs;
            AvailableOwned.RemoveAll([&](FName ID) { return UsedCoreSkillIDs.Contains(ID); });
            if (AvailableOwned.Num() == 0) continue;

            const FName Picked = AvailableOwned[FMath::RandRange(0, AvailableOwned.Num() - 1)];
            Candidate.TargetSkillID = Picked;
            Candidate.PointAmount = 1;
            UsedCoreSkillIDs.Add(Picked);

            FSkillDisplayData SkillData;
            if (SkillLibrary->GetSkillDisplayData(Picked, SkillData))
            {
                Candidate.DisplayName = FText::Format(
                    NSLOCTEXT("LevelUp", "CorePointName", "코어 강화 - {0}"), SkillData.DisplayName);
                Candidate.Description = NSLOCTEXT("LevelUp", "CorePointDesc",
                    "선택한 스킬의 고유 수치를 강화할 포인트를 얻습니다.");
                Candidate.Icon = SkillData.Icon;
            }
            break;
        }
        default:
            break;
        }

        Result.Add(Candidate);
    }

    return Result;
}

void ACC_GameModeBase::ApplyLevelUpCandidate(const FLevelUpCandidate& SelectedCandidate)
{
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    ACC_PlayerState* PS = PC ? PC->GetPlayerState<ACC_PlayerState>() : nullptr;
    if (!PS) return;

    switch (SelectedCandidate.CandidateType)
    {
    case ELevelUpCandidateType::SkillGrant:
        PS->GrantSkillByRowName(SelectedCandidate.SkillRowName);
        break;

    case ELevelUpCandidateType::AddonPoint:
        PS->AddAddonPoints(SelectedCandidate.TargetAddonType, SelectedCandidate.PointAmount);
        break;

    case ELevelUpCandidateType::CorePoint:
        PS->AddSkillCorePoints(SelectedCandidate.TargetSkillID, SelectedCandidate.PointAmount);
        break;

    default:
        break;
    }

    UE_LOG(LogTemp, Log, TEXT("[GameModeBase] LevelUp candidate applied: %s"),
        *SelectedCandidate.DisplayName.ToString());
}
// =============================================================
// 편의 접근자
// =============================================================


ACC_PlayerCharacter* ACC_GameModeBase::GetPlayerCharacter() const
{
	return Cast<ACC_PlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
}

ACC_PlayerController* ACC_GameModeBase::GetPlayerControllerRef() const
{
	return Cast<ACC_PlayerController>(UGameplayStatics::GetPlayerController(this, 0));
}

void ACC_GameModeBase::OnCubeSystemReadyHandler()
{
	if (CurrentGameState != EGameState::WaitingToStart) return; // 이미 전환됨(타임아웃 등) — 중복 방지

	GetWorldTimerManager().ClearTimer(WaitingToStartTimeoutHandle);
	UE_LOG(LogTemp, Log, TEXT("[GameMode] CubeWorldManager ready signal received."));
	StartMinimumDelayThenPlaying();
}

void ACC_GameModeBase::OnWaitingToStartTimeout()
{
	if (CurrentGameState != EGameState::WaitingToStart) return;

	UE_LOG(LogTemp, Warning,
		TEXT("[GameMode] CubeWorldManager ready signal NOT received within %.1fs — falling back to Playing."),
		WaitingToStartTimeout);
	SetGameState(EGameState::Playing);
}

void ACC_GameModeBase::StartMinimumDelayThenPlaying()
{
	if (MinimumStartDelay <= 0.f)
	{
		SetGameState(EGameState::Playing);
		return;
	}

	GetWorldTimerManager().SetTimer(
		MinimumStartDelayHandle,
		this,
		&ACC_GameModeBase::OnMinimumStartDelayElapsed,
		MinimumStartDelay,
		false);

	UE_LOG(LogTemp, Log,
		TEXT("[GameMode] Cube system ready — holding %.1fs more before Playing (load buffer)."),
		MinimumStartDelay);
}

void ACC_GameModeBase::OnMinimumStartDelayElapsed()
{
	if (CurrentGameState != EGameState::WaitingToStart) return;

	SetGameState(EGameState::Playing);
}
