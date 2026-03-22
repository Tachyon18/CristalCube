#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "CC_CycleManager.h"
#include "CC_EnemyManager.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

namespace CristalCube::Tests
{
	static UWorld* CreateTestWorld(FAutomationTestBase& Test)
	{
		UWorld* World = FAutomationEditorCommonUtils::CreateNewMap();
		Test.TestNotNull(TEXT("Created automation map"), World);
		return World;
	}

	static FCycleConfig MakeCycleConfig(const int32 KillsRequired)
	{
		FCycleConfig Config;
		Config.KillsRequired = KillsRequired;
		Config.TimeLimit = 0.0f;
		Config.MaxEnemies = KillsRequired;
		Config.SpawnInterval = 0.0f;
		return Config;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCCPlayerDamageSingleApplicationTest,
	"CristalCube.Regression.Player.Damage.SingleApplication",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCPlayerDamageSingleApplicationTest::RunTest(const FString& Parameters)
{
	using namespace CristalCube::Tests;

	UWorld* World = CreateTestWorld(*this);
	if (!World)
	{
		return false;
	}

	ACC_PlayerCharacter* Player = World->SpawnActor<ACC_PlayerCharacter>();
	TestNotNull(TEXT("Spawned player"), Player);
	if (!Player)
	{
		return false;
	}

	const float AppliedDamage = Player->TakeDamage(25.0f, FDamageEvent(), nullptr, nullptr);

	TestEqual(TEXT("TakeDamage returns applied damage once"), AppliedDamage, 25.0f);
	TestEqual(TEXT("Player health is reduced once"), Player->GetCurrentHealth(), 75.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCCEnemyDeathCountsOnlyRealKillsTest,
	"CristalCube.Regression.Cycle.KillCount.OnlyActualDeaths",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCEnemyDeathCountsOnlyRealKillsTest::RunTest(const FString& Parameters)
{
	using namespace CristalCube::Tests;

	UWorld* World = CreateTestWorld(*this);
	if (!World)
	{
		return false;
	}

	ACC_EnemyManager* EnemyManager = World->SpawnActor<ACC_EnemyManager>();
	ACC_CycleManager* CycleManager = World->SpawnActor<ACC_CycleManager>();

	TestNotNull(TEXT("Spawned enemy manager"), EnemyManager);
	TestNotNull(TEXT("Spawned cycle manager"), CycleManager);
	if (!EnemyManager || !CycleManager)
	{
		return false;
	}

	CycleManager->CycleConfigs = { MakeCycleConfig(1) };
	EnemyManager->OnEnemyKilled.AddDynamic(CycleManager, &ACC_CycleManager::HandleEnemyKilled);
	CycleManager->StartFirstCycle();

	AActor* CleanupEnemy = World->SpawnActor<AActor>();
	AActor* KilledEnemy = World->SpawnActor<AActor>();

	TestNotNull(TEXT("Spawned cleanup enemy"), CleanupEnemy);
	TestNotNull(TEXT("Spawned killed enemy"), KilledEnemy);
	if (!CleanupEnemy || !KilledEnemy)
	{
		return false;
	}

	EnemyManager->RegisterEnemy(CleanupEnemy);
	EnemyManager->RegisterEnemy(KilledEnemy);

	EnemyManager->UnregisterEnemy(CleanupEnemy);
	TestEqual(TEXT("Cleanup unregister does not increase kill count"), CycleManager->GetKillsThisCycle(), 0);

	EnemyManager->NotifyEnemyKilled(KilledEnemy);
	TestEqual(TEXT("Explicit kill notification increases kill count"), CycleManager->GetKillsThisCycle(), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCCCycleClearFlowRegressionTest,
	"CristalCube.Regression.Cycle.ClearFlow.CompletesOnRequiredKills",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCCycleClearFlowRegressionTest::RunTest(const FString& Parameters)
{
	using namespace CristalCube::Tests;

	UWorld* World = CreateTestWorld(*this);
	if (!World)
	{
		return false;
	}

	ACC_CycleManager* CycleManager = World->SpawnActor<ACC_CycleManager>();
	TestNotNull(TEXT("Spawned cycle manager"), CycleManager);
	if (!CycleManager)
	{
		return false;
	}

	CycleManager->CycleConfigs = { MakeCycleConfig(2) };
	CycleManager->StartFirstCycle();

	CycleManager->HandleEnemyKilled(nullptr);
	TestTrue(TEXT("Cycle remains active before kill target is met"), CycleManager->bCycleActive);
	TestEqual(TEXT("Kill count after first kill"), CycleManager->GetKillsThisCycle(), 1);

	CycleManager->HandleEnemyKilled(nullptr);
	TestFalse(TEXT("Cycle stops once required kills are reached"), CycleManager->bCycleActive);
	TestEqual(TEXT("Kill count reaches requirement"), CycleManager->GetKillsThisCycle(), 2);
	TestEqual(TEXT("Kill progress is clamped to completion"), CycleManager->GetKillProgress(), 1.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCCCubeClearRewardApplicationTest,
	"CristalCube.Regression.Rewards.ApplyHealAndStatBoost",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCCubeClearRewardApplicationTest::RunTest(const FString& Parameters)
{
	using namespace CristalCube::Tests;

	UWorld* World = CreateTestWorld(*this);
	if (!World)
	{
		return false;
	}

	ACC_PlayerCharacter* Player = World->SpawnActor<ACC_PlayerCharacter>();
	TestNotNull(TEXT("Spawned player"), Player);
	if (!Player)
	{
		return false;
	}

	Player->TakeDamage(40.0f, FDamageEvent(), nullptr, nullptr);
	TestEqual(TEXT("Player health after taking damage"), Player->GetCurrentHealth(), 60.0f);

	FCubeClearReward HealReward;
	HealReward.RewardType = ECubeClearRewardType::HealFull;
	HealReward.DisplayName = FText::FromString(TEXT("Full Heal"));

	TestTrue(TEXT("Heal reward applies"), Player->ApplyCubeClearReward(HealReward));
	TestEqual(TEXT("Heal reward restores full health"), Player->GetCurrentHealth(), 100.0f);

	FCubeClearReward StatReward;
	StatReward.RewardType = ECubeClearRewardType::StatBoost;
	StatReward.StatUpgradeType = EUpgradeType::Health;
	StatReward.StatValue = 0.5f;
	StatReward.DisplayName = FText::FromString(TEXT("Max Health Up"));

	TestTrue(TEXT("Stat reward applies"), Player->ApplyCubeClearReward(StatReward));
	TestEqual(TEXT("Health multiplier increases"), Player->GetPlayerStats().BasicStats.HealthMultiplier, 1.5f);
	TestEqual(TEXT("Max health scales from the captured base value"), Player->GetMaxHealth(), 150.0f);
	TestEqual(TEXT("Full-health player stays full after health boost"), Player->GetCurrentHealth(), 150.0f);

	return true;
}

#endif
