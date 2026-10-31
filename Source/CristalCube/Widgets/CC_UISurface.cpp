// CC_UISurface.cpp — Surface v2
// 위치 제안: Source/CristalCube/Widgets/CC_UISurface.cpp

#include "CC_UISurface.h"
#include "Components/Image.h"
#include "Components/BackgroundBlur.h"
#include "Components/OverlaySlot.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace UISurfaceParam
{
	static const FName LayoutSize(TEXT("LayoutSize"));
	static const FName RadiusPx(TEXT("RadiusPx"));
	static const FName RadiusMaxRatio(TEXT("RadiusMaxRatio"));
	static const FName OuterPadPx(TEXT("OuterPadPx"));
	static const FName HeadPx(TEXT("HeadPx"));
	static const FName HotStr(TEXT("HotStr"));
	static const FName SheenOpacity(TEXT("SheenOpacity"));
	static const FName RimBase(TEXT("RimBase"));
	static const FName BodyStr(TEXT("BodyStr"));
	static const FName BodyColor(TEXT("BodyColor"));
	static const FName HotAnchor(TEXT("HotAnchor"));
	static const FName HotOffsetPx(TEXT("HotOffsetPx"));
	static const FName HeadStr(TEXT("HeadStr"));
	static const FName TrailPx(TEXT("TrailPx"));
	static const FName TrailDir(TEXT("TrailDir"));
}

UCC_UISurface::UCC_UISurface(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 상태 기본값 — 음수 = 역할 MI 값 유지
	StateLooks.Add(EUISurfaceState::Rest, FUISurfaceStateLook());

	FUISurfaceStateLook Hover;    Hover.HotStr = 1.f;
	StateLooks.Add(EUISurfaceState::Hover, Hover);

	FUISurfaceStateLook Pressed;  Pressed.HotStr = 0.7f;
	StateLooks.Add(EUISurfaceState::Pressed, Pressed);

	FUISurfaceStateLook Selected; Selected.HotStr = 1.f; Selected.SheenOpacity = 0.14f;
	StateLooks.Add(EUISurfaceState::Selected, Selected);

	FUISurfaceStateLook Disabled; Disabled.HotStr = 0.f; Disabled.SheenOpacity = 0.f; Disabled.Opacity = 0.45f;
	StateLooks.Add(EUISurfaceState::Disabled, Disabled);
}

// ─────────────────────────────────────────────────────────── 생명주기

void UCC_UISurface::NativePreConstruct()
{
	Super::NativePreConstruct();
	InitSurface();                       // 디자이너 미리보기
}

void UCC_UISurface::NativeConstruct()
{
	Super::NativeConstruct();
	InitSurface();
	if (bPlayIntroOnConstruct)
	{
		PlayRimIntro();
	}
}

void UCC_UISurface::InitSurface()
{
	if (!SurfaceImage || !SurfaceMaterial)
	{
		return;
	}

	if (!SurfaceMID || SurfaceMID->Parent != SurfaceMaterial)
	{
		SurfaceMID = UMaterialInstanceDynamic::Create(SurfaceMaterial, this);
		SurfaceImage->SetBrushFromMaterial(SurfaceMID);
	}
	SurfaceImage->SetColorAndOpacity(FLinearColor::White);   // Tint 1,1,1,1 필수 (Tint가 출력에 곱해짐)

	CacheBaseValues();
	ApplyOuterPad();

	CurAnchor = BaseHotAnchor;
	CurHead = BaseHeadStr;
	CurTrail = 0.f;
	W_Hot = W_Sheen = W_Rim = W_Body = W_Anchor = W_Head = W_Trail = W_TrailDir = -999.f;
	LastLocalSize = FVector2D::ZeroVector;   // 다음 Tick에서 LayoutSize 재전달

	CurrentState = InitialState;
	ApplyStateTargets(true);
}

