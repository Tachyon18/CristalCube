// Fill out your copyright notice in the Description page of Project Settings.

#include "CC_RunResultWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "../CC_GameModeBase.h"

void UCC_RunResultWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (RetryButton)
	{
		RetryButton->OnClicked.AddUniqueDynamic(this, &UCC_RunResultWidget::HandleRetryClicked);
	}
	if (MainMenuButton)
	{
		MainMenuButton->OnClicked.AddUniqueDynamic(this, &UCC_RunResultWidget::HandleMainMenuClicked);
	}
}

void UCC_RunResultWidget::SetRunResult(const FCCRunResult& InResult)
{
	RunResult = InResult;

	if (TitleText)        TitleText->SetText(RunResult.bCleared ? GameClearTitle : GameOverTitle);
	if (TimeText)         TimeText->SetText(FormatTime(RunResult.SurvivalTime));
	if (KillText)         KillText->SetText(FText::AsNumber(RunResult.TotalKills));
	if (CubeClearText)    CubeClearText->SetText(FText::AsNumber(RunResult.CubeClearCount));
	if (LevelText)        LevelText->SetText(FText::AsNumber(RunResult.ReachedLevel));
	if (BossProgressText) BossProgressText->SetText(FText::AsPercent(RunResult.BossProgressRatio));

	OnRunResultSet(RunResult);
}

void UCC_RunResultWidget::HandleRetryClicked()
{
	LockButtons();
	if (ACC_GameModeBase* GM = Cast<ACC_GameModeBase>(UGameplayStatics::GetGameMode(this)))
	{
		GM->RestartCurrentLevel();
	}
}

void UCC_RunResultWidget::HandleMainMenuClicked()
{
	LockButtons();
	if (ACC_GameModeBase* GM = Cast<ACC_GameModeBase>(UGameplayStatics::GetGameMode(this)))
	{
		GM->GoToMainMenu();
	}
}

void UCC_RunResultWidget::LockButtons()
{
	if (RetryButton)    RetryButton->SetIsEnabled(false);
	if (MainMenuButton) MainMenuButton->SetIsEnabled(false);
}

FText UCC_RunResultWidget::FormatTime(float Seconds)
{
	const int32 Total = FMath::FloorToInt(FMath::Max(Seconds, 0.0f));
	return FText::FromString(FString::Printf(TEXT("%02d:%02d"), Total / 60, Total % 60));
}