// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CC_UISurface.h"           // EUISurfaceState · UCC_UISurface
#include "CC_UIControl.generated.h"

// CC_UIControl.h — L2 공통 부모 (Button · IconButton · Tab)
// 투명 UButton(HitButton)이 입력을 받고 → 내부 Surface(ControlSurface)에 상태를 전달 + 눌림 스케일(내용 포함).
// 상태 처리 코드는 여기 한 곳. 파생 WBP/C++는 내용(글자·아이콘)과 상태별 글자색만 담당.

class UButton;
class UCC_UIControl;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUIControlEvent, UCC_UIControl*, Control);

/**
 * WBP 구조 (이름 일치 필수):
 *   [Root] 아무 패널 (SizeBox / Overlay)       ← 눌림 스케일 대상 = 루트 전체 (Pivot 0.5, 0.5)
 *     └ Overlay
 *        ├ ControlSurface (WBP_UI_Surface, Fill/Fill)   BindWidgetOptional — 없으면 Surface 없는 Text 버튼
 *        ├ (내용 — TextBlock / Image …)                 Visibility = Not Hit-Testable
 *        └ HitButton (Button, Fill/Fill, 맨 위)         BindWidget — 스타일은 C++가 투명으로 덮어씀
 *
 * 상태 우선순위: Disabled > Pressed > Selected > Hover > Rest
 */
UCLASS(Abstract)
class CRISTALCUBE_API UCC_UIControl : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 활성/비활성 — 비활성이면 클릭·호버 무시, Surface Disabled */
	UFUNCTION(BlueprintCallable, Category = "Control")
	void SetControlEnabled(bool bInEnabled);

	UFUNCTION(BlueprintPure, Category = "Control")
	bool IsControlEnabled() const { return bControlEnabled; }

	/** 선택 유지 (Tab · 토글) — 클릭으로 자동 토글하지 않음, 부모가 지시 */
	UFUNCTION(BlueprintCallable, Category = "Control")
	void SetSelected(bool bInSelected);

	UFUNCTION(BlueprintPure, Category = "Control")
	bool IsSelected() const { return bSelected; }

	/** 현재 표시 상태 (Rest/Hover/Pressed/Selected/Disabled) */
	UFUNCTION(BlueprintPure, Category = "Control")
	EUISurfaceState GetVisualState() const { return VisualState; }

	UPROPERTY(BlueprintAssignable, Category = "Control")
	FOnUIControlEvent OnControlClicked;

	UPROPERTY(BlueprintAssignable, Category = "Control")
	FOnUIControlEvent OnControlHovered;

	UPROPERTY(BlueprintAssignable, Category = "Control")
	FOnUIControlEvent OnControlUnhovered;

	// ── 에디터 설정 ──────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Control")
	bool bStartEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Control")
	bool bStartSelected = false;

	/** 눌림 시 루트 전체(Surface + 내용) 축소 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Control")
	bool bUsePressedScale = true;

	/** −1 = Token Motion.PressedScale (0.97) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Control")
	float PressedScale = -1.f;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativePreConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** 상태가 바뀔 때 (파생 C++) — 글자색 등. bInstant = PreConstruct/즉시 적용 */
	virtual void NativeOnVisualStateChanged(EUISurfaceState NewState, bool bInstant) {}

	/** 상태가 바뀔 때 (파생 WBP) — PreConstruct에서도 호출되어 디자이너에 반영 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Control", meta = (DisplayName = "On Visual State Changed"))
	void BP_OnVisualStateChanged(EUISurfaceState NewState);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> HitButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCC_UISurface> ControlSurface;

private:
	UFUNCTION() void HandleHovered();
	UFUNCTION() void HandleUnhovered();
	UFUNCTION() void HandlePressed();
	UFUNCTION() void HandleReleased();
	UFUNCTION() void HandleClicked();

	void MakeButtonTransparent();
	void RefreshVisualState(bool bInstant);
	void ApplyScale(float Scale);
	float ResolvePressedScale() const;
	float ResolveBlendTime() const;

	bool bControlEnabled = true;
	bool bSelected = false;
	bool bHovered = false;
	bool bPressed = false;
	bool bHasVisualState = false;
	EUISurfaceState VisualState = EUISurfaceState::Rest;

	float CurScale = 1.f;
	float TgtScale = 1.f;
};