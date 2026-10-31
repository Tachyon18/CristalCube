// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../CristalCubeStruct.h"
#include "CC_RunResultWidget.generated.h"

class UButton;
class UTextBlock;

/**
 * Game Over / Game Clear 공용 결과 화면.
 *
 * [Blueprint 설정]
 *  1. 이 클래스를 부모로 WBP_RunResult 생성 (기존 WBP_GameOver를 Reparent해도 됨)
 *  2. 필수: RetryButton, MainMenuButton (UButton)
 *  3. 선택: TitleText, TimeText, KillText, CubeClearText, LevelText, BossProgressText (UTextBlock)
 *     — "처치", "생존 시간" 같은 라벨은 WBP의 고정 텍스트로 둠
 *  4. 배경은 WBP 안에 글래스 패널(WBP_GlassPanel 등)을 깔아서 사용
 *  5. 등장 연출은 OnRunResultSet 이벤트에서 위젯 애니메이션 재생
 */
UCLASS()
class CRISTALCUBE_API UCC_RunResultWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Run Result")
	void SetRunResult(const FCCRunResult& InResult);

	UFUNCTION(BlueprintPure, Category = "Run Result")
	const FCCRunResult& GetRunResult() const { return RunResult; }

protected:
	virtual void NativeConstruct() override;

	/** SetRunResult 끝에서 호출 — BP에서 등장 애니메이션 등 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Run Result")
	void OnRunResultSet(const FCCRunResult& InResult);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run Result|Text")
	FText GameOverTitle = NSLOCTEXT("RunResult", "GameOverTitle", "GAME OVER");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run Result|Text")
	FText GameClearTitle = NSLOCTEXT("RunResult", "GameClearTitle", "GAME CLEAR");

	// ── 버튼 (필수) ─────────────────────────────────────────
	UPROPERTY(meta = (BindWidget))
	UButton* RetryButton;

	UPROPERTY(meta = (BindWidget))
	UButton* MainMenuButton;

	// ── 값 표시 (선택) ──────────────────────────────────────
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TimeText;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* KillText;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* CubeClearText;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* LevelText;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* BossProgressText;

private:
	UPROPERTY()
	FCCRunResult RunResult;

	UFUNCTION()
	void HandleRetryClicked();

	UFUNCTION()
	void HandleMainMenuClicked();

	/** 두 버튼 중복 클릭 방지 (레벨 로딩 중 연타) */
	void LockButtons();

	static FText FormatTime(float Seconds);
};