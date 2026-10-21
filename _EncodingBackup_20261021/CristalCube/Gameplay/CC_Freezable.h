// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CC_Freezable.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, Blueprintable)
class UCC_Freezable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Freezable Interface
 * 큐브가 Frozen 상태가 될 때 Actor들이 구현해야 하는 인터페이스
 * Enemy, Projectile 등이 구현
 */
class CRISTALCUBE_API ICC_Freezable
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	// Time Clock (On Cube Freeze)

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Cube|Freeze")
	void Freeze();
	virtual void Freeze_Implementation() = 0;

	// Time Start Clock (On Cube Unfreeze)

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Cube|Freeze")
	void Unfreeze();
	virtual void Unfreeze_Implementation() = 0;

	// Check if Frozen

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Cube|Freeze")
	bool IsFrozen() const;
	virtual bool IsFrozen_Implementation() const = 0;
};
