// Fill out your copyright notice in the Description page of Project Settings.

#include "CC_UIGaugePip.h"
#include "CC_UIStyleData.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"

void UCC_UIGaugePip::NativePreConstruct()
{
	Super::NativePreConstruct();
	if (IsDesignTime() || !bValueSet)
	{
		Spent = FMath::Clamp(PreviewSpent, 0, PreviewMax);
		Max = PreviewMax;
	}
	BuiltCount = -1;                       // 디자이너에서 크기·간격 Override를 바꿨을 때도 다시 만듦
	Rebuild();
	RefreshColors();
}

void UCC_UIGaugePip::NativeConstruct()
{
	Super::NativeConstruct();
	Rebuild();
	RefreshColors();
}

void UCC_UIGaugePip::SetValue(int32 InSpent, int32 InMax)
{
	bValueSet = true;
	Max = FMath::Max(InMax, 0);
	Spent = FMath::Clamp(InSpent, 0, Max);
	Rebuild();                             // Max가 같으면 아무 일 없음
	RefreshColors();
}

// ─────────────────────────────────────────────────────────── 내부

void UCC_UIGaugePip::Rebuild()
{
	if (!PipBox || !WidgetTree || BuiltCount == Max)
	{
		return;
	}
	PipBox->ClearChildren();
	Pips.Reset();

	const FVector2D Size = GetPipSize();
	const float Gap = GetGap();
	for (int32 i = 0; i < Max; ++i)
	{
		UImage* Img = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		Img->SetVisibility(ESlateVisibility::HitTestInvisible);
		Img->SetDesiredSizeOverride(Size);
		if (UHorizontalBoxSlot* HSlot = PipBox->AddChildToHorizontalBox(Img))
		{
			HSlot->SetPadding(FMargin(0.f, 0.f, (i < Max - 1) ? Gap : 0.f, 0.f));
			HSlot->SetVerticalAlignment(VAlign_Center);
			HSlot->SetHorizontalAlignment(HAlign_Left);
			HSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}
		Pips.Add(Img);
	}
	BuiltCount = Max;
}

void UCC_UIGaugePip::RefreshColors()
{
	const UCC_UIStyleData* Style = UCC_UIStyleData::GetUIStyle();
	const FVector2D Size = GetPipSize();

	// Token 없을 때 대비 기본값 (§1-1 값)
	static const TArray<FColor> DefaultFill = { FColor(0xF2, 0xC4, 0xB0), FColor(0xE3, 0xA3, 0xB8), FColor(0xB9, 0xA8, 0xE6) };
	const TArray<FColor>& FillStops = (Style && Style->PipFill.Num() > 0) ? Style->PipFill : DefaultFill;
	const FLinearColor NextColor = Style ? FLinearColor(Style->PipNext) : FLinearColor(FColor(255, 255, 255, 46));
	const FLinearColor EmptyColor = Style ? FLinearColor(Style->PipEmpty) : FLinearColor(FColor(255, 255, 255, 18));

	for (int32 i = 0; i < Pips.Num(); ++i)
	{
		UImage* Img = Pips[i];
		if (!Img)
		{
			continue;
		}
		const bool bFilled = i < Spent;
		const bool bNext = bShowNext && (i == Spent) && (Spent < Max);

		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.ImageSize = Size;
		Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;   // 6px → 반경 3
		Brush.OutlineSettings.Width = 0.f;

		FLinearColor Tint;
		if (bFilled)
		{
			if (FillTexture)
			{
				Brush.SetResourceObject(FillTexture);
				Tint = FLinearColor::White;
			}
			else
			{
				const float T = (Max > 1) ? (float)i / (float)(Max - 1) : 0.f;
				Tint = UCC_UIStyleData::SampleGradient(FillStops, T);
			}
		}
		else
		{
			Tint = bNext ? NextColor : EmptyColor;
		}
		Brush.TintColor = FSlateColor(Tint);
		Img->SetBrush(Brush);
	}
}

FVector2D UCC_UIGaugePip::GetPipSize() const
{
	const UCC_UIStyleData* Style = UCC_UIStyleData::GetUIStyle();
	const FVector2D Token = Style ? Style->Sizes.Pip : FVector2D(13.f, 6.f);
	return FVector2D(PipSizeOverride.X > 0.f ? PipSizeOverride.X : Token.X,
		PipSizeOverride.Y > 0.f ? PipSizeOverride.Y : Token.Y);
}

float UCC_UIGaugePip::GetGap() const
{
	if (GapOverride >= 0.f)
	{
		return GapOverride;
	}
	const UCC_UIStyleData* Style = UCC_UIStyleData::GetUIStyle();
	return Style ? Style->Sizes.PipGap : 4.f;
}