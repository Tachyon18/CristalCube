// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "CC_AddonUpgradeCardWidget.generated.h"

class UCC_AddonUpgradeEntryData;
class UCC_AddonUpgradeRowWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAddonAttributeSpendRequestedFromCard,
	ESkillAddonType, AddonType, FName, AttributeID);

/**
 * AddonUpgradeTileView의 항목 1개 = Addon 타입 1개.
 * 헤더(아이콘+이름+은행 잔액) + UpgradeAttributes 개수만큼 CC_AddonUpgradeRowWidget 동적 생성.
 *
 * [Blueprint 설정]
 *  1. 이 클래스를 부모로 WBP_AddonUpgradeCard 생성
 *  2. AttributeRowContainer는 VerticalBox로 준비 (행 개수가 Addon마다 2~3개로 다름)
 *  3. AddonUpgradeRowWidgetClass에 WBP_AddonUpgradeRow 지정
 */
UCLASS()
class CRISTALCUBE_API UCC_AddonUpgradeCardWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Addon Upgrade")
    TSubclassOf<UCC_AddonUpgradeRowWidget> AddonUpgradeRowWidgetClass;

    UPROPERTY(BlueprintAssignable, Category = "Addon Upgrade")
    FOnAddonAttributeSpendRequestedFromCard OnSpendRequested;

protected:
    virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

    UPROPERTY(meta = (BindWidget))
    class UImage* AddonIcon;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* AddonName;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* BankPointsText;

    UPROPERTY(meta = (BindWidget))
    class UPanelWidget* AttributeRowContainer;

private:

    /** ListItemObject를 새로 세팅할 때마다 컨테이너를 비우고 다시 채움 — TileView가 위젯을 재활용하므로 필수 */
    void RebuildAttributeRows(const class UCC_AddonUpgradeEntryData* EntryData);

    UFUNCTION()
    void HandleRowSpendRequested(ESkillAddonType AddonType, FName AttributeID);
    	
};
