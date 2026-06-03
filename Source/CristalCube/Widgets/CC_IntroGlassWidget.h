// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CC_GlassWidget.h"
#include "CC_IntroGlassWidget.generated.h"

/**
 * CC_GlassWidget 확장 — 등장 시 빛 스윕 + 내부 컨텐츠 떠오르는 연출.
 *
 * [Blueprint 설정]
 *  1. 이 클래스를 부모로 하는 WBP를 생성.
 *  2. GlassBlur / GlassTintLayer / GlassBorderImage 는 기존과 동일하게 바인딩.
 *  3. 내부 컨텐츠 루트 위젯의 이름을 "ContentContainer" 로 설정.
 *  4. NativeConstruct 시 PlayIntro()가 자동 호출됨.
 *
 * [머티리얼 파라미터 필요]
 *  M_Glass_Panel 에 BeamOffset / BeamWidth / BeamSoftness / BeamIntensity 파라미터가
 *  있어야 합니다. (위 CC_GlassWidget.cpp 수정과 세트)
 */
UCLASS()
class CRISTALCUBE_API UCC_IntroGlassWidget : public UCC_GlassWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    // ── 타이밍 ───────────────────────────────────────────────

/** 빛 스윕 전체 소요 시간 (초) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intro|Timing")
    float BeamDuration = 0.60f;

    /** 빛 시작 후 컨텐츠 페이드인이 시작되기까지의 지연 (초) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intro|Timing")
    float ContentFadeDelay = 0.40f;

    /** 컨텐츠 페이드인 소요 시간 (초) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intro|Timing")
    float ContentFadeDuration = 0.45f;

    // ── 컨텐츠 움직임 ─────────────────────────────────────────

    /** 떠오르기 시작 Y 오프셋 (px). 양수 = 아래에서 위로 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intro|Content")
    float ContentFloatOffset = 20.0f;

    // ── 빛 파라미터 ───────────────────────────────────────────

    /** 빛 시작 UV X (-0.2 권장, 화면 왼쪽 밖) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intro|Beam")
    float BeamStartOffset = -0.20f;

    /** 빛 종료 UV X (1.2 권장, 화면 오른쪽 밖) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intro|Beam")
    float BeamEndOffset = 1.20f;

    /** 빛 띠 너비 (UV 단위) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intro|Beam")
    float BeamWidth = 0.13f;

    /** 빛 띠 가장자리 소프트니스 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intro|Beam")
    float BeamSoftness = 0.06f;

    /** 빛 최대 강도 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intro|Beam")
    float BeamPeakIntensity = 1.0f;

    // ── 공개 API ─────────────────────────────────────────────

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intro")
    bool bAutoPlayOnConstruct = true;

    /**
     * 인트로 애니메이션을 처음부터 재실행.
     * 레벨업 연출 등 런타임 재호출 가능.
     */
    UFUNCTION(BlueprintCallable, Category = "Intro")
    void PlayIntro();
	
protected:
    /**
     * BP에서 이름이 정확히 "ContentContainer" 인 위젯을 바인딩.
     * 없어도 컴파일 에러 없음 (BindWidgetOptional).
     */
    UPROPERTY(meta = (BindWidgetOptional))
    UWidget* ContentContainer;

    float ElapsedTime = 0.f;
    bool  bAnimating = false;

    // 추후에 조절 가능하도록 변경
    static FORCEINLINE float EaseInOut(float t)
    {
        return t < 0.5f ? 2.f * t * t : -1.f + (4.f - 2.f * t) * t;
    }

    static FORCEINLINE float EaseOut(float t)
    {
        return 1.f - (1.f - t) * (1.f - t);
    }

};
