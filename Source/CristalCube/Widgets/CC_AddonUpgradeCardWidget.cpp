// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_AddonUpgradeCardWidget.h"
#include "CC_AddonUpgradeRowWidget.h"
#include "CC_UISurface.h"
#include "CC_UIIconWell.h"
#include "CC_UICounter.h"
#include "CC_UIStyleData.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/PanelWidget.h"
#include "Components/SizeBox.h"
#include "Styling/SlateTypes.h"
#include "Styling/SlateBrush.h"

// ─────────────────────────────────────────────────────────── 생명주기

void UCC_AddonUpgradeCardWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (HeaderButton)
	{
		HeaderButton->OnClicked.AddUniqueDynamic(this, &UCC_AddonUpgradeCardWidget::HandleHeaderClicked);
		HeaderButton->OnHovered.AddUniqueDynamic(this, &UCC_AddonUpgradeCardWidget::HandleHeaderHovered);
		HeaderButton->OnUnhovered.AddUniqueDynamic(this, &UCC_AddonUpgradeCardWidget::HandleHeaderUnhovered);
	}
}

void UCC_AddonUpgradeCardWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (IsDesignTime())
	{
		bExpanded = bPreviewExpanded;
	}
	MakeHeaderButtonTransparent();

	if (const UCC_UIStyleData* Style = UCC_UIStyleData::GetUIStyle())
	{
		if (HeaderSizeBox)
		{
			HeaderSizeBox->SetHeightOverride(Style->Sizes.AddonHeaderHeight);   // 66
		}
	}
	if (AddonName) { UCC_UIStyleData::ApplyTextStyle(AddonName, EUITextStyle::TitleSmall, true); }
	if (SharedSkillsText) { UCC_UIStyleData::ApplyTextStyle(SharedSkillsText, EUITextStyle::BodySmall, true); }
	if (TotalText)
	{
		UCC_UIStyleData::ApplyTextStyle(TotalText, EUITextStyle::Numeric, false);
		const UCC_UIStyleData* Style = UCC_UIStyleData::GetUIStyle();
		TotalText->SetColorAndOpacity(FSlateColor(FLinearColor(Style ? Style->TextSecondary : FColor(0xC9, 0xCC, 0xE0))));
	}
	for (UWidget* W : { static_cast<UWidget*>(AddonName.Get()), static_cast<UWidget*>(SharedSkillsText.Get()),
		static_cast<UWidget*>(TotalText.Get()), static_cast<UWidget*>(AddonWell.Get()), ExpandChevron.Get() })
	{
		if (W)
		{
			W->SetVisibility(ESlateVisibility::HitTestInvisible);   // 클릭은 HeaderButton만
		}
	}
	if (NoticeDot && (IsDesignTime() || !bCardDataSet))
	{
		// 런타임 값이 이미 있으면(SetAddonData가 AddChild보다 먼저) 그 값 유지
		NoticeDot->SetVisibility(IsDesignTime() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	ApplySurfaceConfig();
	ApplyExpandedVisual(true);
}

void UCC_AddonUpgradeCardWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ApplySurfaceState(true);               // 자식 Surface 초기화(InitialState) 뒤에 이 카드 상태로 복원
}

void UCC_AddonUpgradeCardWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (RowsFadeElapsed < 0.f || !AttributeRowContainer)
	{
		return;
	}
	RowsFadeElapsed += InDeltaTime;
	const float Dur = GetFadeTime();
	const float A = (Dur > KINDA_SMALL_NUMBER) ? FMath::Clamp(RowsFadeElapsed / Dur, 0.f, 1.f) : 1.f;
	AttributeRowContainer->SetRenderOpacity(FMath::InterpEaseOut(0.f, 1.f, A, 2.f));
	if (A >= 1.f)
	{
		RowsFadeElapsed = -1.f;
	}
}

// ─────────────────────────────────────────────────────────── 공개 API

