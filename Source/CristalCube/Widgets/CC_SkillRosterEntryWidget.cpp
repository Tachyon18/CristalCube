// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_SkillRosterEntryWidget.h"
#include "CC_UIIconWell.h"
#include "CC_UIStyleData.h"
#include "CC_SkillSlotDragOp.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Engine/GameInstance.h"
#include "../SkillSystem/CC_SkillBase.h"
#include "../SkillSystem/CC_SkillLibrarySubsystem.h"
#include "../CC_PlayerState.h"

// ─────────────────────────────────────────────────────────── 생명주기

void UCC_SkillRosterEntryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetVisibility(ESlateVisibility::Visible);   // 클릭·드래그·호버를 이 위젯이 직접 받음 (드래그 복제본은 PrepareAsDragGhost가 다시 끔)
}

void UCC_SkillRosterEntryWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (const UCC_UIStyleData* Style = UCC_UIStyleData::GetUIStyle())
	{
		if (EntrySizeBox)
		{
			EntrySizeBox->SetHeightOverride(Style->Sizes.EntryHeight);   // 76
		}
	}
	for (UWidget* W : { static_cast<UWidget*>(SkillWell.Get()), static_cast<UWidget*>(SkillName.Get()),
		static_cast<UWidget*>(SummaryText.Get()) })
	{
		if (W)
		{
			W->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}

	if (IsDesignTime())
	{
		ApplyPreview();
		return;
	}
	ApplyTextStyles(IsEmptySlot());
	ApplySurfaceConfig();
}

void UCC_SkillRosterEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ApplySurfaceState(true);               // 자식 Surface가 InitialState로 초기화된 뒤 복원
}

// ─────────────────────────────────────────────────────────── 공개 API

void UCC_SkillRosterEntryWidget::SetSkillData(int32 InSlotIndex, UCC_SkillBase* Skill, ACC_PlayerState* PlayerState)
{
	SlotIndex = InSlotIndex;
	BoundSkill = Skill;
	BoundPlayerState = PlayerState;
	bDataSet = true;

	if (IsEmptySlot())
	{
		// 빈 슬롯 — 선택 불가, Ctrl+드래그 드롭 대상 전용
		bHovered = false;
		bHasUnspent = false;
		CurrentElement = ESkillElementType::None;
		ApplyTextStyles(true);
		if (SkillName) { SkillName->SetText(EmptyName); }
		if (SummaryText) { SummaryText->SetText(EmptySummary); }
		if (NoticeDot) { NoticeDot->SetVisibility(ESlateVisibility::Collapsed); }
		if (EmptySlotOverlay) { EmptySlotOverlay->SetVisibility(ESlateVisibility::HitTestInvisible); }
		if (SkillWell)
		{
			SkillWell->SetIcon(nullptr, ESkillElementType::None);
			SkillWell->SetWellState(EUISurfaceState::Empty);
		}
		if (EntrySurface) { EntrySurface->SetElement(ESkillElementType::None); }
		ApplySurfaceState(false);
		return;
	}

	ApplyTextStyles(false);
	if (EmptySlotOverlay) { EmptySlotOverlay->SetVisibility(ESlateVisibility::Collapsed); }

	UGameInstance* GI = GetGameInstance();
	UCC_SkillLibrarySubsystem* Library = GI ? GI->GetSubsystem<UCC_SkillLibrarySubsystem>() : nullptr;
	FSkillDisplayData Data;
	if (Library && Library->GetSkillDisplayData(Skill->GetSkillID(), Data))
	{
		const FName SkillID = Skill->GetSkillID();
		if (SkillName) { SkillName->SetText(Data.DisplayName); }

		if (SkillWell)
		{
			SkillWell->SetIcon(Data.Icon, Data.ElementType);
			SkillWell->SetWellState(EUISurfaceState::Rest);
		}
		if (CurrentElement != Data.ElementType)
		{
			CurrentElement = Data.ElementType;
			if (EntrySurface) { EntrySurface->SetElement(CurrentElement); }   // Selected 때 원소 몸체 10%
		}

		// 요약 — CoreType · CORE(배분 합계) · ADDON(구현된 Addon 수)
		int32 CoreSpent = 0;
		for (const FCoreUpgradeAttribute& Attr : Library->GetSkillCoreUpgradeAttributes(SkillID))
		{
			CoreSpent += PlayerState->GetSkillCoreAttributeSpentPoints(SkillID, Attr.AttributeType);
		}
		const int32 AddonCount = Library->GetAddonBadgesForSkill(Data).Num();
		if (SummaryText)
		{
			SummaryText->SetText(FText::Format(NSLOCTEXT("CC_UI", "RosterSummary", "{0} · CORE {1} · ADDON {2}"),
				UEnum::GetDisplayValueAsText(Data.CoreType), FText::AsNumber(CoreSpent), FText::AsNumber(AddonCount)));
		}

		// 미사용 포인트 — 이 스킬 Core 은행 또는 이 스킬이 가진 Addon 종류의 은행
		bHasUnspent = PlayerState->GetSkillCoreUnspentPoints(SkillID) > 0;
		for (const ESkillAddonType AddonType : Data.Addons)
		{
			bHasUnspent |= PlayerState->GetAddonUnspentPoints(AddonType) > 0;
		}
	}
	if (NoticeDot)
	{
		NoticeDot->SetVisibility(bHasUnspent ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	ApplySurfaceState(false);
}

void UCC_SkillRosterEntryWidget::SetSelected(bool bInSelected, bool bInstant)
{
	if (bSelected == bInSelected && !bInstant)
	{
		return;
	}
	bSelected = bInSelected;
	ApplySurfaceState(bInstant);
}

// ─────────────────────────────────────────────────────────── 입력

void UCC_SkillRosterEntryWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	bHovered = !IsEmptySlot();
	ApplySurfaceState(false);
}

void UCC_SkillRosterEntryWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	bHovered = false;
	ApplySurfaceState(false);
}

