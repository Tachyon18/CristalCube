// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../CristalCubeStruct.h"   // ESkillElementType
#include "CC_UISurface.h"           // EUISurfaceState · EUIRadius · UCC_UISurface
#include "CC_UIIconWell.generated.h"

// CC_UIIconWell.h — L2 Primitive: IconWell (원소색 아이콘 우물)
// Well Surface(Token Role · RadiusToken · Element) + 흰 아이콘. 클릭 없음 (표시 전용 — 클릭은 감싸는 Card/Entry가 받음).
// 쓰임: 로스터 엔트리(M 48) · 상세 헤더(L 64) · Addon 요약(S 40)

class UImage;
class USizeBox;
class UTexture2D;

/** Well 크기 — Token Sizes.WellS/M/L (40/48/64) · Radii.WellS/M/L (12/14/18) */
UENUM(BlueprintType)
enum class EUIWellSize : uint8
{
	S,
	M,
	L
};

/**
 * WBP_UI_IconWell 구조 (이름 일치 필수):
 *   [Root] WellSizeBox (SizeBox)                 BindWidgetOptional — 크기 = Token (C++가 Override 설정)
 *     └ Overlay
 *        ├ WellSurface (WBP_UI_Surface, Fill/Fill)   BindWidget — SurfaceMaterial 비움 (Role은 C++가 Well로)
 *        └ IconImage   (Image, Center/Center)        BindWidget — 크기 = Well × IconRatio
 *
 * 상태: Rest · Selected(원소 몸체 +0.10 · 핫스팟 · Sheen) · Empty(점선, 아이콘 숨김) · Disabled(Opacity 0.45)
 */
UCLASS()
class CRISTALCUBE_API UCC_UIIconWell : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 아이콘 + 원소 (원소 None = MI 몸체색) */
	UFUNCTION(BlueprintCallable, Category = "IconWell")
	void SetIcon(UTexture2D* InIcon, ESkillElementType InElement);

	/** SkillLibrary(DT_Skill)에서 Icon · ElementType을 읽어 적용. 실패(디자이너·ID 없음) 시 false */
	UFUNCTION(BlueprintCallable, Category = "IconWell")
	bool SetSkill(FName SkillID);

	UFUNCTION(BlueprintCallable, Category = "IconWell")
	void SetWellSize(EUIWellSize InSize);

	/** Rest / Selected / Empty / Disabled (Hover·Pressed는 감싸는 Control 쪽에서) */
	UFUNCTION(BlueprintCallable, Category = "IconWell")
	void SetWellState(EUISurfaceState NewState, bool bInstant = false);

	UFUNCTION(BlueprintPure, Category = "IconWell")
	EUISurfaceState GetWellState() const { return WellState; }

	UFUNCTION(BlueprintPure, Category = "IconWell")
	ESkillElementType GetElement() const { return Element; }

	// ── 에디터 설정 (디자이너 미리보기에 바로 반영) ─────────
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IconWell")
	EUIWellSize WellSize = EUIWellSize::M;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IconWell")
	ESkillElementType Element = ESkillElementType::None;

	/** 기본/미리보기 아이콘 (런타임에는 보통 SetIcon · SetSkill로) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IconWell")
	TObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IconWell")
	EUISurfaceState InitialWellState = EUISurfaceState::Rest;

	/** 아이콘 한 변 = Well 한 변 × IconRatio (Kit4 시안 28/66 ≈ 0.42 · 래스터 아이콘 여백 감안 0.5 — B6에서 확정) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IconWell", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float IconRatio = 0.5f;

	/** 아이콘 자체는 흰색 (아트 규칙 3) — 색 있는 원본이면 그대로 곱해짐 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IconWell")
	FLinearColor IconTint = FLinearColor::White;

	/** Head 공간(바깥 Pad) 확보 — 목록(ScrollBox) 안에서는 ✘ (클립되어 사각 귀퉁이로 보임) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IconWell")
	bool bReserveHeadPad = false;

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCC_UISurface> WellSurface;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> IconImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> WellSizeBox;

private:
	void ApplyAll();                      // 크기 · 반경 · 원소 · 아이콘 · 상태 전부 (Surface 재초기화 포함)
	void ApplyIcon();
	void ApplyIconState();
	float GetWellPx() const;
	EUIRadius GetRadiusToken() const;

	EUISurfaceState WellState = EUISurfaceState::Rest;
	bool bStateInitialized = false;
};
