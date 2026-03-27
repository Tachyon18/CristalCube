// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_GameHUD.h"
#include "CC_GlassWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UCC_GameHUD::NativeConstruct()
{
	Super::NativeConstruct();
}

void UCC_GameHUD::UpdateHealth(float CurrentHealth, float MaxHealth)
{
	HealthBar->SetPercent(CurrentHealth / MaxHealth);
}

void UCC_GameHUD::UpdateExp(float CurrentExp, float MaxExp, int32 Level)
{
	ExpBar->SetPercent(CurrentExp / MaxExp);
	FText InLevelText = FText::Format(FText::FromString(TEXT("Level : {0}")), Level);

	LevelText->SetText(InLevelText);
}

void UCC_GameHUD::UpdateTimer(float TotalSeconds)
{
	int32 Minutes = FMath::Floor(TotalSeconds / 60);
	int32 Seconds = FMath::Floor(int32(TotalSeconds) % 60);

	FText InTimerText = FText::Format(FText::FromString(TEXT("{0} : {1}")), Minutes, Seconds);

	TimerText->SetText(InTimerText);
}

void UCC_GameHUD::PlayHitFlash()
{
    // 붉은 Flash 테마 즉시 적용
    FGlassThemeData FlashTheme;
    FlashTheme.TintColor = FLinearColor(0.80f, 0.05f, 0.05f, 1.f);
    FlashTheme.PanelOpacity = 0.22f;
    FlashTheme.PrismIntensity = 0.08f;
    FlashTheme.GlowColor = FLinearColor(1.00f, 0.10f, 0.10f, 1.f);
    FlashTheme.GlowOpacity = 0.45f;
    FlashTheme.BlurStrength = 15.f;
    StatsPanel->SetCustomTheme(FlashTheme);

    // 0.18초 후 원래 테마로 복귀
    GetWorld()->GetTimerManager().ClearTimer(HitFlashTimer);
    GetWorld()->GetTimerManager().SetTimer(
        HitFlashTimer,
        this,
        &UCC_GameHUD::ResetHitFlash,
        0.18f,
        false
    );
}

void UCC_GameHUD::ResetHitFlash()
{
    if(StatsPanel)
	    StatsPanel->SetTheme(StatsPanel->DefaultTheme);
}

