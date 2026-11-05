// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../CristalCubeStruct.h"
#include "CC_AddonUpgradeCardWidget.generated.h"

class UButton;
class UTextBlock;
class UPanelWidget;
class USizeBox;
class UCC_UISurface;
class UCC_UIIconWell;
class UCC_UICounter;
class UCC_AddonUpgradeRowWidget;
class UCC_AddonUpgradeCardWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnAddonAttributePointRequestedFromCard,
	ESkillAddonType, AddonType, FName, AttributeID, bool, bIsRefund);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAddonCardToggleRequested, UCC_AddonUpgradeCardWidget*, Card);

/**
 * WBP_AddonUpgradeCard 구조 (이름 일치):
 *   [Root] Overlay
 *     ├ CardSurface (WBP_UI_Surface, Fill/Fill)                 BindWidget — Role/반경은 C++ (Card · SubCard 16)
 *     └ Vertical Box (Fill)
 *        ├ Overlay  ← 헤더 (HeaderSizeBox로 높이 66 고정해도 됨)
 *        │   ├ Horizontal Box (Padding 14,12 · 세로 Center)
 *        │   │   ├ AddonWell (WBP_UI_IconWell — Well Size S)
 *        │   │   ├ Spacer 14×1
 *        │   │   ├ Vertical Box (Fill)
 *        │   │   │   ├ Horizontal Box : AddonName · Spacer 8 · TotalText (선택) · Spacer 8 · NoticeDot (WBP_UI_NoticeDot, 선택)
 *        │   │   │   └ SharedSkillsText (선택) — "공유 · Fireball · MeteorShower"
 *        │   │   ├ BankCounter (WBP_UI_Counter, 선택 — Show Surface ✘)
 *        │   │   ├ Spacer 10×1
 *        │   │   └ ExpandChevron (Image 16×16 또는 TextBlock "⌄", 선택 — 펼침 시 180° 회전)
 *        │   └ HeaderButton (Button, Fill/Fill, 맨 위)          BindWidget — 스타일은 C++가 투명으로
 *        └ AttributeRowContainer (Vertical Box, Padding 68,0,14,8)   BindWidget — 행은 C++가 채움
 */
UCLASS()
class CRISTALCUBE_API UCC_AddonUpgradeCardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 카드 내용 설정. 같은 Addon · 같은 속성 구성이면 행 위젯 재사용(값만 갱신) */
	void SetAddonData(ESkillAddonType InAddonType, const FAddonTableRow& InRow, int32 BankPoints,
		const TMap<FName, int32>& SpentPoints, const FText& InSharedText);

	/** 펼침/접힘 — Detail이 "한 번에 하나"로 지시. 펼칠 때만 행 오퍼시티 페이드, 접을 때는 즉시 */
	UFUNCTION(BlueprintCallable, Category = "Addon Upgrade")
	void SetExpanded(bool bInExpanded, bool bInstant = false);

	UFUNCTION(BlueprintPure, Category = "Addon Upgrade")
	bool IsExpanded() const { return bExpanded; }

	UFUNCTION(BlueprintPure, Category = "Addon Upgrade")
	ESkillAddonType GetAddonType() const { return AddonType; }

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Addon Upgrade")
	TSubclassOf<UCC_AddonUpgradeRowWidget> AddonUpgradeRowWidgetClass;

	UPROPERTY(BlueprintAssignable, Category = "Addon Upgrade")
	FOnAddonAttributePointRequestedFromCard OnPointRequested;

	/** 헤더 클릭 — 실제 펼침은 Detail이 SetExpanded로 */
	UPROPERTY(BlueprintAssignable, Category = "Addon Upgrade")
	FOnAddonCardToggleRequested OnToggleRequested;

	// ── 디자이너 미리보기 ──
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Addon Upgrade|Preview")
	bool bPreviewExpanded = true;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCC_UISurface> CardSurface;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCC_UIIconWell> AddonWell;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> AddonName;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> HeaderButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> AttributeRowContainer;

	/** 합계 "3 / 10" (Numeric · Secondary) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TotalText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SharedSkillsText;

	/** 이 Addon 종류의 미사용 포인트 (Show Surface ✘ = 글자형) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCC_UICounter> BankCounter;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> NoticeDot;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> ExpandChevron;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> HeaderSizeBox;

private:
	UFUNCTION() void HandleHeaderClicked();
	UFUNCTION() void HandleHeaderHovered();
	UFUNCTION() void HandleHeaderUnhovered();
	UFUNCTION() void HandleRowPointRequested(ESkillAddonType InAddonType, FName AttributeID, bool bIsRefund);

	void ApplySurfaceConfig();
	void ApplySurfaceState(bool bInstant);
	void ApplyExpandedVisual(bool bInstant);
	void MakeHeaderButtonTransparent();
	float GetFadeTime() const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UCC_AddonUpgradeRowWidget>> RowWidgets;

	ESkillAddonType AddonType = ESkillAddonType::None;
	bool bExpanded = false;
	bool bHeaderHovered = false;
	bool bCardDataSet = false;
	float RowsFadeElapsed = -1.f;          // < 0 = 페이드 없음
};
