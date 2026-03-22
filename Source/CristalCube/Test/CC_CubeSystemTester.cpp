// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_CubeSystemTester.h"
#include "CC_TestActor.h"
#include "Kismet/GameplayStatics.h"
#include "../CC_CubeWorldManager.h"
#include "../Gameplay/CC_Cube.h"
#include "TimerManager.h"

// Sets default values
ACC_CubeSystemTester::ACC_CubeSystemTester()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ACC_CubeSystemTester::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoRunTests)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CubeSystemTester] Legacy manual tester actor invoked. Prefer Unreal automation tests under CristalCube.Regression.* for regression coverage."));
		// 딜레이 후 테스트 실행
		GetWorld()->GetTimerManager().SetTimer(
			TestTimerHandle,
			this,
			&ACC_CubeSystemTester::RunAllTests,
			TestStartDelay,
			false
		);
	}
}

// Called every frame
void ACC_CubeSystemTester::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ACC_CubeSystemTester::RunAllTests()
{
	UE_LOG(LogTemp, Warning, TEXT("=============================================="));
	UE_LOG(LogTemp, Warning, TEXT("   CUBE SYSTEM - AUTOMATED TESTING"));
	UE_LOG(LogTemp, Warning, TEXT("=============================================="));
	
	TestResults.Empty();
	FindCubeManager();

	if (!CubeManager)
	{
		UE_LOG(LogTemp, Error, TEXT("[Test] CubeWorldManager not found! Cannot run tests."));
		return;
	}

	// 8가지 테스트 실행
	Test_ManagerInitialization();
	Test_GridGeneration();
	Test_CubeSpawning();
	Test_FreezeSystem();
	Test_BoundaryDetection();
	Test_CoordinateWrapping();
	Test_CubeTransition();
	Test_ActorManagement();

	// 리포트 출력
	PrintTestReport();
}

