// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CristalCubeStruct.h"
#include "CC_CubeWorldManager.generated.h"


UCLASS()
class CRISTALCUBE_API ACC_CubeWorldManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACC_CubeWorldManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

    static ACC_CubeWorldManager* Instance;

    // Get() 추가
    UFUNCTION(BlueprintPure, Category = "Cube World Manager",
        meta = (WorldContext = "WorldContextObject"))
    static ACC_CubeWorldManager* Get(const UObject* WorldContextObject);

    // ========================================
    // Grid Configuration
    // ========================================

    /** 그리드 가로 크기 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube|Grid")
    int32 GridWidth = 3;

	/** 그리드 세로 크기 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube|Grid")
    int32 GridHeight = 3;

    /** 큐브 크기*/
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube|Grid")
    float CubeSize = 500.0f;

    /** 큐브 클래스*/
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube|Grid")
    TSubclassOf<class ACC_Cube> CubeClass;

    /** 좌표별 테마 할당 DataTable (행 구조: FCubeThemeAssignmentRow) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube|Theme")
    UDataTable* ThemeAssignmentTable = nullptr;

    /** 테마별 데이터 (BP에서 할당) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube|Theme")
    TMap<ECubeTheme, FCubeThemeData> ThemeDataMap;

    /** 테마별 무드 프리셋 — Cube 생성 시 해당 테마 값만 그 Cube에 주입 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube|Theme")
    TMap<ECubeTheme, FCubeMoodSettings> MoodSettingsMap;

	/** 현재 큐브 좌표 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cube|Grid")
	FIntPoint CurrentCubeCoord;

    	/** 큐브 전환 중 여부 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cube|Grid")
	bool bIsTransitioning = false;

    /** 커스텀 레이아웃 사용 여부 (비정방형/테트리스형) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube|Grid")
	bool bUseCustomLayout = false;

    /** 커스텀 레이아웃 유효 셀 목록 (bUseCustomLayout=true 일 때 사용) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube|Grid",
        meta = (EditCondition = "bUseCustomLayout"))
    TArray<FIntPoint> CustomValidCells;

    /** 시작 큐브 좌표 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube|Grid")
    FIntPoint StartCoordinate = FIntPoint(1, 1);

    /** 유효 좌표 여부 확인 */
    UFUNCTION(BlueprintCallable, Category = "Cube|Grid")
    bool IsValidCoordinate(FIntPoint Coord) const;

    // ========================================
    // Persistent Enemy System
    // ========================================

    /** Lock 발동 임계값 (기본: 12) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube|Persistent")
    int32 LockThreshold = 12;

    /** 현재 Lock 상태 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cube|Persistent")
    bool bCubeLocked = false;

    UPROPERTY(BlueprintAssignable, Category = "Cube|Events")
    FOnCubeWaveCleared OnCubeWaveCleared;

    /** WorldManager 레벨 관리 Persistent Enemy 목록 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cube|Persistent")
    TArray<AActor*> PersistentEnemyList;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cube|Persistent")
    int32 PersistentEnemyCount = 0;

    UFUNCTION(BlueprintCallable, Category = "Cube")
    void NotifyCubeWaveCleared(FIntPoint Coord);

    UFUNCTION(BlueprintCallable, Category = "Cube|Persistent")
    void RegisterPersistentEnemy(AActor* Enemy);

    UFUNCTION(BlueprintCallable, Category = "Cube|Persistent")
    void UnregisterPersistentEnemy(AActor* Enemy);

    void CheckLockCondition();
    void TeleportPersistentEnemiesToCube(ACC_Cube* TargetCube);

    // ========================================
    // Data Structures
    // ========================================

    /** ť�� ������ �׸��� (9��) */
    UPROPERTY()
    TMap<FIntPoint, FCubeData> CubeGrid;

    /** 종전 Active 큐브 */
    UPROPERTY()
    ACC_Cube* ActiveCube;

    /** 현재 Active Cube 조회 — TestRoom HUD/Console 용 */
    UFUNCTION(BlueprintPure, Category = "Cube")
    ACC_Cube* GetActiveCube() const { return ActiveCube; }

    /** 로드된 모든 Cube의 ManagedActors 합계 — 누수 감시용 (PrintDebugInfo의 동일 계산을 BP에서도 즉시 조회) */
    UFUNCTION(BlueprintPure, Category = "Cube|Debug")
    int32 GetTotalManagedActorsCount() const;

    /** Spawn�� ť��� */
    UPROPERTY()
    TArray<ACC_Cube*> LoadedCubes;

    // ========================================
    // Initialization
    // ========================================

    /** �ý��� �ʱ�ȭ */
    UFUNCTION(BlueprintCallable, Category = "Cube")
    void InitializeSystem();

    /** ť�� �׸��� ������ ���� */
    void InitializeCubeGrid();

    // ========================================
    // Cube Management
    // ========================================

    /** ť�� Spawn */
    UFUNCTION(BlueprintCallable, Category = "Cube")
    ACC_Cube* SpawnCube(FIntPoint Coordinate);

    /** ť�� Despawn (�޸� �����, ���߿� ����) */
    UFUNCTION(BlueprintCallable, Category = "Cube")
    void DespawnCube(FIntPoint Coordinate);

    /** ť�� ã�� �Ǵ� Spawn */
    UFUNCTION(BlueprintCallable, Category = "Cube")
    ACC_Cube* FindOrSpawnCube(FIntPoint Coordinate);

    /** ť�� ã�� */
    UFUNCTION(BlueprintCallable, Category = "Cube")
    ACC_Cube* FindCube(FIntPoint Coordinate) const;

    void SetupCubeContent(ACC_Cube* NewCube);

    // ========================================
    // Transition System
    // ========================================

    /** ť�� ��ȯ ��û */
    UFUNCTION(BlueprintCallable, Category = "Cube|Transition")
    void RequestTransition(EBoundaryDirection Direction);

    /** ���� ť�� ��ǥ ��� (��ȯ ����) */
    UFUNCTION(BlueprintCallable, Category = "Cube|Transition")
    FIntPoint GetNextCubeCoord(FIntPoint Current, EBoundaryDirection Direction) const;

    /** ��ȯ ���� (���� �� ȣ��) */
    void PerformTransition(FIntPoint NextCoord);

    /** �÷��̾� ��ġ ��� (��ȯ ��) */
    FVector CalculatePlayerPositionInCube(ACC_Cube* TargetCube, EBoundaryDirection FromDirection) const;

    // ========================================
    // Player Management
    // ========================================

    /** �÷��̾� ���� �������� */
    UFUNCTION(BlueprintCallable, Category = "Cube")
    ACharacter* GetPlayerCharacter() const;

    /** �÷��̾ ť��� �̵� */
    UFUNCTION(BlueprintCallable, Category = "Cube")
    void MovePlayerToCube(FIntPoint Coordinate);

    // ========================================
    // Events
    // ========================================

    /** ť�� ��ȯ �Ϸ� �̺�Ʈ */
    UPROPERTY(BlueprintAssignable, Category = "Cube|Events")
    FOnCubeTransition OnCubeTransition;

    /** 큐브 시스템(InitializeSystem) 준비 완료 신호 — GameModeBase가 이 신호를 받은 뒤 Playing으로 전환 */
    UPROPERTY(BlueprintAssignable, Category = "Cube|Events")
    FOnCubeSystemReady OnCubeSystemReady;

    /** OnCubeSystemReady가 이미 Broadcast됐는지 — BeginPlay 순서가 안 보장되는 상황에서
    리스너가 늦게 바인딩되어 신호를 놓치는 경우를 외부(GameModeBase)가 즉시 감지할 수 있게 함 */
    UFUNCTION(BlueprintPure, Category = "Cube")
    bool IsCubeSystemReady() const { return bSystemReady; }

    // ========================================
    // Debug
    // ========================================

    /** ����� ���� ��� */
    UFUNCTION(BlueprintCallable, Category = "Cube|Debug")
    void PrintDebugInfo();

    /** ��� ť�� Draw */
    UFUNCTION(BlueprintCallable, Category = "Cube|Debug")
    void DrawAllCubes();

    /** �ڵ� �׽�Ʈ ��� */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube|Debug")
    bool bAutoTest = false;

protected:
    
    /** ���� ��ȯ ���� (�׽�Ʈ��) */
    EBoundaryDirection LastTransitionDirection;

    TMap<FIntPoint, ECubeTheme> CoordinateThemeCache;
    void BuildCoordinateThemeCache();

public:
    UFUNCTION()
    void OnEmenyUnregistered(AActor* Enemy);

	UFUNCTION(BlueprintCallable, Category = "Cube|Theme")
    ECubeTheme AssignThemeForCoord(FIntPoint Coord) const;

    UFUNCTION(BlueprintCallable, Category = "Cube")
    void LinkSpawnersToNearestCube(ACC_Cube* TargetCube = nullptr);

protected:

    float DebugLogTimer = 0.f;

    bool bSystemReady = false;
};
