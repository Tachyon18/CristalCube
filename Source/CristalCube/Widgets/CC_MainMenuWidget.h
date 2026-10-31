// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CC_MainMenuWidget.generated.h"

class UButton;
class UWidgetSwitcher;

/** 메뉴 페이지 — MenuSwitcher 자식 순서와 같아야 함 (0 = 메인, 1 = 설정, 2 = 스킬 도감) */
UENUM(BlueprintType)
enum class ECCMenuPage : uint8
{
	Main          UMETA(DisplayName = "Main"),
	Settings      UMETA(DisplayName = "Settings"),
	Encyclopedia  UMETA(DisplayName = "Encyclopedia")
};

/**
 * 메인 메뉴 — 좌상단 게임명 + 그 아래 메뉴 버튼. 오른쪽은 쇼케이스 큐브(3D 레벨)가 보이도록 비워 둠.
 *
 * [Blueprint 설정]
 *  1. WBP_MainMenu를 이 클래스로 Reparent
 *  2. 필수: StartButton, QuitButton (UButton)
 *  3. 선택: SettingsButton, EncyclopediaButton (UButton), MenuSwitcher (UWidgetSwitcher)
 *     MenuSwitcher 자식 순서: [0] 메인 버튼 묶음 [1] 설정 페이지 [2] 스킬 도감 페이지
 *  4. 하위 페이지 "뒤로" 버튼 이름: SettingsBackButton / EncyclopediaBackButton (C++에서 자동 연결). ESC도 메인으로 돌아감
 */
UCLASS()
class CRISTALCUBE_API UCC_MainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Main Menu")
	void ShowPage(ECCMenuPage Page);

	UFUNCTION(BlueprintCallable, Category = "Main Menu")
	void ShowMainPage() { ShowPage(ECCMenuPage::Main); }

	UFUNCTION(BlueprintPure, Category = "Main Menu")
	ECCMenuPage GetCurrentPage() const { return CurrentPage; }

protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/** 페이지가 바뀐 뒤 호출 — BP에서 전환 애니메이션 등 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Main Menu")
	void OnPageChanged(ECCMenuPage NewPage);

	UPROPERTY(meta = (BindWidget))
	UButton* StartButton;

	UPROPERTY(meta = (BindWidget))
	UButton* QuitButton;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* SettingsButton;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* EncyclopediaButton;

	UPROPERTY(meta = (BindWidgetOptional))
	UWidgetSwitcher* MenuSwitcher;

	/** 설정 페이지의 "뒤로" 버튼 — 있으면 자동으로 ShowMainPage에 연결 */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton* SettingsBackButton;

	/** 스킬 도감 페이지의 "뒤로" 버튼 — 있으면 자동으로 ShowMainPage에 연결 */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton* EncyclopediaBackButton;

private:
	ECCMenuPage CurrentPage = ECCMenuPage::Main;

	UFUNCTION()
	void HandleStartClicked();

	UFUNCTION()
	void HandleSettingsClicked();

	UFUNCTION()
	void HandleEncyclopediaClicked();

	UFUNCTION()
	void HandleQuitClicked();

	UFUNCTION()
	void HandleBackClicked();

	/** 시작 시 1회 — 바인딩 누락/스위처 자식 수를 로그로 출력 */
	void LogBindingState() const;
};