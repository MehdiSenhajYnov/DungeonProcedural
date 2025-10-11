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

	UPROPERTY()
	TArray<AActor*> OtherActorsToClear;
	
	UFUNCTION(BlueprintCallable)
	void GenerateMap(int NbRoom, TArray<FRoomType> RoomTypes);

	UFUNCTION(BlueprintCallable)
	void MegaTriangle(TSubclassOf<ARoomParent> Room);

	UFUNCTION(BlueprintCallable)
	void Triangulation(TSubclassOf<ARoomParent> RoomP);

	UFUNCTION(BlueprintCallable)
	void TriangulationStepByStep(TSubclassOf<ARoomParent> RoomP);

	UFUNCTION(BlueprintCallable)
	void TriangulationLittleStep(TSubclassOf<ARoomParent> RoomP);
	
	UPROPERTY()
	TArray<FTriangle> TriangleErased;
	UPROPERTY()
	TArray<FTriangle> LastTrianglesCreated;

	UPROPERTY()
	TArray<FTriangle> AllTriangles;

	// Fonction principale à appeler
	void ResolveRoomOverlaps(TArray<ARoomParent*>& SpawnedActors);


	void DrawAll();
	void ClearDrawAll();

	UFUNCTION(BlueprintCallable)
	void ClearAll();
private:
	bool CheckOverlapping(const UBoxComponent* BoxA, const UBoxComponent* BoxB);

	int CurrentStep = 0;
	int CurrentLittleStep = 0;
	
	// Stocker les positions du méga-triangle pour pouvoir les supprimer à la fin
	FVector MegaTrianglePointA;
	FVector MegaTrianglePointB;
	FVector MegaTrianglePointC;
	
	void RemoveSuperTriangles();
	
};


struct ToDrawCircle
{
	ToDrawCircle() = default;

	ToDrawCircle(const FVector& Center, float Radius)
		: Center(Center),
		  radius(Radius)
	{
	}

	FVector Center;
	float radius;
	
};

