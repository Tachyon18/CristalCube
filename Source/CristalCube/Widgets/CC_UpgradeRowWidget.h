// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CC_UpgradeRowWidget.generated.h"

class UTextBlock;
class UWidget;
class UCC_UIGaugePip;
class UCC_UIIconButton;
class UCC_UIControl;

// CC_UpgradeRowWidget.h — L3 공통 부모: 강화 행 (Core 행 · Addon 펼침 행이 같은 스킨)
// 이름 + 효과값 + Pip + "3 / 10" + −/+ (캔버스 InvLayoutB). 표시 전용 — 실제 배분은 Detail이 PlayerState에 요청.
// 파생(Core/Addon)은 '무엇을 올리는가'(ID)와 자기 델리게이트만 가짐 → RequestPoint 오버라이드.


/**
 * WBP 구조 (WBP_CoreUpgradeRow · WBP_AddonUpgradeRow 공통 — 이름 일치):
 *   [Root] Vertical Box
 *     ├ Horizontal Box (Padding 0,9,0,9 · 세로 Center)
 *     │   ├ Vertical Box (Fill)
 *     │   │   ├ Horizontal Box : AttributeName (TextBlock) · Spacer 10×1 · EffectText (TextBlock, 선택)
 *     │   │   ├ Spacer 1×7
 *     │   │   └ PipGauge (WBP_UI_GaugePip, 선택)
 *     │   ├ Size Box W 44 (Addon 40) › PointsText (TextBlock, 오른쪽 정렬)
 *     │   ├ Spacer 10×1 · MinusButton (WBP_UI_IconButton, 선택 — Glyph Minus)
 *     │   └ Spacer 10×1 · PlusButton  (WBP_UI_IconButton — Glyph Plus · bAccent ✔)
 *     └ RowDivider (WBP_UI_Divider, 선택 — 행 사이 1px 흰 5%)
 *   Addon 행: IconButton Button Size = Compact (28)
 */
UCLASS(Abstract)
class CRISTALCUBE_API UCC_UpgradeRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 맨 끝 행이면 구분선 숨김 (Detail · Card가 지정) */
	UFUNCTION(BlueprintCallable, Category = "Upgrade Row")
	void SetDividerVisible(bool bVisible);

	// ── 디자이너 미리보기 (런타임에는 SetAttributeData 값) ──
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade Row|Preview")
	FText PreviewName = NSLOCTEXT("CC_UI", "UpgradeRowPreviewName", "피해량");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade Row|Preview")
	FText PreviewEffect = NSLOCTEXT("CC_UI", "UpgradeRowPreviewEffect", "+30%");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade Row|Preview", meta = (ClampMin = "0"))
	int32 PreviewSpent = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade Row|Preview", meta = (ClampMin = "0", ClampMax = "30"))
	int32 PreviewMax = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade Row|Preview", meta = (ClampMin = "0"))
	int32 PreviewBank = 2;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativePreConstruct() override;

	/** 공통 표시 — 파생 SetAttributeData가 호출 */
	void ApplyRow(const FText& InName, const FText& InEffect, int32 Spent, int32 Max, int32 Bank);

	/** −/+ 클릭 → 파생이 자기 델리게이트로 위에 요청 */
	virtual void RequestPoint(bool bIsRefund) {}

	/** 누적 효과 → "+30%" · "−8%" (Core 배율 — ValuePerPoint 0.1 = 10%) */
	static FText FormatPercent(float Value);

	/** 누적 효과 → "+20" · "−0.4" (Addon — 단위가 속성마다 달라 숫자만, 단위 표는 B6) */
	static FText FormatSigned(float Value);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> AttributeName;

	/** "3 / 10" (MaxPoints 0 = 무제한이면 "3") */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PointsText;

	/** + (배분) — 은행 0 또는 최대면 Disabled */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCC_UIIconButton> PlusButton;

	/** − (회수) — 배분 0이면 Disabled */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCC_UIIconButton> MinusButton;

	/** "+30%" (Text Style Effect · Mint) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EffectText;

	/** 칸 수 = MaxPoints (MaxPoints 0이면 숨김) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCC_UIGaugePip> PipGauge;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> RowDivider;

private:
	UFUNCTION()
	void HandlePlusClicked(UCC_UIControl* Control);

	UFUNCTION()
	void HandleMinusClicked(UCC_UIControl* Control);

	void ApplyStyles();
	void ApplyRowVisual(const FText& InName, const FText& InEffect, int32 Spent, int32 Max, int32 Bank);

	bool bRowDataSet = false;
};