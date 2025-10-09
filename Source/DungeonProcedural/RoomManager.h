// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DungeonProcedural/ConfigRoomDataAsset.h"
#include "DungeonProcedural/Triangle.h"
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
	UPROPERTY()
	TArray<ARoomParent*> SpawnedActors;

	
	UFUNCTION(BlueprintCallable)
	void GenerateMap(int NbRoom, TArray<FRoomType> RoomTypes);

	UFUNCTION(BlueprintCallable)
	void MegaTriangle(TSubclassOf<ARoomParent> Room);

	UFUNCTION(BlueprintCallable)
	void Triangulation(TSubclassOf<ARoomParent> RoomP);



	UPROPERTY()
	TArray<FTriangle> AllTriangles;

	// Fonction principale à appeler
	void ResolveRoomOverlaps(TArray<ARoomParent*>& SpawnedActors);

private:
	bool CheckOverlapping(const UBoxComponent* BoxA, const UBoxComponent* BoxB);
};