void UCC_AddonUpgradeCardWidget::SetAddonData(ESkillAddonType InAddonType, const FAddonTableRow& InRow, int32 BankPoints,
	const TMap<FName, int32>& SpentPoints, const FText& InSharedText)
{
	const bool bSameAddon = bCardDataSet && (AddonType == InAddonType);
	AddonType = InAddonType;

	if (AddonWell)
	{
		AddonWell->SetIcon(InRow.Icon, ESkillElementType::None);   // Addon은 원소 없음 — MI 몸체색 (B6에서 Addon별 색 검토)
	}
	if (AddonName)
	{
		AddonName->SetText(InRow.DisplayName);
	}
	if (SharedSkillsText)
	{
		SharedSkillsText->SetText(InSharedText);
	}

	// 합계 "배분 / 최대" (무제한 속성이 하나라도 있으면 배분 수만)
	int32 SumSpent = 0, SumMax = 0;
	bool bUnlimited = false;
	for (const FAddonUpgradeAttribute& Attr : InRow.UpgradeAttributes)
	{
		SumSpent += SpentPoints.FindRef(Attr.AttributeID);
		SumMax += Attr.MaxPoints;
		bUnlimited |= (Attr.MaxPoints <= 0);
	}
	if (TotalText)
	{
		TotalText->SetText(bUnlimited
			? FText::AsNumber(SumSpent)
			: FText::Format(NSLOCTEXT("CC_UI", "AddonCardTotal", "{0} / {1}"), FText::AsNumber(SumSpent), FText::AsNumber(SumMax)));
	}
	if (BankCounter)
	{
		BankCounter->SetValue(BankPoints, bSameAddon);   // 처음 표시는 펄스 없이, 이후 값 변화만 펄스
	}
	if (NoticeDot)
	{
		NoticeDot->SetVisibility(BankPoints > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	// 행 — 같은 Addon · 같은 개수면 재사용
	if (!AttributeRowContainer || !AddonUpgradeRowWidgetClass)
	{
		bCardDataSet = true;
		return;
	}
	const int32 N = InRow.UpgradeAttributes.Num();
	const bool bReuse = bSameAddon && (RowWidgets.Num() == N);
	if (!bReuse)
	{
		AttributeRowContainer->ClearChildren();
		RowWidgets.Reset();
	}
	for (int32 i = 0; i < N; ++i)
	{
		const FAddonUpgradeAttribute& Attr = InRow.UpgradeAttributes[i];
		UCC_AddonUpgradeRowWidget* Row = bReuse ? RowWidgets[i].Get() : CreateWidget<UCC_AddonUpgradeRowWidget>(this, AddonUpgradeRowWidgetClass);
		if (!Row)
		{
			continue;
		}
		Row->SetAttributeData(AddonType, Attr, SpentPoints.FindRef(Attr.AttributeID), BankPoints);
		if (!bReuse)
		{
			Row->OnPointRequested.AddUniqueDynamic(this, &UCC_AddonUpgradeCardWidget::HandleRowPointRequested);
			AttributeRowContainer->AddChild(Row);
			RowWidgets.Add(Row);
		}
	}
	// 구분선 = 행 위쪽(시안 border-top) — 부모 행의 RowDivider는 아래쪽이므로 마지막 행만 숨김
	for (int32 i = 0; i < RowWidgets.Num(); ++i)
	{
		if (RowWidgets[i])
		{
			RowWidgets[i]->SetDividerVisible(i < RowWidgets.Num() - 1);
		}
	}
	bCardDataSet = true;
}

void UCC_AddonUpgradeCardWidget::SetExpanded(bool bInExpanded, bool bInstant)
{
	if (bExpanded == bInExpanded && !bInstant)
	{
		return;
	}
	bExpanded = bInExpanded;
	ApplyExpandedVisual(bInstant);
	ApplySurfaceState(bInstant);
}

// ─────────────────────────────────────────────────────────── 입력

void UCC_AddonUpgradeCardWidget::HandleHeaderClicked()
{
	OnToggleRequested.Broadcast(this);
}

void UCC_AddonUpgradeCardWidget::HandleHeaderHovered()
{
	bHeaderHovered = true;
	ApplySurfaceState(false);
}

void UCC_AddonUpgradeCardWidget::HandleHeaderUnhovered()
{
	bHeaderHovered = false;
	ApplySurfaceState(false);
}

void UCC_AddonUpgradeCardWidget::HandleRowPointRequested(ESkillAddonType InAddonType, FName AttributeID, bool bIsRefund)
{
	// 카드는 PlayerState를 모름 (표시 전용) — Detail이 배분 + 새로고침
	OnPointRequested.Broadcast(InAddonType, AttributeID, bIsRefund);
}

// ─────────────────────────────────────────────────────────── 내부

void UCC_AddonUpgradeCardWidget::ApplySurfaceConfig()
{
	if (!CardSurface)
	{
		return;
	}
	CardSurface->SurfaceRole = EUISurfaceRole::Card;
	CardSurface->RadiusToken = EUIRadius::SubCard;     // 16
	CardSurface->Element = ESkillElementType::None;
	CardSurface->bAutoOuterPad = false;                  // ScrollBox 안 — Pad 영역이 잘려 사각 귀퉁이가 됨
	CardSurface->bAutoHoverState = false;
	CardSurface->SetVisibility(ESlateVisibility::HitTestInvisible);
	CardSurface->RefreshFromStyle();
	ApplySurfaceState(true);
}

void UCC_AddonUpgradeCardWidget::ApplySurfaceState(bool bInstant)
{
	if (CardSurface)
	{
		// 시안 InvLayoutB: 펼친 카드 = `.rim`(핫스팟 테두리) = Hover 모양. 헤더 호버도 같은 모양
		CardSurface->SetSurfaceState((bExpanded || bHeaderHovered) ? EUISurfaceState::Hover : EUISurfaceState::Rest, bInstant);
	}
}

void UCC_AddonUpgradeCardWidget::ApplyExpandedVisual(bool bInstant)
{
	if (ExpandChevron)
	{
		ExpandChevron->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		ExpandChevron->SetRenderTransformAngle(bExpanded ? 180.f : 0.f);
	}
	if (!AttributeRowContainer)
	{
		return;
	}
	if (bExpanded)
	{
		// Visibility 즉시 → 오퍼시티만 0 → 1 (높이 애니메이션 없음 — 로스터 아코디언에서 검증된 방식)
		AttributeRowContainer->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		if (bInstant)
		{
			AttributeRowContainer->SetRenderOpacity(1.f);
			RowsFadeElapsed = -1.f;
		}
		else
		{
			AttributeRowContainer->SetRenderOpacity(0.f);
			RowsFadeElapsed = 0.f;
		}
	}
	else
	{
		// 접을 때는 즉시 — 다른 카드가 동시에 펼쳐져도 높이가 한 번만 바뀜
		AttributeRowContainer->SetVisibility(ESlateVisibility::Collapsed);
		AttributeRowContainer->SetRenderOpacity(1.f);
		RowsFadeElapsed = -1.f;
	}
}

void UCC_AddonUpgradeCardWidget::MakeHeaderButtonTransparent()
{
	if (!HeaderButton)
	{
		return;
	}
	FSlateBrush NoDraw;
	NoDraw.DrawAs = ESlateBrushDrawType::NoDrawType;

	FButtonStyle Style = HeaderButton->GetStyle();
	Style.SetNormal(NoDraw);
	Style.SetHovered(NoDraw);
	Style.SetPressed(NoDraw);
	Style.SetDisabled(NoDraw);
	Style.SetNormalPadding(FMargin(0.f));
	Style.SetPressedPadding(FMargin(0.f));
	HeaderButton->SetStyle(Style);
}

float UCC_AddonUpgradeCardWidget::GetFadeTime() const
{
	const UCC_UIStyleData* Style = UCC_UIStyleData::GetUIStyle();
	return Style ? Style->Motion.AccordionFade : 0.15f;
}