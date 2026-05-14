// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "CC_CubeScatterComponent.generated.h"

/**
 * CC_CubeScatterComponent
 *
 * CC_Cube에 붙는 컴포넌트.
 * FRandomStream + CubeCoordinate 시드 기반으로
 * HISM 인스턴스를 결정론적으로 생성/제거합니다.
 *
 * - 최초 진입 시 Generate() → HISM AddInstances
 * - Freeze 시 SetVisibility(false) → 파괴 없이 숨김
 * - Unfreeze 시 SetVisibility(true) → 재생성 없이 표시
 * - 같은 CubeCoordinate → 항상 같은 배치
 */
UCLASS( ClassGroup=(CristalCube), meta=(BlueprintSpawnableComponent) )
class CRISTALCUBE_API UCC_CubeScatterComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCC_CubeScatterComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // ========== 에디터 설정 ==========

    /** 배치할 스태틱 메시 목록 (블루프린트에서 할당) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter|Meshes")
    TArray<UStaticMesh*> ScatterMeshes;

    /** 메시별 가중치 (ScatterMeshes와 인덱스 일치, 비어있으면 균등 분배) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter|Meshes")
    TArray<int32> MeshWeights;

    /** 큐브 반경 (ACC_Cube::CubeSize / 2.0f 와 맞출 것) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter|Area")
    float ScatterRadius = 2800.0f;

    /** 스폰할 인스턴스 최소 개수 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter|Count")
    int32 MinCount = 15;

    /** 스폰할 인스턴스 최대 개수 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter|Count")
    int32 MaxCount = 35;

    /** 인스턴스 간 최소 거리 (겹침 방지) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter|Placement")
    float MinDistance = 200.0f;

    /** 스케일 최솟값 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter|Transform")
    float ScaleMin = 0.7f;

    /** 스케일 최댓값 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter|Transform")
    float ScaleMax = 1.4f;

    /** Z축 배치 오프셋 (바닥 위 높이) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter|Transform")
    float ZOffset = 0.0f;

    // ========== 런타임 상태 ==========

    /** 이미 생성되었는지 여부 (재진입 시 재생성 방지) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Scatter|State")
    bool bGenerated = false;
	
    // ========== 공개 함수 ==========

    /**
     * 스캐터 생성.
     * CubeCoordinate 기반 시드로 결정론적 배치.
     * bGenerated가 true면 무시 (재생성하지 않음).
     */
    UFUNCTION(BlueprintCallable, Category = "Scatter")
    void Generate(FIntPoint CubeCoordinate);

    /** Freeze: 모든 HISM 가시성 OFF. 인스턴스 파괴하지 않음. */
    UFUNCTION(BlueprintCallable, Category = "Scatter")
    void FreezeScatter();

    /** Unfreeze: 모든 HISM 가시성 ON. */
    UFUNCTION(BlueprintCallable, Category = "Scatter")
    void UnfreezeScatter();

    /** 스캐터 완전 제거 (큐브 Destroy 시 호출). */
    UFUNCTION(BlueprintCallable, Category = "Scatter")
    void ClearScatter();

private:

    /** 메시별 HISM 컴포넌트 (인덱스 = ScatterMeshes 인덱스) */
    UPROPERTY()
    TArray<UHierarchicalInstancedStaticMeshComponent*> HISMComponents;

    /** 이미 배치된 위치 목록 (MinDistance 체크용) */
    TArray<FVector> PlacedLocations;

    /** 가중치 기반 랜덤 메시 인덱스 선택 */
    int32 PickMeshIndex(FRandomStream& RNG) const;

    /** 위치가 기존 배치와 MinDistance 이상 떨어져 있는지 확인 */
    bool IsLocationValid(const FVector& Candidate) const;

    /** HISM 컴포넌트 생성 및 등록 */
    UHierarchicalInstancedStaticMeshComponent* CreateHISM(UStaticMesh* Mesh);

protected:

    UPROPERTY()
    class ACC_Cube* OwnerCube = nullptr;

public:
    
    UFUNCTION()
    void SetOwnerCube(ACC_Cube* InCube) { OwnerCube = InCube; }

};
