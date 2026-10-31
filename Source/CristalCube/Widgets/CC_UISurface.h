// CC_UISurface.h — Surface v2 (M_UI_GlassSurface 전용 경량 위젯)
// 위치 제안: Source/CristalCube/Widgets/CC_UISurface.h

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CC_UISurface.generated.h"

class UImage;
class UBackgroundBlur;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/** Surface 상태 (UI_ArtDirection §4 — Empty(점선)는 별도 MI로 처리) */
UENUM(BlueprintType)
enum class EUISurfaceState : uint8
{
	Rest,
	Hover,
	Pressed,
	Selected,
	Disabled
};

/** 상태별 덮어쓸 값. 음수 = MI(역할) 기본값 유지 */
USTRUCT(BlueprintType)
struct FUISurfaceStateLook
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface")
	float HotStr = -1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface")
	float SheenOpacity = -1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface")
	float RimBase = -1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface")
	float BodyStr = -1.f;

	/** 위젯 RenderOpacity (Disabled 등) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface", meta = (ClampMin = "0", ClampMax = "1"))
	float Opacity = 1.f;
};

/**
 * M_UI_GlassSurface v2 MID를 소유하는 틀 위젯.
 *  - 크기가 바뀔 때만 LayoutSize 전달 (머티리얼이 화면 px / 레이아웃 px 배율을 직접 계산)
 *  - OuterPadPx 만큼 SurfaceImage를 위젯 밖으로 확장 (Head가 테두리 밖으로 튀어나올 공간)
 *  - 상태(Hover/Selected …) → HotStr·SheenOpacity 등 부드럽게 전환
 *  - PlayRimIntro / PlayRimSweep / SetHotspotTarget : Head + Trail 이동 (이동 중에만 파라미터 갱신)
 *
 * WBP 구조 (이름 일치 필수):
 *   [Root] Overlay
 *     ├ SurfaceBlur  (BackgroundBlur, 선택 — Plate만, Fill/Fill, Corner 0)
 *     └ SurfaceImage (Image, Fill/Fill, Tint 1,1,1,1)
 *   └ (내용은 이 위젯을 감싸는 쪽 Overlay에 형제로 배치, 또는 Named Slot 추가)
 */
UCLASS()
class CRISTALCUBE_API UCC_UISurface : public UUserWidget
{
	GENERATED_BODY()

public:
	UCC_UISurface(const FObjectInitializer& ObjectInitializer);

	// ── 상태 ─────────────────────────────────────────────────
	UFUNCTION(BlueprintCallable, Category = "Surface")
	void SetSurfaceState(EUISurfaceState NewState, bool bInstant = false);

	UFUNCTION(BlueprintPure, Category = "Surface")
	EUISurfaceState GetSurfaceState() const { return CurrentState; }

	/** 원소 몸체 색 (Linear 값) — Well 용 */
	UFUNCTION(BlueprintCallable, Category = "Surface")
	void SetBodyColor(FLinearColor LinearColor);

	// ── 핫스팟 모션 (Head + Trail) ──────────────────────────
	/** 패널 열림: 둘레를 SweepFraction 만큼 미끄러져 기본 위치에 자리잡음 */
	UFUNCTION(BlueprintCallable, Category = "Surface|Motion")
	void PlayRimIntro(float Duration = 0.5f, float SweepFraction = 0.35f);

	/** 큐브 전환·레벨업: 테두리 한 바퀴 */
	UFUNCTION(BlueprintCallable, Category = "Surface|Motion")
	void PlayRimSweep(float Duration = 0.9f, bool bClockwise = true);

	/** 핫스팟을 둘레 비율 TargetAnchor(0~1)로 최단 경로 이동 */
	UFUNCTION(BlueprintCallable, Category = "Surface|Motion")
	void SetHotspotTarget(float TargetAnchor, float Duration = 0.25f);

	/** 위젯 로컬 좌표(px) → 그 점에서 가장 가까운 둘레 위치의 HotAnchor 값 */
	UFUNCTION(BlueprintPure, Category = "Surface|Motion")
	float LocalPosToAnchor(FVector2D LocalPos) const;

	UFUNCTION(BlueprintPure, Category = "Surface")
	UMaterialInstanceDynamic* GetSurfaceMID() const { return SurfaceMID; }

