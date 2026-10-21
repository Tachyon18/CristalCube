// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_FadeTransitionComponent.h"
#include "Kismet/GameplayStatics.h"


// Sets default values for this component's properties
UCC_FadeTransitionComponent::UCC_FadeTransitionComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UCC_FadeTransitionComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	SetClear();
}


// Called every frame
void UCC_FadeTransitionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
	if (bIsFading)
	{
		UpdateFade(DeltaTime);
	}
}

//void UCC_FadeTransitionComponent::FadeToBlack(TFunction<void()> OnComplete)
//{
//	bIsFading = true;
//	bFadeToBlack = true;
//	ElapsedTime = 0.0f;
//	CompletionCallback = OnComplete;
//
//	UE_LOG(LogTemp, Log, TEXT("[Fade] Starting FadeToBlack (Duration: %.2f)"), FadeDuration);
//}
//
//void UCC_FadeTransitionComponent::FadeFromBlack(TFunction<void()> OnComplete)
//{
//	bIsFading = true;
//	bFadeToBlack = false;
//	ElapsedTime = 0.0f;
//	CompletionCallback = OnComplete;
//
//	UE_LOG(LogTemp, Log, TEXT("[Fade] Starting FadeFromBlack (Duration: %.2f)"), FadeDuration);
//
//}

void UCC_FadeTransitionComponent::SetBlack()
{
	CurrentFadeAlpha = 1.0f;
	bIsFading = false;

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PC && PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->SetManualCameraFade(CurrentFadeAlpha, FLinearColor::Black, false);
	}

	UE_LOG(LogTemp, Log, TEXT("[Fade] Set to Black"));
}

void UCC_FadeTransitionComponent::SetClear()
{
	CurrentFadeAlpha = 0.0f;
	bIsFading = false;

	// 플레이어 카메라에 페이드 적용
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PC && PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->SetManualCameraFade(CurrentFadeAlpha, FLinearColor::Black, false);
	}

	UE_LOG(LogTemp, Log, TEXT("[Fade] Set to Clear"));
}

void UCC_FadeTransitionComponent::UpdateFade(float DeltaTime)
{
	ElapsedTime += DeltaTime;
	float Progress = FMath::Clamp(ElapsedTime / FadeDuration, 0.0f, 1.0f);

	// 페이드 방향에 따라 알파 계산
	if (bFadeToBlack)
	{
		CurrentFadeAlpha = Progress; // 0 -> 1
	}
	else
	{
		CurrentFadeAlpha = 1.0f - Progress; // 1 -> 0
	}

	// 플레이어 카메라에 페이드 적용
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PC && PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->SetManualCameraFade(CurrentFadeAlpha, FLinearColor::Black, false);
	}

	// 완료 체크
	if (Progress >= 1.0f)
	{
		CompleteFade();
	}
}

void UCC_FadeTransitionComponent::CompleteFade()
{
	bIsFading = false;

	UE_LOG(LogTemp, Log, TEXT("[Fade] Fade complete (Alpha: %.2f)"), CurrentFadeAlpha);

	// 이벤트 발생
	OnFadeComplete.Broadcast();

	// 콜백 실행
	if (CompletionCallback)
	{
		CompletionCallback();
		CompletionCallback = nullptr;
	}
}