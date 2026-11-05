// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_SkillUpgradeDetailWidget.h"
#include "CC_CoreUpgradeRowWidget.h"
#include "CC_AddonUpgradeCardWidget.h"
#include "CC_UISurface.h"
#include "CC_UIIconWell.h"
#include "CC_UICounter.h"
#include "CC_UIStyleData.h"
#include "Components/TextBlock.h"
#include "Components/PanelWidget.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Button.h"
#include "Engine/GameInstance.h"
#include "../SkillSystem/CC_SkillBase.h"
#include "../SkillSystem/CC_SkillLibrarySubsystem.h"
#include "../CC_PlayerState.h"

// ─────────────────────────────────────────────────────────── 생명주기

void UCC_SkillUpgradeDetailWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (CloseButton)
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &UCC_SkillUpgradeDetailWidget::HandleCloseClicked);
	}
}

void UCC_SkillUpgradeDetailWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	const UCC_UIStyleData* Style = UCC_UIStyleData::GetUIStyle();
	const FLinearColor Secondary(Style ? Style->TextSecondary : FColor(0xC9, 0xCC, 0xE0));

	if (SkillName) { UCC_UIStyleData::ApplyTextStyle(SkillName, EUITextStyle::Heading, true); }
	if (SkillDescription) { UCC_UIStyleData::ApplyTextStyle(SkillDescription, EUITextStyle::BodyMuted, true); SkillDescription->SetAutoWrapText(true); }
	if (SkillMetaText) { UCC_UIStyleData::ApplyTextStyle(SkillMetaText, EUITextStyle::Numeric, false); SkillMetaText->SetColorAndOpacity(FSlateColor(Secondary)); }
	if (ElementText) { UCC_UIStyleData::ApplyTextStyle(ElementText, EUITextStyle::Numeric, false); }
	if (CoreLabel) { UCC_UIStyleData::ApplyTextStyle(CoreLabel, EUITextStyle::Caption, true); }
	if (AddonLabel) { UCC_UIStyleData::ApplyTextStyle(AddonLabel, EUITextStyle::Caption, true); }
	if (CoreSubLabel) { UCC_UIStyleData::ApplyTextStyle(CoreSubLabel, EUITextStyle::BodySmall, true); }
	if (AddonSubLabel) { UCC_UIStyleData::ApplyTextStyle(AddonSubLabel, EUITextStyle::BodySmall, true); }
	if (AddonEmptyText) { UCC_UIStyleData::ApplyTextStyle(AddonEmptyText, EUITextStyle::BodySmall, true); }

	ApplySurfaceConfig();
}

void UCC_SkillUpgradeDetailWidget::NativeConstruct()
{
	Super::NativeConstruct();
	// 자식 Surface가 InitialState로 초기화된 뒤 — 이 패널의 상태로 복원
	if (HeaderSurface)
	{
		HeaderSurface->SetElement(HeaderElement);
		HeaderSurface->SetSurfaceState(CurrentSkill.IsValid() ? EUISurfaceState::Selected : EUISurfaceState::Rest, true);
	}
}

// ─────────────────────────────────────────────────────────── 공개 API

void UCC_SkillUpgradeDetailWidget::ShowSkill(UCC_SkillBase* Skill, ACC_PlayerState* PlayerState)
{
	if (!Skill || !PlayerState)
	{
		ShowEmpty();
		return;
	}
	UCC_SkillLibrarySubsystem* Library = GetSkillLibrary();
	if (!Library)
	{
		return;
	}

	const bool bSkillChanged = (CurrentSkill.Get() != Skill);
	CurrentSkill = Skill;
	BoundPlayerState = PlayerState;

	RefreshHeader(Skill, Library);
	RefreshCore(Skill, PlayerState, Library, bSkillChanged);
	RefreshAddons(Skill, PlayerState, Library, bSkillChanged);

	if (bSkillChanged && bHeaderIntroOnSkillChange && HeaderSurface)
	{
		HeaderSurface->PlayRimIntro();     // Token RimIntro 0.5s / 0.35 둘레
	}
}

