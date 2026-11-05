// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../CristalCubeStruct.h"   // ESkillElementType
#include "CC_UISurface.generated.h"

class UImage;
class UBackgroundBlur;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UCC_UIStyleData;

/** Surface 상태 (UI_ArtDirection §4) — 새 값은 항상 끝에 추가 */
UENUM(BlueprintType)
enum class EUISurfaceState : uint8
{
	Rest,
	Hover,
	Pressed,
	Selected,
	Disabled,
	Empty        // rev.5 — 점선 테두리 (빈 슬롯)
};

/** Surface 역할 — Token(DA_UIStyle)의 RoleMaterials에서 MI를 고름. Custom = SurfaceMaterial 직접 지정 */
UENUM(BlueprintType)
enum class EUISurfaceRole : uint8
{
	Custom,
	Plate,
	Card,
	Well,
	Track,
	Pill,
	Filled
};

/** 반경 Token (golden-b1-b2-provide §1-3). FromMaterial = MI 값 유지 */
UENUM(BlueprintType)
enum class EUIRadius : uint8
{
	FromMaterial,
	Plate,
	Section,
	Entry,
	SubCard,
	Button,
	IconButton,
	WellL,
	WellM,
	WellS,
	Full
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

	/** (rev.5) 원소 몸체 가산 — Element가 지정된 Surface에만 더함 (Selected 카드 원소색 10%) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface")
	float BodyStrAdd = 0.f;

	/** (rev.5) 0 = 일반 · 1 = Empty(점선, 유리 효과 소거) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface", meta = (ClampMin = "0", ClampMax = "1"))
	float EmptyMix = 0.f;

	/** 위젯 RenderOpacity (Disabled 등) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface", meta = (ClampMin = "0", ClampMax = "1"))
	float Opacity = 1.f;
};

/**
 * M_UI_GlassSurface MID를 소유하는 틀 위젯.
 *  - 크기가 바뀔 때만 LayoutSize 전달 (머티리얼이 화면 px / 레이아웃 px 배율을 직접 계산)
 *  - OuterPadPx 만큼 SurfaceImage를 위젯 밖으로 확장 (Head가 테두리 밖으로 튀어나올 공간)
 *  - 상태(Hover/Selected/Empty …) → HotStr·SheenOpacity·EmptyMix 등 부드럽게 전환
 *  - PlayRimIntro / PlayRimSweep / SetHotspotTarget : Head + Trail 이동 (이동 중에만 파라미터 갱신)
 *  - (rev.5) Token 연결: SurfaceRole → MI, RadiusToken → 반경, Element → 원소색, StateLooks·모션 시간
 *
 * WBP 구조 (이름 일치 필수):
 *   [Root] Overlay
 *     ├ SurfaceBlur  (BackgroundBlur, 선택 — Plate만, Fill/Fill, Corner 0)
 *     └ SurfaceImage (Image, Fill/Fill, Tint 1,1,1,1)
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

	/** 원소 몸체 색 (Linear 값) 직접 지정 — 보통은 SetElement 사용 */
	UFUNCTION(BlueprintCallable, Category = "Surface")
	void SetBodyColor(FLinearColor LinearColor);

	/** (rev.5) Token 원소색으로 BodyColor 지정. None = MI 값으로 되돌림 */
	UFUNCTION(BlueprintCallable, Category = "Surface")
	void SetElement(ESkillElementType InElement);

	/** (rev.5) Token(DA_UIStyle)을 다시 읽어 MID·상태·모션 재적용 */
	UFUNCTION(BlueprintCallable, Category = "Surface")
	void RefreshFromStyle();

	// ── 핫스팟 모션 (Head + Trail) — 인자 −1 = Token 값 ──────
	/** 패널 열림: 둘레를 SweepFraction 만큼 미끄러져 기본 위치에 자리잡음 */
	UFUNCTION(BlueprintCallable, Category = "Surface|Motion")
	void PlayRimIntro(float Duration = -1.f, float SweepFraction = -1.f);

	/** 큐브 전환·레벨업: 테두리 한 바퀴 */
	UFUNCTION(BlueprintCallable, Category = "Surface|Motion")
	void PlayRimSweep(float Duration = -1.f, bool bClockwise = true);

	/** 핫스팟을 둘레 비율 TargetAnchor(0~1)로 최단 경로 이동 */
	UFUNCTION(BlueprintCallable, Category = "Surface|Motion")
	void SetHotspotTarget(float TargetAnchor, float Duration = -1.f);

	/** 위젯 로컬 좌표(px) → 그 점에서 가장 가까운 둘레 위치의 HotAnchor 값 */
	UFUNCTION(BlueprintPure, Category = "Surface|Motion")
	float LocalPosToAnchor(FVector2D LocalPos) const;

