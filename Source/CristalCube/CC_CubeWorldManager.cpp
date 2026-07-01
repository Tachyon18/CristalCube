// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_CubeWorldManager.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Gameplay/CC_Cube.h"
#include "Gameplay/CC_CubeMoodComponent.h"
#include "Characters/CC_EnemyCharacter.h"
#include "CC_EnemyManager.h"
#include "CC_EnemySpawner.h"

ACC_CubeWorldManager* ACC_CubeWorldManager::Instance = nullptr;

static TAutoConsoleVariable<float> CVarCubeDebugLogInterval(
	TEXT("CC.Cube.DebugLogInterval"),
	0.0f,
	TEXT("0보다 크면 그 주기(초)마다 PrintDebugInfo()를 자동 출력. 0이면 비활성."),
	ECVF_Cheat);

// Sets default values
ACC_CubeWorldManager::ACC_CubeWorldManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ACC_CubeWorldManager::BeginPlay()
{
	Instance = this;

	Super::BeginPlay();
	
	InitializeSystem();

	FTimerHandle BindTimer;
	GetWorldTimerManager().SetTimer(BindTimer, [this]()
		{
			if (ACC_EnemyManager* EnemyMgr = ACC_EnemyManager::Get(this))
			{
				EnemyMgr->OnEnemyUnregistered.AddDynamic(
					this, &ACC_CubeWorldManager::OnEmenyUnregistered);
				UE_LOG(LogTemp, Log,
					TEXT("[CubeWorldManager] OnEnemyUnregistered bound."));
			}
			else
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[CubeWorldManager] EnemyManager not found — bind skipped."));
			}
		}, 0.2f, false);
}

void ACC_CubeWorldManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Instance == this) Instance = nullptr;
	Super::EndPlay(EndPlayReason);
}

// Called every frame
void ACC_CubeWorldManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bAutoTest)
	{
		DrawAllCubes();
	}

	// D-05 소크 테스트: 설정된 주기마다 자동으로 디버그 정보 로그
	const float LogInterval = CVarCubeDebugLogInterval.GetValueOnGameThread();
	if (LogInterval > 0.f)
	{
		DebugLogTimer += DeltaTime;
		if (DebugLogTimer >= LogInterval)
		{
			DebugLogTimer = 0.f;
			PrintDebugInfo();
		}
	}
}

ACC_CubeWorldManager* ACC_CubeWorldManager::Get(const UObject* WorldContextObject)
{
	if (Instance) return Instance;

	// fallback
	if (UWorld* World = GEngine->GetWorldFromContextObject(
		WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		TArray<AActor*> Found;
		UGameplayStatics::GetAllActorsOfClass(
			World, ACC_CubeWorldManager::StaticClass(), Found);
		if (Found.Num() > 0)
			Instance = Cast<ACC_CubeWorldManager>(Found[0]);
	}
	return Instance;
}

bool ACC_CubeWorldManager::IsValidCoordinate(FIntPoint Coord) const
{
	return CubeGrid.Contains(Coord);
}

void ACC_CubeWorldManager::NotifyCubeWaveCleared(FIntPoint Coord)
{
	if (CubeGrid.Contains(Coord))
	{
		CubeGrid[Coord].bCleared = true;  // 기존에 선언만 돼 있던 필드 — 드디어 실사용
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Manager] NotifyCubeWaveCleared — (%d,%d) not found in CubeGrid"),
			Coord.X, Coord.Y);
	}

	UE_LOG(LogTemp, Warning, TEXT("[Manager] Cube (%d,%d) CUBE CLEAR — true wave-clear achieved."),
		Coord.X, Coord.Y);

	OnCubeWaveCleared.Broadcast(Coord);
}

void ACC_CubeWorldManager::RegisterPersistentEnemy(AActor* Enemy)
{
	if (!Enemy) return;

	PersistentEnemyList.AddUnique(Enemy);
	PersistentEnemyCount = PersistentEnemyList.Num();
	UE_LOG(LogTemp, Log, TEXT("[Manager] Persistent enemy registered. Total: %d"),
		PersistentEnemyCount);

	CheckLockCondition();
}

