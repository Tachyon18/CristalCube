// Fill out your copyright notice in the Description page of Project Settings.

#include "CC_UIControl.h"
#include "CC_UIStyleData.h"
#include "Components/Button.h"
#include "Styling/SlateTypes.h"
#include "Styling/SlateBrush.h"

// CC_UIControl.cpp — L2 공통 부모 · golden-rev6-b4-provide §8-1
// 위치: Source/CristalCube/Widgets/CC_UIControl.cpp

// ─────────────────────────────────────────────────────────── 생명주기

void UCC_UIControl::NativeOnInitialized()
{
	Super::NativeOnInitialized();          // 런타임 1회 (디자이너 X)

	bControlEnabled = bStartEnabled;
	bSelected = bStartSelected;

	if (HitButton)
	{
		HitButton->OnHovered.AddUniqueDynamic(this, &UCC_UIControl::HandleHovered);
		HitButton->OnUnhovered.AddUniqueDynamic(this, &UCC_UIControl::HandleUnhovered);
		HitButton->OnPressed.AddUniqueDynamic(this, &UCC_UIControl::HandlePressed);
		HitButton->OnReleased.AddUniqueDynamic(this, &UCC_UIControl::HandleReleased);
		HitButton->OnClicked.AddUniqueDynamic(this, &UCC_UIControl::HandleClicked);
	}
}

void UCC_UIControl::NativePreConstruct()
{
	Super::NativePreConstruct();           // 디자이너 + 런타임 (자식 Surface는 이미 초기화된 뒤)

	if (IsDesignTime())
	{
		bControlEnabled = bStartEnabled;   // 디자이너 미리보기 = 에디터 설정값
		bSelected = bStartSelected;
	}
	bHovered = false;
	bPressed = false;

	MakeButtonTransparent();
	if (HitButton)
	{
		HitButton->SetIsEnabled(bControlEnabled);
	}
	if (ControlSurface)
	{
		ControlSurface->bAutoHoverState = false;                         // 상태는 Control이 지시
		ControlSurface->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	bHasVisualState = false;
	RefreshVisualState(true);
}

void UCC_UIControl::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!FMath::IsNearlyEqual(CurScale, TgtScale, 1e-4f))
	{
		const float T = ResolveBlendTime();
		const float A = (T <= KINDA_SMALL_NUMBER) ? 1.f : 1.f - FMath::Exp(-InDeltaTime * 4.6f / T);   // Surface 상태 보간과 같은 곡선
		CurScale = FMath::Lerp(CurScale, TgtScale, A);
		if (FMath::IsNearlyEqual(CurScale, TgtScale, 0.0005f))
		{
			CurScale = TgtScale;
		}
		ApplyScale(CurScale);
	}
}

// ─────────────────────────────────────────────────────────── 공개 API

void UCC_UIControl::SetControlEnabled(bool bInEnabled)
{
	if (bControlEnabled == bInEnabled && bHasVisualState)
	{
		return;
	}
	bControlEnabled = bInEnabled;
	if (HitButton)
	{
		HitButton->SetIsEnabled(bInEnabled);
	}
	bPressed = false;
	bHovered = bInEnabled && HitButton && HitButton->IsHovered();   // 커서가 위에 있는 채로 다시 켜질 때
	RefreshVisualState(false);
}

void UCC_UIControl::SetSelected(bool bInSelected)
{
	if (bSelected == bInSelected && bHasVisualState)
	{
		return;
	}
	bSelected = bInSelected;
	RefreshVisualState(false);
}

// ─────────────────────────────────────────────────────────── 입력

void UCC_UIControl::HandleHovered()
{
	bHovered = true;
	RefreshVisualState(false);
	OnControlHovered.Broadcast(this);
}

void UCC_UIControl::HandleUnhovered()
{
	bHovered = false;                      // 누른 채 밖으로 나가면 Pressed 유지 (UButton 캡처) → 놓을 때 Released
	RefreshVisualState(false);
	OnControlUnhovered.Broadcast(this);
}

void UCC_UIControl::HandlePressed()
{
	bPressed = true;
	RefreshVisualState(false);
}

void UCC_UIControl::HandleReleased()
{
	bPressed = false;
	RefreshVisualState(false);
}

void UCC_UIControl::HandleClicked()
{
	if (bControlEnabled)
	{
		OnControlClicked.Broadcast(this);
	}
}

// ─────────────────────────────────────────────────────────── 내부

void UCC_UIControl::RefreshVisualState(bool bInstant)
{
	EUISurfaceState NewState = EUISurfaceState::Rest;
	if (!bControlEnabled) { NewState = EUISurfaceState::Disabled; }
	else if (bPressed) { NewState = EUISurfaceState::Pressed; }
	else if (bSelected) { NewState = EUISurfaceState::Selected; }
	else if (bHovered) { NewState = EUISurfaceState::Hover; }

	if (bHasVisualState && NewState == VisualState && !bInstant)
	{
		return;
	}
	VisualState = NewState;
	bHasVisualState = true;

	if (ControlSurface)
	{
		ControlSurface->SetSurfaceState(NewState, bInstant);
	}

	TgtScale = (bUsePressedScale && NewState == EUISurfaceState::Pressed) ? ResolvePressedScale() : 1.f;
	if (bInstant)
	{
		CurScale = TgtScale;
		ApplyScale(CurScale);
	}

	NativeOnVisualStateChanged(NewState, bInstant);
	BP_OnVisualStateChanged(NewState);
}

void UCC_UIControl::ApplyScale(float Scale)
{
	if (UWidget* Root = GetRootWidget())
	{
		Root->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		Root->SetRenderScale(FVector2D(Scale, Scale));
	}
}

void UCC_UIControl::MakeButtonTransparent()
{
	if (!HitButton)
	{
		return;
	}
	FSlateBrush NoDraw;
	NoDraw.DrawAs = ESlateBrushDrawType::NoDrawType;

	FButtonStyle Style = HitButton->GetStyle();
	Style.SetNormal(NoDraw);
	Style.SetHovered(NoDraw);
	Style.SetPressed(NoDraw);
	Style.SetDisabled(NoDraw);
	Style.SetNormalPadding(FMargin(0.f));
	Style.SetPressedPadding(FMargin(0.f));
	HitButton->SetStyle(Style);
}

float UCC_UIControl::ResolvePressedScale() const
{
	if (PressedScale > 0.f)
	{
		return PressedScale;
	}
	const UCC_UIStyleData* Style = UCC_UIStyleData::GetUIStyle();
	return Style ? Style->Motion.PressedScale : 0.97f;
}

float UCC_UIControl::ResolveBlendTime() const
{
	const UCC_UIStyleData* Style = UCC_UIStyleData::GetUIStyle();
	return Style ? Style->Motion.StateBlend : 0.12f;
}