void UCC_UISurface::CacheBaseValues()
{
	namespace P = UISurfaceParam;
	UMaterialInstanceDynamic* M = SurfaceMID;
	// K2_GetScalarParameterValue : MI → 부모 순으로 값을 찾음 (MI에서 안 덮은 값 = 부모 기본값)
	BaseHotStr = M->K2_GetScalarParameterValue(P::HotStr);
	BaseSheen = M->K2_GetScalarParameterValue(P::SheenOpacity);
	BaseRimBase = M->K2_GetScalarParameterValue(P::RimBase);
	BaseBodyStr = M->K2_GetScalarParameterValue(P::BodyStr);
	BaseHotAnchor = M->K2_GetScalarParameterValue(P::HotAnchor);
	BaseHotOffsetPx = M->K2_GetScalarParameterValue(P::HotOffsetPx);
	BaseHeadStr = M->K2_GetScalarParameterValue(P::HeadStr);
	RadiusPx = M->K2_GetScalarParameterValue(P::RadiusPx);
	RadiusMaxRatio = M->K2_GetScalarParameterValue(P::RadiusMaxRatio);
	OuterPadPx = M->K2_GetScalarParameterValue(P::OuterPadPx);

	// Head가 테두리 밖으로 나올 공간 자동 확보 — MI에 OuterPadPx를 안 넣었거나 작으면 2 × HeadPx로 올림
	// (Pad가 0이면 Head 바깥 절반이 위젯 사각 경계에서 잘려 모서리에서 '사각형을 타는' 것처럼 보임)
	if (bAutoOuterPad)
	{
		const float Need = FMath::CeilToFloat(2.f * M->K2_GetScalarParameterValue(P::HeadPx));
		if (OuterPadPx < Need)
		{
			OuterPadPx = Need;
			M->SetScalarParameterValue(P::OuterPadPx, OuterPadPx);
		}
	}
}

void UCC_UISurface::ApplyOuterPad()
{
	// Image를 위젯 밖으로 Pad만큼 확장 → 틀(도형)은 위젯 크기 그대로, Head가 잘리지 않음
	if (UOverlaySlot* OS = Cast<UOverlaySlot>(SurfaceImage->Slot))
	{
		OS->SetHorizontalAlignment(HAlign_Fill);
		OS->SetVerticalAlignment(VAlign_Fill);
		OS->SetPadding(FMargin(-OuterPadPx));
	}
}

void UCC_UISurface::UpdateLayoutSize(const FVector2D& LocalSize)
{
	if (!SurfaceMID || LocalSize.X < 1.f || LocalSize.Y < 1.f)
	{
		return;
	}
	if (LocalSize.Equals(LastLocalSize, 0.25f))
	{
		return;                                   // 크기 변경 시에만 전달
	}
	LastLocalSize = LocalSize;
	const float Pad2 = 2.f * OuterPadPx;
	SurfaceMID->SetVectorParameterValue(UISurfaceParam::LayoutSize,
		FLinearColor(LocalSize.X + Pad2, LocalSize.Y + Pad2, 0.f, 0.f));

	if (bDebugLogLayout)
	{
		UE_LOG(LogTemp, Log, TEXT("[UISurface] %s  LayoutSize = %.1f x %.1f  (Pad %.1f)  DesignTime=%d"),
			*GetName(), LocalSize.X + Pad2, LocalSize.Y + Pad2, OuterPadPx, IsDesignTime() ? 1 : 0);
	}
}