void ACC_CubeWorldManager::UnregisterPersistentEnemy(AActor* Enemy)
{
	if (!Enemy) return;

	PersistentEnemyList.Remove(Enemy);
	PersistentEnemyCount = PersistentEnemyList.Num();
	UE_LOG(LogTemp, Log, TEXT("[Manager] Persistent enemy unregistered. Total: %d"),
		PersistentEnemyCount);

	CheckLockCondition();
}

void ACC_CubeWorldManager::CheckLockCondition()
{
	if (!bCubeLocked && PersistentEnemyCount >= LockThreshold)
	{
		bCubeLocked = true;
		if (ActiveCube)  ActiveCube->SetLockWall(true);
		UE_LOG(LogTemp, Warning, TEXT("[Manager] Cube LOCKED! Count: %d"), PersistentEnemyCount);
	}
	else if (bCubeLocked && PersistentEnemyCount < LockThreshold)
	{
		bCubeLocked = false;
		if (ActiveCube) ActiveCube->SetLockWall(false);
		UE_LOG(LogTemp, Warning, TEXT("[Manager] Cube UNLOCKED! Count: %d"), PersistentEnemyCount);
	}
}

void ACC_CubeWorldManager::TeleportPersistentEnemiesToCube(ACC_Cube* TargetCube)
{
	if (!TargetCube) return;

	FVector CubeCenter = TargetCube->GetCubeCenter();
	float Spread = 600.0f;

	int32 TeleportedCount = 0;

	for (AActor* Enemy : PersistentEnemyList)
	{
		if (!IsValid(Enemy)) continue;

		FVector Offset(
			FMath::RandRange(-Spread, Spread),
			FMath::RandRange(-Spread, Spread),
			0.0f
		);

		Enemy->SetActorLocation(CubeCenter + Offset, false, nullptr, ETeleportType::TeleportPhysics);

		++TeleportedCount;
	}

	if (TeleportedCount > 0)
	{
		UE_LOG(LogTemp, Log,
			TEXT("[Manager] Teleported %d persistent enemies to Cube (%d,%d)"),
			TeleportedCount, TargetCube->CubeCoordinate.X, TargetCube->CubeCoordinate.Y);
	}
}

int32 ACC_CubeWorldManager::GetTotalManagedActorsCount() const
{
	int32 Total = 0;
	for (const ACC_Cube* Cube : LoadedCubes)
	{
		if (Cube) Total += Cube->ManagedActors.Num();
	}
	return Total;
}

void ACC_CubeWorldManager::InitializeSystem()
{
	UE_LOG(LogTemp, Warning, TEXT("=============================================="));
	UE_LOG(LogTemp, Warning, TEXT("   CUBE WORLD MANAGER - INITIALIZATION"));
	UE_LOG(LogTemp, Warning, TEXT("=============================================="));

	BuildCoordinateThemeCache();
	InitializeCubeGrid();
	
	int32 PreSpawnedCount = 0;
	for (const auto& GridEntry : CubeGrid)
	{
		if (SpawnCube(GridEntry.Key))
		{
			++PreSpawnedCount;
		}
	}
	UE_LOG(LogTemp, Log, TEXT("[Manager] Pre-spawned %d / %d cubes"),
		PreSpawnedCount, CubeGrid.Num());

	CurrentCubeCoord = StartCoordinate;
	ActiveCube = FindCube(CurrentCubeCoord);

	if (ActiveCube)
	{
		ActiveCube->Unfreeze();
		UE_LOG(LogTemp, Log, TEXT("[Manager] Active cube set to: %d, %d"),
			CurrentCubeCoord.X, CurrentCubeCoord.Y);
	}
	else
	{
		UE_LOG(LogTemp, Error,
			TEXT("[Manager] StartCoordinate (%d,%d)에 해당하는 Cube가 없습니다 — "
				"CubeGrid/CustomValidCells 설정을 확인하세요."),
			CurrentCubeCoord.X, CurrentCubeCoord.Y);
	}
	MovePlayerToCube(CurrentCubeCoord);

	bSystemReady = true;
	UE_LOG(LogTemp, Warning, TEXT("[Manager] System initialized successfully!"));
	UE_LOG(LogTemp, Warning, TEXT("=============================================="));

	OnCubeSystemReady.Broadcast();
}

