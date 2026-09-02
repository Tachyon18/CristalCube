// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_SkillRosterEntryWidget.h"
#include "CC_SkillUpgradeDetailWidget.h"
#include "CC_AddonBadgeWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/PanelWidget.h"
#include "Components/SizeBox.h"
#include "../SkillSystem/CC_SkillBase.h"
#include "../SkillSystem/CC_SkillLibrarySubsystem.h"
#include "../CC_PlayerState.h"


void UCC_SkillRosterEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

    if (CardHeaderButton && !CardHeaderButton->OnClicked.IsAlreadyBound(this, &UCC_SkillRosterEntryWidget::HandleCardHeaderClicked))
    {
        CardHeaderButton->OnClicked.AddDynamic(this, &UCC_SkillRosterEntryWidget::HandleCardHeaderClicked);
    }

	bIsExpanded = false;
    bRevealAnimating = false;
    bSelfFadeAnimating = false;

    if (DrawerPanel)
    {
        DrawerPanel->SetVisibility(ESlateVisibility::Collapsed);
        DrawerPanel->SetRenderOpacity(1.0f);
    }
    if (ExpandChevron)
    {
        ExpandChevron->SetRenderTransformAngle(0.0f);
    }
	SetRenderOpacity(1.0f);
}

void UCC_SkillRosterEntryWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (bRevealAnimating && DrawerPanel)
    {
        RevealElapsed += InDeltaTime;
        const float Alpha = FMath::Clamp(DrawerAnimDuration > 0.0f ? RevealElapsed / DrawerAnimDuration : 1.0f, 0.0f, 1.0f);
        const float Eased = FMath::InterpEaseOut(0.0f, 1.0f, Alpha, 2.0f);

        DrawerPanel->SetRenderOpacity(bRevealingIn ? Eased : (1.0f - Eased));

        if (Alpha >= 1.0f)
        {
            bRevealAnimating = false;

            if (!bRevealingIn)
            {
                DrawerPanel->SetVisibility(ESlateVisibility::Collapsed);
            }
        }
    }

    if (bSelfFadeAnimating)
    {
        SelfFadeElapsed += InDeltaTime;
        const float Alpha = FMath::Clamp(DrawerAnimDuration > 0.0f ? SelfFadeElapsed / DrawerAnimDuration : 1.0f, 0.0f, 1.0f);
        const float Eased = FMath::InterpEaseOut(0.0f, 1.0f, Alpha, 2.0f);

        SetRenderOpacity(bSelfFadingIn ? Eased : (1.0f - Eased));

        if (Alpha >= 1.0f)
        {
            bSelfFadeAnimating = false;

            if (!bSelfFadingIn)
            {
                SetVisibility(ESlateVisibility::Collapsed);
            }
        }
    }
}

void UCC_SkillRosterEntryWidget::SetSkillData(int32 InSlotIndex, UCC_SkillBase* Skill, ACC_PlayerState* PlayerState)
{
    SlotIndex = InSlotIndex;
    BoundSkill = Skill;
    BoundPlayerState = PlayerState;

    if (!Skill || !PlayerState) return; // 빈 슬롯 — 현재는 Inventory 쪽에서 아예 리스트에 안 넣음

    UCC_SkillLibrarySubsystem* SkillLibrary =
        GetWorld() ? GetWorld()->GetGameInstance()->GetSubsystem<UCC_SkillLibrarySubsystem>() : nullptr;
    if (!SkillLibrary) return;

    FSkillDisplayData SkillData;
    if (SkillLibrary->GetSkillDisplayData(Skill->GetSkillID(), SkillData))
    {
        if (SkillIcon) SkillIcon->SetBrushFromTexture(SkillData.Icon);
        if (SkillName) SkillName->SetText(SkillData.DisplayName);
        RefreshAddonIconStrip(SkillLibrary->GetAddonBadgesForSkill(SkillData));
    }

    if (bIsExpanded && DrawerPanel)
    {
        DrawerPanel->ShowSkill(Skill, PlayerState);
    }
}

void UCC_SkillRosterEntryWidget::SetExpanded(bool bNewExpanded)
{
    if (bIsExpanded == bNewExpanded) return;
    bIsExpanded = bNewExpanded;

    if (ExpandChevron)
    {
        ExpandChevron->SetRenderTransformAngle(bIsExpanded ? 180.0f : 0.0f);
    }

    if (!DrawerPanel) return;

    if (bIsExpanded)
    {
        if (BoundSkill && BoundPlayerState)
        {
            DrawerPanel->ShowSkill(BoundSkill, BoundPlayerState);
        }

        DrawerPanel->SetVisibility(ESlateVisibility::Visible);
        DrawerPanel->SetRenderOpacity(0.0f);
        bRevealAnimating = true;
        bRevealingIn = true;
        RevealElapsed = 0.0f;
    }
    else
    {
        bRevealAnimating = true;
        bRevealingIn = false;
        RevealElapsed = 0.0f;
    }
}

void UCC_SkillRosterEntryWidget::PlayFadeOut()
{
    bSelfFadeAnimating = true;
    bSelfFadingIn = false;
    SelfFadeElapsed = 0.0f;
}

void UCC_SkillRosterEntryWidget::PlayFadeIn()
{
    SetVisibility(ESlateVisibility::Visible);
    SetRenderOpacity(0.0f);

    bSelfFadeAnimating = true;
    bSelfFadingIn = true;
    SelfFadeElapsed = 0.0f;
}

void UCC_SkillRosterEntryWidget::HandleCardHeaderClicked()
{
    if (!BoundSkill) return;
    OnExpandRequested.Broadcast(this);
}

void UCC_SkillRosterEntryWidget::RefreshAddonIconStrip(const TArray<FAddonTableRow>& AddonBadges)
{
    if (!AddonIconStrip) return;

    AddonIconStrip->ClearChildren();
    if (!AddonIconWidgetClass) return;

    for (const FAddonTableRow& Addon : AddonBadges)
    {
        UCC_AddonBadgeWidget* BadgeWidget = CreateWidget<UCC_AddonBadgeWidget>(this, AddonIconWidgetClass);
        if (!BadgeWidget) continue;

        BadgeWidget->SetAddonData(Addon);
        AddonIconStrip->AddChild(BadgeWidget);
    }
}
