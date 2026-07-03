// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CC_SkillEncyclopediaWidget.generated.h"

class UTileView;
class UCC_SkillCardData;

/**
 * TestRoom용 스킬 백과사전 컨테이너.
 * UCC_SkillLibrarySubsystem에서 DT_Skill 전체를 읽어
 * UCC_SkillCardData 래퍼로 감싼 뒤 TileView에 채운다.
 *
 * [Blueprint 설정]
 *  1. 이 클래스를 부모로 WBP_SkillEncyclopedia 생성
 *  2. SkillTileView를 UTileView로 바인딩
 *  3. TileView Details 패널에서 "List Entry Widget Class"를 WBP_SkillCard로 지정
 *     (C++에서 강제하지 않음 — 디자이너 쪽 설정이 표준 경로)
 */
UCLASS()
class CRISTALCUBE_API UCC_SkillEncyclopediaWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:

	virtual void NativeConstruct() override;

	/** 필요 시 외부(F1~F8 콘솔 등)에서 강제 새로고침할 수 있도록 공개 */
	UFUNCTION(BlueprintCallable, Category = "Skill Encyclopedia")
	void RefreshSkillList();

protected:
	UPROPERTY(meta = (BindWidget))
	UTileView* SkillTileView;

private:
	/** TileView는 UObject*를 참조만 하므로, GC 방지를 위해 위젯이 직접 소유권을 보유 */
	UPROPERTY()
	TArray<UCC_SkillCardData*> CardDataPool;
};
