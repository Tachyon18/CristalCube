// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_IntroGlassWidget.h"

void UCC_IntroGlassWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ContentContainer)
	{
		ContentContainer->SetRenderOpacity(0.f);
		ContentContainer->SetRenderTranslation(FVector2D(0.f, ContentFloatOffset));
	}

    if(bAutoPlayOnConstruct)
    {
        PlayIntro();
    }
}

void UCC_IntroGlassWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

    if (!bAnimating) return;

    ElapsedTime += InDeltaTime;

    // ── Phase 1: 빛 스윕 ─────────────────────────────────────────────────────
    {
        const float BeamT = FMath::Clamp(ElapsedTime / BeamDuration, 0.f, 1.f);
        const float EasedT = EaseInOut(BeamT);
        const float CurOffset = FMath::Lerp(BeamStartOffset, BeamEndOffset, EasedT);

        // 강도: Sin 커브로 중간이 가장 밝고 양 끝은 0
        const float CurIntensity = BeamPeakIntensity * FMath::Sin(BeamT * PI);

        SetBeamParams(CurOffset, BeamWidth, BeamSoftness, CurIntensity);
    }

    // ── Phase 2: 컨텐츠 페이드인 + 떠오르기 ────────────────────────────────
    if (ContentContainer)
    {
        const float FadeElapsed = ElapsedTime - ContentFadeDelay;

        if (FadeElapsed > 0.f)
        {
            const float FadeT = FMath::Clamp(FadeElapsed / ContentFadeDuration, 0.f, 1.f);
            const float EasedFade = EaseOut(FadeT);

            ContentContainer->SetRenderOpacity(EasedFade);
            ContentContainer->SetRenderTranslation(
                FVector2D(0.f, FMath::Lerp(ContentFloatOffset, 0.f, EasedFade))
            );
        }
    }

    // ── 종료 ─────────────────────────────────────────────────────────────────
    const float TotalDuration = FMath::Max(BeamDuration,
        ContentFadeDelay + ContentFadeDuration);

    if (ElapsedTime >= TotalDuration + 0.05f)   // 0.05 여유
    {
        bAnimating = false;

        // 빔 완전 제거 (오프셋을 화면 밖으로)
        SetBeamParams(999.f, BeamWidth, BeamSoftness, 0.f);

        // 컨텐츠 최종 상태 확정
        if (ContentContainer)
        {
            ContentContainer->SetRenderOpacity(1.f);
            ContentContainer->SetRenderTranslation(FVector2D::ZeroVector);
        }
    }
}

void UCC_IntroGlassWidget::PlayIntro()
{
	ElapsedTime = 0.f;
	bAnimating = true;

	SetBeamParams(BeamStartOffset, BeamWidth, BeamSoftness, 0.f);

	if (ContentContainer)
	{
		ContentContainer->SetRenderOpacity(0.f);
		ContentContainer->SetRenderTranslation(FVector2D(0.f, ContentFloatOffset));
	}
}
