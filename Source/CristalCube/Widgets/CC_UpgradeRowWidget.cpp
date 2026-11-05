// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_UpgradeRowWidget.h"
#include "CC_UIStyleData.h"
#include "CC_UIIconButton.h"
#include "CC_UIGaugePip.h"
#include "Components/TextBlock.h"

// ─────────────────────────────────────────────────────────── 생명주기

void UCC_UpgradeRowWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();          // 런타임 1회 (CreateWidget 시점 — SetAttributeData보다 먼저)

	if (PlusButton)
	{
		PlusButton->OnControlClicked.AddUniqueDynamic(this, &UCC_UpgradeRowWidget::HandlePlusClicked);
	}
	if (MinusButton)
	{
		MinusButton->OnControlClicked.AddUniqueDynamic(this, &UCC_UpgradeRowWidget::HandleMinusClicked);
	}
}

void UCC_UpgradeRowWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	ApplyStyles();

	// 런타임에서는 SetAttributeData가 AddChild보다 먼저 불리므로 미리보기로 덮지 않음
	if (IsDesignTime() || !bRowDataSet)
	{
		ApplyRowVisual(PreviewName, PreviewEffect, PreviewSpent, PreviewMax, PreviewBank);
	}
}

// ─────────────────────────────────────────────────────────── 공개 / 파생용

void UCC_UpgradeRowWidget::SetDividerVisible(bool bVisible)
{
	if (RowDivider)
	{
		RowDivider->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UCC_UpgradeRowWidget::ApplyRow(const FText& InName, const FText& InEffect, int32 Spent, int32 Max, int32 Bank)
{
	bRowDataSet = true;
	ApplyRowVisual(InName, InEffect, Spent, Max, Bank);
}

FText UCC_UpgradeRowWidget::FormatPercent(float Value)
{
	FNumberFormattingOptions Opt;
	Opt.SetUseGrouping(false).SetMinimumFractionalDigits(0).SetMaximumFractionalDigits(1);

	const float Pct = Value * 100.f;
	const bool  bNeg = Pct < -0.05f;
	return FText::Format(NSLOCTEXT("CC_UI", "UpgradeRowPercent", "{0}{1}%"),
		FText::FromString(bNeg ? TEXT("−") : TEXT("+")),        // 시안 '−' (U+2212)
		FText::AsNumber(FMath::Abs(Pct), &Opt));
}

FText UCC_UpgradeRowWidget::FormatSigned(float Value)
{
	FNumberFormattingOptions Opt;
	Opt.SetUseGrouping(false).SetMinimumFractionalDigits(0).SetMaximumFractionalDigits(2);

	const bool bNeg = Value < -0.005f;
	return FText::Format(NSLOCTEXT("CC_UI", "UpgradeRowSigned", "{0}{1}"),
		FText::FromString(bNeg ? TEXT("−") : TEXT("+")),
		FText::AsNumber(FMath::Abs(Value), &Opt));
}

// ─────────────────────────────────────────────────────────── 입력

void UCC_UpgradeRowWidget::HandlePlusClicked(UCC_UIControl* Control)
{
	RequestPoint(false);
}

void UCC_UpgradeRowWidget::HandleMinusClicked(UCC_UIControl* Control)
{
	RequestPoint(true);
}

// ─────────────────────────────────────────────────────────── 내부

void UCC_UpgradeRowWidget::ApplyStyles()
{
	if (AttributeName)
	{
		UCC_UIStyleData::ApplyTextStyle(AttributeName, EUITextStyle::Body, true);
		AttributeName->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (EffectText)
	{
		UCC_UIStyleData::ApplyTextStyle(EffectText, EUITextStyle::Effect, true);
	}
	if (PointsText)
	{
		UCC_UIStyleData::ApplyTextStyle(PointsText, EUITextStyle::Numeric, true);
		PointsText->SetJustification(ETextJustify::Right);
		PointsText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UCC_UpgradeRowWidget::ApplyRowVisual(const FText& InName, const FText& InEffect, int32 Spent, int32 Max, int32 Bank)
{
	if (AttributeName)
	{
		AttributeName->SetText(InName);
	}
	if (EffectText)
	{
		EffectText->SetText(InEffect);
		EffectText->SetVisibility(InEffect.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	if (PointsText)
	{
		PointsText->SetText(Max > 0
			? FText::Format(NSLOCTEXT("CC_UI", "UpgradeRowPoints", "{0} / {1}"), FText::AsNumber(Spent), FText::AsNumber(Max))
			: FText::AsNumber(Spent));
	}
	if (PipGauge)
	{
		PipGauge->SetVisibility(Max > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (Max > 0)
		{
			PipGauge->SetValue(Spent, Max);
		}
	}

	const bool bMaxed = (Max > 0) && (Spent >= Max);
	if (PlusButton)
	{
		PlusButton->SetControlEnabled(Bank > 0 && !bMaxed);
	}
	if (MinusButton)
	{
		MinusButton->SetControlEnabled(Spent > 0);
	}
}

