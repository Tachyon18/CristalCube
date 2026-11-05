// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CC_SkillInventoryWidget.generated.h"

class UCC_SkillRosterEntryWidget;
class UCC_SkillUpgradeDetailWidget;
class UCC_SkillBase;
class UCC_UISurface;
class ACC_PlayerState;
class UPanelWidget;
class UTextBlock;

/**
 * WBP_SkillInventory 구조 (이름 일치):
 *   [Root] Canvas Panel
 *     └ Overlay (Anchor 가운데 · Size 1340×820)
 *        ├ InventoryPlate (WBP_UI_Surface_Plate — 블러 포함, 선택)       C++: Plate · Plate 30 · 열릴 때 Intro
 *        └ Vertical Box (Padding 32,28)
 *           ├ Horizontal Box : TitleText "SKILL INVENTORY" · (Fill) · GlobalNotice [NoticeDot · Spacer 8 · GlobalNoticeText "미사용 포인트 있음"] (선택)
 *           ├ Spacer 1×18
 *           ├ Horizontal Box (Fill)
 *           │   ├ Size Box W 330 › Vertical Box : ListCaptionText "장착 스킬 · 6" (선택) · Spacer 1×10 · RosterContainer (Vertical Box)
 *           │   ├ Spacer 20×1
 *           │   └ DetailPanel (WBP_SkillUpgradeDetail, Fill)
 *           ├ Spacer 1×18
 *           └ FooterText (선택) "왼쪽 목록 클릭 · 스킬 전환 · Addon 카드 클릭 · 강화 항목 열기"
 */
UCLASS()
class CRISTALCUBE_API UCC_SkillInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Skill Inventory")
	void RefreshInventory();

	/** 슬롯 선택 (빈 슬롯이면 무시) */
	UFUNCTION(BlueprintCallable, Category = "Skill Inventory")
	void SelectSlot(int32 SlotIndex);

	/** Plate 테두리 Intro — NativeConstruct에서 자동. 열기를 Visibility로 하는 경우 여는 쪽에서 호출 */
	UFUNCTION(BlueprintCallable, Category = "Skill Inventory")
	void PlayOpenIntro();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill Inventory")
	TSubclassOf<UCC_SkillRosterEntryWidget> RosterEntryWidgetClass;

	// ── 테스트용 (Shipping 제외) — 열 때 포인트 지급. 0 = 끔 ──
	/** 장착 스킬마다 Core 포인트 N */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Inventory|Debug", meta = (ClampMin = "0"))
	int32 DebugGrantCorePoints = 0;

	/** 장착 스킬이 가진 Addon 종류마다 포인트 N */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Inventory|Debug", meta = (ClampMin = "0"))
	int32 DebugGrantAddonPoints = 0;

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> RosterContainer;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCC_SkillUpgradeDetailWidget> DetailPanel;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCC_UISurface> InventoryPlate;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> ListCaptionText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> GlobalNotice;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> GlobalNoticeText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> FooterText;

private:
	UFUNCTION() void HandleEntrySelectRequested(UCC_SkillRosterEntryWidget* Entry);
	UFUNCTION() void HandleSlotDropRequested(int32 SourceSlotIndex, int32 TargetSlotIndex);
	UFUNCTION() void HandleSkillsChanged();
	UFUNCTION() void HandleDetailPointsChanged();

	void EnsureEntries();
	void RefreshRosterData();
	void ResolveSelection();
	void ApplySelection(bool bInstant);
	void RefreshGlobalNotice();
	void DebugGrantPoints();

	static constexpr int32 NumEquipSlots = 6;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UCC_SkillRosterEntryWidget>> RosterEntries;

	UPROPERTY(Transient)
	TObjectPtr<ACC_PlayerState> BoundPlayerState;

	/** 선택은 스킬로 기억 — 슬롯 교환 후에도 같은 스킬을 따라감 */
	TWeakObjectPtr<UCC_SkillBase> SelectedSkill;
	int32 SelectedSlot = INDEX_NONE;
};