	UFUNCTION(BlueprintPure, Category = "Surface")
	UMaterialInstanceDynamic* GetSurfaceMID() const { return SurfaceMID; }

	// ── 에디터 설정 ──────────────────────────────────────────
	/** 역할 MI 직접 지정 (비우면 SurfaceRole의 Token MI) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surface")
	TObjectPtr<UMaterialInterface> SurfaceMaterial;

	/** (rev.5) 역할 — SurfaceMaterial이 비어 있을 때 Token에서 MI를 고름 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surface|Style")
	EUISurfaceRole SurfaceRole = EUISurfaceRole::Custom;

	/** (rev.5) 반경 Token — FromMaterial = MI 값 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surface|Style")
	EUIRadius RadiusToken = EUIRadius::FromMaterial;

	/** (rev.5) 원소 — None = MI의 BodyColor 유지 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surface|Style")
	ESkillElementType Element = ESkillElementType::None;

	/** (rev.5) 비우면 Project Settings › CristalCube UI › Default Style */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surface|Style")
	TObjectPtr<UCC_UIStyleData> StyleOverride;

	/** (rev.5) Token의 StateLooks 사용 (끄면 아래 StateLooks를 인스턴스 값으로) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surface|Style")
	bool bUseStyleStates = true;

	/** (rev.5) Token의 모션 시간(StateBlend·Trail·HeadFade·Hotspot) 사용 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surface|Style")
	bool bUseStyleMotion = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surface")
	EUISurfaceState InitialState = EUISurfaceState::Rest;

	/** 상태별 덮어쓰기 값 (생성자 기본값 — bUseStyleStates ✔면 Token 값으로 교체됨) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface")
	TMap<EUISurfaceState, FUISurfaceStateLook> StateLooks;

	/** 상태 전환 시간 (99% 도달) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface", meta = (ClampMin = "0"))
	float StateBlendTime = 0.12f;

	/** 마우스 진입/이탈 시 Hover/Rest 자동 전환 (Rest일 때만 — Selected·Disabled·Empty는 무시) */
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

	/** 현재 적용 중인 Token (StyleOverride 또는 전역) */
	UPROPERTY(Transient)
	TObjectPtr<UCC_UIStyleData> ActiveStyle;

	void InitSurface();
	void ApplyStyleToMID();
	void CacheBaseValues();
	void ApplyOuterPad();
	void UpdateLayoutSize(const FVector2D& LocalSize);
	void ApplyStateTargets(bool bInstant);
	void StartAnchorMotion(float From, float To, float Duration, bool bEaseInOut);
	float ComputePerimeterPx(const FVector2D& ShapeSize) const;
	void SetScalar(FName Name, float Value, float& LastWritten);
	bool UseStyleMotion() const;

	EUISurfaceState CurrentState = EUISurfaceState::Rest;

	// MI 기본값 (역할 값 — Token 반경·원소 적용 후)
	float BaseHotStr = 0.f, BaseSheen = 0.f, BaseRimBase = 0.1f, BaseBodyStr = 0.f;
	float BaseHotAnchor = 0.f, BaseHotOffsetPx = 70.f, BaseHeadStr = 0.f;
	float RadiusPx = 20.f, RadiusMaxRatio = 0.35f, OuterPadPx = 0.f;
	FLinearColor BaseBodyColor = FLinearColor::Black;

	// 상태 전환 (현재 → 목표)
	float CurHot = 0.f, CurSheen = 0.f, CurRim = 0.f, CurBody = 0.f, CurEmpty = 0.f, CurOpacity = 1.f;
	float TgtHot = 0.f, TgtSheen = 0.f, TgtRim = 0.f, TgtBody = 0.f, TgtEmpty = 0.f, TgtOpacity = 1.f;
	bool bStateBlending = false;

	// 핫스팟 이동
	bool  bAnchorMoving = false;
	bool  bEaseInOut = false;
	float AnchorFrom = 0.f, AnchorTo = 0.f, AnchorT = 0.f, AnchorDuration = 0.f;
	float CurAnchor = 0.f, CurHead = 0.f, CurTrail = 0.f, CurTrailDir = 1.f;

	// 마지막으로 MID에 쓴 값 (중복 Set 방지)
	float W_Hot = -999.f, W_Sheen = -999.f, W_Rim = -999.f, W_Body = -999.f, W_Empty = -999.f;
	float W_Anchor = -999.f, W_Head = -999.f, W_Trail = -999.f, W_TrailDir = -999.f;

	FVector2D LastLocalSize = FVector2D::ZeroVector;
	float LastHoverAnchor = -1.f;
};