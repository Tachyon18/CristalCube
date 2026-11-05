// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DeveloperSettings.h"
#include "Fonts/SlateFontInfo.h"
#include "../CristalCubeStruct.h"   // ESkillElementType
#include "CC_UISurface.h"           // EUISurfaceRole · EUIRadius · EUISurfaceState · FUISurfaceStateLook
#include "CC_UIStyleData.generated.h"

class UFont;
class UTextBlock;
class UMaterialInterface;

/** Text Style 종류 (golden-b1-b2-provide §1-7) */
UENUM(BlueprintType)
enum class EUITextStyle : uint8
{
	Display,
	DisplayHero,
	Heading,
	Title,
	TitleSmall,
	Body,
	BodyMuted,
	BodySmall,
	Caption,
	CaptionTight,
	Numeric,
	Counter,
	Effect,
	Button,
	ButtonGhost
};

/** 원소 하나의 UI 색 — sRGB로 저장, 사용 시 Linear 변환 */
USTRUCT(BlueprintType)
struct FUIElementTone
{
	GENERATED_BODY()

	/** 유리 몸체 색 (Well BodyColor · Selected 카드 몸체) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Element")
	FColor Body = FColor::White;

	/** 원소 이름 강조 텍스트 색 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Element")
	FColor Text = FColor::White;
};

/** 글꼴 스타일 하나 — SizePx는 시안(웹) px, 적용 시 × PxToSlatePt */
USTRUCT(BlueprintType)
struct FUITextStyle
{
	GENERATED_BODY()

	/** 폰트 패밀리 에셋 (UFont, 예: /Game/Develop/Fonts/Orbitron/Orbitron) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Text")
	TSoftObjectPtr<UFont> Font;

	/** 패밀리 안의 Typeface 이름 (Regular / Medium / SemiBold / Bold / Black …) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Text")
	FName Typeface = TEXT("Regular");

	/** 시안(웹, 1080p) 기준 글자 크기 px */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Text", meta = (ClampMin = "1"))
	float SizePx = 14.f;

	/** 자간 — 1/1000 em (CSS .12em = 120) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Text")
	int32 LetterSpacing = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Text")
	FColor Color = FColor::White;
};

USTRUCT(BlueprintType)
struct FUIRadiusTokens
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radius") float Plate = 30.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radius") float Section = 22.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radius") float Entry = 18.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radius") float SubCard = 16.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radius") float Button = 14.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radius") float IconButton = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radius") float WellL = 18.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radius") float WellM = 14.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radius") float WellS = 12.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radius") float Full = 999.f;
};

USTRUCT(BlueprintType)
struct FUIMotionTokens
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion") float StateBlend = 0.12f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion") float HotspotMove = 0.25f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion") float HotspotFollow = 0.18f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion") float HotspotReturn = 0.35f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion") float RimIntro = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion") float RimIntroSweep = 0.35f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion") float RimSweep = 0.9f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion") float TrailTime = 0.12f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion") float TrailMaxPx = 160.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion") float HeadFade = 0.25f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion") float PressedScale = 0.97f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion") float AccordionFade = 0.15f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion") float CounterPulse = 0.25f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion") float PanelFade = 0.20f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion") float BreathPeriod = 5.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion") float BreathAmp = 0.10f;
};

USTRUCT(BlueprintType)
struct FUISizeTokens
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Size") float WellS = 40.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Size") float WellM = 48.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Size") float WellL = 64.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Size") float IconButton = 32.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Size") float IconButtonCompact = 28.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Size") float HitMin = 44.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Size") float ButtonHeight = 50.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Size") float EntryHeight = 76.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Size") float AddonHeaderHeight = 66.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Size") float NoticeDot = 7.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Size") FVector2D Pip = FVector2D(13.f, 6.f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Size") float PipGap = 4.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Size") float GaugeBarHeight = 10.f;
};

USTRUCT(BlueprintType)
struct FUISpacingTokens
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spacing") float XXS = 4.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spacing") float XS = 6.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spacing") float S = 8.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spacing") float M = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spacing") float L = 14.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spacing") float XL = 18.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spacing") float XXL = 20.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spacing") float XXXL = 28.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spacing") FVector2D PlatePad = FVector2D(32.f, 28.f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spacing") FVector2D SectionPad = FVector2D(20.f, 18.f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spacing") float EntryPadX = 16.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spacing") float ListGap = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spacing") float ColumnGap = 20.f;
};

/**
 * L0 Token — UI의 모든 값(색·반경·상태·모션·글꼴)을 한 곳에.
 * 에셋: DA_UIStyle (Data Asset › CC_UIStyleData). 새로 만들면 생성자 기본값(= v4 시안 값)이 채워져 있음.
 * 전역 기본은 Project Settings › Game › CristalCube UI › Default Style.
 */
UCLASS(BlueprintType)
class CRISTALCUBE_API UCC_UIStyleData : public UDataAsset
{
	GENERATED_BODY()

public:
	UCC_UIStyleData();

