// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_Cube.h"
#include "CC_Tile.h"
#include "CC_Freezable.h"
#include "../CC_CubeWorldManager.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Components/BoxComponent.h"


// Sets default values
ACC_Cube::ACC_Cube()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	// Floor Mesh (visual floor)
	FloorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FloorMesh"));
	FloorMesh->SetupAttachment(RootComponent);
	FloorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	CubeState = ECubeState::Unloaded;

	ScatterComponent = CreateDefaultSubobject<UCC_CubeScatterComponent>(TEXT("ScatterComponent"));
	ScatterComponent->SetOwnerCube(this);

}

// Called when the game starts or when spawned
void ACC_Cube::BeginPlay()
{
	Super::BeginPlay();

	//if (ScatterComponent)
	//{
	//	ScatterComponent->Generate(1);
	//}
}

// Called every frame
void ACC_Cube::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ACC_Cube::GenerateCube()
{
	if (!TileClass)
	{
		UE_LOG(LogTemp, Error, TEXT("Cube: TileClass not set!"));
		return;
	}

	// Remove existing tiles
	for (ACC_Tile* Tile : CubeTiles)
	{
		if (Tile)
		{
			Tile->Destroy();
		}
	}
	CubeTiles.Empty();

	// Generate 3x3 tiles
	for (int32 Row = 0; Row < 3; ++Row)
	{
		for (int32 Col = 0; Col < 3; ++Col)
		{
			int32 Index = Row * 3 + Col;

			// Tile position calculation
			float X = (Col - 1) * TileSize;  // -2000, 0, 2000
			float Y = (Row - 1) * TileSize;  // -2000, 0, 2000
			FVector TileCenter = GetActorLocation() + FVector(X, Y, 0);

			// Spawn tile
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			ACC_Tile* NewTile = GetWorld()->SpawnActor<ACC_Tile>(
				TileClass,
				TileCenter,
				FRotator::ZeroRotator,
				SpawnParams
			);

			if (NewTile)
			{
				NewTile->InitializeTile(Index, TileCenter, TileSize);
				CubeTiles.Add(NewTile);
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Cube: Generated 9 tiles in 3x3 grid"));
}

ACC_Tile* ACC_Cube::GetTileAt(int32 Index) const
{
	if (Index >= 0 && Index < CubeTiles.Num())
	{
		return CubeTiles[Index];
	}
	return nullptr;
}

int32 ACC_Cube::GetTileIndexAtPosition(const FVector& Position) const
{
	for (int32 i = 0; i < CubeTiles.Num(); ++i)
	{
		if (CubeTiles[i] && CubeTiles[i]->IsPositionInTile(Position))
		{
			return i;
		}
	}
	return -1;
}

void ACC_Cube::ActivateTilesAroundPlayer(int32 PlayerTileIndex)
{
	// Activate only the tile the player is currently on
	ActivateOnlyTile(PlayerTileIndex);
	CurrentPlayerTileIndex = PlayerTileIndex;
}

void ACC_Cube::ActivateOnlyTile(int32 TileIndex)
{
	for (int32 i = 0; i < CubeTiles.Num(); ++i)
	{
		if (CubeTiles[i])
		{
			if (i == TileIndex)
			{
				CubeTiles[i]->ActivateTile();
			}
			else
			{
				CubeTiles[i]->DeactivateTile();
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Cube: Activated tile %d only"), TileIndex);
}

void ACC_Cube::ActivateAllTiles()
{
	for (ACC_Tile* Tile : CubeTiles)
	{
		if (Tile)
		{
			Tile->ActivateTile();
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Cube: All tiles activated"));
}

void ACC_Cube::DeactivateAllTiles()
{
	for (ACC_Tile* Tile : CubeTiles)
	{
		if (Tile)
		{
			Tile->DeactivateTile();
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Cube: All tiles deactivated"));
}

// ========== Cube Wall Overlap Events ==========

void ACC_Cube::OnCubeWallHit_Right(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor) return;

	// Right wall hit: teleport to left side
	FVector CurrentPos = OtherActor->GetActorLocation();
	FVector NewPos = CurrentPos - FVector(CubeSize, 0, 0);
	OtherActor->SetActorLocation(NewPos);

	UE_LOG(LogTemp, Warning, TEXT("[CubeWall] %s: Right -> Left (X: %.1f -> %.1f)"),
		*OtherActor->GetName(), CurrentPos.X, NewPos.X);
}

void ACC_Cube::OnCubeWallHit_Left(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor) return;

	// Left wall hit: teleport to right side
	FVector CurrentPos = OtherActor->GetActorLocation();
	FVector NewPos = CurrentPos + FVector(CubeSize, 0, 0);
	OtherActor->SetActorLocation(NewPos);

	UE_LOG(LogTemp, Warning, TEXT("[CubeWall] %s: Left -> Right (X: %.1f -> %.1f)"),
		*OtherActor->GetName(), CurrentPos.X, NewPos.X);
}

void ACC_Cube::OnCubeWallHit_Top(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor) return;

	// Top wall hit: teleport to bottom side
	FVector CurrentPos = OtherActor->GetActorLocation();
	FVector NewPos = CurrentPos - FVector(0, CubeSize, 0);
	OtherActor->SetActorLocation(NewPos);

	UE_LOG(LogTemp, Warning, TEXT("[CubeWall] %s: Top -> Bottom (Y: %.1f -> %.1f)"),
		*OtherActor->GetName(), CurrentPos.Y, NewPos.Y);
}

void ACC_Cube::OnCubeWallHit_Bottom(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor) return;

	// Bottom wall hit: teleport to top side
	FVector CurrentPos = OtherActor->GetActorLocation();
	FVector NewPos = CurrentPos + FVector(0, CubeSize, 0);
	OtherActor->SetActorLocation(NewPos);

	UE_LOG(LogTemp, Warning, TEXT("[CubeWall] %s: Bottom -> Top (Y: %.1f -> %.1f)"),
		*OtherActor->GetName(), CurrentPos.Y, NewPos.Y);
}

void ACC_Cube::IndexToRowCol(int32 Index, int32& OutRow, int32& OutCol) const
{
	OutRow = Index / 3;
	OutCol = Index % 3;
}

int32 ACC_Cube::RowColToIndex(int32 Row, int32 Col) const
{
	return Row * 3 + Col;
}

void ACC_Cube::InitializeCubeWalls()
{
	float HalfCube = CubeSize / 2.0f;
	float HalfHeight = CubeHeight / 2.0f;
	float HalfThickness = WallThickness / 2.0f;

	// Right wall (X+) - Red
	CubeWall_Right = CreateDefaultSubobject<UBoxComponent>(TEXT("CubeWall_Right"));
	CubeWall_Right->SetupAttachment(RootComponent);
	CubeWall_Right->SetRelativeLocation(FVector(HalfCube + HalfThickness, 0, HalfHeight));
	CubeWall_Right->SetBoxExtent(FVector(HalfThickness, HalfCube, HalfHeight));
	CubeWall_Right->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CubeWall_Right->SetCollisionResponseToAllChannels(ECR_Ignore);
	CubeWall_Right->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CubeWall_Right->SetGenerateOverlapEvents(true);
	CubeWall_Right->ShapeColor = FColor::Red;

	// Left wall (X-) - Blue
	CubeWall_Left = CreateDefaultSubobject<UBoxComponent>(TEXT("CubeWall_Left"));
	CubeWall_Left->SetupAttachment(RootComponent);
	CubeWall_Left->SetRelativeLocation(FVector(-HalfCube - HalfThickness, 0, HalfHeight));
	CubeWall_Left->SetBoxExtent(FVector(HalfThickness, HalfCube, HalfHeight));
	CubeWall_Left->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CubeWall_Left->SetCollisionResponseToAllChannels(ECR_Ignore);
	CubeWall_Left->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CubeWall_Left->SetGenerateOverlapEvents(true);
	CubeWall_Left->ShapeColor = FColor::Blue;

	// Top wall (Y+) - Green
	CubeWall_Top = CreateDefaultSubobject<UBoxComponent>(TEXT("CubeWall_Top"));
	CubeWall_Top->SetupAttachment(RootComponent);
	CubeWall_Top->SetRelativeLocation(FVector(0, HalfCube + HalfThickness, HalfHeight));
	CubeWall_Top->SetBoxExtent(FVector(HalfCube, HalfThickness, HalfHeight));
	CubeWall_Top->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CubeWall_Top->SetCollisionResponseToAllChannels(ECR_Ignore);
	CubeWall_Top->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CubeWall_Top->SetGenerateOverlapEvents(true);
	CubeWall_Top->ShapeColor = FColor::Green;

	// Bottom wall (Y-) - Yellow
	CubeWall_Bottom = CreateDefaultSubobject<UBoxComponent>(TEXT("CubeWall_Bottom"));
	CubeWall_Bottom->SetupAttachment(RootComponent);
	CubeWall_Bottom->SetRelativeLocation(FVector(0, -HalfCube - HalfThickness, HalfHeight));
	CubeWall_Bottom->SetBoxExtent(FVector(HalfCube, HalfThickness, HalfHeight));
	CubeWall_Bottom->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CubeWall_Bottom->SetCollisionResponseToAllChannels(ECR_Ignore);
	CubeWall_Bottom->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CubeWall_Bottom->SetGenerateOverlapEvents(true);
	CubeWall_Bottom->ShapeColor = FColor::Yellow;
}

void ACC_Cube::InitializeCube(FIntPoint Coordinate)
{
	CubeCoordinate = Coordinate;
	CubeState = ECubeState::Active;

	UE_LOG(LogTemp, Log, TEXT("[Cube] Initializing at CubeSize is :(%f)"), CubeSize);

	// Set cube world location based on coordinate
	FVector CubeWorldLocation(
		-Coordinate.X * CubeSize,
		Coordinate.Y * CubeSize,
		0.0f
	);
	SetActorLocation(CubeWorldLocation);

	// Scale floor mesh to cube size
	if (FloorMesh)
	{
		FloorMesh->SetWorldScale3D(FVector(CubeSize / 100.0f, CubeSize / 100.0f, 1.0f));
	}

	// Create boundary triggers
	CreateBoundaryTriggers();

	//DrawDebugInfo();

	//UE_LOG(LogTemp, Log, TEXT("[Cube] Initialized at (%d, %d) - Location: %s"), Coordinate.X, Coordinate.Y, *CubeWorldLocation.ToString());

	if (ScatterComponent && !ScatterComponent->bGenerated)
	{
		ScatterComponent->ScatterRadius = CubeSize / 2.0f;
		UE_LOG(LogTemp, Warning, TEXT("[Cube %d,%d] ScatterRadius before Generate: %.1f"),
			Coordinate.X, Coordinate.Y, ScatterComponent->ScatterRadius);
		ScatterComponent->Generate(CubeCoordinate);
	}

}

void ACC_Cube::CreateBoundaryTriggers()
{
	BoundaryTriggers.Empty();
	BoundaryDirectionMap.Empty();

	float HalfSize = CubeSize / 2.0f;
	float TriggerThickness = 100.0f;
	float TriggerHeight = 500.0f;

	struct FBoundaryInfo
	{
		EBoundaryDirection Direction;
		FVector Location;
		FVector Extent;
		FName Name;
		FLinearColor Color;
	};

	TArray<FBoundaryInfo> Boundaries = {
		{ EBoundaryDirection::Up,    FVector(HalfSize,  0,         0), FVector(TriggerThickness, HalfSize,         TriggerHeight), TEXT("UpBoundary"),    FLinearColor::Red    },
		{ EBoundaryDirection::Down,  FVector(-HalfSize, 0,         0), FVector(TriggerThickness, HalfSize,         TriggerHeight), TEXT("DownBoundary"),  FLinearColor::Blue   },
		{ EBoundaryDirection::Left,  FVector(0,         -HalfSize, 0), FVector(HalfSize,         TriggerThickness, TriggerHeight), TEXT("LeftBoundary"),  FLinearColor::Green  },
		{ EBoundaryDirection::Right, FVector(0,         HalfSize,  0), FVector(HalfSize,         TriggerThickness, TriggerHeight), TEXT("RightBoundary"), FLinearColor::Yellow }
	};

	for (const FBoundaryInfo& Info : Boundaries)
	{
		UBoxComponent* Trigger = NewObject<UBoxComponent>(this, Info.Name);
		if (Trigger)
		{
			Trigger->RegisterComponent();
			Trigger->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
			Trigger->SetRelativeLocation(Info.Location);
			Trigger->SetBoxExtent(Info.Extent);
			Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
			Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
			Trigger->SetGenerateOverlapEvents(true);
			Trigger->ShapeColor = Info.Color.ToFColor(true);

			// Bind overlap event
			Trigger->OnComponentBeginOverlap.AddDynamic(this, &ACC_Cube::OnBoundaryOverlap);

			BoundaryTriggers.Add(Trigger);
			BoundaryDirectionMap.Add(Trigger, Info.Direction);

			UE_LOG(LogTemp, Log, TEXT("[Cube] Created boundary: %s at %s"),
				*Info.Name.ToString(), *Info.Location.ToString());
		}
	}
}

void ACC_Cube::SetBoundaryTriggersEnabled(bool bEnabled)
{
	for (UBoxComponent* Trigger : BoundaryTriggers)
	{
		if (!Trigger) continue;

		Trigger->SetGenerateOverlapEvents(bEnabled);
		Trigger->SetCollisionEnabled(
			bEnabled ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	}

	UE_LOG(LogTemp, Warning, TEXT("[Cube %d,%d] Boundary triggers %s"),
		CubeCoordinate.X, CubeCoordinate.Y,
		bEnabled ? TEXT("ENABLED") : TEXT("DISABLED"));
}

void ACC_Cube::RegisterActor(AActor* Actor)
{
	if (!Actor || ManagedActors.Contains(Actor))
		return;

	ManagedActors.Add(Actor);

	UE_LOG(LogTemp, Log, TEXT("[Cube %d,%d] Registered actor: %s (Total: %d)"),
		CubeCoordinate.X, CubeCoordinate.Y, *Actor->GetName(), ManagedActors.Num());
}

void ACC_Cube::UnregisterActor(AActor* Actor)
{
	if (!Actor)
		return;

	ManagedActors.Remove(Actor);

	UE_LOG(LogTemp, Log, TEXT("[Cube %d,%d] Unregistered actor: %s (Total: %d)"),
		CubeCoordinate.X, CubeCoordinate.Y, *Actor->GetName(), ManagedActors.Num());
}

bool ACC_Cube::IsActorInCube(AActor* Actor) const
{
	if (!Actor)
		return false;

	FVector ActorLocation = Actor->GetActorLocation();
	FBox CubeBounds = GetCubeBounds();

	return CubeBounds.IsInside(ActorLocation);
}

void ACC_Cube::Freeze()
{
	if (CubeState == ECubeState::Frozen)
		return;

	CubeState = ECubeState::Frozen;

	// Hide and disable the entire cube
	SetActorHiddenInGame(true);
	SetActorTickEnabled(false);

	//if (PCGComponent) PCGComponent->CleanupLocalGeneration(false, false);

	// Freeze all managed actors
	for (AActor* Actor : ManagedActors)
	{
		if (!Actor || Actor->IsPendingKillPending())
			continue;

		// Basic deactivation
		Actor->SetActorHiddenInGame(true);
		Actor->SetActorTickEnabled(false);

		// Actor implementing Freezable interface
		if (Actor->GetClass()->ImplementsInterface(UCC_Freezable::StaticClass()))
		{
			ICC_Freezable::Execute_Freeze(Actor);
		}
		else
		{
			// Default freeze handling for Characters
			if (ACharacter* Character = Cast<ACharacter>(Actor))
			{
				// Stop AI
				if (AAIController* AI = Cast<AAIController>(Character->GetController()))
				{
					AI->StopMovement();
					if (AI->BrainComponent)
					{
						AI->BrainComponent->StopLogic("Frozen");
					}
				}

				// Stop time dilation
				Character->CustomTimeDilation = 0.0f;

				// Pause animation
				if (USkeletalMeshComponent* Mesh = Character->GetMesh())
				{
					Mesh->bPauseAnims = true;
				}
			}

			// Stop physics simulation
			TArray<UActorComponent*> Components;
			Actor->GetComponents(UPrimitiveComponent::StaticClass(), Components);
			for (UActorComponent* Comp : Components)
			{
				if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Comp))
				{
					if (Prim->IsSimulatingPhysics())
					{
						Prim->SetSimulatePhysics(false);
					}
				}
			}
		}
	}

	if (ScatterComponent)
	{
		ScatterComponent->FreezeScatter();
	}

	UE_LOG(LogTemp, Warning, TEXT("[Cube %d,%d] FROZEN (%d actors)"),
		CubeCoordinate.X, CubeCoordinate.Y, ManagedActors.Num());
}

void ACC_Cube::Unfreeze()
{
	if (CubeState != ECubeState::Frozen)
		return;

	CubeState = ECubeState::Active;

	// Show and enable the entire cube
	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);

	//if (PCGComponent) PCGComponent->GenerateLocal(false);

	// Unfreeze all managed actors
	for (AActor* Actor : ManagedActors)
	{
		if (!Actor || Actor->IsPendingKillPending())
			continue;

		// Basic reactivation
		Actor->SetActorHiddenInGame(false);
		Actor->SetActorTickEnabled(true);

		// Actor implementing Freezable interface
		if (Actor->GetClass()->ImplementsInterface(UCC_Freezable::StaticClass()))
		{
			ICC_Freezable::Execute_Unfreeze(Actor);
		}
		else
		{
			// Default unfreeze handling for Characters
			if (ACharacter* Character = Cast<ACharacter>(Actor))
			{
				// Resume AI
				if (AAIController* AI = Cast<AAIController>(Character->GetController()))
				{
					if (AI->BrainComponent)
					{
						AI->BrainComponent->RestartLogic();
					}
				}

				// Resume time dilation
				Character->CustomTimeDilation = 1.0f;

				// Resume animation
				if (USkeletalMeshComponent* Mesh = Character->GetMesh())
				{
					Mesh->bPauseAnims = false;
				}
			}
		}
	}

	if (ScatterComponent)
	{
		if (!ScatterComponent->bGenerated)
		{
			ScatterComponent->Generate(CubeCoordinate);
		}
		else
		{
			ScatterComponent->UnfreezeScatter();
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[Cube %d,%d] UNFROZEN (%d actors)"),
		CubeCoordinate.X, CubeCoordinate.Y, ManagedActors.Num());
}

void ACC_Cube::OnBoundaryOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	UE_LOG(LogTemp, Warning, TEXT("[Cube %d,%d] Boundary overlap detected by %s"),
		CubeCoordinate.X, CubeCoordinate.Y,
		*OtherActor->GetName());

	if (IsFrozen())
		return;

	// Check if it is the player
	ACharacter* Player = Cast<ACharacter>(OtherActor);
	if (!Player || !Player->IsPlayerControlled())
		return;

	UBoxComponent* Trigger = Cast<UBoxComponent>(OverlappedComp);
	if (!Trigger || !BoundaryDirectionMap.Contains(Trigger))
		return;

	EBoundaryDirection Direction = BoundaryDirectionMap[Trigger];

	UE_LOG(LogTemp, Warning, TEXT("[Cube %d,%d] Player crossed boundary: %s"),
		CubeCoordinate.X, CubeCoordinate.Y,
		*UEnum::GetValueAsString(Direction));

	// Find CubeWorldManager and request transition
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACC_CubeWorldManager::StaticClass(), FoundActors);

	if (FoundActors.Num() > 0)
	{
		ACC_CubeWorldManager* Manager = Cast<ACC_CubeWorldManager>(FoundActors[0]);
		if (Manager)
		{
			Manager->RequestTransition(Direction);
		}
	}
}

FVector ACC_Cube::GetCubeCenter() const
{
	return GetActorLocation();
}

FBox ACC_Cube::GetCubeBounds() const
{
	FVector Center = GetCubeCenter();
	FVector HalfExtent(CubeSize / 2.0f);
	return FBox(Center - HalfExtent, Center + HalfExtent);
}

void ACC_Cube::DrawDebugInfo()
{
	if (!GetWorld())
		return;

	FColor Color = (CubeState == ECubeState::Active) ? FColor::Green : FColor::Red;

	//DrawDebugBox(GetWorld(), Bounds.GetCenter(), Bounds.GetExtent(),
	//	Color, false, -1.0f, 0, 50.0f);

	// Coordinate text
	FString CoordText = FString::Printf(TEXT("Cube [%d, %d]"), CubeCoordinate.X, CubeCoordinate.Y);
	DrawDebugString(GetWorld(), GetCubeCenter() + FVector(0, 0, 500),
		CoordText, nullptr, Color, -1.0f, true, 2.0f);

	for (UBoxComponent* Trigger : BoundaryTriggers)
	{
		if (!Trigger)
			continue;

		FVector TriggerLocation = Trigger->GetComponentLocation();
		FVector TriggerExtent = Trigger->GetScaledBoxExtent();
		FRotator TriggerRotation = Trigger->GetComponentRotation();
		FColor TriggerColor = Trigger->ShapeColor;

		DrawDebugBox(GetWorld(), TriggerLocation, TriggerExtent, TriggerRotation.Quaternion(),
			TriggerColor, false, 10.0f, 0, 5.0f);

		EBoundaryDirection* Direction = BoundaryDirectionMap.Find(Trigger);
		if (Direction)
		{
			FString DirectionText = UEnum::GetValueAsString(*Direction);
			// "EBoundaryDirection::Right" -> "Right"
			DirectionText.RemoveFromStart(TEXT("EBoundaryDirection::"));

			DrawDebugString(GetWorld(), TriggerLocation,
				DirectionText, nullptr, TriggerColor, -1.0f, true, 1.5f);
		}
	}
}