void ACC_CubeWorldManager::InitializeCubeGrid()
{
	CubeGrid.Empty();

	if(bUseCustomLayout)
	{
		// 커스텀 레이아웃: 지정된 셀만 등록
		for (const FIntPoint& Cell : CustomValidCells)
		{
			FCubeData NewCubeData;
			NewCubeData.Coordinate = Cell;
			NewCubeData.State = ECubeState::Unloaded;
			CubeGrid.Add(Cell, NewCubeData);
		}
	}
	else
	{
		// 직사각형 모드: GridWidth x GridHeight
		for (int32 X = 0; X < GridHeight; X++)
		{
			for (int32 Y = 0; Y < GridWidth; Y++)
			{
				FCubeData NewCubeData;
				NewCubeData.Coordinate = FIntPoint(X, Y);
				NewCubeData.State = ECubeState::Unloaded;
				CubeGrid.Add(FIntPoint(X, Y), NewCubeData);
			}
		}
	}
	

	UE_LOG(LogTemp, Log, TEXT("[Manager] Initialized %d cube data entries"), CubeGrid.Num());
}

ACC_Cube* ACC_CubeWorldManager::SpawnCube(FIntPoint Coordinate)
{
	ACC_Cube* ExistingCube = FindCube(Coordinate);
	if (ExistingCube)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Manager] Cube (%d, %d) already exists"),
			Coordinate.X, Coordinate.Y);
		return ExistingCube;
	}

	if (!CubeClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[Manager] CubeClass is not set!"));
		return nullptr;
	}

	// Spawn
	FVector SpawnLocation(-Coordinate.X * CubeSize, Coordinate.Y * CubeSize, 0.0f);
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	FTransform SpawnTransform(FRotator::ZeroRotator, SpawnLocation);

	//ACC_Cube* NewCube = GetWorld()->SpawnActor<ACC_Cube>(CubeClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);

	// BeginPlay 이전에 프로퍼티를 설정하기 위해 Deferred 사용
	ACC_Cube* NewCube = GetWorld()->SpawnActorDeferred<ACC_Cube>(CubeClass, SpawnTransform, this);

	if (!NewCube)
	{
		UE_LOG(LogTemp, Error, TEXT("[Manager] Failed to spawn cube at (%d, %d)"),
			Coordinate.X, Coordinate.Y);
		return nullptr;
	}

	// BeginPlay 전에 CubeSize 확정
	NewCube->CubeSize = CubeSize;

	// BeginPlay 실행
	UGameplayStatics::FinishSpawningActor(NewCube, SpawnTransform);


	// ① 테마 결정 — WorldManager 레벨 우선, 없으면 Cube Blueprint 디폴트 유지
	ECubeTheme AssignedTheme = AssignThemeForCoord(Coordinate);
	if (AssignedTheme == ECubeTheme::None)
	{
		AssignedTheme = NewCube->CubeTheme;  // Blueprint 설정 폴백
	}
	else
	{
		NewCube->CubeTheme = AssignedTheme;
	}
	
	// ② ApplyTheme 먼저 — ScatterComponent에 메시 주입 (Generate 전!)
	if (AssignedTheme != ECubeTheme::None)
	{
		if (const FCubeThemeData* ThemeData = ThemeDataMap.Find(AssignedTheme))
		{
			NewCube->ApplyTheme(*ThemeData);
		}
		else
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[Manager] 테마 %s 에 해당하는 ThemeDataMap 항목 없음. "
					"BP_CubeWorldManager의 ThemeDataMap을 확인하세요."),
				*UEnum::GetValueAsString(AssignedTheme));
		}
	}

	// ③ Mood 데이터 주입 — Cube 자신의 MoodComponent엔 "내 테마 하나"만 들어가면 됨
	if (AssignedTheme != ECubeTheme::None && NewCube->MoodComponent)
	{
		if (const FCubeMoodSettings* MoodData = MoodSettingsMap.Find(AssignedTheme))
		{
			NewCube->MoodComponent->MoodSettings.SetNum(1);
			NewCube->MoodComponent->MoodSettings[0] = *MoodData;
		}
		else
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[Manager] 테마 %s 에 해당하는 MoodSettingsMap 항목 없음. BP_CubeWorldManager를 확인하세요."),
				*UEnum::GetValueAsString(AssignedTheme));
		}
	}

	UE_LOG(LogTemp, Error, TEXT("[SpawnCube] Calling InitializeCube with: (%d,%d)"),
		Coordinate.X, Coordinate.Y);
	NewCube->InitializeCube(Coordinate);
	LoadedCubes.Add(NewCube);

	if (CubeGrid.Contains(Coordinate))
	{
		CubeGrid[Coordinate].State = ECubeState::Active;
	}

	//if (NewCube->CubeTheme != ECubeTheme::None)
	//{
	//	if (const FCubeThemeData* ThemeData = ThemeDataMap.Find(NewCube->CubeTheme))
	//	{
	//		NewCube->ApplyTheme(*ThemeData);
	//	}
	//}

	UE_LOG(LogTemp, Log, TEXT("[Manager] Spawned cube at (%d,%d) - Location: %s"),
		Coordinate.X, Coordinate.Y, *SpawnLocation.ToString());

	SetupCubeContent(NewCube);

	NewCube->Freeze();

	return NewCube;
}