	// ── 팔레트 (sRGB) ─────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette") FColor Backdrop;
	/** Butter · Peach · Rose · Lilac · Sky · Mint */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette") TArray<FColor> SheenRamp;
	/** Filled 버튼 그라데이션 4단 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette") TArray<FColor> FilledStops;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette") FColor AccentNotice;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette|Text") FColor TextPrimary;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette|Text") FColor TextSecondary;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette|Text") FColor TextTertiary;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette|Text") FColor TextDisabled;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette|Text") FColor TextGhost;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette|Text") FColor TextEffect;
	/** Pip 채움 그라데이션 (Peach → Rose → Lilac) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette|Gauge") TArray<FColor> PipFill;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette|Gauge") FColor PipNext;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette|Gauge") FColor PipEmpty;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette|Gauge") TArray<FColor> GaugeHP;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette|Gauge") TArray<FColor> GaugeBoss;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette|Gauge") TArray<FColor> GaugeCubeEnergy;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Element")
	TMap<ESkillElementType, FUIElementTone> Elements;

	// ── Surface ─────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surface")
	TMap<EUISurfaceRole, TSoftObjectPtr<UMaterialInterface>> RoleMaterials;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surface")
	FUIRadiusTokens Radii;

	/** 모든 Surface 공통 상태 값 (Surface의 bUseStyleStates ✔일 때 사용) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surface")
	TMap<EUISurfaceState, FUISurfaceStateLook> StateLooks;

	// ── 모션 · 크기 · 간격 ───────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion") FUIMotionTokens Motion;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout") FUISizeTokens Sizes;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout") FUISpacingTokens Spacing;

	// ── 글꼴 ─────────────────────────────────────────────────
	/** 시안 px → Slate pt (Slate = 96 DPI 포인트 → 0.75). T-F1 실측 후 확정 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Text", meta = (ClampMin = "0.1"))
	float PxToSlatePt = 0.75f;

	/** 자간 보정 — Token의 LetterSpacing(CSS em×1000) × 이 값 = Slate LetterSpacing. T-F3 실측 후 확정 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Text", meta = (ClampMin = "0"))
	float LetterSpacingScale = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Text")
	TMap<EUITextStyle, FUITextStyle> TextStyles;

	// ── 조회 ─────────────────────────────────────────────────
	/** Project Settings › CristalCube UI › Default Style (없으면 nullptr) */
	UFUNCTION(BlueprintPure, Category = "UI Style", meta = (DisplayName = "Get UI Style"))
	static UCC_UIStyleData* GetUIStyle();

	/** sRGB(FColor) → Linear — 머티리얼 Vector Param · Slate 색에 넘길 값 */
	UFUNCTION(BlueprintPure, Category = "UI Style")
	static FLinearColor ToLinear(FColor SRGB) { return FLinearColor(SRGB); }

	/** 그라데이션 Stops(sRGB)를 T(0~1)에서 샘플 — sRGB 공간 보간(CSS와 같음) 후 Linear 반환 */
	UFUNCTION(BlueprintPure, Category = "UI Style")
	static FLinearColor SampleGradient(const TArray<FColor>& Stops, float T);

	UFUNCTION(BlueprintPure, Category = "UI Style")
	FLinearColor GetElementBodyLinear(ESkillElementType Element) const;

	UFUNCTION(BlueprintPure, Category = "UI Style")
	FLinearColor GetElementTextLinear(ESkillElementType Element) const;

	UFUNCTION(BlueprintPure, Category = "UI Style")
	float GetRadius(EUIRadius Radius) const;

	UFUNCTION(BlueprintCallable, Category = "UI Style")
	UMaterialInterface* GetRoleMaterial(EUISurfaceRole Role) const;

	/** Text Style → FSlateFontInfo (Size = SizePx × PxToSlatePt) */
	UFUNCTION(BlueprintCallable, Category = "UI Style")
	FSlateFontInfo MakeFontInfo(EUITextStyle Style) const;

	/** TextBlock에 글꼴·자간·색을 한 번에 적용 (PreConstruct에서 호출하면 디자이너에도 반영) */
	UFUNCTION(BlueprintCallable, Category = "UI Style", meta = (DisplayName = "Apply Text Style"))
	static void ApplyTextStyle(UTextBlock* Text, EUITextStyle Style, bool bApplyColor = true);
};

/** Project Settings › Game › CristalCube UI */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "CristalCube UI"))
class CRISTALCUBE_API UCC_UISettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** 전역 Token 에셋 (DA_UIStyle) */
	UPROPERTY(Config, EditAnywhere, Category = "Style")
	TSoftObjectPtr<UCC_UIStyleData> DefaultStyle;

	virtual FName GetCategoryName() const override { return TEXT("Game"); }
};