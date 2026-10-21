// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_TileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Gameplay/CC_Tile.h"

ACC_TileManager* ACC_TileManager::Instance = nullptr;
// Sets default values
ACC_TileManager::ACC_TileManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ACC_TileManager::BeginPlay()
{
	Super::BeginPlay();

	Instance = this;

	GenerateTileGrid();

	UE_LOG(LogTemp, Warning, TEXT("[TileManager] Initialized - 3x3 Grid Created"));	
}

void ACC_TileManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Instance == this)
	{
		Instance = nullptr;
		UE_LOG(LogTemp, Warning, TEXT("[TileManager] Instance reset on EndPlay"));
	}
	
	Super::EndPlay(EndPlayReason);
	
}

// Called every frame
void ACC_TileManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

ACC_TileManager* ACC_TileManager::Get(UWorld* World)
{
	if (IsValid(Instance))
	{
		return Instance;
	}

	Instance = nullptr;

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(World, ACC_TileManager::StaticClass(), FoundActors);

	if (FoundActors.Num() > 0)
	{
		Instance = Cast<ACC_TileManager>(FoundActors[0]);
		return Instance;
	}

	Instance = World->SpawnActor<ACC_TileManager>(ACC_TileManager::StaticClass());
	return Instance;
}

void ACC_TileManager::GenerateTileGrid()
{
	if (!TileClass)
	{
		UE_LOG(LogTemp, Error, TEXT("TileClass is not set!"));
		return;
	}

	for (ACC_Tile* Tile : Tiles)
	{
		if (Tile)
		{
			Tile->Destroy();
		}
	}
	Tiles.Empty();

	float TotalSize = TileSize + TileSpacing;

	//float TotalSizeX = -(TileSize + TileSpacing);
	//float TotalSizeY = -TotalSizeX;
	FVector StartLocation = GetActorLocation() - FVector(-TotalSize, TotalSize, 0.0f);

	for (int32 Row = 0; Row < 3; ++Row)
	{
		for (int32 Col = 0; Col < 3; ++Col)
		{
			int32 Index = Row * 3 + Col;

			FVector TileLocation = StartLocation + FVector(
				-(Row * TotalSize),
				Col * TotalSize,
				0.0f
			);

			ACC_Tile* NewTile = GetWorld()->SpawnActor<ACC_Tile>(
				TileClass,
				TileLocation,
				FRotator::ZeroRotator
			);

			if (NewTile)
			{
				NewTile->InitializeTile(Index, TileLocation, TileSize);
				Tiles.Add(NewTile);
			}
		}
	}

	//UE_LOG(LogTemp, Log, TEXT("[TileManager] Generated %d tiles in 3x3 grid"), Tiles.Num());
}

int32 ACC_TileManager::GetTileIndexAtPosition(const FVector& Position) const
{
	for (int32 i = 0; i < Tiles.Num(); ++i)
	{
		if (Tiles[i] && Tiles[i]->IsPositionInTile(Position))
		{
			//UE_LOG(LogTemp, Log, TEXT("[TileManager] Position %s is in Tile %d"), *Position.ToString(), i);
			return i;
		}
	}

	return -1; // Out of Tile
}

ACC_Tile* ACC_TileManager::GetTileByIndex(int32 Index) const
{
	if (Index >= 0 && Index < Tiles.Num())
	{
		return Tiles[Index];
	}

	return nullptr;
}

void ACC_TileManager::UpdatePlayerPosition(const FVector& PlayerPosition)
{
	int32 NewTileIndex = GetTileIndexAtPosition(PlayerPosition);

	if (NewTileIndex != -1 && NewTileIndex != CurrentPlayerTileIndex)
	{
		UE_LOG(LogTemp, Log, TEXT("Player moved from Tile %d to Tile %d"), CurrentPlayerTileIndex, NewTileIndex);

		CurrentPlayerTileIndex = NewTileIndex;

		ActivateTilesAroundPlayer(CurrentPlayerTileIndex);

		// Tile Change Event
		// OnTileChanged.Broadcast(CurrentPlayerTileIndex);
	}
}

