// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_UIIconButton.h"
#include "CC_UIStyleData.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"

void UCC_UIIconButton::NativePreConstruct()
{
	ApplyLayout();                         // Surface 설정·재초기화를 먼저 → 부모가 그 위에 상태를 지시
	ApplyGlyph();
	Super::NativePreConstruct();           // UCC_UIControl: 버튼 투명화 + 상태 적용 (→ NativeOnVisualStateChanged)
}

void UCC_UIIconButton::SetGlyph(EUIIconGlyph InGlyph)
{
	Glyph = InGlyph;
	ApplyGlyph();
}

void UCC_UIIconButton::NativeOnVisualStateChanged(EUISurfaceState NewState, bool bInstant)
{
	// 강조(+): Rest → Hover 모양, Hover → Selected 모양. Pressed·Disabled·Selected는 그대로
	if (bAccent && ControlSurface)
	{
		if (NewState == EUISurfaceState::Rest)
		{
			ControlSurface->SetSurfaceState(EUISurfaceState::Hover, bInstant);
		}
		else if (NewState == EUISurfaceState::Hover)
		{
			ControlSurface->SetSurfaceState(EUISurfaceState::Selected, bInstant);
		}
	}
	ApplyGlyphColor(NewState);
}

// ─────────────────────────────────────────────────────────── 내부

void UCC_UIIconButton::ApplyLayout()
{
	const UCC_UIStyleData* Style = UCC_UIStyleData::GetUIStyle();
	const float VisualPx = (ButtonSize == EUIIconButtonSize::Compact)
		? (Style ? Style->Sizes.IconButtonCompact : 28.f)
		: (Style ? Style->Sizes.IconButton : 32.f);
	const float HitPx = bExpandHitArea ? FMath::Max(VisualPx, Style ? Style->Sizes.HitMin : 44.f) : VisualPx;

	if (VisualSizeBox)
	{
		VisualSizeBox->SetWidthOverride(VisualPx);
		VisualSizeBox->SetHeightOverride(VisualPx);
	}
	if (HitSizeBox)
	{
		HitSizeBox->SetWidthOverride(HitPx);
		HitSizeBox->SetHeightOverride(HitPx);
	}
	if (ControlSurface)
	{
		ControlSurface->SurfaceRole = EUISurfaceRole::Card;
		ControlSurface->RadiusToken = EUIRadius::IconButton;
		ControlSurface->Element = ESkillElementType::None;
		ControlSurface->bAutoOuterPad = false;                  // 목록(행) 안 — Head 미사용, 클립 안전
		ControlSurface->RefreshFromStyle();
	}
}

void UCC_UIIconButton::ApplyGlyph()
{
	if (!GlyphText)
	{
		return;
	}
	FText Text;
	switch (Glyph)
	{
	case EUIIconGlyph::Plus:  Text = FText::FromString(TEXT("+")); break;
	case EUIIconGlyph::Minus: Text = FText::FromString(FString(1, TEXT("−"))); break;   // − (U+2212, + 와 같은 폭)
	case EUIIconGlyph::Close: Text = FText::FromString(FString(1, TEXT("×"))); break;   // ×
	default:                  Text = CustomGlyph; break;
	}
	GlyphText->SetText(Text);

	// 글꼴 = Oxanium Regular (Token Effect 스타일의 패밀리) · 크기만 글리프용
	if (const UCC_UIStyleData* Style = UCC_UIStyleData::GetUIStyle())
	{
		FSlateFontInfo Font = Style->MakeFontInfo(EUITextStyle::Effect);
		const float Px = (ButtonSize == EUIIconButtonSize::Compact) ? CompactGlyphSizePx : GlyphSizePx;
		Font.Size = Px * Style->PxToSlatePt;
		Font.LetterSpacing = 0;
		GlyphText->SetFont(Font);
	}
	GlyphText->SetJustification(ETextJustify::Center);
	GlyphText->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UCC_UIIconButton::ApplyGlyphColor(EUISurfaceState State)
{
	if (!GlyphText)
	{
		return;
	}
	const UCC_UIStyleData* Style = UCC_UIStyleData::GetUIStyle();
	const FColor C = (State == EUISurfaceState::Disabled)
		? (Style ? Style->TextDisabled : FColor(0x8A, 0x8F, 0xA8))
		: (Style ? Style->TextPrimary : FColor(0xEE, 0xF0, 0xFF));
	GlyphText->SetColorAndOpacity(FSlateColor(FLinearColor(C)));
}