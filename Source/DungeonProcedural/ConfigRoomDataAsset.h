// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DungeonProcedural/RoomParent.h"
#include "Engine/DataAsset.h"
#include "ConfigRoomDataAsset.generated.h"

// Configuration structure for defining room types in procedural generation
// Contains room class, spawn probability, and size variation parameters
// Defines a room type with its properties for procedural generation
USTRUCT(Blueprintable, BlueprintType)
struct FRoomType
{
	GENERATED_BODY()
	
	// Blueprint class to spawn for this room type
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon Setting")
	TSubclassOf<ARoomParent> TypeOfRoomToSpawn;

	// Weighted probability for room selection (higher = more likely)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon Setting")
	float Probability;

	// Minimum scale multiplier for room size variation
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="Dungeon Setting")
	float SizeMin;

	// Maximum scale multiplier for room size variation
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="Dungeon Setting")
	float SizeMax;
};

// Data Asset for configuring room generation parameters
// Stores array of room types with their spawn settings
UCLASS(BlueprintType, Blueprintable)
class DUNGEONPROCEDURAL_API UConfigRoomDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	// Array of available room types for procedural generation
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	TArray<FRoomType> RoomTypes;
};
