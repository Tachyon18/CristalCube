// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_SkillInventoryWidget.h"
#include "CC_SkillInventorySlotWidget.h"
#include "../SkillSystem/CC_SkillLibrarySubsystem.h"
#include "../CC_PlayerState.h"
#include "../SkillSystem/CC_SkillBase.h"

void UCC_SkillInventoryWidget::NativeConstruct()
{
    Super::NativeConstruct();

    Slots = { Slot1, Slot2, Slot3, Slot4, Slot5, Slot6 };

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

void UCC_SkillInventoryWidget::HandleSkillsChanged()
{
    RefreshInventory();
}
