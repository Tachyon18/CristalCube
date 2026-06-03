// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CC_CubeScatterComponent.h"
#include "../CristalCubeStruct.h"
#include "CC_Cube.generated.h"

UCLASS()
class CRISTALCUBE_API ACC_Cube : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACC_Cube();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:

	/** 큐브 전체 크기 (3x3 = 6000) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube")
	float CubeSize = 500.0f;

	/** 개별 타일 크기 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube")
	float TileSize = 2000.0f;

	/** 큐브 높이 (벽면의 크기) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube")
	float CubeHeight = 10000.0f;

	/** 큐브 벽 두께 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube")
	float WallThickness = 500.0f;

	/** 타일 클래스 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube")
	TSubclassOf<class ACC_Tile> TileClass;

	/** 이 큐브의 테마 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube|Theme")
	ECubeTheme CubeTheme = ECubeTheme::None;

	/** FloorMesh Dynamic Material Instance */
	UPROPERTY()
	UMaterialInstanceDynamic* FloorMID = nullptr;

	// ========== 큐브 벽 (외부 충돌) ==========

	/** 큐브 벽 - 오른쪽 (X+) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cube Walls")
	class UBoxComponent* CubeWall_Right;

	/** 큐브 벽 - 왼쪽 (X-) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cube Walls")
	class UBoxComponent* CubeWall_Left;

	/** 큐브 벽 - 위쪽 (Y+) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cube Walls")
	class UBoxComponent* CubeWall_Top;

	/** 큐브 벽 - 아래쪽 (Y-) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cube Walls")
	class UBoxComponent* CubeWall_Bottom;

	// ========== 큐브 타일 ==========

	/** 생성된 타일들 (9개) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cube")
	TArray<class ACC_Tile*> CubeTiles;

	/** 현재 플레이어 타일 인덱스 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cube")
	int32 CurrentPlayerTileIndex = 4;

	// ========== 큐브 관련 함수 ==========

	/** 큐브 생성 (3x3 타일 + 외부 벽) This Function is not Using */
	UFUNCTION(BlueprintCallable, Category = "Cube")
	void GenerateCube();

	/** 인덱스로 타일 가져오기 (0-8) */
	UFUNCTION(BlueprintCallable, Category = "Cube")
	ACC_Tile* GetTileAt(int32 Index) const;

	/** 위치로 타일 인덱스 가져오기 */
	UFUNCTION(BlueprintCallable, Category = "Cube")
	int32 GetTileIndexAtPosition(const FVector& Position) const;

	/** 플레이어 타일 주변 활성화 */
	UFUNCTION(BlueprintCallable, Category = "Cube")
	void ActivateTilesAroundPlayer(int32 PlayerTileIndex);

	/** 특정 타일만 활성화 */
	UFUNCTION(BlueprintCallable, Category = "Cube")
	void ActivateOnlyTile(int32 TileIndex);

	/** 모든 타일 활성화 */
	UFUNCTION(BlueprintCallable, Category = "Cube")
	void ActivateAllTiles();

	/** 모든 타일 비활성화 */
	UFUNCTION(BlueprintCallable, Category = "Cube")
	void DeactivateAllTiles();

	/** 테마 머티리얼 적용 */
	UFUNCTION(BlueprintCallable, Category = "Cube|Theme")
	void ApplyTheme(const FCubeThemeData& ThemeData);

	UFUNCTION(BlueprintCallable, Category = "Cube")
	void SetLockWall(bool bLock);

	/** 오른쪽 벽 충돌 시 왼쪽으로 전환 */
	UFUNCTION()
	void OnCubeWallHit_Right(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	/** 왼쪽 벽 충돌 시 오른쪽으로 전환 */
	UFUNCTION()
	void OnCubeWallHit_Left(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	/** 위쪽 벽 충돌 시 아래쪽 전환 */
	UFUNCTION()
	void OnCubeWallHit_Top(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	/** 아래쪽 벽 충돌 시 위쪽 전환 */
	UFUNCTION()
	void OnCubeWallHit_Bottom(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

protected:
	/** 인덱스 행 열/행 변환 */
	void IndexToRowCol(int32 Index, int32& OutRow, int32& OutCol) const;

	/** 행/열 행 인덱스 변환 */
	int32 RowColToIndex(int32 Row, int32 Col) const;

	/** 큐브 벽 초기화 */
	void InitializeCubeWalls();

public:

	/** 큐브 좌표 (0,0) ~ (2,2) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cube")
	FIntPoint CubeCoordinate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment")
	UCC_CubeScatterComponent* ScatterComponent;

	/** 현재 상태 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cube")
	ECubeState CubeState;

	/** 이 큐브가 관리하는 Actor들 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cube")
	TArray<AActor*> ManagedActors;

	/** 큐브 바닥 (시각적) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	UStaticMeshComponent* FloorMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TArray<UBoxComponent*> BoundaryTriggers;

	/** 큐브 초기화 */
	UFUNCTION(BlueprintCallable, Category = "Cube")
	void InitializeCube(FIntPoint Coordinate);

	/** 경계 트리거 생성 */
	void CreateBoundaryTriggers();

	UFUNCTION(BlueprintCallable, Category = "Cube")
	void SetBoundaryTriggersEnabled(bool bEnabled);

	/** Actor 등록 (이 큐브에 소속) */
	UFUNCTION(BlueprintCallable, Category = "Cube")
	void RegisterActor(AActor* Actor);

	/** Actor 해제 */
	UFUNCTION(BlueprintCallable, Category = "Cube")
	void UnregisterActor(AActor* Actor);

	/** Actor가 이 큐브 내부에 있는지 확인 */
	UFUNCTION(BlueprintCallable, Category = "Cube")
	bool IsActorInCube(AActor* Actor) const;

	/** 큐브 정지 (시간 멈춤) */
	UFUNCTION(BlueprintCallable, Category = "Cube")
	void Freeze();

	/** 큐브 재개 (시간 재개) */
	UFUNCTION(BlueprintCallable, Category = "Cube")
	void Unfreeze();

	/** Frozen 여부인지 확인 */
	UFUNCTION(BlueprintCallable, Category = "Cube")
	bool IsFrozen() const { return CubeState == ECubeState::Frozen; }

	UFUNCTION()
	void OnBoundaryOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION(BlueprintCallable, Category = "Cube")
	FVector GetCubeCenter() const;

	/** 큐브 경계 Box */
	UFUNCTION(BlueprintCallable, Category = "Cube")
	FBox GetCubeBounds() const;

	/** 디버그 Draw */
	UFUNCTION(BlueprintCallable, Category = "Cube|Debug")
	void DrawDebugInfo();


protected:

	TMap<UBoxComponent*, EBoundaryDirection> BoundaryDirectionMap;

public:
};
