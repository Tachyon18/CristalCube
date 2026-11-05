// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_SkillInventoryWidget.h"
#include "CC_SkillRosterEntryWidget.h"
#include "CC_SkillUpgradeDetailWidget.h"
#include "CC_UISurface.h"
#include "CC_UIStyleData.h"
#include "Components/PanelWidget.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/TextBlock.h"
#include "../CC_PlayerState.h"
#include "../SkillSystem/CC_SkillBase.h"

// ─────────────────────────────────────────────────────────── 생명주기

void UCC_SkillInventoryWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (TitleText) { UCC_UIStyleData::ApplyTextStyle(TitleText, EUITextStyle::Display, true); }
	if (ListCaptionText) { UCC_UIStyleData::ApplyTextStyle(ListCaptionText, EUITextStyle::Caption, true); }
	if (GlobalNoticeText) { UCC_UIStyleData::ApplyTextStyle(GlobalNoticeText, EUITextStyle::BodyMuted, true); }
	if (FooterText) { UCC_UIStyleData::ApplyTextStyle(FooterText, EUITextStyle::Caption, true); }

	if (InventoryPlate)
	{
		InventoryPlate->SurfaceRole = EUISurfaceRole::Plate;
		InventoryPlate->RadiusToken = EUIRadius::Plate;     // 30
		InventoryPlate->bAutoOuterPad = true;                  // Intro Head 공간
		InventoryPlate->bAutoHoverState = false;
		InventoryPlate->SetVisibility(ESlateVisibility::HitTestInvisible);
		InventoryPlate->RefreshFromStyle();
	}
}

void UCC_SkillInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BoundPlayerState = GetOwningPlayerState<ACC_PlayerState>();
	if (BoundPlayerState)
	{
		BoundPlayerState->OnSkillsChanged.AddUniqueDynamic(this, &UCC_SkillInventoryWidget::HandleSkillsChanged);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[SkillInventory] Owning PlayerState is not ACC_PlayerState!"));
	}
	if (DetailPanel)
	{
		DetailPanel->OnPointsChanged.AddUniqueDynamic(this, &UCC_SkillInventoryWidget::HandleDetailPointsChanged);
	}

	DebugGrantPoints();
	RefreshInventory();
	PlayOpenIntro();
}

void UCC_SkillInventoryWidget::NativeDestruct()
{
	if (BoundPlayerState)
	{
		BoundPlayerState->OnSkillsChanged.RemoveDynamic(this, &UCC_SkillInventoryWidget::HandleSkillsChanged);
	}
	if (DetailPanel)
	{
		DetailPanel->OnPointsChanged.RemoveDynamic(this, &UCC_SkillInventoryWidget::HandleDetailPointsChanged);
	}
	Super::NativeDestruct();
}

// ─────────────────────────────────────────────────────────── 공개 API

void UCC_SkillInventoryWidget::RefreshInventory()
{
	if (!RosterContainer || !RosterEntryWidgetClass || !BoundPlayerState)
	{
		return;
	}
	EnsureEntries();
	RefreshRosterData();
	ResolveSelection();
	ApplySelection(false);

	if (ListCaptionText)
	{
		ListCaptionText->SetText(FText::Format(NSLOCTEXT("CC_UI", "InvListCaption", "장착 스킬 · {0}"),
			FText::AsNumber(BoundPlayerState->GetSkillCount())));
	}
}

void UCC_SkillInventoryWidget::SelectSlot(int32 SlotIndex)
{
	if (!BoundPlayerState || !RosterEntries.IsValidIndex(SlotIndex))
	{
		return;
	}
	UCC_SkillBase* Skill = BoundPlayerState->GetSkillAtSlot(SlotIndex);
	if (!Skill || SlotIndex == SelectedSlot)
	{
		return;
	}
	SelectedSkill = Skill;
	SelectedSlot = SlotIndex;
	ApplySelection(false);
}

void UCC_SkillInventoryWidget::PlayOpenIntro()
{
	if (InventoryPlate)
	{
		InventoryPlate->PlayRimIntro();      // Token RimIntro 0.5s — 좌상단 → 기본 위치
	}
}

// ─────────────────────────────────────────────────────────── 이벤트

void UCC_SkillInventoryWidget::HandleEntrySelectRequested(UCC_SkillRosterEntryWidget* Entry)
{
	if (Entry)
	{
		SelectSlot(Entry->GetSlotIndex());
	}
}

void UCC_SkillInventoryWidget::HandleSlotDropRequested(int32 SourceSlotIndex, int32 TargetSlotIndex)
{
	if (BoundPlayerState)
	{
		BoundPlayerState->SwapSlots(SourceSlotIndex, TargetSlotIndex);   // → OnSkillsChanged → RefreshInventory (선택은 스킬을 따라감)
	}
}

void UCC_SkillInventoryWidget::HandleSkillsChanged()
{
	RefreshInventory();
}

void UCC_SkillInventoryWidget::HandleDetailPointsChanged()
{
	RefreshRosterData();                     // 요약 CORE n · Notice Dot (선택·상세는 그대로)
}

// ─────────────────────────────────────────────────────────── 내부

