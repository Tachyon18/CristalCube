// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_UIButton.h"
#include "CC_UIStyleData.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"

// CC_UIButton.cpp — L2 Primitive: Button (Ghost · Text · Filled)
// rev.7 변경: Filled = Surface Role `Filled`(MI_Surface_Filled — UseFilled ✔ · FilledMix 1) · 임시 모양(Rest→Hover 매핑) 제거.
//            Ghost/Text 동작은 그대로.

void UCC_UIButton::NativePreConstruct()
{
	ApplyLayout();                         // Surface 설정 먼저 → 부모가 상태 지시 (→ NativeOnVisualStateChanged에서 글자색)
	Super::NativePreConstruct();
	if (ControlSurface && Variant == EUIButtonVariant::Text)
	{
		ControlSurface->SetVisibility(ESlateVisibility::Collapsed);   // 부모가 HitTestInvisible로 되돌리므로 다시 숨김
	}
}

void UCC_UIButton::SetLabel(FText InLabel)
{
	Label = InLabel;
	if (LabelText)
	{
		LabelText->SetText(Label);
	}
}

void UCC_UIButton::SetVariant(EUIButtonVariant InVariant)
{
	if (Variant == InVariant)
	{
		return;
	}
	Variant = InVariant;
	ApplyLayout();                         // Role이 Card ↔ Filled로 바뀌면 Surface가 MID를 새로 만듦
	if (ControlSurface)
	{
		ControlSurface->SetSurfaceState(GetVisualState(), true);   // 재초기화로 Rest가 된 Surface에 현재 상태 복원
	}
	NativeOnVisualStateChanged(GetVisualState(), true);
}

void UCC_UIButton::NativeOnVisualStateChanged(EUISurfaceState NewState, bool bInstant)
{
	// rev.7: Filled도 Token 상태 그대로 (Rest = 몸체만 · Hover = 핫스팟 테두리 + 글로우 · Pressed 0.7 · Disabled 0.45)
	ApplyLabelColor(NewState);
}

// ─────────────────────────────────────────────────────────── 내부

void UCC_UIButton::ApplyLayout()
{
	const UCC_UIStyleData* Style = UCC_UIStyleData::GetUIStyle();

	if (ButtonSizeBox)
	{
		const float H = (HeightOverride < 0.f) ? (Style ? Style->Sizes.ButtonHeight : 50.f) : HeightOverride;
		if (H > 0.f)
		{
			ButtonSizeBox->SetHeightOverride(H);
		}
		else
		{
			ButtonSizeBox->ClearHeightOverride();
		}
	}

	if (ControlSurface)
	{
		const bool bSurface = (Variant != EUIButtonVariant::Text);
		ControlSurface->SetVisibility(bSurface ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (bSurface)
		{
			// Filled = MI_Surface_Filled (Token RoleMaterials) — 바깥 글로우 공간 Pad 36은 MI 값 (bAutoOuterPad ✘라도 MI Pad는 적용됨)
			ControlSurface->SurfaceRole = (Variant == EUIButtonVariant::Filled) ? EUISurfaceRole::Filled : EUISurfaceRole::Card;
			ControlSurface->RadiusToken = EUIRadius::Button;     // 14
			ControlSurface->Element = ESkillElementType::None;
			ControlSurface->bAutoOuterPad = false;
			ControlSurface->RefreshFromStyle();
		}
	}

	if (LabelText)
	{
		LabelText->SetText(Label);
		LabelText->SetJustification(ETextJustify::Center);
		LabelText->SetVisibility(ESlateVisibility::HitTestInvisible);
		UCC_UIStyleData::ApplyTextStyle(LabelText,
			(Variant == EUIButtonVariant::Filled) ? EUITextStyle::Button : EUITextStyle::ButtonGhost,
			false);                        // 색은 상태별로 ApplyLabelColor
	}
}

void UCC_UIButton::ApplyLabelColor(EUISurfaceState State)
{
	if (!LabelText)
	{
		return;
	}
	const UCC_UIStyleData* Style = UCC_UIStyleData::GetUIStyle();
	const FColor Primary = Style ? Style->TextPrimary : FColor(0xEE, 0xF0, 0xFF);
	const FColor Ghost = Style ? Style->TextGhost : FColor(0xD6, 0xD9, 0xEC);
	const FColor Secondary = Style ? Style->TextSecondary : FColor(0xC9, 0xCC, 0xE0);
	const FColor Disabled = Style ? Style->TextDisabled : FColor(0x8A, 0x8F, 0xA8);

	const bool bActive = (State == EUISurfaceState::Hover || State == EUISurfaceState::Pressed || State == EUISurfaceState::Selected);
	FColor C;
	if (State == EUISurfaceState::Disabled) { C = Disabled; }
	else if (Variant == EUIButtonVariant::Filled) { C = Primary; }
	else if (Variant == EUIButtonVariant::Text) { C = bActive ? Primary : Secondary; }
	else { C = bActive ? Primary : Ghost; }   // Ghost (Kit4: Hover = 흰 글자)
	LabelText->SetColorAndOpacity(FSlateColor(FLinearColor(C)));
}