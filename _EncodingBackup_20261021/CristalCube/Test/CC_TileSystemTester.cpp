// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_TileSystemTester.h"
#include "CC_TestActor.h"
#include "../CC_TileManager.h"
#include "../Gameplay/CC_Tile.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

ACC_TileSystemTester::ACC_TileSystemTester()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ACC_TileSystemTester::BeginPlay()
{
	Super::BeginPlay();

	// TileManager 찾기
	//TileManager = ACC_TileManager::Get(GetWorld());

	//if (bAutoRunTests)
	//{
	//	// 딜레이 후 테스트 시작
	//	FTimerHandle TimerHandle;
	//	GetWorld()->GetTimerManager().SetTimer(
	//		TimerHandle,
	//		this,
	//		&ACC_TileSystemTester::RunAllTests,
	//		TestStartDelay,
	//		false
	//	);
	//}
}

// Called every frame
void ACC_TileSystemTester::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

//void ACC_TileSystemTester::RunAllTests()
//{
//	UE_LOG(LogTemp, Warning, TEXT("=============================================="));
//	UE_LOG(LogTemp, Warning, TEXT("   TILE SYSTEM AUTOMATED TEST STARTED"));
//	UE_LOG(LogTemp, Warning, TEXT("=============================================="));
//
//	TestsPassed = 0;
//	TestsFailed = 0;
//	TestLog.Empty();
//
//	Test_TileSystemInitialization();
//	Test_TileGeneration();
//	Test_ActorRegistration();
//	Test_TileActivation();
//	Test_PlayerMovementSimulation();
//	Test_ActorDeactivation();
//	Test_PerformanceMeasurement();
//	Test_WrapAroundSystem();
//
//	PrintTestReport();
//}
//
//bool ACC_TileSystemTester::Test_TileSystemInitialization()
//{
//	FString TestName = TEXT("[TileSystemTester] Tile System Initialization");
//
//	if (!TileManager)
//	{
//		LogTest(TestName, false, TEXT("TileManager not found!"));
//		return false;
//	}
//
//	int32 TileCount = TileManager->GetAllTiles().Num();
//
//	if (TileCount != 9)
//	{
//		LogTest(TestName, false, FString::Printf(TEXT("Expected 9 tiles, found %d"), TileCount));
//		return false;
//	}
//
//	LogTest(TestName, true, TEXT("TileManager initialized with 9 tiles"));
//	return true;
//}
//
//bool ACC_TileSystemTester::Test_TileGeneration()
//{
//	FString TestName = TEXT("Tile Generation & Layout");
//
//	for (int32 i = 0; i < 9; ++i)
//	{
//		ACC_Tile* Tile = TileManager->GetTileByIndex(i);
//		if (!Tile)
//		{
//			LogTest(TestName, false, FString::Printf(TEXT("Tile %d not found"), i));
//			return false;
//		}
//
//		int32 TileIndex = Tile->GetTileIndex();
//
//		if (TileIndex != i)
//		{
//			LogTest(TestName, false, FString::Printf(TEXT("Tile %d has wrong index: %d"), i, TileIndex));
//			return false;
//		}
//	}
//
//	LogTest(TestName, true, TEXT("All 9 tiles generated correctly"));
//	return true;
//}
//
//bool ACC_TileSystemTester::Test_ActorRegistration()
//{
//	FString TestName = TEXT("Actor Registration/Unregistration");
//
//	if (!TestActorClass)
//	{
//		LogTest(TestName, false, TEXT("TestActorClass not set!"));
//		return false;
//	}
//
//	ACC_TestActor* TestActor = SpawnTestActorAtTile(4);
//
//	if (!TestActor)
//	{
//		LogTest(TestName, false, TEXT("Failed to spawn test actor"));
//		return false;
//	}
//
//	FPlatformProcess::Sleep(0.1f);
//
//	ACC_Tile* Tile4 = TileManager->GetTileByIndex(4);
//	TArray<AActor*> RegisteredActors = Tile4->GetRegisteredActors();
//
//	if (!RegisteredActors.Contains(TestActor))
//	{
//		LogTest(TestName, false, TEXT("Actor not registered in Tile 4"));
//		TestActor->Destroy();
//		return false;
//	}
//
//	TestActor->Destroy();
//	FPlatformProcess::Sleep(0.1f);
//
//	if (RegisteredActors.Contains(TestActor))
//	{
//		LogTest(TestName, false, TEXT("Actor not unregistered from Tile 4"));
//		return false;
//	}
//
//	LogTest(TestName, true, TEXT("Actor registration/unregistration works correctly"));
//	return true;
//}
//
//bool ACC_TileSystemTester::Test_TileActivation()
//{
//	FString TestName = TEXT("Tile Activation/Deactivation");
//
//	TileManager->ActivateAllTiles();
//	FPlatformProcess::Sleep(0.1f);
//
//	for (int32 i = 0; i < 9; ++i)
//	{
//		ACC_Tile* Tile = TileManager->GetTileByIndex(i);
//		if (!Tile->IsTileActive())
//		{
//			LogTest(TestName, false, FString::Printf(TEXT("Tile %d not activated"), i));
//			return false;
//		}
//	}
//
//	TileManager->ActivateOnlyTile(0);
//	FPlatformProcess::Sleep(0.1f);
//
//	ACC_Tile* Tile0 = TileManager->GetTileByIndex(0);
//	if (!Tile0->IsTileActive())
//	{
//		LogTest(TestName, false, TEXT("Tile 0 not activated"));
//		return false;
//	}
//
//	for (int32 i = 1; i < 9; ++i)
//	{
//		ACC_Tile* Tile = TileManager->GetTileByIndex(i);
//
//		if (Tile->IsTileActive())
//		{
//			LogTest(TestName, false, FString::Printf(TEXT("Tile %d should be deactivated"), i));
//			return false;
//		}
//	}
//
//	LogTest(TestName, true, TEXT("Tile activation/deactivation works correctly"));
//	return true;
//}
//
//bool ACC_TileSystemTester::Test_PlayerMovementSimulation()
//{
//	FString TestName = TEXT("Player Movement Simulation");
//
//	int32 Path[] = { 4, 5, 8, 7, 4 };
//
//	for (int32 TileIndex : Path)
//	{
//		TileManager->ActivateTilesAroundPlayer(TileIndex);
//		FPlatformProcess::Sleep(0.1f);
//
//		ACC_Tile* ActiveTile = TileManager->GetTileByIndex(TileIndex);
//		if (!ActiveTile->IsTileActive())
//		{
//			LogTest(TestName, false, FString::Printf(TEXT("Tile %d should be active"), TileIndex));
//			return false;
//		}
//
//		for (int32 i = 0; i < 9; ++i)
//		{
//			if (i != TileIndex)
//			{
//				ACC_Tile* Tile = TileManager->GetTileByIndex(i);
//				if (Tile->IsTileActive())
//				{
//					LogTest(TestName, false, FString::Printf(TEXT("Tile %d should be inactive"), i));
//					return false;
//				}
//			}
//		}
//	}
//
//	LogTest(TestName, true, TEXT("Player movement simulation successful"));
//	return true;
//}
//
//bool ACC_TileSystemTester::Test_ActorDeactivation()
//{
//	FString TestName = TEXT("Actor Deactivation on Tile Switch");
//
//	if (!TestActorClass)
//	{
//		LogTest(TestName, false, TEXT("TestActorClass not set!"));
//		return false;
//	}
//
//	TArray<ACC_TestActor*> TestActors;
//	for (int32 i = 0; i < 5; ++i)
//	{
//		ACC_TestActor* TestActor = SpawnTestActorAtTile(4);
//		if (TestActor)
//		{
//			TestActors.Add(TestActor);
//		}
//	}
//
//	FPlatformProcess::Sleep(0.2f);
//
//	TileManager->ActivateTilesAroundPlayer(4);
//	FPlatformProcess::Sleep(0.1f);
//
//	for (ACC_TestActor* TestActor : TestActors)
//	{
//		if (!TestActor->IsActorTickEnabled())
//		{
//			LogTest(TestName, false, TEXT("TestActor should be active in Tile 4"));
//			CleanupTestActors();
//			return false;
//		}
//	}
//
//	TileManager->ActivateTilesAroundPlayer(5);
//	FPlatformProcess::Sleep(0.1f);
//
//	for (ACC_TestActor* TestActor : TestActors)
//	{
//		if (TestActor->IsActorTickEnabled())
//		{
//			LogTest(TestName, false, TEXT("TestActor should be deactivated when player leaves tile"));
//			CleanupTestActors();
//			return false;
//		}
//	}
//
//	TileManager->ActivateTilesAroundPlayer(4);
//	FPlatformProcess::Sleep(0.1f);
//
//	for (ACC_TestActor* TestActor : TestActors)
//	{
//		if (!TestActor->IsActorTickEnabled())
//		{
//			LogTest(TestName, false, TEXT("TestActor should be reactivated when player returns"));
//			CleanupTestActors();
//			return false;
//		}
//	}
//
//	for (ACC_TestActor* TestActor : TestActors)
//	{
//		if (TestActor)
//		{
//			TestActor->Destroy();
//		}
//	}
//
//	LogTest(TestName, true, TEXT("TestActor deactivation system works correctly"));
//	return true;
//}
//
//bool ACC_TileSystemTester::Test_PerformanceMeasurement()
//{
//	FString TestName = TEXT("Performance Measurement");
//
//	TArray<ACC_TestActor*> PerformanceTestActors;
//	for (int32 i = 0; i < NumPerformanceTestActors; ++i)
//	{
//		int32 TileIndex = i % 9; 
//		ACC_TestActor* TestActor = SpawnTestActorAtTile(TileIndex);
//		if (TestActor)
//		{
//			PerformanceTestActors.Add(TestActor);
//		}
//	}
//
//	FPlatformProcess::Sleep(0.3f);
//
//	TileManager->ActivateTilesAroundPlayer(4);
//	FPlatformProcess::Sleep(0.1f);
//
//	int32 ActiveEnemies = 0;
//	for (ACC_TestActor* TestActor : PerformanceTestActors)
//	{
//		if (TestActor && TestActor->IsActorTickEnabled())
//		{
//			ActiveEnemies++;
//		}
//	}
//
//	int32 ExpectedActive = NumPerformanceTestActors / 9; 
//	if (FMath::Abs(ActiveEnemies - ExpectedActive) > 2)
//	{
//		LogTest(TestName, false, FString::Printf(TEXT("Expected ~%d active enemies, found %d"), ExpectedActive, ActiveEnemies));
//		CleanupTestActors();
//		return false;
//	}
//
//	float ReductionPercent = (1.0f - (float)ActiveEnemies / NumPerformanceTestActors) * 100.0f;
//	FString Message = FString::Printf(TEXT("%d/%d enemies active (%.1f%% reduction)"),
//		ActiveEnemies, NumPerformanceTestActors, ReductionPercent);
//
//	for (ACC_TestActor* TestActor : PerformanceTestActors)
//	{
//		if (TestActor)
//		{
//			TestActor->Destroy();
//		}
//	}
//
//	LogTest(TestName, true, Message);
//	return true;
//}
//
//bool ACC_TileSystemTester::Test_WrapAroundSystem()
//{
//	FString TestName = TEXT("Wrap Around System");
//
//	FVector Tile2Center = TileManager->GetTileByIndex(2)->GetTileCenter();
//	FVector Tile0Center = TileManager->GetTileByIndex(0)->GetTileCenter();
//
//	FVector TestPos = Tile2Center + FVector(1000.0f, 0.0f, 0.0f);
//
//	FVector WrappedPos = TileManager->HandleWrapAround(TestPos, 2, 0);
//
//	if (WrappedPos.Equals(TestPos, 1.0f))
//	{
//		LogTest(TestName, false, TEXT("[TileSystemTester] Wrap around did not change position"));
//		return false;
//	}
//
//	if (WrappedPos.X >= TestPos.X)
//	{
//		LogTest(TestName, false, TEXT("[TileSystemTester] Wrap around moved in wrong direction"));
//		return false;
//	}
//
//	LogTest(TestName, true, FString::Printf(TEXT("Wrap around successful: %.1f → %.1f"), TestPos.X, WrappedPos.X));
//	return true;
//}
//
//void ACC_TileSystemTester::PrintTestReport()
//{
//	UE_LOG(LogTemp, Warning, TEXT(""));
//	UE_LOG(LogTemp, Warning, TEXT("=============================================="));
//	UE_LOG(LogTemp, Warning, TEXT("   TILE SYSTEM TEST REPORT"));
//	UE_LOG(LogTemp, Warning, TEXT("=============================================="));
//	UE_LOG(LogTemp, Warning, TEXT("Total Tests: %d"), TestsPassed + TestsFailed);
//	UE_LOG(LogTemp, Warning, TEXT("Passed: %d"), TestsPassed);
//	UE_LOG(LogTemp, Warning, TEXT("Failed: %d"), TestsFailed);
//	UE_LOG(LogTemp, Warning, TEXT("Success Rate: %.1f%%"), (float)TestsPassed / (TestsPassed + TestsFailed) * 100.0f);
//	UE_LOG(LogTemp, Warning, TEXT("=============================================="));
//	UE_LOG(LogTemp, Warning, TEXT(""));
//
//	UE_LOG(LogTemp, Warning, TEXT("Detailed Results:"));
//	for (const FString& Log : TestLog)
//	{
//		UE_LOG(LogTemp, Warning, TEXT("%s"), *Log);
//	}
//
//	UE_LOG(LogTemp, Warning, TEXT(""));
//	UE_LOG(LogTemp, Warning, TEXT("=============================================="));
//
//	if (TestsFailed == 0)
//	{
//		UE_LOG(LogTemp, Warning, TEXT("[TileSystemTester] ALL TESTS PASSED!"));
//	}
//	else
//	{
//		UE_LOG(LogTemp, Error, TEXT("[TileSystemTester] SOME TESTS FAILED!"));
//	}
//	UE_LOG(LogTemp, Warning, TEXT("=============================================="));
//}
//
//void ACC_TileSystemTester::CleanupTestActors()
//{
//	for (ACC_TestActor* TestActor : SpawnedTestActors)
//	{
//		if (TestActor)
//		{
//			TestActor->Destroy();
//		}
//	}
//	SpawnedTestActors.Empty();
//}
//
//void ACC_TileSystemTester::LogTest(const FString& TestName, bool bPassed, const FString& Message)
//{
//	if (bPassed)
//	{
//		TestsPassed++;
//		FString Log = FString::Printf(TEXT("[PASS] %s: %s"), *TestName, *Message);
//		TestLog.Add(Log);
//		UE_LOG(LogTemp, Log, TEXT("[TileSystemTester] %s"), *Log);
//	}
//	else
//	{
//		TestsFailed++;
//		FString Log = FString::Printf(TEXT("[FAIL] %s: %s"), *TestName, *Message);
//		TestLog.Add(Log);
//		UE_LOG(LogTemp, Error, TEXT("[TileSystemTester] %s"), *Log);
//	}
//}
//
//ACC_TestActor* ACC_TileSystemTester::SpawnTestActorAtTile(int32 TileIndex)
//{
//	if (!TestActorClass) return nullptr;
//
//	ACC_Tile* Tile = TileManager->GetTileByIndex(TileIndex);
//	if (!Tile) return nullptr;
//
//	FVector SpawnLocation = Tile->GetTileCenter() + FVector(0, 0, 150);
//	FActorSpawnParameters SpawnParams;
//	SpawnParams.Owner = this;
//
//	ACC_TestActor* TestActor = GetWorld()->SpawnActor<ACC_TestActor>(
//		TestActorClass,
//		SpawnLocation,
//		FRotator::ZeroRotator,
//		SpawnParams
//	);
//
//	if (TestActor)
//	{
//		SpawnedTestActors.Add(TestActor);
//	}
//
//	return TestActor;
//}