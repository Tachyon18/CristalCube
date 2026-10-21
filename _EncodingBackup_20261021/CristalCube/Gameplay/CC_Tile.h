// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CC_Tile.generated.h"

UCLASS()
class CRISTALCUBE_API ACC_Tile : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACC_Tile();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tile")
	int32 TileIndex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tile")
	float TileSize = 2000.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Tile")
	bool bIsActive = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tile")
	TArray<AActor*> ActorsInTile;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tile")
	TArray<class ACC_EnemyCharacter*> EnemiesInTile;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	UStaticMeshComponent* FloorMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	class UBoxComponent* TileBounds;

public:

	/** 타일 활성화 */
	UFUNCTION(BlueprintCallable, Category = "Tile")
	void ActivateTile();

	/** 타일 비활성화 */
	UFUNCTION(BlueprintCallable, Category = "Tile")
	void DeactivateTile();

	/** 타일 초기화 */
	UFUNCTION(BlueprintCallable, Category = "Tile")
	void InitializeTile(int32 Index, const FVector& Center, float Size);

	// ========== 공간 정보 ==========

	/** 타일 중심 */
	UFUNCTION(BlueprintCallable, Category = "Tile")
	FVector GetTileCenter() const;

	/** 타일 경계 */
	UFUNCTION(BlueprintCallable, Category = "Tile")
	FBox GetTileBounds() const;

	/** 위치가 타일 안에 있는지 (XY만) */
	UFUNCTION(BlueprintCallable, Category = "Tile")
	bool IsPositionInTile(const FVector& Position) const;

	// ========== Actor 관리 ==========

	/** Actor 등록 */
	UFUNCTION(BlueprintCallable, Category = "Tile")
	void RegisterActor(AActor* Actor);

	/** Actor 해제 */
	UFUNCTION(BlueprintCallable, Category = "Tile")
	void UnregisterActor(AActor* Actor);

	/** 타일 인덱스 */
	UFUNCTION(BlueprintCallable, Category = "Tile")
	int32 GetTileIndex() const { return TileIndex; }

	/** 타일 활성 상태 */
	UFUNCTION(BlueprintCallable, Category = "Tile")
	bool IsTileActive() const { return bIsActive; }

	/** 모든 액터 */
	UFUNCTION(BlueprintCallable, Category = "Tile")
	TArray<AActor*> GetActorsInTile() const { return ActorsInTile; }

	/** 적들만 */
	UFUNCTION(BlueprintCallable, Category = "Tile")
	TArray<ACC_EnemyCharacter*> GetEnemiesInTile() const { return EnemiesInTile; }

	/** 적 활성화/비활성화 */
	UFUNCTION(BlueprintCallable, Category = "Tile")
	void SetEnemiesActive(bool bActive);


};