void ACC_CubeWorldManager::DespawnCube(FIntPoint Coordinate)
{
	ACC_Cube* Cube = FindCube(Coordinate);
	if (!Cube)
		return;

	LoadedCubes.Remove(Cube);
	Cube->Destroy();

	if (CubeGrid.Contains(Coordinate))
	{
		CubeGrid[Coordinate].State = ECubeState::Unloaded;
	}

	UE_LOG(LogTemp, Log, TEXT("[Manager] Despawned cube at (%d, %d)"), Coordinate.X, Coordinate.Y);
}

ACC_Cube* ACC_CubeWorldManager::FindOrSpawnCube(FIntPoint Coordinate)
{
	ACC_Cube* Cube = FindCube(Coordinate);
	if (Cube)
		return Cube;

	return SpawnCube(Coordinate);
}

ACC_Cube* ACC_CubeWorldManager::FindCube(FIntPoint CubeCoord) const
{
	for (ACC_Cube* Cube : LoadedCubes)
	{
		if (Cube && Cube->CubeCoordinate == CubeCoord)
		{
			return Cube;
		}
	}
	return nullptr;
}

void ACC_CubeWorldManager::SetupCubeContent(ACC_Cube* NewCube)
{
	if (!NewCube)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[Manager] SetupCubeContent — Cube (%d,%d)"),
		NewCube->CubeCoordinate.X, NewCube->CubeCoordinate.Y);

	LinkSpawnersToNearestCube(NewCube);
}

void ACC_CubeWorldManager::RequestTransition(EBoundaryDirection Direction)
{
	if (bCubeLocked)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Manager] Cube is LOCKED. Clear persistent enemies first!"));
		return;
	}

	if (bIsTransitioning)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Manager] Already transitioning, ignoring request"));
		return;
	}

	FIntPoint NextCoord = GetNextCubeCoord(CurrentCubeCoord, Direction);

	if (NextCoord == CurrentCubeCoord)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Manager] No valid neighbor in direction: %s"),
			*UEnum::GetValueAsString(Direction));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Manager] Transition started: (%d,%d) -> (%d,%d)"),
		CurrentCubeCoord.X, CurrentCubeCoord.Y, NextCoord.X, NextCoord.Y);

	bIsTransitioning = true;
	LastTransitionDirection = Direction;

	PerformTransition(NextCoord);

	bIsTransitioning = false;

	UE_LOG(LogTemp, Warning, TEXT("[Manager] Transition complete!"));
}