FReply UCC_SkillRosterEntryWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && InMouseEvent.IsControlDown() && BoundSkill != nullptr)
	{
		return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
	}
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && BoundSkill != nullptr)
	{
		bPendingClick = true;
		return FReply::Handled().CaptureMouse(TakeWidget());
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UCC_SkillRosterEntryWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bPendingClick)
	{
		bPendingClick = false;
		// 눌렀던 카드 위에서 뗐을 때만 클릭 (버튼의 hover-cancel과 같은 감각)
		if (InGeometry.IsUnderLocation(InMouseEvent.GetScreenSpacePosition()) && BoundSkill)
		{
			OnSelectRequested.Broadcast(this);
		}
		return FReply::Handled().ReleaseMouseCapture();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UCC_SkillRosterEntryWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	UCC_SkillSlotDragOp* DragOp = NewObject<UCC_SkillSlotDragOp>(this);
	DragOp->SourceSlotIndex = SlotIndex;

	// (fix1) 커서를 따라오는 것 = 이 카드의 복제본 (같은 WBP 클래스 · Selected 모양 · 같은 폭)
	//        아이콘이 없는 스킬(DT_Skill Icon 비어 있음)이어도 카드째 보이므로 '잡고 있음'이 분명함
	if (UCC_SkillRosterEntryWidget* Ghost = CreateWidget<UCC_SkillRosterEntryWidget>(this, GetClass()))
	{
		Ghost->SetSkillData(SlotIndex, BoundSkill, BoundPlayerState);
		Ghost->PrepareAsDragGhost(InGeometry.GetLocalSize().X);
		DragOp->DefaultDragVisual = Ghost;
	}
	DragOp->Pivot = EDragPivot::MouseDown;      // 잡은 위치 그대로 따라옴
	DragOp->OnDrop.AddDynamic(this, &UCC_SkillRosterEntryWidget::HandleDragEnded);
	DragOp->OnDragCancelled.AddDynamic(this, &UCC_SkillRosterEntryWidget::HandleDragEnded);

	SetDragSource(true);                        // 원래 자리 카드 흐리게
	OutOperation = DragOp;
}

bool UCC_SkillRosterEntryWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	const bool bSuperHandled = Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	bDragOver = false;
	ApplySurfaceState(false);
	if (UCC_SkillSlotDragOp* DragOp = Cast<UCC_SkillSlotDragOp>(InOperation))
	{
		OnSlotDropRequested.Broadcast(DragOp->SourceSlotIndex, SlotIndex);
		return true;
	}
	return bSuperHandled;
}

void UCC_SkillRosterEntryWidget::NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragEnter(InGeometry, InDragDropEvent, InOperation);
	const UCC_SkillSlotDragOp* DragOp = Cast<UCC_SkillSlotDragOp>(InOperation);
	bDragOver = DragOp && DragOp->SourceSlotIndex != SlotIndex;   // 자기 자신 위는 표시 안 함
	ApplySurfaceState(false);
}

void UCC_SkillRosterEntryWidget::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragLeave(InDragDropEvent, InOperation);
	bDragOver = false;
	ApplySurfaceState(false);
}

void UCC_SkillRosterEntryWidget::HandleDragEnded(UDragDropOperation* Operation)
{
	SetDragSource(false);
}

// ─────────────────────────────────────────────────────────── 드래그 복제본 (fix1)

