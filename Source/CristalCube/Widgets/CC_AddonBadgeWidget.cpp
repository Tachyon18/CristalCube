// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_AddonBadgeWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"


void UCC_AddonBadgeWidget::SetAddonData(const FAddonTableRow& InAddonData)
{
    if (AddonIcon && InAddonData.Icon)
    {
        AddonIcon->SetBrushFromTexture(InAddonData.Icon);
    }

    if (AddonName)
    {
        AddonName->SetText(InAddonData.DisplayName);
    }
}
