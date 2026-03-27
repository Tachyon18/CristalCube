// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_GlassWidget.h"
#include "Components/Image.h"
#include "Components/BackgroundBlur.h"
#include "Materials/MaterialInstanceDynamic.h"

void UCC_GlassWidget::NativeConstruct()
{
	Super::NativeConstruct();
	InitMaterials();
	SetTheme(DefaultTheme);
}

void UCC_GlassWidget::SetTheme(EGlassTheme Theme)
{
	if (Theme == EGlassTheme::Custom)
	{
		ApplyThemeData(CustomThemeData);
		return;
	}

	FGlassThemeData D = GetThemeData(Theme);

	D.AspectRatio = CustomThemeData.AspectRatio;  
	D.CornerRadius = CustomThemeData.CornerRadius;

	ApplyThemeData(D);
}

void UCC_GlassWidget::SetCustomTheme(const FGlassThemeData& ThemeData)
{
	ApplyThemeData(ThemeData);
}

void UCC_GlassWidget::SetBlurStrength(float Strength)
{
	if (GlassBlur)
		GlassBlur->SetBlurStrength(Strength);
}

void UCC_GlassWidget::InitMaterials()
{
	if (PanelMaterial && GlassTintLayer)
	{
		PanelMI = UMaterialInstanceDynamic::Create(PanelMaterial, this);
		GlassTintLayer->SetBrushFromMaterial(PanelMI);
	}

	if (CornerGlowMaterial && GlassCornerGlow)
	{
		CornerGlowMI = UMaterialInstanceDynamic::Create(CornerGlowMaterial, this);
		GlassCornerGlow->SetBrushFromMaterial(CornerGlowMI);
	}

	if (BorderMaterial && GlassBorderImage)
	{
		BorderMI = UMaterialInstanceDynamic::Create(BorderMaterial, this);
		GlassBorderImage->SetBrushFromMaterial(BorderMI);
	}
}

void UCC_GlassWidget::ApplyThemeData(const FGlassThemeData& Data)
{
	// Panel MI
	if (PanelMI)
	{
		PanelMI->SetVectorParameterValue(TEXT("TintColor"), Data.TintColor);
		PanelMI->SetScalarParameterValue(TEXT("Opacity"), Data.PanelOpacity);
		PanelMI->SetScalarParameterValue(TEXT("PrismIntensity"), Data.PrismIntensity);
		PanelMI->SetScalarParameterValue(TEXT("CornerRadius"), Data.CornerRadius);
		PanelMI->SetScalarParameterValue(TEXT("AspectRatio"), Data.AspectRatio);
	}

	// CornerGlow MI
	if (CornerGlowMI)
	{
		CornerGlowMI->SetVectorParameterValue(TEXT("GlowColor"), Data.GlowColor);
		CornerGlowMI->SetScalarParameterValue(TEXT("GlowOpacity"), Data.GlowOpacity);
	}

	if (BorderMI)
	{
		// Border 색상은 테마 GlowColor와 동일 계열로 연동
		BorderMI->SetVectorParameterValue(TEXT("BorderColor"), Data.GlowColor);
		BorderMI->SetScalarParameterValue(TEXT("BorderOpacity"), Data.GlowOpacity);
		BorderMI->SetScalarParameterValue(TEXT("CornerRadius"), Data.CornerRadius);
		BorderMI->SetScalarParameterValue(TEXT("AspectRatio"), Data.AspectRatio);
	}

	// Blur
	if (GlassBlur)
		GlassBlur->SetBlurStrength(Data.BlurStrength);

}

FGlassThemeData UCC_GlassWidget::GetThemeData(EGlassTheme Theme)
{
	FGlassThemeData D;
	switch (Theme)
	{
	case EGlassTheme::Ocean:
		D.TintColor = FLinearColor(0.08f, 0.25f, 0.80f, 1.f);
		D.PrismIntensity = 0.18f;
		D.GlowColor = FLinearColor(0.20f, 0.60f, 1.00f, 1.f);
		D.GlowOpacity = 0.28f;
		break;
	case EGlassTheme::Galaxy:
		D.TintColor = FLinearColor(0.25f, 0.10f, 0.80f, 1.f);
		D.PrismIntensity = 0.20f;
		D.GlowColor = FLinearColor(0.55f, 0.25f, 1.00f, 1.f);
		D.GlowOpacity = 0.26f;
		break;
	case EGlassTheme::Sunset:
		D.TintColor = FLinearColor(0.80f, 0.12f, 0.30f, 1.f);
		D.PrismIntensity = 0.18f;
		D.GlowColor = FLinearColor(1.00f, 0.30f, 0.55f, 1.f);
		D.GlowOpacity = 0.24f;
		break;
	case EGlassTheme::Fire:
		D.TintColor = FLinearColor(0.60f, 0.20f, 0.05f, 1.f);
		D.PrismIntensity = 0.16f;
		D.GlowColor = FLinearColor(1.00f, 0.45f, 0.10f, 1.f);
		D.GlowOpacity = 0.26f;
		break;
	case EGlassTheme::Forest:
		D.TintColor = FLinearColor(0.06f, 0.50f, 0.25f, 1.f);
		D.PrismIntensity = 0.16f;
		D.GlowColor = FLinearColor(0.15f, 0.85f, 0.55f, 1.f);
		D.GlowOpacity = 0.24f;
		break;
	case EGlassTheme::White:
		D.TintColor = FLinearColor(0.80f, 0.85f, 1.00f, 1.f);
		D.PrismIntensity = 0.12f;
		D.GlowColor = FLinearColor(0.70f, 0.80f, 1.00f, 1.f);
		D.GlowOpacity = 0.22f;
		break;
	default: break;
	}
	return D;
}

