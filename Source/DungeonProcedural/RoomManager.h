// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DungeonProcedural/ConfigRoomDataAsset.h"
#include "Subsystems/WorldSubsystem.h"
#include "RoomManager.generated.h"

/**
 * 
 */
UCLASS()
class DUNGEONPROCEDURAL_API URoomManager : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable)
	void GenerateMap(int NbRoom, TArray<FRoomType> RoomTypes);
};