void UCC_SkillInventoryWidget::EnsureEntries()
{
	if (RosterEntries.Num() == NumEquipSlots)
	{
		return;                              // 최초 1회만 생성 — 이후 SetSkillData로 내용만 갱신
	}
	RosterContainer->ClearChildren();
	RosterEntries.Reset();

	const UCC_UIStyleData* Style = UCC_UIStyleData::GetUIStyle();
	const float Gap = Style ? Style->Spacing.ListGap : 10.f;

	for (int32 SlotIndex = 0; SlotIndex < NumEquipSlots; ++SlotIndex)
	{
		UCC_SkillRosterEntryWidget* Entry = CreateWidget<UCC_SkillRosterEntryWidget>(this, RosterEntryWidgetClass);
		if (!Entry)
		{
			continue;
		}
		Entry->OnSelectRequested.AddUniqueDynamic(this, &UCC_SkillInventoryWidget::HandleEntrySelectRequested);
		Entry->OnSlotDropRequested.AddUniqueDynamic(this, &UCC_SkillInventoryWidget::HandleSlotDropRequested);
		Entry->SetSkillData(SlotIndex, BoundPlayerState->GetSkillAtSlot(SlotIndex), BoundPlayerState);

		if (UVerticalBoxSlot* VSlot = Cast<UVerticalBoxSlot>(RosterContainer->AddChild(Entry)))
		{
			VSlot->SetPadding(FMargin(0.f, 0.f, 0.f, (SlotIndex < NumEquipSlots - 1) ? Gap : 0.f));
			VSlot->SetHorizontalAlignment(HAlign_Fill);
		}
		RosterEntries.Add(Entry);
	}
}

void UCC_SkillInventoryWidget::RefreshRosterData()
{
	if (!BoundPlayerState)
	{
		return;
	}
	for (int32 SlotIndex = 0; SlotIndex < RosterEntries.Num(); ++SlotIndex)
	{
		if (UCC_SkillRosterEntryWidget* Entry = RosterEntries[SlotIndex])
		{
			Entry->SetSkillData(SlotIndex, BoundPlayerState->GetSkillAtSlot(SlotIndex), BoundPlayerState);
		}
	}
	RefreshGlobalNotice();
}

void UCC_SkillInventoryWidget::ResolveSelection()
{
	SelectedSlot = INDEX_NONE;
	int32 FirstOccupied = INDEX_NONE;
	for (int32 i = 0; i < NumEquipSlots; ++i)
	{
		UCC_SkillBase* Skill = BoundPlayerState->GetSkillAtSlot(i);
		if (!Skill)
		{
			continue;
		}
		if (FirstOccupied == INDEX_NONE)
		{
			FirstOccupied = i;
		}
		if (SelectedSkill.IsValid() && Skill == SelectedSkill.Get())
		{
			SelectedSlot = i;
		}
	}
	if (SelectedSlot == INDEX_NONE)
	{
		SelectedSlot = FirstOccupied;                       // 처음 열 때 · 선택 스킬이 빠졌을 때
		SelectedSkill = (FirstOccupied != INDEX_NONE) ? BoundPlayerState->GetSkillAtSlot(FirstOccupied) : nullptr;
	}
}

void UCC_SkillInventoryWidget::ApplySelection(bool bInstant)
{
	for (int32 i = 0; i < RosterEntries.Num(); ++i)
	{
		if (RosterEntries[i])
		{
			RosterEntries[i]->SetSelected(i == SelectedSlot, bInstant);
		}
	}
	if (DetailPanel)
	{
		if (SelectedSkill.IsValid())
		{
			DetailPanel->ShowSkill(SelectedSkill.Get(), BoundPlayerState);   // 같은 스킬이면 값만 갱신
		}
		else
		{
			DetailPanel->ShowEmpty();
		}
	}
}

void UCC_SkillInventoryWidget::RefreshGlobalNotice()
{
	bool bAny = false;
	for (const UCC_SkillRosterEntryWidget* Entry : RosterEntries)
	{
		bAny |= (Entry && Entry->HasUnspentPoints());
	}
	if (GlobalNotice)
	{
		GlobalNotice->SetVisibility(bAny ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UCC_SkillInventoryWidget::DebugGrantPoints()
{
#if !UE_BUILD_SHIPPING
	if (!BoundPlayerState || (DebugGrantCorePoints <= 0 && DebugGrantAddonPoints <= 0))
	{
		return;
	}
	TSet<ESkillAddonType> Granted;
	for (UCC_SkillBase* Skill : BoundPlayerState->GetAllSkills())
	{
		if (!IsValid(Skill))
		{
			continue;
		}
		if (DebugGrantCorePoints > 0)
		{
			BoundPlayerState->AddSkillCorePoints(Skill->GetSkillID(), DebugGrantCorePoints);
		}
		if (DebugGrantAddonPoints > 0)
		{
			for (const UCC_SkillAddonBase* Addon : Skill->GetDefinition().Addons)
			{
				if (Addon && !Granted.Contains(Addon->AddonType))
				{
					Granted.Add(Addon->AddonType);
					BoundPlayerState->AddAddonPoints(Addon->AddonType, DebugGrantAddonPoints);
				}
			}
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("[SkillInventory] DEBUG 포인트 지급 — Core %d/스킬 · Addon %d/종류 (%d종)"),
		DebugGrantCorePoints, DebugGrantAddonPoints, Granted.Num());
#endif
}
