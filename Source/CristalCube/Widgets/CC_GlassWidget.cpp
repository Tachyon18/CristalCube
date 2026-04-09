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

		PanelMI->SetVectorParameterValue(TEXT("Light0_PosRadius"), Data.Light0_PosRadius);
		PanelMI->SetVectorParameterValue(TEXT("Light0_Color"), Data.Light0_Color);
		PanelMI->SetScalarParameterValue(TEXT("Light0_Intensity"), Data.Light0_Intensity);

		PanelMI->SetVectorParameterValue(TEXT("Light1_PosRadius"), Data.Light1_PosRadius);
		PanelMI->SetVectorParameterValue(TEXT("Light1_Color"), Data.Light1_Color);
		PanelMI->SetScalarParameterValue(TEXT("Light1_Intensity"), Data.Light1_Intensity);

		PanelMI->SetVectorParameterValue(TEXT("Light2_PosRadius"), Data.Light2_PosRadius);
		PanelMI->SetVectorParameterValue(TEXT("Light2_Color"), Data.Light2_Color);
		PanelMI->SetScalarParameterValue(TEXT("Light2_Intensity"), Data.Light2_Intensity);

		PanelMI->SetVectorParameterValue(TEXT("Light3_PosRadius"), Data.Light3_PosRadius);
		PanelMI->SetVectorParameterValue(TEXT("Light3_Color"), Data.Light3_Color);
		PanelMI->SetScalarParameterValue(TEXT("Light3_Intensity"), Data.Light3_Intensity);

		PanelMI->SetScalarParameterValue(TEXT("SceneLightsOpacityBoost"), Data.SceneLightsOpacityBoost);
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

		D.Light0_PosRadius = FLinearColor(0.00f, 0.00f, 0.55f, 0.f);
		D.Light0_Color = FLinearColor(0.20f, 0.60f, 1.00f, 0.f);
		D.Light0_Intensity = 0.22f;
		D.Light1_PosRadius = FLinearColor(0.80f, 0.18f, 0.28f, 0.f);
		D.Light1_Color = FLinearColor(1.00f, 0.80f, 0.40f, 0.f);
		D.Light1_Intensity = 0.045f;
		D.Light2_PosRadius = FLinearColor(0.16f, 0.78f, 0.32f, 0.f);
		D.Light2_Color = FLinearColor(0.30f, 0.65f, 1.00f, 0.f);
		D.Light2_Intensity = 0.032f;
		D.Light3_PosRadius = FLinearColor(0.55f, 0.55f, 0.14f, 0.f);
		D.Light3_Color = FLinearColor(0.55f, 0.35f, 1.00f, 0.f);
		D.Light3_Intensity = 0.020f;
		D.SceneLightsOpacityBoost = 0.35f;
		break;
	case EGlassTheme::Galaxy:
		D.TintColor = FLinearColor(0.25f, 0.10f, 0.80f, 1.f);
		D.PrismIntensity = 0.20f;
		D.GlowColor = FLinearColor(0.55f, 0.25f, 1.00f, 1.f);
		D.GlowOpacity = 0.26f;

		D.Light0_PosRadius = FLinearColor(0.00f, 0.00f, 0.55f, 0.f);
		D.Light0_Color = FLinearColor(0.55f, 0.25f, 1.00f, 0.f);
		D.Light0_Intensity = 0.24f;
		D.Light1_PosRadius = FLinearColor(0.82f, 0.15f, 0.30f, 0.f);
		D.Light1_Color = FLinearColor(0.80f, 0.45f, 1.00f, 0.f);
		D.Light1_Intensity = 0.48f;
		D.Light2_PosRadius = FLinearColor(0.15f, 0.80f, 0.34f, 0.f);
		D.Light2_Color = FLinearColor(0.30f, 0.18f, 1.00f, 0.f);
		D.Light2_Intensity = 0.36f;
		D.Light3_PosRadius = FLinearColor(0.55f, 0.50f, 0.14f, 0.f);
		D.Light3_Color = FLinearColor(1.00f, 0.45f, 0.90f, 0.f);
		D.Light3_Intensity = 0.22f;
		D.SceneLightsOpacityBoost = 0.35f;
		break;
	case EGlassTheme::Sunset:
		D.TintColor = FLinearColor(0.80f, 0.12f, 0.30f, 1.f);
		D.PrismIntensity = 0.18f;
		D.GlowColor = FLinearColor(1.00f, 0.30f, 0.55f, 1.f);
		D.GlowOpacity = 0.24f;

		D.Light0_PosRadius = FLinearColor(0.00f, 0.00f, 0.55f, 0.f);
		D.Light0_Color = FLinearColor(1.00f, 0.30f, 0.55f, 0.f);
		D.Light0_Intensity = 0.22f;
		D.Light1_PosRadius = FLinearColor(0.80f, 0.18f, 0.28f, 0.f);
		D.Light1_Color = FLinearColor(1.00f, 0.65f, 0.30f, 0.f);
		D.Light1_Intensity = 0.040f;
		D.Light2_PosRadius = FLinearColor(0.16f, 0.80f, 0.30f, 0.f);
		D.Light2_Color = FLinearColor(0.80f, 0.20f, 0.50f, 0.f);
		D.Light2_Intensity = 0.028f;
		D.Light3_PosRadius = FLinearColor(0.52f, 0.52f, 0.13f, 0.f);
		D.Light3_Color = FLinearColor(1.00f, 0.50f, 0.70f, 0.f);
		D.Light3_Intensity = 0.018f;
		D.SceneLightsOpacityBoost = 0.35f;
		break;
	case EGlassTheme::Fire:
		D.TintColor = FLinearColor(0.60f, 0.20f, 0.05f, 1.f);
		D.PrismIntensity = 0.16f;
		D.GlowColor = FLinearColor(1.00f, 0.45f, 0.10f, 1.f);
		D.GlowOpacity = 0.26f;

		D.Light0_PosRadius = FLinearColor(0.00f, 0.00f, 0.55f, 0.f);
		D.Light0_Color = FLinearColor(1.00f, 0.45f, 0.10f, 0.f);
		D.Light0_Intensity = 0.24f;
		D.Light1_PosRadius = FLinearColor(0.82f, 0.15f, 0.30f, 0.f);
		D.Light1_Color = FLinearColor(1.00f, 0.70f, 0.20f, 0.f);
		D.Light1_Intensity = 0.055f;
		D.Light2_PosRadius = FLinearColor(0.16f, 0.80f, 0.30f, 0.f);
		D.Light2_Color = FLinearColor(1.00f, 0.25f, 0.05f, 0.f);
		D.Light2_Intensity = 0.030f;
		D.Light3_PosRadius = FLinearColor(0.50f, 0.52f, 0.12f, 0.f);
		D.Light3_Color = FLinearColor(1.00f, 0.80f, 0.20f, 0.f);
		D.Light3_Intensity = 0.018f;
		D.SceneLightsOpacityBoost = 0.35f;
		break;
	case EGlassTheme::Forest:
		D.TintColor = FLinearColor(0.06f, 0.50f, 0.25f, 1.f);
		D.PrismIntensity = 0.16f;
		D.GlowColor = FLinearColor(0.15f, 0.85f, 0.55f, 1.f);
		D.GlowOpacity = 0.24f;

		D.Light0_PosRadius = FLinearColor(0.00f, 0.00f, 0.55f, 0.f);
		D.Light0_Color = FLinearColor(0.15f, 0.85f, 0.55f, 0.f);
		D.Light0_Intensity = 0.20f;
		D.Light1_PosRadius = FLinearColor(0.80f, 0.18f, 0.28f, 0.f);
		D.Light1_Color = FLinearColor(0.60f, 1.00f, 0.30f, 0.f);
		D.Light1_Intensity = 0.038f;
		D.Light2_PosRadius = FLinearColor(0.16f, 0.80f, 0.30f, 0.f);
		D.Light2_Color = FLinearColor(0.10f, 0.60f, 0.40f, 0.f);
		D.Light2_Intensity = 0.025f;
		D.Light3_PosRadius = FLinearColor(0.52f, 0.52f, 0.12f, 0.f);
		D.Light3_Color = FLinearColor(0.30f, 1.00f, 0.55f, 0.f);
		D.Light3_Intensity = 0.016f;
		D.SceneLightsOpacityBoost = 0.35f;
		break;
	case EGlassTheme::White:
		D.TintColor = FLinearColor(0.80f, 0.85f, 1.00f, 1.f);
		D.PrismIntensity = 0.12f;
		D.GlowColor = FLinearColor(0.70f, 0.80f, 1.00f, 1.f);
		D.GlowOpacity = 0.22f;

		D.Light0_PosRadius = FLinearColor(0.00f, 0.00f, 0.55f, 0.f);
		D.Light0_Color = FLinearColor(0.70f, 0.80f, 1.00f, 0.f);
		D.Light0_Intensity = 0.18f;
		D.Light1_PosRadius = FLinearColor(0.80f, 0.18f, 0.28f, 0.f);
		D.Light1_Color = FLinearColor(1.00f, 1.00f, 0.90f, 0.f);
		D.Light1_Intensity = 0.030f;
		D.Light2_PosRadius = FLinearColor(0.16f, 0.80f, 0.30f, 0.f);
		D.Light2_Color = FLinearColor(0.70f, 0.85f, 1.00f, 0.f);
		D.Light2_Intensity = 0.022f;
		D.Light3_PosRadius = FLinearColor(0.52f, 0.52f, 0.12f, 0.f);
		D.Light3_Color = FLinearColor(0.80f, 0.80f, 1.00f, 0.f);
		D.Light3_Intensity = 0.014f;
		D.SceneLightsOpacityBoost = 0.35f;
		break;
	default: break;
	}
	return D;
}

