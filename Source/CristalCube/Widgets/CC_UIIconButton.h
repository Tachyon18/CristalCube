// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CC_UIControl.h"
#include "CC_UIIconButton.generated.h"

// CC_UIIconButton.h — L2 Primitive: IconButton (−/+ · 닫기)
// UCC_UIControl 파생 — 상태·눌림·클릭은 부모. 여기는 크기(32/28) · 클릭 영역 44 · 글리프 · 강조(Accent).

class USizeBox;
class UTextBlock;

UENUM(BlueprintType)
enum class EUIIconGlyph : uint8
{
	Plus,
	Minus,
	Close,
	Custom        // CustomGlyph 텍스트 사용
};

UENUM(BlueprintType)
enum class EUIIconButtonSize : uint8
{
	Regular,      // 32 (Core 행)
	Compact       // 28 (Addon 펼침 행)
};

/**
 * WBP_UI_IconButton 구조 (이름 일치 필수):
 *   [Root] HitSizeBox (Size Box)                ← 클릭 영역 = max(보이는 크기, HitMin 44) · 눌림 스케일 대상
 *     └ Overlay
 *        ├ VisualSizeBox (Size Box, Center/Center)   ← 보이는 크기 32 / 28
 *        │   └ Overlay
 *        │      ├ ControlSurface (WBP_UI_Surface, Fill/Fill)   SurfaceMaterial 비움
 *        │      └ GlyphText (TextBlock, Center/Center, Not Hit-Testable)
 *        └ HitButton (Button, Fill/Fill)              ← 44 전체가 클릭 영역
 *
 * Surface = Card · RadiusToken IconButton(10) — InvLayoutB `.ib card` (− 버튼) / `.ib card rim` (+ 버튼 = bAccent)
 */
UCLASS()
class CRISTALCUBE_API UCC_UIIconButton : public UCC_UIControl
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "IconButton")
	void SetGlyph(EUIIconGlyph InGlyph);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IconButton")
	EUIIconGlyph Glyph = EUIIconGlyph::Plus;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IconButton", meta = (EditCondition = "Glyph == EUIIconGlyph::Custom"))
	FText CustomGlyph;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IconButton")
	EUIIconButtonSize ButtonSize = EUIIconButtonSize::Regular;

	/** 강조 (+ 버튼): Rest에도 핫스팟 테두리 = Hover 모양, Hover = Selected 모양 (InvLayoutB `.rim`) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IconButton")
	bool bAccent = false;

	/** 클릭 영역을 Token HitMin(44)까지 넓힘 — 보이는 크기는 그대로 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IconButton")
	bool bExpandHitArea = true;

	/** 글리프 크기 (시안 px) — Regular 17 / Compact 15 (InvLayoutB `.ib` Oxanium 17px) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IconButton", meta = (ClampMin = "6"))
	float GlyphSizePx = 17.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IconButton", meta = (ClampMin = "6"))
	float CompactGlyphSizePx = 15.f;

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeOnVisualStateChanged(EUISurfaceState NewState, bool bInstant) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> GlyphText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> HitSizeBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> VisualSizeBox;

private:
	void ApplyLayout();
	void ApplyGlyph();
	void ApplyGlyphColor(EUISurfaceState State);
};