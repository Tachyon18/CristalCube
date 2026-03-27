// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CC_LevelUpWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponSelected, FName, WeaponName);
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
	void SetWeaponChoices(const TArray<FName>& InWeaponNames);

	UPROPERTY(BlueprintAssignable, Category = "LevelUp")
	FOnWeaponSelected OnWeaponSelected;

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

    UPROPERTY(meta = (BindWidget)) class UCC_GlassWidget* CardPanel1;
    UPROPERTY(meta = (BindWidget)) UCC_GlassWidget* CardPanel2;
    UPROPERTY(meta = (BindWidget)) UCC_GlassWidget* CardPanel3;
    UFUNCTION() void OnChoice1Clicked();
    UFUNCTION() void OnChoice2Clicked();
    UFUNCTION() void OnChoice3Clicked();

protected:

    TArray<FName> WeaponChoices;

    void SelectWeapon(int32 ChoiceIndex);

};
