// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DungeonProcedural/RoomParent.h"
#include "Engine/DataAsset.h"
#include "ConfigRoomDataAsset.generated.h"

/**
 * 
 */
USTRUCT(Blueprintable, BlueprintType)
struct FRoomType
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon Setting")
	TSubclassOf<ARoomParent> TypeOfRoomToSpawn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon Setting")
	float Probability;

	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="Dungeon Setting")
	float SizeMin;

	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="Dungeon Setting")
	float SizeMax;
};

UCLASS(BlueprintType, Blueprintable)
class DUNGEONPROCEDURAL_API UConfigRoomDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	TArray<FRoomType> RoomTypes;
};
