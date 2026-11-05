// Fill out your copyright notice in the Description page of Project Settings.


// CC_UIStyleData.cpp — L0 Token
#include "CC_UIStyleData.h"
#include "Engine/Font.h"
#include "Components/TextBlock.h"
#include "Materials/MaterialInterface.h"

namespace UIStyleDefaults
{
	static FColor Hex(const TCHAR* InHex) { return FColor::FromHex(InHex); }

	static FUIElementTone Tone(const TCHAR* Body, const TCHAR* Text)
	{
		FUIElementTone T;
		T.Body = Hex(Body);
		T.Text = Hex(Text);
		return T;
	}

	static FUITextStyle Text(const TCHAR* FontPath, const TCHAR* Typeface, float SizePx, int32 Spacing, const FColor& Color)
	{
		FUITextStyle S;
		S.Font = TSoftObjectPtr<UFont>(FSoftObjectPath(FontPath));
		S.Typeface = FName(Typeface);
		S.SizePx = SizePx;
		S.LetterSpacing = Spacing;
		S.Color = Color;
		return S;
	}

	static TSoftObjectPtr<UMaterialInterface> MI(const TCHAR* Path)
	{
		return TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(Path));
	}
}

UCC_UIStyleData::UCC_UIStyleData()
{
	using namespace UIStyleDefaults;

	// ── 팔레트 (sRGB) — golden-b1-b2-provide §1-1
	Backdrop = Hex(TEXT("06060A"));
	SheenRamp = { Hex(TEXT("EEF0C8")), Hex(TEXT("F2C4B0")), Hex(TEXT("E3A3B8")), Hex(TEXT("B9A8E6")), Hex(TEXT("9CC8E8")), Hex(TEXT("A8E3CF")) };
	FilledStops = { Hex(TEXT("9C5A78")), Hex(TEXT("7A5FB0")), Hex(TEXT("4A7DB4")), Hex(TEXT("3E8C80")) };
	AccentNotice = Hex(TEXT("F6D2A8"));
	TextPrimary = Hex(TEXT("EEF0FF"));
	TextSecondary = Hex(TEXT("C9CCE0"));
	TextTertiary = Hex(TEXT("9096B0"));
	TextDisabled = Hex(TEXT("8A8FA8"));
	TextGhost = Hex(TEXT("D6D9EC"));
	TextEffect = Hex(TEXT("A8E3CF"));
	PipFill = { Hex(TEXT("F2C4B0")), Hex(TEXT("E3A3B8")), Hex(TEXT("B9A8E6")) };
	PipNext = FColor(255, 255, 255, 46);    // 18%
	PipEmpty = FColor(255, 255, 255, 18);    // 7%
	GaugeHP = { Hex(TEXT("E3A3B8")), Hex(TEXT("F2C4B0")) };
	GaugeBoss = { Hex(TEXT("B9A8E6")) };
	GaugeCubeEnergy = { Hex(TEXT("A8E3CF")), Hex(TEXT("9CC8E8")) };

	// ── 원소 §1-2
	Elements.Add(ESkillElementType::Red, Tone(TEXT("F08A9C"), TEXT("F08A9C")));
	Elements.Add(ESkillElementType::Blue, Tone(TEXT("8AB8F5"), TEXT("8AB8F5")));
	Elements.Add(ESkillElementType::Green, Tone(TEXT("7FE0B8"), TEXT("7FE0B8")));
	Elements.Add(ESkillElementType::Black, Tone(TEXT("7C88A6"), TEXT("A6B0CC")));

	// ── 역할 MI §1-8
	RoleMaterials.Add(EUISurfaceRole::Plate, MI(TEXT("/Game/Develop/Materials/MI_Surface_Plate.MI_Surface_Plate")));
	RoleMaterials.Add(EUISurfaceRole::Card, MI(TEXT("/Game/Develop/Materials/MI_Surface_Card.MI_Surface_Card")));
	RoleMaterials.Add(EUISurfaceRole::Well, MI(TEXT("/Game/Develop/Materials/MI_Surface_Well.MI_Surface_Well")));
	RoleMaterials.Add(EUISurfaceRole::Track, MI(TEXT("/Game/Develop/Materials/MI_Surface_Track.MI_Surface_Track")));
	RoleMaterials.Add(EUISurfaceRole::Pill, MI(TEXT("/Game/Develop/Materials/MI_Surface_Pill.MI_Surface_Pill")));
	RoleMaterials.Add(EUISurfaceRole::Filled, MI(TEXT("/Game/Develop/Materials/MI_Surface_Filled.MI_Surface_Filled")));   // rev.7

	// ── 상태 §1-4 (음수 = 역할 MI 값 유지)
	StateLooks.Add(EUISurfaceState::Rest, FUISurfaceStateLook());
	{ FUISurfaceStateLook L; L.HotStr = 1.f;                                         StateLooks.Add(EUISurfaceState::Hover, L); }
	{ FUISurfaceStateLook L; L.HotStr = 0.7f;                                        StateLooks.Add(EUISurfaceState::Pressed, L); }
	{ FUISurfaceStateLook L; L.HotStr = 1.f; L.SheenOpacity = 0.14f; L.BodyStrAdd = 0.10f; StateLooks.Add(EUISurfaceState::Selected, L); }
	{ FUISurfaceStateLook L; L.HotStr = 0.f; L.SheenOpacity = 0.f; L.RimBase = 0.07f; L.BodyStr = 0.f; L.Opacity = 0.45f; StateLooks.Add(EUISurfaceState::Disabled, L); }
	{ FUISurfaceStateLook L; L.HotStr = 0.f; L.SheenOpacity = 0.f; L.EmptyMix = 1.f; StateLooks.Add(EUISurfaceState::Empty, L); }

	// ── 글꼴 §1-7 (SizePx = 시안 px)
	const TCHAR* Orbitron = TEXT("/Game/Develop/Fonts/Orbitron/Orbitron.Orbitron");
	const TCHAR* Tomorrow = TEXT("/Game/Develop/Fonts/Tomorrow/Tomorrow.Tomorrow");
	const TCHAR* Oxanium = TEXT("/Game/Develop/Fonts/Oxanium/Oxanium.Oxanium");
	const TCHAR* Suit = TEXT("/Game/Develop/Fonts/SUIT/SUIT.SUIT");

	TextStyles.Add(EUITextStyle::Display, Text(Orbitron, TEXT("Bold"), 24.f, 120, TextPrimary));
	TextStyles.Add(EUITextStyle::DisplayHero, Text(Orbitron, TEXT("Black"), 44.f, 50, TextPrimary));
	TextStyles.Add(EUITextStyle::Heading, Text(Tomorrow, TEXT("SemiBold"), 24.f, 0, TextPrimary));
	TextStyles.Add(EUITextStyle::Title, Text(Tomorrow, TEXT("SemiBold"), 16.f, 0, TextPrimary));
	TextStyles.Add(EUITextStyle::TitleSmall, Text(Tomorrow, TEXT("SemiBold"), 15.f, 0, TextPrimary));
	TextStyles.Add(EUITextStyle::Body, Text(Suit, TEXT("Regular"), 14.f, 0, TextPrimary));
	TextStyles.Add(EUITextStyle::BodyMuted, Text(Suit, TEXT("Regular"), 13.5f, 0, TextSecondary));
	TextStyles.Add(EUITextStyle::BodySmall, Text(Suit, TEXT("Regular"), 12.f, 0, TextTertiary));
	TextStyles.Add(EUITextStyle::Caption, Text(Oxanium, TEXT("Regular"), 11.f, 220, TextTertiary));
	TextStyles.Add(EUITextStyle::CaptionTight, Text(Oxanium, TEXT("Regular"), 10.5f, 80, TextTertiary));
	TextStyles.Add(EUITextStyle::Numeric, Text(Oxanium, TEXT("SemiBold"), 13.f, 0, TextPrimary));
	TextStyles.Add(EUITextStyle::Counter, Text(Oxanium, TEXT("SemiBold"), 15.f, 0, AccentNotice));
	TextStyles.Add(EUITextStyle::Effect, Text(Oxanium, TEXT("Regular"), 12.f, 0, TextEffect));
	TextStyles.Add(EUITextStyle::Button, Text(Suit, TEXT("Bold"), 15.f, 0, TextPrimary));
	TextStyles.Add(EUITextStyle::ButtonGhost, Text(Suit, TEXT("Regular"), 15.f, 0, TextGhost));
}

