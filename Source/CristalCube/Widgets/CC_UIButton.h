// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CC_UIControl.h"
#include "CC_UIButton.generated.h"

// CC_UIButton.h — L2 Primitive: Button (Ghost · Text · Filled)
// UCC_UIControl 파생 — 상태·눌림·클릭은 부모. 여기는 라벨 · 변형별 Surface · 상태별 글자색.
// Filled(주 행동, 그라데이션 몸체)는 HLSL `UIS_Filled`가 들어오기 전까지 임시 모양 (§4-3).

class USizeBox;
class UTextBlock;

UENUM(BlueprintType)
enum class EUIButtonVariant : uint8
{
	Ghost,        // 보조 — Card Surface + ButtonGhost 글자 (#D6D9EC → Hover #EEF0FF)
	Text,         // Surface 없음 — 글자만 (Secondary → Hover Primary)
	Filled        // 주 행동 (화면당 1개) — Surface Role Filled (rev.7 UIS_Filled 그라데이션 몸체) · ButtonBold 글자
};

/**
 * WBP_UI_Button 구조 (이름 일치):
 *   [Root] ButtonSizeBox (Size Box)        BindWidgetOptional — Height = Token ButtonHeight(50), Width는 배치하는 쪽에서
 *     └ Overlay
 *        ├ ControlSurface (WBP_UI_Surface, Fill/Fill)    SurfaceMaterial 비움
 *        ├ LabelText (TextBlock, Center/Center, Not Hit-Testable)
 *        └ HitButton (Button, Fill/Fill)
 */
UCLASS()
class CRISTALCUBE_API UCC_UIButton : public UCC_UIControl
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Button")
	void SetLabel(FText InLabel);

	UFUNCTION(BlueprintCallable, Category = "Button")
	void SetVariant(EUIButtonVariant InVariant);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Button")
	FText Label = NSLOCTEXT("CC_UI", "ButtonDefault", "Button");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Button")
	EUIButtonVariant Variant = EUIButtonVariant::Ghost;

	/** −1 = Token ButtonHeight (50) · 0 = 높이 지정 안 함 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Button")
	float HeightOverride = -1.f;

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeOnVisualStateChanged(EUISurfaceState NewState, bool bInstant) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> LabelText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> ButtonSizeBox;

private:
	void ApplyLayout();
	void ApplyLabelColor(EUISurfaceState State);
};
