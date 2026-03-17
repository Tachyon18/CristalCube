// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_CubeScatterComponent.h"
#include "Math/RandomStream.h"

// Sets default values for this component's properties
UCC_CubeScatterComponent::UCC_CubeScatterComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	bGenerated = false;

	// ...
}


// Called when the game starts
void UCC_CubeScatterComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UCC_CubeScatterComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

// ============================================================
//  Generate
// ============================================================

void UCC_CubeScatterComponent::Generate(FIntPoint CubeCoordinate)
{
    // 이미 생성되었으면 무시
    if (bGenerated)
    {
        UE_LOG(LogTemp, Log, TEXT("[Scatter (%d,%d)] Already generated, skipping."),
            CubeCoordinate.X, CubeCoordinate.Y);
        return;
    }

    if (ScatterMeshes.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Scatter (%d,%d)] ScatterMeshes is empty! Assign meshes in Blueprint."),
            CubeCoordinate.X, CubeCoordinate.Y);
        return;
    }

    // 1. 시드 계산 (같은 좌표 = 항상 같은 결과)
    int32 Seed = HashCombine(
        GetTypeHash(CubeCoordinate.X * 1000 + 7),
        GetTypeHash(CubeCoordinate.Y * 997 + 13));
    FRandomStream RNG(Seed);

    // 2. 메시별 HISM 컴포넌트 준비
    if (HISMComponents.Num() == 0)
    {
        for (UStaticMesh* Mesh : ScatterMeshes)
        {
            if (Mesh)
            {
                HISMComponents.Add(CreateHISM(Mesh));
            }
            else
            {
                HISMComponents.Add(nullptr);
                UE_LOG(LogTemp, Warning, TEXT("[Scatter] Null mesh entry in ScatterMeshes, skipping."));
            }
        }
    }

    // 3. 배치 개수 결정
    int32 Count = RNG.RandRange(MinCount, MaxCount);

    // 4. 메시별 트랜스폼 배열 준비
    TArray<TArray<FTransform>> TransformsByMesh;
    TransformsByMesh.SetNum(ScatterMeshes.Num());

    PlacedLocations.Empty();
    PlacedLocations.Reserve(Count);

    // 5. 배치 시도 (MaxAttempts로 MinDistance 보장)
    int32 MaxAttempts = Count * 3;
    int32 Placed = 0;

    for (int32 Attempt = 0; Attempt < MaxAttempts && Placed < Count; ++Attempt)
    {
        // 랜덤 위치 (원형 분포)
        float Angle = RNG.FRandRange(0.0f, 360.0f);
        float Dist = RNG.FRandRange(0.0f, ScatterRadius);
        float X = FMath::Cos(FMath::DegreesToRadians(Angle)) * Dist;
        float Y = FMath::Sin(FMath::DegreesToRadians(Angle)) * Dist;
        FVector Location(X, Y, ZOffset);

        // 오너 액터 위치 기준 월드 좌표 변환
        if (AActor* Owner = GetOwner())
        {
            Location += Owner->GetActorLocation();
            Location.Z = Owner->GetActorLocation().Z + ZOffset;
        }

        // MinDistance 검증
        if (!IsLocationValid(Location))
            continue;

        // Yaw만 랜덤 (Pitch/Roll = 0 → 공중에 뜨지 않음)
        float Yaw = RNG.FRandRange(0.0f, 360.0f);
        FRotator Rotation(0.0f, Yaw, 0.0f);

        // 균일 스케일
        float Scale = RNG.FRandRange(ScaleMin, ScaleMax);
        FVector ScaleVec(Scale);

        FTransform T(Rotation, Location, ScaleVec);

        // 메시 선택
        int32 MeshIndex = PickMeshIndex(RNG);
        if (TransformsByMesh.IsValidIndex(MeshIndex))
        {
            TransformsByMesh[MeshIndex].Add(T);
        }

        PlacedLocations.Add(Location);
        ++Placed;
    }

    // 6. HISM에 일괄 추가 (NoCollision 상태)
    for (int32 i = 0; i < HISMComponents.Num(); ++i)
    {
        if (!HISMComponents[i] || TransformsByMesh[i].Num() == 0)
            continue;

        HISMComponents[i]->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        HISMComponents[i]->AddInstances(TransformsByMesh[i], false);
    }

    bGenerated = true;

    UE_LOG(LogTemp, Log, TEXT("[Scatter (%d,%d)] Generated %d / %d instances (Seed: %d)"),
        CubeCoordinate.X, CubeCoordinate.Y, Placed, Count, Seed);
}

// ============================================================
//  Freeze / Unfreeze
// ============================================================

void UCC_CubeScatterComponent::FreezeScatter()
{
    for (UHierarchicalInstancedStaticMeshComponent* HISM : HISMComponents)
    {
        if (HISM)
        {
            HISM->SetVisibility(false, true);
            HISM->SetComponentTickEnabled(false);
        }
    }
}

void UCC_CubeScatterComponent::UnfreezeScatter()
{
    for (UHierarchicalInstancedStaticMeshComponent* HISM : HISMComponents)
    {
        if (HISM)
        {
            HISM->SetVisibility(true, true);
            HISM->SetComponentTickEnabled(false);
        }
    }
}

// ============================================================
//  Clear
// ============================================================

void UCC_CubeScatterComponent::ClearScatter()
{
    for (UHierarchicalInstancedStaticMeshComponent* HISM : HISMComponents)
    {
        if (HISM)
        {
            HISM->ClearInstances();
            HISM->DestroyComponent();
        }
    }

    HISMComponents.Empty();
    PlacedLocations.Empty();
    bGenerated = false;
}

// ============================================================
//  Private Helpers
// ============================================================

int32 UCC_CubeScatterComponent::PickMeshIndex(FRandomStream& RNG) const
{
    int32 MeshCount = ScatterMeshes.Num();
    if (MeshCount == 0) return 0;

    // 가중치 기반 선택
    if (MeshWeights.Num() == MeshCount)
    {
        int32 TotalWeight = 0;
        for (int32 W : MeshWeights) TotalWeight += FMath::Max(W, 1);

        int32 Roll = RNG.RandRange(0, TotalWeight - 1);
        int32 Cumulative = 0;
        for (int32 i = 0; i < MeshCount; ++i)
        {
            Cumulative += FMath::Max(MeshWeights[i], 1);
            if (Roll < Cumulative) return i;
        }
    }

    // 가중치 없으면 균등 분배
    return RNG.RandRange(0, MeshCount - 1);
}

bool UCC_CubeScatterComponent::IsLocationValid(const FVector& Candidate) const
{
    for (const FVector& Placed : PlacedLocations)
    {
        if (FVector::DistSquared(Candidate, Placed) < (MinDistance * MinDistance))
        {
            return false;
        }
    }
    return true;
}

UHierarchicalInstancedStaticMeshComponent* UCC_CubeScatterComponent::CreateHISM(UStaticMesh* Mesh)
{
    AActor* Owner = GetOwner();
    if (!Owner || !Mesh) return nullptr;

    UHierarchicalInstancedStaticMeshComponent* HISM =
        NewObject<UHierarchicalInstancedStaticMeshComponent>(Owner);

    HISM->SetStaticMesh(Mesh);
    HISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    HISM->SetMobility(EComponentMobility::Movable);
    HISM->SetCastShadow(true);
    HISM->PrimaryComponentTick.bCanEverTick = false;

    HISM->RegisterComponent();
    HISM->AttachToComponent(
        Owner->GetRootComponent(),
        FAttachmentTransformRules::KeepWorldTransform);

    return HISM;
}
