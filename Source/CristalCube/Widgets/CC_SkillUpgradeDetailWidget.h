// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../CristalCubeStruct.h"
#include "CC_SkillUpgradeDetailWidget.generated.h"

class UCC_SkillBase;
class ACC_PlayerState;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUpgradePanelCloseRequested);

/**
 * 
 */
UCLASS()
class CRISTALCUBE_API UCC_SkillUpgradeDetailWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual void NativeConstruct() override;

    /** SkillRoster에서 스킬 선택 시 컨테이너가 호출 — 이 스킬 기준으로 Addon/Core 섹션 재구성 */
    UFUNCTION(BlueprintCallable, Category = "Skill Upgrade")
    void ShowSkill(UCC_SkillBase* Skill, ACC_PlayerState* PlayerState);

protected:
    UPROPERTY(meta = (BindWidget)) class UImage* SkillIcon;
    UPROPERTY(meta = (BindWidget)) class UTextBlock* SkillName;
    UPROPERTY(meta = (BindWidget)) UTextBlock* SkillDescription;

    /** 이 스킬이 가진 Addon 카드 리스트. Card(UCC_AddonUpgradeCardWidget)가 IUserObjectListEntry를
     *  구현하고 있으므로 그 계약대로 ListView를 사용 — PanelWidget에 수동으로 채우면
     *  NativeOnListItemObjectSet이 인터페이스 단에서부터 protected라 컴파일 자체가 안 됨. */
    UPROPERTY(meta = (BindWidget)) class UListView* AddonCardContainer;

    /** Core 강화 4행(Damage/Size/Speed/ProjectileCount) — 고정 개수라 BindWidget 4개로 직접 배치 */
    UPROPERTY(meta = (BindWidget)) class UCC_AddonUpgradeRowWidget* CoreDamageRow;
    UPROPERTY(meta = (BindWidget)) UCC_AddonUpgradeRowWidget* CoreSizeRow;
    UPROPERTY(meta = (BindWidget)) UCC_AddonUpgradeRowWidget* CoreSpeedRow;
    UPROPERTY(meta = (BindWidget)) UCC_AddonUpgradeRowWidget* CoreProjectileCountRow;

    UPROPERTY(meta = (BindWidgetOptional)) class UButton* CloseButton;

    UFUNCTION()
    void HandleCardPointRequested(ESkillAddonType AddonType, FName AttributeID, bool bIsRefund);

    UFUNCTION()
    void HandleCloseClicked();

public:

    /** 닫기 버튼 클릭 시 브로드캐스트. 이 위젯 자신은 Visibility를 안 건드림 —
     *  WidgetSwitcher의 자식이라 스스로 숨겨봤자 의미 없고, "그리드로 돌아가기"는
     *  이 이벤트를 구독하는 컨테이너(SkillInventoryWidget)가 SetActiveWidgetIndex(0)으로 처리. */
    UPROPERTY(BlueprintAssignable, Category = "Skill Upgrade")
    FOnUpgradePanelCloseRequested OnCloseRequested;

private:
    TWeakObjectPtr<UCC_SkillBase> CurrentSkill;
    TWeakObjectPtr<ACC_PlayerState> BoundPlayerState;
	
    /** ListView가 엔트리 위젯(Card)을 새로 만들 때마다 호출됨 — 여기서 Card의 OnSpendRequested를 구독 */
    void HandleEntryWidgetGenerated(UUserWidget& EntryWidget);
};