FIntPoint ACC_CubeWorldManager::GetNextCubeCoord(FIntPoint Current, EBoundaryDirection Direction) const
{
	FIntPoint Delta = FIntPoint::ZeroValue;

	switch (Direction)
	{
	case EBoundaryDirection::Right: Delta = FIntPoint(0, 1); break;
	case EBoundaryDirection::Left:  Delta = FIntPoint(0, -1); break;
	case EBoundaryDirection::Up:    Delta = FIntPoint(-1, 0); break;
	case EBoundaryDirection::Down:  Delta = FIntPoint(1, 0); break;
	}

	FIntPoint Next = Current + Delta;

	if (IsValidCoordinate(Next)) return Next;

	// 유효하지 않으면 같은 행/열에서 반대편 끝으로 랩핑
	// 같은 행(Left/Right) 또는 같은 열(Up/Down)의 유효 셀 수집
	TArray<FIntPoint> SameLine;
	for (const auto& Pair : CubeGrid)
	{
		bool bSameAxis = (Direction == EBoundaryDirection::Left ||
			Direction == EBoundaryDirection::Right)
			? (Pair.Key.X == Current.X)   // 같은 행
			: (Pair.Key.Y == Current.Y);  // 같은 열
		if (bSameAxis)
			SameLine.Add(Pair.Key);
	}

	if (SameLine.Num() == 0)
		return Current; // 이동 불가

	// 이동 방향 반대편 끝 셀 반환
	SameLine.Sort([&](const FIntPoint& A, const FIntPoint& B)
		{
			return (Direction == EBoundaryDirection::Right ||
				Direction == EBoundaryDirection::Left)
				? A.Y < B.Y
				: A.X < B.X;
		});

	// Right/Down → 첫 번째(최솟값), Left/Up → 마지막(최댓값)
	bool bWrapToStart = (Direction == EBoundaryDirection::Right ||
		Direction == EBoundaryDirection::Down);

	Next = bWrapToStart ? SameLine[0] : SameLine.Last();

	UE_LOG(LogTemp, Log, TEXT("[Manager] Wrapped (%d,%d) -> (%d,%d)"),
		Current.X, Current.Y, Next.X, Next.Y);

	return Next;
}

void ACC_CubeWorldManager::PerformTransition(FIntPoint NextCoord)
{
	UE_LOG(LogTemp, Log, TEXT("[Manager] Performing transition to (%d, %d)"), NextCoord.X, NextCoord.Y);

	// 1. 다음 큐브 먼저 준비 (Persistent 이전에 위치 정보 필요)
	ACC_Cube* NextCube = FindOrSpawnCube(NextCoord);
	if (!NextCube)
	{
		UE_LOG(LogTemp, Error, TEXT("[Manager] Failed to get next cube!"));
		return;
	}

	// 2. Persistent Enemy 이전 — Freeze 전에 처리해야 소속 문제 없음
	TeleportPersistentEnemiesToCube(NextCube);

	// 3. 이전 큐브 Freeze
	if (ActiveCube)
	{
		ActiveCube->Freeze();
	}

	// 4. 다음 큐브 Unfreeze
	NextCube->Unfreeze();

	// 5. 플레이어 이동
	FVector NewPlayerPos = CalculatePlayerPositionInCube(NextCube, LastTransitionDirection);
	ACharacter* Player = GetPlayerCharacter();
	if (Player)
	{
		// 텔레포트 위치에 Enemy 등이 겹쳐 있으면 엔진이 즉시 밀어내면서
		// 방금 들어온 경계로 도로 튕겨나가는 문제 방지 — 텔레포트 순간만 충돌 끔
		Player->SetActorEnableCollision(false);

		Player->SetActorLocation(NewPlayerPos);
		UE_LOG(LogTemp, Log, TEXT("[Manager] Player moved to: %s"), *NewPlayerPos.ToString());

		// 한 틱 쉬고 충돌 복구 — 같은 프레임에 바로 켜면 그 자리에서
		// 곧바로 다시 밀려날 수 있어서 한 틱 정도 여유를 줌
		TWeakObjectPtr<ACharacter> WeakPlayer = Player;
		GetWorldTimerManager().SetTimerForNextTick([WeakPlayer]()
			{
				if (WeakPlayer.IsValid())
				{
					WeakPlayer->SetActorEnableCollision(true);
				}
			});
	}

	ActiveCube = NextCube;
	CurrentCubeCoord = NextCoord;

	OnCubeTransition.Broadcast(NextCoord);

	UE_LOG(LogTemp, Log, TEXT("[Manager] Transition performed successfully"));
}

