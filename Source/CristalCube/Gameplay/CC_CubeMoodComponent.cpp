// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_CubeMoodComponent.h"
#include "Engine/DirectionalLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/PostProcessVolume.h"
#include "Kismet/GameplayStatics.h"

// Sets default values for this component's properties
UCC_CubeMoodComponent::UCC_CubeMoodComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
        // 기본 무드 3개 (CubeType 0/1/2) 미리 채워두기
    FCubeMoodSettings Meadow;
    Meadow.SunColor = FLinearColor(1.0f, 0.95f, 0.8f);
    Meadow.SunIntensity = 80000.f;
    Meadow.SunRotation = FRotator(-50.f, 0.f, 0.f);
    Meadow.BloomIntensity = 0.675f;
    Meadow.ColorSaturation = 1.1f;
    Meadow.FogDensity = 0.01f;
    Meadow.FogColor = FLinearColor(0.6f, 0.7f, 0.8f);
    Meadow.bVolumetricFog = false;

    FCubeMoodSettings Desert;
    Desert.SunColor = FLinearColor(1.0f, 0.85f, 0.6f);
    Desert.SunIntensity = 111000.f;
    Desert.SunRotation = FRotator(-65.f, 0.f, 0.f);
    Desert.BloomIntensity = 0.4f;
    Desert.ColorSaturation = 1.0f;
    Desert.FogDensity = 0.002f;
    Desert.FogColor = FLinearColor(0.9f, 0.8f, 0.6f);
    Desert.bVolumetricFog = false;

    FCubeMoodSettings Volcanic;
    Volcanic.SunColor = FLinearColor(1.0f, 0.35f, 0.1f);
    Volcanic.SunIntensity = 40000.f;
    Volcanic.SunRotation = FRotator(-30.f, 0.f, 0.f);
    Volcanic.BloomIntensity = 1.2f;
    Volcanic.ColorSaturation = 0.9f;
    Volcanic.FogDensity = 0.05f;
    Volcanic.FogColor = FLinearColor(0.8f, 0.3f, 0.1f);
    Volcanic.bVolumetricFog = true;

    MoodSettings.Add(Meadow);
    MoodSettings.Add(Desert);
    MoodSettings.Add(Volcanic);
}


// Called when the game starts
void UCC_CubeMoodComponent::BeginPlay()
{
	Super::BeginPlay();
    
	// ...
    CacheLightActors();
}


// Called every frame
void UCC_CubeMoodComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

// ============================================================
//  CacheLightActors
// ============================================================

void UCC_CubeMoodComponent::CacheLightActors()
{
    UWorld* World = GetWorld();
    if (!World) return;

    // Directional Light
    TArray<AActor*> Found;
    UGameplayStatics::GetAllActorsOfClass(World, ADirectionalLight::StaticClass(), Found);
    if (Found.Num() > 0)
    {
        CachedSunLight = Cast<ADirectionalLight>(Found[0]);
        UE_LOG(LogTemp, Log, TEXT("[MoodComponent] Cached DirectionalLight: %s"),
            *CachedSunLight->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[MoodComponent] DirectionalLight not found in level!"));
    }

    // Exponential Height Fog
    Found.Empty();
    UGameplayStatics::GetAllActorsOfClass(World, AExponentialHeightFog::StaticClass(), Found);
    if (Found.Num() > 0)
    {
        CachedFog = Cast<AExponentialHeightFog>(Found[0]);
    }

    // Post Process Volume
    Found.Empty();
    UGameplayStatics::GetAllActorsOfClass(World, APostProcessVolume::StaticClass(), Found);
    if (Found.Num() > 0)
    {
        CachedPPV = Cast<APostProcessVolume>(Found[0]);
    }
}

// ============================================================
//  ApplyMood
// ============================================================

void UCC_CubeMoodComponent::ApplyMood(int32 CubeType)
{
    if (!MoodSettings.IsValidIndex(CubeType))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[MoodComponent] CubeType %d has no MoodSettings entry. Add it in Blueprint."),
            CubeType);
        return;
    }

    const FCubeMoodSettings& Settings = MoodSettings[CubeType];

    ApplyToDirectionalLight(Settings);
    ApplyToFog(Settings);
    ApplyToPostProcess(Settings);

    UE_LOG(LogTemp, Log, TEXT("[MoodComponent] Applied mood for CubeType %d"), CubeType);
}

// ============================================================
//  Private: Apply to each actor
// ============================================================

void UCC_CubeMoodComponent::ApplyToDirectionalLight(const FCubeMoodSettings& Settings)
{
    if (!CachedSunLight) return;

    UDirectionalLightComponent* LightComp = Cast<UDirectionalLightComponent>(CachedSunLight->GetLightComponent());
    if (!LightComp) return;

    LightComp->SetLightColor(Settings.SunColor);
    LightComp->SetIntensity(Settings.SunIntensity);
    CachedSunLight->SetActorRotation(Settings.SunRotation);
}

void UCC_CubeMoodComponent::ApplyToFog(const FCubeMoodSettings& Settings)
{
    if (!CachedFog) return;

    UExponentialHeightFogComponent* FogComp = CachedFog->GetComponent();
    if (!FogComp) return;

    FogComp->SetFogDensity(Settings.FogDensity);
    FogComp->SetFogInscatteringColor(Settings.FogColor);
    FogComp->SetVolumetricFog(Settings.bVolumetricFog);
}

void UCC_CubeMoodComponent::ApplyToPostProcess(const FCubeMoodSettings& Settings)
{
    if (!CachedPPV) return;

    // 각 항목은 bOverride_ 플래그를 함께 켜야 적용됨
    FPostProcessSettings& S = CachedPPV->Settings;

    S.bOverride_BloomIntensity = true;
    S.BloomIntensity = Settings.BloomIntensity;

    S.bOverride_ColorSaturation = true;
    S.ColorSaturation = FVector4(
        Settings.ColorSaturation,
        Settings.ColorSaturation,
        Settings.ColorSaturation,
        1.f);

    S.bOverride_ColorContrast = true;
    S.ColorContrast = FVector4(
        Settings.Contrast,
        Settings.Contrast,
        Settings.Contrast,
        1.f);

    S.bOverride_ColorGamma = true;
    S.ColorGamma = FVector4(
        Settings.Gamma,
        Settings.Gamma,
        Settings.Gamma,
        1.f);
}