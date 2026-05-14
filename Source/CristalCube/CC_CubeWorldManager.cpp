// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_CubeWorldManager.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Gameplay/CC_Cube.h"
#include "Characters/CC_EnemyCharacter.h"
#include "CC_EnemyManager.h"

// Sets default values
ACC_CubeWorldManager::ACC_CubeWorldManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ACC_CubeWorldManager::BeginPlay()
{
	Super::BeginPlay();
	
	InitializeSystem();

	if (ACC_EnemyManager* EnemyMgr = ACC_EnemyManager::Get(this))
	{
		//EnemyMgr->OnEnemyUnregistered.AddDynamic(this,)
	}
}

// Called every frame
void ACC_CubeWorldManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bAutoTest)
	{
		DrawAllCubes();
	}

}

bool ACC_CubeWorldManager::IsValidCoordinate(FIntPoint Coord) const
{
	return CubeGrid.Contains(Coord);
}

void ACC_CubeWorldManager::RegisterPersistentEnemy(ACC_EnemyCharacter* Enemy)
{
	if (!Enemy || PersistentEnemyList.Contains(Enemy)) return;

	PersistentEnemyList.Add(Enemy);
	UE_LOG(LogTemp, Log, TEXT("[Manager] Persistent enemy registered. Total: %d"),
		PersistentEnemyList.Num());

	CheckLockCondition();
}

void ACC_CubeWorldManager::UnregisterPersistentEnemy(ACC_EnemyCharacter* Enemy)
{
	if (!Enemy) return;

	PersistentEnemyList.Remove(Enemy);
	UE_LOG(LogTemp, Log, TEXT("[Manager] Persistent enemy unregistered. Total: %d"),
		PersistentEnemyList.Num());

	CheckLockCondition();
}

void ACC_CubeWorldManager::CheckLockCondition()
{
	if(!bCubeLocked && PersistentEnemyList.Num() >= LockThreshold)
	{
		bCubeLocked = true;
		if (ActiveCube) ActiveCube->SetBoundaryTriggersEnabled(false);

		UE_LOG(LogTemp, Warning, TEXT("[Manager] Cube LOCKED! Persistent enemy count: %d"), PersistentEnemyList.Num());
	}
	else if (bCubeLocked && PersistentEnemyList.Num() < LockThreshold)
	{
		bCubeLocked = false;
		if (ActiveCube) ActiveCube->SetBoundaryTriggersEnabled(true);

		UE_LOG(LogTemp, Warning, TEXT("[Manager] Cube UNLOCKED! Persistent enemy count: %d"), PersistentEnemyList.Num());
	}
}

void ACC_CubeWorldManager::TeleportPersistentEnemiesToCube(ACC_Cube* TargetCube)
{
	if (!TargetCube) return;

	FVector CubeCenter = TargetCube->GetCubeCenter();
	float Spread = 600.0f;

	for (ACC_EnemyCharacter* Enemy : PersistentEnemyList)
	{
		if (!IsValid(Enemy)) continue;

		FVector Offset(
			FMath::RandRange(-Spread, Spread),
			FMath::RandRange(-Spread, Spread),
			0.0f
		);
		Enemy->SetActorLocation(CubeCenter + Offset);
	}
}

void ACC_CubeWorldManager::InitializeSystem()
{
	UE_LOG(LogTemp, Warning, TEXT("=============================================="));
	UE_LOG(LogTemp, Warning, TEXT("   CUBE WORLD MANAGER - INITIALIZATION"));
	UE_LOG(LogTemp, Warning, TEXT("=============================================="));

	InitializeCubeGrid();
	
	CurrentCubeCoord = StartCoordinate;
	ActiveCube = FindOrSpawnCube(CurrentCubeCoord);

	if (ActiveCube)
	{
		ActiveCube->Unfreeze();
		UE_LOG(LogTemp, Log, TEXT("[Manager] Active cube set to: %d, %d"),
			CurrentCubeCoord.X, CurrentCubeCoord.Y);
	}

	MovePlayerToCube(CurrentCubeCoord);

	UE_LOG(LogTemp, Warning, TEXT("[Manager] System initialized successfully!"));
	UE_LOG(LogTemp, Warning, TEXT("=============================================="));
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

	// 이후 초기화
	NewCube->InitializeCube(Coordinate);
	LoadedCubes.Add(NewCube);

	if (CubeGrid.Contains(Coordinate))
	{
		CubeGrid[Coordinate].State = ECubeState::Active;
	}

	UE_LOG(LogTemp, Log, TEXT("[Manager] Spawned cube at (%d,%d) - Location: %s"),
		Coordinate.X, Coordinate.Y, *SpawnLocation.ToString());

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
		Player->SetActorLocation(NewPlayerPos);
		UE_LOG(LogTemp, Log, TEXT("[Manager] Player moved to: %s"), *NewPlayerPos.ToString());
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
	float Offset = (CubeSize / 2.0f) - 500.0f; // ��迡�� �ణ ����

	// �ݴ������� ����
	switch (FromDirection)
	{
	case EBoundaryDirection::Right:
		return CubeCenter + FVector(0, Offset, 200); // ���ʿ��� ����

	case EBoundaryDirection::Left:
		return CubeCenter + FVector(0, -Offset, 200); // �����ʿ��� ����

	case EBoundaryDirection::Up:
		return CubeCenter + FVector(Offset, 0, 200); // �Ʒ����� ����

	case EBoundaryDirection::Down:
		return CubeCenter + FVector(-Offset, 0, 200); // ������ ����
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

	if (ActiveCube)
	{
		UE_LOG(LogTemp, Warning, TEXT("Active Cube Actors: %d"), ActiveCube->ManagedActors.Num());
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

void ACC_CubeWorldManager::OnEmenyUnregistered(AActor* Enemy)
{
	if (ACC_EnemyCharacter* EnemyChar = Cast<ACC_EnemyCharacter>(Enemy))
	{
		if (EnemyChar->bPersistent)
		{
			UnregisterPersistentEnemy(EnemyChar);
		}
	}
}
