// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_SkillInventoryWidget.h"
#include "CC_SkillRosterEntryWidget.h"
#include "Components/PanelWidget.h"
#include "../CC_PlayerState.h"
#include "../SkillSystem/CC_SkillBase.h"

void UCC_SkillInventoryWidget::NativeConstruct()
{
    Super::NativeConstruct();

    BoundPlayerState = GetOwningPlayerState<ACC_PlayerState>();

    if (BoundPlayerState)
    {
        BoundPlayerState->OnSkillsChanged.AddDynamic(this, &UCC_SkillInventoryWidget::HandleSkillsChanged);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[SkillInventory] Owning PlayerState is not ACC_PlayerState!"));
    }

    RefreshInventory();

}

void UCC_SkillInventoryWidget::NativeDestruct()
{
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(TransitionTimerHandle);
    }

    if (BoundPlayerState)
    {
        BoundPlayerState->OnSkillsChanged.RemoveDynamic(this, &UCC_SkillInventoryWidget::HandleSkillsChanged);
    }

    Super::NativeDestruct();
}

void UCC_SkillInventoryWidget::RefreshInventory()
{
    if (!RosterContainer || !RosterEntryWidgetClass || !BoundPlayerState) return;

    if (RosterEntries.Num() != NumEquipSlots)
    {
        // 최초 1회만 실제로 위젯을 만들고 붙임 — 이후로는 절대 파괴/재생성 안 함
        // (SkillsPanel의 CC_SkillSlotWidget과 동일하게 "영구 위젯 + SetSkillData로 내용만 갱신")
        if (GetWorld())
        {
            GetWorld()->GetTimerManager().ClearTimer(TransitionTimerHandle);
        }

        RosterContainer->ClearChildren();
        RosterEntries.Reset();
        ExpandedEntry = nullptr;

        for (int32 SlotIndex = 0; SlotIndex < NumEquipSlots; ++SlotIndex)
        {
            UCC_SkillRosterEntryWidget* Entry = CreateWidget<UCC_SkillRosterEntryWidget>(this, RosterEntryWidgetClass);
            if (!Entry) continue;

            Entry->OnExpandRequested.AddDynamic(this, &UCC_SkillInventoryWidget::HandleEntryExpandRequested);
            Entry->OnSlotDropRequested.AddDynamic(this, &UCC_SkillInventoryWidget::HandleSlotDropRequested);

            RosterContainer->AddChild(Entry);
            RosterEntries.Add(Entry);
        }
    }

    // 이후로는(스왑 등으로 다시 호출될 때마다) 기존 위젯 인스턴스에 데이터만 다시 채움
    for (int32 SlotIndex = 0; SlotIndex < RosterEntries.Num(); ++SlotIndex)
    {
        if (UCC_SkillRosterEntryWidget* Entry = RosterEntries[SlotIndex])
        {
            UCC_SkillBase* Skill = BoundPlayerState->GetSkillAtSlot(SlotIndex);
            Entry->SetSkillData(SlotIndex, Skill, BoundPlayerState);
        }
    }
}

void UCC_SkillInventoryWidget::HandleEntryExpandRequested(UCC_SkillRosterEntryWidget* Entry)
{
    if (!Entry || !GetWorld()) return;

    GetWorld()->GetTimerManager().ClearTimer(TransitionTimerHandle);

    if (ExpandedEntry == Entry)
    {
        Entry->SetExpanded(false);
        ExpandedEntry = nullptr;

        FTimerDelegate Del;
        Del.BindUObject(this, &UCC_SkillInventoryWidget::RevealSiblings);
        GetWorld()->GetTimerManager().SetTimer(TransitionTimerHandle, Del, Entry->GetDrawerAnimDuration(), false);
        return;
    }

    if (ExpandedEntry)
    {
        ExpandedEntry->SetExpanded(false);
    }

    for (UCC_SkillRosterEntryWidget* E : RosterEntries)
    {
        if (!E || E == Entry) continue;
        E->PlayFadeOut();
    }

    ExpandedEntry = Entry;

    TWeakObjectPtr<UCC_SkillRosterEntryWidget> WeakEntry(Entry);
    TWeakObjectPtr<UCC_SkillInventoryWidget> WeakSelf(this);

    FTimerDelegate Del;
    Del.BindLambda([WeakSelf, WeakEntry]()
        {
            if (WeakSelf.IsValid() && WeakEntry.IsValid() && WeakSelf->ExpandedEntry == WeakEntry.Get())
            {
                WeakEntry->SetExpanded(true);
            }
        });
    GetWorld()->GetTimerManager().SetTimer(TransitionTimerHandle, Del, Entry->GetDrawerAnimDuration(), false);

}

void UCC_SkillInventoryWidget::HandleSlotDropRequested(int32 SourceSlotIndex, int32 TargetSlotIndex)
{
    if (!BoundPlayerState) return;
    BoundPlayerState->SwapSlots(SourceSlotIndex, TargetSlotIndex);
}

void UCC_SkillInventoryWidget::RevealSiblings()
{
    for (UCC_SkillRosterEntryWidget* E : RosterEntries)
    {
        if (!E || E == ExpandedEntry) continue;
        E->PlayFadeIn();
    }
}

void UCC_SkillInventoryWidget::HandleSkillsChanged()
{
    RefreshInventory();
}

