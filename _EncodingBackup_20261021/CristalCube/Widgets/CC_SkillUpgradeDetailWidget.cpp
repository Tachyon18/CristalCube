// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_SkillUpgradeDetailWidget.h"
#include "CC_AddonUpgradeCardWidget.h"
#include "CC_CoreUpgradeRowWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/ListView.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
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

    RebuildCoreAttributeRows(Skill, PlayerState, SkillLibrary);
}

void UCC_SkillUpgradeDetailWidget::HandleCorePointRequested(ECoreUpgradeAttribute AttributeType, bool bIsRefund)
{
    if (!BoundPlayerState.IsValid() || !CurrentSkill.IsValid()) return;

    UCC_SkillLibrarySubsystem* SkillLibrary =
        GetWorld() ? GetWorld()->GetGameInstance()->GetSubsystem<UCC_SkillLibrarySubsystem>() : nullptr;
    if (!SkillLibrary) return;

    const FName SkillID = CurrentSkill->GetSkillID();

    FCoreUpgradeAttribute AttrDef;
    if (!SkillLibrary->GetSkillCoreUpgradeAttribute(SkillID, AttributeType, AttrDef)) return;

    const bool bSuccess = bIsRefund
        ? BoundPlayerState->RefundSkillCorePoint(SkillID, AttributeType, AttrDef.ValuePerPoint)
        : BoundPlayerState->SpendSkillCorePoint(SkillID, AttributeType, AttrDef.ValuePerPoint, AttrDef.MaxPoints);

    if (bSuccess)
    {
        ShowSkill(CurrentSkill.Get(), BoundPlayerState.Get());
    }
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

void UCC_SkillUpgradeDetailWidget::RebuildCoreAttributeRows(UCC_SkillBase* Skill, ACC_PlayerState* PlayerState, UCC_SkillLibrarySubsystem* SkillLibrary)
{
    if (!CoreAttributeContainer || !CoreAttributeRowWidgetClass || !Skill || !PlayerState || !SkillLibrary) return;

    // 재사용 컨테이너라 매번 비우고 다시 채움 — RebuildAttributeRows()와 동일 원칙
    CoreAttributeContainer->ClearChildren();

    const FName SkillID = Skill->GetSkillID();
    const TArray<FCoreUpgradeAttribute> CoreAttrs = SkillLibrary->GetSkillCoreUpgradeAttributes(SkillID);
    const int32 CoreBank = PlayerState->GetSkillCoreUnspentPoints(SkillID);

    for (const FCoreUpgradeAttribute& Attr : CoreAttrs)
    {
        UCC_CoreUpgradeRowWidget* Row = CreateWidget<UCC_CoreUpgradeRowWidget>(this, CoreAttributeRowWidgetClass);
        if (!Row) continue;

        const int32 CurrentSpent = PlayerState->GetSkillCoreAttributeSpentPoints(SkillID, Attr.AttributeType);
        Row->SetAttributeData(Attr, CurrentSpent, CoreBank);
        Row->OnPointRequested.AddDynamic(this, &UCC_SkillUpgradeDetailWidget::HandleCorePointRequested);

        CoreAttributeContainer->AddChild(Row);
    }
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

