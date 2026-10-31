// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CC_MenuGameMode.generated.h"

class UCC_MainMenuWidget;

/**
 * ACC_MenuGameMode — L_MainMenu 전용.
 *
 * ACC_GameModeBase를 상속하지 않음: 플레이어 폰 / ACC_PlayerController(GameHUD 생성) /
 * CubeWorldManager 대기 로직이 메뉴에는 필요 없음.
 *  - 폰 없음, 엔진 기본 APlayerController
 *  - 메뉴 위젯 생성 + UI 전용 입력
 *  - 레벨에 MenuCameraTag 태그가 붙은 CameraActor가 있으면 그 시점으로 고정
 */
UCLASS()
class CRISTALCUBE_API ACC_MenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACC_MenuGameMode();

	/** WBP_MainMenu (부모: UCC_MainMenuWidget) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu")
	TSubclassOf<UCC_MainMenuWidget> MainMenuWidgetClass;

	/** 시작 버튼으로 여는 게임 레벨 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu")
	FName GameLevelName = FName("L_MainGame");

	/** 메뉴 시점 카메라 태그 — 레벨의 CameraActor > Actor > Tags에 추가 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu")
	FName MenuCameraTag = FName("MenuCamera");

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void StartGame();

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void QuitGame();

	UFUNCTION(BlueprintPure, Category = "Menu")
	UCC_MainMenuWidget* GetMainMenuWidget() const { return MainMenuWidget; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	UCC_MainMenuWidget* MainMenuWidget = nullptr;
};