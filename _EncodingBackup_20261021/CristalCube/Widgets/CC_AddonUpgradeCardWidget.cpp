// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_AddonUpgradeCardWidget.h"
#include "CC_AddonUpgradeRowWidget.h"
#include "../SkillSystem/CC_AddonUpgradeEntryData.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/PanelWidget.h"

void UCC_AddonUpgradeCardWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

    const UCC_AddonUpgradeEntryData* EntryData = Cast<UCC_AddonUpgradeEntryData>(ListItemObject);
    if (!EntryData) return;

    if (AddonIcon) AddonIcon->SetBrushFromTexture(EntryData->AddonRow.Icon);
    if (AddonName) AddonName->SetText(EntryData->AddonRow.DisplayName);
    if (BankPointsText)
    {
        BankPointsText->SetText(FText::FromString(
            FString::Printf(TEXT("보유 포인트: %d"), EntryData->UnspentPoints)));
    }

    RebuildAttributeRows(EntryData);
}

void UCC_AddonUpgradeCardWidget::RebuildAttributeRows(const UCC_AddonUpgradeEntryData* EntryData)
{
    if (!AttributeRowContainer || !AddonUpgradeRowWidgetClass) return;

    // TileView/컨테이너 재사용 위젯이라 매번 비우고 다시 채움 — RefreshAddonBadges()와 동일 원칙
    AttributeRowContainer->ClearChildren();

    for (const FAddonUpgradeAttribute& Attr : EntryData->AddonRow.UpgradeAttributes)
    {
        UCC_AddonUpgradeRowWidget* Row = CreateWidget<UCC_AddonUpgradeRowWidget>(this, AddonUpgradeRowWidgetClass);
        if (!Row) continue;

        Row->SetAttributeData(EntryData->AddonType, Attr,
            EntryData->SpentPoints.FindRef(Attr.AttributeID), EntryData->UnspentPoints);
        Row->OnPointRequested.AddDynamic(this, &UCC_AddonUpgradeCardWidget::HandleRowPointRequested);

        AttributeRowContainer->AddChild(Row);
    }
}

void UCC_AddonUpgradeCardWidget::HandleRowPointRequested(ESkillAddonType AddonType, FName AttributeID, bool bIsRefund)
{
    // 카드 자신은 PlayerState를 안 건드림(표시 전용 원칙 유지) — 위로 그대로 다시 브로드캐스트해서
    // SkillUpgradeDetailWidget이 실제 배분/회수 + 새로고침을 담당하게 함.
    OnPointRequested.Broadcast(AddonType, AttributeID, bIsRefund);
}