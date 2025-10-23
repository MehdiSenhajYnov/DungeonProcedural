// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RoomParent.generated.h"

class UBoxComponent;

// Base class for all room types in the procedural dungeon
// Provides collision detection and serves as Blueprint parent for room variations
UCLASS()
class DUNGEONPROCEDURAL_API ARoomParent : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ARoomParent();

	// Box collision component for overlap detection and room spacing
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Room Parameters")
	UBoxComponent* BoxCollision;
};
