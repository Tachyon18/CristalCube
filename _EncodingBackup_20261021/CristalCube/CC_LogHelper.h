// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CC_LogHelper.generated.h"

/**
 * CristalCube 로그 유틸리티 클래스
 * 깔끔한 로그 출력을 위한 헬퍼 함수들과 매크로들
 */
UCLASS()
class CRISTALCUBE_API UCC_LogHelper : public UObject
{
	GENERATED_BODY()

public:
	// 깔끔한 액터 이름 생성
	UFUNCTION(BlueprintPure, Category = "CC Log", meta = (CallInEditor = "true"))
	static FString GetCleanActorName(const AActor* Actor);

	// 깔끔한 클래스 이름 생성
	UFUNCTION(BlueprintPure, Category = "CC Log")
	static FString GetCleanClassName(const UObject* Object);

	// 로그 타입별 컬러 설정 (화면 출력용)
	UFUNCTION(BlueprintPure, Category = "CC Log")
	static FColor GetLogColor(const FString& InLogType);
	
};

//==============================================================================
// 편의 매크로들
//==============================================================================

// 액터 이름 헬퍼
#define CC_ACTOR_NAME(Actor) (*UCC_LogHelper::GetCleanActorName(Actor))

// 기본 로그 매크로들
#define CC_LOG_PLAYER(Verbosity, Format, ...) \
    UE_LOG(LogTemp, Verbosity, TEXT("[PLAYER] " Format), ##__VA_ARGS__)

#define CC_LOG_ENEMY(Verbosity, Format, ...) \
    UE_LOG(LogTemp, Verbosity, TEXT("[ENEMY] " Format), ##__VA_ARGS__)

#define CC_LOG_AI(Verbosity, Format, ...) \
    UE_LOG(LogTemp, Verbosity, TEXT("[AI] " Format), ##__VA_ARGS__)

#define CC_LOG_COMBAT(Verbosity, Format, ...) \
    UE_LOG(LogTemp, Verbosity, TEXT("[COMBAT] " Format), ##__VA_ARGS__)

#define CC_LOG_WEAPON(Verbosity, Format, ...) \
    UE_LOG(LogTemp, Verbosity, TEXT("[WEAPON] " Format), ##__VA_ARGS__)

#define CC_LOG_SKILL(Verbosity, Format, ...) \
    UE_LOG(LogTemp, Verbosity, TEXT("[SKILL] " Format), ##__VA_ARGS__)

#define CC_LOG_STATS(Verbosity, Format, ...) \
    UE_LOG(LogTemp, Verbosity, TEXT("[STATS] " Format), ##__VA_ARGS__)

#define CC_LOG_SPAWNER(Verbosity, Format, ...) \
    UE_LOG(LogTemp, Verbosity, TEXT("[SPAWNER] " Format), ##__VA_ARGS__)

#define CC_LOG_ERROR(Verbosity, Format, ...) \
    UE_LOG(LogTemp, Verbosity, TEXT("[ERROR] " Format), ##__VA_ARGS__)

// 액터 정보 포함 버전
#define CC_LOG_PLAYER_ACTOR(Verbosity, Actor, Format, ...) \
    UE_LOG(LogTemp, Verbosity, TEXT("[PLAYER][%s] " Format), CC_ACTOR_NAME(Actor), ##__VA_ARGS__)

#define CC_LOG_ENEMY_ACTOR(Verbosity, Actor, Format, ...) \
    UE_LOG(LogTemp, Verbosity, TEXT("[ENEMY][%s] " Format), CC_ACTOR_NAME(Actor), ##__VA_ARGS__)

#define CC_LOG_AI_ACTOR(Verbosity, Actor, Format, ...) \
    UE_LOG(LogTemp, Verbosity, TEXT("[AI][%s] " Format), CC_ACTOR_NAME(Actor), ##__VA_ARGS__)

#define CC_LOG_COMBAT_ACTOR(Verbosity, Actor, Format, ...) \
    UE_LOG(LogTemp, Verbosity, TEXT("[COMBAT][%s] " Format), CC_ACTOR_NAME(Actor), ##__VA_ARGS__)

#define CC_LOG_WEAPON_ACTOR(Verbosity, Actor, Format, ...) \
    UE_LOG(LogTemp, Verbosity, TEXT("[WEAPON][%s] " Format), CC_ACTOR_NAME(Actor), ##__VA_ARGS__)

#define CC_LOG_SKILL_ACTOR(Verbosity, Actor, Format, ...) \
    UE_LOG(LogTemp, Verbosity, TEXT("[SKILL][%s] " Format), CC_ACTOR_NAME(Actor), ##__VA_ARGS__)

#define CC_LOG_SPAWNER_ACTOR(Verbosity, Actor, Format, ...) \
    UE_LOG(LogTemp, Verbosity, TEXT("[SPAWNER][%s] " Format), CC_ACTOR_NAME(Actor), ##__VA_ARGS__)

//==============================================================================
// 화면 출력 매크로들 (컬러 포함)
//==============================================================================

#define CC_SCREEN_LOG(LogType, Duration, Format, ...) \
    if (GEngine) \
    { \
        GEngine->AddOnScreenDebugMessage( \
            INDEX_NONE, \
            Duration, \
            UCC_LogHelper::GetLogColor(LogType), \
            FString::Printf(TEXT("[%s] " Format), *LogType, ##__VA_ARGS__) \
        ); \
    }

// 화면 출력 편의 매크로들
#define CC_SCREEN_ENEMY(Duration, Format, ...) CC_SCREEN_LOG(TEXT("ENEMY"), Duration, Format, ##__VA_ARGS__)
#define CC_SCREEN_COMBAT(Duration, Format, ...) CC_SCREEN_LOG(TEXT("COMBAT"), Duration, Format, ##__VA_ARGS__)
#define CC_SCREEN_SKILL(Duration, Format, ...) CC_SCREEN_LOG(TEXT("SKILL"), Duration, Format, ##__VA_ARGS__)
#define CC_SCREEN_AI(Duration, Format, ...) CC_SCREEN_LOG(TEXT("AI"), Duration, Format, ##__VA_ARGS__)
#define CC_SCREEN_SPAWNER(Duration, Format, ...) CC_SCREEN_LOG(TEXT("SPAWNER"), Duration, Format, ##__VA_ARGS__)