void UCC_SkillUpgradeDetailWidget::ShowEmpty()
{
	CurrentSkill.Reset();
	ExpandedAddon = ESkillAddonType::None;
	HeaderElement = ESkillElementType::None;

	if (SkillName) { SkillName->SetText(NSLOCTEXT("CC_UI", "DetailEmptyName", "장착된 스킬 없음")); }
	if (SkillMetaText) { SkillMetaText->SetText(FText::GetEmpty()); }
	if (ElementText) { ElementText->SetText(FText::GetEmpty()); }
	if (SkillDescription) { SkillDescription->SetText(FText::GetEmpty()); }
	if (SkillWell)
	{
		SkillWell->SetIcon(nullptr, ESkillElementType::None);
		SkillWell->SetWellState(EUISurfaceState::Empty);
	}
	if (HeaderSurface)
	{
		HeaderSurface->SetElement(ESkillElementType::None);
		HeaderSurface->SetSurfaceState(EUISurfaceState::Rest);
	}
	if (CoreBankCounter) { CoreBankCounter->SetValue(0, false); }
	if (CoreAttributeContainer) { CoreAttributeContainer->ClearChildren(); }
	if (AddonCardBox) { AddonCardBox->ClearChildren(); }
	CoreRows.Reset();
	AddonCards.Reset();
	if (AddonEmptyText) { AddonEmptyText->SetVisibility(ESlateVisibility::HitTestInvisible); }
}

// ─────────────────────────────────────────────────────────── 배분 요청 (행 → 여기 → PlayerState)

void UCC_SkillUpgradeDetailWidget::HandleCorePointRequested(ECoreUpgradeAttribute AttributeType, bool bIsRefund)
{
	if (!BoundPlayerState.IsValid() || !CurrentSkill.IsValid())
	{
		return;
	}
	UCC_SkillLibrarySubsystem* Library = GetSkillLibrary();
	if (!Library)
	{
		return;
	}
	const FName SkillID = CurrentSkill->GetSkillID();

	FCoreUpgradeAttribute AttrDef;
	if (!Library->GetSkillCoreUpgradeAttribute(SkillID, AttributeType, AttrDef))
	{
		return;
	}
	const bool bSuccess = bIsRefund
		? BoundPlayerState->RefundSkillCorePoint(SkillID, AttributeType, AttrDef.ValuePerPoint)
		: BoundPlayerState->SpendSkillCorePoint(SkillID, AttributeType, AttrDef.ValuePerPoint, AttrDef.MaxPoints);

	if (bSuccess)
	{
		ShowSkill(CurrentSkill.Get(), BoundPlayerState.Get());   // 같은 스킬 → 값만 갱신
		OnPointsChanged.Broadcast();
	}
}

void UCC_SkillUpgradeDetailWidget::HandleCardPointRequested(ESkillAddonType AddonType, FName AttributeID, bool bIsRefund)
{
	if (!BoundPlayerState.IsValid())
	{
		return;
	}
	UCC_SkillLibrarySubsystem* Library = GetSkillLibrary();
	if (!Library)
	{
		return;
	}
	FAddonTableRow AddonRow;
	if (!Library->GetAddonDisplayDataByType(AddonType, AddonRow))
	{
		return;
	}
	const FAddonUpgradeAttribute* AttrDef = AddonRow.UpgradeAttributes.FindByPredicate(
		[&AttributeID](const FAddonUpgradeAttribute& A) { return A.AttributeID == AttributeID; });
	if (!AttrDef)
	{
		return;
	}
	const bool bSuccess = bIsRefund
		? BoundPlayerState->RefundAddonPoint(AddonType, AttributeID, AttrDef->ValuePerPoint)
		: BoundPlayerState->SpendAddonPoint(AddonType, AttributeID, AttrDef->ValuePerPoint, AttrDef->MaxPoints);

	if (bSuccess && CurrentSkill.IsValid())
	{
		ShowSkill(CurrentSkill.Get(), BoundPlayerState.Get());
		OnPointsChanged.Broadcast();
	}
}

void UCC_SkillUpgradeDetailWidget::HandleCardToggleRequested(UCC_AddonUpgradeCardWidget* Card)
{
	if (!Card)
	{
		return;
	}
	const ESkillAddonType Type = Card->GetAddonType();
	ExpandedAddon = (ExpandedAddon == Type) ? ESkillAddonType::None : Type;   // 같은 카드 = 접기

	for (UCC_AddonUpgradeCardWidget* C : AddonCards)
	{
		if (C)
		{
			C->SetExpanded(C->GetAddonType() == ExpandedAddon, false);   // 한 번에 하나 — 접힘은 즉시, 펼침은 페이드
		}
	}
}