void UCC_UISurface::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!SurfaceMID)
	{
		return;
	}

	// 1) 레이아웃 크기 (Slate 단위 — 줌·DPI·RenderTransform과 무관)
	UpdateLayoutSize(MyGeometry.GetLocalSize());

	// 2) 상태 전환
	if (bStateBlending)
	{
		const float A = (StateBlendTime <= KINDA_SMALL_NUMBER) ? 1.f
			: 1.f - FMath::Exp(-InDeltaTime * 4.6f / StateBlendTime);
		CurHot = FMath::Lerp(CurHot, TgtHot, A);
		CurSheen = FMath::Lerp(CurSheen, TgtSheen, A);
		CurRim = FMath::Lerp(CurRim, TgtRim, A);
		CurBody = FMath::Lerp(CurBody, TgtBody, A);
		CurOpacity = FMath::Lerp(CurOpacity, TgtOpacity, A);

		const bool bDone =
			FMath::IsNearlyEqual(CurHot, TgtHot, 0.002f) && FMath::IsNearlyEqual(CurSheen, TgtSheen, 0.002f) &&
			FMath::IsNearlyEqual(CurRim, TgtRim, 0.002f) && FMath::IsNearlyEqual(CurBody, TgtBody, 0.002f) &&
			FMath::IsNearlyEqual(CurOpacity, TgtOpacity, 0.002f);
		if (bDone)
		{
			CurHot = TgtHot; CurSheen = TgtSheen; CurRim = TgtRim; CurBody = TgtBody; CurOpacity = TgtOpacity;
			bStateBlending = false;
		}
		SetScalar(UISurfaceParam::HotStr, CurHot, W_Hot);
		SetScalar(UISurfaceParam::SheenOpacity, CurSheen, W_Sheen);
		SetScalar(UISurfaceParam::RimBase, CurRim, W_Rim);
		SetScalar(UISurfaceParam::BodyStr, CurBody, W_Body);
		SetRenderOpacity(CurOpacity);
	}

	// 3) 핫스팟 이동 (Head + Trail)
	const bool bTrailing = CurTrail > 0.5f;
	const bool bHeadFading = !FMath::IsNearlyEqual(CurHead, BaseHeadStr, 0.002f);
	if (bAnchorMoving || bTrailing || bHeadFading)
	{
		const float PrevAnchor = CurAnchor;
		if (bAnchorMoving)
		{
			AnchorT = (AnchorDuration > KINDA_SMALL_NUMBER) ? FMath::Min(AnchorT + InDeltaTime / AnchorDuration, 1.f) : 1.f;
			const float E = bEaseInOut ? FMath::InterpEaseInOut(0.f, 1.f, AnchorT, 2.f)
				: 1.f - FMath::Pow(1.f - AnchorT, 3.f);   // EaseOutCubic
			CurAnchor = FMath::Lerp(AnchorFrom, AnchorTo, E);
			CurHead = FMath::FInterpTo(CurHead, 1.f, InDeltaTime, 30.f);
			if (AnchorT >= 1.f)
			{
				bAnchorMoving = false;
				CurAnchor = AnchorTo - FMath::FloorToFloat(AnchorTo);         // 0~1로 정리
			}
		}
		else
		{
			const float FadeA = (HeadFadeTime <= KINDA_SMALL_NUMBER) ? 1.f
				: 1.f - FMath::Exp(-InDeltaTime * 4.6f / HeadFadeTime);
			CurHead = FMath::Lerp(CurHead, BaseHeadStr, FadeA);
			if (FMath::IsNearlyEqual(CurHead, BaseHeadStr, 0.002f)) { CurHead = BaseHeadStr; }
		}

		// 꼬리 = 최근 TrailTime 초 동안 이동한 둘레 거리
		float dA = CurAnchor - PrevAnchor;
		dA -= FMath::RoundToFloat(dA);
		const float PerimPx = ComputePerimeterPx(LastLocalSize);
		const float SpeedPx = (InDeltaTime > KINDA_SMALL_NUMBER) ? FMath::Abs(dA) * PerimPx / InDeltaTime : 0.f;
		const float WantTrail = bAnchorMoving ? FMath::Min(SpeedPx * TrailTime, TrailMaxPx) : 0.f;
		CurTrail = (WantTrail > CurTrail) ? WantTrail : FMath::FInterpTo(CurTrail, WantTrail, InDeltaTime, 10.f);
		if (FMath::Abs(dA) > KINDA_SMALL_NUMBER)
		{
			CurTrailDir = (dA > 0.f) ? 1.f : -1.f;
		}

		SetScalar(UISurfaceParam::HotAnchor, CurAnchor, W_Anchor);
		SetScalar(UISurfaceParam::HeadStr, CurHead, W_Head);
		SetScalar(UISurfaceParam::TrailPx, CurTrail > 0.5f ? CurTrail : 0.f, W_Trail);
		SetScalar(UISurfaceParam::TrailDir, CurTrailDir, W_TrailDir);
	}
}

int32 UCC_UISurface::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	if (IsDesignTime())
	{
		// 디자이너 전용: 크기가 바뀐 경우에만 MID 갱신 (UpdateLayoutSize 내부 비교) — 다음 프레임부터 반영
		const_cast<UCC_UISurface*>(this)->UpdateLayoutSize(AllottedGeometry.GetLocalSize());
	}
	return Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
}

