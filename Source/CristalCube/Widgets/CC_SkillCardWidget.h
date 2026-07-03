// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "../CristalCubeStruct.h"
#include "CC_SkillCardWidget.generated.h"

class UCC_SkillCardData;
class UCC_AddonBadgeWidget;

/**
 * 스킬 백과사전(WBP_SkillEncyclopedia) TileView의 항목 위젯 하나.
 * IUserObjectListEntry로 UCC_SkillCardData를 받아 표시만 한다 —
 * 서브시스템을 직접 호출하지 않음 (데이터는 컨테이너 쪽에서 이미 준비됨).
 *
 * [Blueprint 설정]
 *  1. 이 클래스를 부모로 WBP_SkillCard 생성
 *  2. 루트를 SizeBox로 잡고 WidthOverride/HeightOverride를
 *     CardWidth/CardHeight(아래 EditDefaultsOnly)와 동일하게 맞출 것
 *  3. AddonBadgeContainer는 HorizontalBox 또는 WrapBox로 준비
 *  4. AddonBadgeWidgetClass에 WBP_AddonBadge 지정 (비워두면 뱃지 표시만 스킵, 카드는 정상 동작)
 */
UCLASS()
class CRISTALCUBE_API UCC_SkillCardWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
    /* 카드 고정 크기 — TileView의 EntryWidth/EntryHeight와 반드시 일치시킬 것.
       값은 여기 한 곳에서만 관리하고, WBP_SkillEncyclopedia가 CDO를 조회해 TileView에 그대로 반영한다. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill Card|Layout")
    float CardWidth = 220.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill Card|Layout")
    float CardHeight = 300.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill Card|Addon")
    TSubclassOf<UCC_AddonBadgeWidget> AddonBadgeWidgetClass;

protected:
    // ── IUserObjectListEntry ─────────────────────────────────
    virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

    // ── BindWidget — Blueprint에서 이름 일치 필수 ─────────────
    UPROPERTY(meta = (BindWidget))
    class UImage* SkillIcon;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* SkillName;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* SkillDescription;

    /** 애드온 뱃지가 채워질 컨테이너 */
    UPROPERTY(meta = (BindWidget))
    class UPanelWidget* AddonBadgeContainer;

private:
    void RefreshAddonBadges(const TArray<FAddonTableRow>& AddonBadges);
};
