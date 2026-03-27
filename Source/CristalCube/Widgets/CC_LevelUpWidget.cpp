// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_LevelUpWidget.h"
#include "CC_GlassWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "../CC_WeaponManagerSubsystem.h"

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

void UCC_LevelUpWidget::SetWeaponChoices(const TArray<FName>& InWeaponNames)
{
    WeaponChoices = InWeaponNames;

    UCC_WeaponManagerSubsystem* WeaponManager = GetWorld()->GetGameInstance()->GetSubsystem<UCC_WeaponManagerSubsystem>();
    if (!WeaponManager)
    {
        UE_LOG(LogTemp, Error, TEXT("[LevelUpUI] WeaponManager not found!"));
        return;
    }

    if (WeaponChoices.IsValidIndex(0))
    {
        FWeaponData* WeaponData = WeaponManager->GetWeaponDataPtr(WeaponChoices[0]);
        if (WeaponData && Choice1Name && Choice1Description)
        {
            Choice1Name->SetText(WeaponData->WeaponName);
            Choice1Description->SetText(WeaponData->Description);
        }
    }

    if (WeaponChoices.IsValidIndex(1))
    {
        FWeaponData* WeaponData = WeaponManager->GetWeaponDataPtr(WeaponChoices[1]);
        if (WeaponData && Choice2Name && Choice2Description)
        {
            Choice2Name->SetText(WeaponData->WeaponName);
            Choice2Description->SetText(WeaponData->Description);
        }
    }

    if (WeaponChoices.IsValidIndex(2))
    {
        FWeaponData* WeaponData = WeaponManager->GetWeaponDataPtr(WeaponChoices[2]);
        if (WeaponData && Choice3Name && Choice3Description)
        {
            Choice3Name->SetText(WeaponData->WeaponName);
            Choice3Description->SetText(WeaponData->Description);
        }
    }
}

void UCC_LevelUpWidget::OnChoice1Clicked()
{
    SelectWeapon(0);
}

void UCC_LevelUpWidget::OnChoice2Clicked()
{
    SelectWeapon(1);
}

void UCC_LevelUpWidget::OnChoice3Clicked()
{
    SelectWeapon(2);
}

void UCC_LevelUpWidget::SelectWeapon(int32 ChoiceIndex)
{
    if (WeaponChoices.IsValidIndex(ChoiceIndex))
    {
        FName SelectedWeapon = WeaponChoices[ChoiceIndex];

        UE_LOG(LogTemp, Log, TEXT("[LevelUpUI] Selected: %s"), *SelectedWeapon.ToString());

        OnWeaponSelected.Broadcast(SelectedWeapon);
    }
}
