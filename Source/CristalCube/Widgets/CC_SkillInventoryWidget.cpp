// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_SkillInventoryWidget.h"
#include "CC_SkillSlotWidget.h"
#include "../SkillSystem/CC_SkillLibrarySubsystem.h"
#include "../CC_PlayerState.h"
#include "../SkillSystem/CC_SkillBase.h"

void UCC_SkillInventoryWidget::NativeConstruct()
{
    Super::NativeConstruct();

    Slots = { Slot1, Slot2, Slot3, Slot4, Slot5, Slot6 };

    for (int32 i = 0; i < Slots.Num(); ++i)
    {
        if (Slots[i])
        {
            Slots[i]->SetSlotIndex(i);
            Slots[i]->OnSlotDropped.AddDynamic(this, &UCC_SkillInventoryWidget::HandleSlotDropped);
        }
    }

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
    if (BoundPlayerState)
    {
        BoundPlayerState->OnSkillsChanged.RemoveDynamic(this, &UCC_SkillInventoryWidget::HandleSkillsChanged);
    }

    Super::NativeDestruct();
}

void UCC_SkillInventoryWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (!BoundPlayerState) return;

    for (int32 i = 0; i < Slots.Num(); ++i)
    {
        if (UCC_SkillBase* Skill = BoundPlayerState->GetSkillAtSlot(i))
        {
            Slots[i]->SetCooldownProgress(Skill->GetCooldownProgress());
        }
    }
}

void UCC_SkillInventoryWidget::RefreshInventory()
{
    if (!BoundPlayerState)
    {
        return;
    }

    UCC_SkillLibrarySubsystem* SkillLibrary =
        GetWorld() ? GetWorld()->GetGameInstance()->GetSubsystem<UCC_SkillLibrarySubsystem>() : nullptr;

    if (!SkillLibrary)
    {
        UE_LOG(LogTemp, Error, TEXT("[SkillInventory] SkillLibrarySubsystem not found!"));
        return;
    }

    const TArray<UCC_SkillBase*>& EquippedSkills = BoundPlayerState->GetAllSkills();

    for (int32 i = 0; i < Slots.Num(); ++i)
    {
        if (!Slots[i])
        {
            continue;
        }

        UCC_SkillBase* Skill = BoundPlayerState->GetSkillAtSlot(i);

        FSkillDisplayData Data;
        if (Skill && SkillLibrary->GetSkillDisplayData(Skill->GetSkillID(), Data))
        {
            Slots[i]->SetSkillData(Data);
        }
        else
        {
            Slots[i]->ClearSlot();
        }
    }
}

void UCC_SkillInventoryWidget::HandleSlotDropped(int32 SourceSlotIndex, int32 TargetSlotIndex)
{
    if (BoundPlayerState)
    {
        BoundPlayerState->SwapSlots(SourceSlotIndex, TargetSlotIndex);
        // RefreshInventory()는 SwapSlots 내부의 OnSkillsChanged.Broadcast()가 자동 트리거함 — 수동 호출 불필요
    }
}

void UCC_SkillInventoryWidget::HandleSkillsChanged()
{
    RefreshInventory();
}
