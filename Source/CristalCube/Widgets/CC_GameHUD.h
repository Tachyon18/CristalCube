// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CC_GameHUD.generated.h"

/**
 * 
 */
UCLASS()
class CRISTALCUBE_API UCC_GameHUD : public UUserWidget
{
	GENERATED_BODY()
	
public:

	virtual void NativeConstruct() override;

protected:

	// ── Glass 패널 (각각 독립 인스턴스) ────────────────────────
	UPROPERTY(meta = (BindWidget))
	class UCC_GlassWidget* StatsPanel;

	UPROPERTY(meta = (BindWidget))
	UCC_GlassWidget* TimerPanel;

	UPROPERTY(EditAnywhere, meta = (BindWidget))
	class UProgressBar* HealthBar;

	UPROPERTY(EditAnywhere, meta = (BindWidget))
	UProgressBar* ExpBar;

	UPROPERTY(EditAnywhere, meta = (BindWidget))
	class UTextBlock* LevelText;

	UPROPERTY(EditAnywhere, meta = (BindWidget))
	UTextBlock* TimerText;

public:

	UFUNCTION(BlueprintCallable)
	void UpdateHealth(float CurrentHealth, float MaxHealth);

	UFUNCTION(BlueprintCallable)
	void UpdateExp(float CurrentExp, float MaxExp, int32 Level);

	UFUNCTION(BlueprintCallable)
	void UpdateTimer(float TotalSeconds);

	UFUNCTION(BlueprintCallable)
	void PlayHitFlash();

private:

	FTimerHandle HitFlashTimer;
	void ResetHitFlash();
};
