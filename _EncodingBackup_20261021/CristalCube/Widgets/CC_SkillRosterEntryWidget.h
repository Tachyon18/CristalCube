// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../CristalCubeStruct.h"
#include "CC_SkillRosterEntryWidget.generated.h"

class UCC_SkillBase;
class ACC_PlayerState;
class UCC_SkillUpgradeDetailWidget;
class UCC_AddonBadgeWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRosterEntryExpandRequested, class UCC_SkillRosterEntryWidget*, Entry);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRosterSlotDropped, int32, SourceSlotIndex, int32, TargetSlotIndex);

/**
 * Inventory 리스트 한 줄 = "카드"(아이콘/이름/화살표/Addon 아이콘 요약) + 그 아래
 * WBP_SkillUpgradeDetail 인스턴스("서랍"). 카드는 접힘/펼침과 무관하게 항상 같은 모양이고,
 * 펼침 여부에 따라 드로어의 Visibility만 바뀐다.
 *
 * "하나만 펼쳐지는" 아코디언 판단은 이 위젯이 하지 않는다 — 클릭하면 OnExpandRequested를
 * 위로만 브로드캐스트하고, 실제로 펼치고/접는 건 부모(CC_SkillInventoryWidget)가
 * SetExpanded()로 지시한다.
 */
UCLASS()
class CRISTALCUBE_API UCC_SkillRosterEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry & InGeometry, const FPointerEvent & InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, class UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, class UDragDropOperation* InOperation) override;

public:

	/** 이 슬롯의 스킬 데이터로 카드를 채움. */
	UFUNCTION(BlueprintCallable, Category = "Skill Roster")
	void SetSkillData(int32 InSlotIndex, UCC_SkillBase* Skill, ACC_PlayerState* PlayerState);

	/** 부모가 아코디언 상태를 관장하며 호출 — 실제 펼침/접힘은 이 함수만이 바꿈. */
	UFUNCTION(BlueprintCallable, Category = "Skill Roster")
	void SetExpanded(bool bNewExpanded);

	/** 부모가 형제 카드를 숨길 때 호출 — 카드 전체를 페이드 아웃시키고 끝나면 Collapsed. */
	UFUNCTION(BlueprintCallable, Category = "Skill Roster")
	void PlayFadeOut();

	/** 부모가 형제 카드를 되돌릴 때 호출 — Visible로 바꾸고 카드 전체를 페이드 인. */
	UFUNCTION(BlueprintCallable, Category = "Skill Roster")
	void PlayFadeIn();

	/** Inventory↔Detail 전환 시퀀스의 지연 시간을 부모가 그대로 따라 쓰도록 노출. */
	UFUNCTION(BlueprintPure, Category = "Skill Roster")
	float GetDrawerAnimDuration() const { return DrawerAnimDuration; }

	UFUNCTION(BlueprintPure, Category = "Skill Roster")
	bool IsExpanded() const { return bIsExpanded; }

	UFUNCTION(BlueprintPure, Category = "Skill Roster")
	int32 GetSlotIndex() const { return SlotIndex; }

	UPROPERTY(BlueprintAssignable, Category = "Skill Roster")
	FOnRosterEntryExpandRequested OnExpandRequested;

	/** Ctrl-드래그로 슬롯이 재배치되었을 때 부모(Inventory)에게 통지 — 실제 SwapSlots 호출은 부모 담당 */
	UPROPERTY(BlueprintAssignable, Category = "Skill Roster")
	FOnRosterSlotDropped OnSlotDropRequested;

protected:
	//UPROPERTY(meta = (BindWidget)) class UButton* CardHeaderButton;
	UPROPERTY(meta = (BindWidget)) class UImage* SkillIcon;
	UPROPERTY(meta = (BindWidget)) class UTextBlock* SkillName;
	UPROPERTY(meta = (BindWidget)) UImage* ExpandChevron;

	/** 접힘 상태에서만 보이는 Addon 아이콘 요약 줄 (이름 없이 아이콘만) */
	UPROPERTY(meta = (BindWidget)) class UPanelWidget* AddonIconStrip;

	/** 실제 "서랍" — WBP_SkillUpgradeDetail 인스턴스를 이 이름으로 배치 */
	UPROPERTY(meta = (BindWidget)) UCC_SkillUpgradeDetailWidget* DrawerPanel;

	/** 빈 슬롯(미장착)일 때만 보이는 플레이스홀더 — 카드는 펼쳐지지 않고 드롭 타겟으로만 동작 */
	UPROPERTY(meta = (BindWidgetOptional)) class UWidget* EmptySlotOverlay;

	/** 접힘 상태 Addon 아이콘 뱃지 클래스 — CC_SkillCardWidget과 동일한 뱃지 재사용 가능 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill Roster")
	TSubclassOf<UCC_AddonBadgeWidget> AddonIconWidgetClass;

	/** 펼침/접힘 애니메이션 길이(초) — WBP 디폴트에서 튜닝 가능 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill Roster|Animation")
	float DrawerAnimDuration = 0.18f;

	UFUNCTION()
	void HandleCardHeaderClicked();

private:
	void RefreshAddonIconStrip(const TArray<FAddonTableRow>& AddonBadges);

	int32 SlotIndex = INDEX_NONE;
	bool bIsExpanded = false;

	bool bPendingClick = false;

	/** true인 동안 DrawerPanel의 오퍼시티/스케일만 보간 — 레이아웃 크기는 안 건드림. */
	bool bRevealAnimating = false;
	bool bRevealingIn = false;
	float RevealElapsed = 0.0f;

	// 카드 자신(형제 숨김용) 오퍼시티 트랙
	bool bSelfFadeAnimating = false;
	bool bSelfFadingIn = false;
	float SelfFadeElapsed = 0.0f;

	UPROPERTY()
	UCC_SkillBase* BoundSkill = nullptr;

	UPROPERTY()
	ACC_PlayerState* BoundPlayerState = nullptr;
	
};
