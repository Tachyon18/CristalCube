// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../CristalCubeStruct.h"
#include "CC_LevelUpWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLevelUpCandidateSelected, FLevelUpCandidate, SelectedCandidate);
/**
 * 
 */
UCLASS()
class CRISTALCUBE_API UCC_LevelUpWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:

	virtual void NativeConstruct() override;

public:

    UFUNCTION(BlueprintCallable, Category = "LevelUp")
    void SetLevelUpChoices(const TArray<FLevelUpCandidate>& InCandidates);

    UPROPERTY(BlueprintAssignable, Category = "LevelUp")
    FOnLevelUpCandidateSelected OnLevelUpCandidateSelected;

protected:

    // 버튼 / 텍스트
    UPROPERTY(meta = (BindWidget)) class UButton* Choice1Button;
    UPROPERTY(meta = (BindWidget)) UButton* Choice2Button;
    UPROPERTY(meta = (BindWidget)) UButton* Choice3Button;
    UPROPERTY(meta = (BindWidget)) class UTextBlock* Choice1Name;
    UPROPERTY(meta = (BindWidget)) UTextBlock* Choice2Name;
    UPROPERTY(meta = (BindWidget)) UTextBlock* Choice3Name;
    UPROPERTY(meta = (BindWidget)) UTextBlock* Choice1Description;
    UPROPERTY(meta = (BindWidget)) UTextBlock* Choice2Description;
    UPROPERTY(meta = (BindWidget)) UTextBlock* Choice3Description;
    UPROPERTY(meta = (BindWidget)) class UImage* Choice1Icon;
    UPROPERTY(meta = (BindWidget)) UImage* Choice2Icon;
    UPROPERTY(meta = (BindWidget)) UImage* Choice3Icon;
    UPROPERTY(BlueprintReadOnly , meta = (BindWidget)) class UCC_IntroGlassWidget* CardPanel1;
    UPROPERTY(BlueprintReadOnly , meta = (BindWidget)) UCC_IntroGlassWidget* CardPanel2;
    UPROPERTY(BlueprintReadOnly , meta = (BindWidget)) UCC_IntroGlassWidget* CardPanel3;
    UFUNCTION() void OnChoice1Clicked();
    UFUNCTION() void OnChoice2Clicked();
    UFUNCTION() void OnChoice3Clicked();

protected:

    TArray<FLevelUpCandidate> Candidates;

    void SelectCandidate(int32 ChoiceIndex);

};
