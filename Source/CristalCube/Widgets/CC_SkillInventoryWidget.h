// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CC_SkillInventoryWidget.generated.h"

class UCC_SkillRosterEntryWidget;
class ACC_PlayerState;

/**
 * 장착 스킬을 스크롤 가능한 리스트로 보여주는 인벤토리 위젯. 각 행(WBP_SkillRosterEntry)이
 * 접힘 카드 + 펼침 서랍을 자체적으로 갖고, 이 위젯은 "몇 번 슬롯인지"와
 * "지금 하나만 펼쳐져 있는지"만 관장한다. PlayerState::OnSkillsChanged 구독으로 자동 갱신.
 */
UCLASS()
class CRISTALCUBE_API UCC_SkillInventoryWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UFUNCTION(BlueprintCallable, Category = "Skill Inventory")
    void RefreshInventory();

protected:

    UPROPERTY(meta = (BindWidget))
    class UPanelWidget* RosterContainer;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill Inventory")
    TSubclassOf<UCC_SkillRosterEntryWidget> RosterEntryWidgetClass;

    UFUNCTION()
    void HandleEntryExpandRequested(UCC_SkillRosterEntryWidget* Entry);

private:
    static constexpr int32 NumEquipSlots = 6;

    UPROPERTY()
    TArray<UCC_SkillRosterEntryWidget*> RosterEntries;

    UPROPERTY()
    UCC_SkillRosterEntryWidget* ExpandedEntry = nullptr;

    UPROPERTY()
    ACC_PlayerState* BoundPlayerState = nullptr;

    FTimerHandle TransitionTimerHandle;

    /** 펼치기/접기 시퀀스의 2단계 — 형제 카드들을 전부 페이드 인시킴(접었을 때 호출). */
    void RevealSiblings();

    UFUNCTION()
    void HandleSkillsChanged();

};
