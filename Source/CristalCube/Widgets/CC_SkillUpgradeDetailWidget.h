// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CC_SkillUpgradeDetailWidget.generated.h"

class UCC_SkillBase;
class ACC_PlayerState;


/**
 * 
 */
UCLASS()
class CRISTALCUBE_API UCC_SkillUpgradeDetailWidget : public UUserWidget
{
	GENERATED_BODY()

public:
    /** SkillRoster에서 스킬 선택 시 컨테이너가 호출 — 이 스킬 기준으로 Addon/Core 섹션 재구성 */
    UFUNCTION(BlueprintCallable, Category = "Skill Upgrade")
    void ShowSkill(UCC_SkillBase* Skill, ACC_PlayerState* PlayerState);

protected:
    UPROPERTY(meta = (BindWidget)) class UImage* SkillIcon;
    UPROPERTY(meta = (BindWidget)) class UTextBlock* SkillName;
    UPROPERTY(meta = (BindWidget)) UTextBlock* SkillDescription;

    /** 이 스킬이 가진 Addon 카드들이 쌓이는 컨테이너 (VerticalBox 권장 — 보통 0~3개라 TileView는 과함) */
    UPROPERTY(meta = (BindWidget)) class UPanelWidget* AddonCardContainer;

    /** Core 강화 4행(Damage/Size/Speed/ProjectileCount) — 고정 개수라 BindWidget 4개로 직접 배치 */
    UPROPERTY(meta = (BindWidget)) class UCC_AddonUpgradeRowWidget* CoreDamageRow;
    UPROPERTY(meta = (BindWidget)) UCC_AddonUpgradeRowWidget* CoreSizeRow;
    UPROPERTY(meta = (BindWidget)) UCC_AddonUpgradeRowWidget* CoreSpeedRow;
    UPROPERTY(meta = (BindWidget)) UCC_AddonUpgradeRowWidget* CoreProjectileCountRow;

    UPROPERTY(EditDefaultsOnly, Category = "Skill Upgrade")
    TSubclassOf<class UCC_AddonUpgradeCardWidget> AddonUpgradeCardWidgetClass;

private:
    TWeakObjectPtr<UCC_SkillBase> CurrentSkill;
    TWeakObjectPtr<ACC_PlayerState> BoundPlayerState;
	
};