// ─────────────────────────────────────────────────────────── 상태

void UCC_UISurface::SetSurfaceState(EUISurfaceState NewState, bool bInstant)
{
	CurrentState = NewState;
	ApplyStateTargets(bInstant);
}

void UCC_UISurface::ApplyStateTargets(bool bInstant)
{
	const FUISurfaceStateLook* Look = StateLooks.Find(CurrentState);
	const FUISurfaceStateLook Def;
	const FUISurfaceStateLook& L = Look ? *Look : Def;

	TgtHot = (L.HotStr >= 0.f) ? L.HotStr : BaseHotStr;
	TgtSheen = (L.SheenOpacity >= 0.f) ? L.SheenOpacity : BaseSheen;
	TgtRim = (L.RimBase >= 0.f) ? L.RimBase : BaseRimBase;
	TgtBody = (L.BodyStr >= 0.f) ? L.BodyStr : BaseBodyStr;
	TgtOpacity = L.Opacity;

	if (bInstant || !SurfaceMID)
	{
		CurHot = TgtHot; CurSheen = TgtSheen; CurRim = TgtRim; CurBody = TgtBody; CurOpacity = TgtOpacity;
		bStateBlending = false;
		if (SurfaceMID)
		{
			SetScalar(UISurfaceParam::HotStr, CurHot, W_Hot);
			SetScalar(UISurfaceParam::SheenOpacity, CurSheen, W_Sheen);
			SetScalar(UISurfaceParam::RimBase, CurRim, W_Rim);
			SetScalar(UISurfaceParam::BodyStr, CurBody, W_Body);
		}
		SetRenderOpacity(CurOpacity);
	}
	else
	{
		bStateBlending = true;
	}
}

void UCC_UISurface::SetBodyColor(FLinearColor LinearColor)
{
	if (SurfaceMID)
	{
		SurfaceMID->SetVectorParameterValue(UISurfaceParam::BodyColor, LinearColor);
	}
}

// ─────────────────────────────────────────────────────────── 핫스팟 모션

void UCC_UISurface::StartAnchorMotion(float From, float To, float Duration, bool bInOut)
{
	AnchorFrom = From;
	AnchorTo = To;
	AnchorT = 0.f;
	AnchorDuration = FMath::Max(Duration, 0.f);
	bEaseInOut = bInOut;
	bAnchorMoving = true;
	CurTrailDir = (To >= From) ? 1.f : -1.f;
}

void UCC_UISurface::PlayRimIntro(float Duration, float SweepFraction)
{
	CurAnchor = BaseHotAnchor - SweepFraction;
	StartAnchorMotion(CurAnchor, BaseHotAnchor, Duration, false);
}

void UCC_UISurface::PlayRimSweep(float Duration, bool bClockwise)
{
	const float Dir = bClockwise ? 1.f : -1.f;
	StartAnchorMotion(CurAnchor, CurAnchor + Dir, Duration, true);
}

void UCC_UISurface::SetHotspotTarget(float TargetAnchor, float Duration)
{
	float Delta = TargetAnchor - CurAnchor;
	Delta -= FMath::RoundToFloat(Delta);                 // 최단 경로 (-0.5 ~ 0.5)
	if (FMath::Abs(Delta) < 0.001f)
	{
		return;
	}
	StartAnchorMotion(CurAnchor, CurAnchor + Delta, Duration, false);
}

float UCC_UISurface::ComputePerimeterPx(const FVector2D& ShapeSize) const
{
	const float MinSide = FMath::Max(FMath::Min(ShapeSize.X, ShapeSize.Y), 1.f);
	const float R = FMath::Min(FMath::Min(RadiusPx, RadiusMaxRatio * MinSide), 0.5f * MinSide);
	const float Bx = FMath::Max(ShapeSize.X - 2.f * R, 0.f);
	const float By = FMath::Max(ShapeSize.Y - 2.f * R, 0.f);
	return 2.f * Bx + 2.f * By + 2.f * UE_PI * R;
}

