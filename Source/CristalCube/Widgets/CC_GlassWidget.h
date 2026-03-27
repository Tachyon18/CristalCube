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
    UImage* GlassCornerGlow;            // L3  M_Glass_CornerGlow

    UPROPERTY(meta = (BindWidget))
    UImage* GlassBorderImage;           // L4

    // ── 머티리얼 에셋 레퍼런스 (에디터에서 할당) ─────────────

    UPROPERTY(EditAnywhere, Category = "Glass|Materials")
    UMaterialInterface* PanelMaterial;

    UPROPERTY(EditAnywhere, Category = "Glass|Materials")
    UMaterialInterface* CornerGlowMaterial;

    UPROPERTY(EditAnywhere, Category = "Glass|Materials")
    UMaterialInterface* BorderMaterial;

private:
    UPROPERTY()
    UMaterialInstanceDynamic* PanelMI;

    UPROPERTY()
    UMaterialInstanceDynamic* CornerGlowMI;

    UPROPERTY()
    UMaterialInstanceDynamic* BorderMI;

    void InitMaterials();
    void ApplyThemeData(const FGlassThemeData& Data);

public:

    static FGlassThemeData GetThemeData(EGlassTheme Theme);
};
