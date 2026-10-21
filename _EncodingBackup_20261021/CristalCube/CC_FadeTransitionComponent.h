// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Templates/Function.h" 
#include "CristalCubeStruct.h"
#include "CC_FadeTransitionComponent.generated.h"


/**
 * Fade Transition Component
 * - 큐브 전환 시 암전/밝아짐 효과
 * - 페이드 인/아웃 타이밍 관리
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CRISTALCUBE_API UCC_FadeTransitionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCC_FadeTransitionComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/**페이드 지속시간 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fade")
	float FadeDuration = 0.2f;

	/** 현재 페이드 알파 (0 = 투명, 1 = 완전 검정) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fade")
	float CurrentFadeAlpha = 0.0f;

	/** 페이드 중인지 여부 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fade")
	bool bIsFading = false;

	// 페이드 테스트 보류

	///** 검은색으로 페이드 (암전) */
	//UFUNCTION(BlueprintCallable, Category = "Fade")
	//void FadeToBlack(TFunction<void()> OnComplete);

	///** 투명으로 페이드 (밝아짐) */
	//UFUNCTION(BlueprintCallable, Category = "Fade")
	//void FadeFromBlack(TFunction<void()> OnComplete);

	/** 즉시 검은색으로 */
	UFUNCTION(BlueprintCallable, Category = "Fade")
	void SetBlack();

	/** 즉시 투명으로 */
	UFUNCTION(BlueprintCallable, Category = "Fade")
	void SetClear();

	// ========================================
	// Events
	// ========================================

	/** 페이드 완료 이벤트 */
	UPROPERTY(BlueprintAssignable, Category = "Fade|Events")
	FOnFadeComplete OnFadeComplete;

protected:

	/** 페이드 방향 (true = ToBlack, false = FromBlack) */
	bool bFadeToBlack = true;

	float ElapsedTime = 0.0f;

	TFunction<void()> CompletionCallback;

	void UpdateFade(float DeltaTime);
	void CompleteFade();
};
