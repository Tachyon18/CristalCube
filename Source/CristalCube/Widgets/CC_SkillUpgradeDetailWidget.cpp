// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_SkillUpgradeDetailWidget.h"
#include "CC_AddonUpgradeCardWidget.h"
#include "CC_AddonUpgradeRowWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/ListView.h"
#include "Components/Button.h"
#include "../SkillSystem/CC_SkillBase.h"
#include "../SkillSystem/CC_SkillLibrarySubsystem.h"
#include "../SkillSystem/CC_AddonUpgradeEntryData.h"
#include "../CC_PlayerState.h"

void UCC_SkillUpgradeDetailWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (AddonCardContainer)
    {
        AddonCardContainer->OnEntryWidgetGenerated().AddUObject(
            this, &UCC_SkillUpgradeDetailWidget::HandleEntryWidgetGenerated);
    }

    if (CloseButton && !CloseButton->OnClicked.IsAlreadyBound(this, &UCC_SkillUpgradeDetailWidget::HandleCloseClicked))
    {
        CloseButton->OnClicked.AddDynamic(this, &UCC_SkillUpgradeDetailWidget::HandleCloseClicked);
    }
}

void UCC_SkillUpgradeDetailWidget::ShowSkill(UCC_SkillBase* Skill, ACC_PlayerState* PlayerState)
{
    if (!Skill || !PlayerState) return;

    CurrentSkill = Skill;
    BoundPlayerState = PlayerState;

    UCC_SkillLibrarySubsystem* SkillLibrary =
        GetWorld() ? GetWorld()->GetGameInstance()->GetSubsystem<UCC_SkillLibrarySubsystem>() : nullptr;
    if (!SkillLibrary) return;

    // 헤더
    FSkillDisplayData SkillData;
    if (SkillLibrary->GetSkillDisplayData(Skill->GetSkillID(), SkillData))
    {
        if (SkillIcon) SkillIcon->SetBrushFromTexture(SkillData.Icon);
        if (SkillName) SkillName->SetText(SkillData.DisplayName);
        if (SkillDescription) SkillDescription->SetText(SkillData.Description);
    }

    // Addon 카드 목록 재구성 — ListView라 SetListItems 한 번이면 내부적으로 위젯 생성/재사용까지 처리됨
    TArray<UObject*> Items;

    for (const UCC_SkillAddonBase* Addon : Skill->GetDefinition().Addons)
    {
        if (!Addon) continue;

        FAddonTableRow AddonRow;
        if (!SkillLibrary->GetAddonDisplayDataByType(Addon->AddonType, AddonRow)) continue;
        if (AddonRow.UpgradeAttributes.Num() == 0) continue;   // 배분 가능 속성 없으면 카드 스킵

        UCC_AddonUpgradeEntryData* EntryData = NewObject<UCC_AddonUpgradeEntryData>(this);
        EntryData->AddonType = Addon->AddonType;
        EntryData->AddonRow = AddonRow;
        EntryData->UnspentPoints = PlayerState->GetAddonUnspentPoints(Addon->AddonType);

        for (const FAddonUpgradeAttribute& Attr : AddonRow.UpgradeAttributes)
        {
            EntryData->SpentPoints.Add(Attr.AttributeID,
                PlayerState->GetAddonAttributeSpentPoints(Addon->AddonType, Attr.AttributeID));
        }

        Items.Add(EntryData);
    }

    if (AddonCardContainer)
    {
        AddonCardContainer->SetListItems(Items);
    }

    // Core 강화 4행 — 아직 배분 백엔드 없음(SkillCorePoints 배분 로직 미구현). 다음 세션 대상, 지금은 숨김.
    if (CoreDamageRow) CoreDamageRow->SetVisibility(ESlateVisibility::Collapsed);
    if (CoreSizeRow) CoreSizeRow->SetVisibility(ESlateVisibility::Collapsed);
    if (CoreSpeedRow) CoreSpeedRow->SetVisibility(ESlateVisibility::Collapsed);
    if (CoreProjectileCountRow) CoreProjectileCountRow->SetVisibility(ESlateVisibility::Collapsed);

}

void UCC_SkillUpgradeDetailWidget::HandleCardPointRequested(ESkillAddonType AddonType, FName AttributeID, bool bIsRefund)
{
    if (!BoundPlayerState.IsValid()) return;

    UCC_SkillLibrarySubsystem* SkillLibrary =
        GetWorld() ? GetWorld()->GetGameInstance()->GetSubsystem<UCC_SkillLibrarySubsystem>() : nullptr;
    if (!SkillLibrary) return;

    FAddonTableRow AddonRow;
    if (!SkillLibrary->GetAddonDisplayDataByType(AddonType, AddonRow)) return;

    const FAddonUpgradeAttribute* AttrDef = AddonRow.UpgradeAttributes.FindByPredicate(
        [&AttributeID](const FAddonUpgradeAttribute& A) { return A.AttributeID == AttributeID; });
    if (!AttrDef) return;

    const bool bSuccess = bIsRefund
        ? BoundPlayerState->RefundAddonPoint(AddonType, AttributeID, AttrDef->ValuePerPoint)
        : BoundPlayerState->SpendAddonPoint(AddonType, AttributeID, AttrDef->ValuePerPoint, AttrDef->MaxPoints);

    if (bSuccess && CurrentSkill.IsValid())
    {
        ShowSkill(CurrentSkill.Get(), BoundPlayerState.Get());
    }
}

void UCC_SkillUpgradeDetailWidget::HandleCloseClicked()
{
    OnCloseRequested.Broadcast();
}

void UCC_SkillUpgradeDetailWidget::HandleEntryWidgetGenerated(UUserWidget& EntryWidget)
{
    if (UCC_AddonUpgradeCardWidget* Card = Cast<UCC_AddonUpgradeCardWidget>(&EntryWidget))
    {
        if (!Card->OnPointRequested.IsAlreadyBound(this, &UCC_SkillUpgradeDetailWidget::HandleCardPointRequested))
        {
            Card->OnPointRequested.AddDynamic(this, &UCC_SkillUpgradeDetailWidget::HandleCardPointRequested);
        }
    }
}
