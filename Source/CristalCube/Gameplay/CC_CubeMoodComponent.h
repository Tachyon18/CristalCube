// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../CristalCubeStruct.h"
#include "CC_CubeMoodComponent.generated.h"


/**
 * CC_CubeMoodComponent
 *
 * CubeWorldManager 또는 별도 무드 관리 액터에 붙는 컴포넌트.
 * 큐브 전환 시 레벨의 Directional Light, Post Process, Fog를
 * 무드 설정값에 맞게 런타임으로 교체합니다.
 *
 * 사용 방법:
 *   1. BP_CubeWorldManager에 이 컴포넌트 추가
 *   2. MoodSettings 배열에 무드별 설정값 입력
 *   3. PerformTransition 시점에 ApplyMood(CubeType) 호출
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CRISTALCUBE_API UCC_CubeMoodComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCC_CubeMoodComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // ========== 에디터 설정 ==========

    /**
     * 큐브 타입별 무드 설정 배열
     * 인덱스 0 = CubeType 0 (초원/기본)
     * 인덱스 1 = CubeType 1 (중간 난이도)
     * 인덱스 2 = CubeType 2 (어려움/극한)
     * 필요하면 더 추가 가능
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mood")
    TArray<FCubeMoodSettings> MoodSettings;

    // ========== 공개 함수 ==========

    /**
     * 큐브 타입에 맞는 무드 적용.
     * CubeWorldManager의 PerformTransition에서 호출.
     * @param CubeType - 0/1/2 (Easy/Medium/Hard)
     */
    UFUNCTION(BlueprintCallable, Category = "Mood")
    void ApplyMood(int32 CubeType);

    /**
     * 씬의 조명 액터를 캐시.
     * BeginPlay에서 자동 호출, 없으면 수동 호출.
     */
    UFUNCTION(BlueprintCallable, Category = "Mood")
    void CacheLightActors();

private:

    // 씬 액터 캐시
    UPROPERTY()
    class ADirectionalLight* CachedSunLight = nullptr;

    UPROPERTY()
    class AExponentialHeightFog* CachedFog = nullptr;

    UPROPERTY()
    APostProcessVolume* CachedPPV = nullptr;

    // 내부 적용 함수
    void ApplyToDirectionalLight(const FCubeMoodSettings& Settings);
    void ApplyToFog(const FCubeMoodSettings& Settings);
    void ApplyToPostProcess(const FCubeMoodSettings& Settings);
};
