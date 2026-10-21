// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_CubeTrackerComponent.h"
#include "Gameplay/CC_Cube.h"
#include "Gameplay/CC_Tile.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

// Sets default values for this component's properties
UCC_CubeTrackerComponent::UCC_CubeTrackerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UCC_CubeTrackerComponent::BeginPlay()
{
	Super::BeginPlay();

	// Cube 찾기
	TArray<AActor*> FoundCubes;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACC_Cube::StaticClass(), FoundCubes);

	if (FoundCubes.Num() > 0)
	{
		Cube = Cast<ACC_Cube>(FoundCubes[0]);

		if (Cube)
		{
			// 초기 타일 위치 확인
			AActor* Owner = GetOwner();
			if (Owner)
			{
				FVector Position = Owner->GetActorLocation();
				int32 InitialTileIndex = Cube->GetTileIndexAtPosition(Position);

				if (InitialTileIndex != -1)
				{
					CurrentTileIndex = InitialTileIndex;

					// 타일에 등록
					ACC_Tile* InitialTile = Cube->GetTileAt(InitialTileIndex);
					if (InitialTile)
					{
						InitialTile->RegisterActor(Owner);
					}

					UE_LOG(LogTemp, Log, TEXT("[CubeTracker] %s started at Tile %d"),
						*Owner->GetName(), CurrentTileIndex);
				}
			}

			// 타이머 시작
			GetWorld()->GetTimerManager().SetTimer(
				CheckTimerHandle,
				this,
				&UCC_CubeTrackerComponent::CheckTileBoundary,
				CheckInterval,
				true
			);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[CubeTracker] Failed to find Cube!"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[CubeTracker] No Cube found in level!"));
	}

}

void UCC_CubeTrackerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 타이머 정리
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(CheckTimerHandle);
	}

	// 현재 타일에서 해제
	if (Cube && CurrentTileIndex != -1)
	{
		ACC_Tile* CurrentTile = Cube->GetTileAt(CurrentTileIndex);
		if (CurrentTile)
		{
			CurrentTile->UnregisterActor(GetOwner());
		}
	}

	Super::EndPlay(EndPlayReason);
}

// Called every frame
void UCC_CubeTrackerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

ACC_Tile* UCC_CubeTrackerComponent::GetCurrentTile() const
{
	if (Cube && CurrentTileIndex != -1)
	{
		return Cube->GetTileAt(CurrentTileIndex);
	}
	return nullptr;
}

void UCC_CubeTrackerComponent::CheckTileBoundary()
{
	if (!Cube) return;

	AActor* Owner = GetOwner();
	if (!Owner) return;

	// 현재 위치의 타일 인덱스 확인
	FVector CurrentPosition = Owner->GetActorLocation();
	int32 NewTileIndex = Cube->GetTileIndexAtPosition(CurrentPosition);

	// 타일이 변경되었는지 확인
	if (NewTileIndex != -1 && NewTileIndex != CurrentTileIndex)
	{
		HandleTileTransition(NewTileIndex);
	}

	// 참고: 순환 이동(NewTileIndex == -1)은 CubeWall이 처리하므로 여기서는 신경 쓸 필요 없음!
}

void UCC_CubeTrackerComponent::HandleTileTransition(int32 NewTileIndex)
{
	if (!Cube) return;

	AActor* Owner = GetOwner();
	if (!Owner) return;

	int32 OldTileIndex = CurrentTileIndex;

	// 이전 타일에서 해제
	if (OldTileIndex != -1)
	{
		ACC_Tile* OldTile = Cube->GetTileAt(OldTileIndex);
		if (OldTile)
		{
			OldTile->UnregisterActor(Owner);
		}
	}

	// 새 타일에 등록
	ACC_Tile* NewTile = Cube->GetTileAt(NewTileIndex);
	if (NewTile)
	{
		NewTile->RegisterActor(Owner);
	}

	// 타일 인덱스 업데이트
	CurrentTileIndex = NewTileIndex;

	// 델리게이트 브로드캐스트
	OnTileChanged.Broadcast(OldTileIndex, NewTileIndex);

	UE_LOG(LogTemp, Log, TEXT("[CubeTracker] %s moved: Tile %d → Tile %d"),
		*Owner->GetName(), OldTileIndex, NewTileIndex);

	// Cube에 플레이어 위치 업데이트 알림 (플레이어인 경우)
	if (Owner->ActorHasTag("Player"))
	{
		Cube->ActivateTilesAroundPlayer(NewTileIndex);
	}
}
