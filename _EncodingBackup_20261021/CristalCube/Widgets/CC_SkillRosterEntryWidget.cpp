// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_SkillRosterEntryWidget.h"
#include "CC_SkillUpgradeDetailWidget.h"
#include "CC_AddonBadgeWidget.h"
#include "CC_SkillSlotDragOp.h"
#include "Framework/Application/SlateApplication.h"
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

    //UE_LOG(LogTemp, Warning, TEXT("[RosterDrag] NativeConstruct — new widget instance created (this=%p)"), this);

    //if (CardHeaderButton && !CardHeaderButton->OnClicked.IsAlreadyBound(this, &UCC_SkillRosterEntryWidget::HandleCardHeaderClicked))
    //{
    //    CardHeaderButton->OnClicked.AddDynamic(this, &UCC_SkillRosterEntryWidget::HandleCardHeaderClicked);
    //}

    //if (CardHeaderButton)   CardHeaderButton->SetVisibility(ESlateVisibility::HitTestInvisible);
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

    //if (CardHeaderButton)
    //{
    //    const bool bWantPassThrough = BoundSkill != nullptr && FSlateApplication::Get().GetModifierKeys().IsControlDown();
    //    CardHeaderButton->SetVisibility(bWantPassThrough ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Visible);
    //}

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

FReply UCC_SkillRosterEntryWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && InMouseEvent.IsControlDown() && BoundSkill != nullptr)
    {
        return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
    }

    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && BoundSkill != nullptr)
    {
        bPendingClick = true;
        return FReply::Handled().CaptureMouse(TakeWidget());
    }

    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UCC_SkillRosterEntryWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bPendingClick)
    {
        bPendingClick = false;
        
        // 눌렀던 위치에서 그대로 뗐을 때만 클릭으로 인정 (버튼 클릭의 hover-cancel과 동일한 감각)
        if (InGeometry.IsUnderLocation(InMouseEvent.GetScreenSpacePosition()))
        {
            HandleCardHeaderClicked();
        }
        
        return FReply::Handled().ReleaseMouseCapture();
    }

    return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UCC_SkillRosterEntryWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
    Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);   // BP 쪽에 딸린 부가 로직(있다면)만 실행되게 함

    UCC_SkillSlotDragOp* DragOp = NewObject<UCC_SkillSlotDragOp>(this);
    DragOp->SourceSlotIndex = SlotIndex;

    if (SkillIcon)
    {
        UImage* GhostImage = NewObject<UImage>(this);
        GhostImage->SetBrush(SkillIcon->GetBrush());
        GhostImage->SetRenderOpacity(0.75f);
        GhostImage->SetDesiredSizeOverride(InGeometry.GetLocalSize());
        DragOp->DefaultDragVisual = GhostImage;
    }
    DragOp->Pivot = EDragPivot::MouseDown;
    OutOperation = DragOp;
}

bool UCC_SkillRosterEntryWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    const bool bSuperHandled = Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);

    if (UCC_SkillSlotDragOp* DragOp = Cast<UCC_SkillSlotDragOp>(InOperation))
    {
        OnSlotDropRequested.Broadcast(DragOp->SourceSlotIndex, SlotIndex);
        return true;
    }
    return bSuperHandled;
}

void UCC_SkillRosterEntryWidget::SetSkillData(int32 InSlotIndex, UCC_SkillBase* Skill, ACC_PlayerState* PlayerState)
{
    SlotIndex = InSlotIndex;
    BoundSkill = Skill;
    BoundPlayerState = PlayerState;

    if (!Skill || !PlayerState)
    {
        // 빈 슬롯 — 확장 불가, 드롭 타겟 전용 플레이스홀더로 표시
        if (SkillIcon) SkillIcon->SetVisibility(ESlateVisibility::Hidden);
        if (SkillName) SkillName->SetText(FText::GetEmpty());
        if (ExpandChevron) ExpandChevron->SetVisibility(ESlateVisibility::Hidden);
        if (AddonIconStrip) AddonIconStrip->ClearChildren();
        if (EmptySlotOverlay) EmptySlotOverlay->SetVisibility(ESlateVisibility::Visible);

        return;
    }

    if (EmptySlotOverlay) EmptySlotOverlay->SetVisibility(ESlateVisibility::Collapsed);
    if (SkillIcon) SkillIcon->SetVisibility(ESlateVisibility::Visible);
    if (ExpandChevron) ExpandChevron->SetVisibility(ESlateVisibility::Visible);

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
		BadgeWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
        AddonIconStrip->AddChild(BadgeWidget);
    }
}