FVector ACC_CubeWorldManager::CalculatePlayerPositionInCube(ACC_Cube* TargetCube, EBoundaryDirection FromDirection) const
{
	if (!TargetCube)
		return FVector::ZeroVector;

	FVector CubeCenter = TargetCube->GetCubeCenter();
	float Offset = (CubeSize / 2.0f) - 500.0f; // 경계에서 약간 안쪽

	// FromDirection = 이전 큐브에서 "나간" 방향
	// → 새 큐브에서는 그 반대쪽(=이전 큐브와 맞닿은 면) 근처에 배치해야
	//   다음 경계까지 큐브 폭만큼 제대로 이동하게 됨
	switch (FromDirection)
	{
	case EBoundaryDirection::Right:
		return CubeCenter + FVector(0, -Offset, 200); // Right로 나감 → 새 큐브 Left쪽에 배치

	case EBoundaryDirection::Left:
		return CubeCenter + FVector(0, Offset, 200); // Left로 나감 → 새 큐브 Right쪽에 배치

	case EBoundaryDirection::Up:
		return CubeCenter + FVector(-Offset, 0, 200); // Up로 나감 → 새 큐브 Down쪽에 배치

	case EBoundaryDirection::Down:
		return CubeCenter + FVector(Offset, 0, 200); // Down로 나감 → 새 큐브 Up쪽에 배치
	}

	return CubeCenter;
}

ACharacter* ACC_CubeWorldManager::GetPlayerCharacter() const
{
	return UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
}

void ACC_CubeWorldManager::MovePlayerToCube(FIntPoint Coordinate)
{
	ACC_Cube* Cube = FindCube(Coordinate);
	if (!Cube)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Manager] Cannot move player - cube not found at (%d, %d)"),
			Coordinate.X, Coordinate.Y);
		return;
	}

	ACharacter* Player = GetPlayerCharacter();

	if (Player)
	{
		FVector CubeCenter = Cube->GetCubeCenter();
		Player->SetActorLocation(CubeCenter + FVector(0, 0, 300));

		UE_LOG(LogTemp, Log, TEXT("[Manager] Moved player to cube (%d, %d) at %s"),
			Coordinate.X, Coordinate.Y, *CubeCenter.ToString());
	}
}

