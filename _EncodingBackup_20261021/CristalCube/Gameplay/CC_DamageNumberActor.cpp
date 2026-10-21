// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_DamageNumberActor.h"
#include "../SkillSystem/CC_EffectorPoolSubsystem.h"
#include "Components/TextRenderComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
ACC_DamageNumberActor::ACC_DamageNumberActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.05f;

	TextRender = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TextRender"));
	SetRootComponent(TextRender);

	TextRender->SetHorizontalAlignment(EHTA_Center);
	TextRender->SetVerticalAlignment(EVRTA_TextCenter);
	TextRender->SetWorldSize(150.0f);

	SetActorEnableCollision(false);

}

void ACC_DamageNumberActor::Initialize(float DamageAmount, const FVector& SpawnLocation, bool bIsCritical)
{
	ElapsedTime = 0.0f;

	SetActorLocation(SpawnLocation);
	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);

	const FString DamageText = FString::Printf(TEXT("%.0f"), DamageAmount);
	TextRender->SetText(FText::FromString(DamageText));
	TextRender->SetTextRenderColor(bIsCritical ? CriticalColor : NormalColor);
	TextRender->SetWorldSize(bIsCritical ? 44.0f : 150.0f);
}

void ACC_DamageNumberActor::SetOwningPool(UCC_EffectorPoolSubsystem* InPool)
{
	OwningPool = InPool;
}

// Called when the game starts or when spawned
void ACC_DamageNumberActor::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		CachedCameraManager = PC->PlayerCameraManager;
	}
	
}

// Called every frame
void ACC_DamageNumberActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ElapsedTime += DeltaTime;

	// 위로 떠오르기
	AddActorWorldOffset(FVector(0.f, 0.f, RiseSpeed * DeltaTime));

	// 카메라 쪽을 향하게(빌보드) — PlayerCameraManager 기준
	if (CachedCameraManager)
	{
		const FVector CamLoc = CachedCameraManager->GetCameraLocation();
		const FRotator LookAt = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), CamLoc);
		SetActorRotation(LookAt);
	}

	// 페이드 아웃 — TextRenderComponent에 알파를 반영하는 Translucent/Masked 머티리얼이
	// 할당돼 있어야 시각적으로 보임. 기본 Opaque 머티리얼이면 알파 변화는 무시되고
	// 위로 떠오르는 움직임만 보임(그래도 테스트 목적으론 충분).
	const float Alpha = FMath::Clamp(1.0f - (ElapsedTime / LifeTime), 0.0f, 1.0f);
	FColor CurrentColor = TextRender->TextRenderColor;
	CurrentColor.A = static_cast<uint8>(Alpha * 255);
	TextRender->SetTextRenderColor(CurrentColor);

	if (ElapsedTime >= LifeTime)
	{
		Deactivate();
	}

}

void ACC_DamageNumberActor::Deactivate()
{
	SetActorTickEnabled(false);
	SetActorHiddenInGame(true);

	if (OwningPool)
	{
		OwningPool->ReleaseEffector(this);
	}
	else
	{
		Destroy();
	}
}

