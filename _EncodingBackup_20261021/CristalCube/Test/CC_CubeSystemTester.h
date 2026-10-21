// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CC_CubeSystemTester.generated.h"

USTRUCT(BlueprintType)
struct FTestResult
{
	GENERATED_BODY()

	UPROPERTY()
	FString TestName;

	UPROPERTY()
	bool bPassed = false;

	UPROPERTY()
	FString Message;
};

UCLASS()
class CRISTALCUBE_API ACC_CubeSystemTester : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACC_CubeSystemTester();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:
	// ========== 설정 ==========

	/** 자동 테스트 실행 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Testing")
	bool bAutoRunTests = true;

	/** 테스트 시작 딜레이 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Testing")
	float TestStartDelay = 2.0f;

	/** 테스트용 적 수 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Testing")
	int32 NumTestEnemies = 20;

	/** 성능 테스트용 적 수 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Testing")
	int32 NumPerformanceTestActors = 100;

	/** 테스트 Enemy 클래스 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Testing")
	TSubclassOf<class ACC_TestActor> TestActorClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Testing")
	class ACC_CubeWorldManager* CubeManager;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Testing")
	TArray<FTestResult> TestResults;


	// ========== 상태 ==========

	/** Cube 참조 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Testing")
	class ACC_Cube* Cube;

	/** 테스트 결과 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Testing")
	int32 TestsPassed = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Testing")
	int32 TestsFailed = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Testing")
	TArray<FString> TestLog;

	// ========== 테스트 함수 ==========

	/** 모든 테스트 실행 */
	UFUNCTION(BlueprintCallable, Category = "Testing")
	void RunAllTests();

	/** 테스트 리포트 출력 */
	UFUNCTION(BlueprintCallable, Category = "Testing")
	void PrintTestReport();

	// ========================================
	// Individual Tests
	// ========================================

	/** Test 1: Manager 초기화 */
	UFUNCTION(BlueprintCallable, Category = "Testing")
	bool Test_ManagerInitialization();

	/** Test 2: 3x3 그리드 생성 */
	UFUNCTION(BlueprintCallable, Category = "Testing")
	bool Test_GridGeneration();

	/** Test 3: 큐브 Spawn */
	UFUNCTION(BlueprintCallable, Category = "Testing")
	bool Test_CubeSpawning();

	/** Test 4: Freeze/Unfreeze */
	UFUNCTION(BlueprintCallable, Category = "Testing")
	bool Test_FreezeSystem();

	/** Test 5: 경계 감지 */
	UFUNCTION(BlueprintCallable, Category = "Testing")
	bool Test_BoundaryDetection();

	/** Test 6: 순환 좌표 계산 */
	UFUNCTION(BlueprintCallable, Category = "Testing")
	bool Test_CoordinateWrapping();

	/** Test 7: 큐브 전환 (시뮬레이션) */
	UFUNCTION(BlueprintCallable, Category = "Testing")
	bool Test_CubeTransition();

	/** Test 8: Actor 관리 */
	UFUNCTION(BlueprintCallable, Category = "Testing")
	bool Test_ActorManagement();

	// ========================================
	// Utilities
	// ========================================

	/** 테스트 결과 추가 */
	void AddTestResult(const FString& TestName, bool bPassed, const FString& Message);

	/** Manager 찾기 */
	void FindCubeManager();

protected:

	FTimerHandle TestTimerHandle;

};
