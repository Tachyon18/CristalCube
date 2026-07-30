// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_SkillsPanelWidget.h"
#include "CC_SkillSlotWidget.h"
#include "../SkillSystem/CC_SkillLibrarySubsystem.h"
#include "../SkillSystem/CC_SkillBase.h"
#include "../CC_PlayerState.h"


void UCC_SkillsPanelWidget::NativeConstruct()
{
    Super::NativeConstruct();

    Slots = { Slot1, Slot2, Slot3, Slot4, Slot5, Slot6 };

    for (int32 i = 0; i < Slots.Num(); ++i)
    {
        if (Slots[i])
        {
            Slots[i]->SetSlotIndex(i);
            Slots[i]->OnSlotDropped.AddDynamic(this, &UCC_SkillsPanelWidget::HandleSlotDropped);
        }
    }

    BoundPlayerState = GetOwningPlayerState<ACC_PlayerState>();

    if (BoundPlayerState)
    {
        BoundPlayerState->OnSkillsChanged.AddDynamic(this, &UCC_SkillsPanelWidget::HandleSkillsChanged);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[SkillsPanel] Owning PlayerState is not ACC_PlayerState!"));
    }

    RefreshPanel();
}

void UCC_SkillsPanelWidget::NativeDestruct()
{
    if (BoundPlayerState)
    {
        BoundPlayerState->OnSkillsChanged.RemoveDynamic(this, &UCC_SkillsPanelWidget::HandleSkillsChanged);
    }

    Super::NativeDestruct();
}

void UCC_SkillsPanelWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (!BoundPlayerState) return;

    for (int32 i = 0; i < Slots.Num(); ++i)
    {
        if (!Slots[i]) continue;

        if (UCC_SkillBase* Skill = BoundPlayerState->GetSkillAtSlot(i))
        {
            Slots[i]->SetCooldownProgress(Skill->GetCooldownProgress());
        }
    }
}

void UCC_SkillsPanelWidget::RefreshPanel()
{
    if (!BoundPlayerState)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SkillsPanel] RefreshPanel: BoundPlayerState is null"));
        return;
    }

    UCC_SkillLibrarySubsystem* SkillLibrary =
        GetWorld() ? GetWorld()->GetGameInstance()->GetSubsystem<UCC_SkillLibrarySubsystem>() : nullptr;

    if (!SkillLibrary)
    {
        UE_LOG(LogTemp, Error, TEXT("[SkillsPanel] SkillLibrarySubsystem not found!"));
        return;
    }

    for (int32 i = 0; i < Slots.Num(); ++i)
    {
        if (!Slots[i])
        {
            UE_LOG(LogTemp, Error, TEXT("[SkillsPanel] RefreshPanel: Slots[%d] widget pointer is NULL"), i);
            continue;
        }

        UCC_SkillBase* Skill = BoundPlayerState->GetSkillAtSlot(i);

        if (!Skill)
        {
            UE_LOG(LogTemp, Log, TEXT("[SkillsPanel] Slot %d -> empty, ClearSlot()"), i);
            Slots[i]->ClearSlot();
            continue;
        }

        FSkillDisplayData Data;
        const bool bFound = SkillLibrary->GetSkillDisplayData(Skill->GetSkillID(), Data);

        UE_LOG(LogTemp, Log, TEXT("[SkillsPanel] Slot %d -> Skill=%s, LookupFound=%d, Icon=%s"),
            i, *Skill->GetSkillID().ToString(), bFound, Data.Icon ? *Data.Icon->GetName() : TEXT("NULL"));

        if (bFound)
        {
            Slots[i]->SetSkillData(Data);
        }
        else
        {
            Slots[i]->ClearSlot();
        }
    }
}

void UCC_SkillsPanelWidget::HandleSlotDropped(int32 SourceSlotIndex, int32 TargetSlotIndex)
{
    if (BoundPlayerState)
    {
        BoundPlayerState->SwapSlots(SourceSlotIndex, TargetSlotIndex);
    }
}

void UCC_SkillsPanelWidget::HandleSkillsChanged()
{
    RefreshPanel();
}
