// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CC_UICounter.generated.h"

// CC_UICounter.h — L2 Primitive: Counter (미사용 포인트 은행)
// Pill Surface + 라벨("미사용") + 값. 값이 바뀌면 펄스(스케일 1 → 1.12 → 1 + 밝게, Token Motion.CounterPulse 0.25s).
// 값 > 0 = AccentNotice(#F6D2A8) · 값 0 = TextTertiary. 쓰임: Core 섹션 헤더(`CoreBankText`) · Addon 카드 요약(Surface 없이)

class UTextBlock;
class UCC_UISurface;

/**
 * WBP_UI_Counter 구조 (이름 일치):
 *   [Root] Overlay
 *     ├ CounterSurface (WBP_UI_Surface, Fill/Fill)        BindWidgetOptional — 없거나 bShowSurface ✘면 글자만 (Addon 요약 줄)
 *     └ Horizontal Box  (Padding 14, 6, 14, 6 · Center)
 *        ├ CaptionText (TextBlock)                        BindWidgetOptional — "미사용"
 *        ├ Spacer 8
 *        └ ValueText   (TextBlock)                        BindWidget — 숫자
 */
UCLASS()
class CRISTALCUBE_API UCC_UICounter : public UUserWidget
{
	GENERATED_BODY()

public:
	/** bPulse = 값이 실제로 바뀌었을 때만 펄스 */
	UFUNCTION(BlueprintCallable, Category = "Counter")
	void SetValue(int32 InValue, bool bPulse = true);

	UFUNCTION(BlueprintPure, Category = "Counter")
	int32 GetValue() const { return Value; }

	UFUNCTION(BlueprintCallable, Category = "Counter")
	void SetCaption(FText InCaption);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Counter")
	FText Caption = NSLOCTEXT("CC_UI", "CounterUnspent", "미사용");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Counter", meta = (ClampMin = "0"))
	int32 PreviewValue = 2;

	/** ✘ = 알약 바탕 없이 글자만 (Addon 카드 요약 줄) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Counter")
	bool bShowSurface = true;

	/** 펄스 최대 배율 (시안 1.12) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Counter", meta = (ClampMin = "1.0", ClampMax = "1.5"))
	float PulseScale = 1.12f;

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ValueText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CaptionText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCC_UISurface> CounterSurface;

private:
	void ApplyStatic();
	void ApplyValueVisual(float PulseAlpha);   // 0 = 평소, 1 = 펄스 정점

	int32 Value = 0;
	bool bValueSet = false;
	float PulseElapsed = -1.f;                 // < 0 = 펄스 없음
};