float UCC_UISurface::LocalPosToAnchor(FVector2D LocalPos) const
{
	// 셰이더의 둘레 좌표 s 계산과 동일 (틀 크기 = 위젯 크기, 중심 원점, y 아래 +)
	const FVector2D Size = LastLocalSize.X > 1.f ? LastLocalSize : GetCachedGeometry().GetLocalSize();
	if (Size.X < 1.f || Size.Y < 1.f)
	{
		return BaseHotAnchor;
	}
	const float MinSide = FMath::Min(Size.X, Size.Y);
	const float r = FMath::Min(FMath::Min(RadiusPx, RadiusMaxRatio * MinSide), 0.5f * MinSide);
	const FVector2D p = LocalPos - Size * 0.5f;
	const FVector2D b = Size * 0.5f - FVector2D(r, r);
	const FVector2D q(FMath::Abs(p.X) - b.X, FMath::Abs(p.Y) - b.Y);
	const float Lx = 2.f * b.X, Ly = 2.f * b.Y, Arc = UE_HALF_PI * r;
	const float P = 2.f * Lx + 2.f * Ly + 4.f * Arc;

	float s;
	if (q.X > 0.f && q.Y > 0.f)
	{
		if (p.X >= 0.f && p.Y < 0.f) { const FVector2D c = p - FVector2D(b.X, -b.Y); s = Lx + FMath::Atan2(c.X, -c.Y) * r; }
		else if (p.X >= 0.f) { const FVector2D c = p - FVector2D(b.X, b.Y); s = Lx + Arc + Ly + FMath::Atan2(c.Y, c.X) * r; }
		else if (p.Y >= 0.f) { const FVector2D c = p - FVector2D(-b.X, b.Y); s = 2.f * Lx + 2.f * Arc + Ly + FMath::Atan2(-c.X, c.Y) * r; }
		else { const FVector2D c = p - FVector2D(-b.X, -b.Y); s = 2.f * Lx + 3.f * Arc + 2.f * Ly + FMath::Atan2(-c.Y, -c.X) * r; }
	}
	else if (q.Y > q.X)
	{
		s = (p.Y < 0.f) ? (p.X + b.X) : (Lx + 2.f * Arc + Ly + b.X - p.X);
	}
	else
	{
		s = (p.X >= 0.f) ? (Lx + Arc + p.Y + b.Y) : (2.f * Lx + 3.f * Arc + Ly + b.Y - p.Y);
	}

	// hpos = Anchor·P + Offset → Anchor = (s − Offset) / P
	float A = (s - BaseHotOffsetPx) / FMath::Max(P, 1.f);
	return A - FMath::FloorToFloat(A);
}

// ─────────────────────────────────────────────────────────── 입력

void UCC_UISurface::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	if (bAutoHoverState && CurrentState == EUISurfaceState::Rest)
	{
		SetSurfaceState(EUISurfaceState::Hover);
	}
}

void UCC_UISurface::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	if (bAutoHoverState && CurrentState == EUISurfaceState::Hover)
	{
		SetSurfaceState(EUISurfaceState::Rest);
	}
	if (bHotspotFollowsCursor)
	{
		LastHoverAnchor = -1.f;
		SetHotspotTarget(BaseHotAnchor, 0.35f);          // 기본 위치로 복귀
	}
}

FReply UCC_UISurface::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	FReply Reply = Super::NativeOnMouseMove(InGeometry, InMouseEvent);
	if (bHotspotFollowsCursor)
	{
		const FVector2D Local = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
		const float A = LocalPosToAnchor(Local);
		float D = A - LastHoverAnchor;
		D -= FMath::RoundToFloat(D);
		if (LastHoverAnchor < 0.f || FMath::Abs(D) > 0.01f)   // 1% 이상 움직였을 때만
		{
			LastHoverAnchor = A;
			SetHotspotTarget(A, 0.18f);
		}
	}
	return Reply;
}

// ─────────────────────────────────────────────────────────── 유틸

void UCC_UISurface::SetScalar(FName Name, float Value, float& LastWritten)
{
	if (SurfaceMID && !FMath::IsNearlyEqual(Value, LastWritten, 1e-4f))
	{
		SurfaceMID->SetScalarParameterValue(Name, Value);
		LastWritten = Value;
	}
}