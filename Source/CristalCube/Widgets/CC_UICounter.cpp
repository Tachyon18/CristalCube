// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_UICounter.h"
#include "CC_UIStyleData.h"
#include "CC_UISurface.h"
#include "Components/TextBlock.h"

void UCC_UICounter::NativePreConstruct()
{
	Super::NativePreConstruct();
	if (IsDesignTime() || !bValueSet)
	{
		Value = PreviewValue;
	}
	PulseElapsed = -1.f;
	ApplyStatic();
	ApplyValueVisual(0.f);
}

void UCC_UICounter::SetValue(int32 InValue, bool bPulse)
{
	const bool bChanged = (InValue != Value);
	bValueSet = true;
	Value = InValue;
	if (ValueText)
	{
		ValueText->SetText(FText::AsNumber(Value));
	}
	if (bPulse && bChanged)
	{
		PulseElapsed = 0.f;               // Tick이 정점까지 올렸다 내림
	}
	ApplyValueVisual(0.f);                  // 기본색(값 0 ↔ >0) 즉시 반영 — 펄스는 다음 Tick부터
}

void UCC_UICounter::SetCaption(FText InCaption)
{
	Caption = InCaption;
	if (CaptionText)
	{
		CaptionText->SetText(Caption);
	}
}

void UCC_UICounter::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (PulseElapsed < 0.f)
	{
		return;
	}
	const UCC_UIStyleData* Style = UCC_UIStyleData::GetUIStyle();
	const float Duration = Style ? Style->Motion.CounterPulse : 0.25f;
	PulseElapsed += InDeltaTime;
	const float T = (Duration > KINDA_SMALL_NUMBER) ? FMath::Clamp(PulseElapsed / Duration, 0.f, 1.f) : 1.f;
	const float Alpha = FMath::Sin(T * UE_PI);              // 0 → 1 → 0
	ApplyValueVisual(T >= 1.f ? 0.f : Alpha);
	if (T >= 1.f)
	{
		PulseElapsed = -1.f;
	}
}

// ─────────────────────────────────────────────────────────── 내부

void UCC_UICounter::ApplyStatic()
{
	UCC_UIStyleData::ApplyTextStyle(ValueText, EUITextStyle::Counter, false);
	if (ValueText)
	{
		ValueText->SetText(FText::AsNumber(Value));
		ValueText->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		ValueText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (CaptionText)
	{
		UCC_UIStyleData::ApplyTextStyle(CaptionText, EUITextStyle::Caption, true);
		CaptionText->SetText(Caption);
		CaptionText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (CounterSurface)
	{
		CounterSurface->SurfaceRole = EUISurfaceRole::Pill;
		CounterSurface->RadiusToken = EUIRadius::Full;
		CounterSurface->bAutoOuterPad = false;
		CounterSurface->bAutoHoverState = false;
		CounterSurface->SetVisibility(bShowSurface ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (bShowSurface)
		{
			CounterSurface->RefreshFromStyle();
		}
	}
}

void UCC_UICounter::ApplyValueVisual(float PulseAlpha)
{
	if (!ValueText)
	{
		return;
	}
	const UCC_UIStyleData* Style = UCC_UIStyleData::GetUIStyle();
	const FLinearColor Base = (Value > 0)
		? FLinearColor(Style ? Style->AccentNotice : FColor(0xF6, 0xD2, 0xA8))
		: FLinearColor(Style ? Style->TextTertiary : FColor(0x90, 0x96, 0xB0));
	const FLinearColor Color = FMath::Lerp(Base, FLinearColor::White, 0.5f * PulseAlpha);   // 정점에서 흰색 쪽으로 50%
	ValueText->SetColorAndOpacity(FSlateColor(Color));

	const float S = FMath::Lerp(1.f, PulseScale, PulseAlpha);
	ValueText->SetRenderScale(FVector2D(S, S));
}