void UCC_SkillUpgradeDetailWidget::HandleCloseClicked()
{
	OnCloseRequested.Broadcast();
}

// ─────────────────────────────────────────────────────────── 내부

UCC_SkillLibrarySubsystem* UCC_SkillUpgradeDetailWidget::GetSkillLibrary() const
{
	UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<UCC_SkillLibrarySubsystem>() : nullptr;
}

void UCC_SkillUpgradeDetailWidget::ApplySurfaceConfig()
{
	if (HeaderSurface)
	{
		HeaderSurface->SurfaceRole = EUISurfaceRole::Card;
		HeaderSurface->RadiusToken = EUIRadius::Section;   // 22
		HeaderSurface->Element = HeaderElement;
		HeaderSurface->bAutoOuterPad = true;                  // Intro Head 공간 (스크롤 밖이라 잘리지 않음)
		HeaderSurface->bAutoHoverState = false;
		HeaderSurface->SetVisibility(ESlateVisibility::HitTestInvisible);
		HeaderSurface->RefreshFromStyle();
		HeaderSurface->SetSurfaceState((IsDesignTime() || CurrentSkill.IsValid()) ? EUISurfaceState::Selected : EUISurfaceState::Rest, true);
	}
	for (UCC_UISurface* S : { CoreSurface.Get(), AddonSurface.Get() })
	{
		if (!S)
		{
			continue;
		}
		S->SurfaceRole = EUISurfaceRole::Card;
		S->RadiusToken = EUIRadius::Section;
		S->Element = ESkillElementType::None;
		S->bAutoOuterPad = false;
		S->bAutoHoverState = false;
		S->SetVisibility(ESlateVisibility::HitTestInvisible);
		S->RefreshFromStyle();
		S->SetSurfaceState(EUISurfaceState::Rest, true);
	}
}

void UCC_SkillUpgradeDetailWidget::RefreshHeader(UCC_SkillBase* Skill, UCC_SkillLibrarySubsystem* Library)
{
	FSkillDisplayData Data;
	if (!Library->GetSkillDisplayData(Skill->GetSkillID(), Data))
	{
		return;
	}
	HeaderElement = Data.ElementType;

	if (SkillName) { SkillName->SetText(Data.DisplayName); }
	if (SkillDescription) { SkillDescription->SetText(Data.Description); }
	if (SkillMetaText)
	{
		SkillMetaText->SetText(FText::Format(NSLOCTEXT("CC_UI", "DetailMeta", "{0} · "),
			UEnum::GetDisplayValueAsText(Data.CoreType)));
	}
	if (ElementText)
	{
		ElementText->SetText(GetElementLabel(Data.ElementType));
		if (const UCC_UIStyleData* Style = UCC_UIStyleData::GetUIStyle())
		{
			ElementText->SetColorAndOpacity(FSlateColor(Style->GetElementTextLinear(Data.ElementType)));
		}
	}
	if (SkillWell)
	{
		SkillWell->SetIcon(Data.Icon, Data.ElementType);
		SkillWell->SetWellState(EUISurfaceState::Rest);
	}
	if (HeaderSurface)
	{
		HeaderSurface->SetElement(Data.ElementType);                     // Selected BodyStrAdd = 원소색 10%
		HeaderSurface->SetSurfaceState(EUISurfaceState::Selected);
	}
}