void ACC_CubeWorldManager::PrintDebugInfo()
{
	UE_LOG(LogTemp, Warning, TEXT("=============================================="));
	UE_LOG(LogTemp, Warning, TEXT("   CUBE WORLD DEBUG INFO"));
	UE_LOG(LogTemp, Warning, TEXT("=============================================="));
	UE_LOG(LogTemp, Warning, TEXT("Grid Size: %d x %d"), GridWidth, GridHeight);
	UE_LOG(LogTemp, Warning, TEXT("Current Cube: (%d, %d)"), CurrentCubeCoord.X, CurrentCubeCoord.Y);
	UE_LOG(LogTemp, Warning, TEXT("Loaded Cubes: %d"), LoadedCubes.Num());
	UE_LOG(LogTemp, Warning, TEXT("Transitioning: %s"), bIsTransitioning ? TEXT("Yes") : TEXT("No"));

	int32 TotalManagedActors = 0;
	for (ACC_Cube* Cube : LoadedCubes)
	{
		if (!Cube) continue;
		TotalManagedActors += Cube->ManagedActors.Num();
		UE_LOG(LogTemp, Warning, TEXT("  - Cube (%d,%d) [%s]: %d actors"),
			Cube->CubeCoordinate.X, Cube->CubeCoordinate.Y,
			Cube->IsFrozen() ? TEXT("Frozen") : TEXT("Active"),
			Cube->ManagedActors.Num());
	}
	UE_LOG(LogTemp, Warning, TEXT("Total ManagedActors (all cubes): %d"), TotalManagedActors);

	if (ACC_EnemyManager* Manager = ACC_EnemyManager::Get(this))
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemyManager ActiveEnemies: %d"), Manager->GetEnemyCount());
	}

	UE_LOG(LogTemp, Warning, TEXT("=============================================="));

}

void ACC_CubeWorldManager::DrawAllCubes()
{
	UE_LOG(LogTemp, Log, TEXT("[Manager] Drawing debug info for all loaded cubes..."));

	for (ACC_Cube* Cube : LoadedCubes)
	{
		if (Cube)
		{
			UE_LOG(LogTemp, Log, TEXT("[Manager] Drawing debug info for cube (%d, %d)"), Cube->CubeCoordinate.X, Cube->CubeCoordinate.Y);
			Cube->DrawDebugInfo();
		}
	}
}

void ACC_CubeWorldManager::BuildCoordinateThemeCache()
{
	CoordinateThemeCache.Empty();

	if (!ThemeAssignmentTable)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Manager] ThemeAssignmentTable 미설정 — 모든 큐브가 Blueprint 디폴트 테마 사용."));
		return;
	}

	TArray<FCubeThemeAssignmentRow*> Rows;
	ThemeAssignmentTable->GetAllRows<FCubeThemeAssignmentRow>(TEXT("BuildCoordinateThemeCache"), Rows);

	for (const FCubeThemeAssignmentRow* Row : Rows)
	{
		if (Row)
		{
			CoordinateThemeCache.Add(Row->Coordinate, Row->Theme);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[Manager] Loaded %d coordinate-theme assignments"), CoordinateThemeCache.Num());

}

void ACC_CubeWorldManager::OnEmenyUnregistered(AActor* Enemy)
{
	if (!Enemy) return;

	UE_LOG(LogTemp, Warning, TEXT("[CubeWorldManager] OnEmenyUnregistered called for: %s"),
		*GetNameSafe(Enemy));

	bool bFound = false;
	// ─── 모든 LoadedCubes에서 죽은 Enemy UnregisterActor ──────────────────
	for (ACC_Cube* Cube : LoadedCubes)
	{
		if (!Cube) continue;
		if (Cube->ManagedActors.Contains(Enemy))
		{
			bFound = true;
			Cube->UnregisterActor(Enemy);
			UE_LOG(LogTemp, Log,
				TEXT("[CubeWorldManager] Unregistered dead enemy [%s] from Cube (%d,%d)"),
				*GetNameSafe(Enemy),
				Cube->CubeCoordinate.X, Cube->CubeCoordinate.Y);
			break; // 한 Cube에만 소속
		}
	}

	// ← 추가: 못 찾은 경우를 명시적으로 로그
	if (!bFound)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[CubeWorldManager] %s was NOT found in any Cube's ManagedActors! "),
			*GetNameSafe(Enemy));
	}

	// [버그.5 수정] Persistent 카운트 차감은 ACC_EnemyCharacter::Die()에서 즉시 처리됨.
	// 여기서 다시 차감하면 이중 차감 발생 (Die() → EndPlay() → 본 함수 순으로 같은 적에 대해 두 번 호출됨).
	// 주의: 만약 향후 Persistent 적이 Die()를 거치지 않고 파괴되는 경로가 생기면
	// (예: DespawnCube의 강제 정리 등) 그 경로에서 Persistent 카운트 차감을 별도로 챙겨야 함.

	//if (ACC_EnemyCharacter* EnemyChar = Cast<ACC_EnemyCharacter>(Enemy))
	//{
	//	if (EnemyChar->bPersistent)
	//	{
	//		UnregisterPersistentEnemy(EnemyChar);
	//	}
	//}
}

