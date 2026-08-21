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

        // 스펜트 카운트는 EntryData가 안 들고 있음(카드 하나가 카탈로그+은행만 담당) —
        // 컨테이너(SkillUpgradeDetailWidget)가 SetAttributeData 호출 전에 PlayerState에서
        // 직접 조회해서 넘기는 쪽이 책임 분리에 맞음. 여기서는 0으로 넘기고, 실제 착수 시
        // EntryData에 스펜트 스냅샷도 같이 캐싱하거나, Row 생성 후 컨테이너가 다시 SetAttributeData를
        // 호출해 덮어쓰는 방식 중 하나로 결정 필요 — 지금은 후자를 권장(카드가 PlayerState를 직접 참조 안 하게).
        Row->SetAttributeData(EntryData->AddonType, Attr, /*CurrentSpent*/ 0, EntryData->UnspentPoints);
        Row->OnSpendRequested.AddDynamic(this, &UCC_AddonUpgradeCardWidget::HandleRowSpendRequested);

        AttributeRowContainer->AddChild(Row);
    }
}

void UCC_AddonUpgradeCardWidget::HandleRowSpendRequested(ESkillAddonType AddonType, FName AttributeID)
{
    // 카드 자신은 PlayerState를 안 건드림(표시 전용 원칙 유지, CC_SkillSlotWidget과 동일 패턴) —
    // 위로 그대로 다시 브로드캐스트해서 SkillUpgradeDetailWidget이 실제 배분+새로고침을 담당하게 함.
    OnSpendRequested.Broadcast(AddonType, AttributeID);
}
