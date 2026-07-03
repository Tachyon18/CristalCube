// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../CristalCubeStruct.h"
#include "CC_AddonBadgeWidget.generated.h"


/**
 * 스킬 카드 안에서 애드온 하나를 표시하는 최소 단위 위젯.
 * 지금은 아이콘 + 이름만 채우는 스텁 — 비주얼이 확정되면
 * 이 클래스 내부만 갈아끼우면 되고, 카드 쪽(UCC_SkillCardWidget)은 그대로 유지된다.
 */
UCLASS()
class CRISTALCUBE_API UCC_AddonBadgeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Addon Badge")
    void SetAddonData(const FAddonTableRow& InAddonData);

protected:
    UPROPERTY(meta = (BindWidgetOptional))
    class UImage* AddonIcon;

    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* AddonName;
	
};