void UCC_SkillRosterEntryWidget::PrepareAsDragGhost(float Width)
{
	bIsDragGhost = true;
	bSelected = true;                       // 잡힌 카드 = Selected 모양 (테두리 + Sheen + 원소색)
	SetVisibility(ESlateVisibility::HitTestInvisible);   // 드롭 판정을 가로채지 않게
	if (EntrySizeBox && Width > 0.f)
	{
		EntrySizeBox->SetWidthOverride(Width);
	}
	SetRenderOpacity(DragGhostOpacity);
	ApplySurfaceState(true);
}

void UCC_SkillRosterEntryWidget::SetDragSource(bool bInDragSource)
{
	bDragSource = bInDragSource;
	SetRenderOpacity(bDragSource ? DragSourceOpacity : 1.f);
}

// ─────────────────────────────────────────────────────────── 내부

bool UCC_SkillRosterEntryWidget::IsEmptySlot() const
{
	return bDataSet && (BoundSkill == nullptr || BoundPlayerState == nullptr);
}

void UCC_SkillRosterEntryWidget::ApplySurfaceConfig()
{
	if (!EntrySurface)
	{
		return;
	}
	EntrySurface->SurfaceRole = EUISurfaceRole::Card;
	EntrySurface->RadiusToken = EUIRadius::Entry;      // 18
	EntrySurface->Element = CurrentElement;
	EntrySurface->bAutoOuterPad = false;                 // 목록 — 핫스팟 이동 없음 (정적 Selected 테두리만)
	EntrySurface->bAutoHoverState = false;                 // 상태는 이 위젯이 지시
	EntrySurface->SetVisibility(ESlateVisibility::HitTestInvisible);
	EntrySurface->RefreshFromStyle();
	ApplySurfaceState(true);
}

void UCC_SkillRosterEntryWidget::ApplySurfaceState(bool bInstant)
{
	if (!EntrySurface)
	{
		return;
	}
	EUISurfaceState State = EUISurfaceState::Rest;
	if (bDragOver) { State = EUISurfaceState::Hover; }   // (fix1) 놓을 자리 — 빈 슬롯도 테두리로 표시
	else if (IsEmptySlot()) { State = EUISurfaceState::Empty; }
	else if (bSelected) { State = EUISurfaceState::Selected; }
	else if (bHovered) { State = EUISurfaceState::Hover; }
	EntrySurface->SetSurfaceState(State, bInstant);
}

void UCC_SkillRosterEntryWidget::ApplyTextStyles(bool bEmpty)
{
	if (SkillName)
	{
		UCC_UIStyleData::ApplyTextStyle(SkillName, EUITextStyle::Title, true);
		if (bEmpty)
		{
			const UCC_UIStyleData* Style = UCC_UIStyleData::GetUIStyle();
			SkillName->SetColorAndOpacity(FSlateColor(FLinearColor(Style ? Style->TextTertiary : FColor(0x90, 0x96, 0xB0))));
		}
	}
	if (SummaryText)
	{
		UCC_UIStyleData::ApplyTextStyle(SummaryText, EUITextStyle::CaptionTight, true);
	}
}

void UCC_SkillRosterEntryWidget::ApplyPreview()
{
	// 디자이너 전용 — 데이터 없이 Rest / Selected / Empty 모양 확인
	CurrentElement = bPreviewEmpty ? ESkillElementType::None : PreviewElement;
	ApplyTextStyles(bPreviewEmpty);
	if (SkillName) { SkillName->SetText(bPreviewEmpty ? EmptyName : PreviewName); }
	if (SummaryText) { SummaryText->SetText(bPreviewEmpty ? EmptySummary : PreviewSummary); }
	if (NoticeDot) { NoticeDot->SetVisibility((bPreviewUnspent && !bPreviewEmpty) ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed); }
	if (EmptySlotOverlay) { EmptySlotOverlay->SetVisibility(bPreviewEmpty ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed); }
	if (SkillWell)
	{
		SkillWell->SetIcon(SkillWell->Icon, CurrentElement);
		SkillWell->SetWellState(bPreviewEmpty ? EUISurfaceState::Empty : EUISurfaceState::Rest, true);
	}
	if (EntrySurface)
	{
		EntrySurface->SurfaceRole = EUISurfaceRole::Card;
		EntrySurface->RadiusToken = EUIRadius::Entry;
		EntrySurface->Element = CurrentElement;
		EntrySurface->bAutoOuterPad = false;
		EntrySurface->bAutoHoverState = false;
		EntrySurface->RefreshFromStyle();
		EntrySurface->SetSurfaceState(bPreviewEmpty ? EUISurfaceState::Empty
			: (bPreviewSelected ? EUISurfaceState::Selected : EUISurfaceState::Rest), true);
	}
}