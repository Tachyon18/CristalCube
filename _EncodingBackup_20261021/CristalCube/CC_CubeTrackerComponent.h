// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CC_CubeTrackerComponent.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTileChanged, int32, OldTileIndex, int32, NewTileIndex);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CRISTALCUBE_API UCC_CubeTrackerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCC_CubeTrackerComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ========== 설정 ==========

	/** 타일 체크 간격 (초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube Tracker")
	float CheckInterval = 0.2f;

	// ========== 상태 ==========

	/** 현재 타일 인덱스 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cube Tracker")
	int32 CurrentTileIndex = -1;

	/** Cube 참조 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cube Tracker")
	class ACC_Cube* Cube;

	// ========== 델리게이트 ==========

	/** 타일 변경 시 호출 */
	UPROPERTY(BlueprintAssignable, Category = "Cube Tracker")
	FOnTileChanged OnTileChanged;

	// ========== 함수 ==========

	/** 현재 타일 가져오기 */
	UFUNCTION(BlueprintCallable, Category = "Cube Tracker")
	class ACC_Tile* GetCurrentTile() const;

protected:

	void CheckTileBoundary();

	/** 타일 전환 처리 */
	void HandleTileTransition(int32 NewTileIndex);

	/** 타이머 핸들 */
	FTimerHandle CheckTimerHandle;
};
