// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../CristalCubeStruct.h"
#include "CC_GlassWidget.generated.h"

class UImage;
class UBackgroundBlur;
class UMaterialInstanceDynamic;

/**
 * M_Glass_Panel / M_Glass_CornerGlow MI를 내부에서 생성·관리.
 * 상속받은 위젯에서 SetTheme() 또는 SetCustomTheme()을 호출해 외형 변경.
 */
UCLASS()
class CRISTALCUBE_API UCC_GlassWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:

	virtual void NativeConstruct() override;

    // ── 테마 API ─────────────────────────────────────────────

    /** 프리셋 테마 적용. 런타임 중 언제든 호출 가능 */
    UFUNCTION(BlueprintCallable, Category = "Glass")
    void SetTheme(EGlassTheme Theme);

    /** 수치 직접 지정 (Custom 테마 또는 런타임 Flash용) */
    UFUNCTION(BlueprintCallable, Category = "Glass")
    void SetCustomTheme(const FGlassThemeData& ThemeData);

    /** Blur Strength만 변경 (레벨업 연출 등) */
    UFUNCTION(BlueprintCallable, Category = "Glass")
    void SetBlurStrength(float Strength);

    // ── 기본 테마 에디터 설정 ─────────────────────────────────

    /** 에디터에서 기본 테마 지정 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Glass")
    EGlassTheme DefaultTheme = EGlassTheme::Ocean;

    /** DefaultTheme == Custom 일 때 사용 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Glass",
        meta = (EditCondition = "DefaultTheme == EGlassTheme::Custom"))
    FGlassThemeData CustomThemeData;

protected:
    // ── BindWidget — Blueprint에서 이름 일치 필수 ─────────────

    UPROPERTY(meta = (BindWidget))
    UBackgroundBlur* GlassBlur;         // L1

    UPROPERTY(meta = (BindWidget))
    UImage* GlassTintLayer;             // L2  M_Glass_Panel

    UPROPERTY(meta = (BindWidget))
    UImage* GlassBorderImage;           // L3

    // ── 머티리얼 에셋 레퍼런스 (에디터에서 할당) ─────────────

    UPROPERTY(EditAnywhere, Category = "Glass|Materials")
    UMaterialInterface* PanelMaterial;

    UPROPERTY(EditAnywhere, Category = "Glass|Materials")
    UMaterialInterface* BorderMaterial;

    /**
     * 서브클래스에서 빔 파라미터를 직접 구동할 수 있도록 노출.
     * PanelMI에 BeamOffset / BeamWidth / BeamSoftness / BeamIntensity 파라미터를 전달.
     * @param Offset    UV X 위치 (-0.2 = 화면 밖 왼쪽, 1.2 = 화면 밖 오른쪽)
     * @param Width     빛 띠 너비 (UV 단위, 0.13 권장)
     * @param Softness  가장자리 소프트니스 (0.04~0.10 권장)
     * @param Intensity 빛 강도 배율 (0 = 꺼짐, 1 = 기본, >1 = 강조)
     */
    UFUNCTION(BlueprintCallable, Category = "Glass|Beam")
    void SetBeamParams(float Offset, float Width, float Softness, float Intensity);

private:
    UPROPERTY()
    UMaterialInstanceDynamic* PanelMI;

    UPROPERTY()
    UMaterialInstanceDynamic* BorderMI;

    void InitMaterials();
    void ApplyThemeData(const FGlassThemeData& Data);

public:

    static FGlassThemeData GetThemeData(EGlassTheme Theme);
};