void ACC_CubeSystemTester::PrintTestReport()
{
	int32 PassedTests = 0;
	int32 TotalTests = TestResults.Num();

	for (const FTestResult& Result : TestResults)
	{
		if (Result.bPassed)
			PassedTests++;
	}


	float SuccessRate = (TotalTests) > 0
		? (PassedTests * 100.f / TotalTests)
		: 0.0f;

	UE_LOG(LogTemp, Warning, TEXT("=============================================="));
	UE_LOG(LogTemp, Warning, TEXT("   CUBE SYSTEM TEST REPORT"));
	UE_LOG(LogTemp, Warning, TEXT("=============================================="));
	UE_LOG(LogTemp, Warning, TEXT("Total Tests: %d"), TotalTests);
	UE_LOG(LogTemp, Warning, TEXT("Passed: %d"), PassedTests);
	UE_LOG(LogTemp, Warning, TEXT("Failed: %d"), TotalTests - PassedTests);
	UE_LOG(LogTemp, Warning, TEXT("Success Rate: %.1f%%"), SuccessRate);
	UE_LOG(LogTemp, Warning, TEXT("=============================================="));
	UE_LOG(LogTemp, Warning, TEXT("Detailed Results:"));

	for (const FTestResult& Result : TestResults)
	{
		FString Status = Result.bPassed ? TEXT("[PASS]") : TEXT("[FAIL]");
		UE_LOG(LogTemp, Warning, TEXT("%s %s: %s"), *Status, *Result.TestName, *Result.Message);
	}

	UE_LOG(LogTemp, Warning, TEXT("=============================================="));

	if (PassedTests == TotalTests)
	{
		UE_LOG(LogTemp, Warning, TEXT("   ALL TESTS PASSED!"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("   SOME TESTS FAILED!"));
	}

	UE_LOG(LogTemp, Warning, TEXT("=============================================="));
}

bool ACC_CubeSystemTester::Test_ManagerInitialization()
{
    bool bPassed = (CubeManager != nullptr);
    FString Message = bPassed
        ? TEXT("CubeWorldManager found and initialized")
        : TEXT("CubeWorldManager not found");

    AddTestResult(TEXT("Manager Initialization"), bPassed, Message);
    return bPassed;
}

bool ACC_CubeSystemTester::Test_GridGeneration()
{
    if (!CubeManager)
    {
        AddTestResult(TEXT("Grid Generation"), false, TEXT("Manager not available"));
        return false;
    }

    int32 ExpectedGridSize = 9; // 3x3
    int32 ActualGridSize = CubeManager->CubeGrid.Num();

    bool bPassed = (ActualGridSize == ExpectedGridSize);
    FString Message = FString::Printf(TEXT("Grid size: %d (Expected: %d)"), ActualGridSize, ExpectedGridSize);

    AddTestResult(TEXT("Grid Generation"), bPassed, Message);
    return bPassed;
}

bool ACC_CubeSystemTester::Test_CubeSpawning()
{
    if (!CubeManager)
    {
        AddTestResult(TEXT("Cube Spawning"), false, TEXT("Manager not available"));
        return false;
    }

    // 중앙 큐브가 Spawn되었는지 확인
    ACC_Cube* CentralCube = CubeManager->FindCube(FIntPoint(1, 1));
    bool bPassed = (CentralCube != nullptr);

    FString Message = bPassed
        ? FString::Printf(TEXT("Central cube spawned at (%d, %d)"), 1, 1)
        : TEXT("Central cube not spawned");

    AddTestResult(TEXT("Cube Spawning"), bPassed, Message);
    return bPassed;
}

bool ACC_CubeSystemTester::Test_FreezeSystem()
{
    if (!CubeManager)
    {
        AddTestResult(TEXT("Freeze System"), false, TEXT("Manager not available"));
        return false;
    }

    // 테스트 큐브 생성
    ACC_Cube* TestCube = CubeManager->FindOrSpawnCube(FIntPoint(0, 0));
    if (!TestCube)
    {
        AddTestResult(TEXT("Freeze System"), false, TEXT("Test cube spawn failed"));
        return false;
    }

    // Freeze 테스트
    TestCube->Unfreeze();
    bool bActiveState = (TestCube->CubeState == ECubeState::Active);

    TestCube->Freeze();
    bool bFrozenState = (TestCube->CubeState == ECubeState::Frozen);

    bool bPassed = bActiveState && bFrozenState;
    FString Message = FString::Printf(TEXT("Active: %s, Frozen: %s"),
        bActiveState ? TEXT("OK") : TEXT("FAIL"),
        bFrozenState ? TEXT("OK") : TEXT("FAIL"));

    AddTestResult(TEXT("Freeze System"), bPassed, Message);
    return bPassed;
}

bool ACC_CubeSystemTester::Test_BoundaryDetection()
{
    if (!CubeManager)
    {
        AddTestResult(TEXT("Boundary Detection"), false, TEXT("Manager not available"));
        return false;
    }

    ACC_Cube* TestCube = CubeManager->FindCube(FIntPoint(1, 1));
    if (!TestCube)
    {
        AddTestResult(TEXT("Boundary Detection"), false, TEXT("Test cube not found"));
        return false;
    }

    // 경계 트리거 확인
    int32 ExpectedTriggers = 4; // Right, Left, Up, Down
    int32 ActualTriggers = TestCube->BoundaryTriggers.Num();

    bool bPassed = (ActualTriggers == ExpectedTriggers);
    FString Message = FString::Printf(TEXT("Boundary triggers: %d (Expected: %d)"),
        ActualTriggers, ExpectedTriggers);

    AddTestResult(TEXT("Boundary Detection"), bPassed, Message);
    return bPassed;
}

bool ACC_CubeSystemTester::Test_CoordinateWrapping()
{
    if (!CubeManager)
    {
        AddTestResult(TEXT("Coordinate Wrapping"), false, TEXT("Manager not available"));
        return false;
    }

    // 순환 계산 테스트
    FIntPoint TestCoord(2, 2); // 오른쪽 하단

    FIntPoint RightWrap = CubeManager->GetNextCubeCoord(TestCoord, EBoundaryDirection::Right);
    FIntPoint DownWrap = CubeManager->GetNextCubeCoord(TestCoord, EBoundaryDirection::Down);

    bool bRightCorrect = (RightWrap.X == 0 && RightWrap.Y == 2); // (2,2) -> (0,2)
    bool bDownCorrect = (DownWrap.X == 2 && DownWrap.Y == 0);    // (2,2) -> (2,0)

    bool bPassed = bRightCorrect && bDownCorrect;
    FString Message = FString::Printf(TEXT("Right wrap: %s, Down wrap: %s"),
        bRightCorrect ? TEXT("OK") : TEXT("FAIL"),
        bDownCorrect ? TEXT("OK") : TEXT("FAIL"));

    AddTestResult(TEXT("Coordinate Wrapping"), bPassed, Message);
    return bPassed;
}

bool ACC_CubeSystemTester::Test_CubeTransition()
{
    if (!CubeManager)
    {
        AddTestResult(TEXT("Cube Transition"), false, TEXT("Manager not available"));
        return false;
    }

    // 전환 전 상태 저장
    FIntPoint BeforeCoord = CubeManager->CurrentCubeCoord;

    // 전환 시뮬레이션 (실제론 안 함 - 테스트용)
    FIntPoint SimulatedNext = CubeManager->GetNextCubeCoord(BeforeCoord, EBoundaryDirection::Right);

    bool bPassed = (SimulatedNext != BeforeCoord);
    FString Message = FString::Printf(TEXT("Before: (%d,%d), After: (%d,%d)"),
        BeforeCoord.X, BeforeCoord.Y, SimulatedNext.X, SimulatedNext.Y);

    AddTestResult(TEXT("Cube Transition"), bPassed, Message);
    return bPassed;
}

bool ACC_CubeSystemTester::Test_ActorManagement()
{
    if (!CubeManager)
    {
        AddTestResult(TEXT("Actor Management"), false, TEXT("Manager not available"));
        return false;
    }

    ACC_Cube* TestCube = CubeManager->FindCube(FIntPoint(1, 1));
    if (!TestCube)
    {
        AddTestResult(TEXT("Actor Management"), false, TEXT("Test cube not found"));
        return false;
    }

    // 테스트 Actor 생성
    AActor* TestActor = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), TestCube->GetCubeCenter(), FRotator::ZeroRotator);

    // 등록
    TestCube->RegisterActor(TestActor);
    bool bRegistered = TestCube->ManagedActors.Contains(TestActor);

    // 해제
    TestCube->UnregisterActor(TestActor);
    bool bUnregistered = !TestCube->ManagedActors.Contains(TestActor);

    // 정리
    if (TestActor)
    {
        TestActor->Destroy();
    }

    bool bPassed = bRegistered && bUnregistered;
    FString Message = FString::Printf(TEXT("Register: %s, Unregister: %s"),
        bRegistered ? TEXT("OK") : TEXT("FAIL"),
        bUnregistered ? TEXT("OK") : TEXT("FAIL"));

    AddTestResult(TEXT("Actor Management"), bPassed, Message);
    return bPassed;
}

// ========================================
// Utilities
// ========================================

void ACC_CubeSystemTester::AddTestResult(const FString& TestName, bool bPassed, const FString& Message)
{
    FTestResult Result;
    Result.TestName = TestName;
    Result.bPassed = bPassed;
    Result.Message = Message;
    TestResults.Add(Result);
}

void ACC_CubeSystemTester::FindCubeManager()
{
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACC_CubeWorldManager::StaticClass(), FoundActors);

    if (FoundActors.Num() > 0)
    {
        CubeManager = Cast<ACC_CubeWorldManager>(FoundActors[0]);
        UE_LOG(LogTemp, Log, TEXT("[Test] Found CubeWorldManager"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[Test] CubeWorldManager not found in level!"));
    }
}