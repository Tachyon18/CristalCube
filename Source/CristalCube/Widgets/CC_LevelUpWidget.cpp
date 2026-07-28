// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_LevelUpWidget.h"
#include "CC_GlassWidget.h"
#include "CC_IntroGlassWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

void UCC_LevelUpWidget::NativeConstruct()
{
	Super::NativeConstruct();

    // 카드별 테마 — 희귀도 고정
    if (CardPanel1) CardPanel1->SetTheme(EGlassTheme::Ocean);   // Common
    if (CardPanel2) CardPanel2->SetTheme(EGlassTheme::Galaxy);  // Rare
    if (CardPanel3) CardPanel3->SetTheme(EGlassTheme::Fire);    // Epic

    if (Choice1Button) Choice1Button->OnClicked.AddDynamic(this, &UCC_LevelUpWidget::OnChoice1Clicked);
    if (Choice2Button) Choice2Button->OnClicked.AddDynamic(this, &UCC_LevelUpWidget::OnChoice2Clicked);
    if (Choice3Button) Choice3Button->OnClicked.AddDynamic(this, &UCC_LevelUpWidget::OnChoice3Clicked);
}

void UCC_LevelUpWidget::SetLevelUpChoices(const TArray<FLevelUpCandidate>& InCandidates)
{
    Candidates = InCandidates;

    if (Candidates.IsValidIndex(0) && Choice1Name && Choice1Description)
    {
        Choice1Name->SetText(Candidates[0].DisplayName);
        Choice1Description->SetText(Candidates[0].Description);
        if (Choice1Icon && Candidates[0].Icon)
        {
            Choice1Icon->SetBrushFromTexture(Candidates[0].Icon);
        }
    }

    if (Candidates.IsValidIndex(1) && Choice2Name && Choice2Description)
    {
        Choice2Name->SetText(Candidates[1].DisplayName);
        Choice2Description->SetText(Candidates[1].Description);
        if (Choice2Icon && Candidates[1].Icon)
        {
            Choice2Icon->SetBrushFromTexture(Candidates[1].Icon);
        }
    }

    if (Candidates.IsValidIndex(2) && Choice3Name && Choice3Description)
    {
        Choice3Name->SetText(Candidates[2].DisplayName);
        Choice3Description->SetText(Candidates[2].Description);
        if (Choice3Icon && Candidates[2].Icon)
        {
            Choice3Icon->SetBrushFromTexture(Candidates[2].Icon);
        }
    }
}


void UCC_LevelUpWidget::OnChoice1Clicked()
{
    SelectCandidate(0);
}

void UCC_LevelUpWidget::OnChoice2Clicked()
{
    SelectCandidate(1);
}

void UCC_LevelUpWidget::OnChoice3Clicked()
{
    SelectCandidate(2);
}

void UCC_LevelUpWidget::SelectCandidate(int32 ChoiceIndex)
{
    if (Candidates.IsValidIndex(ChoiceIndex))
    {
        const FLevelUpCandidate& Selected = Candidates[ChoiceIndex];

        UE_LOG(LogTemp, Log, TEXT("[LevelUpUI] Selected: %s"), *Selected.DisplayName.ToString());

        OnLevelUpCandidateSelected.Broadcast(Selected);
    }
}

