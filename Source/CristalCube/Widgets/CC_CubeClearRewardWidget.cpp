// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_CubeClearRewardWidget.h"
#include "CC_IntroGlassWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

void UCC_CubeClearRewardWidget::NativeConstruct()
{
	Super::NativeConstruct();

    // LevelUpWidget과 동일한 카드별 테마 고정
    if (CardPanel1) CardPanel1->SetTheme(EGlassTheme::Ocean);
    if (CardPanel2) CardPanel2->SetTheme(EGlassTheme::Galaxy);
    if (CardPanel3) CardPanel3->SetTheme(EGlassTheme::Fire);

    if (Choice1Button) Choice1Button->OnClicked.AddDynamic(this, &UCC_CubeClearRewardWidget::OnChoice1Clicked);
    if (Choice2Button) Choice2Button->OnClicked.AddDynamic(this, &UCC_CubeClearRewardWidget::OnChoice2Clicked);
    if (Choice3Button) Choice3Button->OnClicked.AddDynamic(this, &UCC_CubeClearRewardWidget::OnChoice3Clicked);

}

void UCC_CubeClearRewardWidget::SetRewardChoices(const TArray<FCubeClearReward>& InRewards)
{
    CurrentChoices = InRewards;

    ApplyCardVisual(0, Choice1Name, Choice1Description, Choice1Icon);
    ApplyCardVisual(1, Choice2Name, Choice2Description, Choice2Icon);
    ApplyCardVisual(2, Choice3Name, Choice3Description, Choice3Icon);
}

void UCC_CubeClearRewardWidget::OnChoice1Clicked() { SelectReward(0); }
void UCC_CubeClearRewardWidget::OnChoice2Clicked() { SelectReward(1); }
void UCC_CubeClearRewardWidget::OnChoice3Clicked() { SelectReward(2); }


void UCC_CubeClearRewardWidget::SelectReward(int32 ChoiceIndex)
{
    if (CurrentChoices.IsValidIndex(ChoiceIndex))
    {
        OnCubeClearRewardSelected.Broadcast(CurrentChoices[ChoiceIndex]);
    }
}

void UCC_CubeClearRewardWidget::ApplyCardVisual(int32 Index, UTextBlock* NameText, UTextBlock* DescText, UImage* IconImage)
{
    if (!CurrentChoices.IsValidIndex(Index)) return;

    const FCubeClearReward& Reward = CurrentChoices[Index];

    if (NameText) NameText->SetText(Reward.DisplayName);
    if (DescText) DescText->SetText(Reward.Description);
    if (IconImage && Reward.Icon) IconImage->SetBrushFromTexture(Reward.Icon);
}
