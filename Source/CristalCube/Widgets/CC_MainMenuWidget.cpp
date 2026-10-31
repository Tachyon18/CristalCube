// Fill out your copyright notice in the Description page of Project Settings.

#include "CC_MainMenuWidget.h"
#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"
#include "Kismet/GameplayStatics.h"
#include "../CC_MenuGameMode.h"

void UCC_MainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetIsFocusable(true); // ESC 처리용 키 입력 수신

	if (StartButton)            StartButton->OnClicked.AddUniqueDynamic(this, &UCC_MainMenuWidget::HandleStartClicked);
	if (QuitButton)             QuitButton->OnClicked.AddUniqueDynamic(this, &UCC_MainMenuWidget::HandleQuitClicked);
	if (SettingsButton)         SettingsButton->OnClicked.AddUniqueDynamic(this, &UCC_MainMenuWidget::HandleSettingsClicked);
	if (EncyclopediaButton)     EncyclopediaButton->OnClicked.AddUniqueDynamic(this, &UCC_MainMenuWidget::HandleEncyclopediaClicked);
	if (SettingsBackButton)     SettingsBackButton->OnClicked.AddUniqueDynamic(this, &UCC_MainMenuWidget::HandleBackClicked);
	if (EncyclopediaBackButton) EncyclopediaBackButton->OnClicked.AddUniqueDynamic(this, &UCC_MainMenuWidget::HandleBackClicked);

	LogBindingState();

	ShowPage(ECCMenuPage::Main);
}

void UCC_MainMenuWidget::ShowPage(ECCMenuPage Page)
{
	CurrentPage = Page;

	const int32 Index = static_cast<int32>(Page);
	if (!MenuSwitcher)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MainMenu] ShowPage(%d): MenuSwitcher not bound - check widget name 'MenuSwitcher'"), Index);
	}
	else if (Index >= MenuSwitcher->GetNumWidgets())
	{
		UE_LOG(LogTemp, Warning, TEXT("[MainMenu] ShowPage(%d): MenuSwitcher has only %d child(ren) - pages must be direct children"),
			Index, MenuSwitcher->GetNumWidgets());
	}
	else
	{
		MenuSwitcher->SetActiveWidgetIndex(Index);
	}

	// 클릭한 버튼이 페이지 전환으로 숨겨지면 포커스가 빠져 ESC가 안 들어옴 - 메뉴 위젯이 다시 가져감
	SetFocus();

	OnPageChanged(Page);
}

FReply UCC_MainMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape && CurrentPage != ECCMenuPage::Main)
	{
		ShowMainPage();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UCC_MainMenuWidget::HandleStartClicked()
{
	if (StartButton) StartButton->SetIsEnabled(false); // 로딩 중 연타 방지

	if (ACC_MenuGameMode* GM = Cast<ACC_MenuGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->StartGame();
	}
}

void UCC_MainMenuWidget::HandleSettingsClicked()
{
	ShowPage(ECCMenuPage::Settings);
}

void UCC_MainMenuWidget::HandleEncyclopediaClicked()
{
	ShowPage(ECCMenuPage::Encyclopedia);
}

void UCC_MainMenuWidget::HandleQuitClicked()
{
	if (ACC_MenuGameMode* GM = Cast<ACC_MenuGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->QuitGame();
	}
}

void UCC_MainMenuWidget::HandleBackClicked()
{
	ShowMainPage();
}

void UCC_MainMenuWidget::LogBindingState() const
{
	UE_LOG(LogTemp, Log, TEXT("[MainMenu] Bindings - Start:%d Quit:%d Settings:%d Encyclopedia:%d SettingsBack:%d EncyclopediaBack:%d Switcher:%d (children %d)"),
		StartButton != nullptr, QuitButton != nullptr,
		SettingsButton != nullptr, EncyclopediaButton != nullptr,
		SettingsBackButton != nullptr, EncyclopediaBackButton != nullptr,
		MenuSwitcher != nullptr, MenuSwitcher ? MenuSwitcher->GetNumWidgets() : 0);
}