void UCC_SkillUpgradeDetailWidget::RefreshCore(UCC_SkillBase* Skill, ACC_PlayerState* PlayerState,
	UCC_SkillLibrarySubsystem* Library, bool bSkillChanged)
{
	const FName SkillID = Skill->GetSkillID();
	const TArray<FCoreUpgradeAttribute> CoreAttrs = Library->GetSkillCoreUpgradeAttributes(SkillID);
	const int32 CoreBank = PlayerState->GetSkillCoreUnspentPoints(SkillID);

	if (CoreBankCounter)
	{
		CoreBankCounter->SetValue(CoreBank, !bSkillChanged);   // 스킬 전환은 펄스 없이
	}
	if (!CoreAttributeContainer || !CoreAttributeRowWidgetClass)
	{
		return;
	}

	// 같은 속성 구성이면 행 재사용 (스킬이 바뀌어도 Damage/Cooldown/… 순서가 같으면 그대로)
	bool bReuse = (CoreRows.Num() == CoreAttrs.Num());
	for (int32 i = 0; bReuse && i < CoreAttrs.Num(); ++i)
	{
		bReuse = CoreRows[i] && CoreRows[i]->GetAttributeType() == CoreAttrs[i].AttributeType;
	}
	if (!bReuse)
	{
		CoreAttributeContainer->ClearChildren();
		CoreRows.Reset();
	}

	for (int32 i = 0; i < CoreAttrs.Num(); ++i)
	{
		const FCoreUpgradeAttribute& Attr = CoreAttrs[i];
		UCC_CoreUpgradeRowWidget* Row = bReuse ? CoreRows[i].Get() : CreateWidget<UCC_CoreUpgradeRowWidget>(this, CoreAttributeRowWidgetClass);
		if (!Row)
		{
			continue;
		}
		Row->SetAttributeData(Attr, PlayerState->GetSkillCoreAttributeSpentPoints(SkillID, Attr.AttributeType), CoreBank);
		if (!bReuse)
		{
			Row->OnPointRequested.AddUniqueDynamic(this, &UCC_SkillUpgradeDetailWidget::HandleCorePointRequested);
			CoreAttributeContainer->AddChild(Row);
			CoreRows.Add(Row);
		}
	}
	for (int32 i = 0; i < CoreRows.Num(); ++i)
	{
		if (CoreRows[i])
		{
			CoreRows[i]->SetDividerVisible(i < CoreRows.Num() - 1);
		}
	}
}