UCC_UIStyleData* UCC_UIStyleData::GetUIStyle()
{
	const UCC_UISettings* Settings = GetDefault<UCC_UISettings>();
	return Settings ? Settings->DefaultStyle.LoadSynchronous() : nullptr;
}

FLinearColor UCC_UIStyleData::SampleGradient(const TArray<FColor>& Stops, float T)
{
	if (Stops.Num() == 0) { return FLinearColor::White; }
	if (Stops.Num() == 1) { return FLinearColor(Stops[0]); }
	const float X = FMath::Clamp(T, 0.f, 1.f) * (Stops.Num() - 1);
	const int32 I = FMath::Min(FMath::FloorToInt(X), Stops.Num() - 2);
	const float F = X - I;
	// CSS 그라데이션과 같은 sRGB 공간 보간 → Linear
	const FColor& A = Stops[I];
	const FColor& B = Stops[I + 1];
	const FColor Mixed(
		(uint8)FMath::RoundToInt(FMath::Lerp((float)A.R, (float)B.R, F)),
		(uint8)FMath::RoundToInt(FMath::Lerp((float)A.G, (float)B.G, F)),
		(uint8)FMath::RoundToInt(FMath::Lerp((float)A.B, (float)B.B, F)),
		(uint8)FMath::RoundToInt(FMath::Lerp((float)A.A, (float)B.A, F)));
	return FLinearColor(Mixed);
}

