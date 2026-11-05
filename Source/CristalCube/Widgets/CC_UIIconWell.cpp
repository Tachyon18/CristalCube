// Fill out your copyright notice in the Description page of Project Settings.

#include "CC_UIIconWell.h"
#include "CC_UIStyleData.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Engine/Texture2D.h"
#include "Engine/GameInstance.h"
#include "../SkillSystem/CC_SkillLibrarySubsystem.h"

// CC_UIIconWell.cpp — L2 Primitive: IconWell · golden-rev6-b4-provide §8-2
// 위치: Source/CristalCube/Widgets/CC_UIIconWell.cpp

// ─────────────────────────────────────────────────────────── 생명주기

void UCC_UIIconWell::NativePreConstruct()
{
	Super::NativePreConstruct();
	ApplyAll();                            // 디자이너 미리보기 (자식 Surface 초기화 뒤에 덮어씀)
}

void UCC_UIIconWell::NativeConstruct()
{
	Super::NativeConstruct();
	ApplyAll();
}

// ─────────────────────────────────────────────────────────── 공개 API

void UCC_UIIconWell::SetIcon(UTexture2D* InIcon, ESkillElementType InElement)
{
	Icon = InIcon;
	if (Element != InElement)
	{
		Element = InElement;
		if (WellSurface)
		{
			WellSurface->SetElement(InElement);   // 몸체색 + Selected BodyStrAdd 재계산 (Surface 재초기화 없음)
		}
	}
	ApplyIcon();
	ApplyIconState();
}

bool UCC_UIIconWell::SetSkill(FName SkillID)
{
	UGameInstance* GI = GetGameInstance();
	UCC_SkillLibrarySubsystem* Library = GI ? GI->GetSubsystem<UCC_SkillLibrarySubsystem>() : nullptr;
	if (!Library)
	{
		return false;
	}
	FSkillDisplayData Data;
	if (!Library->GetSkillDisplayData(SkillID, Data))
	{
		return false;
	}
	SetIcon(Data.Icon, Data.ElementType);
	return true;
}

void UCC_UIIconWell::SetWellSize(EUIWellSize InSize)
{
	if (WellSize == InSize)
	{
		return;
	}
	WellSize = InSize;
	ApplyAll();                            // 반경 Token이 바뀜 → Surface 재초기화
}

void UCC_UIIconWell::SetWellState(EUISurfaceState NewState, bool bInstant)
{
	WellState = NewState;
	bStateInitialized = true;
	if (WellSurface)
	{
		WellSurface->SetSurfaceState(NewState, bInstant);
	}
	ApplyIconState();
}

// ─────────────────────────────────────────────────────────── 내부

void UCC_UIIconWell::ApplyAll()
{
	if (IsDesignTime() || !bStateInitialized)
	{
		WellState = InitialWellState;      // 런타임은 첫 1회만 — 이후 SetWellState 값 유지 (재Construct 시에도)
		bStateInitialized = !IsDesignTime();
	}

	const float WellPx = GetWellPx();
	if (WellSizeBox)
	{
		WellSizeBox->SetWidthOverride(WellPx);
		WellSizeBox->SetHeightOverride(WellPx);
	}

	if (WellSurface)
	{
		WellSurface->SurfaceRole = EUISurfaceRole::Well;
		WellSurface->RadiusToken = GetRadiusToken();
		WellSurface->Element = Element;
		WellSurface->bAutoOuterPad = bReserveHeadPad;
		WellSurface->bAutoHoverState = false;
		WellSurface->SetVisibility(ESlateVisibility::HitTestInvisible);
		WellSurface->RefreshFromStyle();                      // MI · 반경 · 원소색 다시 읽음 (상태는 InitialState로 초기화됨)
		WellSurface->SetSurfaceState(WellState, true);        // → 이 위젯의 상태로 즉시 복원
	}

	ApplyIcon();
	ApplyIconState();
}

void UCC_UIIconWell::ApplyIcon()
{
	if (!IconImage)
	{
		return;
	}
	if (Icon)
	{
		IconImage->SetBrushFromTexture(Icon, false);
	}
	const float IconPx = FMath::RoundToFloat(GetWellPx() * IconRatio);
	IconImage->SetDesiredSizeOverride(FVector2D(IconPx, IconPx));
	IconImage->SetColorAndOpacity(IconTint);
}

void UCC_UIIconWell::ApplyIconState()
{
	if (!IconImage)
	{
		return;
	}
	// 텍스처 없는 Brush는 흰 사각형으로 그려짐 → 아이콘 없으면 숨김. Empty도 숨김(점선만)
	const bool bShow = (Icon != nullptr) && (WellState != EUISurfaceState::Empty);
	IconImage->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);

	// Disabled — Surface와 같은 Opacity (Token StateLooks · 기본 0.45). Surface는 0.12s 보간, 아이콘은 즉시
	float Opacity = 1.f;
	if (WellState == EUISurfaceState::Disabled)
	{
		Opacity = 0.45f;
		if (const UCC_UIStyleData* Style = UCC_UIStyleData::GetUIStyle())
		{
			if (const FUISurfaceStateLook* Look = Style->StateLooks.Find(EUISurfaceState::Disabled))
			{
				Opacity = Look->Opacity;
			}
		}
	}
	IconImage->SetRenderOpacity(Opacity);
}

float UCC_UIIconWell::GetWellPx() const
{
	const UCC_UIStyleData* Style = UCC_UIStyleData::GetUIStyle();
	switch (WellSize)
	{
	case EUIWellSize::S: return Style ? Style->Sizes.WellS : 40.f;
	case EUIWellSize::L: return Style ? Style->Sizes.WellL : 64.f;
	default:             return Style ? Style->Sizes.WellM : 48.f;
	}
}

EUIRadius UCC_UIIconWell::GetRadiusToken() const
{
	switch (WellSize)
	{
	case EUIWellSize::S: return EUIRadius::WellS;
	case EUIWellSize::L: return EUIRadius::WellL;
	default:             return EUIRadius::WellM;
	}
}