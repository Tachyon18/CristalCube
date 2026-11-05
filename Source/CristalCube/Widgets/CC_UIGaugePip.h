// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CC_UIGaugePip.generated.h"

// CC_UIGaugePip.h — L2 Primitive: Gauge Pip (강화 단계 칸)
// 머티리얼 0 — Image(Rounded Box) N개를 직접 생성. 칸 수 = Max, 채움/다음/빈 칸 색 = Token.
// (Track Surface는 6px에서 빈 고리로 보여 제외 — Math Lab 13)

class UHorizontalBox;
class UImage;
class UTexture2D;

/**
 * WBP_UI_GaugePip 구조:
 *   [Root] PipBox (Horizontal Box)     BindWidget — 칸은 C++가 채움 (WBP에 자식 두지 않음)
 *
 * 칸 상태 (InvLayoutB):  0 ~ Spent-1 = 채움  ·  Spent = 다음 칸(흰 18%)  ·  나머지 = 빈 칸(흰 7%)
 */
UCLASS()
class CRISTALCUBE_API UCC_UIGaugePip : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Spent = 채운 칸 수, Max = 전체 칸 수. Max가 바뀔 때만 칸을 다시 만듦 */
	UFUNCTION(BlueprintCallable, Category = "GaugePip")
	void SetValue(int32 InSpent, int32 InMax);

	UFUNCTION(BlueprintPure, Category = "GaugePip")
	int32 GetSpent() const { return Spent; }

	UFUNCTION(BlueprintPure, Category = "GaugePip")
	int32 GetMax() const { return Max; }

	// ── 에디터 설정 ──────────────────────────────────────────
	/** 디자이너 미리보기 값 (런타임에도 SetValue 전까지 이 값) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GaugePip", meta = (ClampMin = "0"))
	int32 PreviewSpent = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GaugePip", meta = (ClampMin = "0", ClampMax = "30"))
	int32 PreviewMax = 10;

	/** '다음 칸' 강조 (Spent < Max일 때 Spent번째 칸을 흰 18%로) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GaugePip")
	bool bShowNext = true;

	/**
	 * 채움 방식
	 *  - 비움(기본): 칸마다 단색 — Token PipFill(Peach → Rose → Lilac)을 **칸 위치**로 샘플 (줄 전체가 하나의 그라데이션)
	 *  - 텍스처 지정: 칸마다 그 텍스처를 둥근 칸 모양으로 (InvLayoutB처럼 **칸 하나 안에** Peach → Rose → Lilac — 가로 그라데이션 텍스처 필요)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GaugePip")
	TObjectPtr<UTexture2D> FillTexture;

	/** −1 = Token (Sizes.Pip 13 × 6 · PipGap 4) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GaugePip")
	FVector2D PipSizeOverride = FVector2D(-1.f, -1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GaugePip")
	float GapOverride = -1.f;

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> PipBox;

private:
	void Rebuild();
	void RefreshColors();
	FVector2D GetPipSize() const;
	float GetGap() const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> Pips;

	int32 Spent = 0;
	int32 Max = 0;
	int32 BuiltCount = -1;
	bool bValueSet = false;
};