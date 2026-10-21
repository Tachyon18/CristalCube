// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_Tile.h"
#include "Components/BoxComponent.h"
#include "../Characters/CC_EnemyCharacter.h"

// Sets default values
ACC_Tile::ACC_Tile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	FloorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FloorMesh"));
	FloorMesh->SetupAttachment(RootComponent);
	FloorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FloorMesh->SetCastShadow(false);

	TileBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("TileBounds"));
	TileBounds->SetupAttachment(RootComponent);
	TileBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TileBounds->SetBoxExtent(FVector(1000.0f, 1000.0f, 10.0f));
	TileBounds->SetHiddenInGame(false);
	TileBounds->ShapeColor = FColor::Green;
}

// Called when the game starts or when spawned
void ACC_Tile::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ACC_Tile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ACC_Tile::ActivateTile()
{
	if (bIsActive) return;

	bIsActive = true;

	if (FloorMesh)
	{
		FloorMesh->SetVisibility(true);
	}

	if (TileBounds)
	{
		TileBounds->ShapeColor = FColor::Green;
	}

	SetEnemiesActive(true);

	UE_LOG(LogTemp, Log, TEXT("Tile %d: Activated (%d enemies)"),
		TileIndex, EnemiesInTile.Num());
}

void ACC_Tile::DeactivateTile()
{
	if (!bIsActive) return;

	bIsActive = false;

	if (FloorMesh)
	{
		FloorMesh->SetVisibility(false);
	}

	if (TileBounds)
	{
		TileBounds->ShapeColor = FColor::Red;
	}

	SetEnemiesActive(false);

	UE_LOG(LogTemp, Log, TEXT("Tile %d: Deactivated (%d enemies)"),
		TileIndex, EnemiesInTile.Num());
}

void ACC_Tile::InitializeTile(int32 Index, const FVector& Center, float Size)
{
	TileIndex = Index;
	TileSize = Size;
	SetActorLocation(Center);

	if (TileBounds)
	{
		TileBounds->SetBoxExtent(FVector(Size / 2.0f, Size / 2.0f, 10.0f));
	}

	if (FloorMesh)
	{
		float Scale = Size / 100.0f;
		FloorMesh->SetWorldScale3D(FVector(Scale, Scale, 1.0f));
	}

	UE_LOG(LogTemp, Log, TEXT("Tile %d: Initialized at %s"), Index, *Center.ToString());
}

FVector ACC_Tile::GetTileCenter() const
{
	return GetActorLocation();
}

FBox ACC_Tile::GetTileBounds() const
{
	FVector Center = GetTileCenter();
	float HalfSize = TileSize / 2.0f;

	FVector Min = FVector(Center.X - HalfSize, Center.Y - HalfSize, Center.Z - 10.0f);
	FVector Max = FVector(Center.X + HalfSize, Center.Y + HalfSize, Center.Z + 10.0f);

	return FBox(Min, Max);
}

bool ACC_Tile::IsPositionInTile(const FVector& Position) const
{
	// XY만 체크 (Z축 무시)
	return GetTileBounds().IsInsideXY(Position);
}

void ACC_Tile::RegisterActor(AActor* Actor)
{
	if (!Actor) return;
	if (ActorsInTile.Contains(Actor)) return;

	ActorsInTile.Add(Actor);

	if (ACC_EnemyCharacter* Enemy = Cast<ACC_EnemyCharacter>(Actor))
	{
		EnemiesInTile.Add(Enemy);
	}

	UE_LOG(LogTemp, Log, TEXT("Tile %d: %s registered (Total: %d, Enemies: %d)"),
		TileIndex, *Actor->GetName(), ActorsInTile.Num(), EnemiesInTile.Num());
}

void ACC_Tile::UnregisterActor(AActor* Actor)
{
	if (!Actor) return;

	ActorsInTile.Remove(Actor);

	if (ACC_EnemyCharacter* Enemy = Cast<ACC_EnemyCharacter>(Actor))
	{
		EnemiesInTile.Remove(Enemy);
	}

	UE_LOG(LogTemp, Log, TEXT("Tile %d: %s unregistered (Total: %d, Enemies: %d)"),
		TileIndex, *Actor->GetName(), ActorsInTile.Num(), EnemiesInTile.Num());
}

void ACC_Tile::SetEnemiesActive(bool bActive)
{
	for (ACC_EnemyCharacter* Enemy : EnemiesInTile)
	{
		if (Enemy)
		{
			Enemy->SetActorTickEnabled(bActive);
			Enemy->SetActorHiddenInGame(!bActive);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Tile %d: %d enemies %s"),
		TileIndex, EnemiesInTile.Num(), bActive ? TEXT("activated") : TEXT("deactivated"));
}