ECubeTheme ACC_CubeWorldManager::AssignThemeForCoord(FIntPoint Coord) const
{
	if (const ECubeTheme* Found = CoordinateThemeCache.Find(Coord))
	{
		if (*Found != ECubeTheme::None)
		{
			return *Found;
		}
	}
	return ECubeTheme::None;
}

void ACC_CubeWorldManager::LinkSpawnersToNearestCube(ACC_Cube* TargetCube)
{
	TArray<AActor*> FoundSpawners;
	UGameplayStatics::GetAllActorsOfClass(
		GetWorld(), ACC_EnemySpawner::StaticClass(), FoundSpawners);

	if (FoundSpawners.Num() == 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[Manager] No EnemySpawners found in level."));
		return;
	}

	const float LinkRadius = CubeSize * 0.5f;

	int32 LinkedCount = 0;

	for (AActor* SpawnerActor : FoundSpawners)
	{
		ACC_EnemySpawner* Spawner = Cast<ACC_EnemySpawner>(SpawnerActor);
		if (!Spawner) continue;

		// 이미 OwnerCube가 설정된 Spawner는 건너뜀 (중복 연결 방지)
		if (Spawner->GetOwnerCube()) continue;

		ACC_Cube* MatchedCube = nullptr;
		float MatchedDist = 0.f;

		if (TargetCube)
		{
			// [신규] 특정 Cube 대상 — Cube 생성 이벤트마다 호출되는 지역 패스.
			// 거리가 LinkRadius 이내일 때만 연결 (먼 Spawner를 잘못 묶지 않도록).
			const float Dist = FVector::Dist(
				Spawner->GetActorLocation(), TargetCube->GetActorLocation());

			if (Dist <= LinkRadius)
			{
				MatchedCube = TargetCube;
				MatchedDist = Dist;
			}
		}
		else
		{
			// [레거시] 전역 모드 — LoadedCubes 전체에서 최근접 Cube 탐색 (하위 호환/디버그용)
			float BestDist = FLT_MAX;
			for (ACC_Cube* Cube : LoadedCubes)
			{
				if (!Cube) continue;
				const float Dist = FVector::Dist(
					Spawner->GetActorLocation(), Cube->GetActorLocation());
				if (Dist < BestDist)
				{
					BestDist = Dist;
					MatchedCube = Cube;
				}
			}
			MatchedDist = BestDist;
		}

		if (!MatchedCube)
		{
			// TargetCube 모드라면 "이번엔 매칭 안 됨"이 정상 동작 — 다른 Cube가 생길 때 다시 시도됨.
			continue;
		}

		// 연결 — SetOwnerCube() 내부에서 SpawnedEnemies 재등록 + (조건 충족 시) StartSpawning까지 처리
		Spawner->SetOwnerCube(MatchedCube);
		MatchedCube->RegisterActor(Spawner);
		++LinkedCount;

		UE_LOG(LogTemp, Log,
			TEXT("[Manager] Spawner [%s] linked to Cube (%d,%d) — dist: %.0f"),
			*Spawner->GetName(),
			MatchedCube->CubeCoordinate.X,
			MatchedCube->CubeCoordinate.Y,
			MatchedDist);
	}

	UE_LOG(LogTemp, Log,
		TEXT("[Manager] LinkSpawnersToNearestCube(%s) complete — %d / %d spawners linked."),
		TargetCube
		? *FString::Printf(TEXT("Cube %d,%d"), TargetCube->CubeCoordinate.X, TargetCube->CubeCoordinate.Y)
		: TEXT("Global"),
		LinkedCount, FoundSpawners.Num());
}
