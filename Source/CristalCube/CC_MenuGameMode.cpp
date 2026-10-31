// Fill out your copyright notice in the Description page of Project Settings.

#include "CC_MenuGameMode.h"
#include "Widgets/CC_MainMenuWidget.h"
#include "Camera/CameraActor.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

ACC_MenuGameMode::ACC_MenuGameMode()
{
	DefaultPawnClass = nullptr;                              // 메뉴에는 조작할 폰 없음
	PlayerControllerClass = APlayerController::StaticClass(); // GameHUD를 만드는 ACC_PlayerController 사용 안 함
}

void ACC_MenuGameMode::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC) return;

	// 메뉴 카메라 고정
	for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(MenuCameraTag))
		{
			PC->SetViewTarget(*It);
			break;
		}
	}

	if (MainMenuWidgetClass)
	{
		MainMenuWidget = CreateWidget<UCC_MainMenuWidget>(PC, MainMenuWidgetClass);
		if (MainMenuWidget)
		{
			MainMenuWidget->AddToViewport();
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[MenuGameMode] MainMenuWidgetClass not set!"));
	}

	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	if (MainMenuWidget)
	{
		InputMode.SetWidgetToFocus(MainMenuWidget->TakeWidget());
	}
	PC->SetInputMode(InputMode);
	PC->SetShowMouseCursor(true);
}

void ACC_MenuGameMode::StartGame()
{
	UGameplayStatics::OpenLevel(this, GameLevelName);
}

void ACC_MenuGameMode::QuitGame()
{
	UKismetSystemLibrary::QuitGame(this, UGameplayStatics::GetPlayerController(this, 0), EQuitPreference::Quit, false);
}