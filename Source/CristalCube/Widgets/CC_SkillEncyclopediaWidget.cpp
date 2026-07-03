// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_SkillEncyclopediaWidget.h"
#include "../SkillSystem/CC_SkillCardData.h"
#include "../SkillSystem/CC_SkillLibrarySubsystem.h"
#include "Components/TileView.h"

void UCC_SkillEncyclopediaWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshSkillList();
}

void UCC_SkillEncyclopediaWidget::RefreshSkillList()
{
    if (!SkillTileView)
    {
        UE_LOG(LogTemp, Error, TEXT("[SkillEncyclopedia] SkillTileView not bound!"));
        return;
    }

    UCC_SkillLibrarySubsystem* SkillLibrary =
        GetWorld() ? GetWorld()->GetGameInstance()->GetSubsystem<UCC_SkillLibrarySubsystem>() : nullptr;

    if (!SkillLibrary)
    {
        UE_LOG(LogTemp, Error, TEXT("[SkillEncyclopedia] SkillLibrarySubsystem not found!"));
        return;
    }

    SkillTileView->ClearListItems();
    CardDataPool.Empty();

    TArray<FSkillDisplayData> AllSkills = SkillLibrary->GetAllSkillDisplayData();
    CardDataPool.Reserve(AllSkills.Num());

    for (const FSkillDisplayData& SkillData : AllSkills)
    {
        UCC_SkillCardData* CardData = NewObject<UCC_SkillCardData>(this);
        CardData->SkillData = SkillData;
        CardData->AddonBadges = SkillLibrary->GetAddonBadgesForSkill(SkillData);

        CardDataPool.Add(CardData);
        SkillTileView->AddItem(CardData);
    }
}
