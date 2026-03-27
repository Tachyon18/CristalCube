// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CC_TileManager.generated.h"

UCLASS()
class CRISTALCUBE_API ACC_TileManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACC_TileManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	static ACC_TileManager* Instance;
	
	UFUNCTION(BlueprintCallable, Category = "Tile Manager")
	static ACC_TileManager* Get(UWorld* World);

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tile Settings")
	float TileSize = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tile Settings")
	float TileSpacing = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tile Settings")
	TSubclassOf<class ACC_Tile> TileClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tile State")
	int32 CurrentPlayerTileIndex = 4;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tiles")
	TArray<ACC_Tile*> Tiles;

public:

	UFUNCTION(BlueprintCallable, Category = "Tile Manager")
	void GenerateTileGrid();

	UFUNCTION(BlueprintCallable, Category = "Tile Manager")
	int32 GetTileIndexAtPosition(const FVector& Position) const;

	UFUNCTION(BlueprintCallable, Category = "Tile Manager")
	ACC_Tile* GetTileByIndex(int32 Index) const;

	UFUNCTION(BlueprintCallable, Category = "Tile Manager")
	TArray<ACC_Tile*> GetAllTiles() const { return Tiles; }

	UFUNCTION(BlueprintCallable, Category = "Tile Manager")
	void UpdatePlayerPosition(const FVector& PlayerPosition);

	UFUNCTION(BlueprintCallable, Category = "Tile Manager")
	FVector HandleWrapAround(const FVector& PlayerPosition, int32 FromTileIndex, int32 ToTileIndex);

	UFUNCTION(BlueprintCallable, Category = "Tile Manager")
	bool IsPositionOnTileBoundary(const FVector& Position, int32 TileIndex, FVector& OutDirection) const;

	UFUNCTION(BlueprintCallable, Category = "Tile Manager")
	int32 GetAdjacentTileIndex(int32 CurrentIndex, const FVector& Direction) const;

	UFUNCTION(BlueprintCallable, Category = "Tile Manager")
	void ActivateAllTiles();

	UFUNCTION(BlueprintCallable, Category = "Tile Manager")
	void ActivateOnlyTile(int32 TileIndex);

	UFUNCTION(BlueprintCallable, Category = "Tile Manager")
	void ActivateTilesAroundPlayer(int32 PlayerTileIndex);

	UFUNCTION(BlueprintCallable, Category = "Tile Manager")
	void DeactivateAllTiles();
protected:

	void IndexToRowCol(int32 Index, int32& OutRow, int32& OutCol) const;

	int32 IndexToRow(int32 Index) const { return Index / 3; }
	int32 IndexToCol(int32 Index) const { return Index % 3; }

	int32 RowColToIndex(int32 Row, int32 Col) const;
};