	// ── 에디터 설정 ──────────────────────────────────────────
	/** 역할 MI (MI_Surface_Plate / Card / Well / Pill …) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surface")
	TObjectPtr<UMaterialInterface> SurfaceMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surface")
	EUISurfaceState InitialState = EUISurfaceState::Rest;

	/** 상태별 덮어쓰기 값 (생성자에서 기본값 채움) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface")
	TMap<EUISurfaceState, FUISurfaceStateLook> StateLooks;

	/** 상태 전환 시간 (99% 도달) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface", meta = (ClampMin = "0"))
	float StateBlendTime = 0.12f;

	/** 마우스 진입/이탈 시 Hover/Rest 자동 전환 (Selected·Disabled일 땐 무시) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface")
	bool bAutoHoverState = false;

	/** 호버 중 핫스팟이 커서 쪽 둘레로 따라옴 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface|Motion")
	bool bHotspotFollowsCursor = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface|Motion")
	bool bPlayIntroOnConstruct = false;

	/** 꼬리 길이 = 최근 TrailTime 초 동안 이동한 둘레 거리 (최대 TrailMaxPx) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface|Motion", meta = (ClampMin = "0"))
	float TrailTime = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface|Motion", meta = (ClampMin = "0"))
	float TrailMaxPx = 160.f;

	/** 이동이 끝난 뒤 Head가 사라지는 시간 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface|Motion", meta = (ClampMin = "0"))
	float HeadFadeTime = 0.25f;

	/** Head 공간 자동 확보: OuterPadPx < 2 × HeadPx 이면 MID의 OuterPadPx를 올림 (목록 안처럼 클립되는 곳은 끄고 Head 안 씀) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface|Motion")
	bool bAutoOuterPad = true;

	/** LayoutSize를 보낼 때마다 Output Log에 출력 (테스트용 — C++가 크기를 넘기는지 확인) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface|Debug")
	bool bDebugLogLayout = false;

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	/** 디자이너는 Tick이 돌지 않음 → 그리는 순간 크기를 넘겨 디자이너에서도 줌 불변 */
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> SurfaceImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBackgroundBlur> SurfaceBlur;

private:
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SurfaceMID;

	void InitSurface();
	void CacheBaseValues();
	void ApplyOuterPad();
	void UpdateLayoutSize(const FVector2D& LocalSize);
	void ApplyStateTargets(bool bInstant);
	void StartAnchorMotion(float From, float To, float Duration, bool bEaseInOut);
	float ComputePerimeterPx(const FVector2D& ShapeSize) const;
	void SetScalar(FName Name, float Value, float& LastWritten);

	EUISurfaceState CurrentState = EUISurfaceState::Rest;

	// MI 기본값 (역할 값)
	float BaseHotStr = 0.f, BaseSheen = 0.f, BaseRimBase = 0.1f, BaseBodyStr = 0.f;
	float BaseHotAnchor = 0.f, BaseHotOffsetPx = 70.f, BaseHeadStr = 0.f;
	float RadiusPx = 20.f, RadiusMaxRatio = 0.35f, OuterPadPx = 0.f;

	// 상태 전환 (현재 → 목표)
	float CurHot = 0.f, CurSheen = 0.f, CurRim = 0.f, CurBody = 0.f, CurOpacity = 1.f;
	float TgtHot = 0.f, TgtSheen = 0.f, TgtRim = 0.f, TgtBody = 0.f, TgtOpacity = 1.f;
	bool bStateBlending = false;

	// 핫스팟 이동
	bool  bAnchorMoving = false;
	bool  bEaseInOut = false;
	float AnchorFrom = 0.f, AnchorTo = 0.f, AnchorT = 0.f, AnchorDuration = 0.f;
	float CurAnchor = 0.f, CurHead = 0.f, CurTrail = 0.f, CurTrailDir = 1.f;

	// 마지막으로 MID에 쓴 값 (중복 Set 방지)
	float W_Hot = -999.f, W_Sheen = -999.f, W_Rim = -999.f, W_Body = -999.f;
	float W_Anchor = -999.f, W_Head = -999.f, W_Trail = -999.f, W_TrailDir = -999.f;

	FVector2D LastLocalSize = FVector2D::ZeroVector;
	float LastHoverAnchor = -1.f;
};