void UCC_SkillUpgradeDetailWidget::RefreshAddons(UCC_SkillBase* Skill, ACC_PlayerState* PlayerState,
	UCC_SkillLibrarySubsystem* Library, bool bSkillChanged)
{
	// 1) 이 스킬의 배분 가능 Addon 목록 (카탈로그에 속성이 있는 것만, 중복 제거)
	struct FAddonView
	{
		ESkillAddonType Type;
		FAddonTableRow Row;
	};
	TArray<FAddonView> Views;
	for (const UCC_SkillAddonBase* Addon : Skill->GetDefinition().Addons)
	{
		if (!Addon)
		{
			continue;
		}
		const ESkillAddonType Type = Addon->AddonType;
		if (Views.ContainsByPredicate([Type](const FAddonView& V) { return V.Type == Type; }))
		{
			continue;
		}
		FAddonTableRow Row;
		if (!Library->GetAddonDisplayDataByType(Type, Row) || Row.UpgradeAttributes.Num() == 0)
		{
			continue;
		}
		Views.Add({ Type, Row });
	}

	UE_LOG(LogTemp, Warning, TEXT("[AddonDiag] %s · 런타임 Addons %d · 카드 대상 %d · CardBox %d · CardClass %d · EmptyText %d"),
		*Skill->GetSkillID().ToString(), Skill->GetDefinition().Addons.Num(), Views.Num(),
		AddonCardBox ? 1 : 0, AddonCardWidgetClass ? 1 : 0, AddonEmptyText ? 1 : 0);

	if (AddonEmptyText)
	{
		AddonEmptyText->SetVisibility(Views.Num() == 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	// 2) 펼침 대상 — 스킬이 바뀌면 첫 카드(시안 기본), 같은 스킬이면 유지 (사라졌으면 None)
	if (bSkillChanged)
	{
		ExpandedAddon = (Views.Num() > 0) ? Views[0].Type : ESkillAddonType::None;
	}
	else if (!Views.ContainsByPredicate([this](const FAddonView& V) { return V.Type == ExpandedAddon; }))
	{
		ExpandedAddon = ESkillAddonType::None;
	}

	if (!AddonCardWidgetClass)
	{
		// 배치된 인스턴스 값이 비어 있으면 클래스 기본값(WBP Class Defaults)에서 가져옴
		if (const UCC_SkillUpgradeDetailWidget* CDO = GetClass()->GetDefaultObject<UCC_SkillUpgradeDetailWidget>())
		{
			AddonCardWidgetClass = CDO->AddonCardWidgetClass;
		}
	}

	if (!AddonCardBox || !AddonCardWidgetClass)
	{
		return;
	}

	// 3) 카드 — 같은 스킬 · 같은 Addon 구성이면 재사용
	bool bReuse = !bSkillChanged && (AddonCards.Num() == Views.Num());
	for (int32 i = 0; bReuse && i < Views.Num(); ++i)
	{
		bReuse = AddonCards[i] && AddonCards[i]->GetAddonType() == Views[i].Type;
	}
	if (!bReuse)
	{
		AddonCardBox->ClearChildren();
		AddonCards.Reset();
	}

	const UCC_UIStyleData* Style = UCC_UIStyleData::GetUIStyle();
	const float Gap = Style ? Style->Spacing.M : 10.f;

	for (int32 i = 0; i < Views.Num(); ++i)
	{
		const FAddonView& V = Views[i];
		UCC_AddonUpgradeCardWidget* Card = bReuse ? AddonCards[i].Get() : CreateWidget<UCC_AddonUpgradeCardWidget>(this, AddonCardWidgetClass);
		if (!Card)
		{
			UE_LOG(LogTemp, Warning, TEXT("[AddonDiag]   CreateWidget 실패"));
			continue;
		}

		TMap<FName, int32> Spent;
		for (const FAddonUpgradeAttribute& Attr : V.Row.UpgradeAttributes)
		{
			Spent.Add(Attr.AttributeID, PlayerState->GetAddonAttributeSpentPoints(V.Type, Attr.AttributeID));
		}
		Card->SetAddonData(V.Type, V.Row, PlayerState->GetAddonUnspentPoints(V.Type), Spent,
			BuildSharedText(V.Type, PlayerState, Library));

		if (!bReuse)
		{
			Card->SetExpanded(V.Type == ExpandedAddon, true);
			Card->OnPointRequested.AddUniqueDynamic(this, &UCC_SkillUpgradeDetailWidget::HandleCardPointRequested);
			Card->OnToggleRequested.AddUniqueDynamic(this, &UCC_SkillUpgradeDetailWidget::HandleCardToggleRequested);
			if (UVerticalBoxSlot* VSlot = Cast<UVerticalBoxSlot>(AddonCardBox->AddChild(Card)))
			{
				VSlot->SetPadding(FMargin(0.f, 0.f, 0.f, (i < Views.Num() - 1) ? Gap : 0.f));
				VSlot->SetHorizontalAlignment(HAlign_Fill);
			}
			AddonCards.Add(Card);
			UE_LOG(LogTemp, Warning, TEXT("[AddonDiag]   카드 추가 %s · Box 자식 %d"),
				*UEnum::GetValueAsString(V.Type), AddonCardBox->GetChildrenCount());
		}
	}
}

FText UCC_SkillUpgradeDetailWidget::BuildSharedText(ESkillAddonType AddonType, ACC_PlayerState* PlayerState,
	UCC_SkillLibrarySubsystem* Library) const
{
	// 이 Addon을 가진 장착 스킬 전부 (배분이 그 스킬들에 함께 적용됨 — PlayerState::SpendAddonPoint)
	TArray<FText> Names;
	for (const UCC_SkillBase* S : PlayerState->GetAllSkills())
	{
		if (!IsValid(S) || !S->HasAddon(AddonType))
		{
			continue;
		}
		FSkillDisplayData D;
		Names.Add(Library->GetSkillDisplayData(S->GetSkillID(), D) ? D.DisplayName : FText::FromName(S->GetSkillID()));
	}
	return FText::Format(NSLOCTEXT("CC_UI", "AddonShared", "공유 · {0}"),
		FText::Join(FText::FromString(TEXT(" · ")), Names));
}

FText UCC_SkillUpgradeDetailWidget::GetElementLabel(ESkillElementType Element)
{
	switch (Element)
	{
	case ESkillElementType::Red:   return NSLOCTEXT("CC_UI", "ElemRed", "Red 공격");
	case ESkillElementType::Blue:  return NSLOCTEXT("CC_UI", "ElemBlue", "Blue 제어");
	case ESkillElementType::Green: return NSLOCTEXT("CC_UI", "ElemGreen", "Green 생명");
	case ESkillElementType::Black: return NSLOCTEXT("CC_UI", "ElemBlack", "Black 무채");
	default:                       return FText::GetEmpty();
	}
}