FLinearColor UCC_UIStyleData::GetElementBodyLinear(ESkillElementType Element) const
{
	const FUIElementTone* T = Elements.Find(Element);
	return T ? FLinearColor(T->Body) : FLinearColor::Black;
}

FLinearColor UCC_UIStyleData::GetElementTextLinear(ESkillElementType Element) const
{
	const FUIElementTone* T = Elements.Find(Element);
	return T ? FLinearColor(T->Text) : FLinearColor(TextPrimary);
}

float UCC_UIStyleData::GetRadius(EUIRadius Radius) const
{
	switch (Radius)
	{
	case EUIRadius::Plate:      return Radii.Plate;
	case EUIRadius::Section:    return Radii.Section;
	case EUIRadius::Entry:      return Radii.Entry;
	case EUIRadius::SubCard:    return Radii.SubCard;
	case EUIRadius::Button:     return Radii.Button;
	case EUIRadius::IconButton: return Radii.IconButton;
	case EUIRadius::WellL:      return Radii.WellL;
	case EUIRadius::WellM:      return Radii.WellM;
	case EUIRadius::WellS:      return Radii.WellS;
	case EUIRadius::Full:       return Radii.Full;
	default:                    return -1.f;   // FromMaterial
	}
}

UMaterialInterface* UCC_UIStyleData::GetRoleMaterial(EUISurfaceRole Role) const
{
	const TSoftObjectPtr<UMaterialInterface>* Found = RoleMaterials.Find(Role);
	return Found ? Found->LoadSynchronous() : nullptr;
}

FSlateFontInfo UCC_UIStyleData::MakeFontInfo(EUITextStyle Style) const
{
	FSlateFontInfo Info;
	if (const FUITextStyle* S = TextStyles.Find(Style))
	{
		Info.FontObject = S->Font.LoadSynchronous();
		Info.TypefaceFontName = S->Typeface;
		Info.Size = S->SizePx * PxToSlatePt;
		// 기존: Info.LetterSpacing = S->LetterSpacing;
		Info.LetterSpacing = FMath::RoundToInt(S->LetterSpacing * LetterSpacingScale);
	}
	return Info;
}

void UCC_UIStyleData::ApplyTextStyle(UTextBlock* Text, EUITextStyle Style, bool bApplyColor)
{
	const UCC_UIStyleData* Data = GetUIStyle();
	if (!Text || !Data)
	{
		return;
	}
	const FUITextStyle* S = Data->TextStyles.Find(Style);
	if (!S)
	{
		return;
	}
	Text->SetFont(Data->MakeFontInfo(Style));
	if (bApplyColor)
	{
		Text->SetColorAndOpacity(FSlateColor(FLinearColor(S->Color)));
	}
}