FVector ACC_TileManager::HandleWrapAround(const FVector& PlayerPosition, int32 FromTileIndex, int32 ToTileIndex)
{
	// 순환 이동이 필요한지 확인
	int32 FromRow, FromCol, ToRow, ToCol;
	IndexToRowCol(FromTileIndex, FromRow, FromCol);
	IndexToRowCol(ToTileIndex, ToRow, ToCol);

	FVector NewPosition = PlayerPosition;
	bool bWrapped = false;

	UE_LOG(LogTemp, Warning, TEXT("[TileManager] HandleWrapAround: From Tile %d (Row %d, Col %d) -> To Tile %d (Row %d, Col %d)"),
		FromTileIndex, FromRow, FromCol, ToTileIndex, ToRow, ToCol);

	// 좌우 순환 (0열 ↔ 2열)
	if (FromCol == 0 && ToCol == 2)
	{
		// 왼쪽 → 오른쪽
		NewPosition.Y += TileSize * 3;
		bWrapped = true;
	}
	else if (FromCol == 2 && ToCol == 0)
	{
		// 오른쪽 → 왼쪽
		NewPosition.Y -= TileSize * 3;
		bWrapped = true;
	}

	// 상하 순환 (0행 ↔ 2행)
	if (FromRow == 0 && ToRow == 2)
	{
		// 위 → 아래
		NewPosition.X += TileSize * 3;
		bWrapped = true;
	}
	else if (FromRow == 2 && ToRow == 0)
	{
		// 아래 → 위
		NewPosition.X -= TileSize * 3;
		bWrapped = true;
	}

	if (bWrapped)
	{
		UE_LOG(LogTemp, Warning, TEXT("Wrap Around: Tile %d -> %d, New Position: %s"),
			FromTileIndex, ToTileIndex, *NewPosition.ToString());
	}

	return NewPosition;
}

bool ACC_TileManager::IsPositionOnTileBoundary(const FVector& Position, int32 TileIndex, FVector& OutDirection) const
{
	ACC_Tile* Tile = GetTileByIndex(TileIndex);
	if (!Tile) return false;

	FBox Bounds = Tile->GetTileBounds();
	FVector Min = Bounds.Min;
	FVector Max = Bounds.Max;

	const float Threshold = 50.0f; // 경계 임계값

	// 각 경계 체크
	if (FMath::Abs(Position.X - Min.X) < Threshold)
	{
		OutDirection = FVector(-1, 0, 0); // 왼쪽
		return true;
	}
	if (FMath::Abs(Position.X - Max.X) < Threshold)
	{
		OutDirection = FVector(1, 0, 0); // 오른쪽
		return true;
	}
	if (FMath::Abs(Position.Y - Min.Y) < Threshold)
	{
		OutDirection = FVector(0, -1, 0); // 아래
		return true;
	}
	if (FMath::Abs(Position.Y - Max.Y) < Threshold)
	{
		OutDirection = FVector(0, 1, 0); // 위
		return true;
	}

	return false;
}

int32 ACC_TileManager::GetAdjacentTileIndex(int32 CurrentIndex, const FVector& Direction) const
{
	int32 Row, Col;
	IndexToRowCol(CurrentIndex, Row, Col);

	if (Direction.X > 0.5f) Col++;
	else if (Direction.X < -0.5f) Col--;

	if (Direction.Y > 0.5f) Row++;
	else if (Direction.Y < -0.5f) Row--;

	Col = (Col + 3) % 3;
	Row = (Row + 3) % 3;

	return RowColToIndex(Row, Col);
}

void ACC_TileManager::ActivateAllTiles()
{
	for (ACC_Tile* Tile : Tiles)
	{
		if (Tile)
		{
			Tile->ActivateTile();
		}
	}
}

void ACC_TileManager::ActivateOnlyTile(int32 TileIndex)
{
	for (int32 i = 0; i < Tiles.Num(); ++i)
	{
		if (Tiles[i])
		{
			if (i == TileIndex)
			{
				Tiles[i]->ActivateTile();
			}
			else
			{
				Tiles[i]->DeactivateTile();
			}
		}
	}
}

void ACC_TileManager::ActivateTilesAroundPlayer(int32 PlayerTileIndex)
{
	ActivateOnlyTile(PlayerTileIndex);

	UE_LOG(LogTemp, Log, TEXT("TileManager: Activated tile %d (Player tile)"), PlayerTileIndex);
}

void ACC_TileManager::DeactivateAllTiles()
{
	for (ACC_Tile* Tile : Tiles)
	{
		if (Tile)
		{
			Tile->DeactivateTile();
		}
	}

	UE_LOG(LogTemp, Log, TEXT("TileManager: All tiles deactivated"));
}

void ACC_TileManager::IndexToRowCol(int32 Index, int32& OutRow, int32& OutCol) const
{
	OutRow = Index / 3;
	OutCol = Index % 3;
}

int32 ACC_TileManager::RowColToIndex(int32 Row, int32 Col) const
{
	return Row * 3 + Col;
}