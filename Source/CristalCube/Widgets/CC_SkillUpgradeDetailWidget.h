// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../CristalCubeStruct.h"
#include "CC_SkillUpgradeDetailWidget.generated.h"

class UCC_SkillBase;
class ACC_PlayerState;
class UCC_SkillLibrarySubsystem;
class UCC_UISurface;
class UCC_UIIconWell;
class UCC_UICounter;
class UCC_CoreUpgradeRowWidget;
class UCC_AddonUpgradeCardWidget;
class UTextBlock;
class UPanelWidget;
class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUpgradePanelCloseRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUpgradePointsChanged);

/**
 * WBP_SkillUpgradeDetail 구조 (이름 일치 — 굵은 글씨 = BindWidget, 나머지 선택):
 *   [Root] Vertical Box
 *     ├ Overlay  ← 헤더 카드
 *     │   ├ HeaderSurface (WBP_UI_Surface)                         C++: Card · Section 22 · Selected · 원소
 *     │   └ Horizontal Box (Padding 22,18 · 세로 Center)
 *     │       ├ SkillWell (WBP_UI_IconWell — Well Size L)
 *     │       ├ Spacer 16×1
 *     │       └ Vertical Box (Fill)
 *     │           ├ Horizontal Box : SkillName · Spacer 10 · SkillMetaText ("Projectile · ") · ElementText ("Red 공격")
 *     │           └ SkillDescription
 *     ├ Spacer 1×14
 *     └ Horizontal Box (Fill)  ← 두 열 (각 슬롯 Fill 1.0, 사이 Spacer 14)
 *         ├ Overlay  ← Core 섹션
 *         │   ├ CoreSurface (WBP_UI_Surface)                       C++: Card · Section 22 · Rest
 *         │   └ Vertical Box (Padding 20,18)
 *         │       ├ Horizontal Box : VerticalBox[CoreLabel "CORE 강화" · CoreSubLabel "이 스킬에만 적용"] · (Fill) · CoreBankCounter (WBP_UI_Counter)
 *         │       └ **CoreAttributeContainer** (Vertical Box)
 *         └ Overlay  ← Addon 섹션
 *             ├ AddonSurface (WBP_UI_Surface)                      C++: Card · Section 22 · Rest
 *             └ Vertical Box (Padding 20,18)
 *                 ├ AddonLabel "ADDON 강화" · AddonSubLabel "같은 Addon을 가진 모든 스킬에 함께 적용"
 *                 ├ AddonEmptyText ("Addon 없음", 선택)
 *                 └ Scroll Box (Fill) › **AddonCardBox** (Vertical Box)
 */
UCLASS()
class CRISTALCUBE_API UCC_SkillUpgradeDetailWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 이 스킬 기준으로 헤더 · Core · Addon 표시. 같은 스킬이면 위젯 재사용 · 값만 갱신 */
	UFUNCTION(BlueprintCallable, Category = "Skill Upgrade")
	void ShowSkill(UCC_SkillBase* Skill, ACC_PlayerState* PlayerState);

	/** 장착 스킬이 하나도 없을 때 */
	UFUNCTION(BlueprintCallable, Category = "Skill Upgrade")
	void ShowEmpty();

	/** 배분/회수 성공 시 — 인벤토리가 로스터 요약 · Notice Dot 갱신 */
	UPROPERTY(BlueprintAssignable, Category = "Skill Upgrade")
	FOnUpgradePointsChanged OnPointsChanged;

	/** (이전 호환) 닫기 버튼 — CloseButton을 배치했을 때만 */
	UPROPERTY(BlueprintAssignable, Category = "Skill Upgrade")
	FOnUpgradePanelCloseRequested OnCloseRequested;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill Upgrade")
	TSubclassOf<UCC_CoreUpgradeRowWidget> CoreAttributeRowWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill Upgrade")
	TSubclassOf<UCC_AddonUpgradeCardWidget> AddonCardWidgetClass;

	/** 스킬을 바꿀 때 헤더 테두리 Intro (PlayRimIntro) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Upgrade")
	bool bHeaderIntroOnSkillChange = true;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> CoreAttributeContainer;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> AddonCardBox;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCC_UISurface> HeaderSurface;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCC_UISurface> CoreSurface;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCC_UISurface> AddonSurface;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCC_UIIconWell> SkillWell;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> SkillName;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> SkillMetaText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> ElementText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> SkillDescription;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCC_UICounter> CoreBankCounter;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> CoreLabel;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> CoreSubLabel;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> AddonLabel;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> AddonSubLabel;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> AddonEmptyText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> CloseButton;

private:
	UFUNCTION() void HandleCorePointRequested(ECoreUpgradeAttribute AttributeType, bool bIsRefund);
	UFUNCTION() void HandleCardPointRequested(ESkillAddonType AddonType, FName AttributeID, bool bIsRefund);
	UFUNCTION() void HandleCardToggleRequested(UCC_AddonUpgradeCardWidget* Card);
	UFUNCTION() void HandleCloseClicked();

	UCC_SkillLibrarySubsystem* GetSkillLibrary() const;
	void ApplySurfaceConfig();
	void RefreshHeader(UCC_SkillBase* Skill, UCC_SkillLibrarySubsystem* Library);
	void RefreshCore(UCC_SkillBase* Skill, ACC_PlayerState* PlayerState, UCC_SkillLibrarySubsystem* Library, bool bSkillChanged);
	void RefreshAddons(UCC_SkillBase* Skill, ACC_PlayerState* PlayerState, UCC_SkillLibrarySubsystem* Library, bool bSkillChanged);
	FText BuildSharedText(ESkillAddonType AddonType, ACC_PlayerState* PlayerState, UCC_SkillLibrarySubsystem* Library) const;
	static FText GetElementLabel(ESkillElementType Element);

	TWeakObjectPtr<UCC_SkillBase> CurrentSkill;
	TWeakObjectPtr<ACC_PlayerState> BoundPlayerState;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UCC_CoreUpgradeRowWidget>> CoreRows;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UCC_AddonUpgradeCardWidget>> AddonCards;

	/** 지금 펼친 Addon (None = 전부 접힘). 같은 스킬 재표시 때 유지, 스킬 바뀌면 첫 카드 */
	ESkillAddonType ExpandedAddon = ESkillAddonType::None;

	ESkillElementType HeaderElement = ESkillElementType::None;
};